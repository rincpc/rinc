// node webstore.mjs keeper.html watcher.html: a page's storage in IndexedDB (a stand-in for it here, as node has
// none) and other tabs told of each change (node's own BroadcastChannel): keeper (it counts its starts in
// a save) run, run again (a reload: the same IndexedDB), with watcher (another tab, the same storage name)
// running meanwhile, which logs each count it sees; then keeper in a fresh IndexedDB, with what a page
// before IndexedDB kept in localStorage (41), moved in. Prints what the pages logged.
import { readFileSync } from "node:fs";
import vm from "node:vm";

const [keeperPage, watcherPage] = process.argv.slice(2);
// IndexedDB, as much of it as the page uses: one database, one store of keys and values; its requests
// answer after a turn (as IndexedDB's do), and each transaction completes after its requests
const fakeIndexedDB = data => ({
  open: () => {
    const r = {};
    setTimeout(() => {
      const db = {
        createObjectStore: () => {},
        transaction: () => {
          const tx = { requests: 0 };
          const done = () => setTimeout(() => { if (--tx.requests === 0 && tx.oncomplete) tx.oncomplete(); });
          const ask = f => { const q = {}; tx.requests++; setTimeout(() => { q.result = f(); if (q.onsuccess) q.onsuccess(); done(); }); return q; };
          tx.objectStore = () => ({
            put: (v, k) => ask(() => data.set(k, v.slice())), delete: k => ask(() => data.delete(k)),
            getAllKeys: () => ask(() => [...data.keys()].sort()), getAll: () => ask(() => [...data.keys()].sort().map(k => data.get(k))),
          });
          return tx;
        },
      };
      r.result = db;
      if (data.fresh && r.onupgradeneeded) r.onupgradeneeded();
      data.fresh = false;
      r.onsuccess();
    });
    return r;
  },
});
const storage = map => ({
  get length() { return map.size; }, key: i => [...map.keys()][i] ?? null,
  getItem: k => (map.has(k) ? map.get(k) : null), setItem: (k, v) => map.set(k, String(v)), removeItem: k => map.delete(k),
});
let logged = "";
const run = (page, name, data, local) => { // (a tab: its script, running; tick(n) runs n frames)
  const script = readFileSync(page, "utf8").match(/<script>([\s\S]*)<\/script>/)[1];
  const noop = () => {};
  const element = { addEventListener: noop, getContext: () => ({ createImageData: (w, h) => ({ data: new Uint8ClampedArray(w * h * 4) }), putImageData: noop }), getBoundingClientRect: () => ({ left: 0, top: 0, width: 320, height: 240 }) };
  const tab = { tick: null, clock: 0 }; // (its own clock, a 60th of a second a frame: the page runs its app a frame each 60th, as the clock it's given says, so a machine's fast or slow timers don't change what's run)
  const errEl = { hidden: true, textContent: "" };
  vm.runInNewContext(script, {
    localStorage: storage(local), indexedDB: fakeIndexedDB(data), BroadcastChannel, performance: { now: () => tab.clock }, atob, Blob, Response, DecompressionStream, TextDecoder, TextEncoder, Uint8Array, WebAssembly, URL, Math, Promise, Date,
    console: { log: s => { logged += name + ": " + s + "\n"; } }, navigator: {},
    document: { querySelector: () => element, getElementById: () => errEl, addEventListener: noop, documentElement: {} },
    addEventListener: noop, requestAnimationFrame: f => { tab.tick = f; }, confirm: () => true, location: { reload: noop },
  });
  tab.frames = async n => {
    for (let i = 0; i < 300 && !tab.tick; i++) await new Promise(r => setTimeout(r, 10));
    for (let i = 0; i < n && tab.tick; i++) { const f = tab.tick; tab.tick = null; tab.clock += 1000 / 60; f(tab.clock); await new Promise(r => setImmediate(r)); }
  };
  return tab;
};
const data = new Map(); data.fresh = true;
const local = new Map();
await run(keeperPage, "keeper", data, local).frames(5);
const watcher = run(watcherPage, "watcher", data, local);
await watcher.frames(15);
await run(keeperPage, "keeper again", data, local).frames(5);
await watcher.frames(15);
const before = new Map(); before.fresh = true;
await run(keeperPage, "keeper, from localStorage", before, new Map([["shared/save/shared/count", "41"]])).frames(5);
console.log(logged + "localStorage: " + JSON.stringify([...local.keys()]));
process.exit(0);
