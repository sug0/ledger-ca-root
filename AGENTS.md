# AGENTS.md

Instructions for AI coding agents working in this repository.

## What this is

CA Root, a Ledger application acting as an offline root CA: it signs X.509
TBSCertificate structures with a device-derived non-exportable key. Keep it
self-contained and portable (Linux + macOS).

## Layout

- `src/` — application code: APDU handlers, DER/TBS parsing, UI
- `Makefile` — app build configuration; includes the SDK's makefiles
- `mkconfig` — per-user local paths (gitignored; created from `mkconfig.example`)
- `ledger_app.toml` — app manifest for Ledger tooling
- `tests/` — speculos end-to-end test (pure speculos client, no ragger);
  `unit-tests/` — C unit tests (cmake + Unity)
- `icons/`, `glyphs/` — app icons and NBGL glyph sources

## Building

- GNU make >= 4.2 required. On macOS run `gmake` (`brew install make`);
  `/usr/bin/make` (3.81) breaks the SDK's install_params step.
- `BOLOS_SDK` must point to a ledger-secure-sdk checkout at API level 27
  (tag `v27.1.x`), never `master`. Set it in `mkconfig` or the environment.
- Plain `make` builds both curve variants (`make ed25519` / `make p256` for
  one). Default device is Nano S+ (`TARGET=nanos2`); override with
  `TARGET=nanox|stax|flex|apex_p|apex_m`.
- `DEBUG=1` enables `PRINTF` (visible in Speculos); `VERBOSE=1` prints commands.
- Outputs: `build/<variant>/<target>/bin/app.elf` (+ `.apdu`, `.hex`,
  `.sha256`), with copies in `bin/` and `debug/` at the repo root.

## Platform notes

- macOS: Homebrew LLVM, lld and GNU coreutils are keg-only and must take
  precedence over the Apple system tools — wire them via `CLANGPATH` and
  `PREPEND_PATH` in `mkconfig` (see `mkconfig.example`).
- Never commit machine-specific paths or platform detection. Everything
  user-specific belongs in `mkconfig`; the committed build files must stay
  platform-agnostic.

## Testing

- Unit tests: `make unit-test` (wraps the cmake project in `unit-tests/`
  — configure, build, `ctest`).
- End-to-end: one full issuance-flow test (`make e2e-test BACKEND=speculos`
  against the emulator — the backend is always explicit — or `BACKEND=device`
  against a real device over USB); both variants on CI; needs a test venv
  (`SPECULOS_VENV` in `mkconfig`) and Linux for `qemu-arm-static`, see
  `tests/README.md`.
- `.github/workflows/ci.yml` runs both suites on ubuntu-latest.

## Conventions

- C style enforced by `.clang-format` — run `make fmt` (or clang-format -i
  on the files you touched).
- Protocol changes belong in `doc/` (APDU, TBS) and
  `APP_SPECIFICATION.md` alongside the code.
