#pragma once

#include <stdint.h>  // uint*_t

#include "ca_key.h"

/**
 * Helper to send APDU response with the raw CA public key.
 *
 * response = raw_public_key length (1) ||
 *            raw_public_key
 *
 * @param[in] raw_public_key raw CA public key, CA_PUBLIC_KEY_LEN bytes
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_pubkey(const uint8_t *raw_public_key);

/**
 * Helper to send APDU response with the TBSCertificate signature.
 *
 * response = G_context.tbs.signature_len (1) ||
 *            G_context.tbs.signature (G_context.tbs.signature_len)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_sig(void);

/**
 * Helper to send the failure of a CA key operation: the stage as the status
 * word, the raw cx error code of the failing syscall (big endian) as
 * response data.
 *
 * @param[in] stage    failure stage reported by the CA key module
 * @param[in] err_code raw cx error code, 0 when the stage has none
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_ca_key_error(ca_key_error_t stage, uint16_t err_code);
