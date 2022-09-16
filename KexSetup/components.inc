// The actual VxKex files which are going to be installed.
// KexComponents
						VxKexLdr.exe					RCDATA VxKexLdr.exe
						KexMon32.exe					RCDATA KexMon32.exe
WIN64_ONLY(				KexMon64.exe					RCDATA KexMon64.exe)
						KexShlEx.dll					RCDATA KexShlEx.dll
						KexCfg.exe						RCDATA KexCfg.exe

DEBUG_ONLY(				VxKexLdr.pdb					RCDATA VxKexLdr.pdb)
DEBUG_ONLY(				KexMon32.pdb					RCDATA KexMon32.pdb)
DEBUG_ONLY(WIN64_ONLY(	KexMon64.pdb					RCDATA KexMon64.pdb))
DEBUG_ONLY(				KexShlEx.pdb					RCDATA KexShlEx.pdb)
DEBUG_ONLY(				KexCfg.pdb						RCDATA KexCfg.pdb)

// ExtendedDlls
KEX_DLL_RESOURCES(combase)
KEX_DLL_RESOURCES(dxg1)
KEX_DLL_RESOURCES(kernelba33)
KEX_DLL_RESOURCES(kernel33)
KEX_DLL_RESOURCES(ole33)
KEX_DLL_RESOURCES(shcore)
KEX_DLL_RESOURCES(user33)
KEX_DLL_RESOURCES(ntd33)
KEX_DLL_RESOURCES(powrpr0f)
KEX_DLL_RESOURCES(mfpl4t)
KEX_DLL_RESOURCES(bluetoothapis)

// miscellaneous
ChangeLog.txt								RCTEXT ChangeLog.txt
