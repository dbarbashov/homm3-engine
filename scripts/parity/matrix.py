#!/usr/bin/env python3
"""Run the scenario matrix, reference repeat and seeded faults on one fixture.
Nonzero exit means divergence OR a failed harness gate; summary.json separates them.
Build and generate a combat fixture once before running this command.
"""
import argparse
import json
import subprocess
import sys
from pathlib import Path
from compare import compare, report
from coverage import adventure, combat_regressions
from seeded_faults import check
from transition_checks import check_run

ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--fixture',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--runtime',type=Path,default=ROOT.parent/'homm3-web/.local/runtime')
p.add_argument('--site-root',type=Path,default=ROOT.parent/'homm3-web')
p.add_argument('--relay',type=Path,default=ROOT.parent/'homm3-lobby/target/debug/homm3-lobby')
p.add_argument('--worker',type=Path,default=ROOT/'build/cmake-parity-native/homm3-worker')
p.add_argument('--web',type=Path,default=ROOT/'build/cmake-parity-web/web')
p.add_argument('--quick',action='store_true',help='Combat + reference repeat + seeded faults; omit long town/week scenarios')
args=p.parse_args();args.out=args.out.resolve();args.fixture=args.fixture.resolve()
if args.out.exists() and any(args.out.iterdir()):raise SystemExit('Choose a new output directory to preserve prior evidence')
args.out.mkdir(parents=True,exist_ok=True)
summary={'harnessPassed':True,'parityPassed':True,'runs':{}}
suites=['combat'] if args.quick else ['combat','adventure','turns']
for suite in suites+['repeat-'+name for name in suites]:
    repeating=suite.startswith('repeat-');scenario=suite.removeprefix('repeat-')
    out=args.out/suite
    command=[sys.executable,str(ROOT/'scripts/parity/run.py'),'--fixture',str(args.fixture),'--out',str(out)]
    command+=['--scenario',str(ROOT/'scripts/parity/scenarios/combat.json')] if scenario=='combat' else ['--suite',scenario,'--virtual-ms','512' if scenario=='turns' else '60000']
    for key in ['runtime','site_root','relay','worker','web']:command += ['--'+key.replace('_','-'),str(getattr(args,key).resolve())]
    run=subprocess.run(command,cwd=ROOT)
    traces={mode:json.loads((out/mode/'trace.json').read_text()) for mode in ['legacy','authority'] if (out/mode/'trace.json').exists()}
    complete=len(traces)==2 and all(not t.get('failure') and not t.get('overflow') for t in traces.values())
    summary['harnessPassed'] &= complete
    result=json.loads((out/'comparison.json').read_text()) if (out/'comparison.json').exists() else {'passed':False}
    summary['runs'][suite]={'complete':complete,'exitCode':run.returncode,'parity':result['passed'],'report':suite+'/index.html'}
    if not repeating:summary['parityPassed'] &= result['passed']
    if scenario=='combat' and complete and not repeating:
        regressions=combat_regressions(traces['legacy'],traces['authority'])
        (out/'regressions.json').write_text(json.dumps(regressions,indent=2))
        summary['runs'][suite]['regressionsPassed']=regressions['passed']
        summary['parityPassed'] &= regressions['passed']
    if scenario=='adventure' and complete:
        coverage={mode:adventure(t) for mode,t in traces.items()};(out/'coverage.json').write_text(json.dumps(coverage,indent=2))
        summary['harnessPassed'] &= coverage['legacy']['passed'];summary['parityPassed'] &= coverage.get('authority',{'passed':True})['passed']
    if scenario=='turns' and complete:
        transitions=check_run(out,'turns')
        (out/'transition-checks.json').write_text(json.dumps(transitions,indent=2)+'\n')
        summary['runs'][suite]['transitionsPassed']=transitions['passed']
        summary['parityPassed'] &= transitions['passed']
try:
    summary['repeatability']={}
    for suite in suites:
        summary['repeatability'][suite]={}
        for mode in ['legacy','authority']:
            reference=json.loads((args.out/suite/mode/'trace.json').read_text());repeated=json.loads((args.out/('repeat-'+suite)/mode/'trace.json').read_text())
            repeat=compare(reference,repeated)
            (args.out/(suite+'-'+mode+'-repeatability.json')).write_text(json.dumps(repeat,indent=2))
            summary['repeatability'][suite][mode]={'passed':repeat['passed'],'unstableChannels':sorted({f['channel'] for f in repeat['failures']})}
            summary['harnessPassed'] &= repeat['passed']
    faults=check(json.loads((args.out/'combat/legacy/trace.json').read_text()))
    (args.out/'seeded-faults.json').write_text(json.dumps(faults,indent=2))
    summary['seededFaults']=faults['passed'];summary['harnessPassed'] &= faults['passed']
except (OSError,ValueError,KeyError,StopIteration) as error:summary['harnessPassed']=False;summary['validationError']=repr(error)
(args.out/'summary.json').write_text(json.dumps(summary,indent=2))
failures=[]
if not summary['harnessPassed']:failures.append({'channel':'harness','error':'Incomplete run, reference instability, coverage or fault detector failure; inspect summary'})
if not summary['parityPassed']:failures.append({'channel':'parity','error':'Legacy/authority differences remain; open the individual run reports'})
report({'passed':not failures,'failures':failures},args.out/'index.html',summary)
print(json.dumps(summary,indent=2));raise SystemExit(0 if not failures else 1)
