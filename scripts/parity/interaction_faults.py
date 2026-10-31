"""Validate interaction checks with faults in actual paired traces."""
import argparse
import copy
import json
from pathlib import Path
from interaction_checks import check_run


def mutate(traces, fault):
    actions=traces['authority']['actions']
    if fault=='stale-cross':actions['settled/p0']['state']['adventure']['motion']['showRoute']=True
    elif fault=='horse-during-exchange':
        a=actions['meet/p0'];a['audioEvents']=[e for e in a['audioEvents'] if not (e['kind']=='homm3Sfx.stop' and e['data']['name'].lower().startswith('horse'))]
    elif fault=='hidden-pointer':actions['close/p0']['state']['adventure']['motion']['pointerVisible']=False
    elif fault=='generic-shipyard':actions['visit/p0']['state']['window']['type']='10TDialogBox'
    elif fault=='double-charge':actions['blocked-close/p0']['state']['world']['resources'][6]-=1000
    elif fault=='late-shipyard-sound':
        a=actions['visit/p0'];a['audioEvents']=[e for e in a['audioEvents'] if not(e['kind']=='homm3Sfx.play' and e['data'].get('name')=='Store.wav')]
    elif fault=='second-battle-closed':actions['second-ready/p0']['state']['game']['combat']=0
    elif fault=='second-battle-no-control':actions['second-ready/p0']['state']['combat']['control']=False
    elif fault=='second-battle-stale-army':actions['second-ready/p0']['state']['combat']['stacks'][0]['count']=1
    else:raise ValueError(fault)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for suite in ['shipyard','pickup','exchange','battles']:p.add_argument('--'+suite,type=Path,required=True)
    p.add_argument('--out',type=Path,required=True);a=p.parse_args();results=[]
    cases={'shipyard':['generic-shipyard','double-charge','late-shipyard-sound'],'pickup':['stale-cross'],'exchange':['horse-during-exchange','hidden-pointer'],'battles':['second-battle-closed','second-battle-no-control','second-battle-stale-army']}
    for suite,faults in cases.items():
        directory=getattr(a,suite);traces={m:json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
        assert check_run(directory,suite,traces)['passed'], 'Baseline failed: '+suite
        for fault in faults:
            broken=copy.deepcopy(traces);mutate(broken,fault);result=check_run(directory,suite,broken)
            results.append({'fault':fault,'detected':not result['passed'],'failedChecks':[c['name'] for c in result['checks'] if not c['passed']]})
    result={'passed':all(r['detected'] for r in results),'faults':results};a.out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
