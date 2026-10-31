"""Focused interaction lifecycle gates; strict pixel/PCM parity remains separate."""
import argparse
import copy
import json
from pathlib import Path


def check_run(directory, suite, traces=None):
    directory=Path(directory);checks=[]
    def check(name, value):checks.append({'name':name,'passed':bool(value)})
    try:
        if traces is None:traces={m:json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
        ref,cand=traces['legacy'],traces['authority']
        for mode,t in traces.items():
            steps=json.loads((directory/mode/'scenario.json').read_text())
            check(mode+': complete',not t.get('failure') and not t.get('errors') and not t['overflow'] and set(t['actions'])=={s['name']+f'/p{i}' for s in steps for i in range(2)})
        check('identical fixture, RNG, settings and binaries',all(ref[k]==cand[k] for k in ['fixtureSha256','initial','rng','settings','viewport','wasmSha256','observerSha256']))
        def action(t,key):return t['actions'][key+'/p0']
        def state(t,key):return action(t,key)['state']
        def horse_stopped(t,key):
            active=set();played=False
            for e in action(t,key)['audioEvents']:
                if not isinstance(e['data'],dict):continue
                name=e['data'].get('name','').lower()
                if not name.startswith('horse'):continue
                if e['kind']=='homm3Sfx.play':active.add(name);played=True
                if e['kind']=='homm3Sfx.stop':active.discard(name)
            return played and not active
        def stable_window(t,key):
            w=copy.deepcopy(state(t,key)['window'])
            # The original ship sprite animates continuously; compare its
            # identity/geometry/sequence, not an arbitrary checkpoint phase.
            for widget in w['widgets']:
                if w['type']=='11TShipWindow' and widget['id']==6:widget.pop('frame',None)
            return w
        keys={'shipyard':['visit','cancel','revisit','buy','blocked-visit','blocked-close'],
              'pickup':['collect','settled','step-onto-resource'],
              'exchange':['meet','exchange-idle','close','move-away'],
              'battles':['battle-dismiss','second-ready','second-defend']}[suite]
        for key in keys:
            a,b=state(ref,key),state(cand,key)
            check(key+': owned world matches',a['world']==b['world'])
            check(key+': pointer visible',b['adventure']['motion']['pointerVisible'])
            check(key+': original window type',a['window']['type']==b['window']['type'])
        if suite=='shipyard':
            for key in ['visit','revisit','blocked-visit']:
                check(key+': original widgets, text and resources',stable_window(ref,key)==stable_window(cand,key))
                check(key+': horse stops',horse_stopped(cand,key))
                samples=[e['data'].get('name') for e in action(cand,key)['audioEvents'] if e['kind']=='homm3Sfx.play']
                check(key+': Store sound before closing',samples.count('Store.wav')==1)
            before=state(cand,'visit')['world']['resources'];after=state(cand,'buy')['world']['resources']
            check('cancel costs nothing',state(cand,'cancel')['world']['resources']==before)
            check('boat costs exactly 1000 gold and 10 wood',[x-y for x,y in zip(before,after)]==[10,0,0,0,0,0,1000])
            check('blocked berth cannot charge twice',state(cand,'blocked-close')['world']['resources']==after)
            def boats(mode,key):
                world=json.loads((directory/mode/(key+'-p0.json')).read_text())['worlds'][-1]['world']
                return [i for i,cell in enumerate(world['cells']) if cell[7]==8]
            before_boats=boats('authority','cancel');built=boats('authority','buy')
            check('exactly one boat created on original map cell',len(built)==len(before_boats)+1 and built==boats('legacy','buy'))
            check('blocked berth creates no duplicate boat',built==boats('authority','blocked-close'))
        elif suite=='pickup':
            for key in keys:
                check(key+': route hidden',not state(cand,key)['adventure']['motion']['showRoute'])
                for frame in action(cand,key)['frames']:
                    if key=='collect' or frame.get('held'):continue
                    check(key+': no cross in settled frame',not frame['ui']['adventure']['motion']['showRoute'])
            check('pickup stops horse',horse_stopped(cand,'collect'))
            check('resource target cleared',state(cand,'settled')['adventure']['heroes'][0]['target'][:2]==[-1,-1])
        elif suite=='exchange':
            check('horse stops before exchange idle',horse_stopped(cand,'meet'))
            check('original exchange stays open',state(cand,'exchange-idle')['window']['type']=='11TSwapWindow')
            check('close returns to map',state(cand,'close')['window']['type']=='19TAdventureMapWindow')
            check('input works after exchange',state(cand,'close')['world']['heroes']!=state(cand,'move-away')['world']['heroes'])
        elif suite=='battles':
            check('first battle closed',state(cand,'battle-dismiss')['game']['combat']==0)
            for key in ['second-ready','second-defend']:
                a,b=state(ref,key)['combat'],state(cand,key)['combat']
                check(key+': second battle active and controllable',state(cand,key)['game']['combat']==1 and b['control'])
                check(key+': original stacks and active unit',[(s['side'],s['slot'],s['count'],s['hex']) for s in a['stacks']]==[(s['side'],s['slot'],s['count'],s['hex']) for s in b['stacks']] and (a['side'],a['slot'])==(b['side'],b['slot']))
            check('second battle accepts defend',state(cand,'second-ready')['combat']['slot']!=state(cand,'second-defend')['combat']['slot'])
    except (OSError,KeyError,ValueError,IndexError,TypeError) as e:checks.append({'name':'complete readable evidence','passed':False,'detail':repr(e)})
    return {'passed':bool(checks) and all(c['passed'] for c in checks),'suite':suite,'checks':checks}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--suite',required=True,choices=['shipyard','pickup','exchange','battles']);a=p.parse_args()
    result=check_run(a.directory,a.suite);(a.directory/'interaction-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
