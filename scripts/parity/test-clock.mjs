import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const context = {
  Homm3VirtualAudioContext: class {},
  performance, structuredClone, Uint32Array, Uint8Array, Map, Set,
  TextDecoder, HTMLMediaElement: class { play() {} },
  document: {addEventListener() {}},
  MessageChannel: class {
    constructor() { this.port1 = {}; this.port2 = {postMessage: () => queueMicrotask(() => this.port1.onmessage?.())}; }
  },
  fetch: async () => {},
};
context.window = context;
vm.createContext(context);
vm.runInContext(fs.readFileSync(new URL('./browser.js', import.meta.url), 'utf8'), context);
const h = context.homm3Harness;
h.controlled = true; h.recording = true;
let send;
context.homm3Authority = {actor: 0, revision: 0, start(fn) {send = fn;}, receive(msg) {return msg.valid !== false;}};
const a = context.homm3Authority;
a.start(() => {});
async function transportBoundary(id, response) {
  const done = h.advance(16);
  send({type: 'authority_command', id});
  const before = h.t;
  for (let i = 0; i < 100; ++i) assert.equal(h.tick(8, 100), before, 'IPC cannot consume simulated time');
  a.receive({type: 'authority_result', id, valid: false});
  assert.equal(h.tick(8, 100), before, 'rejected transport message cannot resume clock');
  a.receive({type: 'authority_status', activeActor: 0});
  assert.equal(h.tick(8, 100), before, 'same actor still waits');
  a.receive(response);
  h.tick(8, 100); h.tick(8, 100);
  await done;
  assert.equal(h.t, before + 16);
}
await transportBoundary('one', {type: 'authority_result', id: 'one'});
await transportBoundary('two', {type: 'authority_choice', token: 2});
await transportBoundary('three', {type: 'authority_status', activeActor: 1});
assert.equal(h.events.filter(e => e.kind === 'transport-ready').length, 3);
h.world('rng', {}, -987567403);
assert.equal(h.worlds[0].rng, 3307399893, 'uint32 RNG survives EM_ASM signed words');
console.log('PASS: IPC pause, validated response, decision/turn boundaries, transport wall-time records, uint32 RNG');

const delayed=h.advance(16);send({type:'authority_command',id:'delayed-world'});
const beforeWorld=h.t;
a.receive({type:'authority_result',id:'delayed-world',ok:true,revision:3});
for(let i=0;i<32;++i)assert.equal(h.tick(8,100),beforeWorld,'result before world cannot charge fake movement latency');
a.receive({type:'authority_state',state:{revision:2}});
assert.equal(h.tick(8,100),beforeWorld,'old world cannot release a newer command');
a.receive({type:'authority_state',state:{revision:3}});
h.tick(8,100);h.tick(8,100);await delayed;
assert.equal(h.t,beforeWorld+16);
console.log('PASS: result-before-world packet ordering');
