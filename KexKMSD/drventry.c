#include "buildcfg.h"
#include <KexComm.h>

VOID NTAPI DriverUnload(
	IN	PDRIVER_OBJECT	DriverObject)
{
	ASSERT (DriverObject != NULL);
	return;
}

NTSTATUS NTAPI DriverEntry(
	IN	OUT	PDRIVER_OBJECT	DriverObject,
	IN		PUNICODE_STRING	RegistryPath)
{
	ASSERT (DriverObject != NULL);
	ASSERT (RegistryPath != NULL);

	KdPrint("DriverEntry called\r\n");

	DriverObject->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}