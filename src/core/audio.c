/* sound: 8 voices mixed to 48 kHz 16-bit stereo, the same on every platform. A voice plays sound data
   from the app's own memory as it goes (8- or 16-bit signed, or 4-bit IMA ADPCM; mono or stereo; at
   any rate), with its volume, rate and pan changeable while it plays, and optionally a loop. Linear
   interpolation between samples. The platform asks for samples (rinc_audioMix) from wherever its
   output wants them, and brackets changes from host procs with its own lock (rinc_audioLock).

   ADPCM is IMA's, laid out as in WAV files (format 0x11): blocks that each start with every channel's
   first sample and step index, then 4-bit codes (mono: in byte order, low half first; stereo: 4 bytes,
   8 codes, of each channel in turn). A voice decodes it as it plays, one frame at a time; a block is
   where decoding can start over, so going back (a loop) starts at a block, and soundLoop keeps the
   decoder's state at the loop's start so wrapping around costs nothing. A quarter the size of 16-bit,
   and close to it in sound.

   Or a voice is the synth's (pc.synth): it plays notes from the app's memory, 4 bytes each (pitch,
   wave, volume, effect: Sound's notes), making the samples as it goes at its own rate (16 kHz, Sound's,
   unless set), and interpolated up like any sound's. */
#include <math.h>
#include <string.h>
#include "rinc.h"
#include "pc_host.h" /* (pc.rio's structs and sigs, as C: rio -H src/core/pc_host.h src/core/pc.rio) */

/* ADPCM decoding, one frame at a time: at frame at, each channel's value there (a) and at the next
   frame (b); blk and k: where the next frame is (its block, and which frame of it); pred and index:
   the decoder's state after it */
typedef struct { uint32_t at, k; const uint8_t *blk; int32_t a[2], b[2], pred[2]; uint8_t index[2]; } rinc_AdpcmState;
typedef struct {
  const uint8_t *data; uint32_t frames; /* the sound, in frames (a sample, or a stereo pair) */
  uint32_t pos, frac, step, fstep;      /* where (a frame, and 32 bits of fraction), and how far each output frame */
  uint32_t loop0, loop1;                /* loop1 > loop0: play [loop0, loop1) over and over */
  int32_t gl, gr;                       /* gains, 1.15 (32768 is 1): volume with pan */
  float vol, pan;
  uint8_t bits, stereo, on;             /* bits: 8, 16 or 4 (ADPCM) */
  uint32_t block, per;                  /* ADPCM: bytes in a block, and frames */
  rinc_AdpcmState dec, at0;                         /* ADPCM: the decoder, and its state at loop0 */
  /* the synth's (frames: its notes; loop0 and loop1 in notes): the note it's on, sample k of its len,
     the wave's place in its cycle (t, and phaser's second wave's t2) and how far a sample (dt, for
     pitch), the volume as heard (eased toward the note's, so changes don't click), seconds since it
     began (for vibrato and arpeggios), the noise's state, samples left to fade out in once it ends,
     and the samples it's between. From its settings (rinc_SynthSet): its rate and soundRate's speed-up
     (play), a second's part of a sample (inv), its speed, how far the volume eases a sample, rising and
     falling (up, down), the fade's length, and its bend and smoothing. The gains as heard (cgl, cgr),
     eased toward gl and gr over a chunk, so volume and pan don't click */
  const uint8_t *notes; uint32_t note, k, len, eighth, noise, tail, speed, fade;
  float t, t2, dt, pitch, level, sec, rate, play, inv, up, down, bend;
  int32_t sa, sb, cgl, cgr; uint8_t synth, smooth, set;
} rinc_Voice;

static rinc_Voice rinc_voices[rinc_VOICES], rinc_maker; /* (rinc_maker: synthMake's) */
static volatile int32_t rinc_master = 32768; /* the system's volume, over all of it (1.15) */
static int rinc_level = rinc_VOLUME_STEPS, rinc_muted;
void (*rinc_audioLock)(int on);
static void rinc_lock(int on) { if (rinc_audioLock) rinc_audioLock(on); }

static void rinc_gains(rinc_Voice *v) { /* balance: the center is full on both sides */
  float l = v->pan > 0 ? 1 - v->pan : 1, r = v->pan < 0 ? 1 + v->pan : 1;
  v->gl = (int32_t)(v->vol * l * 32768.0f); v->gr = (int32_t)(v->vol * r * 32768.0f);
}
static float rinc_clampf(float x, float lo, float hi) { return x != x ? lo : x < lo ? lo : x > hi ? hi : x; }
static void rinc_stepFor(rinc_Voice *v, float rate) {
  uint64_t s = (uint64_t)((double)rinc_clampf(rate, 0, rinc_RATE * 16.0f) / rinc_RATE * 4294967296.0);
  v->step = (uint32_t)(s >> 32); v->fstep = (uint32_t)s;
}

/* IMA ADPCM */
static const int16_t rinc_steps[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
  73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894,
  6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int8_t rinc_moves[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
/* channel c of the frame at (blk, k), into the decoder's state */
static int32_t rinc_HOT rinc_adpcmFrame(const rinc_Voice *v, rinc_AdpcmState *d, int c) {
  const uint8_t *b = d->blk; uint32_t k = d->k, step; int32_t diff, p; int code, x;
  if (!k) { /* a block's header: the sample itself, and the step index */
    d->pred[c] = (int16_t)(b[c * 4] | b[c * 4 + 1] << 8); d->index[c] = b[c * 4 + 2] > 88 ? 88 : b[c * 4 + 2];
    return d->pred[c];
  }
  k--; /* (which code) */
  code = (v->stereo ? b[8 + (k >> 3) * 8 + (uint32_t)c * 4 + (k & 7) / 2] : b[4 + k / 2]) >> (k & 1) * 4 & 15;
  step = (uint32_t)rinc_steps[d->index[c]];
  diff = (int32_t)(step >> 3);
  if (code & 1) diff += (int32_t)(step >> 2);
  if (code & 2) diff += (int32_t)(step >> 1);
  if (code & 4) diff += (int32_t)step;
  p = d->pred[c] + (code & 8 ? -diff : diff);
  d->pred[c] = p > 32767 ? 32767 : p < -32768 ? -32768 : p;
  x = d->index[c] + rinc_moves[code & 7]; d->index[c] = (uint8_t)(x < 0 ? 0 : x > 88 ? 88 : x);
  return d->pred[c];
}
/* the decoder a frame on */
static void rinc_HOT rinc_adpcmNext(const rinc_Voice *v, rinc_AdpcmState *d) {
  int c, n = v->stereo ? 2 : 1;
  d->at++;
  if (++d->k == v->per) { d->k = 0; d->blk += v->block; }
  for (c = 0; c < n; c++) { d->a[c] = d->b[c]; if (d->at + 1 < v->frames) d->b[c] = rinc_adpcmFrame(v, d, c); }
}
/* the decoder at frame i: from i's block if it's past i, then forward */
static void rinc_HOT rinc_adpcmTo(const rinc_Voice *v, rinc_AdpcmState *d, uint32_t i) {
  if (d->at > i || d->at == 0xFFFFFFFFu) {
    int c, n = v->stereo ? 2 : 1;
    d->at = i - i % v->per; d->k = 0; d->blk = v->data + i / v->per * v->block;
    for (c = 0; c < n; c++) d->a[c] = d->b[c] = rinc_adpcmFrame(v, d, c);
    if (d->at + 1 < v->frames) { d->k = 1; for (c = 0; c < n; c++) d->b[c] = rinc_adpcmFrame(v, d, c); }
    else d->k = 1; /* (a one-frame sound) */
  }
  while (d->at < i) rinc_adpcmNext(v, d);
}
/* frames in n bytes of ADPCM (blocks of block bytes, the last maybe short) */
static uint32_t rinc_adpcmFrames(uint32_t n, uint32_t block, int stereo) {
  uint32_t ch = stereo ? 2 : 1, per = (block / ch - 4) * 2 + 1, r = n % block, last = 0;
  if (r >= 4 * ch) last = stereo ? 1 + (r - 4 * ch) / 8 * 8 : 1 + (r - 4) * 2;
  return n / block * per + last;
}

/* what's mixed, a chunk at a time: each frame's left and right, summed */
#define rinc_CHUNK 64
static int32_t rinc_sums[rinc_CHUNK * 2];

/* the synth: notes made into samples (docs/spec/sound.md#the-synth), as it plays them: each note speed
   120ths of a second (133 samples a speed at 16 kHz, Sound's rate), C0 (65.4 Hz) at pitch 0 */
#define rinc_SYNTH_RATE 16000
#define rinc_SYNTH_TICK 133
#define rinc_SYNTH_EASE 0.0062187f /* seconds the volume takes to ease about 2/3 of the way: 1% a sample at 16 kHz */
#define rinc_SYNTH_PEAK 10240.0f /* a full-volume wave's peak: 8 dB under 25600, since a synth wave's peak is near its
                                    average, so it's as loud as recorded sound at the same volume (notes/native-synth.md) */
/* a voice's settings, kept from one pc.synth to the next (and synthMake's, after the voices): the rate
   it makes samples at, a bend in semitones, the volume's easing (rising, falling), smoothing on or off */
typedef struct { float rate, bend, attack, release; uint8_t smooth; } rinc_SynthSet;
static rinc_SynthSet rinc_synthSets[rinc_VOICES + 1];
/* waves 8 to 15: a cycle each, of 16-bit samples in the app's memory (none: silent) */
static struct { const uint8_t *data; uint32_t n; } rinc_waves[8];
static rinc_SynthSet *rinc_synthSet(int i) {
  rinc_SynthSet *s = &rinc_synthSets[i];
  if (!s->rate) { s->rate = rinc_SYNTH_RATE; s->attack = s->release = rinc_SYNTH_EASE; }
  return s;
}
static float rinc_tri(float x) { return fabsf(x * 4 - 2) - 1; } /* (x from 0 to 1, a cycle) */
static float rinc_fracf(float x) { return x - floorf(x); }
/* PolyBLEP: what rounds off a jump up of 2 at the cycle's start (x), over a sample's step (dt) either
   side of it, so the jump's overtones past what the rate can hold mostly don't fold back as false tones */
static float rinc_HOT rinc_blep(float x, float dt) {
  if (x < dt) { x /= dt; return x + x - x * x - 1; }
  if (x > 1 - dt) { x = (x - 1) / dt; return x * x + x + x + 1; }
  return 0;
}
static float rinc_HOT rinc_wave(rinc_Voice *v, int w) {
  float x = v->t, d = v->dt < 0.5f ? v->dt : 0.5f;
  if (v->smooth) switch (w & 15) { /* (the waves that jump, smoothed) */
  case 2: return (x * 2 - 1 - rinc_blep(x, d)) * 0.7f;
  case 3: return (x < 0.5f ? 0.5f : -0.5f) + 0.5f * (rinc_blep(x, d) - rinc_blep(rinc_fracf(x + 0.5f), d));
  case 4: return (x < 0.25f ? 0.5f : -0.5f) + 0.5f * (rinc_blep(x, d) - rinc_blep(rinc_fracf(x + 0.75f), d));
  }
  switch (w & 15) {
  case 1: return x < 0.875f ? x / 0.875f * 2 - 1 : (1 - x) / 0.125f * 2 - 1; /* tilted */
  case 2: return (x * 2 - 1) * 0.7f;                                         /* saw */
  case 3: return x < 0.5f ? 0.5f : -0.5f;                                    /* square */
  case 4: return x < 0.25f ? 0.5f : -0.5f;                                   /* pulse */
  case 5: return (rinc_tri(x) + rinc_tri(rinc_fracf(x * 2)) * 0.5f) * 0.67f; /* organ: and its octave */
  case 6: {                                                                  /* noise: 8 values a cycle */
    uint32_t e = (uint32_t)(x * 8);
    if (e != v->eighth) { v->eighth = e; v->noise = v->noise * 1664525u + 1013904223u; }
    return (float)(int32_t)v->noise / 2147483648.0f;
  }
  case 7: return (rinc_tri(x) + rinc_tri(v->t2)) * 0.5f;                     /* phaser: two a little apart */
  case 0: return rinc_tri(x);                                                /* triangle */
  default: {                                                                 /* the app's own */
    const uint8_t *p = rinc_waves[(w & 15) - 8].data; uint32_t n = rinc_waves[(w & 15) - 8].n, i, j;
    float at, a, b;
    if (!p) return 0;
    at = x * (float)n; i = (uint32_t)at; if (i >= n) i = n - 1;
    j = i + 1 < n ? i + 1 : 0;
    a = (float)(int16_t)(p[i * 2] | p[i * 2 + 1] << 8); b = (float)(int16_t)(p[j * 2] | p[j * 2 + 1] << 8);
    return (a + (b - a) * (at - (float)i)) / 32768.0f;
  }
  }
}
static float rinc_pitchOf(const uint8_t *n) { return n[0] > 95 ? 95.0f : (float)n[0]; }
/* the frequency at pitch p (0 to 95; past them, the nearer): from a table of the semitones, in a
   straight line between them (within a cent of 2^(p / 12), for much less than working that out each
   sample) */
static float rinc_semitones[97];
static float rinc_HOT rinc_hzOf(float p) {
  int i;
  if (!(p > 0)) return rinc_semitones[0];
  i = (int)p;
  if (i >= 95) return rinc_semitones[95];
  return rinc_semitones[i] + (rinc_semitones[i + 1] - rinc_semitones[i]) * (p - (float)i);
}
/* the next sample (16-bit), the synth a sample on; at its end, off */
static int32_t rinc_HOT rinc_synthNext(rinc_Voice *v) {
  const uint8_t *n;
  float p, target, u, s;
  int e;
  if (v->k >= v->len && !v->tail) { /* the next note, or back to the loop's start, or the end: fading out */
    if (v->loop1 > v->loop0 && v->note + 1 >= v->loop1) v->note = v->loop0, v->k = 0;
    else if (v->note + 1 < v->frames) v->note++, v->k = 0;
    else v->tail = v->fade;
  }
  if (v->tail && !--v->tail) { v->on = 0; return 0; }
  n = v->notes + v->note * 4;
  p = rinc_pitchOf(n); target = (float)(n[2] & 7) / 7; e = n[3] & 7;
  u = v->k < v->len ? (float)v->k / (float)v->len : 1;
  switch (e) {
  case 1: { /* slide: from the note before, its pitch and volume */
    const uint8_t *m = v->notes + (v->note ? v->note - 1 : 0) * 4;
    float p0 = rinc_pitchOf(m), v0 = (float)(m[2] & 7) / 7;
    p = p0 + (p - p0) * u; target = v0 + (target - v0) * u;
    break;
  }
  case 2: p += 0.5f * rinc_tri(rinc_fracf(v->sec * 7.5f)); break; /* vibrato: a quarter tone either way */
  case 3: p *= 1 - u; break;                                      /* drop */
  case 4: target *= u; break;                                     /* fade in, and out */
  case 5: target *= 1 - u; break;
  case 6: case 7: { /* an arpeggio: the four notes of its group in turn, 30 a second (fast) or 15 */
    uint32_t g = v->note - v->note % 4 + (uint32_t)(v->sec * (e == 6 ? 30.0f : 15.0f)) % 4;
    p = rinc_pitchOf(v->notes + (g < v->frames ? g : v->note) * 4);
    break;
  }
  }
  if (v->tail) target = 0;
  p += v->bend;
  if (p != v->pitch) { v->pitch = p; v->dt = rinc_hzOf(p) * v->inv; }
  v->t += v->dt; v->t -= (float)(int)v->t;
  v->t2 += v->dt * 1.007f; v->t2 -= (float)(int)v->t2;
  s = rinc_wave(v, n[1]);
  v->level += (target - v->level) * (target > v->level ? v->up : v->down);
  v->k++;
  if ((v->sec += v->inv) >= 4) v->sec -= 4; /* (every effect's period divides 4 seconds) */
  return (int32_t)(s * v->level * rinc_SYNTH_PEAK);
}
/* how far the volume eases a sample, to get about 2/3 of the way in secs at rate (0: all the way) */
static float rinc_ease(float secs, float rate) { return secs > 0 ? 1 - expf(-1 / (secs * rate)) : 1; }
/* a synth voice's settings, into it (a note partway along stays as far along at a new rate) */
static void rinc_synthApply(rinc_Voice *v) {
  const rinc_SynthSet *s = rinc_synthSet(v->set);
  float r = s->rate, f = r / 20, ff = 5 * s->release * r;
  uint32_t len = (uint32_t)((float)(v->speed * rinc_SYNTH_TICK) * r / rinc_SYNTH_RATE + 0.5f);
  if (!len) len = 1;
  if (v->len && len != v->len) v->k = (uint32_t)((uint64_t)v->k * len / v->len);
  v->len = len;
  v->rate = r; v->inv = 1 / r; v->pitch = -1000; /* (no pitch: its step worked out again) */
  v->up = rinc_ease(s->attack, r); v->down = rinc_ease(s->release, r);
  v->fade = (uint32_t)(ff > f ? ff : f); /* (at least 50 ms, and long enough for the release) */
  if (v->tail > v->fade) v->tail = v->fade;
  v->bend = s->bend; v->smooth = s->smooth;
  rinc_stepFor(v, r * v->play);
}
/* a voice starting on notes (count of them) at speed, with settings set (a voice, or synthMake's) */
static void rinc_synthStart(rinc_Voice *v, const uint8_t *notes, uint32_t count, int speed, int set) {
  int k;
  if (!rinc_semitones[1]) for (k = 0; k < 97; k++) rinc_semitones[k] = 65.406f * exp2f((float)k / 12);
  memset(v, 0, sizeof *v);
  v->synth = 1; v->notes = notes; v->frames = count; v->set = (uint8_t)set;
  v->speed = (uint32_t)(speed < 1 ? 1 : speed > 255 ? 255 : speed);
  v->noise = 1; v->play = 1;
  rinc_synthApply(v);
  v->vol = 1; rinc_gains(v); v->cgl = v->gl; v->cgr = v->gr;
  v->on = 1;
  v->sb = rinc_synthNext(v);
}
/* a synth voice into the sums: the samples made as they're reached, interpolated between; its gains
   eased across the chunk to where they're set */
static void rinc_HOT rinc_mixSynth(rinc_Voice *v, int n) {
  uint32_t fr = v->frac, fstep = v->fstep, step = v->step;
  int32_t gl = v->cgl, gr = v->cgr, dl = (v->gl - gl) / n, dr = (v->gr - gr) / n, *out = rinc_sums, a = v->sa, b = v->sb;
  int f;
  for (f = 0; f < n && v->on; f++, out += 2, gl += dl, gr += dr) {
    int32_t x = a + ((b - a) * (int32_t)(fr >> 17) >> 15);
    uint32_t go;
    out[0] += x * gl >> 15; out[1] += x * gr >> 15;
    fr += fstep;
    for (go = step + (fr < fstep); go && v->on; go--) { a = b; b = rinc_synthNext(v); }
  }
  v->frac = fr; v->sa = a; v->sb = b;
  v->cgl = f == n ? v->gl : gl; v->cgr = f == n ? v->gr : gr;
}

/* one voice into the sums for n frames, as it goes: where it is, its loop, its next frame (to
   interpolate toward), its gains */
static void rinc_HOT rinc_mixVoice(rinc_Voice *v, int n) {
  uint32_t i = v->pos, fr = v->frac, step = v->step, fstep = v->fstep, loop0 = v->loop0, loop1 = v->loop1;
  uint32_t end = loop1 > loop0 ? loop1 : v->frames;
  int32_t gl = v->gl, gr = v->gr, *out = rinc_sums; int f, looping = loop1 > loop0, bits = v->bits, stereo = v->stereo;
  const uint8_t *data = v->data;
  for (f = 0; f < n; f++, out += 2) {
    int32_t t, a, b, l, r; uint32_t j;
    if (i >= end) {
      if (!looping) { v->on = 0; break; }
      do i -= loop1 - loop0; while (i >= end);
      if (bits == 4 && i < v->dec.at) v->dec = v->at0; /* (wrapped around: the decoder from the loop's start) */
    }
    j = i + 1 < end ? i + 1 : looping ? loop0 : i; /* the next frame, for interpolating */
    t = (int32_t)(fr >> 17); /* (15 bits: (b - a) * t stays in 32) */
    if (bits == 16) {
      const uint8_t *p = data + i * (stereo ? 4u : 2u), *q = data + j * (stereo ? 4u : 2u);
      a = (int16_t)(p[0] | p[1] << 8); b = (int16_t)(q[0] | q[1] << 8); l = a + ((b - a) * t >> 15);
      if (stereo) { a = (int16_t)(p[2] | p[3] << 8); b = (int16_t)(q[2] | q[3] << 8); r = a + ((b - a) * t >> 15); } else r = l;
    } else if (bits == 8) {
      const int8_t *p = (const int8_t *)data + i * (stereo ? 2u : 1u), *q = (const int8_t *)data + j * (stereo ? 2u : 1u);
      a = p[0] * 256; b = q[0] * 256; l = a + ((b - a) * t >> 15);
      if (stereo) { a = p[1] * 256; b = q[1] * 256; r = a + ((b - a) * t >> 15); } else r = l;
    } else { /* ADPCM: frame i, and the next (at a loop's end, its start: kept since soundLoop) */
      rinc_AdpcmState *d = &v->dec;
      if (d->at != i) rinc_adpcmTo(v, d, i);
      a = d->a[0]; b = j == i + 1 ? d->b[0] : j == i ? a : v->at0.a[0]; l = a + ((b - a) * t >> 15);
      if (stereo) { a = d->a[1]; b = j == i + 1 ? d->b[1] : j == i ? a : v->at0.a[1]; r = a + ((b - a) * t >> 15); } else r = l;
    }
    out[0] += l * gl >> 15; out[1] += r * gr >> 15;
    fr += fstep; i += step + (fr < fstep); /* (the fraction's carry) */
  }
  v->pos = i; v->frac = fr;
}
/* the common cases, with nothing in the way: mono 16-bit, and mono ADPCM (its decoder in locals) */
static void rinc_HOT rinc_mixPcm16(rinc_Voice *v, int n) {
  uint32_t i = v->pos, fr = v->frac, step = v->step, fstep = v->fstep, loop0 = v->loop0, loop1 = v->loop1;
  uint32_t end = loop1 > loop0 ? loop1 : v->frames;
  int32_t gl = v->gl, gr = v->gr, *out = rinc_sums; int f, looping = loop1 > loop0;
  const int16_t *data = (const int16_t *)(const void *)v->data; /* (data from rio_sliceBytes: 2-byte aligned for 16-bit) */
  for (f = 0; f < n; f++, out += 2) {
    int32_t a, b, x; uint32_t j;
    if (i >= end) {
      if (!looping) { v->on = 0; break; }
      do i -= loop1 - loop0; while (i >= end);
    }
    j = i + 1 < end ? i + 1 : looping ? loop0 : i;
    a = data[i]; b = data[j]; x = a + ((b - a) * (int32_t)(fr >> 17) >> 15);
    out[0] += x * gl >> 15; out[1] += x * gr >> 15;
    fr += fstep; i += step + (fr < fstep);
  }
  v->pos = i; v->frac = fr;
}
static void rinc_HOT rinc_mixAdpcm1(rinc_Voice *v, int n) {
  uint32_t i = v->pos, fr = v->frac, step = v->step, fstep = v->fstep, loop0 = v->loop0, loop1 = v->loop1;
  uint32_t end = loop1 > loop0 ? loop1 : v->frames, frames = v->frames, per = v->per, block = v->block;
  int32_t gl = v->gl, gr = v->gr, *out = rinc_sums; int f, looping = loop1 > loop0;
  uint32_t at = v->dec.at, k = v->dec.k; const uint8_t *blk = v->dec.blk; int32_t a = v->dec.a[0], b = v->dec.b[0], pred = v->dec.pred[0];
  int index = v->dec.index[0];
  for (f = 0; f < n; f++, out += 2) {
    int32_t x, bb; uint32_t j;
    if (i >= end) {
      if (!looping) { v->on = 0; break; }
      do i -= loop1 - loop0; while (i >= end);
    }
    if (at != i) {
      if (at > i) { /* (back: to the loop's start, or a block) */
        rinc_AdpcmState *d = &v->dec;
        if (looping && i >= loop0 && v->at0.at <= i) *d = v->at0; else d->at = 0xFFFFFFFFu;
        rinc_adpcmTo(v, d, i);
        at = d->at; k = d->k; blk = d->blk; a = d->a[0]; b = d->b[0]; pred = d->pred[0]; index = d->index[0];
      }
      while (at < i) { /* a frame on: decode the one after */
        at++; a = b;
        if (++k == per) { k = 0; blk += block; }
        if (at + 1 < frames) {
          if (!k) { pred = (int16_t)(blk[0] | blk[1] << 8); index = blk[2] > 88 ? 88 : blk[2]; }
          else {
            uint32_t c = (uint32_t)(blk[4 + (k - 1) / 2] >> ((k - 1) & 1) * 4 & 15), st = (uint32_t)rinc_steps[index];
            int32_t diff = (int32_t)(st >> 3);
            if (c & 1) diff += (int32_t)(st >> 2);
            if (c & 2) diff += (int32_t)(st >> 1);
            if (c & 4) diff += (int32_t)st;
            pred += c & 8 ? -diff : diff;
            pred = pred > 32767 ? 32767 : pred < -32768 ? -32768 : pred;
            index += rinc_moves[c & 7]; index = index < 0 ? 0 : index > 88 ? 88 : index;
          }
          b = pred;
        }
      }
    }
    j = i + 1 < end ? i + 1 : looping ? loop0 : i;
    bb = j == i + 1 ? b : j == i ? a : v->at0.a[0];
    x = a + ((bb - a) * (int32_t)(fr >> 17) >> 15);
    out[0] += x * gl >> 15; out[1] += x * gr >> 15;
    fr += fstep; i += step + (fr < fstep);
  }
  v->pos = i; v->frac = fr;
  v->dec.at = at; v->dec.k = k; v->dec.blk = blk; v->dec.a[0] = a; v->dec.b[0] = b; v->dec.pred[0] = pred; v->dec.index[0] = (uint8_t)index;
}
void rinc_HOT rinc_audioMix(int16_t *out, int frames) {
  while (frames > 0) {
    int n = frames < rinc_CHUNK ? frames : rinc_CHUNK, k, f;
    memset(rinc_sums, 0, (size_t)n * 2 * sizeof rinc_sums[0]);
    for (k = 0; k < rinc_VOICES; k++) {
      rinc_Voice *v = &rinc_voices[k];
      if (!v->on) continue;
      if (v->synth) rinc_mixSynth(v, n);
      else if (v->stereo) rinc_mixVoice(v, n);
      else if (v->bits == 4) rinc_mixAdpcm1(v, n);
      else if (v->bits == 16 && !((uintptr_t)v->data & 1)) rinc_mixPcm16(v, n);
      else rinc_mixVoice(v, n);
    }
    if (rinc_master != 32768) { int32_t m = rinc_master; for (f = 0; f < n * 2; f++) rinc_sums[f] = (int32_t)((int64_t)rinc_sums[f] * m >> 15); }
    for (f = 0; f < n * 2; f++) out[f] = (int16_t)(rinc_sums[f] < -32768 ? -32768 : rinc_sums[f] > 32767 ? 32767 : rinc_sums[f]);
    out += n * 2; frames -= n;
  }
}

/* the system's volume (a keyboard's volume knob, say; not the app's to change): a step up or down,
   2.5 dB each, from full down to -40 dB; 0 turns mute on or off */
void rinc_audioVolume(int step) {
  if (!step) rinc_muted = !rinc_muted;
  else rinc_level = rinc_level + step < 0 ? 0 : rinc_level + step > rinc_VOLUME_STEPS ? rinc_VOLUME_STEPS : rinc_level + step;
  rinc_master = rinc_muted ? 0 : (int32_t)(32768.0f * powf(10.0f, -(float)(rinc_VOLUME_STEPS - rinc_level) * 2.5f / 20.0f) + 0.5f);
}
int rinc_audioLevel(void) { return rinc_muted ? -1 : rinc_level; }

void rinc_audioReset(void) {
  rinc_lock(1);
  memset(rinc_voices, 0, sizeof rinc_voices); memset(rinc_synthSets, 0, sizeof rinc_synthSets); memset(rinc_waves, 0, sizeof rinc_waves); memset(&rinc_maker, 0, sizeof rinc_maker);
  rinc_lock(0);
}

/* host procs. sound(voice, data, bits, channels, rate): play data (8 or 16 bits, or 4 for ADPCM in
   blocks of rinc_ADPCM_BLOCK bytes a channel; 1 or 2 channels) at rate frames a second, full volume,
   centered, once. A voice out of 0-7, or bad data, does nothing */
static rinc_Voice *rinc_voice(rio_Val *a) { return a[0].u < rinc_VOICES ? &rinc_voices[a[0].u] : 0; }
static void rinc_play(rinc_Voice *v, const uint8_t *p, uint32_t bytes, int bits, int channels, float rate, uint32_t block) {
  rinc_lock(1);
  v->synth = 0;
  v->data = p; v->bits = (uint8_t)bits; v->stereo = channels == 2;
  if (bits == 4) {
    v->block = block; v->per = (block / (uint32_t)channels - 4) * 2 + 1;
    v->frames = rinc_adpcmFrames(bytes, block, v->stereo);
    v->dec.at = 0xFFFFFFFFu;
  } else v->frames = bytes / (uint32_t)(bits / 8 * channels);
  v->pos = v->frac = 0; rinc_stepFor(v, rate); v->loop0 = v->loop1 = 0; v->vol = 1; v->pan = 0; rinc_gains(v);
  v->on = v->frames > 0;
  rinc_lock(0);
}
static void rinc_hostSound(rio_Vm *vm, rio_Val *a) {
  rinc_Voice *v = rinc_voice(a); const uint8_t *p = (const uint8_t *)rio_sliceBytes(vm, a + 1);
  pc_Sound s;
  int bits, ch;
  rio_getStruct(vm, a + 3, &s, sizeof s);
  bits = s.bits ? s.bits : 16; ch = s.channels ? s.channels : 1; /* (zeros: the defaults) */
  if (!v || !p || (bits != 4 && bits != 8 && bits != 16) || (ch != 1 && ch != 2)) return;
  rinc_play(v, p, a[2].u, bits, ch, s.rate != 0 ? s.rate : 16000.0f, rinc_ADPCM_BLOCK * (uint32_t)ch);
}
/* soundWav(voice, data): play a WAV file's sound (8- or 16-bit PCM, or IMA ADPCM; mono or stereo) at
   its own rate. Returns whether it could */
static uint32_t rinc_le(const uint8_t *p, int n) { return n == 2 ? (uint32_t)(p[0] | p[1] << 8) : (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void rinc_hostWav(rio_Vm *vm, rio_Val *a) {
  rinc_Voice *v = rinc_voice(a); const uint8_t *p = (const uint8_t *)rio_sliceBytes(vm, a + 1), *fmt = 0; uint32_t n = a[2].u, k = 12, size;
  int ok = 0;
  if (v && p && n >= 12 && !memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WAVE", 4))
    for (; k + 8 <= n; k += 8 + size + (size & 1)) {
      size = rinc_le(p + k + 4, 4);
      if (!memcmp(p + k, "fmt ", 4) && size >= 16 && k + 8 + size <= n) fmt = p + k + 8;
      if (!memcmp(p + k, "data", 4) && fmt) {
        uint32_t tag = rinc_le(fmt, 2), ch = rinc_le(fmt + 2, 2), rate = rinc_le(fmt + 4, 4), align = rinc_le(fmt + 12, 2), bits = rinc_le(fmt + 14, 2);
        if (size > n - k - 8) size = n - k - 8; /* (a cut-short file: what's there) */
        if ((ch == 1 || ch == 2) && rate && ((tag == 1 && (bits == 8 || bits == 16)) || (tag == 0x11 && bits == 4 && align >= 4 * ch + ch * 4 && align % (4 * ch) == 0))) {
          if (tag == 1 && bits == 8) { /* (WAV's 8-bit is unsigned: Rinc's is signed, so not this) */
            break;
          }
          rinc_play(v, p + k + 8, size, (int)bits, (int)ch, (float)rate, align);
          ok = 1;
        }
        break;
      }
    }
  a[0].i = ok;
}
/* soundRate(voice, rate): on the synth, a speed-up of the whole (16000: none) */
static void rinc_hostRate(rio_Vm *vm, rio_Val *a) {
  rinc_Voice *v = rinc_voice(a); (void)vm;
  if (!v) return;
  rinc_lock(1);
  if (v->synth) { v->play = rinc_clampf(a[1].f, 0, rinc_RATE * 16.0f) / rinc_SYNTH_RATE; rinc_stepFor(v, v->rate * v->play); }
  else rinc_stepFor(v, a[1].f);
  rinc_lock(0);
}
static void rinc_hostVolume(rio_Vm *vm, rio_Val *a) { rinc_Voice *v = rinc_voice(a); (void)vm; if (v) { rinc_lock(1); v->vol = rinc_clampf(a[1].f, 0, 1); rinc_gains(v); rinc_lock(0); } }
static void rinc_hostPan(rio_Vm *vm, rio_Val *a) { rinc_Voice *v = rinc_voice(a); (void)vm; if (v) { rinc_lock(1); v->pan = rinc_clampf(a[1].f, -1, 1); rinc_gains(v); rinc_lock(0); } }
/* soundLoop(voice, start, end): from end, go back to start (frames); end 0 or less: no loop */
static void rinc_hostLoop(rio_Vm *vm, rio_Val *a) {
  rinc_Voice *v = rinc_voice(a); (void)vm;
  if (!v) return;
  rinc_lock(1);
  v->loop0 = a[1].i > 0 ? a[1].u : 0; v->loop1 = a[2].i > 0 ? a[2].u : 0;
  if (v->loop1 > v->frames) v->loop1 = v->frames;
  if (v->loop0 >= v->loop1) v->loop0 = v->loop1 = 0;
  if (v->bits == 4 && v->loop1) { v->at0.at = 0xFFFFFFFFu; rinc_adpcmTo(v, &v->at0, v->loop0); } /* (the decoder at the loop's start) */
  rinc_lock(0);
}
/* synth(voice, notes, speed): play notes (4 bytes each) on the synth, each speed 120ths of a second */
static void rinc_hostSynth(rio_Vm *vm, rio_Val *a) {
  rinc_Voice *v = rinc_voice(a); const uint8_t *p = (const uint8_t *)rio_sliceBytes(vm, a + 1);
  if (!v) return;
  rinc_lock(1);
  if (p && a[2].u >= 4) rinc_synthStart(v, p, a[2].u / 4, a[3].i, (int)(v - rinc_voices));
  else v->on = 0;
  rinc_lock(0);
}
/* synthMake(out, notes, speed): the notes made into out instead, as 16-bit samples (little-endian) at
   the rate set for voice -1 (16 kHz unless set), to its end or out's: how many. Speed 0: on from where
   the last one stopped (its next sample, in the notes given now) */
static void rinc_hostSynthMake(rio_Vm *vm, rio_Val *a) {
  uint8_t *out = (uint8_t *)rio_sliceBytes(vm, a); const uint8_t *notes = (const uint8_t *)rio_sliceBytes(vm, a + 2);
  uint32_t cap = a[1].u / 2, k = 0, count = a[3].u / 4;
  rinc_Voice *v = &rinc_maker;
  a[0].u = 0;
  if (!out || !notes || !count) return;
  if (a[4].i) rinc_synthStart(v, notes, count, a[4].i, rinc_VOICES);
  else {
    if (!v->on) return;
    v->notes = notes; v->frames = count;
    if (v->note >= count) { v->note = count - 1; if (!v->tail) v->tail = v->fade; } /* (fewer notes now: past them, the end) */
    rinc_synthApply(v);
  }
  for (; k < cap && v->on; k++, v->sb = rinc_synthNext(v)) {
    int32_t x = v->sb < -32768 ? -32768 : v->sb > 32767 ? 32767 : v->sb;
    out[k * 2] = (uint8_t)(x & 255); out[k * 2 + 1] = (uint8_t)(x >> 8 & 255);
  }
  a[0].u = k;
}
/* the synth's settings for voice a[0] (-1: synthMake's), changed by change, and into the voice if it's
   the synth's now */
static void rinc_synthChange(rio_Val *a, void (*change)(rinc_SynthSet *s, rio_Val *a)) {
  int i = a[0].i == -1 ? rinc_VOICES : a[0].i;
  if (i < 0 || i > rinc_VOICES) return;
  rinc_lock(1);
  change(rinc_synthSet(i), a);
  if (i < rinc_VOICES && rinc_voices[i].synth) rinc_synthApply(&rinc_voices[i]);
  rinc_lock(0);
}
static void rinc_setRate(rinc_SynthSet *s, rio_Val *a) { s->rate = rinc_clampf(a[1].f, 8000, rinc_RATE); }
static void rinc_setBend(rinc_SynthSet *s, rio_Val *a) { s->bend = rinc_clampf(a[1].f, -96, 96); }
static void rinc_setEnvelope(rinc_SynthSet *s, rio_Val *a) { s->attack = rinc_clampf(a[1].f, 0, 10); s->release = rinc_clampf(a[2].f, 0, 10); }
static void rinc_setSmooth(rinc_SynthSet *s, rio_Val *a) { s->smooth = a[1].i != 0; }
/* synthRate(voice, rate): samples made a second, 8000 to 48000 */
static void rinc_hostSynthRate(rio_Vm *vm, rio_Val *a) { (void)vm; rinc_synthChange(a, rinc_setRate); }
/* synthBend(voice, semitones): every pitch moved by that much, -96 to 96 */
static void rinc_hostSynthBend(rio_Vm *vm, rio_Val *a) { (void)vm; rinc_synthChange(a, rinc_setBend); }
/* synthEnvelope(voice, attack, release): seconds the volume takes to rise and to fall about 2/3 of the
   way to a note's (0 to 10; 0: at once) */
static void rinc_hostSynthEnvelope(rio_Vm *vm, rio_Val *a) { (void)vm; rinc_synthChange(a, rinc_setEnvelope); }
/* synthSmooth(voice, on): the waves that jump (saw, square, pulse) smoothed where they jump */
static void rinc_hostSynthSmooth(rio_Vm *vm, rio_Val *a) { (void)vm; rinc_synthChange(a, rinc_setSmooth); }
/* synthWave(w, cycle): wave w (8 to 15) a cycle of 16-bit samples (little-endian), read as it plays;
   fewer than 2 samples: silent */
static void rinc_hostSynthWave(rio_Vm *vm, rio_Val *a) {
  const uint8_t *p = (const uint8_t *)rio_sliceBytes(vm, a + 1);
  uint32_t w = a[0].u - 8;
  if (w >= 8) return;
  rinc_lock(1);
  rinc_waves[w].data = p && a[2].u >= 4 ? p : 0; rinc_waves[w].n = rinc_waves[w].data ? a[2].u / 2 : 0;
  rinc_lock(0);
}
/* soundStop(voice): at once, or on the synth, fading out as after its last note (so it doesn't click) */
static void rinc_hostStop(rio_Vm *vm, rio_Val *a) {
  rinc_Voice *v = rinc_voice(a); (void)vm;
  if (!v) return;
  rinc_lock(1);
  if (v->synth && v->on) { v->loop0 = v->loop1 = 0; if (!v->tail) v->tail = v->fade; }
  else v->on = 0;
  rinc_lock(0);
}
/* soundSeek(voice, at): on from frame at (on the synth, note at; past the end, the last) */
static void rinc_hostSeek(rio_Vm *vm, rio_Val *a) {
  rinc_Voice *v = rinc_voice(a); uint32_t at = a[1].i > 0 ? a[1].u : 0; (void)vm;
  if (!v || !v->on || !v->frames) return;
  rinc_lock(1);
  if (at >= v->frames) at = v->frames - 1;
  if (v->synth) { v->note = at; v->k = 0; v->tail = 0; }
  else { v->pos = at; v->frac = 0; if (v->bits == 4) v->dec.at = 0xFFFFFFFFu; }
  rinc_lock(0);
}
static void rinc_hostPlaying(rio_Vm *vm, rio_Val *a) { rinc_Voice *v = rinc_voice(a); (void)vm; a[0].i = v && v->on; }

int rinc_audioProcs(rio_Vm *vm) {
  return rio_addFfi(vm, "pc.sound", pc_sound_SIG, rinc_hostSound) || rio_addFfi(vm, "pc.soundRate", "if", rinc_hostRate) || rio_addFfi(vm, "pc.soundVolume", "if", rinc_hostVolume) ||
         rio_addFfi(vm, "pc.soundPan", "if", rinc_hostPan) || rio_addFfi(vm, "pc.soundLoop", "iii", rinc_hostLoop) || rio_addFfi(vm, "pc.soundStop", "i", rinc_hostStop) ||
         rio_addFfi(vm, "pc.soundPlaying", "i>i", rinc_hostPlaying) || rio_addFfi(vm, "pc.soundWav", "ib>i", rinc_hostWav) ||
         rio_addFfi(vm, "pc.synth", "ibi", rinc_hostSynth) || rio_addFfi(vm, "pc.synthMake", "bbi>i", rinc_hostSynthMake) ||
         rio_addFfi(vm, "pc.synthRate", "if", rinc_hostSynthRate) || rio_addFfi(vm, "pc.synthBend", "if", rinc_hostSynthBend) ||
         rio_addFfi(vm, "pc.synthEnvelope", "iff", rinc_hostSynthEnvelope) || rio_addFfi(vm, "pc.synthSmooth", "ii", rinc_hostSynthSmooth) ||
         rio_addFfi(vm, "pc.synthWave", "ib", rinc_hostSynthWave) || rio_addFfi(vm, "pc.soundSeek", "ii", rinc_hostSeek);
}
