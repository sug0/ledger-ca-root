"""
Thin X.509 tooling for the e2e test: DER builders for the TBSCertificate sent
to the device, assembly of the signed certificate, and chain verification of
the intermediate certificate against the CA root public key. Only relies on
pycryptodomex.
"""

import hashlib
import hmac
import os

from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS, eddsa

# Must mirror the seed passed to speculos (conftest.py)
SEED = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
CA_PATH = [8888, 0]  # m/8888'/0', the fixed derivation path of the CA root key

OID_CN = bytes.fromhex("550403")  # 2.5.4.3 CommonName
OID_ED25519 = bytes.fromhex("2B6570")  # 1.3.101.112
OID_EC_PUBLIC_KEY = bytes.fromhex("2A8648CE3D0201")  # 1.2.840.10045.2.1
OID_PRIME256V1 = bytes.fromhex("2A8648CE3D030107")  # 1.2.840.10045.3.1.7
OID_ECDSA_WITH_SHA256 = bytes.fromhex("2A8648CE3D040301")  # 1.2.840.10045.4.3.1


def tlv(tag, content):
    n = len(content)
    if n < 0x80:
        length = bytes([n])
    elif n < 0x100:
        length = b"\x81" + bytes([n])
    elif n < 0x10000:
        length = b"\x82" + n.to_bytes(2, "big")
    else:
        raise ValueError("content too long")
    return bytes([tag]) + length + content


def read_tlv(data, offset=0):
    """Read one TLV; return (tag, content, offset past the TLV)."""
    tag = data[offset]
    i = offset + 1
    n = data[i]
    i += 1
    if n & 0x80:
        k = n & 0x7F
        n = int.from_bytes(data[i:i + k], "big")
        i += k
    return tag, data[i:i + n], i + n


# ---------------------------------------------------------------------------
# keys
# ---------------------------------------------------------------------------

def bip39_seed(mnemonic=SEED):
    return hashlib.pbkdf2_hmac("sha512", mnemonic.encode(), b"mnemonic", 2048, 64)


def ed25519_root_pubkey_from_seed():
    """Expected CA root public key: SLIP-0010 ed25519 at m/8888'/0'."""
    k = chain = None
    I = hmac.new(b"ed25519 seed", bip39_seed(), hashlib.sha512).digest()
    k, chain = I[:32], I[32:]
    for index in CA_PATH:
        I = hmac.new(chain, b"\x00" + k + (index | 0x80000000).to_bytes(4, "big"),
                     hashlib.sha512).digest()
        k, chain = I[:32], I[32:]
    return eddsa.import_private_key(k).public_key().export_key(format="raw")


def make_intermediate_key(variant):
    """Generate the intermediate CA key pair; returns (private key, raw pubkey)."""
    if variant == "ed25519":
        priv = eddsa.import_private_key(os.urandom(32))
        return priv, priv.public_key().export_key(format="raw")
    key = ECC.generate(curve="P-256")
    pub = key.public_key().pointQ
    return key, b"\x04" + pub.x.to_bytes(32, "big") + pub.y.to_bytes(32, "big")


def p256_point_from_raw(pub65):
    """Decode an uncompressed P-256 point; raises if it is not on the curve."""
    if len(pub65) != 65 or pub65[0] != 0x04:
        raise ValueError("not an uncompressed P-256 public key")
    return ECC.construct(curve="P-256",
                         point_x=int.from_bytes(pub65[1:33], "big"),
                         point_y=int.from_bytes(pub65[33:65], "big"))


# ---------------------------------------------------------------------------
# certificate building
# ---------------------------------------------------------------------------

def rdn(oid, value, string_tag=0x0C):
    """One RelativeDistinguishedName: SET { SEQ { OID, string } }."""
    return tlv(0x31, tlv(0x30, tlv(0x06, oid) + tlv(string_tag, value)))


def make_spki(pub, variant):
    """SubjectPublicKeyInfo carrying the real intermediate public key."""
    if variant == "ed25519":
        alg = tlv(0x30, tlv(0x06, OID_ED25519))
    else:
        alg = tlv(0x30, tlv(0x06, OID_EC_PUBLIC_KEY) + tlv(0x06, OID_PRIME256V1))
    return tlv(0x30, alg + tlv(0x03, b"\x00" + pub))


def make_tbs(variant, serial, not_before, not_after, issuer_cn, subject_rdns, spki):
    """TBSCertificate DER for an intermediate CA certificate.

    subject_rdns is a list of pre-built RDN TLVs; the CommonName must be the
    first RDN for the review screen to pick it up.
    """
    if len(serial) > 21:
        raise ValueError("serial too long")
    version = tlv(0xA0, tlv(0x02, b"\x02"))  # v3
    serial_tlv = tlv(0x02, serial)
    if variant == "ed25519":
        signature_alg = tlv(0x30, tlv(0x06, OID_ED25519))
    else:
        signature_alg = tlv(0x30, tlv(0x06, OID_ECDSA_WITH_SHA256))
    issuer = tlv(0x30, rdn(OID_CN, issuer_cn))
    validity = tlv(0x30, tlv(0x17, not_before) + tlv(0x17, not_after))
    subject = tlv(0x30, b"".join(subject_rdns))
    return tlv(0x30, version + serial_tlv + signature_alg + issuer + validity + subject + spki)


def assemble_cert(tbs, signature, variant):
    """Certificate ::= SEQUENCE { tbsCertificate, signatureAlgorithm, signatureValue }."""
    if variant == "ed25519":
        alg = tlv(0x30, tlv(0x06, OID_ED25519))
    else:
        alg = tlv(0x30, tlv(0x06, OID_ECDSA_WITH_SHA256))
    return tlv(0x30, tbs + alg + tlv(0x03, b"\x00" + signature))


# ---------------------------------------------------------------------------
# chain verification
# ---------------------------------------------------------------------------

def _get_cn(name_content):
    """CommonName value from a Name (RDNSequence) content, or None."""
    offset = 0
    while offset < len(name_content):
        _, rdn_value, offset = read_tlv(name_content, offset)
        _, atv, _ = read_tlv(rdn_value, 0)
        _, oid, after_oid = read_tlv(atv, 0)
        if oid == OID_CN:
            _, value, _ = read_tlv(atv, after_oid)
            return value
    return None


def parse_tbs(tbs):
    """Minimal walk of our TBSCertificate; returns the interesting fields."""
    tag, content, _ = read_tlv(tbs)
    if tag != 0x30:
        raise ValueError("TBS is not a SEQUENCE")
    fields = {"serial": None, "not_before": None, "not_after": None,
              "issuer_cn": None, "subject_cn": None, "spki": None}
    offset = 0
    seen_sig_alg = False
    while offset < len(content):
        tag, value, offset = read_tlv(content, offset)
        if tag == 0xA0:
            continue  # version
        if tag == 0x02 and fields["serial"] is None:
            fields["serial"] = value
        elif tag == 0x30 and not seen_sig_alg:
            seen_sig_alg = True  # signature AlgorithmIdentifier
        elif tag == 0x30 and fields["issuer_cn"] is None:
            fields["issuer_cn"] = _get_cn(value)
        elif tag == 0x30 and fields["not_before"] is None:
            _, nb, after_nb = read_tlv(value, 0)
            _, na, _ = read_tlv(value, after_nb)
            fields["not_before"], fields["not_after"] = nb, na
        elif tag == 0x30 and fields["subject_cn"] is None:
            fields["subject_cn"] = _get_cn(value)
        elif tag == 0x30:
            _, alg, after_alg = read_tlv(value, 0)
            _, bits, _ = read_tlv(value, after_alg)
            if bits[0] != 0x00:
                raise ValueError("unexpected unused bits in SPKI")
            fields["spki"] = bits[1:]
    return fields


def verify_chain(root_pub, cert, variant):
    """Verify the intermediate certificate against the CA root public key.

    Checks that the signature in the certificate verifies over the exact TBS
    bytes with the root key, and returns the intermediate public key carried
    in the certificate (next link of the chain).
    """
    tag, body, _ = read_tlv(cert)
    if tag != 0x30:
        raise ValueError("certificate is not a SEQUENCE")
    # the first element is the full TBS TLV; the device signs exactly those bytes
    _, _, offset = read_tlv(body)
    tbs = body[:offset]
    _, alg, offset = read_tlv(body, offset)
    _, bits, offset = read_tlv(body, offset)
    signature = bits[1:]

    if variant == "ed25519":
        if alg != tlv(0x06, OID_ED25519):
            raise ValueError("unexpected signature algorithm")
        eddsa.new(eddsa.import_public_key(root_pub), "rfc8032").verify(tbs, signature)
    else:
        if alg != tlv(0x06, OID_ECDSA_WITH_SHA256):
            raise ValueError("unexpected signature algorithm")
        vk = p256_point_from_raw(root_pub)
        DSS.new(vk, "fips-186-3", encoding="der").verify(SHA256.new(tbs), signature)

    fields = parse_tbs(tbs)
    return fields["spki"], fields, signature


def key_roundtrip(priv, pub, variant):
    """Prove the certificate binds a key we control: sign/verify a nonce."""
    nonce = os.urandom(32)
    if variant == "ed25519":
        signature = eddsa.new(priv, "rfc8032").sign(nonce)
        eddsa.new(eddsa.import_public_key(pub), "rfc8032").verify(nonce, signature)
    else:
        signature = DSS.new(priv, "fips-186-3", encoding="der").sign(SHA256.new(nonce))
        vk = p256_point_from_raw(pub)
        DSS.new(vk, "fips-186-3", encoding="der").verify(SHA256.new(nonce), signature)
