# End-to-end tests

A single end-to-end test (`tests/test_e2e.py`) exercises the full certificate
issuance flow against the application running in [speculos](https://github.com/LedgerHQ/speculos),
the Ledger device emulator:

1. the host generates an intermediate CA key pair,
2. builds the TBSCertificate of the intermediate certificate (subject,
   serial, validity, and the real intermediate public key in the SPKI),
3. sends the TBS DER to the application in chunked APDUs and approves the
   on-device review (navigate to the hold page, hold to sign — automated via
   the speculos REST API, no manual interaction),
4. receives the signature, assembles the intermediate certificate,
5. verifies the chain: the signature over the exact TBS bytes must verify
   with the CA root public key read from the device (`GET_CA_PUBLIC_KEY`),
   and the public key carried in the certificate must belong to the
   intermediate key pair generated in step 1.

`tests/application_client/` contains the protocol client (APDU command
sender) and the minimal X.509 tooling used by the test.

## Setup

The test needs the application built and a python venv with speculos:

```sh
python3 -m venv ~/.venvs/speculos
~/.venvs/speculos/bin/pip install speculos pycryptodomex pytest
```

Point `SPECULOS_VENV` in your `mkconfig` (see `mkconfig.example`) at that
venv.

**Important:** speculos requires `qemu-arm-static`, which only exists on
Linux. On macOS, either run the tests on a Linux machine/CI, or run the
official speculos Docker image; see the
[speculos documentation](https://ledgerhq.github.io/speculos/user/macos.html).

## Run

```sh
make test                   # ed25519 variant
make test CA_VARIANT=p256   # p256 variant
```

`make test` builds the variant first, then runs pytest. The test device
defaults to Nano S+ (`--model nanosp`); the speculos seed is fixed
(`tests/conftest.py`) so the expected CA root key is derived deterministically
and cross-checked in the test (ed25519 variant, SLIP-0010 at m/8888'/0').

Screenshots captured during the review navigation are written to
`tests/screenshots/` (gitignored) for failure analysis.

## CI

`.github/workflows/ci.yml` runs the same flow on GitHub Actions
(`ubuntu-latest`, both variants, plus the unit tests). No configuration
needed for public repositories.
