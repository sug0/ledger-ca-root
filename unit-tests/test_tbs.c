/*
 * Unit tests for src/tbs.c (TBSCertificate parsing for the review screen).
 *
 * Fixtures are hand-built minimal DER structures. Only the fields the
 * on-device screen shows (serial, validity, subject CN) carry real content;
 * signature/issuer/SPKI are opaque SEQUENCEs to the parser.
 */

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "der.h"
#include "tbs.h"

// ---- fixtures --------------------------------------------------------------

// v3: version, serial=01, sigalg, empty issuer, validity, CN="Test Subject",
// empty SPKI
static const uint8_t TBS_V3[] = {
    0x30, 0x53,                                      // SEQUENCE, 83 bytes
    0xA0, 0x03, 0x02, 0x01, 0x02,                    // [0] version v3
    0x02, 0x01, 0x01,                                // serial 1
    0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,  // sigalg ecdsa-SHA256
    0x3D, 0x04, 0x03, 0x02,                          //
    0x30, 0x00,                                      // issuer (opaque)
    0x30, 0x20,                                      // validity
    0x17, 0x0D, '2',  '5',  '0',  '1',  '0',  '1',  '0',  '0',  '0', '0',
    '0',  '0',  'Z',  0x18, 0x0F, '2',  '0',  '5',  '0',  '0',  '1', '0',
    '1',  '0',  '0',  '0',  '0',  '0',  '0',  'Z',  0x30, 0x17,  // subject
    0x31, 0x15,                                                  //   SET
    0x30, 0x13,                                                  //     ATV
    0x06, 0x03, 0x55, 0x04, 0x03,                                //     OID CN
    0x0C, 0x0C, 'T',  'e',  's',  't',  ' ',  'S',  'u',  'b',  'j', 'e',
    'c',  't',  0x30, 0x00,  // SPKI (opaque)
};

// v1: no [0] version wrapper
static const uint8_t TBS_V1[] = {
    0x30, 0x4E,        // SEQUENCE, 78 bytes
    0x02, 0x01, 0x01,  // serial 1
    0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x00,  // issuer
    0x30, 0x20,                                                                          // validity
    0x17, 0x0D, '2',  '5',  '0',  '1',  '0',  '1',  '0',  '0',  '0',  '0',  '0',  '0',
    'Z',  0x18, 0x0F, '2',  '0',  '5',  '0',  '0',  '1',  '0',  '1',  '0',  '0',  '0',
    '0',  '0',  '0',  'Z',  0x30, 0x17,  // subject with CN
    0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0C, 'T',  'e',  's',
    't',  ' ',  'S',  'u',  'b',  'j',  'e',  'c',  't',  0x30, 0x00,  // SPKI
};

// v3 with all three optional trailing fields present
static const uint8_t TBS_V3_FULL_TAIL[] = {
    0x30, 0x5F,  // SEQUENCE, 95 bytes
    0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x30, 0x0A, 0x06, 0x08, 0x2A,
    0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x00,  // issuer
    0x30, 0x20,                                            // validity
    0x17, 0x0D, '2',  '5',  '0',  '1',  '0',  '1',  '0',  '0',  '0',  '0',  '0',
    '0',  'Z',  0x18, 0x0F, '2',  '0',  '5',  '0',  '0',  '1',  '0',  '1',  '0',
    '0',  '0',  '0',  '0',  '0',  'Z',  0x30, 0x17,  // subject with CN
    0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0C, 'T',  'e',
    's',  't',  ' ',  'S',  'u',  'b',  'j',  'e',  'c',  't',  0x30, 0x00,  // SPKI
    0x81, 0x02, 0x01, 0x01,                                                  // [1] issuerUniqueID
    0x82, 0x02, 0x01, 0x01,                                                  // [2] subjectUniqueID
    0xA3, 0x02, 0x30, 0x00,                                                  // [3] extensions
};

// subject CN as PrintableString instead of UTF8String
static const uint8_t TBS_CN_PRINTABLE[] = {
    0x30, 0x53, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x30, 0x0A, 0x06,
    0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x00,  // issuer
    0x30, 0x20,                                                        // validity
    0x17, 0x0D, '2',  '5',  '0',  '1',  '0',  '1',  '0',  '0',  '0',  '0',  '0',
    '0',  'Z',  0x18, 0x0F, '2',  '0',  '5',  '0',  '0',  '1',  '0',  '1',  '0',
    '0',  '0',  '0',  '0',  '0',  'Z',  0x30, 0x17,  // subject
    0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0C, 'T',  'e',
    's',  't',  ' ',  'S',  'u',  'b',  'j',  'e',  'c',  't',  0x30, 0x00,  // SPKI
};

// subject without any CommonName attribute (only C=DE)
static const uint8_t TBS_NO_CN[] = {
    0x30, 0x49,  // SEQUENCE, 73 bytes
    0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x30, 0x0A, 0x06, 0x08,
    0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x00,  // issuer
    0x30, 0x20,                                                  // validity
    0x17, 0x0D, '2',  '5',  '0',  '1',  '0',  '1',  '0',  '0',  '0',  '0',
    '0',  '0',  'Z',  0x18, 0x0F, '2',  '0',  '5',  '0',  '0',  '1',  '0',
    '1',  '0',  '0',  '0',  '0',  '0',  '0',  'Z',  0x30, 0x0D,  // subject: C=DE only
    0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06,        // OID countryName
    0x13, 0x02, 'D',  'E',  0x30, 0x00,                          // SPKI
};

void setUp(void) {
}
void tearDown(void) {
}

/**
 * Copy TBS_V3 into `out` with the subject CommonName stretched to `cn_len`
 * bytes of 'A'; every enclosing length field grows to match.
 *
 * @return the total buffer length
 */
static size_t build_tbs_v3_with_cn(uint8_t *out, size_t cn_len) {
    const size_t delta = cn_len - 12;  // TBS_V3 carries a 12 byte CommonName

    // the grown outer SEQUENCE crosses the short-form length limit: emit its
    // length in long form, which shifts the copied TBS_V3 body by one byte
    out[0] = DER_TAG_SEQUENCE;
    out[1] = 0x81;
    out[2] = 83 + delta;              // outer SEQUENCE content
    memcpy(out + 3, TBS_V3 + 2, 68);  // TBS_V3[2..69], through the CN tag
    out[71] = 12 + delta;             // CommonName length
    memset(out + 72, 'A', cn_len);
    memcpy(out + 72 + cn_len, TBS_V3 + 83, 2);  // SPKI

    out[60] += delta;  // subject SEQUENCE length
    out[62] += delta;  // RDN SET length
    out[64] += delta;  // AttributeTypeAndValue length
    return 74 + cn_len;
}

static void check_valid_fixture(const uint8_t *tbs, size_t len) {
    tbs_info_t info;
    memset(&info, 0, sizeof(info));

    TEST_ASSERT_TRUE_MESSAGE(tbs_parse(tbs, len, &info), "fixture should parse");
    TEST_ASSERT_EQUAL(1, info.serial[0]);
    TEST_ASSERT_EQUAL(1, info.serial_len);
    TEST_ASSERT_EQUAL(13, info.not_before_len);
    TEST_ASSERT_EQUAL_MEMORY("250101000000Z", info.not_before, 13);
    TEST_ASSERT_EQUAL(15, info.not_after_len);
    TEST_ASSERT_EQUAL_MEMORY("20500101000000Z", info.not_after, 15);
    TEST_ASSERT_NOT_NULL(info.subject_cn);
    TEST_ASSERT_EQUAL(12, info.subject_cn_len);
    TEST_ASSERT_EQUAL_MEMORY("Test Subject", info.subject_cn, 12);
}

void test_v3_certificate(void) {
    check_valid_fixture(TBS_V3, sizeof(TBS_V3));
}

void test_v1_certificate_without_version_field(void) {
    check_valid_fixture(TBS_V1, sizeof(TBS_V1));
}

void test_all_optional_trailing_fields(void) {
    check_valid_fixture(TBS_V3_FULL_TAIL, sizeof(TBS_V3_FULL_TAIL));
}

void test_printable_string_cn(void) {
    check_valid_fixture(TBS_CN_PRINTABLE, sizeof(TBS_CN_PRINTABLE));
}

void test_missing_cn_is_not_an_error(void) {
    tbs_info_t info;
    memset(&info, 0, sizeof(info));

    TEST_ASSERT_TRUE(tbs_parse(TBS_NO_CN, sizeof(TBS_NO_CN), &info));
    TEST_ASSERT_NULL(info.subject_cn);
    TEST_ASSERT_EQUAL(0, info.subject_cn_len);
}

void test_max_length_cn_accepted(void) {
    uint8_t buf[sizeof(TBS_V3) + TBS_CN_MAX_LEN];
    tbs_info_t info;

    size_t len = build_tbs_v3_with_cn(buf, TBS_CN_MAX_LEN);
    TEST_ASSERT_TRUE(tbs_parse(buf, len, &info));
    TEST_ASSERT_EQUAL(TBS_CN_MAX_LEN, info.subject_cn_len);
}

void test_oversized_cn_rejected(void) {
    uint8_t buf[sizeof(TBS_V3) + TBS_CN_MAX_LEN];
    tbs_info_t info;

    size_t len = build_tbs_v3_with_cn(buf, TBS_CN_MAX_LEN + 1);
    TEST_ASSERT_FALSE(tbs_parse(buf, len, &info));
}

void test_bad_outer_tag_rejected(void) {
    uint8_t broken[sizeof(TBS_V3)];
    memcpy(broken, TBS_V3, sizeof(TBS_V3));
    broken[0] = 0x31;  // SET instead of SEQUENCE
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(broken, sizeof(broken), &info));
}

void test_truncated_buffer_rejected(void) {
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(TBS_V3, sizeof(TBS_V3) - 1, &info));
    TEST_ASSERT_FALSE(tbs_parse(TBS_V3, 0, &info));
}

void test_null_buffer_rejected(void) {
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(NULL, sizeof(TBS_V3), &info));
}

void test_bad_version_number_rejected(void) {
    uint8_t broken[sizeof(TBS_V3)];
    memcpy(broken, TBS_V3, sizeof(TBS_V3));
    broken[5] = 0x03;  // version INTEGER = 3
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(broken, sizeof(broken), &info));
}

void test_bad_serial_tag_rejected(void) {
    uint8_t broken[sizeof(TBS_V3)];
    memcpy(broken, TBS_V3, sizeof(TBS_V3));
    broken[7] = 0x04;  // OCTET STRING instead of INTEGER
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(broken, sizeof(broken), &info));
}

void test_long_serial_rejected(void) {
    // serial content of 22 octets: over the RFC 5280 limit of 20 (+ sign byte)
    uint8_t broken[sizeof(TBS_V3) + 1];
    memcpy(broken, TBS_V3, sizeof(TBS_V3));
    memmove(broken + 6, broken + 5, sizeof(TBS_V3) - 5);
    broken[5] = 0x16;  // INTEGER, 22 bytes
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(broken, sizeof(broken), &info));
}

void test_bad_validity_time_tag_rejected(void) {
    uint8_t broken[sizeof(TBS_V3)];
    memcpy(broken, TBS_V3, sizeof(TBS_V3));
    broken[26] = 0x0C;  // notBefore as UTF8String
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(broken, sizeof(broken), &info));
}

void test_unexpected_tail_field_rejected(void) {
    uint8_t broken[sizeof(TBS_V3) + 2];
    memcpy(broken, TBS_V3, sizeof(TBS_V3));
    // grow the outer SEQUENCE by one element: an unexpected [4] context tag
    broken[1] += 2;
    memcpy(broken + sizeof(TBS_V3), (const uint8_t[]) {0x84, 0x00}, 2);
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(broken, sizeof(broken), &info));
}

void test_extra_content_in_validity_rejected(void) {
    uint8_t broken[sizeof(TBS_V3) + 2];
    memcpy(broken, TBS_V3, sizeof(TBS_V3));
    // shift the tail right by 2 and grow the validity SEQUENCE by one
    // empty SEQUENCE element
    memmove(broken + 26, broken + 24, sizeof(TBS_V3) - 24);
    memcpy(broken + 24, (const uint8_t[]) {0x30, 0x22, 0x05, 0x00}, 4);
    broken[1] += 2;
    tbs_info_t info;

    TEST_ASSERT_FALSE(tbs_parse(broken, sizeof(broken), &info));
}

void test_format_date_utc_time(void) {
    char out[11];

    TEST_ASSERT_TRUE(tbs_format_date((const uint8_t *) "260101000000Z", 13, out));
    TEST_ASSERT_EQUAL_STRING("2026-01-01", out);
}

void test_format_date_utc_century_rule(void) {
    char out[11];

    TEST_ASSERT_TRUE(tbs_format_date((const uint8_t *) "491231235959Z", 13, out));
    TEST_ASSERT_EQUAL_STRING("2049-12-31", out);
    TEST_ASSERT_TRUE(tbs_format_date((const uint8_t *) "500101000000Z", 13, out));
    TEST_ASSERT_EQUAL_STRING("1950-01-01", out);
}

void test_format_date_generalized_time(void) {
    char out[11];

    TEST_ASSERT_TRUE(tbs_format_date((const uint8_t *) "20260101000000Z", 15, out));
    TEST_ASSERT_EQUAL_STRING("2026-01-01", out);
}

void test_format_date_rejects_malformed(void) {
    char out[11];

    // wrong length, missing or wrong trailing Z, non-digit content
    TEST_ASSERT_FALSE(tbs_format_date((const uint8_t *) "260101000000", 12, out));
    TEST_ASSERT_FALSE(tbs_format_date((const uint8_t *) "20260101000000", 14, out));
    TEST_ASSERT_FALSE(tbs_format_date((const uint8_t *) "260101000000X", 13, out));
    TEST_ASSERT_FALSE(tbs_format_date((const uint8_t *) "202601010000000", 15, out));
    TEST_ASSERT_FALSE(tbs_format_date((const uint8_t *) "2A0101000000Z", 13, out));
    TEST_ASSERT_FALSE(tbs_format_date((const uint8_t *) "2026010100000.Z", 15, out));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_v3_certificate);
    RUN_TEST(test_v1_certificate_without_version_field);
    RUN_TEST(test_all_optional_trailing_fields);
    RUN_TEST(test_printable_string_cn);
    RUN_TEST(test_missing_cn_is_not_an_error);
    RUN_TEST(test_max_length_cn_accepted);
    RUN_TEST(test_oversized_cn_rejected);
    RUN_TEST(test_bad_outer_tag_rejected);
    RUN_TEST(test_truncated_buffer_rejected);
    RUN_TEST(test_null_buffer_rejected);
    RUN_TEST(test_bad_version_number_rejected);
    RUN_TEST(test_bad_serial_tag_rejected);
    RUN_TEST(test_long_serial_rejected);
    RUN_TEST(test_bad_validity_time_tag_rejected);
    RUN_TEST(test_format_date_utc_time);
    RUN_TEST(test_format_date_utc_century_rule);
    RUN_TEST(test_format_date_generalized_time);
    RUN_TEST(test_format_date_rejects_malformed);
    RUN_TEST(test_unexpected_tail_field_rejected);
    RUN_TEST(test_extra_content_in_validity_rejected);
    return UNITY_END();
}
