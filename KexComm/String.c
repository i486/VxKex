#include <KexComm.h>

INT __scwprintf(
	IN	PCWSTR Format,
	IN	...)
{
	INT Result;
	va_list ArgList;
	va_start(ArgList, Format);
	Result = _vscwprintf(Format, ArgList);
	va_end(ArgList);
	return Result;
}