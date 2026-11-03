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

#pragma once

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint*_t

/**
 * Compile-time curve variant, selected by the Makefile via CA_VARIANT.
 */
#if defined(USE_ED25519)
#define CA_PUBLIC_KEY_LEN    32  /// Ed25519 raw public key
#define CA_SIGNATURE_MAX_LEN 64
#elif defined(USE_P256)
#define CA_PUBLIC_KEY_LEN    65  /// secp256r1 uncompressed point (0x04 || X || Y)
#define CA_SIGNATURE_MAX_LEN 72
#else
#error "One of USE_ED25519 / USE_P256 must be defined as compile-time variant"
#endif

/**
 * Derive the CA key from the device seed at m/8888'/0' and export the raw
 * public key.
 *
 * @param[out] out raw public key, CA_PUBLIC_KEY_LEN bytes
 *
 * @return true on success
 */
bool ca_get_public_key(uint8_t *out);

/**
 * Sign the DER TBSCertificate bytes with the CA key derived from the device
 * seed. The signature covers the input bytes verbatim (Ed25519) or their
 * SHA-256 digest (ECDSA over secp256r1, DER encoded, deterministic nonce).
 *
 * @param[in]  tbs     TBSCertificate DER bytes
 * @param[in]  tbs_len input length
 * @param[out] sig     signature, at most CA_SIGNATURE_MAX_LEN bytes
 * @param[out] sig_len actual signature length
 *
 * @return true on success
 */
bool ca_sign(const uint8_t *tbs, size_t tbs_len, uint8_t *sig, size_t *sig_len);
