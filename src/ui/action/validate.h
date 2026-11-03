#pragma once

#include <stdbool.h>  // bool

/**
 * Action for TBSCertificate validation and signing.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void validate_tbs(bool choice);
