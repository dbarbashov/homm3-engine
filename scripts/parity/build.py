#!/usr/bin/env python3
"""Build private native+WASM harnesses without overwriting web/port/game.*."""
import argparse, os, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cmake',default=shutil.which('cmake'));p.add_argument('--jobs',type=int,default=5);args=p.parse_args()
if not args.cmake:
    candidates=list((Path.home()/'.cache/uv/archive-v0').glob('*/lib/python*/site-packages/cmake/data/bin/cmake'))
    if candidates:args.cmake=str(candidates[0])
if not args.cmake:raise SystemExit('Install CMake or supply --cmake /path/to/cmake')
toolchain=ROOT/'.local/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake'
if not toolchain.exists():raise SystemExit('Configure the repository Emscripten SDK first')
for target in ['native','web']:
    command=[args.cmake,'-S',str(ROOT),'-B',str(ROOT/f'build/cmake-parity-{target}'),'-DHOMM3_HARNESS=ON','-DCMAKE_BUILD_TYPE=Release']
    command+=['-DHOMM3_SERVER=ON','-DHOMM3_WORKER_SOURCE='+str(Path(os.environ.get('HOMM3_WORKER_SOURCE',ROOT.parent/'homm3-lobby/worker')).resolve())] if target=='native' else ['-DCMAKE_TOOLCHAIN_FILE='+str(toolchain)]
    subprocess.run(command,check=True,cwd=ROOT)
# Both configurations materialize portable-source. Finish them before building.
for target in ['native','web']:subprocess.run([args.cmake,'--build',str(ROOT/f'build/cmake-parity-{target}'),'-j',str(args.jobs)],check=True,cwd=ROOT)
