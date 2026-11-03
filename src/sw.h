#pragma once
#include "status_words.h"

// CA key operation failure stages. A real device has no PRINTF, so the
// failing stage is reported as its own status word and the raw cx error
// code of the failing syscall rides along in the response data.
#define SWO_CA_KEY_DERIVE 0x66A0
#define SWO_CA_KEY_INIT   0x66A1
#define SWO_CA_KEY_PAIR   0x66A2
#define SWO_CA_KEY_SIGN   0x66A3
#define SWO_CA_KEY_LEN    0x66A4
