"""Verify actual turn notifications and calendar audiovisual continuations."""
import argparse
import copy
import json
from pathlib import Path


def check_traces(traces):
    checks=[]
    def record(name,value):checks.append({'name':name,'passed':bool(value)})
    def sample_events(trace,name):
        return [(key,e) for key,a in trace['actions'].items() for e in a['audioEvents']
                if e['kind']=='homm3Sfx.play' and (e.get('data') or {}).get('name','').lower()==name]
    for mode,t in traces.items():
        record(mode+': complete week without errors',not t.get('failure') and not t.get('overflow') and not t.get('errors'))
        record(mode+': initial AI aggression is retained',len(t['initial'].get('heroAggressionBits',[]))==156)
        events=sample_events(t,'newweek.wav')
        record(mode+': exactly one new-week sample',len(events)==1)
        if events:
            name,_=events[0];a=t['actions'][name];s=a['state']
            record(mode+': week sound accompanies native calendar dialog',s['window']['type']=='10TDialogBox' and s['game']['week']==2 and s['game']['day']==1)
            record(mode+': calendar animates under its acknowledgement',any(
                (f.get('ui',{}).get('window') or {}).get('type')=='10TDialogBox' and
                (f.get('ui',{}).get('adventure') or {}).get('bottom',{}).get('type')==1
                for f in a['frames']))
        for n in range(1,15):
            a=t['actions'][f'turn-popup-{n:02}/p{n%2}']
            record(mode+f': turn {n} notification once',sum(e['kind']=='homm3Sfx.play' and (e.get('data') or {}).get('name','').lower()=='sysmsg.wav' for e in a['audioEvents'])==1)
            record(mode+f': turn {n} cursor inactive under notification',not a['state']['adventure']['cursor'])
        for seat in range(2):
            initial=t['actions'][f'idle/p{seat}']['state']['world']
            final=t['actions'][f'settled-14/p{seat}']['state']['world']
            record(mode+f'/p{seat}: two real tavern offers',len(final.get('recruits') or [])==2 and all(h>=0 for h in final['recruits']))
            record(mode+f'/p{seat}: recruit armies observed',len(final.get('recruitArmies') or [])==2 and all(len(a)==7 and any(count>0 for _,count in a) for a in final['recruitArmies']))
            before={town['id']:town['population'] for town in initial['towns']}
            record(mode+f'/p{seat}: weekly creature growth occurred',any(town['id'] in before and sum(town['population'])>sum(before[town['id']]) for town in final['towns']))
    for n in range(1,15):
        for seat in range(2):
            a,b=[t['actions'][f'settled-{n:02}/p{seat}']['state']['world'] for t in traces.values()]
            for field in ['recruits','recruitArmies']:
                record(f'turn {n}/p{seat}: identical {field}',a.get(field) is not None and a[field]==b.get(field))
            record(f'turn {n}/p{seat}: identical town populations',[(t['id'],t['population']) for t in a['towns']]==[(t['id'],t['population']) for t in b['towns']])
    for n in range(1,15):
        for key in [f'turn-popup-{n:02}/p{1-n%2}',f'turn-ack-{n:02}/p{n%2}']:
            themes=[[e['data'][0] for e in t['actions'][key]['audioEvents']
                     if e['kind']=='homm3Music.play' and e['data'][0].startswith('AITheme')]
                    for t in traces.values()]
            # Legacy resolves AI turns locally and restarts the track for each
            # intermediate actor. Compare the settled theme; raw events remain.
            record(key+': same settled waiting theme',bool(themes[0])==bool(themes[1]) and
                   (not themes[0] or themes[0][-1]==themes[1][-1]))
    events=[sample_events(t,'newweek.wav') for t in traces.values()]
    if all(len(e)==1 for e in events):
        states=[t['actions'][e[0][0]]['state'] for t,e in zip(traces.values(),events)]
        record('original week text, buttons and colors',states[0]['window']==states[1]['window'])
        record('original week sample playback parameters',events[0][0][1]['data']==events[1][0][1]['data'])
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'scope':'Event-aligned calendar presentation; strict pixels, PCM and transport timing remain in comparison.json.'}


def rng_check(records):
    left,right=records["legacy"],records["authority"]
    return bool(left) and len(left)==9 and left==right and sum(e["kind"]=="calendar-day-start" for e in left)==7


def check(directory):
    traces={mode:json.loads((Path(directory)/mode/'trace.json').read_text()) for mode in ['legacy','authority']}
    result=check_traces(traces)
    directory=Path(directory)
    records={'legacy':[],'authority':[]}
    # The last human is the original transport's canonical AI/calendar owner.
    for step in json.loads((directory/'legacy/scenario.json').read_text()):
        chunk=json.loads((directory/'legacy'/(step['name']+'-p1.json')).read_text())
        records['legacy'].extend(e['data'] for e in chunk['events'] if e['kind']=='decision' and e['data']['kind'].startswith('calendar-'))
    records['authority']=[e for line in (directory/'authority/decisions.jsonl').read_text().splitlines()
                          if (e:=json.loads(line))['kind'].startswith('calendar-')]
    valid=rng_check(records)
    result['checks'].append({'name':'exact rules RNG at every day/week boundary','passed':valid})
    result['calendarRng']=records
    result['passed'] &= valid
    if result['passed']:
        faults={}
        for field in ['recruits','recruitArmies','population','initial-aggression']:
            original=traces['authority'];action=original['actions']['settled-14/p0']
            world=copy.deepcopy(action['state']['world'])
            broken={**traces,'authority':{**original,'initial':dict(original['initial']),
                'actions':{**original['actions'],'settled-14/p0':{**action,'state':{**action['state'],'world':world}}}}}
            if field=='recruits':world['recruits'][0]=-1
            elif field=='recruitArmies':world['recruitArmies'][0][0][1]+=1
            elif field=='population':world['towns'][0]['population'][0]+=1
            else:broken['authority']['initial'].pop('heroAggressionBits',None)
            faults[field]=not check_traces(broken)['passed']
        broken=copy.deepcopy(records);broken['authority'][0]['rng']=str(int(broken['authority'][0]['rng'])^1)
        faults['rng-boundary']=not rng_check(broken)
        result['faultsDetected']=faults;result['passed'] &= all(faults.values())
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
    result=check(args.directory);(args.directory/'calendar-checks.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
