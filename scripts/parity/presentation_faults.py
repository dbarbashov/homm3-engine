"""Prove focused gates reject regressions injected into actual paired traces."""
import argparse
import copy
import json
from pathlib import Path
from presentation_checks import check_run


def mutate(traces, fault):
    actions=traces['authority']['actions']
    if fault=='brown-new-day':
        for arrow in actions['new-day-route/p0']['state']['adventure']['route']:arrow[3]+=25
    elif fault=='lost-destination':
        actions['exhausted-idle/p0']['state']['adventure']['heroes'][0]['target']=[-1,-1,-1]
    elif fault=='hidden-town-pointer':
        actions['walk-into-town/p0']['state']['adventure']['motion']['pointerVisible']=False
    elif fault=='missing-pickup':
        for action in actions.values():
            action['audioEvents']=[e for e in action['audioEvents'] if not (e['kind']=='homm3Sfx.play' and e['data'].get('loops')==1 and e['data']['name'].lower()!='button.wav')]
    elif fault=='reset-walk-cycle':
        for frame in actions['walk-route/p0']['frames']:
            adv=frame['ui']['adventure']
            if adv['motion']['moving'] and adv['heroes'][0]['x']==5:
                adv['motion']['frame']=0
                break
    elif fault=='defend-hit-animation':
        for frame in actions['battle-defend/p0']['frames']:
            combat=frame['ui'].get('combat')
            if combat and not frame.get('held'):
                stack=next(s for s in combat['stacks'] if s['side']==0 and s['slot']==1)
                stack['sequence']=4
                break
    elif fault=='hd-hero-ghost':
        for frame in actions['encounter-confirm/p0']['frames']:
            if frame['ui']['window']['type']=='13TCombatWindow' and not frame.get('held'):
                assert frame['width']>=864, 'Use an HD combat run'
                frame['tiles'][0]^=1
                break
    else:raise ValueError(fault)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for suite in ['movement','town','sound','speed','combat']:p.add_argument('--'+suite,type=Path,required=True)
    p.add_argument('--out',type=Path,required=True);args=p.parse_args()
    faults={'movement':['brown-new-day','lost-destination'],'town':['hidden-town-pointer'],'sound':['missing-pickup'],'speed':['reset-walk-cycle'],'combat':['defend-hit-animation','hd-hero-ghost']}
    results=[]
    for suite,names in faults.items():
        directory=getattr(args,suite)
        traces={mode:json.loads((directory/mode/'trace.json').read_text()) for mode in ['legacy','authority']}
        assert check_run(directory,suite,traces)['passed'], 'Baseline must pass: '+suite
        for name in names:
            broken=copy.deepcopy(traces);mutate(broken,name)
            result=check_run(directory,suite,broken)
            results.append({'fault':name,'detected':not result['passed'],'failedChecks':[c['name'] for c in result['checks'] if not c['passed']]})
    report={'passed':all(r['detected'] for r in results),'faults':results}
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    raise SystemExit(0 if report['passed'] else 1)
