# TBSCertificate Input Format

The application signs a DER encoded `TBSCertificate` as defined by
[RFC 5280](https://datatracker.ietf.org/doc/html/rfc5280#section-4.1). The
host builds the TBS of the certificate it wants issued (e.g. an intermediate
CA certificate), streams it to the device with `SIGN TBS`, and assembles the
final certificate around the returned signature.

## Structure

The device validates the outer structure before showing the review screen:

```console
TBSCertificate ::= SEQUENCE {
    version         [0] EXPLICIT Version DEFAULT v1,   -- optional
    serialNumber        INTEGER,
    signature           AlgorithmIdentifier,
    issuer              Name,
    validity            Validity,
    subject             Name,
    subjectPublicKeyInfo SubjectPublicKeyInfo,
    ...                 -- [1], [2], [3] optional trailing fields only
}
```

- The TBS as a whole must be at most 1024 bytes (chunks of 255 bytes each
  over APDU).
- `Name` is a `SEQUENCE OF SET { SEQUENCE { OID, value } }`; string values may
  be UTF8String, PrintableString, T61String or IA5String.
- The subject CommonName is optional; if present it must be at most 62 bytes,
  otherwise the request is rejected (no silent truncation).
- The serialNumber is displayed as hex and must fit 21 bytes for display.

## What is signed

The signature covers the **exact bytes** reassembled from the APDU chunks —
the full TBS TLV, including its tag and length. To issue the certificate, the
host appends the signature algorithm and the signature value:

```console
Certificate ::= SEQUENCE {
    tbsCertificate      TBSCertificate,   -- the exact signed bytes
    signatureAlgorithm  AlgorithmIdentifier,
    signatureValue      BIT STRING        -- 0x00 unused-bits byte || signature
}
```

The signature algorithm must match the device variant:

| Variant   | Algorithm                       | Signature value        |
| ---       | ---                             | ---                    |
| `ed25519` | id-Ed25519 (1.3.101.112)        | 64 bytes, RFC 8032     |
| `p256`    | ecdsa-with-SHA256 (1.2.840.10045.4.3.1) | DER, ≤ 72 bytes |

Anyone holding the CA root public key (from `GET CA PUBLIC KEY`) can verify
the certificate chain: the signature over the TBS bytes with the root key,
then the public key carried in the certificate becomes the next link.

## Reference implementation

`tests/application_client/x509.py` contains a minimal host-side
implementation of this flow (DER builders, certificate assembly, chain
verification) used by the end-to-end test.
