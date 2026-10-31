"""Fresh native processes must project identical worlds, even with dirty heap memory.
Uses original map data locally; never emits or stores licensed resources.
"""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--worker', type=Path, default=ROOT/'build/cmake-parity-release-control/homm3-worker')
parser.add_argument('--runtime', type=Path, default=ROOT.parent/'runtime')
args = parser.parse_args()
results = []
for faction in range(9):
    rows = [[0,faction,faction*16,0,0],[1,0,15,0,0],[-1,-1,-1,0,0],[-1,-1,-1,0,0],
            [-1,-1,-1,0,0],[10,3,-1,0,0],[10,5,-1,0,0],[-1,-1,-1,0,0]]
    setup = ','.join(map(str,[1]+[v for row in rows for v in row]))
    command = [str(args.worker.resolve()),str(args.runtime.resolve()),'maps/Arrogance.h3m',
               str(481516+faction),'--humans','2','--setup',setup]
    fingerprints = set()
    for dirty in [False,True,False,True]:
        env = {**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'}
        if dirty: env.update(MallocScribble='1',MALLOC_PERTURB_='165')
        run = subprocess.run(command,input='state 0\nstate 1\nquit\n',text=True,
                             capture_output=True,env=env,check=True,timeout=15)
        states = [json.loads(line) for line in run.stdout.splitlines() if json.loads(line).get('type')=='state']
        assert len(states)==2
        fingerprints.add(hashlib.sha256(json.dumps(states,sort_keys=True).encode()).hexdigest())
    results.append({'faction':faction,'runs':4,'fingerprints':len(fingerprints)})
    assert len(fingerprints)==1, results[-1]
out = ROOT/'test-results/match-history/initial-stability.json'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps({'passed':True,'runs':36,'cases':results},indent=2))
print(out.read_text())
