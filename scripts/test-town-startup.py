"""Regression for a real native startup crash in the tavern rumour lookup."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime', required=True, type=Path)
p.add_argument('--build', type=Path, default=ROOT / 'build/cmake-server')
p.add_argument('--map', default='maps/Arrogance.h3m')
a = p.parse_args()
# Exact room setup/seed that selected the invalid 256th (one-based) index.
setup = '1,0,4,-1,3,0,1,1,-1,3,0,-1,-1,-1,3,0,-1,-1,-1,3,0,-1,-1,-1,3,0,10,3,-1,3,0,10,5,-1,3,0,-1,-1,-1,3,0'
env = {k: v for k, v in os.environ.items() if not k.startswith('HOMM3_HARNESS_')}
env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
results = []
for seed in [1861560145, 0, 42, 481516, 2147483647]:
    run = subprocess.run([str(a.build.resolve() / 'homm3-worker'), str(a.runtime.resolve()), a.map,
                          str(seed), '--humans', '2', '--setup', setup],
                         input='', text=True, capture_output=True, env=env, timeout=30)
    messages = [json.loads(line) for line in run.stdout.splitlines() if line.startswith('{')]
    results.append({'seed': seed, 'returncode': run.returncode,
                    'passed': run.returncode == 0 and any(m.get('type') == 'ready' for m in messages),
                    'stderr': run.stderr})
result = {'passed': all(r['passed'] for r in results), 'cases': results}
out = ROOT / 'test-results/town-startup'
out.mkdir(parents=True, exist_ok=True)
(out / 'results.json').write_text(json.dumps(result, indent=2))
print(json.dumps(result))
raise SystemExit(0 if result['passed'] else 1)
