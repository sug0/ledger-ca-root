#pragma once

#include <string.h>

// NB: the SDK's os_seed.h static inlines call explicit_bzero, which the macOS
// libc headers do not declare; shim it onto memset for the host builds. On
// Linux the declaration exists and this is a no-op.
#if defined(__APPLE__) && !defined(EXPLICIT_BZERO_SHIM)
#define EXPLICIT_BZERO_SHIM
#define explicit_bzero(ptr, len) memset((ptr), 0, (len))
#endif
