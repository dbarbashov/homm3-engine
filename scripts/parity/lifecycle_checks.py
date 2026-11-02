"""Event-aligned fizzle, waiting music and combat-result audio boundaries."""
import argparse
import copy
import json
from pathlib import Path


def check(chunks):
    checks=[];metrics={}
    def record(name,passed,detail=None):
        checks.append({'name':name,'passed':bool(passed),**({'detail':detail} if detail is not None else {})})
    for mode,actions in chunks.items():
        a=actions['battle-dismiss'];events=a['events']
        markers=lambda name:[e['t'] for e in events if e['kind']=='decision' and e['data']['kind']==name]
        starts,ends=markers('adventure-fizzle-start'),markers('adventure-fizzle-end')
        record(mode+': one real map fade',len(starts)==len(ends)==1)
        durations=[b-a for a,b in zip(starts,ends)];metrics[mode]={'fadeDurations':durations}
        if starts and ends:
            frames=[f for f in a['frames'] if not f.get('held') and starts[0]<=f['t']<=ends[0]]
            record(mode+': original eight-frame fade renders',len({tuple(f['tiles']) for f in frames})>=8 and durations==[576])
            plays=[e for e in events if e['kind']=='homm3Music.play']
            record(mode+': adventure music resumes after fade',len(plays)==1 and plays[0]['t']>=ends[0])
            record(mode+': fade sound starts with animation',sum(e['kind']=='homm3Sfx.play' and e['data']['name'].lower()=='killfade.82m' and e['t']==starts[0] for e in events)==1)
        record(mode+': map cursor visible after fade',a['ui']['adventure']['cursor'] and a['ui']['adventure']['motion']['pointerVisible'])
        battle=actions['battle-auto']['events']
        wins=[e for e in battle if e['kind']=='homm3Music.play' and e['data'][0]=='win battle']
        record(mode+': original result theme exactly once',len(wins)==1)
        if wins:
            t=wins[0]['t'];stops=[e for e in battle if e['t']==t and e['kind']=='homm3Music.stop']
            samples=[e for e in battle if e['t']==t and e['kind']=='homm3Sfx.stop' and e['data'].get('name')]
            record(mode+': battle music and samples stopped before results',bool(stops) and bool(samples) and battle.index(stops[-1])<battle.index(wins[0]))
        for action in ['move-out','approach','encounter','battle-dismiss']:
            waiting=actions[action+'-waiting']['ui']
            record(mode+'/'+action+': waiting listener keeps local music',waiting['music']['track']=='Rough' and waiting['adventure']['sound']['terrain']==5 and not waiting['adventure']['sound']['ambient'])
    record('same original fade duration',metrics['legacy']['fadeDurations']==metrics['authority']['fadeDurations'])
    return {'passed':all(c['passed'] for c in checks),'checks':checks,'metrics':metrics}


def run(directory):
    chunks={mode:{key:json.loads((directory/mode/(name+f'-p{seat}.json')).read_text())
                 for key,name,seat in [('battle-auto','battle-auto',0),('battle-dismiss','battle-dismiss',0)]+
                 [(name+'-waiting',name,1) for name in ['move-out','approach','encounter','battle-dismiss']]}
            for mode in ['legacy','authority']}
    result=check(chunks)
    if result['passed']:
        faults={}
        for name in ['missing-fade','early-music','remote-terrain','missing-result-stop']:
            broken=copy.deepcopy(chunks);a=broken['authority']
            if name=='missing-fade':a['battle-dismiss']['events']=[e for e in a['battle-dismiss']['events'] if not(e['kind']=='decision' and e['data']['kind'].startswith('adventure-fizzle'))]
            elif name=='early-music':
                for event in a['battle-dismiss']['events']:
                    if event['kind']=='homm3Music.play':event['t']=0
            elif name=='remote-terrain':a['move-out-waiting']['ui']['music']['track']='Dirt'
            else:a['battle-auto']['events']=[e for e in a['battle-auto']['events'] if e['kind']!='homm3Music.stop']
            faults[name]=not check(broken)['passed']
        result['faultsDetected']=faults;result['passed'] &= all(faults.values())
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
    result=run(args.directory);(args.directory/'lifecycle-checks.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'passed':result['passed'],'failed':[c for c in result['checks'] if not c['passed']],'faults':result.get('faultsDetected')}))
    raise SystemExit(0 if result['passed'] else 1)
