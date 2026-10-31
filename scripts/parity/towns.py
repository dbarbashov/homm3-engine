#!/usr/bin/env python3
"""Nine-faction town UI matrix on rich identical-save fixtures, silently.
Each focused functional result is separate from strict audiovisual parity.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import sys
from town_checks import check_run

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--factions', type=int, nargs='+', choices=range(9), default=list(range(9)))
parser.add_argument('--scenarios', nargs='+', choices=['core', 'residents', 'services', 'rewards', 'transformer', 'university', 'portal'], default=['core', 'residents', 'services'])
parser.add_argument('--town-profile', choices=['basic', 'full'], default='basic')
parser.add_argument('--jobs', type=int, choices=[1, 2], default=1)
parser.add_argument('--browser', choices=['chromium', 'webkit'], default='chromium')
args = parser.parse_args()
special_factions = {'rewards': {0, 2, 3, 6, 7}, 'transformer': {4}, 'university': {8}, 'portal': {5}}
for scenario in args.scenarios:
    if scenario in special_factions and (args.town_profile != 'full' or not set(args.factions) <= special_factions[scenario]):
        parser.error(f'{scenario} requires --town-profile full and factions {sorted(special_factions[scenario])}')
args.out = args.out.resolve()
if args.out.exists() and any(args.out.iterdir()):
    raise SystemExit('Use a new output directory to retain evidence')
args.out.mkdir(parents=True, exist_ok=True)
for faction in args.factions:
    with (args.out / f'fixture-{faction}.log').open('w') as log:
        subprocess.run([sys.executable, str(ROOT / 'scripts/parity/fixture.py'), '--town', str(faction), '--town-profile', args.town_profile, '--out', str(args.out / 'fixtures' / str(faction))], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)


def run(job):
    faction, scenario = job
    name = f'{faction}-{scenario}'
    out = args.out / name
    with (args.out / (name + '.log')).open('w') as log:
        process = subprocess.run([sys.executable, str(ROOT / 'scripts/parity/run.py'), '--browser', args.browser, '--fixture', str(args.out / 'fixtures' / str(faction)), '--scenario', str(ROOT / f'scripts/parity/scenarios/towns/{scenario}.json'), '--out', str(out)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    result = check_run(out, scenario)
    result.update(faction=faction, scenario=scenario, exitCode=process.returncode)
    (out / 'town-checks.json').write_text(json.dumps(result, indent=2))
    print(name, 'PASS' if result['passed'] else 'FAIL', flush=True)
    return name, result

summary = {'functionalPassed': True, 'runs': {}}
with ThreadPoolExecutor(max_workers=args.jobs) as pool:
    for name, result in pool.map(run, [(f, s) for f in args.factions for s in args.scenarios]):
        summary['runs'][name] = result
        summary['functionalPassed'] &= result['passed']
        (args.out / 'summary.json').write_text(json.dumps(summary, indent=2))
print(json.dumps({k: v for k, v in summary.items() if k != 'runs'}))
raise SystemExit(0 if summary['functionalPassed'] else 1)
