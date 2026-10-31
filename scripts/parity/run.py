#!/usr/bin/env python3
"""Isolated real-network parity runs. Requires the private native/WASM builds.
No production relay restart, no audible device, no browser screenshot capture.
"""
from __future__ import annotations
import argparse, asyncio, base64, hashlib, json, os, socket, subprocess, time
from pathlib import Path
from urllib.request import urlopen
from playwright.async_api import async_playwright
from compare import compare, report

ROOT=Path(__file__).resolve().parents[2]


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def free_port():
    with socket.socket() as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]


class Services:
    def __init__(self,args,mode,fixture):self.args=args;self.mode=mode;self.fixture=fixture;self.processes=[];self.files=[]
    def spawn(self,command,env,log):
        stream=log.open('w');self.files.append(stream)
        environment={**os.environ,**env}
        for key in list(environment):
            if key.startswith(('HOMM3_AUTHORITY_','HOMM3_HARNESS_')) and not environment[key]:environment.pop(key)
        p=subprocess.Popen(command,cwd=ROOT,env=environment,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True);self.processes.append(p)
    async def __aenter__(self):
        try:return await self.start()
        except BaseException:
            await self.__aexit__();raise
    async def start(self):
        relay_port,site_port=free_port(),free_port();out=self.args.out/self.mode;out.mkdir(parents=True,exist_ok=True)
        env={'LOBBY_BIND':f'127.0.0.1:{relay_port}','HOMM3_WEB_ROOT':str(ROOT/'web')}
        if self.mode=='authority':env.update(HOMM3_AUTHORITY_ENGINE=str(ROOT/'scripts/parity/worker.py'),HOMM3_HARNESS_ENGINE_BINARY=str(self.args.worker),HOMM3_AUTHORITY_RUNTIME=str(self.args.runtime),HOMM3_HARNESS_SAVE=str(self.args.fixture/'start.gm2'),HOMM3_HARNESS_RNG=str(self.fixture['rng']),HOMM3_HARNESS_OUTPUT=str(out),HOMM3_HARNESS_COMBAT_RNG=self.fixture.get('combatRng',''))
        else:
            # No inherited authority configuration may turn the reference into a candidate.
            for key in ['HOMM3_AUTHORITY_ENGINE','HOMM3_AUTHORITY_RUNTIME','HOMM3_HARNESS_SAVE']:env[key]=''
        self.spawn([str(self.args.relay)],env,out/'relay.log')
        self.spawn(['node','server.mjs'],{'HOST':'127.0.0.1','PORT':str(site_port),'LOBBY_PORT':str(relay_port)},out/'site.log')
        self.url=f'http://127.0.0.1:{site_port}'
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            if any(p.poll() is not None for p in self.processes):raise RuntimeError('Isolated service exited; see logs')
            try:
                await asyncio.to_thread(lambda:urlopen(self.url+'/lobby/health',timeout=1).read());return self
            except OSError:await asyncio.sleep(.05)
        raise TimeoutError('Isolated service startup')
    async def __aexit__(self,*exc):
        for p in self.processes:
            if p.poll() is None:p.terminate()
        for p in self.processes:
            try:p.wait(timeout=5)
            except subprocess.TimeoutExpired:p.kill();p.wait()
        for f in self.files:f.close()


def validate_fixture(directory):
    data=json.loads((directory/'manifest.json').read_text())
    if data.get('combatRng','msvc-lcg32') not in ['msvc-lcg32','browser-libc']:raise ValueError('Unsupported combat RNG')
    if data.get('schema')!=1 or data.get('boundary')!='before-first-turn':raise ValueError('Unsupported fixture schema/boundary')
    if sha(directory/'start.gm2')!=data['sha256']:raise ValueError('Fixture SHA256 mismatch')
    if not isinstance(data.get('rng'),int) or not 0<=data['rng']<=0xffffffff:raise ValueError('Invalid fixture RNG')
    return data


async def mode_run(args,mode,manifest,browser):
    out=args.out/mode;out.mkdir(parents=True,exist_ok=True);contexts=[];pages=[];errors=[];console=[];authority_phases={}
    trace={'schema':1,'mode':mode,'fixtureSha256':manifest['sha256'],'rng':manifest['rng'],'buildConfig':'parity-v1-'+args.clock,'viewport':list(getattr(args,'viewport',[800,600])),'initial':None,'actions':{},'overflow':False,'wasmSha256':sha(args.web/'game.wasm'),'observerSha256':hashlib.sha256(b''.join((ROOT/'scripts/parity'/name).read_bytes() for name in ['browser.js','virtual-audio.js','run.py'])).hexdigest()}
    fixture={**manifest,'saveBase64':base64.b64encode((args.fixture/'start.gm2').read_bytes()).decode()}
    boot_started=time.monotonic()
    async with Services(args,mode,manifest) as service:
      try:
        for seat in range(2):
            view=getattr(args,'viewport',[800,600]);context=await browser.new_context(viewport={'width':view[0],'height':view[1]});contexts.append(context)
            await context.add_init_script(script='window.__homm3ParityClock='+json.dumps(args.clock)+';\n'+(ROOT/'scripts/parity/virtual-audio.js').read_text()+'\n'+(ROOT/'scripts/parity/browser.js').read_text())
            # A single init script avoids unspecified ordering between separate registrations.
            await context.clear_cookies()
            page=await context.new_page();pages.append(page)
            page.on('pageerror',lambda error:errors.append(str(error)))
            def received(data,seat=seat):
                try:
                    message=json.loads(data)
                    if message.get('type')=='authority_status':authority_phases[seat]=message.get('phase')
                    if message.get('type')=='authority_state':
                        state=message.get('state',{})
                        data=json.dumps({**message,'state':{k:v for k,v in state.items() if k!='words'},'wordCount':len(state.get('words',[]))})
                except (ValueError,TypeError):pass
                with (out/'wire.jsonl').open('a') as log:log.write(str(data)[:2500]+'\n')
            page.on('websocket',lambda ws,handler=received:ws.on('framereceived',handler))
            page.on('console',lambda msg:(console.append(msg.text),(out/'live-console.json').write_text(json.dumps(console[-100:]))))
            print(mode,'open seat',seat,flush=True)
            async def game_asset(route):
                name=route.request.url.split('?')[0].rsplit('/',1)[-1]
                await route.fulfill(path=str(args.web/name),content_type='application/wasm' if name.endswith('.wasm') else 'text/javascript')
            await page.route('**/port/game.js*',game_asset);await page.route('**/port/game.wasm*',game_asset)
            await page.goto(service.url+'/port/game.html'+('?hd=0' if list(getattr(args,'viewport',[800,600]))==[800,600] else '')+'#multiplayer')
            await page.evaluate('(fixture)=>window.homm3Harness.fixture=fixture',fixture)
            await page.wait_for_function('!document.querySelector("#start").disabled',timeout=60000)
        a,b=pages;title='Parity '+str(time.time_ns())
        await a.locator('#show-create').click();await a.locator('#player-name').fill('Host');await a.locator('#identity-submit').click()
        await a.locator('#room-name').fill(title);await a.locator('#capacity').select_option('2');await a.locator('#create-submit').click()
        await a.wait_for_function('window.homm3Room?.players.length===1')
        await b.locator('#refresh-rooms').click();await b.get_by_role('button',name='Войти: '+title,exact=True).click();await b.locator('#player-name').fill('Guest');await b.locator('#identity-submit').click()
        print(mode,'joined',flush=True)
        await a.wait_for_function('window.homm3Room?.players.length===2');await a.locator('#ready').click();await b.locator('#ready').click()
        await a.wait_for_function('window.homm3Room?.players.every(p=>p.ready)',timeout=120000);await a.locator('#launch-room').click()
        await a.wait_for_function('window.homm3PortDiagnostics?.screen==="scenarios"',timeout=60000)
        print(mode,'scenarios',flush=True)
        # Startup is not part of the speed score: original assets/JIT and the real
        # network must be ready before virtual time becomes controlled.
        box=await a.locator('#canvas').bounding_box();hd=await a.evaluate('window.homm3HDState||{width:800,height:600}')
        await a.locator('#canvas').click(position={'x':(495+(hd['width']-800)/2)*box['width']/hd['width'],'y':(555+(hd['height']-600)/2)*box['height']/hd['height']})
        deadline=time.monotonic()+90
        while time.monotonic()<deadline:
            if mode=='authority':
                fatal=await a.evaluate('window.homm3Authority?.fatal && window.homm3Authority.lastError')
                if fatal:raise RuntimeError(fatal)
            if any('Game startup:' in line for line in console):raise RuntimeError('Engine startup failed; see live-console.json')
            ready=await asyncio.gather(*(p.evaluate('window.homm3PortDiagnostics?.screen==="adventure" && window.homm3HDState?.layers===0 && !!window.homm3GameState') for p in pages))
            if mode=='authority':ready=[value and authority_phases.get(i)=='adventure' for i,value in enumerate(ready)]
            if all(ready):break
            for p in pages:
                if await p.evaluate('window.homm3HDState?.layers>0 && window.homm3GameState?.day===1'):await p.locator('#canvas').press('Enter')
            for i,p in enumerate(pages):
                if not ready[i] and await p.evaluate('!!window.homm3Harness.fixtureFrozen'):
                    await asyncio.wait_for(p.evaluate('window.homm3Harness.advance(128)'),timeout=10)
            await asyncio.sleep(.01)
        else:raise TimeoutError('Both clients must reach adventure')
        initial=[json.loads(value) for value in await asyncio.gather(*(p.evaluate('JSON.stringify(window.homm3Harness.worlds.find(w=>w.label==="fixture-loaded")||null)') for p in pages))]
        if any(not value for value in initial):raise AssertionError('Missing fixture load checkpoint')
        if initial[0]['world']!=initial[1]['world']:raise AssertionError('Seats loaded different canonical worlds')
        trace['settings']={'game':await a.evaluate('window.homm3Settings'),'browser':args.browser,'browserVersion':browser.version};trace['initial']=initial[0]['world'];trace['startupWallSeconds']=time.monotonic()-boot_started
        if hashlib.sha256(json.dumps(trace['initial'],sort_keys=True).encode()).hexdigest()!=manifest['canonicalSha256']:raise AssertionError('Browser save decoding differs from native fixture')
        if initial[0]['rng']!=manifest['rng'] or initial[1]['rng']!=manifest['rng']:raise AssertionError('RNG not restored')
        if mode=='authority':
            server=json.loads((out/'fixture-loaded.json').read_text())
            if server['world']!=trace['initial'] or server['rng']!=manifest['rng']:raise AssertionError('Server/client initial state mismatch')
        for p in pages:
            await p.locator('#canvas').focus()
            await p.evaluate('window.homm3Harness.controlled=true')
        bounds=await asyncio.gather(*(p.locator('#canvas').bounding_box() for p in pages))
        await asyncio.gather(*(p.evaluate('window.homm3Harness.arm()') for p in pages))
        async def advance(ms):
            return await asyncio.wait_for(asyncio.gather(*(p.evaluate('(ms)=>window.homm3Harness.advance(ms)',ms) for p in pages)),timeout=30)
        # Let the unchanged renderer replace pre-epoch cached pixels and finish
        # scheduled startup fades before the reproducible action stream begins.
        if args.clock=='virtual':await advance(1024)
        async def action(name,seat=0,fault=None,key=None,click=None,hover=False,map=None,widget=None,building=None,build_choice=None,modifiers=None,spell=None,target=None,expect=None,expect_both=None,when=None,until=None,until_seat=None,ms=512):
            for p in pages:await p.evaluate('(name)=>{window.homm3Harness.drain();window.homm3Harness.begin(name);}',name)
            starts=await asyncio.gather(*(p.evaluate('window.homm3Harness.t') for p in pages));begin=time.monotonic()
            def at(value,path):
                if path=='window.text':return '\n'.join(w['text'] for w in (value.get('window') or {}).get('widgets',[]) if w.get('text'))
                for component in path.split('.'):
                    value=value[int(component)] if isinstance(value,list) else (value or {}).get(component)
                return value
            before=await pages[seat].evaluate('window.homm3Harness.ui()')
            enabled=not when or all(at(before,path)==value for path,value in when.items())
            if not enabled:key=click=map=widget=building=build_choice=spell=target=None
            global_click=widget is not None or spell is not None or build_choice is not None or building is not None
            if widget is not None or spell is not None or build_choice is not None:
                await pages[seat].evaluate('Module._homm3_harness_checkpoint()')
                widgets=await pages[seat].evaluate('window.homm3Harness.interfaceState?.widgets||[]')
                choices=[w for w in widgets if w['status']&2==2 and not w['status']&32 and ((build_choice is not None and 700<=w['id']<800 and w.get('frame')==build_choice) or (spell is not None and 213<=w['id']<=224 and w.get('frame')==spell) or (widget is not None and w['id']==widget))]
                if len(choices)!=1:raise AssertionError(f'{name}: semantic control not uniquely available')
                w=choices[0];click=[w['x']+w['width']/2,w['y']+w['height']/2]
            if building is not None:
                await pages[seat].evaluate('Module._homm3_harness_checkpoint()')
                spots=await pages[seat].evaluate('window.homm3Harness.interfaceState?.hotspots||[]')
                spot=next((s for s in spots if s['building']==building),None)
                if not spot:raise AssertionError(f'{name}: building {building} has no visible original hotspot')
                click=[spot['x'],spot['y']]
            if target is not None:
                stacks=await pages[seat].evaluate('window.homm3CombatState?.stacks||[]')
                stack=next(s for s in stacks if [s['side'],s['slot']]==target);hex=stack['hex']
                click=[(hex%17)*44+36+22*(1-(hex//17)%2),(hex//17)*42+106]
            hd=await pages[seat].evaluate('window.homm3HDState||{width:800,height:600,layers:0}')
            if map is not None:
                radar=await pages[seat].evaluate('window.homm3NetworkState')
                click=[8+32*(map[0]-radar['radarX'])+16,8+32*(map[1]-radar['radarY'])+16];global_click=True
            if fault and enabled:
                if fault!='reject-move' or mode!='authority':raise ValueError('Command fault requires an authority-only rejection replay')
                await pages[seat].evaluate("""() => {
                    const api=window.homm3Authority,original=api.send;
                    api.send=function(op,id,direction,...rest){
                        if(op!==1)return original.call(this,op,id,direction,...rest);
                        api.send=original;
                        return original.call(this,op,id,8,...rest);
                    };
                }""")
            if key:await pages[seat].keyboard.press(key)
            for modifier in modifiers or []:await pages[seat].keyboard.down(modifier)
            if click:
                box=await pages[seat].locator('#canvas').bounding_box()
                if not global_click:
                    if hd.get('layers',0):click=[click[0]+(hd['width']-800)/2,click[1]+(hd['height']-600)/2]
                    elif click[0]>=600:click=[click[0]+hd['width']-800,click[1]]
                pointer=pages[seat].mouse.move if hover else pages[seat].mouse.click
                await pointer(box['x']+click[0]*box['width']/hd['width'],box['y']+click[1]*box['height']/hd['height'])
            budget=ms
            try:
                if until:
                    ms=0;observed_seat=seat if until_seat is None else until_seat
                    while ms<budget:
                        step=min(128,budget-ms);await advance(step);ms+=step
                        await pages[observed_seat].evaluate('Module._homm3_harness_checkpoint()')
                        observed=await pages[observed_seat].evaluate('window.homm3Harness.ui()')
                        if all(at(observed,path)==value for path,value in until.items()):break
                    else:raise AssertionError(f'{name}: event not reached within {budget} virtual ms: {until}')
                else:await advance(ms)
            finally:
                for modifier in reversed(modifiers or []):await pages[seat].keyboard.up(modifier)
            for p in pages:await p.evaluate('Module._homm3_harness_checkpoint()')
            chunks=[json.loads(value) for value in await asyncio.gather(*(p.evaluate('JSON.stringify(window.homm3Harness.drain())') for p in pages))];wall=(time.monotonic()-begin)*1000
            for i,chunk in enumerate(chunks):
                world=chunk['worlds'][-1]['world'];state=chunk['ui'];adventure=state.get('adventure') or {};heroes=adventure.get('heroes') or []
                invariants=[{'id':'owned-hero-present','passed':all(h['onMap'] or (h['id']==adventure.get('hero') and adventure.get('mobile')) or any(t['garrisonHero']==h['id'] for t in world['towns']) for h in heroes)}]
                ui_window=state.get('window')
                if ui_window:
                    for item in ui_window['widgets']:item['status']&=1|2|4|8|16|32|8192
                stable={'window':ui_window,'world':{'heroes':[h for h in world['heroes'] if h['owner']==i],'towns':[t for t in world['towns'] if t['owner']==i],'resources':world['players'][i]['resources']},'adventure':adventure,'combat':state.get('combat'),'game':{k:state.get('game',{}).get(k) for k in ['day','week','month','player','combat','round']},'video':state.get('video'),'screen':state.get('screen'),'layers':state.get('hd',{}).get('layers')}
                audio=[{'t':e['t']-starts[i],'kind':e['kind'],'data':e['data']} for e in chunk['events'] if e['kind'].startswith(('homm3Music.','homm3Sfx.'))]
                # Quantized PCM hash detects waveform changes even at equal RMS/peak.
                # Compare runs from the same pinned browser/decoder.
                pcm=[{'rms':round(v['rms'],5),'peak':round(v['peak'],5),'samples':v['samples'],'hash':v['hash']} for v in chunk['audio']]
                trace['actions'][name+f'/p{i}']={'start':starts[i],'inputApplied':enabled,'state':stable,'invariants':invariants,'frames':chunk['frames'],'audioEvents':audio,'pcm':pcm,'virtualMs':ms,'budgetMs':budget,'eventReached':until,'wallMs':wall,'speedup':ms/wall}
                first=chunk['frames'][0] if chunk['frames'] else None
                changed=next((f for f in chunk['frames'] if first and f['tiles']!=first['tiles']),None)
                trace['actions'][name+f'/p{i}']['firstChangedFrameMs']=None if not changed else {'virtual':changed['t']-starts[i],'wall':changed.get('wallMs')}
                trace['overflow']|=chunk['overflow'];(out/(name+f'-p{i}.json')).write_text(json.dumps(chunk,separators=(',',':')))
            for i in range(2):
                failed=[check['id'] for check in trace['actions'][name+f'/p{i}']['invariants'] if not check['passed']]
                if failed:raise AssertionError(f'{name}/p{i}: invariant failed: {failed}')
            for check_seat,checks in [(seat,expect or {})]+[(i,expect_both or {}) for i in range(2)]:
                for path,value in checks.items():
                    current=at(chunks[check_seat]['ui'],path)
                    if current!=value:raise AssertionError(f'{name}/p{check_seat}: {path} expected {value!r}, got {current!r}')
            print(mode,name,f'{ms} virtual ms / {wall:.0f} wall ms',flush=True)
        steps=[{'name':'idle','ms':args.virtual_ms}]
        if args.scenario:
            steps=json.loads(args.scenario.read_text())
        elif args.suite=='adventure':
            steps += [
                {'name':'waiting-town','seat':1,'click':[770,230]},
                {'name':'waiting-hero','seat':1,'click':[640,230]},
                {'name':'move','key':'ArrowDown','ms':1024},
                {'name':'town-entry','key':'ArrowUp','ms':1024},
                {'name':'garrison','key':'Space'}, {'name':'visitor','key':'Space'},
                {'name':'town-exit','key':'Escape'}]
        elif args.suite=='turns':
            for n in range(14):
                seat=n%2;day=1+(n+1)//2
                steps += [{'name':f'handoff-{n+1:02d}','seat':seat,'key':'e','ms':256},
                    {'name':f'confirm-{n+1:02d}','seat':seat,'key':'Enter','when':{'hd.layers':1,'game.player':seat},'ms':4096}]
                for notice in range(4):
                    steps.append({'name':f'accept-{n+1:02d}-{notice+1}','seat':1-seat,'key':'Enter','when':{'hd.layers':1},'ms':4096})
                steps[-1]['expect_both']={'game.player':1-seat,'game.day':(day-1)%7+1,'game.week':(day-1)//7+1}
        (out/'scenario.json').write_text(json.dumps(steps,indent=2))
        names=set()
        import re
        for step in steps:
            if not isinstance(step,dict) or not re.fullmatch(r'[a-zA-Z0-9_-]{1,64}',step.get('name','')) or step['name'] in names:raise ValueError('Invalid/duplicate scenario step')
            if set(step)-{'name','seat','fault','key','click','hover','map','widget','building','build_choice','modifiers','spell','target','expect','expect_both','when','ms','until','until_seat'} or step.get('seat',0) not in [0,1]:raise ValueError('Invalid scenario fields')
            if not 0<step.get('ms',512)<=600000:raise ValueError('Invalid scenario virtual budget')
            if step.get('until_seat',step.get('seat',0)) not in [0,1] or ('until' in step and (not isinstance(step['until'],dict) or not step['until'])):raise ValueError('Invalid event condition')
            names.add(step['name']);await action(**step)
        trace['errors']=errors
        if not trace['actions'] or any(not v['frames'] for v in trace['actions'].values()) or not any(not f.get('held') for v in trace['actions'].values() for f in v['frames']):raise AssertionError('Missing rendered frame evidence')
        if any(not i['passed'] for v in trace['actions'].values() for i in v['invariants']):raise AssertionError('UI invariant failed')
        if errors:raise AssertionError('Browser errors: '+repr(errors))
      except Exception as error:
        trace['failure']=repr(error);trace['overflow']=True
        for i,p in enumerate(pages):
            try:(out/f'failure-p{i}.json').write_text(await asyncio.wait_for(p.evaluate('JSON.stringify({harness:window.homm3Harness?.drain(),room:window.homm3Room,diagnostics:window.homm3PortDiagnostics,authority:window.homm3AuthorityStatus})'),timeout=5))
            except Exception:pass
      finally:
        (out/'trace.json').write_text(json.dumps(trace,separators=(',',':')));(out/'console.json').write_text(json.dumps(console,ensure_ascii=False));(out/'errors.json').write_text(json.dumps(errors))
        for c in contexts:await asyncio.wait_for(c.close(),timeout=5)
    return trace


async def main(args):
    for name in ['out','fixture','runtime','worker','web','relay']:
        setattr(args,name,getattr(args,name).resolve())
    args.out.mkdir(parents=True,exist_ok=True);manifest=validate_fixture(args.fixture)
    async with async_playwright() as playwright:
        browser=await getattr(playwright,args.browser).launch()
        try:
            traces={}
            for mode in args.modes:traces[mode]=await mode_run(args,mode,manifest,browser)
        finally:await browser.close()
    result=compare(traces['legacy'],traces['authority']) if len(traces)==2 else {'passed':not any(t.get('failure') for t in traces.values()),'failures':[{'channel':'run','error':t['failure']} for t in traces.values() if t.get('failure')]}
    metadata={'traces':{mode:{k:t.get(k) for k in ['startupWallSeconds','failure','errors']} for mode,t in traces.items()},'wasmSha256':sha(args.web/'game.wasm'),'workerSha256':sha(args.worker),'fixture':manifest}
    (args.out/'comparison.json').write_text(json.dumps(result,indent=2));report(result,args.out/'index.html',metadata)
    print(json.dumps(result,ensure_ascii=False)[:3000]);return 0 if result['passed'] else 1

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime',type=Path,default=ROOT.parent/'runtime');parser.add_argument('--relay',type=Path,default=ROOT.parent/'homm3-lobby/target/debug/homm3-lobby')
    parser.add_argument('--worker',type=Path,default=ROOT/'build/cmake-parity-native/homm3-worker');parser.add_argument('--web',type=Path,default=ROOT/'build/cmake-parity-web/web')
    parser.add_argument('--fixture',type=Path,default=ROOT/'test-results/parity/fixture');parser.add_argument('--out',type=Path,default=ROOT/'test-results/parity/run')
    parser.add_argument('--scenario',type=Path,help='Replay a saved scenario.json (same fixture required)')
    parser.add_argument('--viewport',type=int,nargs=2,default=[800,600],metavar=('WIDTH','HEIGHT'))
    parser.add_argument('--clock',choices=['virtual','real'],default='virtual')
    parser.add_argument('--browser',choices=['chromium','webkit'],default='chromium');parser.add_argument('--modes',nargs='+',choices=['legacy','authority'],default=['legacy','authority']);parser.add_argument('--suite',choices=['idle','adventure','turns'],default='adventure');parser.add_argument('--virtual-ms',type=int,default=60000)
    raise SystemExit(asyncio.run(main(parser.parse_args())))
