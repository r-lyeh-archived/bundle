/*-----------------------------------------------------------*/
/* Block Sorting, Lossless Data Compression Library.         */
/* Platform specific functions and constants                 */
/*-----------------------------------------------------------*/

/*--

This file is a part of bsc and/or libbsc, a program and a library for
lossless, block-sorting data compression.

   Copyright (c) 2009-2012 Ilya Grebnov <ilya.grebnov@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

Please see the file LICENSE for full copyright information and file AUTHORS
for full list of contributors.

See also the bsc and libbsc web site:
  http://libbsc.com/ for more information.

--*/

#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "platform.h"

#include "../libbsc.h"

#if defined(_WIN32)
  #include <windows.h>
  SIZE_T g_LargePageSize = 0;
#endif

int bsc_platform_init(int features)
{

#if defined(_WIN32)

    if (features & LIBBSC_FEATURE_LARGEPAGES)
    {
        HANDLE hToken = 0;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            LUID luid;
            if (LookupPrivilegeValue(NULL, TEXT("SeLockMemoryPrivilege"), &luid))
            {
                TOKEN_PRIVILEGES tp;

                tp.PrivilegeCount = 1;
                tp.Privileges[0].Luid = luid;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), 0, 0);
            }

            CloseHandle(hToken);
        }

        {
            if (HMODULE hKernel = GetModuleHandle(TEXT("kernel32.dll")))
            {
                typedef SIZE_T (WINAPI * GetLargePageMinimumProcT)();

                GetLargePageMinimumProcT largePageMinimumProc = (GetLargePageMinimumProcT)GetProcAddress(hKernel, "GetLargePageMinimum");
                if (largePageMinimumProc != NULL)
                {
                    SIZE_T largePageSize = largePageMinimumProc();

                    if ((largePageSize & (largePageSize - 1)) != 0) largePageSize = 0;

                    g_LargePageSize = largePageSize;
                }
            }
        }
    }

#endif

    return LIBBSC_NO_ERROR;
}

void * bsc_malloc(size_t size)
{
#if defined(_WIN32)
    if ((g_LargePageSize != 0) && (size >= 256 * 1024))
    {
        void * address = VirtualAlloc(0, (size + g_LargePageSize - 1) & (~(g_LargePageSize - 1)), MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
        if (address != NULL) return address;
    }
    return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
#else
    return malloc(size);
#endif
}

void * bsc_zero_malloc(size_t size)
{
#if defined(_WIN32)
    if ((g_LargePageSize != 0) && (size >= 256 * 1024))
    {
        void * address = VirtualAlloc(0, (size + g_LargePageSize - 1) & (~(g_LargePageSize - 1)), MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
        if (address != NULL) return address;
    }
    return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
#else
    return calloc(1, size);
#endif
}

void bsc_free(void * address)
{
#if defined(_WIN32)
    VirtualFree(address, 0, MEM_RELEASE);
#else
    free(address);
#endif
}

/*-----------------------------------------------------------*/
/* End                                          platform.cpp */
/*-----------------------------------------------------------*/
