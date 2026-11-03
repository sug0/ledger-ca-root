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
 * Maximum subject CommonName length copied for display (without NUL).
 */
#define TBS_CN_MAX_LEN 62

/**
 * Fields of a DER TBSCertificate needed for the on-device review screen.
 * All pointers reference the caller's buffer and are not NUL-terminated;
 * they are only valid while that buffer lives.
 */
typedef struct {
    const uint8_t *serial;      /// serialNumber INTEGER content
    size_t serial_len;          /// may start with a 0x00 sign byte
    const uint8_t *not_before;  /// validity.notBefore, ASCII (UTCTime/GeneralizedTime)
    size_t not_before_len;
    const uint8_t *not_after;  /// validity.notAfter, ASCII (UTCTime/GeneralizedTime)
    size_t not_after_len;
    const uint8_t *subject_cn;  /// subject CommonName, or NULL if absent
    size_t subject_cn_len;
} tbs_info_t;

/**
 * Parse a DER encoded TBSCertificate (RFC 5280) for review purposes.
 *
 * Validates the overall structure: outer SEQUENCE, optional [0] version,
 * serialNumber, signature AlgorithmIdentifier, issuer, validity, subject,
 * subjectPublicKeyInfo, and only the three optional trailing context fields.
 * Does not interpret contents beyond what the review screen shows.
 *
 * @param[in]  buf  DER bytes of the TBSCertificate
 * @param[in]  len  buffer length
 * @param[out] info parsed fields (in-buffer pointers)
 *
 * @return true if the structure is well-formed
 */
bool tbs_parse(const uint8_t *buf, size_t len, tbs_info_t *info);
