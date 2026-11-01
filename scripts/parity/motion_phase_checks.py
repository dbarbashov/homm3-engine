"""Observe real remote walking and idle poses; retain raw transport timing separately."""
import argparse
import copy
import json
from pathlib import Path


def check(chunks,kind):
    checks=[]
    def record(name,passed):checks.append({'name':name,'passed':bool(passed)})
    for action,pair in chunks.items():
        sequences={}
        for mode,c in pair.items():
            sequence=[]
            for frame in c['frames']:
                ui=frame['ui']
                if kind=='remote':
                    motion=(ui.get('adventure') or {}).get('motion',{})
                    if not motion.get('moving'):continue
                    value=[motion[k] for k in ['sequence','frame','scroll','local']]
                else:
                    battle=ui.get('combat')
                    if not battle:continue
                    value=[[s[k] for k in ['side','slot','sequence','frame']] for s in battle['stacks']]
                if not sequence or sequence[-1][1]!=value:sequence.append((frame['t'],value))
            sequences[mode]=sequence
            record(f'{action}/{mode}: real animated frames',len(sequence)>=5)
            if kind=='remote':
                state=c['ui']['adventure']
                if mode=='authority':record(f'{action}/{mode}: waiting control remains inactive',not state['cursor'])
                record(f'{action}/{mode}: uses enemy movement speed',all(not v[-1] for _,v in sequence))
            else:record(f'{action}/{mode}: fidget actually occurs',any(s[2]==1 and s[3]>0 for _,v in sequence for s in v))
        left,right=sequences.values()
        record(action+': identical ordered animation/scroll states',[v for _,v in left]==[v for _,v in right])
        # Absolute start can differ by one transport frame. Once an animation
        # begins, intervals must agree exactly (omit the held boundary image).
        def intervals(seq):return [b[0]-a[0] for a,b in zip(seq[1:],seq[2:])]
        record(action+': identical animation intervals',intervals(left)==intervals(right))
    return {'passed':all(c['passed'] for c in checks),'checks':checks}


def check_run(directory,kind):
    actions=['move-out','approach'] if kind=='remote' else ['encounter-confirm','battle-wait']
    seat=1 if kind=='remote' else 0
    chunks={a:{m:json.loads((directory/m/f'{a}-p{seat}.json').read_text()) for m in ['legacy','authority']} for a in actions}
    result=check(chunks,kind)
    if result['passed']:
        broken=copy.deepcopy(chunks)
        for pair in broken.values():pair['authority']['frames']=pair['authority']['frames'][:1]
        result['missingAnimationDetected']=not check(broken,kind)['passed']
        result['passed'] &= result['missingAnimationDetected']
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--kind',choices=['remote','idle'],required=True);a=p.parse_args()
    result=check_run(a.directory,a.kind)
    (a.directory/(a.kind+'-motion-checks.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
