#include "buildcfg.h"
#include "mlsbdic.h"

//
// Convert escape sequences in a string to their actual character values.
// Supported escapes: \", \n, \r, and \uXXXX.
// Returns FALSE if the output buffer is too small or an invalid escape
// sequence is encountered.
//

STATIC BOOLEAN UnescapeString(
	IN	PWCHAR	Input,
	IN	ULONG	InputCch,
	OUT	PWCHAR	Output,
	IN	ULONG	OutputCch,
	OUT	PULONG	OutputLength)
{
	ULONG Index;
	ULONG OutIndex;
	WCHAR Character;
	WCHAR NextCharacter;
	
	OutIndex = 0;
	
	for (Index = 0; Index < InputCch; ++Index) {
		Character = Input[Index];
		
		if (Character == '\\') {
			if (Index + 1 >= InputCch) {
				return FALSE;
			}
			
			NextCharacter = Input[Index + 1];
			
			if (NextCharacter == '"') {
				if (OutIndex >= OutputCch - 1) {
					return FALSE;
				}

				Output[OutIndex] = '"';
				++OutIndex;
				++Index;
			} else if (NextCharacter == '\\') {
				if (OutIndex >= OutputCch - 1) {
					return FALSE;
				}

				Output[OutIndex] = '\\';
				++OutIndex;
				++Index;
			} else if (NextCharacter == 'n') {
				if (OutIndex >= OutputCch - 1) {
					return FALSE;
				}

				Output[OutIndex] = '\n';
				++OutIndex;
				++Index;
			} else if (NextCharacter == 'r') {
				if (OutIndex >= OutputCch - 1) {
					return FALSE;
				}

				Output[OutIndex] = '\r';
				++OutIndex;
				++Index;
			} else if (NextCharacter == 't') {
				if (OutIndex >= OutputCch - 1) {
					return FALSE;
				}

				Output[OutIndex] = '\t';
				++OutIndex;
				++Index;
			} else if (NextCharacter == 'u') {
				ULONG HexIndex;
				WCHAR HexDigit;
				WCHAR HexCharacter;

				if (Index + 5 >= InputCch) {
					return FALSE;
				}
				
				HexCharacter = 0;

				for (HexIndex = 0; HexIndex < 4; ++HexIndex) {
					HexDigit = Input[Index + (ARRAYSIZE(L"\\u") - 1) + HexIndex];
					HexCharacter *= 16;
					
					if (HexDigit >= L'0' && HexDigit <= '9') {
						HexCharacter += HexDigit - '0';
					} else if (HexDigit >= 'a' && HexDigit <= 'f') {
						HexCharacter += HexDigit - 'a' + 10;
					} else if (HexDigit >= 'A' && HexDigit <= 'F') {
						HexCharacter += HexDigit - 'A' + 10;
					} else {
						return FALSE;
					}
				}
				
				if (OutIndex >= OutputCch - 1) {
					return FALSE;
				}

				Output[OutIndex] = HexCharacter;
				++OutIndex;
				Index += 5;
			} else {
				return FALSE;
			}
		} else {
			if (OutIndex >= OutputCch - 1) {
				return FALSE;
			}

			Output[OutIndex] = Character;
			++OutIndex;
		}
	}
	
	Output[OutIndex] = '\0';
	*OutputLength = OutIndex;
	return TRUE;
}

//
// Parse a single line of the format "English"="Foreign" and unescape both
// strings into the provided buffers.
//

BOOLEAN ParseDictionaryLine(
	IN	PWCHAR	Line,
	IN	ULONG	LineCch,
	OUT	PWCHAR	EnglishBuffer,
	IN	ULONG	EnglishBufferCch,
	OUT	PULONG	EnglishCch,
	OUT	PWCHAR	ForeignBuffer,
	IN	ULONG	ForeignBufferCch,
	OUT	PULONG	ForeignCch)
{
	ULONG QuoteIndex;
	ULONG EnglishStart;
	ULONG EnglishLength;
	ULONG ForeignStart;
	ULONG ForeignLength;
	ULONG Index;
	WCHAR Character;
	BOOLEAN Success;
	
	if (LineCch == 0) {
		return FALSE;
	}
	
	if (Line[0] != L'"') {
		return FALSE;
	}

	//
	// Find the end quote of the English string, and therefore its length.
	//

	EnglishStart = 1;
	QuoteIndex = 1;
	
	while (QuoteIndex < LineCch) {
		Character = Line[QuoteIndex];
		
		if (Character == L'\\') {
			if (QuoteIndex + 1 >= LineCch) {
				return FALSE;
			}

			QuoteIndex += 2;
		} else if (Character == L'"') {
			break;
		} else {
			++QuoteIndex;
		}
	}
	
	if (QuoteIndex >= LineCch || Line[QuoteIndex] != L'"') {
		return FALSE;
	}
	
	EnglishLength = QuoteIndex - EnglishStart;
	
	if (EnglishLength == 0) {
		return FALSE;
	}

	//
	// Verify that there is an Equals sign and another opening string
	// for the foreign-language text.
	//
	
	if (QuoteIndex + 1 >= LineCch || Line[QuoteIndex + 1] != '=') {
		return FALSE;
	}
	
	if (QuoteIndex + 2 >= LineCch || Line[QuoteIndex + 2] != '"') {
		return FALSE;
	}

	//
	// Find the end quote and length of foreign text.
	//
	
	ForeignStart = QuoteIndex + 3;
	QuoteIndex = ForeignStart;
	
	while (QuoteIndex < LineCch) {
		Character = Line[QuoteIndex];
		
		if (Character == '\\') {
			if (QuoteIndex + 1 >= LineCch) {
				return FALSE;
			}

			QuoteIndex += 2;
		} else if (Character == '"') {
			break;
		} else {
			++QuoteIndex;
		}
	}
	
	if (QuoteIndex >= LineCch || Line[QuoteIndex] != '"') {
		return FALSE;
	}
	
	ForeignLength = QuoteIndex - ForeignStart;

	//
	// Make sure there aren't any junk characters past the end of the
	// key-value pair.
	//
	
	for (Index = QuoteIndex + 1; Index < LineCch; ++Index) {
		Character = Line[Index];
		if (Character != ' ' && Character != '\t') {
			return FALSE;
		}
	}

	//
	// Convert escape sequences in the strings to their real character
	// equivalents.
	//
	
	Success = UnescapeString(
		&Line[EnglishStart],
		EnglishLength,
		EnglishBuffer,
		EnglishBufferCch,
		EnglishCch);
	
	if (!Success) {
		return FALSE;
	}
	
	Success = UnescapeString(
		&Line[ForeignStart],
		ForeignLength,
		ForeignBuffer,
		ForeignBufferCch,
		ForeignCch);
	
	if (!Success) {
		return FALSE;
	}
	
	return TRUE;
}