# AGENTS.md

Instructions for AI coding agents working in this repository.

## What this is

Scaffolding for Ledger hardware wallet applications in C. Derived projects
start from a clone of this repo; keep it self-contained and portable
(Linux + macOS).

## Layout

- `src/` — application code: APDU handlers, UI, transaction parsing
- `Makefile` — app build configuration; includes the SDK's makefiles
- `mkconfig` — per-user local paths (gitignored; created from `mkconfig.example`)
- `ledger_app.toml` — app manifest for Ledger tooling
- `tests/` — ragger functional tests; `unit-tests/` — C unit tests (cmake + Unity)
- `icons/`, `glyphs/` — app icons and NBGL glyph sources

## Building

- GNU make >= 4.2 required. On macOS run `gmake` (`brew install make`);
  `/usr/bin/make` (3.81) breaks the SDK's install_params step.
- `BOLOS_SDK` must point to a ledger-secure-sdk checkout at API level 27
  (tag `v27.1.x`), never `master`. Set it in `mkconfig` or the environment.
- Default device is Nano S+ (`TARGET=nanos2`); override with
  `TARGET=nanox|stax|flex|apex_p|apex_m`.
- `DEBUG=1` enables `PRINTF` (visible in Speculos); `VERBOSE=1` prints commands.
- Outputs: `build/<target>/bin/app.elf` (+ `.apdu`, `.hex`, `.sha256`),
  with copies in `bin/` and `debug/` at the repo root.

## Platform notes

- macOS: Homebrew LLVM, lld and GNU coreutils are keg-only and must take
  precedence over the Apple system tools — wire them via `CLANGPATH` and
  `PREPEND_PATH` in `mkconfig` (see `mkconfig.example`).
- Never commit machine-specific paths or platform detection. Everything
  user-specific belongs in `mkconfig`; the committed build files must stay
  platform-agnostic.

## Testing

- Unit tests: cmake project in `unit-tests/`.
- Functional tests: ragger + speculos, see `tests/README.md`.

## Conventions

- C style enforced by `.clang-format` — run it on touched files.
- Protocol changes belong in `doc/` (APDU, transaction) and
  `APP_SPECIFICATION.md` alongside the code.
