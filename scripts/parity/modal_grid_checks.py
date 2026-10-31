#!/usr/bin/env python3
"""Focused original-vs-authority UI gates; raw AV parity remains a separate gate."""
import argparse
import copy
import json
from pathlib import Path


def check_run(directory, suite, traces=None):
    checks=[]
    def check(name, passed):checks.append({'name':name,'passed':bool(passed)})
    try:
        traces=traces or {m:json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
        ref,cand=traces['legacy'],traces['authority']
        check('same save, RNG and builds',all(ref[k]==cand[k] for k in ['fixtureSha256','initial','rng','settings','viewport','wasmSha256','observerSha256']))
        for mode,t in traces.items():
            steps=json.loads((directory/mode/'scenario.json').read_text())
            expected={step['name']+f'/p{i}' for step in steps for i in range(2)}
            check(mode+': complete capture',not t.get('failure') and not t.get('overflow') and not t.get('errors') and set(t['actions'])==expected)
        def action(t,name):return t['actions'][name+'/p0']
        def state(t,name):return action(t,name)['state']
        def buttons(s):return {w['id']:w['status']&8 for w in s['window']['widgets'] if 2008<=w['id']<=2010}
        if suite=='guild':
            colors=['PRRed.pcx','PRBlue.pcx','PRTan.pcx','PRGreen.pcx','PROrange.pcx','PRPurple.pcx','PRTeal.pcx','PRRose.pcx']
            active=[i for i,p in enumerate(ref['initial']['players']) if not p['disabled']]
            for mode,t in traces.items():
                for name in ['guild','guild-idle']:
                    s=state(t,name)
                    check(mode+': '+name+' pauses video',s['window']['type']=='19TThievesGuildWindow' and s['video']['active'] and s['video']['paused'])
                    widgets={w['id']:w for w in s['window']['widgets']}
                    check(mode+': '+name+' player-colored cards',[widgets[900+i].get('image') for i in range(len(active))]==[colors[i] for i in active])
                frames=action(t,'guild-idle')['frames']
                check(mode+': static guild pixels',bool(frames) and len({tuple(f['tiles']) for f in frames})==1)
                for name in ['guild-close','tavern-idle']:
                    s=state(t,name);check(mode+': '+name+' resumes video',s['window']['type']=='13TTavernWindow' and s['video']['active'] and not s['video']['paused'])
        elif suite=='portraits':
            for name in ['garrison-open','visitor-open']:
                for mode,t in traces.items():check(mode+': '+name+' immediately opens hero',state(t,name)['window']['type']=='17THeroScreenWindow')
            for name in ['to-garrison','to-visitor']:
                check(name+': identical town/army state',state(ref,name)['world']==state(cand,name)['world'])
        elif suite=='combat':
            for name in ['encounter-confirm','battle-hover','battle-fidget','battle-wait','battle-defend','cast-haste','second-phase','wait-disabled']:
                a,b=state(ref,name),state(cand,name)
                check(name+': control and stack',all(a['combat'][k]==b['combat'][k] for k in ['side','slot','round','control','processing']))
                check(name+': original range',a['combat']['grid']['shade']==b['combat']['grid']['shade'] and a['combat']['grid']['drawn']==b['combat']['grid']['drawn'])
                check(name+': original button flags',buttons(a)==buttons(b))
                # Row 576..599 avoids the animation/tooltip area; these tiles
                # cover the book, wait and defend buttons. This catches NODRAW
                # flags changing without repainting the actual control bar.
                af,bf=action(ref,name)['frames'][-1],action(cand,name)['frames'][-1]
                check(name+': painted controls match',all(af['tiles'][450+x]==bf['tiles'][450+x] for x in [21,22,23,24]))
            for mode,t in traces.items():
                enemy=[f for name,a in t['actions'].items() if name.endswith('/p0') for f in a['frames'] if (f.get('ui',{}).get('combat') or {}).get('side')==1]
                check(mode+': opponent range never drawn',bool(enemy) and all(not any(f['ui']['combat']['grid']['drawn']) for f in enemy))
                idle=action(t,'battle-fidget')['frames'];hover=state(t,'battle-hover')['combat']['grid']['hover']
                check(mode+': animated idle preserves hover',bool(hover) and len(idle)>5 and all(f['ui']['combat']['grid']['hover']==hover for f in idle))
                a,b=state(t,'second-phase'),state(t,'wait-disabled')
                check(mode+': second wait disabled',bool(buttons(a).get(2009)) and all(a['combat'][k]==b['combat'][k] for k in ['slot','side','round','secondPhase']))
                check(mode+': battle closes',state(t,'battle-dismiss')['game']['combat']==0)
        else:raise ValueError(suite)
    except (OSError,KeyError,ValueError,IndexError,TypeError) as e:
        checks.append({'name':'complete readable evidence','passed':False,'detail':repr(e)})
    return {'passed':bool(checks) and all(c['passed'] for c in checks),'suite':suite,'checks':checks}


def faults(directory,suite):
    traces={m:json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
    assert check_run(directory,suite,traces)['passed'],'Positive control must pass first'
    cases={'guild':['video-over-guild','red-cards'],'portraits':['dead-garrison-portrait','dead-visitor-portrait'],'combat':['enemy-range','stale-button-pixels','lost-hover','wait-enabled']}
    results=[]
    for fault in cases[suite]:
        broken=copy.deepcopy(traces);a=broken['authority']['actions']
        if fault=='video-over-guild':a['guild-idle/p0']['state']['video']['paused']=False
        elif fault=='red-cards':
            for w in a['guild/p0']['state']['window']['widgets']:
                if 900<=w['id']<908:w['image']='PRRed.pcx'
        elif fault.startswith('dead-'):a[('garrison-open' if 'garrison' in fault else 'visitor-open')+'/p0']['state']['window']['type']='17TTownScreenWindow'
        elif fault=='enemy-range':
            frame=next(f for key,act in a.items() if key.endswith('/p0') for f in act['frames'] if (f.get('ui',{}).get('combat') or {}).get('side')==1)
            frame['ui']['combat']['grid']['drawn'][35]=1
        elif fault=='stale-button-pixels':a['cast-haste/p0']['frames'][-1]['tiles'][471]^=1
        elif fault=='lost-hover':a['battle-fidget/p0']['frames'][-1]['ui']['combat']['grid']['hover']=[]
        elif fault=='wait-enabled':
            for w in a['second-phase/p0']['state']['window']['widgets']:
                if w['id']==2009:w['status']&=~8
        result=check_run(directory,suite,broken)
        results.append({'fault':fault,'detected':not result['passed']})
    return {'passed':all(r['detected'] for r in results),'faults':results}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--suite',required=True,choices=['guild','portraits','combat']);p.add_argument('--faults',action='store_true');a=p.parse_args()
    result=faults(a.directory,a.suite) if a.faults else check_run(a.directory,a.suite)
    (a.directory/('presentation-faults.json' if a.faults else 'presentation-checks.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
