#pragma once

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "uxtheme.lib")

// The graphical asserts use task dialogs which we won't have access to
// when running as local SYSTEM.
#define KEX_DISABLE_GRAPHICAL_ASSERTS

#define KEX_TARGET_TYPE_EXE
#define KEX_ENV_WIN32
#define KEX_COMPONENT L"KexCfg"
#define FRIENDLYAPPNAME L"VxKex Configuration Tool"
