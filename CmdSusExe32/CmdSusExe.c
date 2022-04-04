#include <Windows.h>

#ifdef _M_X64
#  error "CmdSusExe32 is only supposed to be built for 32 bit. Fix the build configuration."
#endif

// This is a helper executable that returns the address of the 32-bit LoadLibaryW procedure
// within the WOW64 32-bit kernel32.dll. On a 32-bit installation, this executable is neither
// installed nor used because the loader will do the work.
//
// This is sort of a hack but it works just fine because the return code of a process is
// stored as a DWORD, which will always fit the 32-bit pointer. Also, the 32-bit kernel32.dll
// is be mapped to the same virtual address for every process.

VOID EntryPoint(
	VOID)
{
	ExitProcess((DWORD) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW"));
}