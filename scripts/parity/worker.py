#!/usr/bin/env python3
"""Private supervisor adapter retaining worker stderr without changing IPC."""
import json, os, sys
from pathlib import Path
folder=Path(os.environ['HOMM3_HARNESS_OUTPUT'])
arguments=sys.argv[1:]
# GM2 restores the world and RNG, but native pre-load map/AI initialization also
# depends on the constructor seed. A random lobby seed is not a fixture input.
seed=int(os.environ['HOMM3_HARNESS_SEED'])
if not 0<=seed<=2147483647 or len(arguments)<3:raise ValueError('Invalid fixture seed/worker arguments')
arguments[2]=str(seed)
(folder/'worker-argv.json').write_text(json.dumps(arguments))
fd=os.open(folder/'worker.stderr',os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600)
os.dup2(fd,2);os.close(fd)
binary=os.environ['HOMM3_HARNESS_ENGINE_BINARY']
os.execv(binary,[binary,*arguments])
