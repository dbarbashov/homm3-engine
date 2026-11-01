"""Verify actual turn notifications and calendar audiovisual continuations."""
import argparse
import json
from pathlib import Path


def check(directory):
    traces={mode:json.loads((Path(directory)/mode/'trace.json').read_text()) for mode in ['legacy','authority']}
    checks=[]
    def record(name,value):checks.append({'name':name,'passed':bool(value)})
    def sample_events(trace,name):
        return [(key,e) for key,a in trace['actions'].items() for e in a['audioEvents']
                if e['kind']=='homm3Sfx.play' and (e.get('data') or {}).get('name','').lower()==name]
    for mode,t in traces.items():
        record(mode+': complete week without errors',not t.get('failure') and not t.get('overflow') and not t.get('errors'))
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
    events=[sample_events(t,'newweek.wav') for t in traces.values()]
    if all(len(e)==1 for e in events):
        states=[t['actions'][e[0][0]]['state'] for t,e in zip(traces.values(),events)]
        record('original week text, buttons and colors',states[0]['window']==states[1]['window'])
        record('original week sample playback parameters',events[0][0][1]['data']==events[1][0][1]['data'])
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'scope':'Event-aligned calendar presentation; strict pixels, PCM and transport timing remain in comparison.json.'}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
    result=check(args.directory);(args.directory/'calendar-checks.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
