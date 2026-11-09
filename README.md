# CA Root — Ledger Certificate Authority

A Ledger application that acts as the offline root of a small certificate
authority: it holds a non-exportable CA key derived from the device seed and
signs X.509 `TBSCertificate` structures on approval. The signature covers the
exact TBS bytes that go into the distributed certificate, so the returned
signature can be attached to the certificate as-is.

Two compile-time variants exist and can be installed side by side:

| Variant   | App name        | CA key                            | Signature                    |
| --------- | --------------- | --------------------------------- | ---------------------------- |
| `ed25519` | CA Root Ed25519 | SLIP-0010 Ed25519 at `m/8888'/0'` | Ed25519, 64 bytes, raw       |
| `p256`    | CA Root P256    | BIP32 secp256r1 at `m/8888'/0'`   | ECDSA-SHA256, DER, ≤ 72 bytes |

Protocol documentation: [APP_SPECIFICATION.md](APP_SPECIFICATION.md),
[doc/APDU.md](doc/APDU.md), [doc/TBS.md](doc/TBS.md).

## Prerequisites

| Tool | macOS (Homebrew) | Debian/Ubuntu |
|------|------------------|---------------|
| GNU make >= 4.2 | `brew install make` (run as `gmake`) | `apt install make` |
| LLVM clang + lld | `brew install llvm lld` | `apt install clang lld` |
| GNU coreutils | `brew install coreutils` | included |
| python3 with venv | included | included |

A Python virtualenv with ledgerblue is used by the build's install-params step
and for sideloading:

```shell
python3 -m venv ~/.venvs/ledger
~/.venvs/ledger/bin/pip install ledgerblue ledgered pillow
```

## Setup

All machine-specific paths live in `mkconfig`, which is gitignored. Copy the
template and edit it:

```shell
cp mkconfig.example mkconfig
```

`BOLOS_SDK` must point to a [ledger-secure-sdk](https://github.com/LedgerHQ/ledger-secure-sdk)
checkout at API level 27 (tag `v27.1.x`). Do not build against `master`.

macOS example (Homebrew LLVM/lld/coreutils are keg-only and must take
precedence over the Apple system tools):

```make
BOLOS_SDK ?= /path/to/ledger-secure-sdk
LEDGER_VENV ?= ~/.venvs/ledger
CLANGPATH ?= /opt/homebrew/opt/llvm/bin/
PREPEND_PATH ?= /opt/homebrew/opt/llvm/bin /opt/homebrew/opt/lld/bin /opt/homebrew/opt/coreutils/libexec/gnubin
```

Linux example (distro packages, nothing extra needed):

```make
BOLOS_SDK ?= /path/to/ledger-secure-sdk
LEDGER_VENV ?= ~/.venvs/ledger
```

## Build

```shell
make          # Linux: builds both variants
gmake         # macOS
```

Builds for Nano S+ by default. Other devices: `make TARGET=nanox|stax|flex|apex_p|apex_m`.
A single variant: `make ed25519` or `make p256`.

Useful flags: `DEBUG=1` (enable `PRINTF`), `VERBOSE=1` (print every command).

Outputs (per variant):

| File | Purpose |
|------|---------|
| `build/<variant>/<target>/bin/app.elf` | linked application (run this in Speculos) |
| `build/<variant>/<target>/bin/app.apdu` | APDU stream to install the app |
| `build/<variant>/<target>/bin/app.hex`, `app.sha256` | flashing / integrity |
| `bin/`, `debug/` (repo root) | copies of the above for the variant built last, plus link map and disassembly |

## Load on a device

Device must be connected, unlocked, on the dashboard:

```shell
make load              # build, then sideload (default variant: ed25519)
make load CA_VARIANT=p256
make load-offline
make delete            # remove the app
```

With a developer custom CA installed on the device, set `SCP_PRIVKEY` in
`mkconfig` to skip on-device confirmations. Without one, use Ledger Live's
developer mode instead.

## Run on Speculos

```shell
speculos --model nanosp build/ed25519/nanos2/bin/app.elf
```

Note that speculos needs `qemu-arm-static`, which only exists on Linux; on
macOS use the speculos Docker image
([docs](https://ledgerhq.github.io/speculos/user/macos.html)).

## Tests

- Unit tests (C, cmake + Unity): `make unit-test` (configure, build,
  `ctest` in one go).
- End-to-end test (certificate issuance flow): the backend is always
  explicit — `make e2e-test BACKEND=speculos` against the emulator (CI), or
  `make e2e-test BACKEND=device` against a real device over USB; one variant
  per run, pick with `CA_VARIANT=p256`. The flow signs two certificates
  per run, so a device run asks for two approvals. See `tests/README.md`
  for the venv setup (`SPECULOS_VENV` in `mkconfig`).
- CI (`.github/workflows/ci.yml`) runs both suites on GitHub-hosted runners.

## Documentation

The protocol documentation (application specification, APDU framing, TBS
format) is generated with doxygen:

```shell
doxygen .doxygen/Doxyfile
```

Outputs HTML into `doc/html`.

## License

Apache License 2.0 — see [LICENSE.md](LICENSE.md). © 2026 Tiago Carvalho.
Derived from Ledger's
[app-boilerplate](https://github.com/LedgerHQ/app-boilerplate); portions
remain © Ledger SAS.
