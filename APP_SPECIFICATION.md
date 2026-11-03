# Technical Specification — CA Root

## About

This documentation describes the APDU interface of the **CA Root** application,
an offline signing oracle for the root key of a small certificate authority.

The device holds a non-exportable CA key derived from its own seed at the fixed
path `m/8888'/0'` and offers exactly two operations built on top of it:

- Export the CA root public key (the trust anchor to distribute out of band).
- Receive a DER encoded `TBSCertificate` (RFC 5280), display its main fields
  for approval, and sign the **exact bytes received**.

The signature is produced over the TBSCertificate that goes into the
distributed certificate, so the holder of the returned signature can assemble
the final certificate by appending a signature algorithm and the signature
itself. This makes the device a root CA in the X.509 sense without it ever
parsing or storing full certificates.

Two compile-time variants exist, selected by the Makefile variable
`CA_VARIANT`:

| Variant    | App name           | CA key                                    | Signature                             |
| ---------- | ------------------ | ----------------------------------------- | ------------------------------------- |
| `ed25519`  | CA Root Ed25519    | SLIP-0010 Ed25519 at `m/8888'/0'`         | Ed25519 (RFC 8032), 64 bytes, raw     |
| `p256`     | CA Root P256       | BIP32 (secp256r1) at `m/8888'/0'`         | ECDSA over P-256 with SHA-256, DER, deterministic nonce (RFC 6979), ≤ 72 bytes |

Both variants can be installed on the same device side by side. The interface
can be accessed over HID or BLE.

## APDUs

The command APDU framing follows [doc/APDU.md](doc/APDU.md): 1-byte `Lc`, no
`Le` field, at most 255 bytes of command data.

### GET VERSION

Returns the application version as three raw bytes.

##### `Command`

| CLA | INS | P1  | P2  | Lc  |
| --- | --- | --- | --- | --- |
| E0  | 03  | 00  | 00  | 00  |

##### `Output data`

| Description               | Length |
| ---                       | ---    |
| Application major version | 1      |
| Application minor version | 1      |
| Application patch version | 1      |

### GET APP NAME

Returns the application name as an ASCII string (`CA Root Ed25519` or
`CA Root P256` depending on the variant).

##### `Command`

| CLA | INS | P1  | P2  | Lc  |
| --- | --- | --- | --- | --- |
| E0  | 04  | 00  | 00  | 00  |

##### `Output data`

| Description      | Length   |
| ---              | ---      |
| Application name | variable |

### GET CA PUBLIC KEY

Derives the CA key from the device seed at the fixed path `m/8888'/0'` and
returns its raw public key. The key is never exposed outside this command.

##### `Command`

| CLA | INS | P1  | P2  | Lc  |
| --- | --- | --- | --- | --- |
| E0  | 05  | 00  | 00  | 00  |

##### `Output data`

| Description        | Length                          |
| ---                | ---                             |
| Public key length  | 1                               |
| Public key         | 32 (Ed25519) / 65 (P-256 uncompressed point, `0x04 \|\| X \|\| Y`) |

### SIGN TBS

Receives a DER `TBSCertificate` in chunks, has the user review and approve it
on the device, and returns the signature over the exact bytes received.

##### `Command`

| CLA | INS | P1                | P2             | Lc       |
| --- | --- | ---               | ---            | ---      |
| E0  | 06  | Chunk index       | More chunks    | variable |

**P1 - Chunk index:**

- `0x00`: first chunk, resets any flow in progress
- `0x01`, `0x02`, ...: subsequent chunks, must arrive in strict sequence

**P2 - More chunks flag:**

- `0x00`: last chunk (triggers the review)
- `0x80`: more chunks to follow

##### `Input data`

| Description                                                  | Length   |
| ---                                                          | ---      |
| Chunk of the DER encoded TBSCertificate (≤ 255 bytes per APDU, ≤ 1024 bytes total) | variable |

##### `Output data` (intermediate chunks)

None, status word `9000`.

##### `Output data` (last chunk, after approval)

| Description        | Length   |
| ---                | ---      |
| Signature length   | 1        |
| Signature          | 64 (Ed25519, raw) / ≤ 72 (P-256, DER) |

The signature covers the reassembled TBSCertificate bytes verbatim: Ed25519
signs them directly (RFC 8032); P-256 signs their SHA-256 digest with a
deterministic nonce (RFC 6979) and returns the DER encoded signature.

##### `Review`

After the last chunk the application parses the structure (see
[doc/TBS.md](doc/TBS.md)) and shows up to three fields:

| Field   | Content                                            |
| ---     | ---                                                |
| Subject | subject CommonName (omitted if the name has none)  |
| Serial  | serialNumber content as `0x`-prefixed hex          |
| Valid   | `notBefore - notAfter` as `YYYY-MM-DD` (RFC 5280), raw content if malformed |

Approving (hold to sign) produces the signature; rejecting returns
`SWO_CONDITIONS_NOT_SATISFIED`. A CommonName longer than 62 bytes is a hard
reject rather than a truncated display.

## Status Words

| SW   | SW name                      | Description                                        |
| ---  | ---                          | ---                                                |
| 66A0 | SWO_CA_KEY_DERIVE            | Seed derivation refused or failed                  |
| 66A1 | SWO_CA_KEY_INIT              | CA private key import failed                       |
| 66A2 | SWO_CA_KEY_PAIR              | CA public key computation failed                   |
| 66A3 | SWO_CA_KEY_SIGN              | Signature computation failed                       |
| 66A4 | SWO_CA_KEY_LEN               | CA public key has an unexpected length             |
| 6985 | SWO_CONDITIONS_NOT_SATISFIED | Rejected by user, or a signing flow is in progress |
| 6A80 | SWO_INCORRECT_DATA           | Malformed TBSCertificate or out-of-sequence chunk  |
| 6A86 | SWO_INCORRECT_P1_P2          | Either P1 or P2 is incorrect                       |
| 6A87 | SWO_WRONG_DATA_LENGTH        | TBS exceeds 1024 bytes, or unexpected command data |
| 6D00 | SWO_INVALID_INS              | No command exists with INS                         |
| 6E00 | SWO_INVALID_CLA              | Bad CLA used for this application                  |
| 9000 | OK                           | Success                                            |

The `66Ax` words are returned by GET CA PUBLIC KEY and SIGN TBS when a CA key
operation fails. Their response data carries the raw error code of the
failing syscall as two big-endian bytes, which localizes the failure on a
real device where the application cannot print diagnostics.
