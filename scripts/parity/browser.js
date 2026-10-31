/* Inject before boot. Accessible only in the private harness build/runner. */
(() => {
  const queue=[];
  const channel=new MessageChannel();channel.port1.onmessage=()=>queue.shift()?.();
  const immediate=()=>new Promise(resolve=>{queue.push(resolve);channel.port2.postMessage(0);});
  const contexts=[];
  let assetPending=0;
  const transportPending=new Map();
  const nativeFetch=window.fetch.bind(window);
  window.fetch=async function(input,init){
    const url=typeof input==='string'?input:input.url;
    if(!String(url).includes('/api/music?'))return nativeFetch(input,init);
    ++assetPending;
    try {const response=await nativeFetch(input,init);const bytes=await response.arrayBuffer();return new Response(bytes,{status:response.status,statusText:response.statusText,headers:response.headers});}
    finally{--assetPending;}
  };
  const clock=window.__homm3ParityClock||'virtual';
  if(clock==='virtual')window.AudioContext=class extends Homm3VirtualAudioContext {constructor(){super();contexts.push(this);this.observer=event=>h.event('audio-node',event);}};
  else {
    const Native=window.AudioContext;
    window.AudioContext=class extends Native {
      constructor(){
        const silent='setSinkId' in Native.prototype;
        super(silent?{sinkId:{type:'none'}}:{});
        if(!silent){const output=super.destination,mute=this.createGain();mute.gain.value=0;mute.connect(output);Object.defineProperty(this,'destination',{get:()=>mute});}
      }
    };
  }
  const mediaPlay=HTMLMediaElement.prototype.play;
  HTMLMediaElement.prototype.play=function(...args){this.muted=true;this.volume=0;return mediaPlay.apply(this,args);};
  let pending=null,batch=0,wake=null;
  const h=window.homm3Harness={
    schema:1,clock,t:0,recording:false,action:'boot',fixture:null,frames:[],events:[],worlds:[],audio:[],inputs:[],overflow:false,
    maxRecords:100000,wallStart:performance.now(),frameCount:0,pumpCount:0,
    append(list,value){if(list.length>=h.maxRecords){h.overflow=true;throw Error('Harness trace budget exceeded');}list.push(value);},
    event(kind,data){if(h.recording)h.append(h.events,{t:h.t,action:h.action,kind,data});},
    tick(ms,realTicks){
      ++h.pumpCount;++batch;
      let advance=h.controlled?ms:Math.max(0,realTicks-h.t);
      if(pending){
        try{if(pending.predicate?.()){pending.resolve({t:h.t});pending=null;advance=0;}}
        catch(error){pending.reject(error);pending=null;advance=0;}
      }
      if(h.controlled&&!pending)advance=0;
      if(pending&&clock==='virtual')advance=assetPending||transportPending.size||contexts.some(c=>c.pending)?0:Math.min(ms,pending.end-h.t);
      if(advance){
        h.t+=advance;
        for(const ctx of contexts){const pcm=ctx.advance(advance);if(h.recording)h.append(h.audio,{t:h.t,action:h.action,...pcm});}
      }
      if(pending&&h.t>=pending.end){const task=pending;pending=null;task.predicate?task.reject(Error('Virtual deadline: '+h.action)):task.resolve({t:h.t});}
      return h.t;
    },
    shouldYield(){return !pending||batch>=16||assetPending||transportPending.size||contexts.some(c=>c.pending);},
    async yield(){batch=0;if(h.controlled&&!pending)await new Promise(resolve=>{wake=resolve;});do {await immediate();} while(clock==='virtual'&&(assetPending||contexts.some(c=>c.pending)));},
    async advance(ms,predicate=null){
      if(pending)throw Error('Concurrent advance');
      if(!Number.isFinite(ms)||ms<=0||ms>600000)throw Error('Invalid virtual budget');
      h.controlled=true;
      return new Promise((resolve,reject)=>{pending={end:h.t+ms,predicate,resolve,reject};const resume=wake;wake=null;resume?.();});
    },
    async arm(){
      if(clock==='real')return;
      h.controlled=true;
      const music={...window.homm3Music.state};
      Module._homm3_harness_arm();
      window.homm3Music.close();
      for(const ctx of contexts){ctx.advance(0);if(ctx.sources.size)throw Error('Audio still active at fixture epoch');ctx.sample=0;ctx.timeMs=0;ctx.recent.fill(0);}
      h.t=1000000;
      if(music.requestedTrack||music.track)await window.homm3Music.play(music.requestedTrack||music.track,0,music.volume);
    },
    begin(action){h.action=action;h.recording=true;h.wallStart=performance.now();h.event('action',{});if(h.lastFrame)h.append(h.frames,{...h.lastFrame,t:h.t,wallMs:0,action,held:true});},
    frame(width,height,tiles){
      if(!h.recording)return;++h.frameCount;
      const ui=h.ui();h.lastFrame={t:h.t,wallMs:performance.now()-h.wallStart,action:h.action,width,height,tiles,ui};h.append(h.frames,h.lastFrame);
    },
    ui(){return structuredClone({window:h.interfaceState,game:window.homm3GameState,adventure:window.homm3AdventureState,combat:h.exactCombat,video:h.video,network:window.homm3NetworkState,hd:window.homm3HDState,turn:window.homm3TurnPresentation,screen:window.homm3PortDiagnostics?.screen,music:window.homm3Music?.state});},
    world(label,world,rng){h.append(h.worlds,{label,world,rng:typeof rng==='string'?rng:rng>>>0,t:h.t});},
    installFixture(FS){
      if(!h.fixture)return false;
      if(h.fixture.schema!==1||!Number.isInteger(h.fixture.rng)||!h.fixture.saveBase64)throw Error('Invalid fixture manifest');
      const bytes=Uint8Array.from(atob(h.fixture.saveBase64),c=>c.charCodeAt(0));
      FS.writeFile('/runtime/data/RMTparity.gm2',bytes);return true;
    },
    snapshot(){return {t:h.t,ui:h.ui(),frames:h.frameCount,pumps:h.pumpCount,wallMs:performance.now()-h.wallStart};},
    drain(){const out={schema:h.schema,overflow:h.overflow,action:h.action,...h.snapshot(),frames:h.frames,events:h.events,worlds:h.worlds,audio:h.audio,inputs:h.inputs};h.frames=[];h.events=[];h.audio=[];h.inputs=[];h.worlds=[];return out;}
  };
  // Real IPC must not consume an arbitrarily accelerated simulation budget.
  // Keep pumping the original continuation; only pause virtual time until the
  // response/decision arrives. Real transport duration remains in the trace.
  let authority;
  let latestWorldRevision=-1;
  const awaitingWorld=new Map();
  Object.defineProperty(window,'homm3Authority',{configurable:true,get:()=>authority,set:next=>{
    authority=next;
    const start=next.start,receive=next.receive;
    next.start=function(send){return start.call(this,message=>{
      if(['authority_command','authority_answer'].includes(message.type)){
        transportPending.set(String(message.id),performance.now());h.event('transport-send',{id:message.id,type:message.type});
      }
      return send(message);
    });};
    next.receive=function(message){
      const accepted=receive.call(this,message);
      if(!accepted)return accepted;
      // A success result may precede its world packet in a separate socket
      // task. Do not charge that delivery gap as simulated movement time.
      if(message.type==='authority_state'||message.type==='state')latestWorldRevision=Math.max(latestWorldRevision,(message.state||message).revision);
      const ids=[];
      if(['authority_result','authority_answer_result'].includes(message.type)){
        const id=String(message.id);
        if(message.ok&&message.revision>Math.max(latestWorldRevision,next.revision))awaitingWorld.set(id,message.revision);
        else ids.push(id);
      }
      if(message.type==='authority_choice'||message.type==='authority_status'&&message.activeActor!==next.actor)ids.push(...transportPending.keys());
      for(const [id,revision] of awaitingWorld)if(revision<=latestWorldRevision)ids.push(id);
      for(const id of ids)awaitingWorld.delete(id);
      for(const id of ids)if(transportPending.has(id)){
        h.event('transport-ready',{id,wallMs:performance.now()-transportPending.get(id),type:message.type});transportPending.delete(id);
      }
      return accepted;
    };
  }});
  let controlled=false;Object.defineProperty(h,'controlled',{get:()=>controlled&&clock==='virtual',set:value=>{controlled=!!value;}});
  for(const type of ['keydown','keyup','pointerdown','pointerup','pointermove'])document.addEventListener(type,e=>{
    if(h.recording)h.append(h.inputs,{t:h.t,wallMs:performance.now()-h.wallStart,action:h.action,type,key:e.key,code:e.code,x:e.clientX,y:e.clientY,button:e.button});
  },true);
  // Trace semantic audio requests without replacing production selection/fade logic.
  for(const name of ['homm3Music','homm3Sfx']){
    let value;Object.defineProperty(window,name,{configurable:true,get:()=>value,set:next=>{
      value=next;
      const names=new Map(),slots=new Map();
      if(name==='homm3Sfx'){
        const load=next.load;next.load=function(id,label,...args){names.set(id,label);return load.call(this,id,label,...args);};
      }
      for(const method of ['play','stop','pause','resume','restart','volume'])if(typeof next[method]==='function'){
        const original=next[method];next[method]=function(...args){
          let data=args;
          if(name==='homm3Sfx'){
            if(method==='play'){slots.set(args[0],names.get(args[1]));data={name:names.get(args[1]),loops:args[2],volume:args[3]};}
            else data={name:slots.get(args[0])||null,value:args[1]??null};
          }
          h.event(name+'.'+method,data);return original.apply(this,args);
        };
      }
    }});
  }
})();
