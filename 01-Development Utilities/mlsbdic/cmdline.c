#include "buildcfg.h"
#include "mlsbdic.h"

//
// Scan a quoted or non-quoted argument parameter and return its length.
//

STATIC ULONG GetLengthOfMaybeQuotedArgumentParameter(
	IN	PCWSTR		Ptr,
	OUT	PBOOLEAN	IsQuoted)
{
	PCWCHAR ValueStart;
	ULONG Length;

	if (*Ptr == L'"') {
		++Ptr;
		ValueStart = Ptr;
		Length = 0;
				
		while (*Ptr != L'\0' && *Ptr != L'"') {
			++Ptr;
			++Length;
		}
				
		if (*Ptr == L'"') {
			++Ptr;
		}

		*IsQuoted = TRUE;
	} else {
		ValueStart = Ptr;
		Length = 0;
				
		while (*Ptr != L'\0' && *Ptr != L' ' && *Ptr != L'\t' && 
				*Ptr != L'\r' && *Ptr != L'\n') {

			++Ptr;
			++Length;
		}

		*IsQuoted = FALSE;
	}

	return Length;
}

//
// Parse the command line string to extract /IN and /OUT arguments.
//

BOOLEAN ParseCommandLine(
	IN	PWSTR		CommandLine,
	OUT	PWCHAR		InPath,
	IN	SIZE_T		InPathCch,
	OUT	PWCHAR		OutPath,
	IN	SIZE_T		OutPathCch,
	OUT	PBOOLEAN	OutPresent)
{
	PWCHAR Ptr;
	BOOLEAN InSeen;
	BOOLEAN OutSeen;
	BOOLEAN IsQuoted;
	SIZE_T Length;
	
	InSeen = FALSE;
	OutSeen = FALSE;
	*OutPresent = FALSE;
	InPath[0] = '\0';
	OutPath[0] = '\0';
	
	Ptr = CommandLine;
	
	while (*Ptr != '\0') {
		while (IsSpace(*Ptr)) {
			++Ptr;
		}
		
		if (*Ptr == '\0') {
			break;
		}
		
		if (StringBeginsWithI(Ptr, L"/IN:")) {
			if (InSeen) {
				ErrorBoxF(L"Duplicate /IN argument.");
				return FALSE;
			}
			
			InSeen = TRUE;
			Ptr += ARRAYSIZE(L"/IN:") - 1;

			Length = GetLengthOfMaybeQuotedArgumentParameter(Ptr, &IsQuoted);
			
			if (Length == 0) {
				ErrorBoxF(L"Missing value for /IN argument.");
				return FALSE;
			}
			
			if (Length >= InPathCch) {
				ErrorBoxF(L"Input path is too long.");
				return FALSE;
			}

			Ptr += IsQuoted;
			KexRtlCopyMemory(InPath, Ptr, Length * sizeof(WCHAR));
			InPath[Length] = '\0';
			Ptr += Length + IsQuoted;
		} else if (StringBeginsWithI(Ptr, L"/OUT:")) {
			if (OutSeen) {
				ErrorBoxF(L"Duplicate /OUT argument.");
				return FALSE;
			}
			
			OutSeen = TRUE;
			*OutPresent = TRUE;
			Ptr += ARRAYSIZE(L"/OUT:") - 1;
			
			Length = GetLengthOfMaybeQuotedArgumentParameter(Ptr, &IsQuoted);
			
			if (Length == 0) {
				ErrorBoxF(L"Missing value for /OUT argument.");
				return FALSE;
			}
			
			if (Length >= OutPathCch) {
				ErrorBoxF(L"Output path is too long.");
				return FALSE;
			}
			
			Ptr += IsQuoted;
			KexRtlCopyMemory(OutPath, Ptr, Length * sizeof(WCHAR));
			OutPath[Length] = '\0';
			Ptr += Length + IsQuoted;
		} else {
			ErrorBoxF(L"Unknown command line argument.");
			return FALSE;
		}
	}
	
	if (!InSeen) {
		return FALSE;
	}
	
	return TRUE;
}