"""Focused, frame-aware regressions for adventure/HD presentation.

Raw comparison.json remains strict. These gates never claim full AV parity.
"""
import argparse
import json
from pathlib import Path
from compare import first_difference


def check_run(directory, suite, traces=None):
    directory=Path(directory);checks=[];metrics={}
    def check(name, passed, detail=None):
        checks.append(dict(name=name,passed=bool(passed),**({'detail':detail} if detail is not None else {})))
    try:
        if traces is None:traces={mode:json.loads((directory/mode/'trace.json').read_text()) for mode in ['legacy','authority']}
        for mode,t in traces.items():
            steps=json.loads((directory/mode/'scenario.json').read_text())
            expected={s['name']+f'/p{i}' for s in steps for i in range(2)}
            check(mode+': complete',not t.get('failure') and not t.get('overflow') and not t.get('errors') and set(t['actions'])==expected,t.get('failure'))
        ref,cand=traces.values()
        check('same save and initial world',all(ref[k]==cand[k] for k in ['fixtureSha256','initial','rng','settings','viewport','wasmSha256','observerSha256']))
        def state(t,key):return t['actions'][key+'/p0']['state']
        def route(s):return s['adventure']['route'] if s['adventure']['motion']['showRoute'] else []
        def visible(t,key):return state(t,key)['adventure']['motion']['pointerVisible']
        def frames(t,key):return [f for f in t['actions'][key+'/p0']['frames'] if not f.get('held')]
        if suite=='movement':
            for key in ['plan-route','walk-route','exhausted-idle','new-day-route','continue-route']:
                a,b=state(ref,key),state(cand,key)
                check(key+': owned world',a['world']==b['world'],first_difference(a['world'],b['world']))
                check(key+': visible route',route(a)==route(b),first_difference(route(a),route(b)))
                check(key+': destination',a['adventure']['heroes'][0]['target']==b['adventure']['heroes'][0]['target'])
                check(key+': original map, visible pointer',b['window']['type']=='19TAdventureMapWindow' and visible(cand,key))
            exhausted=state(cand,'exhausted-idle');fresh=state(cand,'new-day-route')
            check('exhaustion leaves brown route',exhausted['world']['heroes'][0]['movement']==0 and route(exhausted) and all(c[3]>=26 for c in route(exhausted)))
            check('new day recolors without route click',fresh['world']['heroes'][0]['movement']>0 and route(fresh) and all(c[3]<=25 for c in route(fresh)))
        elif suite=='guard':
            count=0
            for key in ref['actions']:
                if not key.startswith('plan-') or not key.endswith('/p0'):continue
                name=key[:-3];a,b=state(ref,name),state(cand,name);count+=len(route(a))
                check(name+': same guarded route',route(a)==route(b),first_difference(route(a),route(b)))
                check(name+': same destination',a['adventure']['heroes'][0]['target']==b['adventure']['heroes'][0]['target'])
            check('nonempty guarded route evidence',count>0)
        elif suite=='town':
            check('walk enters original town',state(cand,'walk-into-town')['window']['type']=='17TTownScreenWindow')
            check('cursor visible in town and after exit',visible(cand,'walk-into-town') and visible(cand,'close-town'))
            for key in ['walk-into-town','close-town']:
                check(key+': owned world',state(ref,key)['world']==state(cand,key)['world'])
        elif suite=='sound':
            def sounds(t,key):
                return [e['data']['name'] for e in t['actions'][key]['audioEvents'] if e['kind']=='homm3Sfx.play' and e['data'].get('loops')==1 and e['data']['name'].lower()!='button.wav']
            meaningful=0
            for key in ref['actions']:
                if not key.endswith('/p0'):continue
                a,b=sounds(ref,key),sounds(cand,key);meaningful+=len(a)
                check(key+': original event SFX once',a==b,{'legacy':a,'authority':b})
            check('actual event sample evidence',meaningful>0);metrics['eventSamples']=meaningful
        elif suite=='speed':
            for mode,t in traces.items():
                action=t['actions']['walk-route/p0'];moving=[f for f in frames(t,'walk-route') if f['ui']['adventure']['motion']['moving']]
                speed=t['settings']['game']['heroSpeed'];metrics[mode]={'speed':speed,'movingFrames':len(moving),'durationMs':moving[-1]['t']-moving[0]['t'] if moving else 0}
                check(mode+': destination reached',state(t,'walk-route')['world']['heroes'][0]['pos']==[9,15,0])
                check(mode+': selected speed observed',all(f['ui']['adventure']['motion']['speed']==speed for f in moving))
                by_cell={}
                for f in moving:
                    a=f['ui']['adventure'];by_cell.setdefault(a['heroes'][0]['x'],[]).append(a['motion']['frame'])
                if speed!=4:
                    check(mode+': animated all six steps',len(by_cell)==6)
                    seq=list(by_cell.values());check(mode+': walk cycle continues across cells',all(a[-1]==b[0] for a,b in zip(seq,seq[1:])),by_cell)
            a,b=metrics['legacy']['durationMs'],metrics['authority']['durationMs']
            check('route pacing follows original speed',abs(a-b)<=max(128,a*.12),metrics)
        elif suite=='combat':
            # The original Defend button changes defense/message; it does not
            # play the block-hit animation or sample. Compare actual requests.
            for key in ['battle-wait','battle-defend']:
                def samples(t):return [e['data']['name'] for e in t['actions'][key+'/p0']['audioEvents'] if e['kind']=='homm3Sfx.play' and e['data'].get('loops')==1]
                a,b=samples(ref),samples(cand)
                metrics[key+'AllSamples']={'match':a==b,'legacy':a,'authority':b}
                # Other actions (AI attacks / morale) may follow in this input
                # window. Keep their full mismatch visible in metrics/raw diff.
                check(key+': no invented block-hit sample',not any('dfnd' in n.lower() or 'defend' in n.lower() for n in b))
            for mode,t in traces.items():
                action=t['actions']['battle-defend/p0'];before=action['frames'][0]['ui']['combat'];identity=(before['side'],before['slot'])
                poses=[s['sequence'] for f in frames(t,'battle-defend') if f['ui'].get('combat') for s in f['ui']['combat']['stacks'] if (s['side'],s['slot'])==identity]
                check(mode+': Defend does not play block-hit pose',bool(poses) and 4 not in poses)
            for mode,t in traces.items():
                after=frames(t,'battle-dismiss')
                check(mode+': no late adventure walk after battle',not any((f['ui'].get('adventure') or {}).get('motion',{}).get('moving') for f in after))
                prior=frames(t,'encounter')[-1]
                opening=[f for f in frames(t,'encounter-confirm') if (f['ui'].get('window') or {}).get('type')=='13TCombatWindow']
                check(mode+': opening evidence',bool(opening))
                if opening and opening[0]['width']>800:
                    width=opening[0]['width'];height=opening[0]['height'];left=(width-800)//2;top=(height-600)//2;cols=(width+31)//32
                    indices=[y*cols+x for y in range((height+31)//32) for x in range(cols) if x*32+32<=left or x*32>=left+800 or y*32+32<=top or y*32>=top+600]
                    check(mode+': HD border evidence',bool(indices))
                    check(mode+': no combat paint outside overlay',all(all(f['tiles'][i]==prior['tiles'][i] for i in indices) for f in opening),{'frames':len(opening),'outsideTiles':len(indices)})
        else:raise ValueError('Unknown suite')
    except (OSError,KeyError,ValueError,IndexError) as e:check('readable complete evidence',False,repr(e))
    return {'passed':all(c['passed'] for c in checks),'suite':suite,'metrics':metrics,'checks':checks}


def check_rejection(directory):
    checks=[]
    def check(name,value):checks.append({'name':name,'passed':bool(value)})
    try:
        t=json.loads((Path(directory)/'authority/trace.json').read_text());a=t['actions']
        before=a['before/p0']['state'];rejected=a['reject/p0']['state'];after=a['dismiss/p0']['state']
        check('complete rejection replay',not t.get('failure') and not t.get('overflow') and not t.get('errors'))
        check('actual optimistic move happened',any(f['ui']['adventure']['motion']['moving'] and f['ui']['adventure']['heroes'][0]['x']==4 for f in a['reject/p0']['frames']))
        check('server rejected the invalid intent',rejected['window']['type']=='10TDialogBox' and any('Invalid direction' in w.get('text','') for w in rejected['window']['widgets']))
        check('canonical world unchanged',before['world']==after['world'])
        check('pointer visible during rejection and after',rejected['adventure']['motion']['pointerVisible'] and after['adventure']['motion']['pointerVisible'])
        check('returns to original map',after['window']['type']=='19TAdventureMapWindow' and not after['adventure']['motion']['moving'])
        first=a['before/p0']['frames'][-1]['ui']['network'];last=a['dismiss/p0']['frames'][-1]['ui']['network']
        check('camera restored',all(first[k]==last[k] for k in ['radarX','radarY']))
    except (OSError,KeyError,ValueError,IndexError) as e:checks.append({'name':'complete readable evidence','passed':False,'detail':repr(e)})
    return {'passed':all(c['passed'] for c in checks),'suite':'rejection','checks':checks}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--suite',required=True,choices=['movement','town','sound','speed','combat','guard','rejection']);a=p.parse_args()
    result=check_rejection(a.directory) if a.suite=='rejection' else check_run(a.directory,a.suite);(a.directory/'presentation-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
