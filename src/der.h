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
 * DER tag bytes used by X.509 certificate structures.
 * High-tag-number form is not used by X.509 and is rejected by the parser.
 */
#define DER_TAG_INTEGER          0x02
#define DER_TAG_OID              0x06
#define DER_TAG_UTF8_STRING      0x0C
#define DER_TAG_SEQUENCE         0x30
#define DER_TAG_SET              0x31
#define DER_TAG_PRINTABLE_STRING 0x13
#define DER_TAG_T61_STRING       0x14
#define DER_TAG_IA5_STRING       0x16
#define DER_TAG_UTC_TIME         0x17
#define DER_TAG_GENERALIZED_TIME 0x18

/**
 * Context-specific tags of the optional trailing TBSCertificate fields
 * ([0] version, [1] issuerUniqueID, [2] subjectUniqueID, [3] extensions).
 */
#define DER_TAG_CONTEXT_0 0xA0
#define DER_TAG_CONTEXT_1 0x81
#define DER_TAG_CONTEXT_2 0x82
#define DER_TAG_CONTEXT_3 0xA3

/**
 * Parsed DER tag-length-value element.
 * `value` points into the caller's buffer; it is never NUL-terminated.
 */
typedef struct {
    uint8_t tag;           /// tag byte (multi-byte tags are rejected)
    const uint8_t *value;  /// content bytes
    size_t length;         /// content length
} der_tlv_t;

/**
 * Read one TLV element at the cursor and advance the cursor past it.
 *
 * Enforces DER rules: definite lengths only (indefinite is BER-only),
 * long-form length only when >= 0x80 (minimality), no leading zero length
 * byte, element fully inside [cursor, end).
 *
 * @param[in,out] cursor position of the next element; advanced past it on success
 * @param[in]     end     one past the last valid byte
 * @param[out]    tlv     parsed element (tag, content pointer and length)
 *
 * @return true if a well-formed element was read, false otherwise
 */
bool der_read_tlv(const uint8_t **cursor, const uint8_t *end, der_tlv_t *tlv);
