#pragma once

/**
 * Helper to send APDU response with the raw CA public key.
 *
 * response = G_context.pk_info.raw_public_key length (1) ||
 *            G_context.pk_info.raw_public_key
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_pubkey(void);

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
