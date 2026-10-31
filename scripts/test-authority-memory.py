"""Measure real ordinary worker map projections and resident memory, silently."""
import argparse,json,os,platform,select,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',required=True,type=Path);args=p.parse_args()
OUT=ROOT/'test-results/authority-memory';OUT.mkdir(parents=True,exist_ok=True)
maps=['Good to Go.h3m','Jihad.h3m','Ascension.h3m','Rediscovery.h3m','Island King Allied.h3m',"The Battle of Daeyan's Ford.h3m",'Reclamation Allied.h3m','War of the Mighty (Allies).h3m']
rows=[]
for name in maps:
    proc=subprocess.Popen([str(ROOT/'build/cmake-server/homm3-worker'),str(args.runtime.resolve()),'maps/'+name,'42','--humans','2'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,bufsize=0,env={**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'})
    pending=bytearray()
    def read():
        until=time.monotonic()+30
        while b'\n' not in pending:
            assert select.select([proc.stdout],[],[],max(0,until-time.monotonic()))[0], 'worker timeout'
            data=os.read(proc.stdout.fileno(),65536)
            assert data, f'worker exited: {proc.stderr.read()}'
            pending.extend(data)
        line,_,rest=pending.partition(b'\n');pending[:]=rest
        return json.loads(line),len(line)
    def call(cmd):
        proc.stdin.write((cmd+'\n').encode());return read()
    try:
        ready,_=read();assert ready['protocol']==2 and ready['ready']
        lengths=[]
        for seat in ready['roster']:
            data,length=call('state '+str(seat['slot']));assert data['type']=='state';lengths.append(length)
            assert length<2*1024*1024, 'projection exceeds relay frame limit'
        after,_=call('status')
        assert after['lodCacheBytes']<=1024*1024
        assert after['peakRssBytes']<96*1024*1024,'bootstrap/projection memory regression'
        rows.append({'map':name,'size':ready['size'],'layers':ready['layers'],'rssBytes':after['rssBytes'],'peakRssBytes':after['peakRssBytes'],'maxProjectionBytes':max(lengths),'lodCacheBytes':after['lodCacheBytes']})
        proc.stdin.write(b'quit\n');proc.wait(timeout=5);assert proc.returncode==0
        print(name,round(after['rssBytes']/1048576,2),'MiB',max(lengths),'bytes',flush=True)
    finally:
        if proc.poll() is None:proc.kill()
        proc.wait()
summary={'passed':True,'platform':platform.platform(),'maps':rows,'scope':'Native ordinary bootstrap and initial per-seat projections; no long-game or Linux capacity claim.'}
(OUT/'results.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
