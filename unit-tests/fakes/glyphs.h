#pragma once

#include "nbgl_types.h"

#if defined(TARGET_STAX) || defined(TARGET_FLEX)
extern const nbgl_icon_details_t C_app_ca_root_64px;
extern const nbgl_icon_details_t C_Warning_64px;
#elif defined(TARGET_APEX_P)
extern const nbgl_icon_details_t C_app_ca_root_48px;
#else
extern const nbgl_icon_details_t C_app_ca_root_14px;
extern const nbgl_icon_details_t C_home_ca_root_14px;
extern const nbgl_icon_details_t C_icon_warning;
#endif
