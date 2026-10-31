# Maintaining the engine source

Builds compile the checked-in `src/` and `include/` directly. A normal checkout
contains all required recovered C++ source; there is no preparation command,
patch stack or upstream Git checkout needed to configure or compile it.
Toolchains, SDL/zlib and FFmpeg remain separate build dependencies.

Edit source files normally and review the changes through Git. The helper source
and cursor/video tables formerly extracted during preparation are ordinary source
files now; changes to their logic must be made in those tracked files.

## Original baseline

`upstream-origin.json` identifies the original repository, revision, license,
clean import commit and derived files. The import commit contains unmodified
source bytes. Its following commit applies the already-tested port adaptations.
The old patch files and extraction script remain available in earlier Git history.

To inspect the original code, use `git show IMPORT_COMMIT:src/hero.cpp` with the
import commit recorded in the origin file. Compare our adaptations using Git diff.

## Bringing in upstream changes

Obtain a chosen upstream revision in a separate temporary checkout. Review the
changes to the relevant source files, transfer the selected fixes into this tree,
and preserve their source attribution in the commit. Never replace the adapted
tree wholesale. Resolve conflicts as normal source changes and run the applicable
native/WASM and parity checks. Record any imported upstream revision in the
change's commit and update provenance when deliberately advancing the baseline.
No upstream updates happen automatically during a build.

## Shared revisions

After validation, commit engine changes and update both consumer engine locks to
the same revision. Include normal client and worker builds in validation; harness
instrumentation is a separate opt-in build and is not shipped with the site.
