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

#include "io.h"

#include "get_ca_public_key.h"
#include "ca_key.h"
#include "globals.h"
#include "send_response.h"
#include "sw.h"

int handler_get_ca_public_key(void) {
    if (!ca_get_public_key(G_context.pk_info.raw_public_key)) {
        return io_send_sw(SWO_SECURITY_ISSUE);
    }
    return helper_send_response_pubkey();
}
