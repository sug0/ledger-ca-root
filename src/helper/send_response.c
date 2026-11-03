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
#include <string.h>  // memmove

#include "buffer.h"

#include "send_response.h"
#include "ca_key.h"
#include "globals.h"
#include "sw.h"

int helper_send_response_pubkey(void) {
    uint8_t resp[1 + CA_PUBLIC_KEY_LEN] = {0};
    size_t offset = 0;

    resp[offset++] = CA_PUBLIC_KEY_LEN;
    memmove(resp + offset, G_context.pk_info.raw_public_key, CA_PUBLIC_KEY_LEN);
    offset += CA_PUBLIC_KEY_LEN;

    return io_send_response_pointer(resp, offset, SWO_SUCCESS);
}

int helper_send_response_sig(void) {
    uint8_t resp[1 + CA_SIGNATURE_MAX_LEN] = {0};
    size_t offset = 0;

    resp[offset++] = (uint8_t) G_context.tbs.signature_len;
    memmove(resp + offset, G_context.tbs.signature, G_context.tbs.signature_len);
    offset += G_context.tbs.signature_len;

    return io_send_response_pointer(resp, offset, SWO_SUCCESS);
}
