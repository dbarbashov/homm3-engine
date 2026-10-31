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
await transportBoundary('three', {type: 'authority_result', id: 'three', ok: false});
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

const answered=h.advance(16);send({type:'authority_answer',id:'answer'});
const beforeAnswer=h.t;
a.receive({type:'authority_answer_result',id:'answer',ok:true,revision:3});
a.receive({type:'authority_status',activeActor:1});
for(let i=0;i<100;++i)assert.equal(h.tick(8,100),beforeAnswer,'accepted answer / turn announcement is not execution completion');
a.receive({type:'authority_choice',token:4});
h.tick(8,100);h.tick(8,100);await answered;
assert.equal(h.t,beforeAnswer+16);
console.log('PASS: answer execution and turn transfer do not consume transport wall time');

const completed=h.advance(16);send({type:'authority_answer',id:'final-answer'});
const finalTime=h.t;
a.receive({type:'authority_answer_result',id:'final-answer',ok:true,revision:3});
a.receive({type:'authority_result',id:'parent-command',ok:true,revision:4});
assert.equal(h.tick(8,100),finalTime);
a.receive({type:'authority_state',state:{revision:4}});
h.tick(8,100);h.tick(8,100);await completed;
assert.equal(h.t,finalTime+16);
console.log('PASS: parent result and world release a completed final answer');

const nested=h.advance(16);send({type:'authority_answer',id:'outer'});
context.homm3Authority={actor:0,revision:4,start(fn){send=fn;},receive(msg){if(msg.type==='authority_choice')send({type:'authority_answer',id:'inner'});return true;}};
context.homm3Authority.start(()=>{});
context.homm3Authority.receive({type:'authority_choice',token:5});
const nestedTime=h.t;assert.equal(h.tick(8,100),nestedTime,'automatic follow-up answer must remain pending');
context.homm3Authority.receive({type:'authority_answer_result',id:'inner',ok:false});
h.tick(8,100);h.tick(8,100);await nested;
console.log('PASS: reentrant decision replies retain their transport barrier');

context.homm3Authority={actor:1,revision:4,start(fn){send=fn;},receive(){return true;}};
context.homm3Authority.start(()=>{});
const remote=h.advance(16);send({type:'authority_answer',id:'next-player-dialog'});
const remoteTime=h.t;
context.homm3Authority.receive({type:'authority_answer_result',id:'next-player-dialog',ok:true,revision:4});
context.homm3Authority.receive({type:'authority_status',phase:'adventure',activeActor:1,revision:5});
assert.equal(h.tick(8,100),remoteTime,'wait for world even when parent command belongs to another seat');
context.homm3Authority.receive({type:'authority_state',state:{revision:5}});
h.tick(8,100);h.tick(8,100);await remote;
assert.equal(h.t,remoteTime+16);
console.log('PASS: next player answer completes through shared status/world');

// Whether IPC finishes before or after the poll must not erase one virtual tick.
const quantum=h.advance(16);send({type:'authority_command',id:'quantum'});
const quantumStart=h.t;assert.equal(h.tick(8,100),quantumStart);
const resumed=h.yield();context.homm3Authority.receive({type:'authority_result',id:'quantum',ok:false});
await resumed;assert.equal(h.t,quantumStart+8,'resume executes the suspended tick exactly once');
h.tick(8,100);await quantum;
console.log('PASS: asynchronous readiness cannot remove a presentation tick');
