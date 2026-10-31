"""Check actual battle cues, shooting recovery and in-action corpses.

Uses render-state frames and original audio requests, never screenshots.
Raw frame/PCM parity remains independent and strict in comparison.json.
"""
import argparse
import copy
import json
from pathlib import Path
from combat_checks import check_run as combat_checks

CUES={'morale-good':'goodmrle.wav','morale-bad':'badmrle.wav','luck-good':'goodluck.82m'}
RANGE={14,15,16}


def check_chunks(chunks, profile):
    checks=[];metrics={}
    def check(name, ok, detail=None):
        checks.append({'name':name,'passed':bool(ok),**({'detail':detail} if detail is not None else {})})
    def decision(c,kind):return [e for e in c['events'] if e['kind']=='decision' and e['data']['kind']==kind]
    def frames(c):return [f for f in c['frames'] if not f.get('held') and f['ui'].get('combat')]
    def cues(c):
        return [(e['data']['kind'],e['data']['side'],e['data']['slot']) for e in c['events'] if e['kind']=='decision' and e['data']['kind'].endswith('-start') and e['data']['kind'][:-6] in CUES]
    check('same original proc order and targets',cues(chunks['legacy'])==cues(chunks['authority']))
    for mode,c in chunks.items():
        fs=frames(c);metrics[mode]={}
        for cue,sample in CUES.items():
            starts=decision(c,cue+'-start');ends=decision(c,cue+'-end');durations=[]
            check(mode+': '+cue+' balanced',len(starts)==len(ends))
            for i,(start,end) in enumerate(zip(starts,ends)):
                duration=end['t']-start['t'];durations.append(duration)
                visible=[f for f in fs if start['t']<=f['t']<=end['t'] and f['ui']['combat']['pow']['active']]
                sounds=[e for e in c['events'] if start['t']<=e['t']<=end['t'] and e['kind']=='homm3Sfx.play' and e['data']['name'].lower()==sample]
                check(f'{mode}: {cue}/{i} visible effect and pause',len(visible)>=2 and duration>=100)
                check(f'{mode}: {cue}/{i} original sound once',len(sounds)==1)
            metrics[mode][cue]=durations
        # Every rendered shot must leave its shooting pose before the next
        # impact completes. A final authoritative snapshot alone cannot pass.
        shots={};recovered=0;dead=set();bodies=set()
        for f in fs:
            b=f['ui']['combat'];bodies.update(tuple(x) for x in b['corpses'])
            for s in b['stacks']:
                key=(s['side'],s['slot'])
                if s['count']<=0:dead.add(key)
                if s['sequence'] in RANGE:shots[key]=True
                elif shots.pop(key,False):
                    recovered+=1
                    check(f'{mode}: shot recovery {recovered}',s['sequence']==2 or s['count']<=0,{'sequence':s['sequence']})
        check(mode+': real shooting evidence',recovered>=2)
        check(mode+': no stranded shooter',not shots)
        check(mode+': corpses observed during action',bool(bodies))
        metrics[mode].update(recoveries=recovered,corpses=sorted(bodies))
    for cue in CUES:
        a,b=metrics['legacy'][cue],metrics['authority'][cue]
        check(cue+': original pause duration',len(a)==len(b) and all(abs(x-y)<=100 for x,y in zip(a,b)),{'legacy':a,'authority':b})
    required={'positive':['morale-good','luck-good'],'negative':['morale-bad'],'zero':[],'unlucky':[]}[profile]
    for cue in required:check(cue+': non-vacuous original and replay evidence',all(metrics[m][cue] for m in metrics))
    if profile in ['zero','unlucky','negative']:check('no invented negative luck proc',all(not metrics[m]['luck-good'] for m in metrics))
    check('same corpse identities and occupied hexes',metrics['legacy']['corpses']==metrics['authority']['corpses'])
    c=chunks['authority'];fs=frames(c);installed=decision(c,'corpse-installed');impacts=decision(c,'impact-end')
    check('in-action death boundaries recorded',len(installed)>0)
    for i,event in enumerate(installed):
        key=(event['data']['side'],event['data']['slot'])
        end=next((e['t'] for e in impacts if e['t']>=event['t']),None)
        observed=next((f['t'] for f in fs if f['t']>=event['t'] and any(tuple(x[1:3])==key for x in f['ui']['combat']['corpses'])),None)
        check(f'corpse {i}: visible before impact returns',end is not None and observed is not None and observed<=end,{'installed':event['t'],'visible':observed,'impactEnd':end})
    for i,end in enumerate(impacts):
        # The next volley may render at the very same virtual timestamp.
        previous=[f for f in fs if f['t']<end['t']]
        if previous:check(f'impact {i}: shooting recovery complete',all(s['sequence'] not in RANGE for s in previous[-1]['ui']['combat']['stacks'] if s['count']>0))
    return {'passed':all(c['passed'] for c in checks),'checks':checks,'metrics':metrics}


def check_run(directory,profile):
    directory=Path(directory)
    chunks={m:json.loads((directory/m/'battle-auto-p0.json').read_text()) for m in ['legacy','authority']}
    result=check_chunks(chunks,profile);rules=combat_checks(directory)
    result['rules']=rules;result['passed'] &= rules['passed'];result['strictParityPassed']=rules.get('strictParityPassed',False)
    # Seed faults into real observed traces. These are assertions about what
    # the checker catches, not synthetic evidence that the game is correct.
    faults={}
    for fault in ['missing-cues','stuck-shot','deferred-corpses']:
        broken=copy.deepcopy(chunks);c=broken['authority']
        if fault=='missing-cues':
            c['events']=[e for e in c['events'] if not(e['kind']=='decision' and any(e['data']['kind'].startswith(k) for k in CUES))]
        for f in c['frames']:
            b=f['ui'].get('combat')
            if not b:continue
            if fault=='stuck-shot':
                for s in b['stacks']:
                    if s['creature'] in [2,3] and s['count']>0:s['sequence']=16
            elif fault=='deferred-corpses':b['corpses']=[]
        faults[fault]=not check_chunks(broken,profile)['passed']
    result['faultsDetected']=faults
    if profile in ['positive','negative']:result['passed'] &= all(faults.values())
    else:result['passed'] &= faults['stuck-shot'] and faults['deferred-corpses']
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--profile',required=True,choices=['positive','negative','zero','unlucky']);a=p.parse_args()
    r=check_run(a.directory,a.profile);(a.directory/'feedback-checks.json').write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({'passed':r['passed'],'failed':[c for c in r['checks'] if not c['passed']],'metrics':r['metrics'],'faults':r['faultsDetected']}));raise SystemExit(0 if r['passed'] else 1)
