#!/usr/bin/env python3
"""Connect an explicit public client checkout to private cross-runtime tests."""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--web', type=Path, required=True)
a = p.parse_args()
source = a.web.resolve()
for name in ['web', 'lib', 'server.mjs']:
    target = ROOT / name
    expected = source / name
    if not expected.exists():
        p.error('Missing public client path: ' + str(expected))
    if target.is_symlink() and target.resolve() == expected:
        continue
    if target.exists() or target.is_symlink():
        p.error('Refusing to replace existing path: ' + str(target))
    target.symlink_to(expected, target_is_directory=expected.is_dir())
print('Integration tests use public client:', source)
