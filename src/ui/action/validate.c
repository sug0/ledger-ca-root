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

    G_context.state = STATE_APPROVED;
    if (!ca_sign(G_context.tbs.tbs,
                 G_context.tbs.tbs_len,
                 G_context.tbs.signature,
                 &G_context.tbs.signature_len)) {
        G_context.state = STATE_NONE;
        io_send_sw(SWO_SECURITY_ISSUE);
        return;
    }
    helper_send_response_sig();
}
