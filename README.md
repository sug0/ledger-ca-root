# Ledger C Application

Starting point for Ledger hardware wallet applications written in C, based on
the [Ledger app-boilerplate](https://github.com/LedgerHQ/app-boilerplate).

## Prerequisites

| Tool | macOS (Homebrew) | Debian/Ubuntu |
|------|------------------|---------------|
| GNU make >= 4.2 | `brew install make` (run as `gmake`) | `apt install make` |
| LLVM clang + lld | `brew install llvm lld` | `apt install clang lld` |
| GNU coreutils | `brew install coreutils` | included |
| arm-none-eabi toolchain with newlib | `brew install --cask gcc-arm-embedded` (the `arm-none-eabi-gcc` formula lacks newlib) | `apt install gcc-arm-none-eabi libnewlib-arm-none-eabi` |
| jq | `brew install jq` | `apt install jq` |

Sanity check: `arm-none-eabi-gcc -print-sysroot` must print a path, not nothing.

Python virtualenv with the tools used by the build and for sideloading:

```shell
python3 -m venv .venv
.venv/bin/pip install ledgerblue ledgered pillow protobuf ecdsa
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
LEDGER_VENV ?= /path/to/.venv
CLANGPATH ?= /opt/homebrew/opt/llvm/bin/
PREPEND_PATH ?= /opt/homebrew/opt/llvm/bin /opt/homebrew/opt/lld/bin /opt/homebrew/opt/coreutils/libexec/gnubin
```

Linux example (distro packages, nothing extra needed):

```make
BOLOS_SDK ?= /path/to/ledger-secure-sdk
LEDGER_VENV ?= /path/to/.venv
```

## Build

```shell
make          # Linux
gmake         # macOS
```

Builds for Nano S+ by default. Other devices: `make TARGET=nanox|stax|flex|apex_p|apex_m`.

Useful flags: `DEBUG=1` (enable `PRINTF`), `VERBOSE=1` (print every command).

Outputs:

| File | Purpose |
|------|---------|
| `build/<target>/bin/app.elf` | linked application (run this in Speculos) |
| `build/<target>/bin/app.apdu` | APDU stream to install the app |
| `build/<target>/bin/app.hex`, `app.sha256` | flashing / integrity |
| `bin/`, `debug/` | copies of the above plus link map and disassembly |

## Load on a device

Device must be connected, unlocked, on the dashboard:

```shell
make load      # build, then sideload
make delete    # remove the app
make load-offline
```

With a developer custom CA installed on the device, set `SCP_PRIVKEY` in
`mkconfig` to skip on-device confirmations. Without one, use Ledger Live's
developer mode instead.

## Run on Speculos

```shell
pip install speculos   # not part of the venv by default
speculos --model nanosp build/nanos2/bin/app.elf
```

## Tests

- Unit tests (C, cmake + Unity): `unit-tests/`
- Functional tests (Python, ragger): `tests/standalone/`, `tests/swap/` —
  install with `pip install -r tests/standalone/requirements.txt`, see
  `tests/README.md`

## Documentation

Protocol documentation (application specification, APDU, transaction
serialization) is generated with doxygen:

```shell
doxygen .doxygen/Doxyfile
```

Outputs HTML into `doc/html`.
