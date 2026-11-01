"""Postconditions for the town scenario, including actual avatar/army transfer."""
def adventure(trace):
    actions=trace['actions'];checks=[]
    def state(name):return actions[name+'/p0']['state']
    initial=state('idle');hero=initial['world']['heroes'][0];ident=hero['id']
    checks.append({'name':'hero moved one tile','passed':state('move')['world']['heroes'][0]['pos'][1]==hero['pos'][1]+1})
    for action,field,other in [('garrison','garrisonHero','visitor'),('visitor','visitor','garrisonHero')]:
        towns=state(action)['world']['towns']
        checks.append({'name':action+' owns hero portrait','passed':any(t[field]==ident and t[other]==-1 for t in towns)})
    checks.append({'name':'exit restored adventure','passed':state('town-exit')['layers']==0 and state('town-exit')['screen']=='adventure'})
    return {'passed':all(c['passed'] for c in checks),'checks':checks}


def adventure_regressions(reference, candidate):
    """Town destinations and waiting-seat ambient policy, independent of pixels."""
    checks=[]
    checks.append({'name':'single step: exact audio requests and onset',
                   'passed':bool(reference['actions']['move/p0']['audioEvents']) and
                   reference['actions']['move/p0']['audioEvents']==candidate['actions']['move/p0']['audioEvents']})
    for name in ['town-entry','garrison','visitor','town-exit']:
        def targets(trace):
            return {h['id']:h['target'] for h in trace['actions'][name+'/p0']['state']['adventure']['heroes']}
        checks.append({'name':name+': original hero destinations',
                       'passed':targets(reference)==targets(candidate)})
    for name in ['waiting-town','waiting-hero']:
        e=reference['actions'][name+'/p1'];a=candidate['actions'][name+'/p1']
        def effects(action):return [v for v in action['audioEvents'] if v['kind'].startswith('homm3Sfx.')]
        checks.append({'name':name+': no restarted ambient effects',
                       'passed':effects(e)==effects(a)==[] and
                       e['state']['adventure']['sound']['ambient']==a['state']['adventure']['sound']['ambient']==False})
    return {'passed':all(c['passed'] for c in checks),'checks':checks}


def combat_regressions(reference, candidate):
    """Focused behavioral gates; full frame/audio/timing comparison stays strict."""
    checks=[]
    def record(name, passed):checks.append({'name':name,'passed':bool(passed)})
    def action(trace, name, seat=0):return trace['actions'][f'{name}/p{seat}']
    expected=action(reference,'encounter')['state'];actual=action(candidate,'encounter')['state']
    record('original monster pursuit question and buttons',expected['layers']>0 and expected['window']==actual['window'])
    record('hero reaches encounter before question',expected['adventure']==actual['adventure'])
    for name in ['move-out','approach','encounter']:
        e=action(reference,name);a=action(candidate,name)
        record(name+': ambient requests preserved',
               [{k:v for k,v in event.items() if k!='t'} for event in e['audioEvents']]==
               [{k:v for k,v in event.items() if k!='t'} for event in a['audioEvents']])
        record(name+': waiting seat keeps selection',action(reference,name,1)['state']['adventure']['hero']==action(candidate,name,1)['state']['adventure']['hero']==-1)
    def spell_sound(trace):
        return [event for event in action(trace,'cast-haste')['audioEvents']
                if isinstance(event.get('data'),dict) and event['data'].get('name')=='Tailwind.wav']
    e=spell_sound(reference);a=spell_sound(candidate)
    record('Haste sound plays once and stops after original duration',
           len(e)==len(a)==2 and [v['kind'] for v in e]==[v['kind'] for v in a] and
           [v['data'] for v in e]==[v['data'] for v in a] and e[1]['t']-e[0]['t']==a[1]['t']-a[0]['t'])
    for name in ['encounter-confirm','cast-haste']:
        e=action(reference,name)['state']['adventure'];a=action(candidate,name)['state']['adventure']
        record(name+': static map hero beneath battle',e==a and not a['mobile'] and not a['cursor'])
    for name in ['encounter-confirm','battle-wait','battle-defend','cast-haste','log-up','log-up-again','log-down']:
        def arrows(trace):return [w for w in action(trace,name)['state']['window']['widgets'] if w['id'] in [2006,2007]]
        e=arrows(reference);a=arrows(candidate)
        record(name+': combat history arrows enabled',len(e)==len(a)==2 and e==a and all(not w['status']&(8|32) for w in a))
    def log(trace,name):
        a=action(trace,name)['state']
        return (a['combat']['log'],next(w['text'] for w in a['window']['widgets'] if w['id']==2005))
    for name in ['log-up','log-up-again','log-down']:
        record(name+': original history position and text',log(reference,name)==log(candidate,name))
    for trace in [reference,candidate]:
        up=log(trace,'log-up')[0]['start'];again=log(trace,'log-up-again')[0]['start'];down=log(trace,'log-down')[0]['start']
        record(trace['mode']+': clicks scroll history in both directions',again==up-1 and down==again+1)
    for name in ['battle-dismiss','post-battle-move']:
        e=action(reference,name)['state'];a=action(candidate,name)['state']
        record(name+': restored hero cursor, route and ambient context',
               e['adventure']==a['adventure'] and a['adventure']['mobile'] and
               a['adventure']['cursor'] and a['adventure']['sound']['ambient'])
        record(name+': canonical position, army, experience and movement',e['world']==a['world'])
        e=action(reference,name,1)['state'];a=action(candidate,name,1)['state']
        record(name+': waiting player does not acquire a hero',
               e['adventure']['hero']==a['adventure']['hero']==-1 and
               not a['adventure']['cursor'] and not a['adventure']['sound']['ambient'])
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'scope':'Targeted regressions only. Does not waive remaining frame, audio onset, combat or timing differences.'}


def retreat_regressions(reference, candidate):
    """Battle return must select the remaining town, not revive the lost hero."""
    checks=[]
    def record(name, passed):checks.append({'name':name,'passed':bool(passed)})
    for name in ['retreat-question','retreat-confirm']:
        e=reference['actions'][name+'/p0']['state'];a=candidate['actions'][name+'/p0']['state']
        record(name+': original modal and static hero',e['window']==a['window'] and
               e['adventure']==a['adventure'] and not a['adventure']['mobile'])
    for seat in [0,1]:
        e=reference['actions'][f'retreat-dismiss/p{seat}']['state']
        a=candidate['actions'][f'retreat-dismiss/p{seat}']['state']
        record(f'p{seat}: canonical post-retreat world',e['world']==a['world'])
        record(f'p{seat}: original remaining selection',
               all(e['adventure'][k]==a['adventure'][k] for k in ['hero','town','heroes','mobile','cursor','route']))
        record(f'p{seat}: adventure restored without a stale hero cursor',
               a['screen']=='adventure' and a['layers']==0 and
               a['adventure']['hero']==-1 and not a['adventure']['mobile'] and not a['adventure']['cursor'])
    a=candidate['actions']['retreat-dismiss/p0']['state']
    record('lost hero removed and remaining town selected',not a['world']['heroes'] and a['adventure']['town']==0)
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'scope':'Retreat lifecycle only. Full audiovisual parity is checked separately.'}
