#include "buildcfg.h"
#include "kxuserp.h"

//
// These are all stubs. No idea how to begin implementing them properly.
// Let's see if they are even that important.
//

KXUSERAPI HRESULT WINAPI BluetoothGATTAbortReliableWrite(
	IN	HANDLE	Device,
	IN	ULONG	ReliableWriteContext,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTBeginReliableWrite(
	IN	HANDLE	Device,
	OUT	PVOID	ReliableWriteContext,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTEndReliableWrite(
	IN	HANDLE	Device,
	IN	ULONG	ReliableWriteContext,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetCharacteristics(
	IN	HANDLE	Device,
	IN	PVOID	Service OPTIONAL,
	IN	USHORT	CharacteristicsBufferCount,
	OUT	PVOID	CharacteristicsBuffer OPTIONAL,
	OUT	PUSHORT	CharacteristicsBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetCharacteristicValue(
	IN	HANDLE	Device,
	IN	PVOID	Characteristic,
	IN	ULONG	CharacteristicValueDataSize,
	OUT	PVOID	CharacteristicValue OPTIONAL,
	OUT	PUSHORT	CharacteristicValueSizeRequired OPTIONAL,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetDescriptors(
	IN	HANDLE	Device,
	IN	PVOID	Characteristic,
	IN	USHORT	DescriptorsBufferCount,
	OUT	PVOID	DescriptorsBuffer OPTIONAL,
	OUT	PUSHORT	DescriptorsBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetDescriptorValue(
	IN	HANDLE	Device,
	IN	PVOID	Descriptor,
	IN	ULONG	DescriptorValueDataSize,
	OUT	PVOID	DescriptorValue OPTIONAL,
	OUT	PUSHORT	DescriptorValueSizeRequired OPTIONAL,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetIncludedServices(
	IN	HANDLE	Device,
	IN	PVOID	ParentService OPTIONAL,
	IN	USHORT	IncludedServicesBufferCount,
	OUT	PVOID	IncludedServicesBuffer OPTIONAL,
	OUT	PUSHORT	IncludedServicesBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTGetServices(
	IN	HANDLE	Device,
	IN	USHORT	ServicesBufferCount,
	OUT	PVOID	ServicesBuffer OPTIONAL,
	OUT	PUSHORT	ServicesBufferActual,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTRegisterEvent(
	IN	HANDLE	Service,
	IN	ULONG	EventType,
	IN	PVOID	EventParameterIn,
	IN	PVOID	Callback,
	IN	PVOID	CallbackContext OPTIONAL,
	OUT	PVOID	EventHandle,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTSetCharacteristicValue(
	IN	HANDLE	Device,
	IN	PVOID	Characteristic,
	IN	PVOID	CharacteristicValue,
	IN	PVOID	ReliableWriteContext OPTIONAL,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTSetDescriptorValue(
	IN	HANDLE	Device,
	IN	PVOID	Descriptor,
	IN	PVOID	DescriptorValue,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}

KXUSERAPI HRESULT WINAPI BluetoothGATTUnregisterEvent(
	IN	HANDLE	EventHandle,
	IN	ULONG	Flags)
{
	return ERROR_NOT_SUPPORTED;
}