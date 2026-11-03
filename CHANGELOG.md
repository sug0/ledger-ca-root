# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-11-03

### Added

- CA Root application: an offline signing oracle for the root key of a small
  certificate authority, replacing the app-boilerplate starting point
- CA key derived from the device seed at `m/8888'/0'`; two compile-time
  variants (`ed25519`: SLIP-0010 Ed25519, `p256`: BIP32 secp256r1) that can
  be installed side by side
- `GET VERSION`, `GET APP NAME`, `GET CA PUBLIC KEY` and chunked `SIGN TBS`
  APDUs; signature over the exact TBSCertificate bytes after on-device review
  (subject CommonName, serial, validity)
- DER/TBS structural parser with unit tests
- End-to-end test issuing an intermediate certificate in speculos and
  verifying the root → intermediate chain on the host
- CI running the unit tests and the end-to-end test for both variants
