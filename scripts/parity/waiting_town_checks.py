"""Read-only town/hero inspection must preserve waiting adventure control."""
import argparse
import copy
import json
from pathlib import Path


def check(trace):
    checks=[]
    def record(name, passed):checks.append({'name':name,'passed':bool(passed)})
    def state(name, seat=1):return trace['actions'][f'{name}/p{seat}']['state']
    def gameplay(world):
        # Opening the original town window materializes its free decorative
        # building cache. Compare the expanded building mask and all gameplay
        # fields here; the strict trace comparator still compares the raw cache.
        return {**world,'towns':[{k:v for k,v in t.items() if k!='storedBuildings'} for t in world['towns']]}
    record('complete run without browser errors',not trace.get('failure') and not trace.get('overflow') and not trace.get('errors'))
    initial=state('idle');original=initial['world']
    for name in ['waiting-town-open','waiting-hero-open']:
        record(name+': original dialog opened',state(name)['layers']==1)
    for name in ['waiting-town-close','waiting-hero-close','waiting-move-blocked','host-move']:
        s=state(name);a=s['adventure']
        # Legacy's global mobile flag can describe the remote acting hero;
        # its local waiting hero must still occupy its map cell with no cursor.
        # Authority has only the seat's presentation and must clear both flags.
        record(name+': no local control or active cursor',s['layers']==0 and not a['motion']['local'] and not a['cursor'] and (trace['mode']=='legacy' or not a['mobile']))
        record(name+': hero remains in its canonical map cell',all(h['onMap'] for h in a['heroes']))
        record(name+': army, position, buildings and resources unchanged',gameplay(s['world'])==gameplay(original))
        record(name+': ambient effects stay disabled',not a['sound']['ambient'])
    s=state('guest-move');a=s['adventure']
    record('next turn grants real movement',s['game']['player']==1 and a['motion']['local'] and a['mobile'] and a['cursor'] and
           s['world']['heroes'][0]['pos']==[4,30,0] and s['world']['heroes'][0]['movement']<original['heroes'][0]['movement'])
    record('active player can enter the town',state('guest-town-entry')['layers']==1)
    a=state('guest-town-close')['adventure']
    record('active town exit restores the mobile hero',state('guest-town-close')['layers']==0 and a['mobile'] and a['cursor'] and a['motion']['local'])
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'scope':'Waiting/active control lifecycle. Legacy remote mobile state stays in the strict comparator; this gate checks local occupancy/control.'}


def seeded_faults(trace):
    if not check(trace)['passed']:raise ValueError('Fault controls require a complete passing trace')
    results=[]
    for name in ['active-waiting-cursor','missing-map-hero','move-on-wrong-turn','no-control-on-own-turn']:
        candidate=copy.deepcopy(trace)
        actions=candidate['actions']
        if name=='active-waiting-cursor':
            a=actions['waiting-town-close/p1']['state']['adventure'];a['mobile']=True;a['cursor']=True
        elif name=='missing-map-hero':actions['waiting-hero-close/p1']['state']['adventure']['heroes'][0]['onMap']=False
        elif name=='move-on-wrong-turn':actions['waiting-move-blocked/p1']['state']['world']['heroes'][0]['pos'][1]+=1
        else:actions['guest-move/p1']['state']['adventure']['cursor']=False
        result=check(candidate)
        results.append({'fault':name,'passed':not result['passed'],'detectedBy':[c['name'] for c in result['checks'] if not c['passed']]})
    return {'passed':all(r['passed'] for r in results),'checks':results}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('--faults',action='store_true');args=parser.parse_args()
    result=(seeded_faults if args.faults else check)(json.loads((args.directory/'authority/trace.json').read_text()))
    (args.directory/('waiting-town-faults.json' if args.faults else 'waiting-town-checks.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
