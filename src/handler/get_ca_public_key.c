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

#include <stdint.h>  // uint*_t

#include "get_ca_public_key.h"
#include "ca_key.h"
#include "globals.h"
#include "send_response.h"

int handler_get_ca_public_key(void) {
    uint16_t err_code = 0;
    ca_key_error_t stage = ca_get_public_key(G_context.pk_info.raw_public_key, &err_code);
    if (stage != CA_KEY_OK) {
        return helper_send_ca_key_error(stage, err_code);
    }
    return helper_send_response_pubkey();
}
