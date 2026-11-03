/*
 * Unit tests for src/helper/send_response.c.
 *
 * helper_send_response_pubkey() / _sig() serialise data from G_context into a
 * response buffer and send it. They call io_send_response_pointer(), a
 * static-inline wrapper, so the mockable symbol is io_send_response_buffers
 * (io_send_response_pointer(ptr, size, sw) == io_send_response_buffers(&{ptr,
 * size, 0}, 1, sw)). A capture callback lets us assert the exact bytes,
 * length and status word produced.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "Mockio.h"

#include "globals.h"
#include "send_response.h"
#include "constants.h"
#include "sw.h"
#include "types.h"

// Value the mocked send returns; the helpers must forward it verbatim.
#define SEND_RET 0x1234

global_ctx_t G_context;

// Capture of what io_send_response_buffers received.
static uint8_t g_resp[260];
static size_t g_resp_size;
static size_t g_count;
static uint16_t g_sw;

static int send_cb(const buffer_t *rdatalist, size_t count, uint16_t sw, int cmock_num_calls) {
    (void) cmock_num_calls;
    g_count = count;
    g_sw = sw;
    g_resp_size = rdatalist->size;
    memcpy(g_resp, rdatalist->ptr, rdatalist->size);
    return SEND_RET;
}

void setUp(void) {
    Mockio_Init();
    memset(&G_context, 0, sizeof(G_context));
    memset(g_resp, 0, sizeof(g_resp));
    g_resp_size = 0;
    g_count = 0;
    g_sw = 0;
}

void tearDown(void) {
    Mockio_Verify();
    Mockio_Destroy();
}

// =========================================================================
// helper_send_response_pubkey
//   resp = CA_PUBLIC_KEY_LEN | raw_public_key
// =========================================================================

void test_send_response_pubkey(void) {
    for (size_t i = 0; i < CA_PUBLIC_KEY_LEN; i++) {
        G_context.pk_info.raw_public_key[i] = (uint8_t) (i + 1);
    }

    io_send_response_buffers_AddCallback(send_cb);
    io_send_response_buffers_ExpectAnyArgsAndReturn(SEND_RET);

    int ret = helper_send_response_pubkey();

    TEST_ASSERT_EQUAL(SEND_RET, ret);
    TEST_ASSERT_EQUAL(1, g_count);
    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, g_sw);
    TEST_ASSERT_EQUAL(1 + CA_PUBLIC_KEY_LEN, g_resp_size);
    TEST_ASSERT_EQUAL(CA_PUBLIC_KEY_LEN, g_resp[0]);
    TEST_ASSERT_EQUAL_MEMORY(G_context.pk_info.raw_public_key, g_resp + 1, CA_PUBLIC_KEY_LEN);
}

// =========================================================================
// helper_send_response_sig
//   resp = signature_len | signature
// =========================================================================

void test_send_response_sig(void) {
    static const uint8_t sig[5] = {0xDE, 0xAD, 0xBE, 0xEF, 0x42};
    memcpy(G_context.tbs.signature, sig, sizeof(sig));
    G_context.tbs.signature_len = sizeof(sig);

    io_send_response_buffers_AddCallback(send_cb);
    io_send_response_buffers_ExpectAnyArgsAndReturn(SEND_RET);

    int ret = helper_send_response_sig();

    TEST_ASSERT_EQUAL(SEND_RET, ret);
    TEST_ASSERT_EQUAL(1, g_count);
    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, g_sw);
    TEST_ASSERT_EQUAL(1 + sizeof(sig), g_resp_size);
    TEST_ASSERT_EQUAL(sizeof(sig), g_resp[0]);
    TEST_ASSERT_EQUAL_MEMORY(sig, g_resp + 1, sizeof(sig));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_send_response_pubkey);
    RUN_TEST(test_send_response_sig);
    return UNITY_END();
}
