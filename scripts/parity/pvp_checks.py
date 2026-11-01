"""Manual PvP: both result windows, loot, level-up, loss and real continuation."""
import argparse
import copy
import json
from pathlib import Path
from compare import first_difference


def check(traces, winner):
    checks=[]
    def record(name, passed, detail=None):
        checks.append({'name':name,'passed':bool(passed),**({'detail':detail} if detail else {})})
    def state(mode, action, seat):
        return traces[mode]['actions'][f'{action}/p{seat}']['state']
    left,right=traces['legacy'],traces['authority']
    record('same save, RNG, build, observer and settings',all(left.get(k)==right.get(k) for k in
           ['fixtureSha256','rng','initial','wasmSha256','observerSha256','settings','viewport','buildConfig']))
    for mode,trace in traces.items():
        record(mode+': complete with real rendered frames',not trace.get('failure') and not trace.get('overflow') and
               not trace.get('errors') and all(a['frames'] for a in trace['actions'].values()))
        for seat in [0,1]:
            result=state(mode,'loser-results-ready',seat)
            record(f'{mode}/p{seat}: original results window',result['window']['type']=='20TCombatResultsWindow')
            settled=state(mode,'settled',seat)
            record(f'{mode}/p{seat}: combat and modals closed',settled['screen']=='adventure' and not settled['game']['combat'] and settled['layers']==0)
            record(f'{mode}/p{seat}: control matches adventure turn',settled['adventure']['cursor']==(seat==winner==0))
        record(mode+': loot before level-up',state(mode,'winner-dismiss',winner)['window']['type']=='10TDialogBox' and
               state(mode,'loot-dismiss',winner)['window']['type']=='14TLevelUpWindow')
        s=state(mode,'settled',winner);heroes=s['world']['heroes']
        rewards=state(mode,'winner-move',winner)['world']['heroes'] if mode=='legacy' and winner else heroes
        record(mode+': winner keeps army and earns experience/level',len(rewards)==1 and rewards[0]['army'][0]==[41,20] and rewards[0]['experience']==1509 and rewards[0]['level']==2)
        record(mode+': captured sword in winner inventory',len(rewards)==1 and any(a[0]==11 for a in rewards[0]['equipped']+rewards[0]['backpack']))
        record(mode+': defeated hero removed',not state(mode,'settled',1-winner)['world']['heroes'])
        moved=state(mode,'winner-move',winner);after=moved['world']['heroes']
        record(mode+': winner really moves on own turn',moved['game']['player']==winner and moved['adventure']['cursor'] and len(after)==1 and
               after[0]['pos']==[6 if winner else 5,8,0] and after[0]['movement']<heroes[0]['movement'])
    # The original waiting defender reloads the pre-battle cache until its own
    # turn. Retain that raw mismatch, but require rewards on its first real turn.
    for action,seats in [('settled',[0] if winner else [0,1]),('winner-move',[winner])]:
        for seat in seats:
            diff=first_difference(state('legacy',action,seat)['world'],state('authority',action,seat)['world'])
            record(f'{action}/p{seat}: canonical rewards and armies match',diff is None,diff)
    for action in ['winner-dismiss','loot-dismiss']:
        diff=first_difference(state('legacy',action,winner)['window'],state('authority',action,winner)['window'])
        record(action+': native dialog text, controls and colors match',diff is None,diff)
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'intermediateDefenderWorldDifference':first_difference(state('legacy','settled',1)['world'],state('authority','settled',1)['world']) if winner else None}


def check_run(directory,winner):
    traces={m:json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
    result=check(traces,winner)
    if result['passed']:
        faults={}
        for name in ['missing-loot','missing-level','lost-control','dead-hero-retained']:
            broken=copy.deepcopy(traces);actions=broken['authority']['actions']
            s=actions[f'settled/p{winner}']['state']
            if name=='missing-loot':
                for artifact in s['world']['heroes'][0]['equipped']+s['world']['heroes'][0]['backpack']:
                    if artifact[0]==11:artifact[0]=-1
            elif name=='missing-level':s['world']['heroes'][0]['level']=1
            elif name=='lost-control':actions[f'winner-move/p{winner}']['state']['adventure']['cursor']=False
            else:actions[f'settled/p{1-winner}']['state']['world']['heroes']=copy.deepcopy(s['world']['heroes'])
            faults[name]=not check(broken,winner)['passed']
        result['faultsDetected']=faults;result['passed'] &= all(faults.values())
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path);parser.add_argument('--winner',choices=['attacker','defender'],required=True)
    args=parser.parse_args();result=check_run(args.directory,int(args.winner=='defender'))
    (args.directory/'pvp-checks.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'passed':result['passed'],'failed':[c for c in result['checks'] if not c['passed']],'faults':result.get('faultsDetected')}))
    raise SystemExit(0 if result['passed'] else 1)
