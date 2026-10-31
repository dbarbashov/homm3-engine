#!/usr/bin/env python3
"""Build the browser engine and export only the public runtime artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--cmake', default=os.environ.get('CMAKE', 'cmake'))
p.add_argument('--jobs', type=int, default=5)
a = p.parse_args()
output = a.output.resolve()
build = ROOT / 'build/client-cmake'
toolchain = ROOT / '.local/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake'
if not toolchain.is_file():
    p.error('Run python3 scripts/setup-port.py first')
subprocess.run([a.cmake, '-S', str(ROOT), '-B', str(build),
                '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_TOOLCHAIN_FILE=' + str(toolchain),
                '-DHOMM3_WEB_OUTPUT=' + str(build / 'artifacts')], check=True)
subprocess.run([a.cmake, '--build', str(build), '-j', str(a.jobs)], check=True)
output.mkdir(parents=True, exist_ok=True)
hashes = {}
for name in ['game.js', 'game.wasm']:
    source = build / 'artifacts' / name
    hashes[name] = hashlib.sha256(source.read_bytes()).hexdigest()
    shutil.copy2(source, output / name)
revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
(output / 'engine-build.json').write_text(json.dumps({
    'schema': 1, 'engineRevision': revision, 'artifacts': hashes,
}, indent=2) + '\n')
