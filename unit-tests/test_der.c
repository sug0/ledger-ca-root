/*
 * Unit tests for src/der.c (generic DER TLV walker).
 */

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "der.h"

void setUp(void) {
}
void tearDown(void) {
}

void test_short_form_element(void) {
    static const uint8_t data[] = {0x02, 0x01, 0x2A};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_TRUE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
    TEST_ASSERT_EQUAL_UINT8(DER_TAG_INTEGER, tlv.tag);
    TEST_ASSERT_EQUAL(1, tlv.length);
    TEST_ASSERT_EQUAL_PTR(data + 2, tlv.value);
    TEST_ASSERT_EQUAL_PTR(data + sizeof(data), cursor);
}

void test_long_form_element(void) {
    // SEQUENCE, 2 length bytes, 300 content bytes
    static const uint8_t data[4 + 300] = {0x30, 0x82, 0x01, 0x2C};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_TRUE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
    TEST_ASSERT_EQUAL_UINT8(DER_TAG_SEQUENCE, tlv.tag);
    TEST_ASSERT_EQUAL(300, tlv.length);
    TEST_ASSERT_EQUAL_PTR(data + 4, tlv.value);
    TEST_ASSERT_EQUAL_PTR(data + sizeof(data), cursor);
}

void test_empty_element(void) {
    static const uint8_t data[] = {0x30, 0x00};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_TRUE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
    TEST_ASSERT_EQUAL(0, tlv.length);
}

void test_truncated_element_rejected(void) {
    // header says 10 content bytes, only 2 present
    static const uint8_t data[] = {0x30, 0x0A, 0x00, 0x00};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

void test_missing_length_byte_rejected(void) {
    static const uint8_t data[] = {0x30};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

void test_indefinite_length_rejected(void) {
    static const uint8_t data[] = {0x30, 0x80, 0x00};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

void test_leading_zero_length_rejected(void) {
    // 0x53 encoded in 2 octets with a leading zero: non-minimal
    static const uint8_t data[4 + 0x53] = {0x30, 0x82, 0x00, 0x53};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

void test_short_value_in_long_form_rejected(void) {
    // length 0x10 in long form: must use the short form
    static const uint8_t data[3 + 0x10] = {0x30, 0x81, 0x10};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

void test_multibyte_tag_rejected(void) {
    static const uint8_t data[] = {0x9F, 0x03, 0x00};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

void test_length_over_4_bytes_rejected(void) {
    static const uint8_t data[10] = {0x30, 0x85, 0x01, 0x00, 0x00, 0x00, 0x00};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

void test_sequential_elements(void) {
    static const uint8_t data[] = {0x02, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02, 0x01, 0x03};
    const uint8_t *cursor = data;
    der_tlv_t tlv;

    for (uint8_t i = 1; i <= 3; i++) {
        TEST_ASSERT_TRUE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
        TEST_ASSERT_EQUAL(1, tlv.length);
        TEST_ASSERT_EQUAL(i, tlv.value[0]);
    }
    TEST_ASSERT_EQUAL_PTR(data + sizeof(data), cursor);
    TEST_ASSERT_FALSE(der_read_tlv(&cursor, data + sizeof(data), &tlv));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_short_form_element);
    RUN_TEST(test_long_form_element);
    RUN_TEST(test_empty_element);
    RUN_TEST(test_truncated_element_rejected);
    RUN_TEST(test_missing_length_byte_rejected);
    RUN_TEST(test_indefinite_length_rejected);
    RUN_TEST(test_leading_zero_length_rejected);
    RUN_TEST(test_short_value_in_long_form_rejected);
    RUN_TEST(test_multibyte_tag_rejected);
    RUN_TEST(test_length_over_4_bytes_rejected);
    RUN_TEST(test_sequential_elements);
    return UNITY_END();
}
