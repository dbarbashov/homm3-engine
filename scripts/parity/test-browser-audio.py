"""Validate the accelerated mixer against real offline WebAudio, silently."""
from pathlib import Path
import json
from playwright.sync_api import sync_playwright
source=(Path(__file__).parent/'virtual-audio.js').read_text()
with sync_playwright() as p:
    results=[]
    for engine in ['chromium','webkit']:
        browser=getattr(p,engine).launch();page=browser.new_page();page.add_script_tag(content=source)
        result=page.evaluate('''async () => {
          const rate=44100,frames=44100;
          const virtual=new Homm3VirtualAudioContext();virtual.recent=new Float32Array(frames);
          const offline=new OfflineAudioContext(1,frames,rate);
          function graph(ctx) {
            const buffer=ctx.createBuffer(1,4410,rate),pcm=buffer.getChannelData(0);
            for(let i=0;i<pcm.length;i++)pcm[i]=.4*Math.sin(2*Math.PI*440*i/rate);
            const source=ctx.createBufferSource(),gain=ctx.createGain();source.buffer=buffer;source.loop=true;
            source.connect(gain);gain.connect(ctx.destination);gain.gain.setValueAtTime(.7,0);gain.gain.setValueAtTime(.3,.2);gain.gain.setValueAtTime(0,.4);gain.gain.setValueAtTime(.5,.6);
            source.start();source.stop(.75);
          }
          graph(virtual);graph(offline);for(let i=0;i<125;i++)virtual.advance(8);
          const expected=(await offline.startRendering()).getChannelData(0),actual=virtual.recent;
          let maxError=0,rmsError=0;
          for(let i=0;i<frames;i++){const error=actual[i]-expected[i];maxError=Math.max(maxError,Math.abs(error));rmsError+=error*error;}
          return {maxError,rmsError:Math.sqrt(rmsError/frames),samples:virtual.sample,active:virtual.sources.size};
        }''')
        assert result['maxError']<1e-5,result
        assert result['samples']==44100 and result['active']==0,result
        results.append({'browser':engine,**result});browser.close()
    print(json.dumps({'passed':True,'results':results},indent=2))
