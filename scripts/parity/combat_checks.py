"""Compare original battle RNG, gameplay and every rendered opening field.

Ignores idle pose phase and local control ownership only in the gameplay gate;
raw frame, PCM and timing differences remain in comparison.json. Original
hero armies commit earlier during results; report that separately and compare
the actual results window and the owned world after returning to adventure.
"""
import argparse
import json
from pathlib import Path
from compare import first_difference


def check_run(directory):
    directory=Path(directory);checks=[];intermediateWorldDifferences=[]
    def check(name,value,detail=None):checks.append({'name':name,'passed':bool(value),**({'detail':detail} if detail else {})})
    try:
        traces={m:json.loads((directory/m/'trace.json').read_text()) for m in ['legacy','authority']}
        steps=json.loads((directory/'legacy/scenario.json').read_text());keys={s['name']+f'/p{i}' for s in steps for i in range(2)}
        for mode,t in traces.items():
            check(mode+': complete',not t.get('failure') and not t.get('overflow') and set(t['actions'])==keys,t.get('failure'))
            check(mode+': no browser errors',not t.get('errors'))
            check(mode+': hero visibility',all(c['passed'] for a in t['actions'].values() for c in a['invariants']))
        left,right=traces.values()
        check('identical initial GM2/RNG/world and build',all(left[k]==right[k] for k in ['fixtureSha256','rng','initial','wasmSha256','observerSha256','settings','viewport','buildConfig']))
        reference=[]
        for step in steps:
            chunk=json.loads((directory/'legacy'/(step['name']+'-p0.json')).read_text())
            reference.extend(e['data'] for e in chunk['events'] if e['kind']=='decision')
        candidate=[json.loads(line) for line in (directory/'authority/decisions.jsonl').read_text().splitlines()]
        rng={m:[e for e in events if e['kind'].startswith('rng-')] for m,events in [('legacy',reference),('authority',candidate)]}
        ai={m:[e for e in events if e['kind']=='combat-ai'] for m,events in [('legacy',reference),('authority',candidate)]}
        check('all RNG draws and reseeds match',bool(rng['legacy']) and rng['legacy']==rng['authority'],first_difference(rng['legacy'],rng['authority']))
        check('every AI decision receives the same RNG',bool(ai['legacy']) and ai['legacy']==ai['authority'],first_difference(ai['legacy'],ai['authority']))
        def gameplay(combat):
            if combat is None:return None
            return {**{k:combat[k] for k in ['side','slot','round','mana','battlefield']},'stacks':[{k:v for k,v in s.items() if k not in ['frame','sequence']} for s in combat['stacks']]}
        for key in sorted(keys):
            a,b=left['actions'][key]['state'],right['actions'][key]['state']
            worldDiff=first_difference(a['world'],b['world'])
            if not a['game']['combat'] and not b['game']['combat']:
                check(key+': owned world outside battle',worldDiff is None,worldDiff)
            elif worldDiff:intermediateWorldDifferences.append({'action':key,**worldDiff})
            if a['screen']=='combat-results':
                check(key+': original results window',a['window']==b['window'],first_difference(a['window'],b['window']))
            check(key+': battle gameplay',gameplay(a['combat'])==gameplay(b['combat']),first_difference(gameplay(a['combat']),gameplay(b['combat'])))
        expected=left['actions']['encounter-confirm/p0']['state']['combat']['battlefield']
        for mode in traces:
            chunk=json.loads((directory/mode/'encounter-confirm-p0.json').read_text())
            fields=[f['ui']['combat']['battlefield'] for f in chunk['frames'] if f.get('ui',{}).get('combat') and (f['ui'].get('window') or {}).get('type')=='13TCombatWindow']
            check(mode+': opening frame evidence',len(fields)>=2)
            check(mode+': canonical obstacles from first rendered combat frame',bool(fields) and all(field==expected for field in fields),{'frames':len(fields),'fieldVariants':len({json.dumps(v,sort_keys=True) for v in fields})})
        return {'passed':all(c['passed'] for c in checks),'intermediateWorldDifferences':intermediateWorldDifferences,'rngRecords':{m:len(v) for m,v in rng.items()},'aiDecisions':{m:len(v) for m,v in ai.items()},'checks':checks,'strictParityPassed':json.loads((directory/'comparison.json').read_text())['passed']}
    except (OSError,KeyError,ValueError) as error:
        check('complete readable evidence',False,repr(error));return {'passed':False,'checks':checks}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);a=p.parse_args();r=check_run(a.directory)
    (a.directory/'combat-checks.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r));raise SystemExit(0 if r['passed'] else 1)
