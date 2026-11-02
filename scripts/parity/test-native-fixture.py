#!/usr/bin/env python3
"""Verify full native fixture continuation is independent of lobby bootstrap RNG.

Requires an instrumented worker supplied explicitly; never uses a live service.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--worker', type=Path, required=True)
p.add_argument('--runtime', type=Path, required=True)
p.add_argument('--fixture', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
a.worker, a.runtime, a.fixture, a.out = [x.resolve() for x in [a.worker,a.runtime,a.fixture,a.out]]
manifest = json.loads((a.fixture/'manifest.json').read_text())
a.out.mkdir(parents=True, exist_ok=True)
noise = {'rssBytes','peakRssBytes','privateBytes','pssBytes','lodCacheBytes'}

def run(seed):
    folder = a.out/str(seed)
    folder.mkdir(exist_ok=False)
    env = {k:v for k,v in os.environ.items() if not k.startswith(('HOMM3_HARNESS_','HOMM3_CHECKPOINT_','HOMM3_AUTHORITY_'))}
    env.update(SDL_AUDIODRIVER='dummy', SDL_VIDEODRIVER='dummy',
               HOMM3_HARNESS_SAVE=str(a.fixture/'start.gm2'),
               HOMM3_HARNESS_RNG=str(manifest['rng']),
               HOMM3_HARNESS_RULES_RNG=manifest.get('rngAlgorithm','msvc-lcg32'),
               HOMM3_HARNESS_SEED=str(manifest['seed']),
               HOMM3_HARNESS_TRACE_RNG='1', HOMM3_HARNESS_OUTPUT=str(folder),
               HOMM3_HARNESS_COMBAT_RNG=manifest.get('combatRng',''),
               HOMM3_HARNESS_ENGINE_BINARY=str(a.worker))
    with (folder/'stderr.log').open('w') as err:
        # Exercise genuinely different constructor seeds. The network adapter
        # pins its bootstrap seed, which would hide omitted save fields here.
        process = subprocess.Popen([str(a.worker), str(a.runtime),
            str(a.runtime/manifest['map']), str(seed),'--humans','2','--setup',
            ','.join(map(str,manifest['setup']))], env=env, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=err, text=True)
        try:
            def read():
                line=process.stdout.readline()
                if not line:raise RuntimeError('Worker ended before the second week')
                return {k:v for k,v in json.loads(line).items() if k not in noise}
            def send(line):
                process.stdin.write(line+'\n');process.stdin.flush()
            ready=read();assert ready['type']=='ready',ready
            revision=0;actor=ready['activeActor'];frames=[ready]
            for n in range(15):
                send(f'command {revision} {actor} {0 if n==0 else 5} 0 0 0')
                for _ in range(1000):
                    frame=read();frames.append(frame)
                    if frame['type']=='choice':send(f'answer {frame["token"]} {frame["actor"]} {frame["options"][0]}')
                    if frame['type']=='result':
                        assert frame['ok'],frame
                        revision=frame['revision'];actor=frame['activeActor'];break
                else:raise RuntimeError('Unbounded command output')
            (folder/'frames.json').write_text(json.dumps(frames))
            initial=json.loads((folder/'fixture-loaded.json').read_text())
            decisions=(folder/'decisions.jsonl').read_text()
            return {'initial':initial,'frames':frames,'decisions':decisions}
        finally:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()

runs=[run(seed) for seed in [190657709,2139359369,481516]]
assert all(run==runs[0] for run in runs[1:]),'Fixture continuation depends on lobby bootstrap seed; inspect frames/decisions'
result={'passed':True,'bootstrapSeeds':3,'handoffs':14,'rngDecisions':len(runs[0]['decisions'].splitlines()),
        'fixtureSha256':hashlib.sha256((a.fixture/'start.gm2').read_bytes()).hexdigest(),
        'framesSha256':hashlib.sha256(json.dumps(runs[0]['frames'],sort_keys=True).encode()).hexdigest()}
(a.out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
