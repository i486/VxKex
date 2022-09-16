#include <KexDll.h>
#include <BluetoothAPIs.h>

HRESULT DLLAPI BluetoothGATTGetServices(
	IN	HANDLE	hDevice,
	IN	USHORT	ServicesBufferCount,
	OUT	LPVOID	ServicesBuffer OPTIONAL,
	OUT	USHORT	*ServicesBufferActual,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTGetCharacteristics(
	IN	HANDLE	hDevice,
	IN	LPVOID	Service OPTIONAL,
	IN	USHORT	CharacteristicsBufferCount,
	OUT	LPVOID	CharacteristicsBuffer OPTIONAL,
	OUT	USHORT	*CharacteristicsBufferActual,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTUnregisterEvent(
	IN	HANDLE	EventHandle,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTRegisterEvent(
	IN	HANDLE	hService,
	IN	DWORD	EventType,
	IN	PVOID	EventParameterIn,
	IN	LPVOID	Callback,
	IN	LPVOID	CallbackContext OPTIONAL,
	OUT	LPVOID	pEventHandle,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTSetDescriptorValue(
	IN	HANDLE	hDevice,
	IN	LPVOID	Descriptor,
	IN	LPVOID	DescriptorValue,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTGetDescriptorValue(
	IN	HANDLE	hDevice,
	IN	LPVOID	Descriptor,
	IN	ULONG	DescriptorValueDataSize,
	OUT	LPVOID	DescriptorValue OPTIONAL,
	OUT	USHORT	*DescriptorValueSizeRequired OPTIONAL,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTSetCharacteristicValue(
	IN	HANDLE	hDevice,
	IN	LPVOID	Characteristic,
	IN	LPVOID	CharacteristicValue,
	IN	DWORD	ReliableWriteContext OPTIONAL,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTGetCharacteristicValue(
	IN	HANDLE	hDevice,
	IN	LPVOID	Characteristic,
	IN	ULONG	CharacteristicValueDataSize,
	OUT	LPVOID	CharacteristicValue OPTIONAL,
	OUT	USHORT	*CharacteristicValueSizeRequired OPTIONAL,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTGetDescriptors(
	IN	HANDLE	hDevice,
	IN	LPVOID	Characteristic,
	IN	USHORT	DescriptorsBufferCount,
	OUT	LPVOID	DescriptorsBuffer OPTIONAL,
	OUT	USHORT	*DescriptorsBufferActual,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTGetIncludedServices(
	IN	HANDLE	hDevice,
	IN	LPVOID	ParentService OPTIONAL,
	IN	USHORT	IncludedServicesBufferCount,
	OUT LPVOID	IncludedServicesBuffer OPTIONAL,
	OUT	USHORT	*IncludedServicesBufferActual,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTAbortReliableWrite(
	IN	HANDLE	hDevice,
	IN	DWORD	ReliableWriteContext,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTBeginReliableWrite(
	IN	HANDLE	hDevice,
	OUT	LPVOID	ReliableWriteContext,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI BluetoothGATTEndReliableWrite(
	IN	HANDLE	hDevice,
	IN	DWORD	ReliableWriteContext,
	IN	ULONG	Flags)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}