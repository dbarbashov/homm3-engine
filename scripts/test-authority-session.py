"""Run the native ordinary-session rule fixture and bounded real worker IPC.

Build HOMM3_SERVER first. This test never starts a listener or changes live rooms.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import select
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime', required=True, type=Path)
parser.add_argument('--build', type=Path, default=ROOT / 'build/cmake-server')
parser.add_argument('--map', default='maps/Good to Go.h3m')
args = parser.parse_args()
output = ROOT / 'test-results/authority-session'
output.mkdir(parents=True, exist_ok=True)
build, runtime = args.build.resolve(), args.runtime.resolve()
env = {**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy'}
objects = [str(p) for p in sorted((build / 'CMakeFiles/homm3-port.dir').rglob('*.o'))
           if p.name != 'main.cpp.o']
assert objects, 'Build the worker first'
media = ROOT / 'build/ffmpeg-native/install'
probe = output / 'session-probe'
command = ['clang++', '-std=c++14', '-DHOMM3_PORT', '-DHOMM3_FULL_GAME', '-DHOMM3_SERVER',
           '-include', 'port/compat/prefix.h', '-Iport/compat', '-Iinclude',
           '-O2', '-Wno-writable-strings', '-Wno-deprecated-declarations',
           *shlex.split(subprocess.check_output(['sdl2-config', '--cflags', '--libs'], text=True)),
           '-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections',
           'port/probes/authority_session.cpp', *objects, '-I' + str(media / 'include'),
           *[str(media / f'lib/lib{name}.a') for name in ['avformat', 'avcodec', 'swscale', 'swresample', 'avutil']],
           '-lz', '-o', str(probe)]
compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
(output / 'probe-build.log').write_text(compiled.stdout + compiled.stderr)
assert compiled.returncode == 0, 'Probe compile failed; see probe-build.log'
run = subprocess.run([str(probe), str(runtime), args.map], env=env, capture_output=True, text=True, timeout=120)
(output / 'probe.log').write_text(run.stdout + run.stderr)
assert run.returncode == 0, 'Original-rule fixture failed; see probe.log'
probe_result = json.loads(run.stdout.strip().splitlines()[-1])
checks = []
with subprocess.Popen([str(build / 'homm3-worker'), str(runtime), args.map, '42', '--humans', '2'],
                      stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                      bufsize=0, env=env) as worker:
    pending = bytearray()

    def send(line):
        worker.stdin.write((line + '\n').encode())
        worker.stdin.flush()

    def read():
        while b'\n' not in pending:
            assert select.select([worker.stdout], [], [], 30)[0], 'Worker response timeout'
            chunk = os.read(worker.stdout.fileno(), 65536)
            assert chunk, f'Worker exited {worker.poll()}: {worker.stderr.read()}'
            pending.extend(chunk)
        line, _, rest = pending.partition(b'\n')
        pending[:] = rest
        return json.loads(line)

    turn_events=[];notices=[]

    def call(line):
        send(line)
        while True:
            event = read()
            if event['type']=='status' and line.startswith('command '):turn_events.append(event)
            if event['type'] == 'choice':
                notices.append(event)
                send(f"answer {event['token']} {event['actor']} {event['options'][0]}")
            elif event['type'] not in ('choice_accepted', 'state') and not (event['type']=='status' and line.startswith('command ')):
                return event

    try:
        ready = read()
        assert ready['protocol'] == 2 and ready['type'] == 'ready' and len(ready['roster']) == 2
        first = ready['activeActor']
        second = next(p['slot'] for p in ready['roster'] if p['slot'] != first)
        checks.append('ready authenticated roster')
        assert not call(f'command 0 {first} 5 0 0 0')['ok']
        assert call(f'command 0 {first} 0 0 0 0')['revision'] == 1
        checks.append('explicit initialized turn')
        for bad in [f'command 1 {second} 5 0 0 0', f'command 0 {first} 5 0 0 0',
                    f'command 1 {first} 6 0 0 0', f'command 1 {first} 2 -1 0 0',
                    f'command 1 {first} 5 0 0 0 9', f'answer 1 {first} 1']:
            answer = call(bad)
            assert answer['type'] == 'result' and not answer['ok'] and answer['revision'] == 1
        checks.append('wrong seat revision import id extra fields rejected')
        after = call(f'command 1 {first} 5 0 0 0')
        assert after['ok'] and after['revision'] == 2 and after['activeActor'] == second
        after = call(f'command 2 {second} 5 0 0 0')
        assert after['ok'] and after['revision'] == 3 and after['activeActor'] == first
        assert call('status')['day'] == ready['day'] + 1
        checks.append('two human turns advance exactly one original day')
        notices.clear()
        for handoff in range(13):
            owner=first if handoff%2==0 else second
            after=call(f"command {after['revision']} {owner} 5 0 0 0")
            assert after['type']=='result' and after['ok'],after
        assert call('status')['day']==ready['day']+7
        assert {notice['actor'] for notice in notices if notice['kind']=='normal'}=={first,second}
        assert len(turn_events)>=15
        checks.append('new week reaches both humans with original acknowledgement and turn lifecycle')

        send('quit')
        worker.wait(timeout=5)
        assert worker.returncode == 0
    finally:
        if worker.poll() is None:
            worker.kill()
            worker.wait()
result = {'passed': True, 'ruleFixture': probe_result, 'checks': checks}
(output / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
