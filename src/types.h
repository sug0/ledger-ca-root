#pragma once

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t

#include "ca_key.h"
#include "tbs.h"

/**
 * Maximum TBSCertificate DER length accepted for signing (bytes).
 */
#define MAX_TBS_LEN 1024

/**
 * Enumeration with expected INS of APDU commands.
 */
typedef enum {
    GET_VERSION = 0x03,        /// version of the application
    GET_APP_NAME = 0x04,       /// name of the application
    GET_CA_PUBLIC_KEY = 0x05,  /// raw public key of the CA key
    SIGN_TBS = 0x06            /// sign a TBSCertificate, sent in chunks
} command_e;

/**
 * Enumeration with parsing state.
 */
typedef enum {
    STATE_NONE,   /// no flow in progress
    STATE_PARSED  /// TBSCertificate received and parsed, awaiting approval
} state_e;

/**
 * Enumeration with user request type.
 */
typedef enum {
    CONFIRM_TBS  /// confirm and sign a TBSCertificate
} request_type_e;

/**
 * Structure for TBSCertificate signing context: the received DER bytes plus
 * the fields formatted for the review screen.
 */
typedef struct {
    uint8_t tbs[MAX_TBS_LEN];                 /// TBSCertificate DER accumulated over chunks
    size_t tbs_len;                           /// bytes accumulated so far
    uint8_t expected_chunk;                   /// P1 sequence number of the next chunk
    char subject_cn[TBS_CN_MAX_LEN + 1];      /// subject CommonName, NUL-terminated
    char serial[2 + 2 * 21 + 1];              /// serialNumber as "0x" + hex
    char validity[13 + 3 + 15 + 1];           /// "YYYY-MM-DD - YYYY-MM-DD", raw content as fallback
    uint8_t signature[CA_SIGNATURE_MAX_LEN];  /// signature produced on approval
    size_t signature_len;                     /// length of the signature
} sign_tbs_ctx_t;

/**
 * Structure for global context.
 */
typedef struct {
    state_e state;            /// state of the context
    request_type_e req_type;  /// user request
    sign_tbs_ctx_t tbs;       /// TBSCertificate signing context
} global_ctx_t;
