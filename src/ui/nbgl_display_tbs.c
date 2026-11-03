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

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "glyphs.h"
#include "os_io_seproxyhal.h"
#include "nbgl_use_case.h"
#include "io.h"

#include "display.h"
#include "globals.h"
#include "sw.h"
#include "validate.h"
#include "menu.h"

// most detailed flow: subject + serial + validity
static nbgl_contentTagValue_t pairs[3];
static nbgl_contentTagValueList_t pairList;

// called when the approve/reject footer is touched
static void review_choice(bool confirm) {
    // answer, display a status page and go back to main
    validate_tbs(confirm);
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, ui_menu_main);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, ui_menu_main);
    }
}

int ui_display_tbs(void) {
    if (G_context.req_type != CONFIRM_TBS || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
    }

    uint8_t pair_index = 0;

    if (G_context.tbs.subject_cn[0] != '\0') {
        pairs[pair_index].item = "Subject";
        pairs[pair_index].value = G_context.tbs.subject_cn;
        pair_index++;
    }
    pairs[pair_index].item = "Serial";
    pairs[pair_index].value = G_context.tbs.serial;
    pair_index++;
    pairs[pair_index].item = "Valid";
    pairs[pair_index].value = G_context.tbs.validity;
    pair_index++;

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = pair_index;
    pairList.pairs = pairs;
    pairList.wrapping = true;

    nbgl_useCaseReview(TYPE_MESSAGE,
                       &pairList,
                       &ICON_APP_BOILERPLATE,
                       "Review certificate\nto sign",
                       NULL,
                       NULL,
                       review_choice);
    return 0;
}
