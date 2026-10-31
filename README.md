# Heroes III shared engine

The portable C++ runtime used by the browser and native authoritative server.
This repository and [homm3-web](https://github.com/dbarbashov/homm3-web) are public.
The Rust server and native authority implementations in `homm3-lobby` remain private.

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
Worker executables, saves, reports and retail assets are never
copied into the public web directory. Browser artifacts contain client code and
are inspectable by design; the server validates gameplay commands independently.
Native workers use exactly the same pinned rules and patches as the WASM client.

## Integration tests

The client and its source-level tests can be built without server access.
Run `npm test` for the standalone C++ input probes. The full legacy/authority
comparison also needs the private server checkout; maintainers run that portion
for public engine contributions. Supply explicit checkouts; the helper creates
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

## Contributing

Gameplay, rendering, animation and portability fixes are welcome as pull requests
here. Website and JavaScript shell changes belong in `homm3-web`. Retail game data
is supplied locally by each developer and must never be committed.

For a matching browser build, clone both public repositories, check out the engine
commit named in the client's `engine.lock.json`, install the SDK here, then run
`npm run build` from the client. The client README contains the complete commands.
To test engine changes, commit them on your local branch and update the client's
lock to that commit. Maintainers update the server pin and run private authority
checks before releasing a shared revision. Native client builds use CMake with
`HOMM3_SERVER=OFF` (the default) and do not require the private worker.

## Provenance and publication

Imported from the committed integration snapshot, preserving its license notices.
The upstream pin and patches remain reviewable. See `LICENSE`, `THIRD_PARTY.md`
and `licenses/`. The public main history contains the extracted shared engine.
The mixed pre-split history, including server implementations, is retained in
private `homm3-engine-history`; it must not be pushed to this repository.

FFmpeg remains LGPL. Its source and the application/linking sources are available;
binary distributions must continue to include the applicable source and relinking
materials. Opening this repository does not publish a new binary release or
include any original retail game resources.
