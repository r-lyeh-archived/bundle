/*-----------------------------------------------------------*/
/* Block Sorting, Lossless Data Compression Library.         */
/* Adler-32 checksum functions                               */
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
#include <memory.h>
#include <string.h>

#include "adler32.h"

#include "../platform/platform.h"
#include "../libbsc.h"

#define BASE 65521UL
#define NMAX 5552

#define DO1(buf, i) { sum1 += (buf)[i]; sum2 += sum1; }
#define DO2(buf, i) DO1(buf, i); DO1(buf, i + 1);
#define DO4(buf, i) DO2(buf, i); DO2(buf, i + 2);
#define DO8(buf, i) DO4(buf, i); DO4(buf, i + 4);
#define DO16(buf)   DO8(buf, 0); DO8(buf, 8);
#define MOD(a)      a %= BASE

unsigned int bsc_adler32(const unsigned char * T, int n, int features)
{
    unsigned int sum1 = 1;
    unsigned int sum2 = 0;

    while (n >= NMAX)
    {
        for (int i = 0; i < NMAX / 16; ++i)
        {
            DO16(T); T += 16;
        }
        MOD(sum1); MOD(sum2); n -= NMAX;
    }

    while (n >= 16)
    {
        DO16(T); T += 16; n -= 16;
    }

    while (n > 0)
    {
        DO1(T, 0); T += 1; n -= 1;
    }

    MOD(sum1); MOD(sum2);

    return sum1 | (sum2 << 16);
}

/*-----------------------------------------------------------*/
/* End                                           adler32.cpp */
/*-----------------------------------------------------------*/
