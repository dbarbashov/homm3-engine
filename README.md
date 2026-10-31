# Heroes III shared engine (private)

The portable C++ runtime used by the browser and native authoritative server.
Only `homm3-web` is public. This repository and `homm3-lobby` are private.

## Ownership

- This repository: pinned recovered rules, portability patches, original UI/rendering,
  SDL/media adapters, shared integration interfaces, native/WASM parity harness.
- `homm3-web`: public website, JavaScript browser/network presentation bridges,
  asset loading, browser audio, bug reports and the public issue tracker.
- `homm3-lobby`: Rust rooms/history/supervision and `worker/` C++ authority adapters.
  Server implementations are not copied into this repository or the public client.

The `server_*.h` headers and conditional patches here are shared integration
contracts required to compile both targets; the worker implementations live in
`homm3-lobby/worker`. The recovered engine includes rendering as well as rules:
this is a repository boundary, not a rewrite of the original game's architecture.
Existing gameplay paths (solo, legacy, simultaneous, authoritative) are preserved.

## Build

Use Python 3, CMake 3.20+, Clang, SDL2 and zlib. Install the pinned browser SDK:

```sh
python3 scripts/setup-port.py
```

Both consumer repositories pin this repository's full commit in `engine.lock.json`.
Their build entry points reject a different revision or modified tracked engine
files. Update both locks together after validating a new engine revision.

```sh
# From homm3-web:
HOMM3_ENGINE_ROOT=/path/to/homm3-engine npm run build
# From homm3-lobby:
python3 scripts/build-worker.py --engine /path/to/homm3-engine
```

The browser build exports only `game.js`, `game.wasm`, and `engine-build.json`.
Private C++ source, worker executables, saves, reports and retail assets are never
copied into the public web directory. Browser artifacts contain client code and
are inspectable by design; private source visibility is not an anti-cheat boundary.
Native workers use exactly the same pinned rules and patches as the WASM client.

## Integration tests

Cross-runtime tests belong here. Supply explicit checkouts; the helper creates
ignored links to the public client and never changes its tracked files:

```sh
python3 scripts/setup-integration.py --web /path/to/homm3-web
export HOMM3_WORKER_SOURCE=/path/to/homm3-lobby/worker
python3 scripts/parity/build.py
python3 scripts/test-authority-session.py --runtime /private/runtime --build /path/to/homm3-lobby/build/worker
python3 scripts/test-authority-combat.py --runtime /private/runtime --build /path/to/homm3-lobby/build/worker
python3 scripts/test-projection-determinism.py --worker /path/to/homm3-lobby/build/worker/homm3-worker --runtime /private/runtime
python3 scripts/test-match-history.py --worker /path/to/homm3-lobby/build/worker/homm3-worker --relay /path/to/homm3-lobby/target/release/homm3-lobby --runtime /private/runtime
npm test
```

The full parity runner accepts `--relay`, `--worker`, `--web` (harness WASM),
`--runtime` and `--fixture`. Test game resources remain outside Git. Use isolated
services and dummy/offline audio; do not restart an existing play session.

## Provenance and publication

Imported from the committed `homm3-web` integration snapshot `dda2854`, preserving
its license notices. The upstream pin and patches remain reviewable. See
`THIRD_PARTY.md` and `licenses/`. Private visibility does not change upstream licenses.
The archive before this split remains in local Git history; existing public Git
history has not been rewritten. Publish only reviewed public client files, never
an engine branch or an unrestricted export of this repository.

FFmpeg remains LGPL. Before distributing new binary bundles from private source,
provide the required FFmpeg source and relinking materials (application objects,
link instructions and dependencies); repository privacy does not waive those
obligations. This migration itself does not publish a new binary release.
