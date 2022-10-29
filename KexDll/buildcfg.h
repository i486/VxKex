#define KEXAPI

#ifdef _M_X64
#  pragma comment(linker, "/export:KexNtQuerySystemTime")
#endif

#define KEX_COMPONENT L"KexDll"
#define KEX_ENV_NATIVE
#define KEX_TARGET_TYPE_DLL
