"""A hero with zero mana may inspect and close the original combat spellbook."""
import argparse
import copy
import json
from pathlib import Path


def check(traces):
    checks=[]
    def record(name,passed):checks.append({'name':name,'passed':bool(passed)})
    for mode,t in traces.items():
        record(mode+': complete',not t.get('failure') and not t.get('overflow') and not t.get('errors'))
        states={name:t['actions'][name+'/p0']['state'] for name in ['encounter-confirm','empty-spellbook','close-book']}
        initial=states['encounter-confirm'];book=states['empty-spellbook'];closed=states['close-book']
        button=next(w for w in initial['window']['widgets'] if w['id']==2008)
        record(mode+': zero mana fixture',initial['combat']['mana'][0]==0)
        record(mode+': readable spellbook enabled',not button['status']&(8|32|4096))
        record(mode+': native spellbook opens',book['layers']==2 and book['window']['type']=='16TSpellbookWindow')
        record(mode+': return to controllable battle',closed['layers']==1 and closed['combat']['control'])
        record(mode+': inspection spends no mana or turn',initial['combat']['mana']==closed['combat']['mana'] and all(initial['combat'][k]==closed['combat'][k] for k in ['side','slot','round']))
        record(mode+': no gameplay mutation',initial['world']==closed['world'])
    return {'passed':all(c['passed'] for c in checks),'checks':checks}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);args=p.parse_args()
    traces={mode:json.loads((args.directory/mode/'trace.json').read_text()) for mode in ['legacy','authority']}
    result=check(traces)
    if result['passed']:
        faults={}
        for fault in ['disabled','missing-book','spent-mana']:
            original=traces['authority'];actions=dict(original['actions'])
            name={'disabled':'encounter-confirm','missing-book':'empty-spellbook','spent-mana':'close-book'}[fault]+'/p0'
            actions[name]={**actions[name],'state':copy.deepcopy(actions[name]['state'])};state=actions[name]['state']
            if fault=='disabled':next(w for w in state['window']['widgets'] if w['id']==2008)['status']|=8
            elif fault=='missing-book':state['layers']=1
            else:state['combat']['mana'][0]=1
            faults[fault]=not check({**traces,'authority':{**original,'actions':actions}})['passed']
        result['faultsDetected']=faults;result['passed'] &= all(faults.values())
    (args.directory/'spellbook-checks.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
