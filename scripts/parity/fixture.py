#!/usr/bin/env python3
"""Generate one original-format save and retain exact RNG continuation.
Round-trip on a differently seeded worker is mandatory before browser replay.
Fixtures contain licensed game data and stay in ignored test-results/.
"""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
from compare import first_difference
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--worker',type=Path,default=ROOT/'build/cmake-parity-native/homm3-worker');p.add_argument('--runtime',type=Path,default=ROOT.parent.parent/'.local/runtime');p.add_argument('--map',default='maps/Arrogance.h3m');p.add_argument('--magic',action='store_true');p.add_argument('--natural-setup',action='store_true',help='Use original map slot/faction/hero setup instead of the Arrogance preset');p.add_argument('--object',type=int,help='Place hero south of an actual object; clear nearby neutral guards');p.add_argument('--exchange',action='store_true',help='Two owned heroes in the walking corridor');p.add_argument('--movement',action='store_true',help='Flat disclosed walking corridor');p.add_argument('--feedback',choices=['positive','negative','zero','unlucky'],help='Non-immune ranged army with saved morale/luck bonuses');p.add_argument('--combat',action='store_true',help='Conflux/Brissa fixture with Haste');p.add_argument('--combat-rng',choices=['msvc-lcg32','browser-libc'],default='msvc-lcg32',help='RNG used by both lanes after the original combat reseed');p.add_argument('--town',type=int,choices=range(9));p.add_argument('--town-profile',choices=['basic','full'],default='basic');p.add_argument('--seed',type=int,default=481516);p.add_argument('--out',type=Path,default=ROOT/'test-results/parity/fixture');args=p.parse_args();args.out=args.out.resolve();args.out.mkdir(parents=True,exist_ok=True)
env={**os.environ,'SDL_AUDIODRIVER':'dummy','SDL_VIDEODRIVER':'dummy','HOMM3_HARNESS_OUTPUT':str(args.out)}
for key in ['HOMM3_HARNESS_SAVE','HOMM3_HARNESS_RNG','HOMM3_HARNESS_EXPORT','HOMM3_HARNESS_ADVENTURE','HOMM3_HARNESS_TOWN','HOMM3_HARNESS_MAGIC','HOMM3_HARNESS_MOVEMENT','HOMM3_HARNESS_EXCHANGE','HOMM3_HARNESS_FEEDBACK']:env.pop(key,None)
if args.feedback:env['HOMM3_HARNESS_FEEDBACK']=args.feedback
if args.exchange:env['HOMM3_HARNESS_EXCHANGE']='1'
if args.movement or args.exchange:env['HOMM3_HARNESS_MOVEMENT']='1'
command=[str(args.worker),str(args.runtime),args.map,str(args.seed),'--humans','2']
if args.combat or args.town is not None or args.object is not None or args.magic:
    rows=[[0,8,137,0,0],[1,0,0,0,0],[-1,-1,-1,0,0],[-1,-1,-1,0,0],[-1,-1,-1,0,0],[10,3,-1,0,0],[10,5,-1,0,0],[-1,-1,-1,0,0]]
    if args.town is not None:
        rows[0][1:3]=[args.town,args.town*16];rows[1][2]=15;env['HOMM3_HARNESS_TOWN']=args.town_profile
    if args.magic:env['HOMM3_HARNESS_MAGIC']='1'
    if args.object is not None:env['HOMM3_HARNESS_ADVENTURE']=str(args.object)
    if not args.natural_setup:command+=['--setup',','.join(map(str,[1]+[v for row in rows for v in row]))]
export=subprocess.run(command,env={**env,'HOMM3_HARNESS_EXPORT':str(args.out/'start.gm2')},capture_output=True,text=True,timeout=30)
if export.returncode:raise RuntimeError('Fixture export failed: '+export.stderr)
expected=json.loads((args.out/'fixture-exported.json').read_text());command[3]='99'
for seat,roster in enumerate(expected['world']['rosters']):
    count=roster['count'];heroes=roster['heroes']
    if not 0<=count<=8 or any(hero<0 for hero in heroes[:count]) or any(hero!=-1 for hero in heroes[count:]):
        raise RuntimeError(f'Invalid hero roster in fixture seat {seat}: {roster}')
subprocess.run(command,input='quit\n',text=True,env={**env,'HOMM3_HARNESS_SAVE':str(args.out/'start.gm2'),'HOMM3_HARNESS_RNG':str(expected['rng'])},check=True,capture_output=True,timeout=30)
actual=json.loads((args.out/'fixture-loaded.json').read_text());diff=first_difference(expected,actual)
if diff:raise RuntimeError('Save round-trip failed: '+json.dumps(diff))
manifest={'schema':1,'boundary':'before-first-turn','map':args.map,'seed':args.seed,'rng':expected['rng'],'rngAlgorithm':'msvc-lcg32','sha256':hashlib.sha256((args.out/'start.gm2').read_bytes()).hexdigest(),'canonicalSha256':hashlib.sha256(json.dumps(expected['world'],sort_keys=True).encode()).hexdigest(),'workerSha256':hashlib.sha256(args.worker.read_bytes()).hexdigest(),'roundTrip':True,'profile':('town-'+args.town_profile) if args.town is not None else ('combat' if args.combat else 'adventure'),'town':args.town,'combatRng':args.combat_rng,'movementCorridor':args.movement or args.exchange,'exchange':args.exchange,'feedback':args.feedback}
setup_path=args.out/'fixture-setup.json'
if setup_path.exists():manifest['setup']=json.loads(setup_path.read_text())
if args.object is not None:
    manifest['adventure']=json.loads((args.out/'adventure-fixture.json').read_text())
    metadata=manifest['adventure'];size=expected['world']['size']
    if not all(0<=n<bound for position in [metadata['target'],metadata['start']] for n,bound in zip(position,size)):
        raise RuntimeError('Adventure fixture placed outside map')
    hero=next(h for h in expected['world']['heroes'] if h['id']==metadata['hero'])
    if hero['pos']!=metadata['start']:raise RuntimeError('Adventure fixture hero not at start')
(args.out/'manifest.json').write_text(json.dumps(manifest,indent=2));print(json.dumps(manifest,indent=2))
