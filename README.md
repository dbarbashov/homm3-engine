# Heroes III shared engine

A portable C++ runtime for Heroes III, with native SDL and browser WebAssembly
targets. It includes the recovered game rules, original interface and rendering,
portability code, media adapters and a deterministic test harness.

The browser shell lives in [homm3-web](https://github.com/dbarbashov/homm3-web).
Original game files are supplied by the player and are not included here.

## Build

Requirements: Python 3, CMake 3.20+, Clang, SDL2 and zlib. For the browser target,
install the pinned Emscripten SDK:

```sh
python3 scripts/setup-port.py
```

Build the native game client:

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native -j5
```

For the browser game, clone `homm3-web` alongside this repository, check out the
engine revision specified in its `engine.lock.json`, and run `npm run build`
from the browser repository. Its README includes the complete commands.
The build exports `game.js`, `game.wasm` and an `engine-build.json` manifest.

## Tests

```sh
npm test
python3 scripts/setup-integration.py --web ../homm3-web
```

The first command runs standalone C++ input probes without game resources.
The second connects a browser checkout to the cross-runtime tests using ignored
local links. Game and browser scenarios require your own original resources.

The [parity harness](scripts/parity/README.md) compares recorded states, animation,
audio and timing. Full network parity checks require compatible service and worker
binaries; their paths are supplied explicitly. Maintainers run those checks for
contributions. Use isolated test services and silent audio.

## Contributing

Gameplay, rendering, animation and portability fixes are welcome as pull requests
here. Website and JavaScript changes belong in `homm3-web`.

To test an engine change in the browser, commit it on your local branch and update
the browser checkout's `engine.lock.json` to that commit. Source builds reject a
mismatched or dirty engine checkout. Maintained releases use a shared tested engine
revision for all targets.

Do not commit original game resources, saves, room credentials or generated builds.
For bugs, include the scenario and reproduction steps; the in-game report button
can attach a read-only reproduction link.

## Provenance and licenses

The recovered source origin is recorded in `upstream-origin.json`. The original
import is a separate Git commit, followed by our adaptations. Edit the tracked
`src/` and `include/` files directly; builds do not download or patch the upstream
source. Helpers previously extracted during configuration are ordinary tracked
files too. See [source maintenance](docs/source-maintenance.md) for updating the base. See `LICENSE`, `THIRD_PARTY.md` and `licenses/` for notices and dependencies.
FFmpeg source and relinking materials must accompany applicable binary releases.
