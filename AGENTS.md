# Project boundaries

- This is the public shared C++ engine: rules, original rendering, portability,
  integration contracts and cross-runtime tests. The browser shell is public
  `homm3-web`; Rust and native authority implementations remain private in
  `homm3-lobby/worker`.
- Never publish the mixed archive history or server implementation branches here.
  They are preserved in private `homm3-engine-history`.
- Do not commit retail resources, saves, journals, credentials or generated builds.
- Keep the web and server engine pins identical for maintained releases.
- Public contributors can build/test the client without server access; full
  authority parity checks require the private worker and are run by maintainers.
- Run browser tests silently on isolated services; do not restart user sessions.
