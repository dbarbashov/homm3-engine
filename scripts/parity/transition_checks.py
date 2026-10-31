"""Focused turn/combat regressions. Does not replace the strict trace comparator."""
import argparse
import json
from pathlib import Path
from compare import first_difference


def check_run(directory, suite):
    directory=Path(directory);checks=[]
    def check(name, passed, detail=None):
        checks.append({'name':name,'passed':bool(passed),**({'detail':detail} if detail else {})})
    try:
        traces={m:json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
        for mode,t in traces.items():
            steps=json.loads((directory/mode/'scenario.json').read_text())
            keys={s['name']+f'/p{i}' for s in steps for i in range(2)}
            check(mode+': complete trace',not t.get('failure') and not t.get('overflow') and set(t['actions'])==keys,t.get('failure'))
            check(mode+': no browser errors',not t.get('errors'))
            check(mode+': heroes visible',all(c['passed'] for a in t['actions'].values() for c in a['invariants']))
        left,right=traces.values()
        check('same GM2, RNG, initial world and observer',all(left[k]==right[k] for k in ['fixtureSha256','rng','initial','wasmSha256','observerSha256','viewport','settings','buildConfig']))
        def state(mode,name,seat=0):return traces[mode]['actions'][f'{name}/p{seat}']['state']
        if suite=='week':
            for n in range(1,15):
                next_seat=n%2;popup=f'turn-popup-{n:02}';settled=f'settled-{n:02}'
                a,b=[state(m,popup,next_seat) for m in traces]
                check(popup+': original modal widgets',a['window']==b['window'],first_difference(a['window'],b['window']))
                for seat in range(2):
                    a,b=[state(m,settled,seat) for m in traces]
                    check(settled+f'/p{seat}: owned world',a['world']==b['world'],first_difference(a['world'],b['world']))
                    check(settled+f'/p{seat}: calendar and active player',a['game']==b['game'])
                    if seat!=next_seat:
                        check(settled+f'/p{seat}: original waiting selection',all(a['adventure'][k]==b['adventure'][k] for k in ['hero','town','mobile','cursor']))
                        check(settled+f'/p{seat}: sending message',a['adventure']['bottom']==b['adventure']['bottom'] and bool(b['adventure']['bottom']['text']))
                for mode in traces:
                    # Receiving feedback must exist before the popup, not only
                    # as stale text in the final resource panel model.
                    chunk=json.loads((directory/mode/f'{popup}-p{next_seat}.json').read_text())
                    check(mode+': receiving panel '+popup,any((f.get('ui',{}).get('adventure') or {}).get('bottom',{}).get('type')==5 and (f.get('ui',{}).get('adventure') or {}).get('bottom',{}).get('text')=='Принимаются данные...' for f in chunk['frames']))
            for mode in traces:
                game=state(mode,'settled-14')['game'];check(mode+': second week reached',(game['month'],game['week'],game['day'])==(1,2,1))
        else:
            for name in ['battle-wait','battle-defend','cast-haste']:
                a,b=[state(m,name)['combat'] for m in traces]
                def poses(c):return [(s['side'],s['slot'],s['frame'],s['sequence']) for s in c['stacks']]
                check(name+': original idle poses',poses(a)==poses(b),{'legacy':poses(a),'authority':poses(b)})
                check(name+': same active stack',(a['side'],a['slot'],a['round'])==(b['side'],b['slot'],b['round']))
            for seat in range(2):
                a,b=[state(m,'battle-dismiss',seat) for m in traces]
                check(f'p{seat}: original final armies and rewards',a['world']==b['world'])
                check(f'p{seat}: back to adventure',a['game']['combat']==b['game']['combat']==0)
            for mode in traces:
                a=state(mode,'cast-haste')['combat'];stack=next(s for s in a['stacks'] if s['side']==s['slot']==0)
                check(mode+': Haste visible on target',stack['effects']==1)
                sounds=[e for e in traces[mode]['actions']['cast-haste/p0']['audioEvents'] if isinstance(e.get('data'),dict) and e['data'].get('name')=='Tailwind.wav']
                check(mode+': Haste starts and stops once',len(sounds)==2)
    except (OSError,KeyError,ValueError,StopIteration) as error:
        check('complete readable evidence',False,repr(error))
    return {'passed':all(c['passed'] for c in checks),'scope':suite+' focused regressions; raw combat positions, timing, pixels and PCM remain strict in comparison.json','checks':checks}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--suite',choices=['week','combat'],required=True);a=p.parse_args()
    result=check_run(a.directory,a.suite);(a.directory/'transition-checks.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
