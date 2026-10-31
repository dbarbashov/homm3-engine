"""Real native worker + isolated Rust relay: crash recovery and report replay.
No live rooms, browsers or sound devices are touched.
"""
import asyncio,json,os,socket,subprocess,time,urllib.request,argparse,shutil
from pathlib import Path
import websockets
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--runtime',type=Path,default=ROOT.parent/'runtime');p.add_argument('--relay',type=Path,default=ROOT.parent/'homm3-lobby/target/release/homm3-lobby');p.add_argument('--worker',type=Path,default=ROOT.parent/'homm3-lobby/build/worker/homm3-worker');p.add_argument('--out',type=Path,default=ROOT/'test-results/match-history');a=p.parse_args();a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=True)
# A unique store preserves previous evidence instead of deleting it.
store=a.out/str(time.time_ns());store.mkdir();relay=a.relay.resolve()
s=socket.socket();s.bind(('127.0.0.1',0));port=s.getsockname()[1];s.close();url=f'http://127.0.0.1:{port}'
env={**os.environ,'LOBBY_BIND':f'127.0.0.1:{port}','HOMM3_AUTHORITY_ENGINE':str(a.worker.resolve()),'HOMM3_AUTHORITY_RUNTIME':str(a.runtime.resolve()),'HOMM3_HISTORY_DIR':str(store),'HOMM3_WEB_ROOT':str(ROOT/'web')}
proc=None;logs=[]
async def start():
 global proc
 log=open(store/f'relay-{len(logs)}.log','w');logs.append(log);proc=subprocess.Popen([str(relay)],env=env,stdout=log,stderr=subprocess.STDOUT)
 for _ in range(600):
  if proc.poll() is not None:raise RuntimeError('Relay failed: '+Path(log.name).read_text())
  try:await asyncio.to_thread(lambda:urllib.request.urlopen(url+'/lobby/health',timeout=1).read());return
  except OSError:await asyncio.sleep(.05)
 raise TimeoutError('Relay boot')
class Peer:
 async def open(self,name,token=''):
  self.ws=await websockets.connect(url.replace('http','ws')+'/lobby/socket',max_size=8*1024*1024);self.generation=None;self.revision=0
  await self.send(type='hello',name=name,token=token);hello=await self.until(lambda v:v['type']=='hello');self.token=hello['token'];return self
 async def send(self,**v):await self.ws.send(json.dumps(v))
 async def read(self):
  v=json.loads(await asyncio.wait_for(self.ws.recv(),35))
  if v['type'] in ['error','authority_fault']:raise RuntimeError(str(v))
  if v['type'].startswith('authority_'):assert 'rng' not in v,'Private RNG leaked to browser'
  if v['type']=='authority_ready':self.generation=v['generation'];self.revision=v['revision']
  if v['type']=='authority_state':self.revision=v['state']['revision'];self.state=v['state']
  return v
 async def until(self,pred):
  while True:
   v=await self.read()
   if pred(v):return v
 async def settle(self,request=None):
  while True:
   v=await self.read()
   if v['type']=='authority_choice':
    if v['kind'] in ['combat','combat_spell']:return v
    text=v.get('text','').encode('latin1').decode('cp1251');value=v['options'][-1] if ('принять' in text or 'присоедин' in text) else v['options'][0]
    await self.send(type='authority_answer',generation=self.generation,id=f'answer:{v["token"]}',token=v['token'],value=value)
   if v['type']=='authority_result' and (request is None or v['id']==request):
    assert v['ok'],v;return v
 async def command(self,request_id,**command):
  await self.send(type='authority_command',generation=self.generation,id=request_id,revision=self.revision,command=command)
  return await self.settle(request_id)
 async def report(self):
  await self.send(type='authority_report',diagnostics={'test':'same-state recovery','viewport':[800,600]})
  v=await self.until(lambda v:v['type']=='authority_report');return v['url'].split('/')[-1]
 async def close(self):await self.ws.close()
async def main():
 global proc
 await start();left=await Peer().open('History host');right=await Peer().open('History guest')
 await left.send(type='create',title='Durable history',capacity=2,private=False);room=(await left.until(lambda v:v['type']=='room'))['room']['id']
 await right.send(type='join',room=room);await right.until(lambda v:v['type']=='room')
 for peer in [left,right]:await peer.send(type='ready',ready=True,build='a'*64)
 await left.until(lambda v:v['type']=='room' and all(x['ready'] for x in v['room']['players']))
 await left.send(type='launch');await left.until(lambda v:v['type']=='room' and v['room']['phase']=='setup')
 setup=[1]+[v for row in [[0,8,137,0,0],[1,0,0,0,0],[-1,-1,-1,0,0],[-1,-1,-1,0,0],[-1,-1,-1,0,0],[10,3,-1,0,0],[10,5,-1,0,0],[-1,-1,-1,0,0]] for v in row]
 await left.send(type='authority_start',map='Arrogance.h3m',setup=setup)
 opening=await left.until(lambda v:v['type']=='authority_choice')
 assert opening['kind']=='normal'
 await left.close();await right.close();proc.kill();proc.wait()
 await start();left=await Peer().open('ignored',left.token);right=await Peer().open('ignored',right.token)
 restored_opening=await left.until(lambda v:v['type']=='authority_choice')
 assert restored_opening==opening,'Pending original dialog changed after crash'
 await left.send(type='authority_answer',generation=left.generation,id='opening-recovered',token=opening['token'],value=opening['options'][0])
 await left.settle('@bootstrap');await left.until(lambda v:v['type']=='authority_state' and v['state']['revision']==1)
 generation=left.generation
 for n,direction in enumerate([4,4,5]):
  result=await left.command(f'move{n}',op=1,id=137,a=direction,b=0)
  if result['type']=='authority_choice':break
  await left.until(lambda v:v['type']=='authority_state' and v['state']['revision']>n+1)
 if result['type']!='authority_choice':raise RuntimeError('Expected battle choice, got '+str(result))
 if result['kind']!='combat':raise RuntimeError('Expected combat')
 for i in range(3):
  action=next(x for x in result['data']['actions'] if x['action']==3)
  await left.send(type='authority_answer',generation=generation,id=f'fight:{i}',token=result['token'],value=action['id'])
  result=await left.until(lambda v:v['type']=='authority_choice' and v['kind']=='combat')
 before=result['data'];token=result['token'];report=await left.report()
 expected=json.loads(json.dumps(before));assert before['presentation'],'Need actual battle history'
 await left.close();await right.close();proc.kill();proc.wait()
 # The next relay uses only durable disk history and the original browser token.
 began=time.monotonic();await start();left=await Peer().open('ignored',left.token)
 restored=await left.until(lambda v:v['type']=='authority_choice' and v['kind']=='combat')
 assert left.generation==generation;assert restored['token']==token;assert restored['data']==expected,'Restored battle differs'
 recovery=time.monotonic()-began
 # Exact retry tokens and an isolated report replay must retain the same state.
 response=await asyncio.to_thread(lambda:json.loads(urllib.request.urlopen(urllib.request.Request(url+f'/lobby/reports/{report}/replay',method='POST'),timeout=40).read()))
 assert response['verified'] and response['choice']['data']==expected
 defend=next(x for x in restored['data']['actions'] if x['action']==3)
 await left.send(type='authority_answer',generation=generation,id='after-recovery',token=token,value=defend['id'])
 next_choice=await left.until(lambda v:v['type']=='authority_choice' and v['token']!=token)
 # Reusing an acknowledged answer must not advance the battle a second time.
 await left.send(type='authority_answer',generation=generation,id='after-recovery',token=token,value=defend['id'])
 ack=await left.until(lambda v:v['type']=='authority_answer_result' and v['id']=='after-recovery');assert ack['ok']
 # Exit to the browser, restore its saved-game list and rejoin the same seat.
 await left.send(type='leave');await left.until(lambda v:v['type']=='left')
 await left.send(type='saved_games');games=await left.until(lambda v:v['type']=='saved_games');assert any(g['room']==room for g in games['games'])
 await left.send(type='rename',name='Renamed host');await left.until(lambda v:v['type']=='renamed')
 await left.send(type='resume_game',room=room);latest=await left.until(lambda v:v['type']=='authority_choice');assert latest['token']==next_choice['token'],'Duplicate input advanced battle'
 # The old bug link stays pinned after the real party continues.
 pinned=await asyncio.to_thread(lambda:json.loads(urllib.request.urlopen(urllib.request.Request(url+f'/lobby/reports/{report}/replay',method='POST'),timeout=40).read()))
 assert pinned['choice']['data']==expected
 outsider=await Peer().open('Not a participant');await outsider.send(type='resume_game',room=room)
 denied=json.loads(await asyncio.wait_for(outsider.ws.recv(),5))
 while denied['type']!='error':denied=json.loads(await asyncio.wait_for(outsider.ws.recv(),5))
 assert 'доступ' in denied['message'];await outsider.close()
 await left.send(type='leave');await left.until(lambda v:v['type']=='left')
 await left.send(type='create',title='Temporary Fast lobby',capacity=2,fastGame=True,private=False)
 await left.until(lambda v:v['type']=='room' and v['room']['fastGame'])
 await left.close();proc.kill();proc.wait();await start();left=await Peer().open('ignored',left.token)
 available=await left.until(lambda v:v['type']=='saved_games');assert any(g['room']==room for g in available['games'])
 await left.send(type='resume_game',room=room);again=await left.until(lambda v:v['type']=='authority_choice');assert again['token']==next_choice['token']
 await left.close();proc.terminate();proc.wait();proc=None
 metrics={'passed':True,'store':str(store),'generation':generation,'report':report,'recoverySeconds':round(recovery,3),'journalBytes':(store/'matches'/generation/'events.bin').stat().st_size,'canonicalBattleSame':True,'pendingTokenSame':True,'reportReplayVerified':True,'continuedAfterRecovery':True,'dialogRecovered':True,'duplicateAnswerIdempotent':True,'savedPartyRejoined':True,'reportPinnedAfterContinuation':True,'outsiderDenied':True,'temporaryFastRoomDoesNotCorruptSavedSeat':True}
 (a.out/'results.json').write_text(json.dumps(metrics,indent=2));print(json.dumps(metrics))
try:asyncio.run(main())
finally:
 if proc and proc.poll() is None:proc.terminate();proc.wait()
 for log in logs:log.close()
