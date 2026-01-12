#define KEXAPI

//
// Set to TRUE to disable the PROTECTED_FUNCTION macro.
// Perhaps useful for debug builds, to catch exceptions under the 
// debugger more quickly - however this will prevent exceptions from
// being logged through VXL.
//
#define DISABLE_PROTECTED_FUNCTION FALSE

#define KEX_COMPONENT L"KexDll"
#define KEX_ENV_NATIVE
#define KEX_TARGET_TYPE_DLL
