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

#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool
#include <stdio.h>    // snprintf
#include <string.h>   // memset, memcpy

#include "buffer.h"
#include "io.h"
#include "format.h"

#include "sign_tbs.h"
#include "dispatcher.h"
#include "tbs.h"
#include "globals.h"
#include "display.h"
#include "sw.h"

/**
 * Format the fields for the review screen into the context.
 */
static uint16_t format_review_fields(void) {
    tbs_info_t info;
    memset(&info, 0, sizeof(info));
    if (!tbs_parse(G_context.tbs.tbs, G_context.tbs.tbs_len, &info)) {
        return SWO_INCORRECT_DATA;
    }

    // subject CommonName is optional
    if (info.subject_cn != NULL) {
        memcpy(G_context.tbs.subject_cn, info.subject_cn, info.subject_cn_len);
        G_context.tbs.subject_cn[info.subject_cn_len] = '\0';
    }

    G_context.tbs.serial[0] = '0';
    G_context.tbs.serial[1] = 'x';
    if (format_hex(info.serial,
                   info.serial_len,
                   G_context.tbs.serial + 2,
                   sizeof(G_context.tbs.serial) - 2) == -1) {
        return SWO_INCORRECT_DATA;
    }

    char not_before[11];
    char not_after[11];
    if (tbs_format_date(info.not_before, info.not_before_len, not_before) &&
        tbs_format_date(info.not_after, info.not_after_len, not_after)) {
        snprintf(G_context.tbs.validity,
                 sizeof(G_context.tbs.validity),
                 "%s - %s",
                 not_before,
                 not_after);
    } else {
        // show the raw time content for anything the structural parser
        // accepted but the formatter rejects
        snprintf(G_context.tbs.validity,
                 sizeof(G_context.tbs.validity),
                 "%.*s - %.*s",
                 (int) info.not_before_len,
                 (const char *) info.not_before,
                 (int) info.not_after_len,
                 (const char *) info.not_after);
    }
    return SWO_SUCCESS;
}

int handler_sign_tbs(buffer_t *cdata, uint8_t chunk, bool more) {
    if (G_context.state != STATE_NONE && G_context.state != STATE_PARSED) {
        return io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
    }

    if (chunk == P1_START) {
        memset(&G_context, 0, sizeof(G_context));
        G_context.req_type = CONFIRM_TBS;
        G_context.state = STATE_NONE;
    } else if (chunk != G_context.tbs.expected_chunk) {
        // chunks must arrive in strict sequence
        G_context.state = STATE_NONE;
        return io_send_sw(SWO_INCORRECT_DATA);
    }

    if (G_context.req_type != CONFIRM_TBS) {
        return io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
    }

    if (G_context.tbs.tbs_len + cdata->size > sizeof(G_context.tbs.tbs)) {
        G_context.state = STATE_NONE;
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }
    if (!buffer_move(cdata, G_context.tbs.tbs + G_context.tbs.tbs_len, cdata->size)) {
        G_context.state = STATE_NONE;
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    G_context.tbs.tbs_len += cdata->size;
    G_context.tbs.expected_chunk = chunk + 1;

    if (more) {
        // more chunks are expected, nothing to parse yet
        return io_send_sw(SWO_SUCCESS);
    }

    uint16_t err = format_review_fields();
    if (err != SWO_SUCCESS) {
        G_context.state = STATE_NONE;
        return io_send_sw(err);
    }
    G_context.state = STATE_PARSED;
    return ui_display_tbs();
}
