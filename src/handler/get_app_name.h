#pragma once

#include "os.h"

/**
 * Length of APPNAME variable in the Makefile.
 */
#define APPNAME_LEN (sizeof(APPNAME) - 1)

/**
 * Maximum length of application name.
 */
#define MAX_APPNAME_LEN 64

/**
 * Handler for GET_APP_NAME command. Send APDU response with ASCII
 * encoded name of the application.
 *
 * @see variable APPNAME in Makefile.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_get_app_name(void);
