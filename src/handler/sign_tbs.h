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

#pragma once

#include <stdbool.h>  // bool
#include "buffer.h"

/**
 * Receive a chunk of the TBSCertificate DER; on the last chunk, parse it and
 * start the review flow.
 *
 * @param[in] cdata chunk payload (raw DER bytes)
 * @param[in] chunk P1 sequence number of this chunk
 * @param[in] more  whether more chunks follow (P2_MORE)
 *
 * @return zero or positive integer if success, -1 otherwise.
 */
int handler_sign_tbs(buffer_t *cdata, uint8_t chunk, bool more);
