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
    return {'passed':all(c['passed'] for c in checks),'checks':checks,
            'scope':'Targeted regressions only. Does not waive remaining frame, audio onset, combat or timing differences.'}
