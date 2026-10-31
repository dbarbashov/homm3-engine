/* Test-only, silent WebAudio subset used by the unchanged production backends.
 * No playbackRate shortcut: every PCM sample and scheduled gain is evaluated.
 * Unsupported graph features throw instead of silently producing a false pass.
 */
(() => {
  const root=globalThis;
  class Param {
    constructor(context,value=1){this.context=context;this.base=value;this.events=[];}
    get value(){return this.at(this.context.currentTime);}
    set value(v){this.base=Number(v);}
    setValueAtTime(value,time){this.events.push({value,time});this.events.sort((a,b)=>a.time-b.time);return this;}
    cancelScheduledValues(time){this.events=this.events.filter(e=>e.time<time);return this;}
    at(time){let value=this.base;for(const e of this.events){if(e.time>time)break;value=e.value;}return value;}
  }
  class Node {
    constructor(context){this.context=context;this.outputs=new Set();}
    connect(node){this.outputs.add(node);return node;}
    disconnect(){this.outputs.clear();}
    gainAt(time,visited=new Set()){
      if(this===this.context.destination)return 1;
      if(visited.has(this))throw Error('Cyclic test audio graph');
      let level=0;visited.add(this);for(const out of this.outputs)level+=out.gainAt(time,new Set(visited));
      return level*(this.gain?this.gain.at(time):1);
    }
  }
  class Buffer {
    constructor(channels,length,rate){this.numberOfChannels=channels;this.length=length;this.sampleRate=rate;this.duration=length/rate;this.data=Array.from({length:channels},()=>new Float32Array(length));}
    getChannelData(ch){return this.data[ch];}
    copyToChannel(data,ch,offset=0){this.data[ch].set(data,offset);}
  }
  class Source extends Node {
    constructor(ctx){super(ctx);this.loop=false;this.started=false;this.ended=false;this.stopAt=Infinity;this.onended=null;}
    start(when=0,offset=0){
      if(this.started||!this.buffer)throw Error('Invalid virtual source start');
      this.started=true;this.startAt=Math.max(when,this.context.currentTime);this.offset=offset;
      this.context.sources.add(this);this.context.event('start',{duration:this.buffer.duration,loop:this.loop,offset});
    }
    stop(when=0){this.stopAt=Math.max(when,this.context.currentTime);this.context.event('stop',{at:this.stopAt});}
    end(){if(this.ended)return;this.ended=true;this.context.sources.delete(this);this.context.event('ended',{});this.onended?.();}
    endAt(){return Math.min(this.stopAt,this.loop?Infinity:this.startAt+Math.max(0,this.buffer.duration-this.offset));}
  }
  class VirtualAudioContext {
    constructor(){this.sampleRate=44100;this.sample=0;this.timeMs=0;this.state='running';this.sources=new Set();this.destination=new Node(this);this.listeners=[];this.pending=0;this.recent=new Float32Array(2048);this.observer=null;}
    get currentTime(){return this.sample/this.sampleRate;}
    createGain(){const n=new Node(this);n.gain=new Param(this);return n;}
    createBufferSource(){return new Source(this);}
    createBuffer(ch,length,rate){return new Buffer(ch,length,rate);}
    createAnalyser(){const n=new Node(this);n.fftSize=2048;n.getFloatTimeDomainData=data=>data.set(this.recent.subarray(0,data.length));return n;}
    addEventListener(type,callback){if(type==='statechange')this.listeners.push(callback);}
    resume(){this.state='running';return Promise.resolve();}
    close(){this.state='closed';this.sources.clear();return Promise.resolve();}
    async decodeAudioData(bytes){
      ++this.pending;
      try {
        if(!root.OfflineAudioContext)throw Error('Offline decoder unavailable');
        if(!this.decoder)this.decoder=new root.OfflineAudioContext(2,1,this.sampleRate);
        return await this.decoder.decodeAudioData(bytes);
      }finally{--this.pending;}
    }
    event(kind,data){this.observer?.({kind,t:this.currentTime*1000,...data});}
    advance(milliseconds){
      this.timeMs+=milliseconds;
      const end=Math.round(this.timeMs*this.sampleRate/1000);
      let sum=0,peak=0,count=0,hash=2166136261;
      // Process each source-end at its exact sample, including finite-loop restarts.
      while(this.sample<end){
        for(const source of [...this.sources])if(this.currentTime+1e-10>=source.endAt())source.end();
        const time=this.currentTime;let left=0,right=0;
        for(const source of this.sources){
          if(time<source.startAt||time>=source.endAt())continue;
          const b=source.buffer,position=(source.offset+time-source.startAt)*b.sampleRate;
          const index=Math.floor(position)%b.length,next=(index+1)%b.length,fraction=position-Math.floor(position),gain=source.gainAt(time);
          const l=b.getChannelData(0),r=b.getChannelData(Math.min(1,b.numberOfChannels-1));
          left+=(l[index]+(l[next]-l[index])*fraction)*gain;right+=(r[index]+(r[next]-r[index])*fraction)*gain;
        }
        this.recent[this.sample%this.recent.length]=(left+right)/2;
        sum+=left*left+right*right;peak=Math.max(peak,Math.abs(left),Math.abs(right));
        hash=Math.imul(hash^Math.round(left*32767),16777619);hash=Math.imul(hash^Math.round(right*32767),16777619);
        ++this.sample;++count;
      }
      for(const source of [...this.sources])if(this.currentTime+1e-10>=source.endAt())source.end();
      return {samples:count,rms:count?Math.sqrt(sum/(2*count)):0,peak,hash:hash>>>0};
    }
  }
  root.Homm3VirtualAudioContext=VirtualAudioContext;
})();
