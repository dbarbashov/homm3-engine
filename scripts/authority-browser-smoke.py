"""Real two-browser ordinary authoritative game: turns, reload and manual combat."""
from pathlib import Path
import json, time, os
from playwright.sync_api import sync_playwright
from browser_audio import use_silent_audio
BASE=os.environ.get('SITE_URL','http://127.0.0.1:8783')
ROOT=Path(__file__).resolve().parents[1]; OUT=ROOT/'test-results'/'authority-browser'/('-'.join(os.environ.get(key,default) for key,default in [('BROWSER','chromium'),('AUDIO_SINK','virtual'),('HD','0'),('HEADFUL','0'),('FAST','0'),('FAULT','0'),('CASE','manual'),('VIEW','desktop')]));OUT.mkdir(parents=True,exist_ok=True);(OUT/'results.json').unlink(missing_ok=True)
with sync_playwright() as p:
    browser=getattr(p,os.environ.get('BROWSER','chromium')).launch(headless=os.environ.get('HEADFUL')!='1',ignore_default_args=['--disable-background-timer-throttling','--disable-renderer-backgrounding','--disable-backgrounding-occluded-windows'] if os.environ.get('BROWSER','chromium')=='chromium' else [])
    credentials=json.loads(Path(os.environ['SITE_ACCESS_FILE']).read_text()) if os.environ.get('SITE_ACCESS_FILE') else None
    options={'http_credentials':{k:credentials[k] for k in ('username','password')}} if credentials else {}
    display={'viewport':{'width':852,'height':393},'device_scale_factor':3,'is_mobile':True,'has_touch':True} if os.environ.get('VIEW')=='iphone' else {'viewport':{'width':1360,'height':1000}}
    contexts=[browser.new_context(**display,**options) for _ in range(2)]
    for context in contexts: use_silent_audio(context,device_sink=os.environ.get('AUDIO_SINK')=='device')
    if os.environ.get('BATTLE_RELOAD')=='1':
        for context in contexts:context.add_init_script("""(() => {
            window.battleResumeProbe={packets:[],replays:0};let api;
            Object.defineProperty(window,'homm3Authority',{configurable:true,get:()=>api,set(value){
                api=value;let size=0;const length=api.combatLength,take=api.takeCombat,busy=api.presentationBusy;
                api.combatLength=function(...args){size=length.apply(this,args);return size;};
                api.takeCombat=function(heap,offset){const result=take.call(this,heap,offset);battleResumeProbe.packets.push(Array.from(heap.slice(offset,offset+size)));if(battleResumeProbe.packets.length>32)battleResumeProbe.packets.shift();return result;};
                api.presentationBusy=function(value){if(value)battleResumeProbe.replays++;return busy.call(this,value);};
            }});
        })();""")
    pages=[c.new_page() for c in contexts];logs=[[],[]];errors=[[],[]];worlds=[None,None]
    def received(payload,i):
        logs[i].append('RECV '+str(payload)[:1500])
        if isinstance(payload,str):
            data=json.loads(payload)
            if data.get('type')=='authority_state':worlds[i]=data['state']
    for i,page in enumerate(pages):
        page.on('console',lambda m,i=i:logs[i].append(m.text))
        page.on('websocket',lambda ws,i=i: (ws.on('framesent',lambda payload:logs[i].append('SEND '+str(payload)[:1500])),ws.on('framereceived',lambda payload:received(payload,i))))
        page.on('pageerror',lambda e,i=i:(errors[i].append(e.stack),print('ERROR',i,e.stack,flush=True)))
    def combat_click(page,x,y):
        box=page.locator('#canvas').bounding_box();size=page.evaluate('window.homm3HDState||{width:800,height:600}')
        x+=(size['width']-800)/2;y+=(size['height']-600)/2
        page.locator('#canvas').click(position={'x':box['width']*x/size['width'],'y':box['height']*y/size['height']})
    def acknowledge_turn(page):
        # Original local turn cards have no server choice token. Dismiss them
        # explicitly before sending the next adventure command.
        for _ in range(8):
            page.wait_for_timeout(150)
            if page.evaluate('window.homm3HDState.layers===0'):return
            assert page.evaluate('!window.homm3GameState.combat')
            choice=page.evaluate('window.homm3Authority.pendingChoice')
            assert not choice or choice['kind']=='normal', choice
            page.locator('#canvas').press('Enter')
        page.wait_for_function('window.homm3HDState.layers===0',timeout=3000)
    def dump(step):
        result=[]
        for i,page in enumerate(pages):
            if os.environ.get('CAPTURE_SCREENSHOTS','1')=='1':page.screenshot(path=str(OUT/f'{step}-{i}.png'),full_page=True)
            result.append(page.evaluate('({room:{title:window.homm3Room?.title,phase:window.homm3Room?.phase},authority:window.homm3AuthorityStatus,authorityRevision:window.homm3Authority?.revision,authorityActor:window.homm3Authority?.actor,state:window.homm3GameState,net:window.homm3NetworkState,heroes:window.homm3HeroStates,screen:window.homm3PortDiagnostics?.screen,queue:window.homm3Network.peek(),combatDebug:{canAct:window.homm3Authority?.combatCanAct(),auto:window.homm3Authority?.combatAuto(),choice:window.homm3Authority?.pendingChoice},status:document.querySelector("#status").textContent})'))
        (OUT/f'{step}.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));print(step,[{'player':c['state'].get('localPlayer'),'day':c['state'].get('day'),'hero':c['state'].get('hero'),'xy':[c['state'].get('x'),c['state'].get('y')]} if c.get('state') else c['screen'] for c in result],flush=True)
    try:
        a,b=pages
        for i,page in enumerate(pages):
            page.goto(BASE+'/port/game.html'+('' if os.environ.get('HD')=='1' else '?hd=0')+'#multiplayer')
            page.wait_for_function('!document.querySelector("#start").disabled',timeout=60000)

        title='Browser integration '+str(time.time_ns());a.locator('#show-create').click();a.locator('#player-name').fill('Host');a.locator('#identity-submit').click();a.locator('#room-name').fill(title);a.locator('#capacity').select_option('2');a.locator('#visibility').select_option('private');
        if os.environ.get('FAST')=='1': a.locator('#fast-game').check()
        a.locator('#create-submit').click()
        a.wait_for_function('window.homm3Room?.players.length === 1')
        # Persistent test parties must never appear in the user's public lobby.
        assert a.evaluate('window.homm3Room.private')
        invitation=a.locator('#invite-link').input_value()
        b.goto(invitation);b.locator('#join-invite').click();b.locator('#player-name').fill('Guest');b.locator('#identity-submit').click()
        a.wait_for_function('window.homm3Room?.players.length === 2')
        if os.environ.get('LOBBY_ONLY')=='1':
            assert a.evaluate('async()=>!(await (await fetch("/lobby/rooms")).json()).rooms.some(r=>r.id===window.homm3Room.id)')
            (OUT/'private-lobby.json').write_text(json.dumps({'passed':True,'privateInvitationJoined':True,'absentFromPublicList':True},indent=2))
            print('PASS private lobby',flush=True);raise SystemExit(0)
        b.locator('#room-chat').click();b.locator('#chat-input').fill('Привет! <b>Проверяем чат</b>');b.locator('#chat-send').click()
        a.wait_for_function('document.querySelector("#chat-log").textContent.includes("Привет!")')
        b.locator('#chat-back').click();a.locator('#ready').click();b.locator('#ready').click()
        a.wait_for_function('window.homm3Room?.players.every(p=>p.ready)',timeout=120000)
        dump('lobby-ready')
        a.locator('#launch-room').click()
        a.wait_for_function('window.homm3PortDiagnostics?.screen === "scenarios"',timeout=60000)
        a.wait_for_timeout(int(os.environ.get('SETUP_DELAY','5000')));dump('scenarios')
        assert all(not page.evaluate('window.homm3EngineError') for page in pages), logs
        if os.environ.get('FAULT')=='1':
            # Exercise the actual native-error event path, then relay disconnect
            # propagation. This is test input, not a production debug hook.
            b.evaluate("window.dispatchEvent(new CustomEvent('homm3-engine-error',{detail:'Truncated campaign text <b>test</b>'}))")
            for page in pages:
                page.wait_for_function('!!window.homm3EngineError')
                assert page.locator('#launch-cover').is_visible()
                assert page.get_by_role('button',name='Перезагрузить страницу',exact=True).is_visible()
                assert page.locator('#status b').count()==0
            a.wait_for_timeout(4000)
            assert all(page.evaluate('!window.homm3Network.connected && !sessionStorage.getItem("homm3.session")') for page in pages)
            dump('fatal-visible-on-both')
            for page in pages:
                page.get_by_role('button',name='Перезагрузить страницу',exact=True).click()
                page.wait_for_function('!document.querySelector("#start").disabled')
                assert page.locator('#room-browser').is_visible()
                assert page.evaluate('!window.homm3Room && !window.homm3EngineError')
            dump('fresh-room-browser')
        else:
            box=a.locator('#canvas').bounding_box();size=a.evaluate('window.homm3HDState||{width:800,height:600}')
            a.locator('#canvas').click(position={'x':box['width']*((size['width']-800)/2+495)/size['width'],'y':box['height']*((size['height']-600)/2+555)/size['height']})
            for page in pages:
                page.wait_for_function('()=>{if(window.homm3EngineError)throw Error(window.homm3EngineError);return window.homm3GameState?.day===1;}',timeout=30000)
            for _ in range(12):
                if all(page.evaluate('window.homm3PortDiagnostics?.screen==="adventure"') for page in pages):break
                for page in pages:
                    if page.evaluate("window.homm3HDState?.layers>0 && !window.homm3GameState?.combat && (!window.homm3Authority?.pendingChoice || window.homm3Authority.pendingChoice.kind==='normal')"):page.locator('#canvas').press('Enter')
                a.wait_for_timeout(1000)
            for page in pages:page.wait_for_function('window.homm3PortDiagnostics?.screen==="adventure"',timeout=15000)
            assert [page.evaluate('window.homm3GameState.localPlayer') for page in pages]==[0,1]
            assert all(not page.evaluate('window.homm3EngineError') for page in pages), logs
            dump('adventure')
            for _ in range(25):
                if all(page.evaluate('window.homm3Authority?.revision>=1') for page in pages):break
                for page in pages:
                    if page.evaluate("window.homm3HDState?.layers>0 && !window.homm3GameState?.combat && (!window.homm3Authority?.pendingChoice || window.homm3Authority.pendingChoice.kind==='normal')"):page.locator('#canvas').press('Enter')
                a.wait_for_timeout(500)
            for page in pages:
                page.wait_for_function('window.homm3Authority?.revision>=1',timeout=3000)
                assert page.evaluate('!window.homm3AuthorityStatus?.error'), page.evaluate('window.homm3AuthorityStatus')
            dump('authority-ready')
            if os.environ.get('PUZZLE')=='1':
                revision=a.evaluate('window.homm3Authority.revision')
                a.locator('#canvas').press('p')
                a.wait_for_function("window.homm3Authority.pendingChoice?.kind==='puzzle' && window.homm3Authority.pendingChoice.presenting && window.homm3HDState.layers>0",timeout=30000)
                dump('original-puzzle')
                a.locator('#canvas').press('Enter')
                a.wait_for_function('(r)=>window.homm3Authority.revision>r && window.homm3HDState.layers===0',arg=revision)
                dump('puzzle-closed')
            initial=[page.evaluate('({...window.homm3GameState,...window.homm3HeroStates[window.homm3GameState.localPlayer]})') for page in pages]
            revision=a.evaluate('window.homm3Authority.revision')
            b.locator('#canvas').press('ArrowDown');a.wait_for_timeout(400)
            assert b.evaluate('window.homm3Authority.revision')==revision
            a.locator('#canvas').press('ArrowDown')
            a.wait_for_function('(r)=>window.homm3Authority.revision>r',arg=revision)
            a.wait_for_function('(y)=>window.homm3GameState.y===y && !window.homm3GameState.moving',arg=initial[0]['y']+1)
            dump('host-moved')
            assert a.evaluate('window.homm3GameState.y')==initial[0]['y']+1
            if os.environ.get('UI_GATES')=='1':
                # Actual D key must reach the server, which rejects a dig after
                # movement. The old prototype guard never sent this intent.
                sent=len(logs[0]);revision=a.evaluate('window.homm3Authority.revision')
                a.locator('#canvas').press('d')
                a.wait_for_function('window.homm3HDState.layers>0')
                assert any('SEND ' in line and '"op":11' in line for line in logs[0][sent:]), logs[0][sent:]
                assert any("Digging needs a full day's movement" in line for line in logs[0][sent:]), logs[0][sent:]
                assert a.evaluate('window.homm3Authority.revision')==revision
                dump('dig-server-rejected-after-movement')
                a.locator('#canvas').press('Enter');a.wait_for_function('window.homm3HDState.layers===0')
            if os.environ.get('TOWN')=='1':
                def click(x,y,double=False):
                    box=a.locator('#canvas').bounding_box();size=a.evaluate('window.homm3HDState||{width:800,height:600,adventure:false}')
                    if not size['adventure']:x+=(size['width']-800)/2;y+=(size['height']-600)/2
                    elif x>=600:x+=size['width']-800
                    getattr(a.locator('#canvas'),'dblclick' if double else 'click')(position={'x':box['width']*x/size['width'],'y':box['height']*y/size['height']},delay=120)
                click(770,231,True);a.wait_for_function('window.homm3HDState.layers===1');a.wait_for_timeout(900)
                gold=worlds[0]['words'][14];revision=a.evaluate('window.homm3Authority.revision')
                dump('town-open')
                click(120,260);a.wait_for_function('window.homm3HDState.layers===2')
                click(300,80);a.wait_for_function('window.homm3HDState.layers===3');a.locator('#canvas').press('Enter')
                a.wait_for_function('(r)=>window.homm3Authority.revision>r && window.homm3HDState.layers===1',arg=revision)
                assert worlds[0]['words'][14]==gold-2500, worlds[0]['words'][8:15]
                dump('town-built');revision=a.evaluate('window.homm3Authority.revision')
                click(590,270);a.wait_for_function('window.homm3HDState.layers===2');a.locator('#canvas').press('Enter')
                a.wait_for_function('(r)=>window.homm3Authority.revision>r && window.homm3HDState.layers===1',arg=revision)
                assert worlds[0]['words'][14]==gold-5000, worlds[0]['words'][8:15]
                dump('town-hired');revision=a.evaluate('window.homm3Authority.revision')
                click(40,475);a.wait_for_function('window.homm3HDState.layers===2');a.wait_for_timeout(500)
                a.locator('#canvas').press('m');a.locator('#canvas').press('Enter')
                a.wait_for_function('(r)=>window.homm3Authority.revision>r && window.homm3HDState.layers===1',arg=revision)
                assert worlds[0]['words'][14]==gold-5500, worlds[0]['words'][8:15]
                dump('town-recruited')
                if os.environ.get('UI_GATES')=='1':
                    sent=len(logs[0]);revision=a.evaluate('window.homm3Authority.revision')
                    a.locator('#canvas').press('F10')
                    a.wait_for_function('(r)=>window.homm3Authority.revision>r && window.homm3HDState.layers===1',arg=revision)
                    assert any('SEND ' in line and '"op":32' in line for line in logs[0][sent:]), logs[0][sent:]
                    dump('town-armies-swapped')
                a.locator('#canvas').press('Escape');a.wait_for_function('window.homm3HDState.layers===0');a.wait_for_timeout(400)
                for _ in range(3):
                    if a.evaluate('window.homm3GameState.hero')==initial[0]['hero']:break
                    a.locator('#canvas').press('h');a.wait_for_timeout(400)
                a.wait_for_function('(id)=>window.homm3GameState.hero===id',arg=initial[0]['hero'])
            acknowledge_turn(a)
            a.locator('#canvas').press('e');a.wait_for_timeout(300);a.locator('#canvas').press('Enter')
            for _ in range(20):
                if b.evaluate('window.homm3AuthorityStatus?.activeActor===1 && window.homm3AuthorityStatus?.phase==="adventure"'):break
                for page in pages:
                    if page.evaluate("window.homm3HDState?.layers>0 && !window.homm3GameState?.combat && (!window.homm3Authority?.pendingChoice || window.homm3Authority.pendingChoice.kind==='normal')"):page.locator('#canvas').press('Enter')
                a.wait_for_timeout(300)
            b.wait_for_function('window.homm3AuthorityStatus?.activeActor===1 && window.homm3AuthorityStatus?.phase==="adventure"')
            acknowledge_turn(b)
            revision=b.evaluate('window.homm3Authority.revision')
            b.locator('#canvas').press('ArrowDown')
            b.wait_for_function('(r)=>window.homm3Authority.revision>r',arg=revision)
            b.wait_for_function('(y)=>window.homm3GameState.y===y && !window.homm3GameState.moving',arg=initial[1]['y']+1)
            dump('guest-moved')
            assert b.evaluate('window.homm3GameState.y')==initial[1]['y']+1
            revision=b.evaluate('window.homm3Authority.revision')
            b.reload()
            b.wait_for_function('(r)=>window.homm3Authority?.revision===r && window.homm3PortDiagnostics?.screen==="adventure" && !!window.homm3GameState',arg=revision,timeout=60000)
            assert b.evaluate('window.homm3GameState.y')==initial[1]['y']+1
            dump('guest-reloaded')
            b.locator('#canvas').press('ArrowDown')
            b.wait_for_function('(r)=>window.homm3Authority.revision>r',arg=revision)
            b.wait_for_function('(y)=>window.homm3GameState.y===y',arg=initial[1]['y']+2)
            dump('guest-moved-after-reload')
            acknowledge_turn(b)
            b.locator('#canvas').press('e');b.wait_for_timeout(300);b.locator('#canvas').press('Enter')
            for _ in range(90):
                if a.evaluate('window.homm3AuthorityStatus?.activeActor===0 && window.homm3AuthorityStatus?.phase==="adventure" && window.homm3GameState?.day===2'):break
                for page in pages:
                    if page.evaluate("window.homm3HDState?.layers>0 && !window.homm3GameState?.combat && (!window.homm3Authority?.pendingChoice || window.homm3Authority.pendingChoice.kind==='normal')"):page.locator('#canvas').press('Enter')
                a.wait_for_timeout(500)
            a.wait_for_function('window.homm3AuthorityStatus?.activeActor===0 && window.homm3GameState?.day===2',timeout=3000)
            acknowledge_turn(a)
            dump('next-day')
            net=a.evaluate('window.homm3NetworkState')
            px=8+32*(6-net['radarX'])+16;py=8+32*(8-net['radarY'])+16
            box=a.locator('#canvas').bounding_box();size=a.evaluate('window.homm3HDState||{width:800,height:600}')
            position={'x':box['width']*px/size['width'],'y':box['height']*py/size['height']}
            a.locator('#canvas').click(position=position);a.wait_for_timeout(200);a.locator('#canvas').click(position=position)
            for _ in range(60):
                if a.evaluate("window.homm3Authority?.pendingChoice?.kind==='combat' && window.homm3Authority.pendingChoice.presenting"):break
                if a.evaluate("window.homm3HDState?.layers>0 && !window.homm3GameState?.combat && (!window.homm3Authority?.pendingChoice || window.homm3Authority.pendingChoice.kind==='normal')"):a.locator('#canvas').press('Enter')
                a.wait_for_timeout(500)
            a.wait_for_function("window.homm3Authority?.pendingChoice?.kind==='combat' && window.homm3Authority.pendingChoice.presenting",timeout=3000)
            a.wait_for_function('window.homm3Authority.combatRendererReady',timeout=30000)
            dump('manual-battle')
            # Original tactics placement precedes regular creature turns.
            # The server authorizes the original Start Combat control.
            box=a.locator('#canvas').bounding_box();size=a.evaluate('window.homm3HDState||{width:800,height:600}')
            if a.evaluate('window.homm3Authority.combatTactics'):
                combat_click(a,525,575)
                a.wait_for_function('!window.homm3Authority.combatTactics')
            a.wait_for_timeout(500)
            token=a.evaluate('window.homm3Authority.pendingChoice.token')
            combat_click(a,770,575)
            a.wait_for_function('(token)=>window.homm3Authority?.pendingChoice?.kind==="combat" && window.homm3Authority.pendingChoice.presenting && window.homm3Authority.pendingChoice.token!==token',arg=token,timeout=30000)
            a.wait_for_function('window.homm3Authority.combatCanAct()',timeout=30000)
            dump('manual-defend')
            if os.environ.get('BATTLE_RELOAD')=='1':
                latest=a.evaluate('window.battleResumeProbe.packets.at(-1)')
                assert latest and a.evaluate('window.battleResumeProbe.replays>0'), 'Need a battle with actual presentation history'
                a.reload()
                a.wait_for_function('window.homm3Authority?.combatRendererReady && window.homm3Authority.combatCanAct()',timeout=60000)
                restored=a.evaluate('window.battleResumeProbe')
                assert restored['packets'][0]==latest, 'Reload opened an old battle image'
                assert restored['replays']==0, 'Reload replayed historical combat actions'
                (OUT/'battle-resume.json').write_text(json.dumps({'passed':True,'canonicalWords':len(latest),'historyReplays':restored['replays'],'snapshots':len(restored['packets'])},indent=2))
                dump('battle-reloaded')
                box=a.locator('#canvas').bounding_box();size=a.evaluate('window.homm3HDState||{width:800,height:600}')
            if os.environ.get('HISTORY')=='1':
                before=a.evaluate('({token:window.homm3Authority.pendingChoice.token,revision:window.homm3Authority.revision})')
                a.locator('#view-toggle').click()
                # Inspect the issue URL without sending anything to GitHub.
                contexts[0].route('https://github.com/dbarbashov/homm3-web/issues/new*',lambda route:route.fulfill(body='<title>Issue form target</title>'))
                with a.expect_popup() as opened:a.locator('[data-report-bug]').click()
                report_page=opened.value;report_page.wait_for_url('https://github.com/**/issues/new?*')
                from urllib.parse import parse_qs,urlsplit
                evidence=parse_qs(urlsplit(report_page.url).query)['evidence'][0]
                reproduction=evidence.split(': ',1)[1]
                assert reproduction.startswith(BASE+'/lobby/reports/')
                report_page.goto(reproduction);report_page.wait_for_function('!document.querySelector("#replay").disabled')
                report_page.locator('#replay').click();report_page.wait_for_function('!document.querySelector("#replay").disabled',timeout=30000)
                assert 'История проверена' in report_page.locator('#state').inner_text(), report_page.locator('#state').inner_text()
                replay=json.loads(report_page.locator('#details').text_content());assert replay['verified'] and replay['choice']['token']==before['token']
                assert 'homm3.session' not in json.dumps(replay['diagnostics'])
                report_url=report_page.url;report_page.close()
                a.locator('#view-toggle').click();a.evaluate('window.homm3Lobby.open()')
                a.once('dialog',lambda d:d.accept())
                with a.expect_navigation(wait_until='domcontentloaded'):a.locator('#leave-room').click()
                a.wait_for_function('!window.homm3Room && document.querySelector("#saved-games") && !document.querySelector("#saved-games").hidden',timeout=30000)
                a.evaluate('sessionStorage.removeItem("homm3.session")');a.reload()
                a.locator('#saved-games-list tr').filter(has_text=title).get_by_role('button',name='Продолжить').click()
                a.wait_for_function('window.homm3Authority?.combatRendererReady && window.homm3Authority.combatCanAct()',timeout=60000)
                assert a.evaluate('window.homm3Authority.pendingChoice.token')==before['token']
                assert a.evaluate('window.battleResumeProbe?.replays ?? 0')==0
                (OUT/'history.json').write_text(json.dumps({'passed':True,'report':report_url,'verified':True,'savedGameRejoined':True,'browserIdentityPersisted':True,'battleTokenPreserved':True},indent=2))
                dump('saved-party-rejoined')
            if os.environ.get('AUTO')=='1':
                combat_click(a,178,575)
                a.wait_for_function('window.homm3Authority.combatAuto()',timeout=10000)
                combat_click(a,178,575)
                a.wait_for_function('!window.homm3Authority.combatAuto() && window.homm3Authority.combatCanAct()',timeout=30000)
                dump('explicit-auto-stopped')
            revision=a.evaluate('window.homm3Authority.revision')
            a.wait_for_function('window.homm3Authority.combatCanAct()',timeout=30000)
            combat_click(a,128,575);a.wait_for_function('window.homm3HDState.layers>1');a.locator('#canvas').press('Enter')
            a.wait_for_function('window.homm3PortDiagnostics.screen==="combat-results"',timeout=30000);dump('retreat-result')
            if os.environ.get('BATTLE_RELOAD')=='1':
                assert a.evaluate('window.battleResumeProbe.replays>0'), 'Fresh post-reload actions must still animate'
            a.locator('#canvas').press('Enter')
            a.wait_for_function('(r)=>window.homm3Authority.revision>r && window.homm3PortDiagnostics?.screen==="adventure"',arg=revision,timeout=30000)
            dump('retreat-adventure')
        assert not any(errors), errors
        assert not any('Game startup:' in line for lines in logs for line in lines), logs
        (OUT/'results.json').write_text(json.dumps({'passed':True,'browser':os.environ.get('BROWSER','chromium'),'headed':os.environ.get('HEADFUL')=='1','fast':os.environ.get('FAST')=='1','fault':os.environ.get('FAULT')=='1','hd':os.environ.get('HD')=='1','town':os.environ.get('TOWN')=='1','uiGates':os.environ.get('UI_GATES')=='1','battleReload':os.environ.get('BATTLE_RELOAD')=='1','history':os.environ.get('HISTORY')=='1','auto':os.environ.get('AUTO')=='1','errors':errors},indent=2))
        print('PASS',flush=True)
    finally:
        try:dump('final')
        except:pass
        (OUT/'console.json').write_text(json.dumps(logs,ensure_ascii=False,indent=2));(OUT/'errors.json').write_text(json.dumps(errors,ensure_ascii=False,indent=2))
        for page in pages:
            try:page.evaluate("window.confirm=()=>true;document.querySelector('#leave-room').click()")
            except:pass
        browser.close()
