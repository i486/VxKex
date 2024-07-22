///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     strmap.c
//
// Abstract:
//
//     This module contains the String Mapper API.
//
//     A string mapper is a data structure that lets you map one string to
//     another. In VxKex it is used as part of the mechanism that rewrites
//     the import directory of an image file.
//
// Author:
//
//     vxiiduu (21-Oct-2022)
//
// Environment:
//
//     String mappers can be used after the RTL heap system is initialized and
//     the process heap has been created.
//
// Revision History:
//
//     vxiiduu              21-Oct-2022  Initial creation.
//     vxiiduu              16-Mar-2024  Remove erroneous OPTIONAL qualifier on
//                                       the 2nd argument to InsertEntry.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Create a new string mapper.
//
//   StringMapper
//     Pointer that receives the address of the opaque string
//     mapper object.
//
//   Flags
//     May contain any of the KEX_RTL_STRING_MAPPER_* flags.
//     Invalid flags will cause STATUS_INVALID_PARAMETER_2.
//
KEXAPI NTSTATUS NTAPI KexRtlCreateStringMapper(
	OUT		PPKEX_RTL_STRING_MAPPER		StringMapper,
	IN		ULONG						Flags OPTIONAL)
{
	PKEX_RTL_STRING_MAPPER Mapper;
	PRTL_DYNAMIC_HASH_TABLE HashTable;
	BOOLEAN Success;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	*StringMapper = NULL;

	if (Flags & ~(KEX_RTL_STRING_MAPPER_FLAGS_VALID_MASK)) {
		return STATUS_INVALID_PARAMETER_2;
	}

	Mapper = SafeAlloc(KEX_RTL_STRING_MAPPER, 1);
	if (!Mapper) {
		return STATUS_NO_MEMORY;
	}

	HashTable = &Mapper->HashTable;
	Success = RtlCreateHashTable(&HashTable, 0, 0);
	if (!Success) {
		// The only way RtlCreateHashTable can fail is by running out of memory.
		// (Or an invalid parameter, but that won't happen to us.)
		SafeFree(Mapper);
		return STATUS_NO_MEMORY;
	}

	Mapper->Flags = Flags;
	*StringMapper = Mapper;

	return STATUS_SUCCESS;
}

//
// Delete a string mapper and free its resources.
//
//   StringMapper
//     Pointer to the opaque string mapper object.
//
KEXAPI NTSTATUS NTAPI KexRtlDeleteStringMapper(
	IN		PPKEX_RTL_STRING_MAPPER		StringMapper)
{
	PKEX_RTL_STRING_MAPPER Mapper;
	RTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator;
	PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry;

	if (!StringMapper || !*StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	Mapper = *StringMapper;

	//
	// Enumerate entries in the hash table and free all the memory.
	//

	RtlInitEnumerationHashTable(&Mapper->HashTable, &Enumerator);

	do {
		Entry = RtlEnumerateEntryHashTable(&Mapper->HashTable, &Enumerator);
		RtlFreeHeap(RtlProcessHeap(), 0, Entry);
	} until (Entry == NULL);

	RtlEndEnumerationHashTable(&Mapper->HashTable, &Enumerator);

	//
	// Free the hash table itself.
	//

	RtlDeleteHashTable(&(*StringMapper)->HashTable);
	SafeFree(*StringMapper);

	return STATUS_SUCCESS;
}

//
// Insert a new key-value pair into the string mapper.
//
// The UNICODE_STRING structures themselves are copied into the string
// mapper, but the actual string data that is pointed to by the Buffer
// member is not managed by the mapper - you must ensure that this data
// is not freed before you destroy the string mapper.
//
//   StringMapper
//     Pointer to a string mapper object
//
//   Key
//     A string which will be hashed. Do not specify a key which is a
//     duplicate of another key you have inserted earlier; this will make
//     lookups unreliable (may return any of the values associated with
//     the same key).
//
//   Value
//     Pointer to an uninterpreted UNICODE_STRING which can be retrieved
//     if you know the Key, using the KexRtlLookupEntryStringMapper API.
//
KEXAPI NTSTATUS NTAPI KexRtlInsertEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER		StringMapper,
	IN		PCUNICODE_STRING			Key,
	IN		PCUNICODE_STRING			Value)
{
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;
	ULONG KeySignature;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Key) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!Value) {
		return STATUS_INVALID_PARAMETER_3;
	}

	Entry = SafeAlloc(KEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY, 1);
	if (!Entry) {
		return STATUS_NO_MEMORY;
	}

	//
	// hash the key and use that as the hash table entry's signature.
	// No need to check RtlHashUnicodeString return value because it can't
	// fail unless an invalid parameter is supplied, which cannot happen.
	//
	RtlHashUnicodeString(
		Key,
		(StringMapper->Flags & KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS),
		HASH_STRING_ALGORITHM_DEFAULT,
		&KeySignature);

	Entry->Key = *Key;
	Entry->Value = *Value;

	RtlInsertEntryHashTable(
		&StringMapper->HashTable,
		&Entry->HashTableEntry,
		KeySignature,
		NULL);

	return STATUS_SUCCESS;
}

STATIC NTSTATUS NTAPI KexRtlpLookupRawEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER						StringMapper,
	IN		PCUNICODE_STRING							Key,
	OUT		PPKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY	EntryOut)
{
	BOOLEAN Success;
	BOOLEAN CaseInsensitive;
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;
	RTL_DYNAMIC_HASH_TABLE_CONTEXT Context;
	ULONG KeySignature;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Key) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!EntryOut) {
		return STATUS_INTERNAL_ERROR;
	}

	CaseInsensitive = (StringMapper->Flags & KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS);

	RtlHashUnicodeString(
		Key,
		CaseInsensitive,
		HASH_STRING_ALGORITHM_DEFAULT,
		&KeySignature);

	Entry = (PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY) RtlLookupEntryHashTable(
		&StringMapper->HashTable,
		KeySignature,
		&Context);

	while (TRUE) {
		if (!Entry || Entry->HashTableEntry.Signature != KeySignature) {
			*EntryOut = NULL;
			return STATUS_STRING_MAPPER_ENTRY_NOT_FOUND;
		}

		Success = RtlEqualUnicodeString(
			Key,
			&Entry->Key,
			CaseInsensitive);

		if (Success) {
			break;
		}

		//
		// If this loop continues more than once, that means there is a hash collision.
		// We will handle the situation by checking the rest of the entries in this
		// bucket.
		//
		// With a decent hash function, this code should very rarely be executed.
		//

		Entry = (PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY) RtlGetNextEntryHashTable(
			&StringMapper->HashTable,
			&Context);
	}

	*EntryOut = Entry;

	return STATUS_SUCCESS;
}

//
// Look up a single value by key.
//
//   StringMapper
//     Pointer to a string mapper object
//
//   Key
//     Should be the same as what you specified to an earlier insert call.
//     If no value with the specified key is found, this function will return
//     the STATUS_STRING_MAPPER_ENTRY_NOT_FOUND error code.
//
//   Value
//     Pointer to a structure which will receive the retrieved value data.
//
KEXAPI NTSTATUS NTAPI KexRtlLookupEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN		PCUNICODE_STRING				Key,
	OUT		PUNICODE_STRING					Value OPTIONAL)
{
	NTSTATUS Status;
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;

	Status = KexRtlpLookupRawEntryStringMapper(
		StringMapper,
		Key,
		&Entry);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (Value) {
		*Value = Entry->Value;
	}

	return Status;
}

//
// Remove a single value by key.
//
//   StringMapper
//     Pointer to a string mapper object
//
//   Key
//     Should be the same as what you specified to an earlier insert call.
//     If no value with the specified key is found, this function will return
//     the STATUS_STRING_MAPPER_ENTRY_NOT_FOUND error code.
//
KEXAPI NTSTATUS NTAPI KexRtlRemoveEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN		PCUNICODE_STRING				Key)
{
	NTSTATUS Status;
	BOOLEAN Success;
	PKEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY Entry;

	Status = KexRtlpLookupRawEntryStringMapper(
		StringMapper,
		Key,
		&Entry);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Success = RtlRemoveEntryHashTable(
		&StringMapper->HashTable,
		&Entry->HashTableEntry,
		NULL);

	SafeFree(Entry);

	if (!Success) {
		return STATUS_INTERNAL_ERROR;
	}

	return STATUS_SUCCESS;
}

//
// This is a convenience function which takes input and returns output
// from a single UNICODE_STRING structure. It is equivalent to calling
// KexRtlLookupEntryStringMapper with the Key and Value parameters pointing
// to the same UNICODE_STRING.
//
KEXAPI NTSTATUS NTAPI KexRtlApplyStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	PUNICODE_STRING					KeyToValue)
{
	return KexRtlLookupEntryStringMapper(StringMapper, KeyToValue, KeyToValue);
}

//
// This is a convenience function which inserts many entries into the
// mapper with a single call. It is intended to be used with static arrays.
//
KEXAPI NTSTATUS NTAPI KexRtlInsertMultipleEntriesStringMapper(
	IN		PKEX_RTL_STRING_MAPPER				StringMapper,
	IN		CONST KEX_RTL_STRING_MAPPER_ENTRY	Entries[],
	IN		ULONG								EntryCount)
{
	NTSTATUS FailureStatus;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Entries) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!EntryCount) {
		return STATUS_INVALID_PARAMETER_3;
	}

	FailureStatus = STATUS_SUCCESS;

	do {
		NTSTATUS Status;

		Status = KexRtlInsertEntryStringMapper(
			StringMapper,
			&Entries[EntryCount-1].Key,
			&Entries[EntryCount-1].Value);

		if (!NT_SUCCESS(Status)) {
			FailureStatus = Status;
		}
	} while (--EntryCount);

	return FailureStatus;
}

KEXAPI NTSTATUS NTAPI KexRtlLookupMultipleEntriesStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	KEX_RTL_STRING_MAPPER_ENTRY		Entries[],
	IN		ULONG							EntryCount)
{
	NTSTATUS FailureStatus;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!Entries) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!EntryCount) {
		return STATUS_INVALID_PARAMETER_3;
	}

	FailureStatus = STATUS_SUCCESS;

	do {
		NTSTATUS Status;

		Status = KexRtlLookupEntryStringMapper(
			StringMapper,
			&Entries[EntryCount-1].Key,
			&Entries[EntryCount-1].Value);

		if (!NT_SUCCESS(Status)) {
			FailureStatus = Status;
		}
	} while (--EntryCount);

	return FailureStatus;
}

KEXAPI NTSTATUS NTAPI KexRtlBatchApplyStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	UNICODE_STRING					KeyToValue[],
	IN		ULONG							KeyToValueCount)
{
	NTSTATUS FailureStatus;

	if (!StringMapper) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!KeyToValue) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!KeyToValueCount) {
		return STATUS_INVALID_PARAMETER_3;
	}

	FailureStatus = STATUS_SUCCESS;

	do {
		NTSTATUS Status;

		Status = KexRtlApplyStringMapper(
			StringMapper,
			&KeyToValue[KeyToValueCount-1]);

		if (!NT_SUCCESS(Status)) {
			FailureStatus = Status;
		}
	} while (--KeyToValueCount);

	return FailureStatus;
}