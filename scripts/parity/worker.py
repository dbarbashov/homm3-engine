#!/usr/bin/env python3
"""Private supervisor adapter retaining worker stderr without changing IPC."""
import json, os, sys
from pathlib import Path
folder=Path(os.environ['HOMM3_HARNESS_OUTPUT'])
(folder/'worker-argv.json').write_text(json.dumps(sys.argv[1:]))
fd=os.open(folder/'worker.stderr',os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600)
os.dup2(fd,2);os.close(fd)
binary=os.environ['HOMM3_HARNESS_ENGINE_BINARY']
os.execv(binary,[binary,*sys.argv[1:]])
