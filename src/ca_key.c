/*****************************************************************************
 *   Ledger CA Root Application
 *   (c) 2026
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdbool.h>  // bool
#include <string.h>   // memset, memcpy

#include "os.h"
#include "cx.h"

#include "ca_key.h"

// CA key derivation path: m/8888'/0' (hardened)
static const uint32_t CA_PATH[] = {0x800022B8, 0x80000000};

#if defined(USE_ED25519)

#define CA_DERIVATION_MODE HDW_ED25519_SLIP10
#define CA_CURVE           CX_CURVE_Ed25519

#elif defined(USE_P256)

#define CA_DERIVATION_MODE HDW_NORMAL
#define CA_CURVE           CX_CURVE_SECP256R1

#endif

/**
 * Derive the CA private key from the device seed. The key material never
 * leaves this module; the caller gets a zeroized key on failure.
 */
static ca_key_error_t derive_ca_privkey(cx_ecfp_private_key_t *privkey, uint16_t *err_code) {
    uint8_t raw_privkey[64];  // 64 bytes per the syscall API, 32 are used
    cx_err_t error = os_derive_bip32_with_seed_no_throw(CA_DERIVATION_MODE,
                                                        CA_CURVE,
                                                        CA_PATH,
                                                        2,
                                                        raw_privkey,
                                                        NULL,
                                                        NULL,
                                                        0);
    ca_key_error_t stage = CA_KEY_ERR_DERIVE;
    if (error == CX_OK) {
        stage = CA_KEY_ERR_INIT;
        error = cx_ecfp_init_private_key_no_throw(CA_CURVE, raw_privkey, 32, privkey);
    }
    memset(raw_privkey, 0, sizeof(raw_privkey));
    if (error != CX_OK) {
        *err_code = (uint16_t) error;
    }
    return error == CX_OK ? CA_KEY_OK : stage;
}

#if defined(USE_ED25519)

/**
 * Copy the public key in the canonical RFC 8032 encoding: little-endian Y
 * with the sign of X in bit 255. Speculos exports the point already in that
 * 32-byte form, while the secure element exports affine coordinates as
 * 04 || X || Y with big-endian bytes (the same layout app-solana converts).
 */
static bool copy_public_key(uint8_t *out, const cx_ecfp_public_key_t *pubkey) {
    if (pubkey->W_len == CA_PUBLIC_KEY_LEN) {
        memcpy(out, pubkey->W, CA_PUBLIC_KEY_LEN);
        return true;
    }
    if (pubkey->W_len != 1 + 2 * CA_PUBLIC_KEY_LEN || pubkey->W[0] != 0x04) {
        return false;
    }
    for (size_t i = 0; i < CA_PUBLIC_KEY_LEN; i++) {
        out[i] = pubkey->W[1 + 2 * CA_PUBLIC_KEY_LEN - 1 - i];
    }
    if (pubkey->W[CA_PUBLIC_KEY_LEN] & 1) {
        out[CA_PUBLIC_KEY_LEN - 1] |= 0x80;
    }
    return true;
}

#elif defined(USE_P256)

static bool copy_public_key(uint8_t *out, const cx_ecfp_public_key_t *pubkey) {
    if (pubkey->W_len != CA_PUBLIC_KEY_LEN) {
        return false;
    }
    memcpy(out, pubkey->W, CA_PUBLIC_KEY_LEN);
    return true;
}

#endif

ca_key_error_t ca_get_public_key(uint8_t *out, uint16_t *err_code) {
    cx_ecfp_private_key_t privkey;
    cx_ecfp_public_key_t pubkey;
    cx_err_t error;
    ca_key_error_t stage = derive_ca_privkey(&privkey, err_code);

    if (stage == CA_KEY_OK) {
        error = cx_ecfp_generate_pair_no_throw(CA_CURVE, &pubkey, &privkey, true);
        if (error != CX_OK) {
            *err_code = (uint16_t) error;
            stage = CA_KEY_ERR_PAIR;
        } else if (!copy_public_key(out, &pubkey)) {
            stage = CA_KEY_ERR_LEN;
        }
    }
    memset(&privkey, 0, sizeof(privkey));
    return stage;
}

#if defined(USE_ED25519)

ca_key_error_t ca_sign(const uint8_t *tbs,
                       size_t tbs_len,
                       uint8_t *sig,
                       size_t *sig_len,
                       uint16_t *err_code) {
    cx_ecfp_private_key_t privkey;
    ca_key_error_t stage = derive_ca_privkey(&privkey, err_code);

    if (stage == CA_KEY_OK) {
        // Ed25519 signs the message directly; cxng computes the two SHA-512
        // passes of RFC 8032 internally
        cx_err_t error =
            cx_eddsa_sign_no_throw(&privkey, CX_SHA512, tbs, tbs_len, sig, CA_SIGNATURE_MAX_LEN);
        if (error == CX_OK) {
            *sig_len = CA_SIGNATURE_MAX_LEN;
        } else {
            *err_code = (uint16_t) error;
            stage = CA_KEY_ERR_SIGN;
        }
    }
    memset(&privkey, 0, sizeof(privkey));
    return stage;
}

#elif defined(USE_P256)

ca_key_error_t ca_sign(const uint8_t *tbs,
                       size_t tbs_len,
                       uint8_t *sig,
                       size_t *sig_len,
                       uint16_t *err_code) {
    cx_ecfp_private_key_t privkey;
    cx_sha256_t hash;
    uint8_t digest[CX_SHA256_SIZE];
    uint32_t info;
    cx_err_t error = CX_OK;

    ca_key_error_t stage = derive_ca_privkey(&privkey, err_code);

    if (stage == CA_KEY_OK) {
        error = cx_sha256_init_no_throw(&hash);
        if (error == CX_OK) {
            error = cx_hash_no_throw((cx_hash_t *) &hash,
                                     CX_LAST,
                                     tbs,
                                     tbs_len,
                                     digest,
                                     sizeof(digest));
        }
        if (error == CX_OK) {
            error = cx_ecdsa_sign_no_throw(&privkey,
                                           CX_RND_RFC6979,
                                           CX_SHA256,
                                           digest,
                                           sizeof(digest),
                                           sig,
                                           sig_len,
                                           &info);
        }
        if (error != CX_OK) {
            *err_code = (uint16_t) error;
            stage = CA_KEY_ERR_SIGN;
        }
    }

    memset(digest, 0, sizeof(digest));
    memset(&hash, 0, sizeof(hash));
    memset(&privkey, 0, sizeof(privkey));
    return stage;
}

#endif
