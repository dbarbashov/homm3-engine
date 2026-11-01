#!/usr/bin/env python3
"""Serializable gameplay randomness, independent of UI and deserialization."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='homm3-rng-') as tmp:
    binary=Path(tmp)/'rng'
    subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-I'+str(root/'port/compat'),'-I'+str(root/'include'),str(root/'port/probes/game_rng.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
