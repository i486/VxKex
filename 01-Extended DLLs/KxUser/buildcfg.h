#ifdef _M_X64
#  pragma comment(lib, "user32_x64.lib")
#else
#  pragma comment(lib, "user32_x86.lib")
#endif

#pragma comment(lib, "gdi32.lib")

#define KXUSERAPI

#define KEX_TARGET_TYPE_DLL
#define KEX_ENV_WIN32
#define KEX_COMPONENT L"KxUser"
