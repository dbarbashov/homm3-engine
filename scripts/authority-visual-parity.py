"""Paired original-UI scenarios. Records pixels/video, not just server revisions."""
from pathlib import Path
import json, os, time
from PIL import Image, ImageChops, ImageStat
from playwright.sync_api import sync_playwright
from browser_audio import use_silent_audio
ROOT=Path(__file__).resolve().parents[1]
MODE=os.environ.get('MODE','authority');BASE=os.environ.get('SITE_URL','http://127.0.0.1:'+('8801' if MODE=='authority' else '8802'))
OUT=ROOT/'test-results/authority-parity'/os.environ.get('CASE','baseline')/MODE
OUT.mkdir(parents=True,exist_ok=True)
with sync_playwright() as p:
    browser=getattr(p,os.environ.get('BROWSER','chromium')).launch()
    contexts=[browser.new_context(viewport={'width':800,'height':600},record_video_dir=str(OUT/'video'),record_video_size={'width':800,'height':600}) for _ in range(2)]
    for c in contexts:use_silent_audio(c)
    video_start=time.time();pages=[c.new_page() for c in contexts];a,b=pages;logs=[[],[]];errors=[];states={}
    for i,page in enumerate(pages):
        page.on('pageerror',lambda e:errors.append(str(e)))
        page.on('console',lambda m,i=i:logs[i].append(m.text))
        page.on('websocket',lambda ws,i=i:ws.on('framereceived',lambda data:logs[i].append(str(data)[:1000])))
    def capture(name):
        a.wait_for_timeout(300)
        a.locator('#canvas').screenshot(path=str(OUT/(name+'.png')))
        states[name]=a.evaluate('({game:window.homm3GameState,net:window.homm3NetworkState,screen:window.homm3PortDiagnostics?.screen,authority:window.homm3AuthorityStatus,choice:window.homm3Authority?.pendingChoice,hd:window.homm3HDState,combat:window.homm3CombatState})')
        states[name]['timestamp']=time.time();states[name]['videoSeconds']=time.time()-video_start
        (OUT/'states.json').write_text(json.dumps(states,ensure_ascii=False,indent=2))
        print(name,states[name],flush=True)
    def click(x,y):a.locator('#canvas').click(position={'x':x,'y':y})
    def key(k):a.locator('#canvas').press(k)
    try:
        for page in pages:
            page.goto(BASE+'/port/game.html?hd=0#multiplayer');page.wait_for_function('!document.querySelector("#start").disabled',timeout=60000)
            page.evaluate('document.fonts.ready')
            assert page.evaluate('Array.from(document.fonts).some(f=>f.family==="HeroesTitle"&&f.status==="loaded")')
        title='Visual parity '+str(time.time_ns())
        a.locator('#show-create').click();a.locator('#player-name').fill('Host');a.locator('#identity-submit').click();a.locator('#room-name').fill(title);a.locator('#capacity').select_option('2');a.locator('#create-submit').click()
        a.wait_for_function('window.homm3Room?.players.length===1')
        b.locator('#refresh-rooms').click();b.get_by_role('button',name='Войти: '+title,exact=True).click();b.locator('#player-name').fill('Guest');b.locator('#identity-submit').click()
        a.wait_for_function('window.homm3Room?.players.length===2');a.locator('#ready').click();b.locator('#ready').click()
        a.wait_for_function('window.homm3Room?.players.every(p=>p.ready)',timeout=120000);a.locator('#launch-room').click()
        a.wait_for_function('window.homm3PortDiagnostics?.screen==="scenarios"',timeout=60000);a.wait_for_timeout(1500)
        capture('00-scenarios')
        click(500,519);a.wait_for_timeout(500)
        click(230 if os.environ.get('RANGED')=='1' else 170,145);a.wait_for_timeout(300)
        for _ in range(2 if os.environ.get('RANGED')=='1' else 10):click(306,145);a.wait_for_timeout(150)
        capture('00-options')
        if os.environ.get('SETUP')=='1':raise SystemExit(0)
        click(495,555)
        for _ in range(120):
            visible=a.evaluate('window.homm3GameState?.day===1 && window.homm3PortDiagnostics?.screen==="adventure" && window.homm3HDState?.layers===0')
            ready=MODE=='legacy' or a.evaluate('window.homm3Authority?.revision>=1 && !window.homm3Authority.pendingChoice')
            if visible and ready:break
            for page in pages:
                if MODE=='authority':
                    if page.evaluate('window.homm3Authority?.pendingChoice?.kind==="normal" && window.homm3Authority.pendingChoice.presenting'):page.locator('#canvas').press('Enter')
                elif page.evaluate('window.homm3GameState?.day===1 && window.homm3HDState?.layers>0'):page.locator('#canvas').press('Enter')
            a.wait_for_timeout(500)
        a.wait_for_function('window.homm3PortDiagnostics?.screen==="adventure" && window.homm3HDState.layers===0')
        a.wait_for_timeout(400)
        initial_hero=a.evaluate('window.homm3GameState.hero')
        capture('01-map-before')
        if os.environ.get('TURNS')=='1':
            import base64
            observations=[]
            for page in pages:
                page.evaluate("""() => {
                    window.turnFrames=[];let last='',value=window.homm3TurnPresentation;
                    Object.defineProperty(window,'homm3TurnPresentation',{configurable:true,get:()=>value,set:next=>{
                      value=next;const key=JSON.stringify(next);if(key===last)return;last=key;
                      if(window.turnFrames.length<100)window.turnFrames.push({state:{...next},png:document.querySelector('#canvas').toDataURL()});
                    }});
                }""")
            def both(name):
                for i,page in enumerate(pages):
                    page.locator('#canvas').screenshot(path=str(OUT/f'{name}-p{i}.png'))
                    observations.append({'name':name,'seat':i,**page.evaluate('({game:window.homm3GameState,hd:window.homm3HDState,authority:window.homm3AuthorityStatus,presentation:window.homm3TurnPresentation,adventure:window.homm3AdventureState,choice:window.homm3Authority?.pendingChoice})')})
                (OUT/'turn-observations.json').write_text(json.dumps(observations,ensure_ascii=False,indent=2))
            both('turn-00-initial')
            if os.environ.get('OFFTURN')=='1':
                def select_waiting(seat,label):
                    page=pages[seat];before=page.evaluate('window.homm3AdventureState');revision=page.evaluate('window.homm3Authority?.revision')
                    for action,x,y in [('town',770,230),('hero',640,230),('town-again',770,230)]:
                        page.locator('#canvas').click(position={'x':x,'y':y});page.wait_for_timeout(400);both(f'{label}-{action}')
                    after=page.evaluate('window.homm3AdventureState')
                    if MODE=='authority':assert page.evaluate('window.homm3Authority.revision')==revision,'selection sent a game command'
                    assert [(h['id'],h['x'],h['y'],h['army'],h['counts']) for h in before['heroes']]==[(h['id'],h['x'],h['y'],h['army'],h['counts']) for h in after['heroes']],'selection changed owned army or coordinates'
                    if os.environ.get('EXPECT_BUG')!='1':assert all(h['onMap'] for h in after['heroes']),after
                select_waiting(1,'offturn-blue')
            for handoff in range(int(os.environ.get('HANDOFFS','14'))):
                seat=handoff%2;target=1-seat;day=1+(handoff+1)//2
                pages[seat].locator('#canvas').press('e')
                modal_counts=[0,0]
                for attempt in range(180):
                    ready=pages[target].evaluate('([target,day])=>window.homm3GameState?.player===target && (window.homm3GameState.week-1)*7+window.homm3GameState.day===day && window.homm3HDState?.layers===0 && window.homm3PortDiagnostics?.screen==="adventure"', [target,day])
                    # Wait until the actor's local turn notification has had a
                    # chance to open; the world may already be installed.
                    if ready and attempt>8:break
                    for i,page in enumerate(pages):
                        modal=page.evaluate('window.homm3HDState?.layers>0')
                        if modal:
                            both(f'turn-{handoff+1:02d}-modal-p{i}-{modal_counts[i]}');modal_counts[i]+=1
                            page.locator('#canvas').press('Enter')
                    a.wait_for_timeout(200)
                assert ready,f'handoff {handoff+1} did not reach seat {target} day {day}'
                both(f'turn-{handoff+1:02d}-settled')
                if os.environ.get('OFFTURN')=='1':
                    select_waiting(seat,f'offturn-{handoff+1:02d}-seat{seat}')
                    active=pages[target];old_y=active.evaluate('window.homm3GameState.y')
                    active.locator('#canvas').press('ArrowDown')
                    active.wait_for_function('(y)=>window.homm3GameState.y===y+1',arg=old_y,timeout=15000)
                    active.wait_for_timeout(400);both(f'offturn-{handoff+1:02d}-active-moved')
                    assert all(h['onMap'] for h in pages[seat].evaluate('window.homm3AdventureState.heroes')),'other player update removed waiting hero'

                print('handoff',handoff+1,'seat',target,'day',day,'modals',modal_counts,flush=True)
                if MODE=='authority':
                    assert not any(page.evaluate('!!window.homm3AuthorityStatus?.error') for page in pages)
            if MODE=='authority':
                for i,page in enumerate(pages):
                    for n,frame in enumerate(page.evaluate('window.turnFrames')):
                        (OUT/f'transition-p{i}-{n:02d}-{frame["state"]["mode"]}.png').write_bytes(base64.b64decode(frame['png'].split(',')[1]))
                a.reload();a.wait_for_function('window.homm3Authority?.revision>=1 && window.homm3PortDiagnostics?.screen==="adventure"',timeout=60000)
                a.wait_for_timeout(1200);both('turn-reconnected')
                assert a.evaluate('window.homm3HDState.layers===0'),'reconnect replayed turn notification'
            assert not errors,errors
            (OUT/'results.json').write_text(json.dumps({'passed':True,'mode':MODE,'handoffs':handoff+1,'errors':errors},indent=2));raise SystemExit(0)
        key('ArrowDown');a.wait_for_function('window.homm3GameState.y===6');a.wait_for_timeout(400);capture('02-map-moved')
        key('ArrowUp');a.wait_for_function('window.homm3HDState.layers===1');a.wait_for_timeout(400);capture('03-town-entry')
        if a.evaluate('window.homm3HDState.layers')==0:
            a.locator('#canvas').dblclick(position={'x':770,'y':231});a.wait_for_timeout(800)
        capture('04-town-before-garrison')
        key('Space');a.wait_for_timeout(800);capture('05-town-garrison')
        key('Space');a.wait_for_timeout(800);capture('06-town-visitor')
        if os.environ.get('EXTENDED')=='1':
            key('Space');a.wait_for_timeout(500)
            click(120,260);a.wait_for_function('window.homm3HDState.layers===2');capture('17-build-menu')
            click(300,80);a.wait_for_function('window.homm3HDState.layers===3');capture('18-build-confirm');key('Enter')
            a.wait_for_function('window.homm3HDState.layers===1');a.wait_for_timeout(700);capture('19-built')
            click(590,270);a.wait_for_function('window.homm3HDState.layers===2');capture('20-tavern');key('Enter')
            a.wait_for_function('window.homm3HDState.layers===1');a.wait_for_timeout(700);capture('21-hired')
            click(40,475);a.wait_for_function('window.homm3HDState.layers===2');capture('22-recruit');key('m');key('Enter')
            a.wait_for_function('window.homm3HDState.layers===1');a.wait_for_timeout(700);capture('23-recruited')
            key('F10');a.wait_for_timeout(700);capture('24-armies-swapped')
            key('Space');a.wait_for_timeout(700);capture('25-heroes-swapped')
            key('b');a.wait_for_function('window.homm3HDState.layers>1');capture('26-market');key('Escape');a.wait_for_timeout(500)
            key('g');a.wait_for_function('window.homm3HDState.layers>1');capture('27-guild');key('Enter');a.wait_for_timeout(500)
            key('Escape');a.wait_for_function('window.homm3HDState.layers===0');a.wait_for_timeout(700)
            a.locator('#canvas').dblclick(position={'x':640,'y':230});a.wait_for_function('window.homm3HDState.layers>0');capture('28-hero-inventory');key('Escape');a.wait_for_timeout(500)
            key('p');a.wait_for_function('window.homm3HDState.layers>0');capture('29-puzzle');key('Enter');a.wait_for_timeout(500)
            key('e');a.wait_for_timeout(500)
            if a.evaluate('window.homm3HDState.layers>0'):key('Enter')
            for _ in range(80):
                ready=b.evaluate('window.homm3AuthorityStatus?.activeActor===1 && window.homm3AuthorityStatus?.phase==="adventure"') if MODE=='authority' else b.evaluate('window.homm3NetworkState?.currentLocal===1 && window.homm3GameState?.player===1')
                ready=ready and b.evaluate('window.homm3HDState.layers===0 && window.homm3PortDiagnostics?.screen==="adventure"')
                if ready:break
                for page in pages:
                    if page.evaluate('window.homm3HDState?.layers>0'):page.locator('#canvas').press('Enter')
                a.wait_for_timeout(500)
            assert ready,'second player did not receive turn'
            capture('30-waiting-other-player');b.locator('#canvas').screenshot(path=str(OUT/'30-guest-turn.png'))
            old=b.evaluate('window.homm3GameState.y');b.locator('#canvas').press('ArrowDown');b.wait_for_function('(old)=>window.homm3GameState.y===old+1',arg=old)
            b.wait_for_timeout(600);b.locator('#canvas').screenshot(path=str(OUT/'31-guest-moved.png'))
            b.locator('#canvas').press('e');a.wait_for_timeout(500)
            if b.evaluate('window.homm3HDState.layers>0'):b.locator('#canvas').press('Enter')
            for _ in range(120):
                if a.evaluate('window.homm3GameState?.day===2 && window.homm3GameState?.player===0'):break
                for page in pages:
                    if page.evaluate('window.homm3HDState?.layers>0'):page.locator('#canvas').press('Enter')
                a.wait_for_timeout(500)
            a.wait_for_function('window.homm3GameState.day===2',timeout=15000);a.wait_for_timeout(700);capture('32-new-day')
            if MODE=='authority':
                a.reload();a.wait_for_function('window.homm3Authority?.revision>0 && window.homm3PortDiagnostics?.screen==="adventure"',timeout=60000);a.wait_for_timeout(700);capture('33-reconnected')
            assert not errors,errors
            (OUT/'results.json').write_text(json.dumps({'passed':True,'mode':MODE,'extended':True,'errors':errors},indent=2));raise SystemExit(0)
        key('Escape');a.wait_for_timeout(800);capture('07-map-return')
        for _ in range(3):
            if a.evaluate('window.homm3GameState.hero')==initial_hero:break
            key('h');a.wait_for_timeout(300)
        if os.environ.get('ROUTE')=='1':
            click(312,344);capture('07b-route-planned');click(312,344)
            a.wait_for_function('window.homm3GameState.y===7 && !window.homm3GameState.moving',timeout=15000);a.wait_for_timeout(500);capture('07c-route-finished')
        else:
            key('ArrowDown');a.wait_for_timeout(1000);key('ArrowDown');a.wait_for_timeout(1000)
        key('End')
        for _ in range(45):
            if a.evaluate('window.homm3GameState?.combat===1'):break
            key('Enter');a.wait_for_timeout(500)
        a.wait_for_function('window.homm3GameState.combat===1',timeout=10000);a.wait_for_timeout(2500)
        if MODE=='authority':a.wait_for_function('window.homm3Authority.combatRendererReady',timeout=30000)
        capture('08-battle-start')
        if os.environ.get('RANGED')=='1':
            if MODE=='authority':a.wait_for_function('window.homm3Authority.combatCanAct() && window.homm3Authority.combatRendererReady',timeout=45000)
            else:a.wait_for_function('window.homm3CombatState && !window.homm3CombatState.processing && !window.homm3CombatState.computer && window.homm3CombatState.control',timeout=45000)
            a.wait_for_timeout(300);click(178,575)
            for step in range(180):
                if a.evaluate('window.homm3PortDiagnostics?.screen==="combat-results"'):break
                if a.evaluate('window.homm3HDState.layers>1 && window.homm3GameState.combat===1'):
                    capture('14d-auto-choice');key('Enter')
                a.wait_for_timeout(500)
            a.wait_for_function('window.homm3PortDiagnostics?.screen==="combat-results"',timeout=15000);capture('15b-battle-result')
            key('Enter');a.wait_for_timeout(800);capture('16-battle-return');assert not errors,errors
            if MODE=='authority':assert not a.evaluate('window.homm3AuthorityStatus?.error')
            (OUT/'results.json').write_text(json.dumps({'passed':True,'mode':MODE,'ranged':True,'errors':errors},indent=2));raise SystemExit(0)
        if os.environ.get('STOP')=='battle':raise SystemExit(0)
        def combat_ready():
            a.wait_for_timeout(300)
            if MODE=='authority':a.wait_for_function('window.homm3Authority.combatCanAct() && window.homm3Authority.combatRendererReady',timeout=45000)
            else:a.wait_for_function('window.homm3CombatState && !window.homm3CombatState.processing && !window.homm3CombatState.computer && window.homm3CombatState.control',timeout=45000)
            a.wait_for_timeout(300)
        combat_ready();click(720,575);combat_ready();capture('09-battle-wait')
        click(770,575);combat_ready();capture('10-battle-defend')
        click(668,575);a.wait_for_function('window.homm3HDState.layers===2',timeout=5000);capture('11-spellbook')
        if os.environ.get('STOP')=='book':raise SystemExit(0)
        reference=Image.open(Path(os.environ.get('SPELL_REFERENCE',ROOT/'test-results/authority-parity/parity-current/authority/11-spellbook.png'))).convert('RGB').crop((305,100,350,145))
        book=Image.open(OUT/'11-spellbook.png').convert('RGB')
        candidates=[]
        for x in (240,330,460,550):
            for y in (125,225,325):
                # Original book columns have different optical offsets.
                # Match pixels locally; the first column is four pixels wider
                # than the second, so a fixed crop rejects valid Haste icons.
                for dx in range(-6,7):
                    crop=book.crop((x+dx-25,y-25,x+dx+20,y+20))
                    candidates.append((sum(ImageStat.Stat(ImageChops.difference(reference,crop)).mean),x+dx,y))
        score,sx,sy=min(candidates);assert score<22, candidates
        before_spell=a.evaluate('window.homm3CombatState.mana[0]')
        click(sx,sy);a.wait_for_function('window.homm3HDState.layers===1',timeout=5000);capture('12-spell-target')
        target_hex=a.evaluate('window.homm3CombatState.stacks.find(s=>s.side===0&&s.slot===0).hex')
        tx=(target_hex%17)*44+36+22*(1-(target_hex//17)%2);ty=(target_hex//17)*42+106
        a.locator('#canvas').hover(position={'x':tx,'y':ty});a.wait_for_timeout(300);click(tx,ty)
        a.wait_for_function('(mana)=>window.homm3CombatState.mana[0]<mana',arg=before_spell,timeout=15000)
        combat_ready();capture('13-spell-cast')
        assert a.evaluate('window.homm3CombatState.stacks.some(s=>s.side===0&&s.effects>0)'), 'buff missing from presentation'
        a.locator('#canvas').click(position={'x':tx,'y':ty});a.wait_for_timeout(400);capture('13b-spell-inspect');key('Enter');a.wait_for_timeout(400)
        # A real move on the original battlefield, followed by explicit Auto.
        click(270,178);combat_ready();capture('14-battle-move')
        if os.environ.get('NETWORK')=='1' and MODE=='authority':
            click(178,575);a.wait_for_timeout(1500);click(178,575)
            a.wait_for_function('!window.homm3Authority.combatAuto() && window.homm3Authority.combatCanAct()',timeout=45000);capture('14b-auto-stopped')
            a.reload();a.wait_for_function('window.homm3Authority?.combatRendererReady && window.homm3Authority.combatCanAct()',timeout=60000);a.wait_for_timeout(600);capture('14c-battle-reconnected')
        click(178,575);a.wait_for_timeout(18000);capture('15-battle-auto')
        a.wait_for_function('window.homm3PortDiagnostics?.screen==="combat-results"',timeout=120000);capture('15b-battle-result')
        if a.evaluate('window.homm3PortDiagnostics?.screen')=='combat-results':
            key('Enter');a.wait_for_timeout(1000);capture('16-battle-return')

        assert not errors,errors
        if MODE=='authority':assert not a.evaluate('window.homm3AuthorityStatus?.error')
        (OUT/'results.json').write_text(json.dumps({'passed':True,'mode':MODE,'errors':errors},indent=2))
    except Exception:
        capture('failure')
        raise
    finally:
        (OUT/'console.json').write_text(json.dumps(logs,ensure_ascii=False));(OUT/'errors.json').write_text(json.dumps(errors))
        for page in pages:
            try:page.evaluate("window.confirm=()=>true;document.querySelector('#leave-room').click()")
            except:pass
        for i,c in enumerate(contexts):
            c.close();pages[i].video.save_as(str(OUT/f'player-{i}.webm'))
        browser.close()
