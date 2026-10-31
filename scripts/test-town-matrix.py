"""Run nine-faction town rules, lifecycle, boundary and coastal fixtures.

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
output = ROOT / 'test-results/town-native'
output.mkdir(parents=True, exist_ok=True)
build, runtime = args.build.resolve(), args.runtime.resolve()
env = {**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy'}
objects = [str(p) for p in sorted((build / 'CMakeFiles/homm3-port.dir').rglob('*.o'))
           if p.name != 'main.cpp.o']
assert objects, 'Build the worker first'
media = ROOT / 'build/ffmpeg-native/install'
probe = output / 'actions-probe'
command = ['clang++', '-std=c++14', '-DHOMM3_PORT', '-DHOMM3_FULL_GAME', '-DHOMM3_SERVER',
           '-include', 'port/compat/prefix.h', '-Iport/compat', '-Iinclude',
           '-O2', '-Wno-writable-strings', '-Wno-deprecated-declarations',
           *shlex.split(subprocess.check_output(['sdl2-config', '--cflags', '--libs'], text=True)),
           '-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections',
           'port/probes/town_matrix.cpp', *objects, '-I' + str(media / 'include'),
           *[str(media / f'lib/lib{name}.a') for name in ['avformat', 'avcodec', 'swscale', 'swresample', 'avutil']],
           '-lz', '-o', str(probe)]
compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
(output / 'probe-build.log').write_text(compiled.stdout + compiled.stderr)
assert compiled.returncode == 0, 'Probe compile failed; see probe-build.log'
results=[]
for faction in range(9):
    run=subprocess.run([str(probe),str(runtime),str(faction)],env=env,capture_output=True,text=True,timeout=60)
    (output/f'{faction}.log').write_text(run.stdout+run.stderr)
    if run.returncode:results.append({'faction':faction,'passed':False,'error':run.stderr[-2000:]})
    else:results.append(json.loads(run.stdout.strip().splitlines()[-1]))
result={'passed':all(r['passed'] for r in results),'cases':results}
(output/'results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));raise SystemExit(0 if result['passed'] else 1)
