"""Exact terrain/footstep presentation checks on the terrain-band fixture."""
import argparse
import json
from pathlib import Path


def check_run(directory):
    root=Path(directory)
    traces={mode:json.loads((root/mode/'trace.json').read_text()) for mode in ['legacy','authority']}
    checks=[]
    def check(name, passed):checks.append({'name':name,'passed':bool(passed)})
    for mode,t in traces.items():
        check(mode+': complete without browser errors',not t.get('failure') and not t.get('overflow') and not t.get('errors'))
        route=t['actions']['walk-terrain-route/p0']
        music=[e['data'][0] for e in route['audioEvents'] if e['kind']=='homm3Music.play']
        footsteps=[e['data']['name'] for e in route['audioEvents'] if e['kind']=='homm3Sfx.play' and e['data']['name'].startswith('horse')]
        check(mode+': crossed dirt then snow',music==['Dirt','Snow','Snow'])
        check(mode+': terrain-specific footsteps',footsteps==['horse02.wav','horse00.wav','horse03.wav'])
        check(mode+': route arrived',route['state']['adventure']['sound']['center']==[9,15,0])
        check(mode+': original route ended',not route['state']['adventure']['motion']['moving'])
    left,right=traces.values()
    check('same save and RNG',all(left[k]==right[k] for k in ['fixtureSha256','rng','initial','wasmSha256','observerSha256','settings']))
    for name in ['walk-terrain-route','return-snow','return-snow-edge','return-dirt']:
        e=left['actions'][name+'/p0'];a=right['actions'][name+'/p0']
        check(name+': exact audio requests including onset',e['audioEvents']==a['audioEvents'])
        check(name+': exact PCM',e['pcm']==a['pcm'])
        check(name+': original terrain/music state',e['state']['adventure']['sound']==a['state']['adventure']['sound'])
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'scope':'Active-player movement audio only; full two-player frame/state comparison remains separate.'}


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory',type=Path);a=p.parse_args()
    result=check_run(a.directory)
    (a.directory/'movement-audio-checks.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
