/*****************************************************************************
 *   Ledger CA Root Application
 *   (c) 2026 Tiago Carvalho
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

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t

#include "der.h"

// maximum long-form length octets handled: elements up to 4 GiB; anything
// larger cannot be addressed by size_t on the target anyway
#define DER_LENGTH_BYTES_MAX 4

bool der_read_tlv(const uint8_t **cursor, const uint8_t *end, der_tlv_t *tlv) {
    if (cursor == NULL || end == NULL || tlv == NULL || *cursor >= end) {
        return false;
    }

    const uint8_t *p = *cursor;

    // X.509 only uses low tag numbers and context tags 0-3; reject the
    // multi-byte tag form outright
    if ((*p & 0x1F) == 0x1F) {
        return false;
    }
    tlv->tag = *p++;

    if (end - p < 1) {
        return false;
    }

    size_t length = 0;
    if (*p < 0x80) {
        // short form
        length = *p++;
    } else if (*p == 0x80) {
        // indefinite length is BER-only, forbidden in DER
        return false;
    } else {
        uint8_t count = *p++ & 0x7F;
        if (count > DER_LENGTH_BYTES_MAX || (size_t) (end - p) < count) {
            return false;
        }
        // long form is only allowed for lengths >= 0x80, and the first
        // length octet must not be zero (DER minimality)
        if (count == 0 || *p == 0 || (count == 1 && *p < 0x80)) {
            return false;
        }
        for (uint8_t i = 0; i < count; i++) {
            length = (length << 8) | *p++;
        }
    }

    if ((size_t) (end - p) < length) {
        return false;
    }

    tlv->value = p;
    tlv->length = length;
    *cursor = p + length;
    return true;
}
