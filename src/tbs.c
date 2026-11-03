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

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memcmp

#include "der.h"
#include "tbs.h"

// CommonName attribute: OID 2.5.4.3
static const uint8_t OID_CN[] = {0x55, 0x04, 0x03};

/**
 * Extract the CommonName value from a DER Name (RDNSequence).
 * Recognizes UTF8String, PrintableString, T61String and IA5String values.
 *
 * @return false on a malformed Name, an unsupported CommonName string type
 * or a CommonName over the display limit; true otherwise, with
 * `info->subject_cn` left NULL when the name carries no CommonName
 */
static bool name_get_cn(const uint8_t *value, size_t length, tbs_info_t *info) {
    const uint8_t *cursor = value;
    const uint8_t *end = value + length;
    der_tlv_t rdn, atv, attr;

    // Name ::= RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
    // RDN ::= SET OF AttributeTypeAndValue
    while (cursor < end) {
        if (!der_read_tlv(&cursor, end, &rdn) || rdn.tag != DER_TAG_SET) {
            return false;
        }

        const uint8_t *set_cursor = rdn.value;
        const uint8_t *set_end = rdn.value + rdn.length;
        while (set_cursor < set_end) {
            if (!der_read_tlv(&set_cursor, set_end, &atv) || atv.tag != DER_TAG_SEQUENCE) {
                return false;
            }

            const uint8_t *atv_cursor = atv.value;
            const uint8_t *atv_end = atv.value + atv.length;
            if (!der_read_tlv(&atv_cursor, atv_end, &attr) || attr.tag != DER_TAG_OID ||
                attr.length != sizeof(OID_CN) || memcmp(attr.value, OID_CN, sizeof(OID_CN)) != 0) {
                continue;  // not the CommonName attribute, try the next one
            }

            if (!der_read_tlv(&atv_cursor, atv_end, &attr)) {
                return false;
            }
            switch (attr.tag) {
            case DER_TAG_UTF8_STRING:
            case DER_TAG_PRINTABLE_STRING:
            case DER_TAG_T61_STRING:
            case DER_TAG_IA5_STRING:
                if (attr.length > TBS_CN_MAX_LEN) {
                    return false;
                }
                info->subject_cn = attr.value;
                info->subject_cn_len = attr.length;
                return true;
            default:
                // unsupported string type for the CommonName
                return false;
            }
        }
    }
    return true;  // well-formed Name without a CommonName
}

bool tbs_parse(const uint8_t *buf, size_t len, tbs_info_t *info) {
    if (buf == NULL || info == NULL) {
        return false;
    }
    memset(info, 0, sizeof(tbs_info_t));

    const uint8_t *cursor = buf;
    const uint8_t *end = buf + len;
    der_tlv_t tlv;

    // TBSCertificate ::= SEQUENCE { ... }
    if (!der_read_tlv(&cursor, end, &tlv) || tlv.tag != DER_TAG_SEQUENCE) {
        return false;
    }
    const uint8_t *seq_end = tlv.value + tlv.length;
    cursor = tlv.value;

    // [0] EXPLICIT INTEGER version, DEFAULT v1 (absent for v1 certificates)
    if (!der_read_tlv(&cursor, seq_end, &tlv)) {
        return false;
    }
    if (tlv.tag == DER_TAG_CONTEXT_0) {
        const uint8_t *ver_cursor = tlv.value;
        der_tlv_t version;
        if (!der_read_tlv(&ver_cursor, tlv.value + tlv.length, &version) ||
            version.tag != DER_TAG_INTEGER || version.length != 1 || version.value[0] > 2) {
            return false;
        }
        if (!der_read_tlv(&cursor, seq_end, &tlv)) {
            return false;
        }
    }

    // serialNumber ::= INTEGER
    if (tlv.tag != DER_TAG_INTEGER || tlv.length == 0 || tlv.length > 21) {
        // RFC 5280: at most 20 octets, DER positive integers may carry a
        // leading 0x00 sign byte
        return false;
    }
    info->serial = tlv.value;
    info->serial_len = tlv.length;

    // signature ::= AlgorithmIdentifier (SEQUENCE) - contents unused
    if (!der_read_tlv(&cursor, seq_end, &tlv) || tlv.tag != DER_TAG_SEQUENCE) {
        return false;
    }

    // issuer ::= Name (SEQUENCE) - contents unused
    if (!der_read_tlv(&cursor, seq_end, &tlv) || tlv.tag != DER_TAG_SEQUENCE) {
        return false;
    }

    // validity ::= SEQUENCE { notBefore, notAfter }
    if (!der_read_tlv(&cursor, seq_end, &tlv) || tlv.tag != DER_TAG_SEQUENCE) {
        return false;
    }
    const uint8_t *validity_cursor = tlv.value;
    const uint8_t *validity_end = tlv.value + tlv.length;
    der_tlv_t time;
    for (uint8_t i = 0; i < 2; i++) {
        if (!der_read_tlv(&validity_cursor, validity_end, &time)) {
            return false;
        }
        if (time.tag != DER_TAG_UTC_TIME && time.tag != DER_TAG_GENERALIZED_TIME) {
            return false;
        }
        if (i == 0) {
            info->not_before = time.value;
            info->not_before_len = time.length;
        } else {
            info->not_after = time.value;
            info->not_after_len = time.length;
        }
    }
    if (validity_cursor != validity_end) {
        return false;
    }

    // subject ::= Name (SEQUENCE)
    if (!der_read_tlv(&cursor, seq_end, &tlv) || tlv.tag != DER_TAG_SEQUENCE) {
        return false;
    }
    // CommonName is optional in X.500 names; on a well-formed Name without
    // one, `info->subject_cn` stays NULL for the review screen placeholder
    if (!name_get_cn(tlv.value, tlv.length, info)) {
        return false;
    }

    // subjectPublicKeyInfo ::= SEQUENCE - contents unused
    if (!der_read_tlv(&cursor, seq_end, &tlv) || tlv.tag != DER_TAG_SEQUENCE) {
        return false;
    }

    // optional trailing fields: [1] issuerUniqueID, [2] subjectUniqueID,
    // [3] extensions - contents unused
    while (cursor < seq_end) {
        if (!der_read_tlv(&cursor, seq_end, &tlv) ||
            (tlv.tag != DER_TAG_CONTEXT_1 && tlv.tag != DER_TAG_CONTEXT_2 &&
             tlv.tag != DER_TAG_CONTEXT_3)) {
            return false;
        }
    }
    return cursor == seq_end;
}

bool tbs_format_date(const uint8_t *time, size_t len, char out[11]) {
    const char *src = (const char *) time;
    size_t year_digits;
    int year;

    if (len == 13 && src[12] == 'Z') {
        // UTCTime: RFC 5280 restricts it to 1950-2049, two-digit year
        year_digits = 2;
        int yy = 10 * (src[0] - '0') + (src[1] - '0');
        year = yy < 50 ? 2000 + yy : 1900 + yy;
    } else if (len == 15 && src[14] == 'Z') {
        year_digits = 4;
        year = 1000 * (src[0] - '0') + 100 * (src[1] - '0') + 10 * (src[2] - '0') + (src[3] - '0');
    } else {
        return false;
    }

    // everything up to the trailing Z must be a decimal digit
    for (size_t i = 0; i < len - 1; i++) {
        if (src[i] < '0' || src[i] > '9') {
            return false;
        }
    }

    const char *month_day = src + year_digits;
    out[0] = (char) ('0' + year / 1000 % 10);
    out[1] = (char) ('0' + year / 100 % 10);
    out[2] = (char) ('0' + year / 10 % 10);
    out[3] = (char) ('0' + year % 10);
    out[4] = '-';
    out[5] = month_day[0];
    out[6] = month_day[1];
    out[7] = '-';
    out[8] = month_day[2];
    out[9] = month_day[3];
    out[10] = '\0';
    return true;
}
