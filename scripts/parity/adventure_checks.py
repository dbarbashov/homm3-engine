"""Adventure semantic checks; strict frame/audio parity remains independent."""
import json
from pathlib import Path
from compare import first_difference


def check_run(directory, fixture):
    directory, fixture = Path(directory), Path(fixture)
    checks = []
    def check(name, value, detail=None):
        checks.append({'name': name, 'passed': bool(value), **({'detail': detail} if detail else {})})
    try:
        traces = {m: json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
        steps = json.loads((directory/'legacy/scenario.json').read_text())
        keys = {s['name']+f'/p{i}' for s in steps for i in range(2)}
        for mode,t in traces.items():
            check(mode+': completed', not t.get('failure') and not t.get('overflow') and set(t['actions'])==keys, t.get('failure'))
            check(mode+': hero visibility invariants',all(c['passed'] for action in t['actions'].values() for c in action.get('invariants',[])))
            check(mode+': browser errors', not t.get('errors') and json.loads((directory/mode/'errors.json').read_text())==[])
        if not all(c['passed'] for c in checks): return {'passed':False,'checks':checks}
        a,b=traces.values()
        check('identical browser/build/observer',all(a.get(k)==b.get(k) for k in ['wasmSha256','observerSha256','settings','viewport','buildConfig']))
        check('identical GM2/RNG/world', a['fixtureSha256']==b['fixtureSha256'] and a['rng']==b['rng'] and a['initial']==b['initial'])
        for key in sorted(keys):
            left,right=a['actions'][key]['state'],b['actions'][key]['state']
            diff=first_difference(left['world'],right['world']);check(key+': owned world',diff is None,diff)
            check(key+': calendar and actor',left['game']==right['game'])
            check(key+': modal type/layers',(left.get('window') or {}).get('type')==(right.get('window') or {}).get('type') and left['layers']==right['layers'])
            if left['layers'] and key.endswith('/p0'):
                diff=first_difference(left['window'],right['window']);check(key+': original dialog',diff is None,diff)
        manifest=json.loads((fixture/'manifest.json').read_text());meta=manifest.get('adventure')
        if meta:
            kind=meta['type'];identity=meta['hero']
            for mode,t in traces.items():
                def world(name):return t['actions'][name+'/p0']['state']['world']
                def hero(name):return next(h for h in world(name)['heroes'] if h['id']==identity)
                before,after,again=hero('idle'),hero('settled'),hero('settled-again')
                initial,resources=world('idle')['resources'],world('settled')['resources']
                if kind==57:
                    def ui(name,seat=0):return t['actions'][name+f'/p{seat}']['state']['adventure']
                    check(mode+': exactly one obelisk visited',sum(ui('idle')['obelisks'])==0 and sum(ui('settled')['obelisks'])==1)
                    check(mode+': repeat preserves visitation',ui('settled')['obelisks']==ui('settled-again')['obelisks'])
                    check(mode+': other seat learns no visitation',sum(ui('settled',1)['obelisks'])==0)
                    for name in ['accept','puzzle-close','settled','leave']:
                        left=a['actions'][name+'/p0']['state']['adventure'];right=b['actions'][name+'/p0']['state']['adventure']
                        check(mode+': original hero selection '+name,all(left[k]==right[k] for k in ['hero','mobile','cursor']))
                    check(mode+': hero moves after closing puzzle',hero('leave')['pos']!=after['pos'])
                if kind==101:
                    reward=next(step['widget'] for step in steps if step['name']=='choose-reward')
                    check(mode+': chosen chest reward only',(resources[6]-initial[6],after['experience']-before['experience'])==((0,500) if reward==30730 else (1000,0)))
                if kind in [79,112,109,55]:check(mode+': resources gained',sum(resources)>sum(initial))
                if kind==5:check(mode+': artifact gained',after['equipped']!=before['equipped'] or after['backpack']!=before['backpack'])
                if kind in [51,23,61,32]:
                    stat={51:0,23:1,61:2,32:3}[kind];check(mode+': primary skill +1',after['primary'][stat]==before['primary'][stat]+1)
                if kind==100:check(mode+': training experience',after['experience']-before['experience']==1000)
                if kind==102:check(mode+': tree level',after['level']==before['level']+1)
                if kind in [56,31]:
                    check(mode+': morale gained',after['moraleBonus']>before['moraleBonus'])
                    check(mode+': movement gained',after['movement']>before['movement'])
                if kind==28:check(mode+': luck gained',after['luckBonus']>before['luckBonus'])
                if kind==79:
                    bottom=[t['actions']['visit/p0']['state']['adventure']['bottom'] for t in traces.values()]
                    check(mode+': original resource panel',bottom[0]==bottom[1] and bottom[0]['type']==6 and bottom[0]['quantity']>0 and bool(bottom[0]['text']))
                if kind in [5,79,101]:
                    raw=json.loads((directory/mode/'settled-p0.json').read_text())['worlds'][-1]['world']
                    x,y,z=meta['target'];width,height,_=raw['size']
                    check(mode+': picked object removed',raw['cells'][(z*height+y)*width+x][7]!=kind)
                if kind==103:
                    check(mode+': underground passage changes level',after['pos'][2]!=before['pos'][2])
                if kind==53:
                    check(mode+': guard transfer removes hero stack',hero('guard-place')['army'][1][1]==0 and before['army'][1][1]>0)
                    check(mode+': guards returned intact',again['army']==before['army'])
                    for action in ['revisit','guard-reopen']:
                        check(mode+': mine guard window '+action,t['actions'][action+'/p0']['state']['window']['type']=='24type_monster_join_window')
                for field in ['experience','primary','skills','moraleBonus','luckBonus','backpack','equipped','visits']:
                    check(mode+': repeat does not duplicate '+field,after[field]==again[field])
                check(mode+': repeat resources unchanged',resources==world('settled-again')['resources'])
        if not meta and 'settled/p0' in a['actions'] and any('spell' in step for step in steps):
            spell=next(step['spell'] for step in steps if 'spell' in step)
            cancelled=any(step['name']=='cancel' for step in steps)
            costs={0:0,1:0,2:2,3:1,4:2,5:1,6:15,7:8,8:20,9:12}
            for mode,t in traces.items():
                def state(action):return t['actions'][action+'/p0']['state']
                before=state('move-out')['world']['heroes'][0];after=state('settled')['world']['heroes'][0]
                check(mode+': spell mana cost',before['mana']-after['mana']==(0 if cancelled else costs[spell]),{'spell':spell,'spent':before['mana']-after['mana']})
                check(mode+': spell completes on adventure map',state('settled')['layers']==0)
                flag={2:3,4:2,6:0,7:1}.get(spell)
                if flag is not None:check(mode+': expert spell effect',after['adventureSpells'][flag]==3)
                if spell==8 and not cancelled:
                    check(mode+': dimension door moved hero',after['pos']!=before['pos'])
                    check(mode+': dimension door daily counter',after['adventureSpells'][4]==before['adventureSpells'][4]+1)
                if spell==9:
                    check(mode+': town portal moved hero',after['pos']!=before['pos'])
                    check(mode+': town portal reached starting town',after['pos']==state('idle')['world']['heroes'][0]['pos'])
                if spell in [0,1] or cancelled:check(mode+': unsuccessful cast leaves state intact',after==before)
        if any(step['name']=='handoff-14' for step in steps):
            for mode,t in traces.items():
                last=t['actions'][steps[-1]['name']+'/p0']['state']
                check(mode+': reached second week',(last['game']['month'],last['game']['week'],last['game']['day'])==(1,2,1))
        return {'passed':all(c['passed'] for c in checks),'checks':checks,'strictParityPassed':json.loads((directory/'comparison.json').read_text())['passed'],
                'simulationMs':sum(v['virtualMs'] for t in traces.values() for k,v in t['actions'].items() if k.endswith('/p0')),
                'executionMs':sum(v['wallMs'] for t in traces.values() for k,v in t['actions'].items() if k.endswith('/p0')),
                'startupSeconds':sum(t['startupWallSeconds'] for t in traces.values())}
    except (ValueError,OSError,KeyError,StopIteration) as error:
        check('evidence readable',False,repr(error));return {'passed':False,'checks':checks}
