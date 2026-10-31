"""Run filtered thieves-guild tiers, ranking ties and nested tavern rules.

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
output = ROOT / 'test-results/authority-guild'
output.mkdir(parents=True, exist_ok=True)
build, runtime = args.build.resolve(), args.runtime.resolve()
env = {**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy'}
objects = [str(p) for p in sorted((build / 'CMakeFiles/homm3-port.dir').rglob('*.o'))
           if p.name != 'main.cpp.o']
assert objects, 'Build the worker first'
media = ROOT / 'build/ffmpeg-native/install'
probe = output / 'guild-probe'
command = ['clang++', '-std=c++14', '-DHOMM3_PORT', '-DHOMM3_FULL_GAME', '-DHOMM3_SERVER',
           '-include', 'port/compat/prefix.h', '-Iport/compat', '-Iinclude',
           '-O2', '-Wno-writable-strings', '-Wno-deprecated-declarations',
           *shlex.split(subprocess.check_output(['sdl2-config', '--cflags', '--libs'], text=True)),
           '-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections',
           'port/probes/authority_guild.cpp', *objects, '-I' + str(media / 'include'),
           *[str(media / f'lib/lib{name}.a') for name in ['avformat', 'avcodec', 'swscale', 'swresample', 'avutil']],
           '-lz', '-o', str(probe)]
compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
(output / 'probe-build.log').write_text(compiled.stdout + compiled.stderr)
assert compiled.returncode == 0, 'Probe compile failed; see probe-build.log'
run = subprocess.run([str(probe), str(runtime), args.map], env=env, capture_output=True, text=True, timeout=120)
(output / 'probe.log').write_text(run.stdout + run.stderr)
assert run.returncode == 0, 'Original-rule fixture failed; see probe.log'
probe_result = json.loads(run.stdout.strip().splitlines()[-1])
(output / "results.json").write_text(json.dumps(probe_result, indent=2)+"\n")
print(json.dumps(probe_result))
