
  ///////////////////////////////////////////////////////////////////////////////
  //
  // Module Name:
  //
  //     rtlgstp.c
  //
  // Abstract:
  //
  //     Implementation of RtlGetSystemTimePrecise (Windows 8+).
  //
  // Environment:
  //
  //     Win32/x64 user mode, inside any process where KexDll is loaded.
  //
  ///////////////////////////////////////////////////////////////////////////////

  #include "buildcfg.h"
  #include "kexdllp.h"

  KEXAPI LARGE_INTEGER NTAPI KexRtlGetSystemTimePrecise(
        VOID)
  {
        LARGE_INTEGER SystemTime;

        SystemTime.QuadPart = 0;
        NtQuerySystemTime(&SystemTime.QuadPart);
        return SystemTime;
  }