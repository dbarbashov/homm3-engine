"""Exercise the real native worker over bounded IPC and record memory, silently.

Requires a fresh CMake HOMM3_SERVER build and locally supplied original assets.
Never starts a browser, public listener, room or production process.
"""
import argparse
import contextlib
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
args = parser.parse_args()
runtime, build = args.runtime.resolve(), args.build.resolve()
binary = build / 'homm3-worker'
environment = {**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_RENDER_DRIVER': 'software', 'SDL_AUDIODRIVER': 'dummy'}
out = ROOT / 'test-results/server-worker'
out.mkdir(parents=True, exist_ok=True)
checks = []


def check(condition, name):
    if not condition:
        raise AssertionError(name)
    checks.append(name)


@contextlib.contextmanager
def worker(map_name='Good to Go.h3m'):
    process = subprocess.Popen([str(binary), str(runtime), 'maps/' + map_name, '42', '--prototype'],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True, env=environment)
    try:
        def read():
            if not select.select([process.stdout], [], [], 10)[0]:
                raise AssertionError('worker response timeout')
            line = process.stdout.readline()
            if not line:
                raise AssertionError(f'worker exited: {process.poll()} {process.stderr.read()}')
            return json.loads(line)

        def call(command):
            process.stdin.write(command + '\n')
            process.stdin.flush()
            return read()

        ready = read()
        check(ready['ready'] and ready['protocol'] == 1, 'protocol handshake: ' + map_name)
        yield call, ready
        process.stdin.write('quit\n')
        process.stdin.flush()
        process.wait(timeout=5)
        check(process.returncode == 0 and not process.stderr.read(), 'clean shutdown: ' + map_name)
    finally:
        if process.poll() is None:
            process.kill()
        process.wait()
        process.stdin.close()
        process.stdout.close()
        process.stderr.close()


# Compile the existing real-rule/PvP/racing-hire fixture against this exact build.
objects = [str(p) for p in sorted((build / 'CMakeFiles/homm3-port.dir').rglob('*.o'))
           if p.name not in {'main.cpp.o', 'fast_game.cpp.o'}]
if not objects:
    raise SystemExit('Build HOMM3_SERVER first')
video = ROOT / 'build/ffmpeg-native/install'
probe = out / 'rules-probe'
command = ['clang++', '-std=c++14', '-DHOMM3_PORT', '-DHOMM3_FULL_GAME', '-DHOMM3_SERVER',
           '-include', 'port/compat/prefix.h', '-Iport/compat', '-Iinclude',
           '-O2', '-Wno-writable-strings', '-Wno-deprecated-declarations',
           *shlex.split(subprocess.check_output(['sdl2-config', '--cflags', '--libs'], text=True)),
           '-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections',
           'port/probes/fast_game.cpp', *objects, '-I' + str(video / 'include'),
           *[str(video / f'lib/lib{name}.a') for name in ['avformat', 'avcodec', 'swscale', 'swresample', 'avutil']],
           '-lz', '-o', str(probe)]
compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
(out / 'rules-build.log').write_text(compiled.stdout + compiled.stderr)
check(compiled.returncode == 0, 'compile headless rules probe')
tested = subprocess.run([str(probe), str(runtime)], env=environment, capture_output=True, text=True, timeout=20)
(out / 'rules-run.log').write_text(tested.stdout + tested.stderr)
check(tested.returncode == 0, 'headless PvP and shared hire fixture')
rules = json.loads(tested.stdout.strip().splitlines()[-1])
check(rules['passed'] and rules['combat'] and rules['sharedHireRace'], 'original rule assertions')

with worker() as (call, ready):
    initial = call('view 0')
    other = call('view 1')
    check(initial['resources'] == [20, 10, 20, 10, 10, 10, 20000], 'LP64 starting resource values')
    hero = initial['heroes'][0]
    check(hero['id'] == 106, 'deterministic fixture seed on this platform')
    check(all(h['id'] != other['heroes'][0]['id'] for h in initial['heroes']), 'view omits other player heroes')
    for command in ['command 0 0 1 18 4 0', 'command 0 0 1 106 2 0', 'command 0 0 2 1 17 0',
                    'command 0 8 2 0 17 0', 'command 0 0 6 0 0 0',
                    'command 0 0 5 0 0 0', 'command 1 0 2 0 17 0',
                    'command 0 0 4 0 7 -1', 'command 0 0 4 0 7 1000001']:
        response = call(command)
        check(not response['ok'] and response['revision'] == 0, 'reject: ' + command)
    check(call('view 0') == initial and call('view 1') == other, 'rejections preserve visible world')
    check(call('command 0 0 1 106 4 0')['revision'] == 1, 'move executes on worker')
    moved = call('view 0')['heroes'][0]
    check(moved['y'] == hero['y'] + 1 and moved['movement'] == hero['movement'] - 100, 'original movement cost')
    check(call('command 1 0 2 0 17 0')['revision'] == 2, 'original building purchase')
    built = call('view 0')
    check(built['resources'] == [15, 10, 15, 10, 10, 10, 18000], 'original building price')
    check(built['towns'][0]['builtToday'] == 1, 'daily build limit recorded')
    check(not call('command 1 0 2 0 17 0')['ok'], 'stale retry cannot spend twice')
    check(not call('command 2 0 2 0 18 0')['ok'], 'second building blocked')
    check(call('command 2 0 4 0 7 1')['revision'] == 3, 'original recruitment')
    check(call('view 0')['towns'][0]['population'][7] == 14, 'recruitment consumes population')
    before = call('status')
    for _ in range(2000):
        assert not call('command 0 0 2 0 17 0')['ok']
    after = call('status')
    check(after['revision'] == 3, '2000 stale commands have no effects')
    check(after['rssBytes'] - before['rssBytes'] < 2 * 1024 * 1024, 'bounded RSS over repeated commands')
    check(after['lodCacheBytes'] <= 1024 * 1024, 'server archive cache <= 1 MiB')

maps = [(36, 1, 'Good to Go.h3m'), (36, 2, 'Jihad.h3m'),
        (72, 1, 'Ascension.h3m'), (72, 2, 'Rediscovery.h3m'),
        (108, 1, 'Island King Allied.h3m'), (108, 2, "The Battle of Daeyan's Ford.h3m"),
        (144, 1, 'Reclamation Allied.h3m'), (144, 2, 'War of the Mighty (Allies).h3m')]
measurements = []
for size, layers, name in maps:
    with worker(name) as (call, ready):
        check(ready['size'] == size and ready['layers'] == layers, 'real map dimensions: ' + name)
        check(ready['peakRssBytes'] < 64 * 1024 * 1024, 'bootstrap memory regression budget: ' + name)
        measurements.append({'map': name, **ready})

for frame in ['x' * 10000 + '\n', 'status', 'command 0 0 2 0 17 0', 'status\0junk\n']:
    result = subprocess.run([str(binary), str(runtime), 'maps/Good to Go.h3m', '42', '--prototype'],
                            input=frame, env=environment, capture_output=True, text=True, timeout=10)
    check(result.returncode == 1 and len(result.stdout.splitlines()) == 1, 'invalid framing is fatal before execution')
with worker() as (call, _):
    for frame in ['status extra', 'view 9999999999999999', 'command 0 0 2 0 17 0 extra']:
        check(not call(frame)['ok'], 'malformed command rejected')

summary = {'passed': True, 'platform': platform.platform(), 'machine': platform.machine(),
           'checks': checks, 'rules': rules, 'maps': measurements, 'commandsBefore': before,
           'commandsAfter': after, 'note': 'Bootstrap/command fixtures only; not long games, manual combat, AI turns or Linux capacity.'}
(out / 'results.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n')
print(json.dumps({'passed': True, 'checks': len(checks), 'rules': rules, 'maps': len(maps)}))
