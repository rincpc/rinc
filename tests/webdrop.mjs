// node webdrop.mjs page.html bundle.zip [reset]: a page made by rinc -web, run as a browser would (its
// script, with a stand-in for the DOM and localStorage), a bundle of apps dropped on it, and some frames
// run; then (reset) its Reset button clicked, and yes to its question. Prints what the page logged, then
// each storage key, a line each
import { readFileSync } from "node:fs";
import vm from "node:vm";

const [page, zip, reset] = process.argv.slice(2);
const script = readFileSync(page, "utf8").match(/<script>([\s\S]*)<\/script>/)[1];
const store = new Map();
const localStorage = {
  get length() { return store.size; },
  key: i => [...store.keys()][i] ?? null,
  getItem: k => (store.has(k) ? store.get(k) : null),
  setItem: (k, v) => store.set(k, String(v)),
  removeItem: k => store.delete(k),
};
const on = {}, noop = () => {};
const element = { addEventListener: noop, getContext: () => ({ createImageData: (w, h) => ({ data: new Uint8ClampedArray(w * h * 4) }), putImageData: noop }), getBoundingClientRect: () => ({ left: 0, top: 0, width: 320, height: 240 }) };
let tick = null, logged = "";
const errEl = { hidden: true, textContent: "" }, resetEl = { hidden: true, on: {}, addEventListener(type, f) { this.on[type] = f; } };
let reloaded = false;
const context = {
  localStorage, performance, atob, Blob, Response, DecompressionStream, TextDecoder, TextEncoder, Uint8Array, WebAssembly, URL, Math, Promise,
  console: { log: s => { logged += s + "\n"; } },
  navigator: {},
  document: { querySelector: () => element, getElementById: id => (id === "reset" ? resetEl : errEl), addEventListener: noop, documentElement: {} },
  addEventListener: (type, f) => { on[type] = f; },
  requestAnimationFrame: f => { tick = f; },
  confirm: () => true, location: { reload: () => { reloaded = true; } },
};
vm.runInNewContext(script, context);
for (let i = 0; i < 200 && !tick; i++) await new Promise(r => setTimeout(r, 10));
if (!tick) { console.log("FAIL: the page didn't start: " + errEl.textContent); process.exit(1); }
let t = 0;
const frames = n => { for (let i = 0; i < n; i++) { const f = tick; tick = null; t += 1000 / 60; f(t); } };
frames(5);
const bytes = readFileSync(zip);
await on.drop({ preventDefault: noop, dataTransfer: { files: [{ name: zip.split(/[\\/]/).pop(), arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.length) }] } });
frames(30);
if (reset) {
  if (resetEl.hidden || !resetEl.on.click) { console.log("FAIL: no Reset button"); process.exit(1); }
  resetEl.on.click();
  logged += reloaded ? "reset, reloaded\n" : "reset, not reloaded\n";
} else if (!resetEl.hidden) logged += "a Reset button\n";
console.log(logged + (errEl.hidden ? "" : "error: " + errEl.textContent + "\n") + "keys:");
for (const k of [...store.keys()].sort()) console.log(k);
