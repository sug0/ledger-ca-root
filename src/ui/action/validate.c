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

#include <stdbool.h>  // bool

#include "ca_key.h"

#include "validate.h"
#include "sw.h"
#include "globals.h"
#include "send_response.h"

void validate_tbs(bool choice) {
    if (!choice) {
        G_context.state = STATE_NONE;
        io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
        return;
    }

    uint16_t err_code = 0;
    ca_key_error_t stage = ca_sign(G_context.tbs.tbs,
                                   G_context.tbs.tbs_len,
                                   G_context.tbs.signature,
                                   &G_context.tbs.signature_len,
                                   &err_code);
    // the flow is over either way; the context must accept a new one
    G_context.state = STATE_NONE;
    if (stage != CA_KEY_OK) {
        helper_send_ca_key_error(stage, err_code);
        return;
    }
    helper_send_response_sig();
}
