#!/usr/bin/env python3
"""Prove detectors reject faults injected into a real, completed browser trace."""
import argparse
import copy
import json
from pathlib import Path
from compare import compare


def check(trace):
    if not compare(trace,trace)['passed']:
        raise ValueError('Fault validation requires a complete, valid reference trace')
    action=next(k for k,a in trace['actions'].items() if a['audioEvents'] and a['pcm'] and len(a['frames'])>2)
    original=trace['actions'][action]
    index=next(i for i in range(1,len(original['frames'])) if original['frames'][i]['tiles']!=original['frames'][i-1]['tiles'])
    def hero(a):a['state']['adventure']['heroes'].clear()
    def animation(a):a['frames'].pop(index)
    def delay(a):
        for f in a['frames'][index:]:f['t']+=250
    def sound(a):a['audioEvents'].clear()
    def pcm(a):a['pcm'][0]['hash']^=1
    results=[]
    for name,mutate,channel in [('missing-hero',hero,'state'),('dropped-animation-frame',animation,'pixels'),('250ms-delay',delay,'timing'),('missing-sound',sound,'audioEvents'),('changed-waveform',pcm,'pcm')]:
        candidate=copy.deepcopy(trace);mutate(candidate['actions'][action]);result=compare(trace,candidate)
        detected=not result['passed'] and channel in [f['channel'] for f in result['failures']]
        results.append({'fault':name,'passed':detected,'action':action,'expectedChannel':channel,'firstDivergence':result.get('firstDivergence')})
    candidate=copy.deepcopy(trace);candidate['rng']^=1;result=compare(trace,candidate)
    results.append({'fault':'different-RNG','passed':result.get('blocked') is True})
    return {'passed':all(r['passed'] for r in results),'checks':results}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('trace',type=Path);p.add_argument('--out',type=Path,required=True);args=p.parse_args()
    result=check(json.loads(args.trace.read_text()));args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(result,indent=2));print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
