/*
 * Synchronization tests
 *
 * Copyright 2018 Daniel Lehman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#undef NDEBUG
#include <assert.h>
#include <windows.h>
#include <stdio.h>

#define WINAPI __stdcall

static BOOL (WINAPI *pWaitOnAddress)(volatile void *, void *, SIZE_T, DWORD);
static void (WINAPI *pWakeByAddressAll)(void *);
static void (WINAPI *pWakeByAddressSingle)(void *);

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#pragma warning(disable:4002)
#define ok assert
#define broken(x) (x)

static LONG address;
static LONG address2;

static DWORD WINAPI test_WaitOnAddress_func(void *arg)
{
    BOOL ret = FALSE;
    LONG compare;

    do
    {
        while (!(compare = address))
        {
			puts("Wait (1) started WaitOnAddress(&address, &compare, sizeof(compare), INFINITE)");
            SetLastError(0xdeadbeef);
            ret = pWaitOnAddress(&address, &compare, sizeof(compare), INFINITE);
			puts("Wait (1) finished");
            ok(ret, "wait failed\n");
            ok(GetLastError() == 0xdeadbeef || broken(GetLastError() == ERROR_SUCCESS) /* Win 8 */, "got error %d\n", GetLastError());
        }
    } while (InterlockedCompareExchange(&address, compare - 1, compare) != compare);

    return 0;
}

static DWORD WINAPI test_WaitOnAddress2_func(void *arg)
{
    BOOL ret = FALSE;
    LONG compare;

    do
    {
        while (!(compare = address2))
        {
			puts("Wait (2) started WaitOnAddress(&address2, &compare, sizeof(compare), INFINITE)");
            SetLastError(0xdeadbeef);
            ret = pWaitOnAddress(&address2, &compare, sizeof(compare), INFINITE);
			puts("Wait (2) finished");
            ok(ret, "wait failed\n");
            ok(GetLastError() == 0xdeadbeef || broken(GetLastError() == ERROR_SUCCESS) /* Win 8 */, "got error %d\n", GetLastError());
        }
    } while (InterlockedCompareExchange(&address2, compare - 1, compare) != compare);

    return 0;
}

static void test_WaitOnAddress(void)
{
    DWORD gle, val, nthreads, nthreads2;
    HANDLE threads[8];
	HANDLE threads2[8];
    LONG compare;
    BOOL ret;
    int i;

    address = 0;
    compare = 0;
    if (0) /* crash on Windows */
    {
        ret = pWaitOnAddress(&address, NULL, 4, 0);
        ret = pWaitOnAddress(NULL, &compare, 4, 0);
    }

    /* invalid arguments */
	puts("WakeByAddressSingle(NULL)");
    SetLastError(0xdeadbeef);
    pWakeByAddressSingle(NULL);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %d\n", gle);

	puts("WakeByAddressAll(NULL)");
    SetLastError(0xdeadbeef);
    pWakeByAddressAll(NULL);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %d\n", gle);

	puts("WaitOnAddress(NULL, NULL, 0, 0)");
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(NULL, NULL, 0, 0);
    gle = GetLastError();
    ok(gle == ERROR_INVALID_PARAMETER, "got %d\n", gle);
    ok(!ret, "got %d\n", ret);

	puts("WaitOnAddress(&address, &compare, 5, 0)");
    address = 0;
    compare = 0;
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 5, 0);
    gle = GetLastError();
    ok(gle == ERROR_INVALID_PARAMETER, "got %d\n", gle);
    ok(!ret, "got %d\n", ret);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));
    ok(compare == 0, "got %s\n", wine_dbgstr_longlong(compare));

    /* no waiters */
	puts("WakeByAddressSingle(&address)");
    address = 0;
    SetLastError(0xdeadbeef);
    pWakeByAddressSingle(&address);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %d\n", gle);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));

	puts("WakeByAddressAll(&address)");
    SetLastError(0xdeadbeef);
    pWakeByAddressAll(&address);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %d\n", gle);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));

    /* different address size */
	puts("WaitOnAddress(&address, &compare, 2, 0)");
    address = 0;
    compare = 0xff00;
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 2, 0);
    gle = GetLastError();
    ok(gle == 0xdeadbeef || broken(gle == ERROR_SUCCESS) /* Win 8 */, "got %d\n", gle);
    ok(ret, "got %d\n", ret);

	puts("WaitOnAddress(&address, &compare, 1, 0)");
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 1, 0);
    gle = GetLastError();
    ok(gle == ERROR_TIMEOUT, "got %d\n", gle);
    ok(!ret, "got %d\n", ret);

    /* simple wait case */
	puts("WaitOnAddress(&address, &compare, 4, 0)");
	address = 0;
    compare = 1;
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 4, 0);
    gle = GetLastError();
    ok(gle == 0xdeadbeef || broken(gle == ERROR_SUCCESS) /* Win 8 */, "got %d\n", gle);
    ok(ret, "got %d\n", ret);

    /* WakeByAddressAll */
    address = 0;
    for (i = 0; i < ARRAY_SIZE(threads); i++)
        threads[i] = CreateThread(NULL, 0, test_WaitOnAddress_func, NULL, 0, NULL);

    Sleep(100);
    address = ARRAY_SIZE(threads);
	puts("WakeByAddressAll(&address)");
    pWakeByAddressAll(&address);
    val = WaitForMultipleObjects(ARRAY_SIZE(threads), threads, TRUE, 5000);
    ok(val == WAIT_OBJECT_0, "got %d\n", val);
    for (i = 0; i < ARRAY_SIZE(threads); i++)
        CloseHandle(threads[i]);
    ok(!address, "got unexpected value %s\n", wine_dbgstr_longlong(address));

    /* WakeByAddressSingle */
    address = 0;
	address2 = 0;
    for (i = 0; i < ARRAY_SIZE(threads); i++)
        threads[i] = CreateThread(NULL, 0, test_WaitOnAddress_func, NULL, 0, NULL);

	for (i = 0; i < ARRAY_SIZE(threads2); i++)
		threads2[i] = CreateThread(NULL, 0, test_WaitOnAddress2_func, NULL, 0, NULL);

    Sleep(100);
    nthreads = ARRAY_SIZE(threads);
	nthreads2 = ARRAY_SIZE(threads2);
    address = ARRAY_SIZE(threads);
	address2 = ARRAY_SIZE(threads2);
    while (nthreads)
    {
		puts("WakeByAddressSingle(&address)");
        pWakeByAddressSingle(&address);
        val = WaitForMultipleObjects(nthreads, threads, FALSE, 2000);
        ok(val < WAIT_OBJECT_0 + nthreads, "got %u\n", val);
        CloseHandle(threads[val]);
        memmove(&threads[val], &threads[val+1], (nthreads - val - 1) * sizeof(threads[0]));
        nthreads--;
    }

	while (nthreads2)
	{
		puts("WakeByAddressSingle(&address2)");
		pWakeByAddressSingle(&address2);
		val = WaitForMultipleObjects(nthreads2, threads2, FALSE, 2000);
		ok(val < WAIT_OBJECT_0 + nthreads2, "got %u\n", val);
		CloseHandle(threads2[val]);
		memmove(&threads2[val], &threads2[val+1], (nthreads2 - val - 1) * sizeof(threads2[0]));
		nthreads2--;
	}

    ok(!address, "got unexpected value %s\n", wine_dbgstr_longlong(address));
}

int main(void)
{
    HMODULE hmod = LoadLibraryW(L"C:\\Program Files\\VxKex\\Kex64\\kernel33.dll");

	if (!hmod) {
		DWORD dwError = GetLastError();
		__debugbreak();
	}

    pWaitOnAddress       = (void *)GetProcAddress(hmod, "WaitOnAddress");
    pWakeByAddressAll    = (void *)GetProcAddress(hmod, "WakeByAddressAll");
    pWakeByAddressSingle = (void *)GetProcAddress(hmod, "WakeByAddressSingle");

    test_WaitOnAddress();
	return 0;
}
