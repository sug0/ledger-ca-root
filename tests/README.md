# End-to-end tests

A single end-to-end test (`tests/test_e2e.py`) exercises the full certificate
issuance flow against the application running on a [speculos](https://github.com/LedgerHQ/speculos)
emulator or a real Ledger device (the backend is selected explicitly, see
[Run](#run)):

1. the host generates an intermediate CA key pair,
2. builds the TBSCertificate of the intermediate certificate (subject,
   serial, validity, and the real intermediate public key in the SPKI),
3. sends the TBS DER to the application in chunked APDUs and approves the
   on-device review (speculos: automated via the REST API; device: you hold
   to sign when prompted),
4. receives the signature, assembles the intermediate certificate,
5. verifies the chain: the signature over the exact TBS bytes must verify
   with the CA root public key read from the device (`GET_CA_PUBLIC_KEY`),
   and the public key carried in the certificate must belong to the
   intermediate key pair generated in step 1,
6. repeats steps 2–5 with a second TBSCertificate (same subject, fresh
   serial) to prove the application accepts consecutive signing flows in
   one session.

`tests/application_client/` contains the protocol client (APDU command
sender) and the minimal X.509 tooling used by the test.

## Setup

The test needs the application built and a python venv with speculos:

```sh
python3 -m venv ~/.venvs/speculos
~/.venvs/speculos/bin/pip install speculos pycryptodome pytest
```

For the device backend, `pip install ledgerblue` as well (see below).

Point `SPECULOS_VENV` in your `mkconfig` (see `mkconfig.example`) at that
venv.

**Important:** speculos requires `qemu-arm-static`, which only exists on
Linux. On macOS, either run the tests on a Linux machine/CI, or run the
official speculos Docker image; see the
[speculos documentation](https://ledgerhq.github.io/speculos/user/macos.html).

## Run

The backend must be given explicitly; the suite refuses to guess:

```sh
make test BACKEND=speculos CA_VARIANT=ed25519   # speculos emulator (CI)
make test BACKEND=device  CA_VARIANT=ed25519    # real device over USB HID
```

`make test` builds the variant first, then runs pytest. The test device
defaults to Nano S+ (`--model nanosp`); the speculos seed is fixed
(`tests/conftest.py`) so the expected CA root key is derived deterministically
and cross-checked in the test (ed25519 variant, SLIP-0010 at m/8888'/0').

Screenshots captured during the review navigation are written to
`tests/screenshots/` (gitignored) for failure analysis.

## Real device

The device backend runs the same flow against the application installed on a
real Ledger over USB HID (`ledgerblue`). Everything is driven by the host
except the review approval, which you perform physically:

- install ledgerblue in the test venv: `pip install ledgerblue`
- load the application: `make load CA_VARIANT=...` — the variant must match
  (`CA_VARIANT` / `--variant`)
- open the application on the device and close Ledger Live (it must not hold
  the USB HID interface)

Then `make test BACKEND=device`. The test prints the expected Subject,
Serial and Validity for comparison with the device screen, and prompts you
to **hold the right button to sign** (a long press on the left button
rejects and fails the test). The flow signs twice, so expect two prompts.
The exchange waits up to `--approve-timeout` seconds (default 120;
override with e.g. `PYTEST_ARGS="--approve-timeout 300"`).

## CI

`.github/workflows/ci.yml` runs the speculos backend on GitHub Actions
(`ubuntu-24.04`, both variants, plus the unit tests). No configuration
needed for public repositories.
