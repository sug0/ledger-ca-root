"""
End-to-end test: issue an intermediate CA certificate with the device.

The host generates an intermediate CA key pair, builds the TBSCertificate of
the intermediate certificate, and has the application (the CA root) sign the
exact TBS bytes over the speculos review flow. The signature is returned,
the certificate is assembled, and the chain root -> intermediate is verified
cryptographically on the host.
"""

import os

from application_client import x509

APP_NAMES = {"ed25519": b"CA Root Ed25519", "p256": b"CA Root P256"}
VERSION = bytes((1, 0, 0))

ISSUER_CN = b"CA Root"
SUBJECT_CN = b"Test Intermediate"
# several RDNs make the TBS long enough to require chunked APDUs on both variants
SUBJECT_RDNS = [
    x509.rdn(x509.OID_CN, SUBJECT_CN),
    x509.rdn(bytes.fromhex("55040A"), b"Test Org"),
    x509.rdn(bytes.fromhex("55040B"), b"Test Organizational Unit"),
    x509.rdn(bytes.fromhex("550406"), b"DE"),
    x509.rdn(bytes.fromhex("550408"), b"Test State"),
    x509.rdn(bytes.fromhex("550407"), b"Test City"),
]
NOT_BEFORE = b"260101000000Z"
NOT_AFTER = b"360101000000Z"


def test_intermediate_certificate_chain(client, sender, variant):
    # --- the application is up and answers the basic commands
    assert sender.get_version() == VERSION
    assert sender.get_app_name() == APP_NAMES[variant]

    # --- the CA root public key
    root_pub = sender.get_ca_public_key()
    if variant == "ed25519":
        assert len(root_pub) == 32
        # fixed path m/8888'/0' over the speculos seed
        assert root_pub == x509.ed25519_root_pubkey_from_seed()
    else:
        assert len(root_pub) == 65 and root_pub[0] == 0x04
        x509.p256_point_from_raw(root_pub)  # raises if not a curve point

    # --- build the intermediate certificate TBS around a fresh key
    inter_priv, inter_pub = x509.make_intermediate_key(variant)
    serial = b"\x00" + os.urandom(15)
    tbs = x509.make_tbs(
        variant,
        serial,
        NOT_BEFORE,
        NOT_AFTER,
        ISSUER_CN,
        SUBJECT_RDNS,
        x509.make_spki(inter_pub, variant),
    )
    assert len(tbs) > 240, "expected a multi-chunk TBS"

    # --- sign on the device over the exact TBS bytes
    signature = sender.sign_tbs(tbs, on_review=lambda: review_approve(client))
    if variant == "ed25519":
        assert len(signature) == 64
    else:
        assert 68 <= len(signature) <= 72  # DER ECDSA with P-256

    # --- assemble the certificate and verify the chain root -> intermediate
    cert = x509.assemble_cert(tbs, signature, variant)
    chain_pub, fields, _ = x509.verify_chain(root_pub, cert, variant)

    assert fields["serial"] == serial
    assert fields["not_before"] == NOT_BEFORE
    assert fields["not_after"] == NOT_AFTER
    assert fields["issuer_cn"] == ISSUER_CN
    assert fields["subject_cn"] == SUBJECT_CN

    # the certificate carries the intermediate public key, which is the next
    # link of the chain and belongs to the key pair generated above
    assert chain_pub == inter_pub
    x509.key_roundtrip(inter_priv, chain_pub, variant)
