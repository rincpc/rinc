// node webplay.mjs page.html frames out.ppm: a page made by rinc -web, run as a browser would (as webdrop.mjs
// runs one: its script, with a stand-in for the DOM and localStorage), for that many frames; its last
// picture saved as a PPM, and what it logged printed
import { readFileSync, writeFileSync } from "node:fs";
import vm from "node:vm";

const [page, count, out] = process.argv.slice(2);
const script = readFileSync(page, "utf8").match(/<script>([\s\S]*)<\/script>/)[1];
const store = new Map();
const localStorage = {
  get length() { return store.size; },
  key: i => [...store.keys()][i] ?? null,
  getItem: k => (store.has(k) ? store.get(k) : null),
  setItem: (k, v) => store.set(k, String(v)),
  removeItem: k => store.delete(k),
};
const noop = () => {};
let last = null;
const element = { addEventListener: noop, getContext: () => ({ createImageData: (w, h) => ({ width: w, height: h, data: new Uint8ClampedArray(w * h * 4) }), putImageData: img => { last = img; } }),
                  getBoundingClientRect: () => ({ left: 0, top: 0, width: 320, height: 240 }) };
let tick = null, logged = "";
const errEl = { hidden: true, textContent: "" }, resetEl = { hidden: true, addEventListener: noop };
const context = {
  localStorage, performance, atob, Blob, Response, DecompressionStream, TextDecoder, TextEncoder, Uint8Array, WebAssembly, URL, Math, Promise,
  console: { log: s => { logged += s + "\n"; } },
  navigator: {},
  document: { querySelector: () => element, getElementById: id => (id === "reset" ? resetEl : errEl), addEventListener: noop, documentElement: {} },
  addEventListener: noop,
  requestAnimationFrame: f => { tick = f; },
  confirm: () => true, location: { reload: noop },
};
vm.runInNewContext(script, context);
for (let i = 0; i < 300 && !tick; i++) await new Promise(r => setTimeout(r, 10));
if (!tick) { console.log("FAIL: the page didn't start: " + errEl.textContent); process.exit(1); }
let t = 0;
for (let i = 0; i < Number(count); i++) { const f = tick; tick = null; t = Math.max(t + 1000 / 60, performance.now()); f(t); }
if (!last) { console.log("FAIL: nothing drawn"); process.exit(1); }
const w = last.width, h = last.height, rgb = Buffer.alloc(w * h * 3);
for (let i = 0; i < w * h; i++) for (let k = 0; k < 3; k++) rgb[i * 3 + k] = last.data[i * 4 + k];
writeFileSync(out, Buffer.concat([Buffer.from(`P6\n${w} ${h}\n255\n`), rgb]));
console.log(logged + (errEl.hidden ? "" : "error: " + errEl.textContent));
