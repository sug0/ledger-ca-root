#pragma once

#include <stdbool.h>  // bool

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define ICON_APP_CA_ROOT C_app_ca_root_14px
#define ICON_APP_HOME    C_home_ca_root_14px
#define ICON_APP_WARNING     C_icon_warning
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_CA_ROOT C_app_ca_root_64px
#define ICON_APP_HOME        ICON_APP_CA_ROOT
#define ICON_APP_WARNING     C_Warning_64px
#elif defined(TARGET_APEX_P)
#define ICON_APP_CA_ROOT C_app_ca_root_48px
#define ICON_APP_HOME        ICON_APP_CA_ROOT
#define ICON_APP_WARNING     LARGE_WARNING_ICON
#endif

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);

/**
 * Display the TBSCertificate review screen and ask the user to sign.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_tbs(void);
