"""Run native manual battle, tactics, spell, retreat and surrender fixtures.

Build HOMM3_SERVER first. This test never starts a listener or changes live rooms.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime', required=True, type=Path)
parser.add_argument('--build', type=Path, default=ROOT / 'build/cmake-server')
args = parser.parse_args()
output = ROOT / 'test-results/authority-combat'
output.mkdir(parents=True, exist_ok=True)
build, runtime = args.build.resolve(), args.runtime.resolve()
env = {**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy'}
objects = [str(p) for p in sorted((build / 'CMakeFiles/homm3-port.dir').rglob('*.o'))
           if p.name != 'main.cpp.o']
assert objects, 'Build the worker first'
media = ROOT / 'build/ffmpeg-native/install'
probe = output / 'combat-probe'
command = ['clang++', '-std=c++14', '-DHOMM3_PORT', '-DHOMM3_FULL_GAME', '-DHOMM3_SERVER',
           '-include', 'port/compat/prefix.h', '-Iport/compat', '-Iinclude',
           '-O2', '-Wno-writable-strings', '-Wno-deprecated-declarations',
           *shlex.split(subprocess.check_output(['sdl2-config', '--cflags', '--libs'], text=True)),
           '-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections',
           'port/probes/authority_combat.cpp', *objects, '-I' + str(media / 'include'),
           *[str(media / f'lib/lib{name}.a') for name in ['avformat', 'avcodec', 'swscale', 'swresample', 'avutil']],
           '-lz', '-o', str(probe)]
compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
(output / 'probe-build.log').write_text(compiled.stdout + compiled.stderr)
assert compiled.returncode == 0, 'Probe compile failed; see probe-build.log'
cases = [({'AI':'1','AUTO':'1'}, 'server_auto_combat'), ({'AI':'1','AUTO':'1','AUTO_STOP':'1'}, 'server_auto_stop'), ({}, 'manual_pvp'), ({'TACTICS':'1'}, 'manual_tactics'),
         ({'AI':'1'}, 'neutral_ai'), ({'ACTION':'4'}, 'retreat'),
         ({'ACTION':'5'}, 'server_surrender_price'), ({'CREATURE':'2'}, 'ranged'),
         ({'CREATURE':'10'}, 'double_wide'), ({'CREATURE':'27'}, 'flying'),
         ({'SIEGE':'1'}, 'siege'), ({'SIEGE':'1','SPELL':'14'}, 'earthquake')]
for spell in [15,17,41,53,54,42,43,44,56,12,13,63,64,65,67]:
    cases.append(({'SPELL':str(spell),**({'CREATURE':'2'} if spell==44 else {})}, 'spell_'+str(spell)))
results=[]
for case, name in cases:
    run=subprocess.run([str(probe),str(runtime)],env={**env,**case},capture_output=True,text=True,timeout=30)
    (output/(name+'.log')).write_text(run.stdout+run.stderr)
    assert run.returncode==0, name+': '+str(run.returncode)+' '+ '\n'.join(line for line in run.stderr.splitlines() if not line.startswith('RESULT '))
    summary = json.loads(run.stdout.strip().splitlines()[-1])
    frames = [json.loads(line.split(' ', 2)[2]) for line in run.stderr.splitlines() if line.startswith('RESULT ')]
    assert len(frames) == (1 if case.get('AI') else 2), name + ': result participants'
    for side, frame in enumerate(frames):
        assert frame['mySide'] == side and frame['winner'] == summary['winner'], name + ': result identity'
        events=frame['presentation']
        assert len(events)<=256 and all(len(e)==12 and all(isinstance(n,int) for n in e) for e in events), name+': bounded visual records'
        assert all(e[0]>0 and (i==0 or e[0]==events[i-1][0]+1) for i,e in enumerate(events)), name+': ordered visual sequence'
        assert all(1<=e[1]<=12 and 0<=e[2]<2 and 0<=e[3]<21 for e in events), name+': visual stack identity'
        assert all(0<=e[6]<81 for e in events if e[1]==9), name+': successful spell sound identity'
        assert all(e[6]>=0 and e[7]>=0 and e[8]>=0 for e in events if e[1]==5), name+': resolved damage facts'
        assert all(len(stack['modifiers'])==25 and len(stack['effectOrder'])<=81 for stack in frame['stacks']), name+': spell presentation modifiers'
        assert frame['actions'] == [] and frame['canAct'] is False, name + ': result action authority'
        assert all(0 <= stack['count'] <= 10000 and stack['originalCount'] >= 0 for stack in frame['stacks']), name + ': casualties'
        if side != summary['winner']:
            assert frame['experience'] == 0 and frame['loot'] == [] and frame['eagleEye'] == [], name + ': reward privacy'
        elif side == 0:
            assert frame['experience'] == summary['xp'], name + ': original experience'
    results.append({'name':name, 'resultEvents':len(frames), 'presentationKinds':sorted(set(e[1] for f in frames for e in f['presentation'])), **summary})
run=subprocess.run([str(probe),str(runtime)],env={**env,'INVALID':'1'},capture_output=True,text=True,timeout=30)
assert run.returncode==1 and 'invalid option' in run.stderr.lower(), 'invalid choice must terminate before action'
results.append({'name':'invalid_choice_fails_closed','passed':True})
run=subprocess.run([str(probe),str(runtime)],env={**env,'INVALID_ARGUMENTS':'1'},capture_output=True,text=True,timeout=30)
assert run.returncode==1 and 'invalid arguments' in run.stderr.lower(), 'forged auxiliary arguments must terminate before action'
results.append({'name':'forged_arguments_fail_closed','passed':True})
(output/'results.json').write_text(json.dumps({'passed':True,'cases':results},indent=2)+'\n')
print(json.dumps({'passed':True,'cases':len(results)}))
