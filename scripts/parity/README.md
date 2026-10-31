# Network presentation parity harness

Private, silent, accelerated two-browser testing of the legacy network game
against the native authoritative worker. Both load the **same original GM2
file**, with a separate RNG continuation. No screenshots, image recognition or
changes to normal production clocks are required.

## Run

Prerequisites: the repository's original game assets (`.local/game.json` and
`.local/runtime`), Emscripten, native SDL/FFmpeg build dependencies, Python with
Playwright browsers, Node, and a compatible room-service debug executable.
Paths can be overridden with `run.py --help` / `fixture.py --help`.

```sh
python3 scripts/parity/build.py
python3 scripts/parity/fixture.py --combat --out test-results/parity/fixture-combat
python3 scripts/parity/run.py --fixture test-results/parity/fixture-combat \
  --scenario scripts/parity/scenarios/combat.json --out test-results/parity/combat
python3 scripts/parity/run.py --fixture test-results/parity/fixture-combat \
  --suite turns --virtual-ms 512 --out test-results/parity/turns
```

For the automated acceptance loop (paired combat, repeated reference, injected
faults), run:

```sh
python3 scripts/parity/matrix.py --quick --fixture test-results/parity/fixture-combat \
  --out test-results/parity/matrix
```

Omit `--quick` to add adventure/week scenarios and repeat **each** reference.
`summary.json` separates `harnessPassed` from `parityPassed`, and lists unstable
channels if A/A fails. Such a run is blocked evidence, not an automatic gameplay
bug. Choose a new output directory for each matrix run.

Open the output directory's `index.html`. Exit 0 means all required gates and
comparisons passed; exit 1 means a divergence, assertion or incomplete run.
Existing authority presentation differences intentionally produce a failing
comparison. A test runner completing is not a claim of feature parity.

`--suite adventure` checks waiting-seat town/hero selection, movement, entering
and leaving a town, and garrison/visitor inputs. `--suite turns` validates both
seats' dates and active player through fourteen handoffs into week two; modal
acknowledgements are conditional and recorded, not silently auto-answered by
the engine. The combat replay asserts encounter entry, wait, defend, opening
the spellbook, selecting Haste by its original spell ID, targeting a stack,
spell effect, auto battle and returning from results. Auto battle is one tested
user action, not an override of the other battle actions.

## Validate the harness itself

Always compare a repeated reference before attributing a new mismatch to the
candidate. Reuse the exact fixture directory and builds, not a regenerated
save of the same map.

```sh
python3 scripts/parity/run.py --modes legacy --fixture test-results/parity/fixture-combat \
  --scenario scripts/parity/scenarios/combat.json --out test-results/parity/repeat
python3 scripts/parity/compare.py test-results/parity/combat/legacy/trace.json \
  test-results/parity/repeat/legacy/trace.json --out test-results/parity/repeatability
python3 scripts/parity/seeded_faults.py test-results/parity/repeat/legacy/trace.json \
  --out test-results/parity/seeded-faults.json
python3 -m unittest discover -s scripts/parity -p 'test_*.py'
node scripts/parity/test-audio.mjs
python3 scripts/parity/test-browser-audio.py
```

The fault validator changes a real captured trace: missing hero, dropped
animation frame, 250 ms delay, missing sound, changed PCM at identical envelope,
and wrong RNG. All must be rejected by the appropriate detector.

## What is measured

* Initial gates: SHA256 of save, canonical decoded map/players/heroes/towns,
  RNG, browser version, viewport/settings, WASM and observer fingerprints.
  Native export/reload and both browsers must decode the fixture identically.
* Each action: owned world, actual top-window widgets/text/flags, adventure
  selection and hero presence, exact combat stacks/animation frames/effects.
* Every composed RGB565 frame: all pixels hashed in 32×32 tiles, including
  cursor and overlays. No GPU readback or screenshot. A retained unchanged
  display is explicitly marked `held`, never counted as a newly rendered frame.
* Audio: unchanged production music/SFX selection and fade policy feeding a
  silent sample-clock mixer. Semantic requests, 16-bit quantized PCM hashes,
  RMS and peak are compared. The mixer is independently checked against
  OfflineAudioContext in Chromium and WebKit.
* Timelines: changed-frame sequence and timing, semantic UI sequence, input to
  first changed frame, simulated time, measured wall time, startup separately.
* Full replay JSON, raw per-seat action traces, console, bounded wire log and
  worker stderr remain next to the report. Trace overflow and missing evidence
  fail; failures are not converted into success by shortening the scenario.

The canonical world observer covers map cells/fog, date/resources, owned heroes
and their armies/items/spells/skills, towns/buildings/garrisons/population/guild.
It is not a byte dump of every internal C++ field or a full engine save comparer.

## Two clocks

Default `--clock virtual` advances the original game timer at pump/poll points.
Animations, CPU composition, sound samples, scheduled fades and game decisions
still execute; real sleeps and GPU upload are omitted. Between replay inputs
the engine continuation is suspended. Asset fetch/decode must settle before
virtual time resumes, so disk/network loading cannot randomly shift music.
The initial fixture freezes time before first-turn presentation starts.

The silent `--clock real` lane retains normal timers and GPU presentation:

```sh
python3 scripts/parity/run.py --clock real --modes legacy --suite idle \
  --virtual-ms 2000 --fixture test-results/parity/fixture-combat \
  --out test-results/parity/realtime
```

Use it for actual responsiveness and transport latency. Virtual timing measures
gameplay pacing, not real-world network performance. Real audio uses the actual
WebAudio sink (silent); virtual PCM records are unavailable in that lane.

## Scope and isolation

`HOMM3_HARNESS` defaults OFF. Private native/WASM outputs live under
`build/cmake-parity-*`; the live preview's worker and `web/port/game.*` stay
untouched. The runner starts its own loopback relay and site on random ports,
uses isolated browser contexts and cleans up its processes. Tests are inaudible.
Do not run `prepare-port.py` or configure a build concurrently with compilation:
all configurations share the generated portable source directory.

Fixtures currently enter at **before-first-turn**, generated once and then
saved/reloaded. Arbitrary mid-battle or mid-turn user-save import is not
supported. The browser scenario starts with two humans on Arrogance; the
combat fixture pins Conflux/Brissa and Haste. A new map requires its own replay
and setup. Original assets and generated saves stay outside Git.

Network transport remains real WebSockets/Rust IPC. Long transfer/AI episodes
can retain scheduling variation; run A/A on a new scenario before using its
strict frame/timing diff as a regression gate. A timing failure alone is not
proof of a gameplay bug. No claim of complete feature coverage, mobile parity,
all spells/campaigns or Internet reconnect is made by these scenarios.

The native LP64 save-width fixes are independent of harness instrumentation:
Win32 save fields must stay 32-bit on the native server. They preserve WASM
widths and are exercised by cross-runtime round-trip plus ordinary-session and
combat regression probes.

Focused regression gates accompany the strict combat comparison in
`combat/regressions.json`: pursuit dialog/buttons, encounter hero position,
ambient request sequence, waiting selection, successful Haste sample/duration,
and the static adventure hero beneath battle. Passing these gates does not
waive frame, PCM, onset-time or combat-state differences. The separate
`scenarios/decline-pursuit.json` replay exercises refusing the encounter.

Native fixture creation validates hero roster sentinels, and canonical initial
checkpoints include all eight rosters. Regenerate older fixtures once after
this observer change, then share the resulting exact save/RNG between lanes.
The runner stops at the first missing-hero invariant and retains both clients'
checkpoint evidence instead of continuing the rest of the week.

## Town matrix

```sh
python3 scripts/test-town-matrix.py --runtime /path/to/runtime --build build/cmake-server
python3 scripts/test-town-startup.py --runtime /path/to/runtime --build build/cmake-server
python3 scripts/parity/towns.py --out test-results/towns --jobs 2
python3 scripts/parity/towns.py --factions 0 --browser webkit --out test-results/towns-webkit
python3 scripts/parity/towns.py --town-profile full --factions 0 2 3 6 7 \
  --scenarios rewards --out test-results/town-rewards
python3 scripts/parity/towns.py --town-profile full --factions 5 \
  --scenarios portal --out test-results/town-portal
node scripts/parity/test-clock.mjs
```

`towns.py` creates nine rich original-format GM2 fixtures (one per faction),
then runs `core`, `residents` and `services` with two real network clients in
both modes. Assets/saves are ignored by Git. Each actor receives one million
of every resource, fixed armies, 50 units per dwelling slot, and basic services.
This is an intentional cheat fixture, not a natural initial map. Mobility is
recomputed after changing armies, before saving, so legacy's cached maximum
and the host's first-turn recalculation describe the same army.
`fixture.py --town N --town-profile full` additionally builds reachable tiers.

Actions use original building hit regions from the town z-buffer, original
hall sprite building IDs (`build_choice`), and actual widget coordinates.
Empty army slots remain valid targets even when their icon is not drawn.
Modifier keys remain held until the simulated input has been processed.
Construction and hiring budgets include the original long sample/fizzle.

`town-checks.json` compares all owned state at action boundaries, town army
widgets (including pressed/selected status), service dialogs, and explicit
postconditions. `functionalPassed` is separate from strict frame/audio parity.
One explicitly recorded transient remains: the host grants guild spells when
buying the spellbook; legacy does so after closing the guild. The entire guild
window is compared, then full spell/army/resources state at `guild-close`.
This exception does not suppress the strict comparator's failure.

Native tests cover all nine factions, reachable construction tiers, all 126
base/upgraded recruitment variants, exact prices, invalid requests, full
armies, splitting/merging/swapping, last-stack protection, garrison transitions,
second hero recruitment, eight-hero limit, dismissal/upgrading, resource trade,
Grail consumption, all four supported shipyard factions, an Inferno gate round
trip, and the original new-week reset/income/population rules. Special service
rules are also covered by `test-authority-actions.py`.

The virtual runner pauses simulation while a real authority request awaits
its reply, decision, or transfer to another player. The engine still pumps;
transport wall time remains in `transport-send`/`transport-ready` events and
in action wall time. Real-clock runs retain real pacing. This prevents faster
simulation from sending the next click into an unfinished network request.
It does not erase latency from measurements or relax frame/PCM comparison.
Live adventure diagnostics are sampled even while a town modal is open.

Additional full-city replays live in `scenarios/towns/{transformer,university,portal}.json`.
Generate `fixture.py --town 4|8|5 --town-profile full` respectively, then pass
that exact fixture and scenario to `run.py`. Transformer checks include the
working lists and final two skeleton stacks; university checks include the
town artwork, learned skill and 2000-gold tuition. The Dungeon replay explicitly
acknowledges Mana Vortex, Battle Scholar Academy and level-up before opening
an empty Portal of Summoning, then reenters and verifies that mana, XP and the
chosen skill persist without a second reward. `rewards.json` covers the other
full-city entry bonuses. Op41 executes original garrison-then-visitor rewards
on the canonical host after the town window is ready for original dialogs;
legacy retains its original local execution. Positive portal stock is exercised natively.
Town Gate and coastal construction/purchase are currently native fixtures,
not browser navigation scenarios. Campaign, siege and every 3–8-player/mobile
combination are outside this town matrix.


## Adventure audit

```sh
python3 scripts/parity/adventure.py --out test-results/adventure --jobs 2
python3 scripts/parity/adventure.py --cases resource mine visions --browser webkit --out test-results/adventure-webkit
python3 scripts/test-adventure-matrix.py --runtime ../runtime --build build/cmake-parity-release-control
```

The first command covers 18 object/reward flows, 11 spell flows and a complete
week (14 human handoffs). Each pair starts from one original GM2 and exact RNG;
fixture export/reload also checks hero coordinates and roster validity. Object
fixtures use real objects in Arrogance, 100,000 of every resource and two hero
stacks. Nearby neutral guards are cleared and counted in the manifest; this
suite does not claim guarded-object or manual-combat coverage. Magic fixtures
have an original spellbook, 500 mana and expert schools.

`adventure-checks.json` checks both seats' owned world, calendar, original modal
contents, gains/costs, repeated visits, pickup removal, spell status, mine guard
placement/persistence/retrieval, and real portal movement. Native probes cover
additional wells, stables, temple and paid/declined/unaffordable trees. Coastal
boat success paths and traversal while flying/water-walking are not covered by
the browser matrix (embark/disembark have separate native probes).

Action snapshots additionally expose visit flags, luck/morale, adventure spell
flags and the original bottom message panel. In action cell records only the
object-type column [7] is resolved beneath an obscuring hero; other columns
retain raw values and must not be mistaken for the underlying object payload.
The initial canonical world remains completely raw.

The week scenario currently FAILS intermediate actor/dialog/world comparisons,
even when the settled date and both owned worlds agree after every handoff.
Keep this failure visible. `functionalPassed` does not waive strict pixel,
audio or transition differences; every original `comparison.json` is retained.
The current reference is the pinned reconstructed legacy engine, not a retail
Windows executable. Fixture/browsers/asset startup is measured separately from
accelerated execution and should not be hidden in performance claims.

## Obelisks and event-aligned turn diagnostics

```sh
python3 scripts/parity/adventure.py --cases obelisk --out test-results/obelisk
python3 scripts/parity/fixture.py --combat --out test-results/week-fixture
python3 scripts/parity/run.py --fixture test-results/week-fixture --scenario scripts/parity/scenarios/adventure/week-events.json --out test-results/week-events
python3 scripts/parity/transition_checks.py test-results/week-events --suite week
python3 scripts/parity/transition_checks.py test-results/combat --suite combat
```

Obelisk fixtures use a real object on **All for One**, since Arrogance contains
none. `--natural-setup` preserves the selected map's original slots/factions;
`fixture-setup.json` stores the 41 original setup fields alongside the GM2 and
is restored in both legacy bootstrap and authoritative worker startup. Normal
Arrogance fixtures retain their existing preset. The scenario covers first
visit, puzzle close, movement, repeated visit and reopening with P. Native
puzzle probes separately cover all nine masks, hidden/partial/full reveal,
world-view nesting and private per-viewer visitation flags.

A scenario action may specify `until` (UI paths and exact values) and optionally
`until_seat`. Its `ms` is then a virtual-time ceiling, polled every 128 virtual
milliseconds. `window.text` joins all nonempty top-window widget texts with
newlines. Traces retain `budgetMs`, actual `virtualMs` and `eventReached`; raw
pixel/audio/timing comparisons are never retimed. The week scenario aligns on
the original turn popup and acknowledges map-specific daily notices. Focused
checks compare popup widgets, settled per-seat worlds and waiting selection;
calendar commit order and the lifetime of original new-day/week cards can
still differ during the transition. Keep those strict failures visible.

The combat checker catches idle-animation regressions after wait/defend/spells
and verifies the final armies/rewards and Haste. It does **not** waive different
AI positions. Harness-only `decision` events and worker `decisions.jsonl` record
the RNG immediately before original `doCompAI`, without drawing random values.
The original 27-draw offset was traced to omitted entry-cue/music choices and
stack cycle initialization in headless combat. Ordered original consumers and
frame timing metadata now run on the server without loading audio or sprites.
No fixed draw-count compensation is used; see the RNG checks below.

Regenerate older fixtures before long turn comparisons: equal GM2/RNG alone
did not restore the original scenario setup. The old fixture intermittently
produced 12 versus 17 recruits in week two; the same GM2/RNG plus its exported
setup passes the paired week check and a repeated authority run.


## Combat RNG and opening battlefield

```sh
python3 scripts/test-server-rng.py
python3 scripts/parity/fixture.py --combat --combat-rng browser-libc --out test-results/combat-production-fixture
python3 scripts/parity/run.py --fixture test-results/combat-production-fixture --scenario scripts/parity/scenarios/combat.json --out test-results/combat-production
python3 scripts/parity/combat_checks.py test-results/combat-production
```

The normal native worker uses the pinned Emscripten musl RNG, matching the
existing WASM client instead of the host's platform-dependent libc. Browser
legacy builds are unchanged. `test-server-rng.py` compiles a native production
prefix probe and an actual Emscripten libc probe (neither uses the harness),
then compares the first eight outputs and a digest of 100,000 draws in nine
sequences. Coverage includes unseeded state, seed zero's unsigned wrap, large
seeds and reseeding across translation units.

Default harness fixtures still use MSVC-LCG32. `--combat-rng browser-libc`
keeps initial save generation/round-trip unchanged, then switches both lanes
to the production algorithm at original combat setup, immediately before the
original coordinate-based reseed. It is a battle comparison, not a claim that
fixture creation or the whole adventure ran with production RNG. States wider
than 32 bits are recorded as decimal strings to avoid JavaScript precision loss.

RNG trace events retain every draw's state, range and algorithm, plus reseeds
and AI decision boundaries. `combat_checks.py` compares all draws, all AI
boundaries, battle positions/counts/effects/mana, results widgets and owned
worlds outside combat. It also compares the actual obstacle list on every
composed opening combat frame: server opening state must be installed before
`addWindow` can paint, not after the intro. Input remains unready until the
original intro completes. The pre-fix trace fails both RNG and opening checks.

An optional manifest field `harnessFaults.authorityCombatSeed` deliberately
changes only the authority renderer's local RNG at combat open. The server and
legacy reference remain untouched. This tests canonical first-frame projection
even when local obstacle generation is wrong. Extend `encounter-confirm.ms`
to 30000 to additionally test a long idle before the first battle action.

The focused gameplay comparison excludes idle pose frame/sequence and local
control flags. It retains `intermediateWorldDifferences`: in the tested archer
battle the original adventure hero army commits a casualty while results are
open, whereas authority commits it on dismissal. Actual results widgets and
final armies agree. Raw frame/PCM/timing comparisons remain strict and still
fail; focused RNG success is not full audiovisual parity. Current paired
coverage is neutral combat with two armies, wait/defend/Haste/auto/results,
Chromium/WebKit, and a deliberately wrong renderer RNG. It does not establish
all-spell, siege, PvP or retail Windows executable parity.

## Adventure presentation regressions

`presentation_checks.py` separates the reported regressions from the still
strict whole-trace comparison. It observes visible route arrows and destination,
walk sequence/frame/scroll, selected speed, pointer visibility, object one-shot
samples, and HD tiles outside the combat overlay. A passing focused gate does
not waive raw audiovisual differences (including other combat events such as
morale). The default remains classic; use `--viewport 1360 900` for HD.

```sh
python3 scripts/parity/fixture.py --combat --movement --out test-results/presentation/fixture
python3 scripts/parity/run.py --fixture test-results/presentation/fixture \
  --scenario scripts/parity/scenarios/presentation/movement.json \
  --out test-results/presentation/movement
python3 scripts/parity/presentation_checks.py test-results/presentation/movement --suite movement
```

The corridor fixture clears/discloses a flat area once before writing GM2 and
rebuilds original adjacent-monster flags. Both clients and the worker then load
those exact save bytes. The scenario spends movement, walks a longer route,
exhausts today's green section, advances both players, checks automatic green
recoloring, and resumes with M. No route click occurs after the new day.

For the six-cell `presentation/speed.json` replay, copy the fixture directory
and set `manifest.json.settings.heroSpeed` to 0, 2 or 4; the GM2 must remain
unchanged. Run `--suite speed` on each output. For `town-walk.json` and
`guarded-route.json`, use a standard `fixture.py --combat` fixture without the
corridor and check `--suite town` / `--suite guard`. Existing `adventure.py`
resource, knowledge, windmill and mine runs can also be checked with
`--suite sound`. Run the usual combat scenario in HD with both `combat_checks.py`
and `presentation_checks.py --suite combat`.

`presentation_faults.py --movement DIR --town DIR --sound DIR --speed DIR
--combat DIR --out FILE` requires passing actual paired traces (speed 2, HD
combat) and deliberately corrupts seven observations. All seven must fail:
brown next-day route, erased destination, hidden town cursor, missing pickup,
reset walk cycle, invented Defend pose, and combat paint outside its HD window.
It does not modify the saved source traces.

`presentation/rejected-move.json` is authority-only (`--modes authority`) with
the corridor fixture. Its runner-only fault changes one transmitted direction
to an invalid value while the client predicts the original clear step. Check
`--suite rejection`: actual prediction must occur, the server must reject it,
and world, camera and visible pointer must recover. No production debug API
or permissive server command is introduced.

Virtual transport pausing now includes success-result-before-world ordering;
receiving the result alone cannot charge a later socket task as gameplay time.
State receipt releases the clock before presentation so real animations still
execute. `test-clock.mjs` covers the delayed packet and stale-packet cases.
Real socket/IPC latency remains recorded separately. Repeated legacy waiting
seat audio onset may still vary; report it as unstable evidence, not a server
regression. World/route/pose checks remain independently inspectable.

Protocol notes: the existing visible-cell word is 0 (hidden), 1 (visible), or
257 (visible plus original MAP_EXTRA_MONSTER guard flag), or 2 (render-only
fragments of already-known objects at a hidden shroud edge). Edge records have
only refCount and object references; they carry no gameplay fields. No other player's
visibility mask is sent. Optional `adventureSounds` is a bounded actor-private
journal of serial IDs and original resource names; initial restore suppresses
historical playback and later projections deduplicate it. Use paired current
client/worker builds and a fresh preview room when testing these changes.

## Interaction lifecycle

Use `fixture.py --object 87` with `scenarios/shipyard.json`, `--object 79`
with `scenarios/pickup-route.json`, and `--combat --exchange` with
`scenarios/hero-exchange.json` (default Arrogance/seed). The exchange fixture
adds a second owned hero to the disclosed corridor through original placement
and verifies save roundtrip. `scenarios/two-battles.json` extends the combat
fixture through results, another encounter and manual defend. It waits for
actual control and the expected active stack before further battle input;
window visibility alone is not readiness.

After paired runs, use `interaction_checks.py DIRECTORY --suite
shipyard|pickup|exchange|battles`. These gates check original window widgets,
boat creation/cost/cancellation/blocked berth, sound before modal dismissal,
route cleanup, horse-loop termination, pointer visibility, post-exchange input
and second-battle control/armies. The ship sprite's current animation frame is
excluded from the widget comparison; raw frame/PCM comparisons stay strict.
`interaction_faults.py --shipyard DIR --pickup DIR --exchange DIR --battles DIR
--out FILE` proves nine injected trace regressions are detected. The bridge
unit test also covers native results returning without a combatFinished poll.


## Town modals and battle grid

`fixture.py --town 0` pairs with `scenarios/tavern-guild.json` and
`scenarios/town-portraits.json`. The first checks video pause/resume around the
original guild window; the second opens hero portraits immediately after both
garrison transfers, with no intervening creature click. Bitmap resource names
are now observable, so player-colored cards can be checked independently of
background animation.

`fixture.py --combat` pairs with `scenarios/combat-presentation.json`. Set
`manifest.settings` to `{"combatGrid":true}` to enable grid/range/hover in both
browsers. This changes only local display preferences, not GM2 or RNG. The
scenario covers idle animation under the pointer, wait/defend, Haste, the
second phase's disabled wait button, auto battle and closing results. `hover:
true` moves the pointer to `click` or `target` coordinates without clicking.

Run `modal_grid_checks.py DIRECTORY --suite guild|portraits|combat`, then repeat
with `--faults`. These focused gates include composed control-bar pixels,
per-frame enemy range and idle hover, not just widget flags. Replaying the
pre-fix captures fails guild/portrait checks; the combat faults catch stale
pixels even when the widget status is correct. Raw whole-frame/PCM comparisons
remain separate and may still fail.

`python3 scripts/test-authority-fog.py --runtime ../runtime --build
build/cmake-parity-native` runs the actual projection writer/reader against
original map fragments. It checks visibility, hidden gameplay fields, unknown
objects and hidden heroes, and rejects an injected missing-fragment fault.
It proves preservation of already-revealed scenery under the one-tile shroud
fringe, not full-map pixel parity for objects entirely concealed by fog.

### Resolved battle feedback and impact lifecycle

`fixture.py --combat --feedback positive|negative|zero|unlucky` saves a ranged
Castle army, 300 neutral pixies, zero mana and explicit morale/luck bonuses.
It recomputes movement before exporting. Both lanes must load this exact GM2;
never reseed or force a proc in the browser. The longer battle is necessary:
a short winning battle can contain no morale/luck checks at all.

```
python3 scripts/parity/fixture.py --combat --feedback positive --out test-results/feedback/fixture
python3 scripts/parity/run.py --fixture test-results/feedback/fixture --scenario scripts/parity/scenarios/combat-feedback.json --out test-results/feedback/positive
python3 scripts/parity/feedback_checks.py test-results/feedback/positive --profile positive
```

Repeat for negative (three actual bad-morale procs with the pinned seed), zero
and unlucky; use `run.py --browser webkit` for the second renderer. The focused
checker requires matching RNG, AI and gameplay, real proc targets/order,
original effect frames and sample requests, pause durations within one original
100 ms animation step, shooter recovery, and corpse references already visible
before the corresponding impact returns. It mutates real traces to prove it
rejects missing cues, stranded shooters and deferred bodies. Negative luck is a
stat/debuff in this vanilla engine; `army::checkLuck` has no negative proc.
Zero and unlucky profiles should retain identical RNG/audio behavior.

Journal presentationVersion 2 adds resolved morale, luck and impact boundaries;
version 1 retains the older replay path for still-running workers. Impact replay
uses original `powEffect` animation, but its client death settlement must not
call `processDeath`: that consumes RNG and changes aura/spell/gameplay state.
The server still owns those operations. Do not claim full AV parity from these
focused passes; comparison.json retains strict frame/PCM/timing differences.
