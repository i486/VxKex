#include <KexComm.h>

ULONG_PTR GetRemoteImageEntryPoint(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	ImageBase)
{
	IMAGE_DOS_HEADER DosHeader;
	IMAGE_NT_HEADERS NtHeaders;

	CHECKED(ReadProcessMemory(ProcessHandle, (PVOID) ImageBase, &DosHeader, sizeof(DosHeader), NULL));
	CHECKED(ReadProcessMemory(ProcessHandle, (PVOID) (ImageBase + DosHeader.e_lfanew), &NtHeaders, sizeof(NtHeaders), NULL));
	return (ULONG_PTR) RVA_TO_VA(ImageBase, NtHeaders.OptionalHeader.AddressOfEntryPoint);

Error:
	return 0;
}