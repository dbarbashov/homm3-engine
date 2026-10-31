#!/usr/bin/env python3
"""Real two-client adventure matrix on identical GM2/RNG fixtures, silently.

Functional checks and strict audiovisual parity remain separate. The week
scenario intentionally reports unresolved intermediate transition differences.
Licensed fixtures and raw traces are retained only in ignored test-results/.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import sys
from adventure_checks import check_run

ROOT = Path(__file__).resolve().parents[2]
OBJECTS = {'chest':101, 'chest-experience':101, 'resource':79, 'artifact':5,
           'attack':51, 'defense':23, 'knowledge':32, 'experience':100,
           'school':61, 'windmill':112, 'water-wheel':109, 'garden':55,
           'oasis':56, 'youth':31, 'luck':30, 'ring':28, 'mine':53, 'gate':103, 'obelisk':57}
SPELLS = ['visions', 'view-earth', 'view-air', 'disguise', 'fly', 'water-walk',
          'dimension-door', 'dimension-door-cancel', 'town-portal',
          'summon-no-water', 'scuttle-cancel']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--cases', nargs='+', choices=[*OBJECTS, *SPELLS, 'week'],
                        default=[*OBJECTS, *SPELLS, 'week'])
    parser.add_argument('--jobs', type=int, choices=[1,2], default=1)
    parser.add_argument('--browser', choices=['chromium','webkit'], default='chromium')
    args = parser.parse_args()
    args.out = args.out.resolve()
    if args.out.exists() and any(args.out.iterdir()):
        parser.error('Use a new output directory to retain evidence')
    args.out.mkdir(parents=True, exist_ok=True)
    fixtures = {}
    for name in args.cases:
        key = str(OBJECTS[name]) if name in OBJECTS else 'base' if name=='week' else 'magic'
        if key not in fixtures:
            fixture = args.out/'fixtures'/key
            flags = ['--object',key] if name in OBJECTS else ['--combat'] if key=='base' else ['--magic']
            if name=='obelisk':flags+=['--natural-setup','--map','maps/All for One.h3m']
            with (args.out/f'fixture-{key}.log').open('w') as log:
                subprocess.run([sys.executable,str(ROOT/'scripts/parity/fixture.py'),
                                *flags,'--out',str(fixture)],cwd=ROOT,
                               stdout=log,stderr=subprocess.STDOUT,check=True)
            fixtures[key] = fixture

    def run(name):
        key = str(OBJECTS[name]) if name in OBJECTS else 'base' if name=='week' else 'magic'
        fixture = fixtures[key]
        scenario_name = 'visit' if name in OBJECTS and name not in ['mine','obelisk'] else name
        steps = json.loads((ROOT/f'scripts/parity/scenarios/adventure/{scenario_name}.json').read_text())
        if name in OBJECTS:
            meta = json.loads((fixture/'manifest.json').read_text())['adventure']
            next(s for s in steps if s['name']=='leave')['when'] = {
                'game.x':meta['target'][0], 'game.y':meta['target'][1]}
            if name in ['chest','chest-experience']:
                steps.insert(2,{'name':'choose-reward','widget':30730 if name=='chest-experience' else 30729,
                                'when':{'window.type':'10TDialogBox'}})
        scenario = args.out/(name+'-scenario.json')
        scenario.write_text(json.dumps(steps,indent=2)+'\n')
        out = args.out/name
        with (args.out/(name+'.log')).open('w') as log:
            process = subprocess.run([sys.executable,str(ROOT/'scripts/parity/run.py'),
                                      '--browser',args.browser,'--fixture',str(fixture),
                                      '--scenario',str(scenario),'--out',str(out)],
                                     cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        out.mkdir(exist_ok=True)
        result = check_run(out,fixture)
        result.update(scenario=name,exitCode=process.returncode)
        (out/'adventure-checks.json').write_text(json.dumps(result,indent=2)+'\n')
        print(name,'PASS' if result['passed'] else 'FAIL',flush=True)
        return name,result

    summary = {'functionalPassed':True,'runs':{}}
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for name,result in pool.map(run,args.cases):
            summary['runs'][name]=result
            summary['functionalPassed'] &= result['passed']
            (args.out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    return 0 if summary['functionalPassed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
