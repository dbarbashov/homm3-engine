# Project boundaries

- Keep shared C++ rules, original rendering, portability code, integration
  contracts and cross-runtime tests here. Website and JavaScript changes belong
  in `homm3-web`.
- Publish only the reviewed engine history. Do not push unrelated archive refs.
- Do not commit retail resources, saves, journals, credentials or generated builds.
- Keep consumer engine pins identical for maintained releases.
- Standalone client builds and source-level tests must work without service access.
  Maintainers run network parity checks using explicitly supplied worker binaries.
- Public documentation should explain usage and contributions, not internal
  repository organization or access policies.
- Run browser tests silently on isolated services; do not restart user sessions.

- Edit tracked `src/` and `include/` directly. Do not recreate a generated source
  tree or patch stack. `upstream-origin.json` records provenance, not a build dependency.
