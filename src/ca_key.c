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

#include <string.h>  // memset, memcpy

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
static bool derive_ca_privkey(cx_ecfp_private_key_t *privkey) {
    uint8_t raw_privkey[64];  // 64 bytes per the syscall API, 32 are used
    cx_err_t error = os_derive_bip32_with_seed_no_throw(CA_DERIVATION_MODE,
                                                        CA_CURVE,
                                                        CA_PATH,
                                                        2,
                                                        raw_privkey,
                                                        NULL,
                                                        NULL,
                                                        0);
    if (error == CX_OK) {
        error = cx_ecfp_init_private_key_no_throw(CA_CURVE, raw_privkey, 32, privkey);
    }
    memset(raw_privkey, 0, sizeof(raw_privkey));
    return error == CX_OK;
}

bool ca_get_public_key(uint8_t *out) {
    cx_ecfp_private_key_t privkey;
    cx_ecfp_public_key_t pubkey;
    bool ok = derive_ca_privkey(&privkey);
    if (ok) {
        ok = cx_ecfp_generate_pair_no_throw(CA_CURVE, &pubkey, &privkey, true) == CX_OK;
    }
    memset(&privkey, 0, sizeof(privkey));
    if (!ok || pubkey.W_len != CA_PUBLIC_KEY_LEN) {
        return false;
    }
    memcpy(out, pubkey.W, CA_PUBLIC_KEY_LEN);
    return true;
}

#if defined(USE_ED25519)

bool ca_sign(const uint8_t *tbs, size_t tbs_len, uint8_t *sig, size_t *sig_len) {
    cx_ecfp_private_key_t privkey;
    bool ok = derive_ca_privkey(&privkey);
    if (ok) {
        // Ed25519 signs the message directly; cxng computes the two SHA-512
        // passes of RFC 8032 internally
        ok = cx_eddsa_sign_no_throw(&privkey, CX_SHA512, tbs, tbs_len, sig, CA_SIGNATURE_MAX_LEN) ==
             CX_OK;
    }
    memset(&privkey, 0, sizeof(privkey));
    if (ok) {
        *sig_len = CA_SIGNATURE_MAX_LEN;
    }
    return ok;
}

#elif defined(USE_P256)

bool ca_sign(const uint8_t *tbs, size_t tbs_len, uint8_t *sig, size_t *sig_len) {
    cx_ecfp_private_key_t privkey;
    cx_sha256_t hash;
    uint8_t digest[CX_SHA256_SIZE];
    uint32_t info;
    cx_err_t error = CX_OK;

    if (!derive_ca_privkey(&privkey)) {
        return false;
    }

    error = cx_sha256_init_no_throw(&hash);
    if (error == CX_OK) {
        error =
            cx_hash_no_throw((cx_hash_t *) &hash, CX_LAST, tbs, tbs_len, digest, sizeof(digest));
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

    memset(digest, 0, sizeof(digest));
    memset(&hash, 0, sizeof(hash));
    memset(&privkey, 0, sizeof(privkey));
    return error == CX_OK;
}

#endif
