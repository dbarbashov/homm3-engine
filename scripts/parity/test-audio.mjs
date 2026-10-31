import assert from 'node:assert/strict';
import {performance} from 'node:perf_hooks';
import './virtual-audio.js';
const ctx=new Homm3VirtualAudioContext();
const buffer=ctx.createBuffer(1,44100,44100);buffer.getChannelData(0).fill(.5);
const gain=ctx.createGain();gain.connect(ctx.destination);gain.gain.setValueAtTime(0,.5);
const source=ctx.createBufferSource();source.buffer=buffer;source.connect(gain);source.start();let ends=0;source.onended=()=>++ends;
assert.ok(Math.abs(ctx.advance(250).rms-.5)<1e-9);
ctx.advance(250);assert.equal(ctx.advance(250).rms,0);ctx.advance(250);assert.equal(ends,1);
const clock=new Homm3VirtualAudioContext();for(let i=0;i<125;i++)clock.advance(8);assert.equal(clock.sample,44100);
const loops=new Homm3VirtualAudioContext();const b=loops.createBuffer(1,441,44100);b.getChannelData(0).fill(.2);
function play(){const s=loops.createBufferSource();s.buffer=b;s.connect(loops.destination);s.onended=()=>{if(++ends<4)play();};s.start();}
ends=0;play();loops.advance(100);assert.equal(ends,4);
const perf=new Homm3VirtualAudioContext();const s=perf.createBufferSource();s.buffer=buffer;s.loop=true;s.connect(perf.destination);s.start();
const begin=performance.now();for(let i=0;i<7500;i++)perf.advance(8);const wallMs=performance.now()-begin;
assert.equal(perf.sample,2646000);console.log(JSON.stringify({test:'virtual-audio',passed:true,virtualMs:60000,wallMs,speedup:60000/wallMs}));
