"""The real worker must honor heroes selected in the original scenario UI."""
from pathlib import Path
import argparse,json,os,subprocess
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime',type=Path,required=True)
args=parser.parse_args();root=Path(__file__).resolve().parents[1]
# Arrogance has humans in colors 0/1 and computer slots 5/6.
setup=[1,0,8,137,3,0,1,6,-1,3,0,-1,-1,-1,3,0,-1,-1,-1,3,0,-1,-1,-1,3,0,10,0,-1,3,0,10,2,-1,3,0,-1,-1,-1,3,0]
results=[]
for hero in (128,137,143):
    setup[3]=hero
    run=subprocess.run([str(root/'build/cmake-server/homm3-worker'),str(args.runtime.resolve()),'maps/Arrogance.h3m','42','--humans','2','--setup',','.join(map(str,setup))],input='view 0\nquit\n',text=True,capture_output=True,env={**os.environ,'SDL_AUDIODRIVER':'dummy','SDL_VIDEODRIVER':'dummy'},timeout=20)
    assert run.returncode==0,run.stderr
    frames=[json.loads(line) for line in run.stdout.splitlines()]
    view=next(frame for frame in frames if frame['type']=='view')
    assert view['heroes'][0]['id']==hero,view
    results.append({'selected':hero,'actual':view['heroes'][0]['id']})
out=root/'test-results/authority-parity/setup.json';out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps({'passed':True,'cases':results},indent=2)+'\n');print(out.read_text())
