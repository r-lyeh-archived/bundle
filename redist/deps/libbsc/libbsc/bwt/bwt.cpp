/*-----------------------------------------------------------*/
/* Block Sorting, Lossless Data Compression Library.         */
/* Burrows Wheeler Transform                                 */
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

#include "bwt.h"

#include "../platform/platform.h"
#include "../libbsc.h"

#include "divsufsort/divsufsort.h"

int bsc_bwt_encode(unsigned char * T, int n, unsigned char * num_indexes, int * indexes, int features)
{
    int index = divbwt(T, T, NULL, n, num_indexes, indexes, features & LIBBSC_FEATURE_MULTITHREADING);
    switch (index)
    {
        case -1 : return LIBBSC_BAD_PARAMETER;
        case -2 : return LIBBSC_NOT_ENOUGH_MEMORY;
    }
    return index;
}

static int bsc_unbwt_mergedTL_serial(unsigned char * RESTRICT T, unsigned int * RESTRICT P, int n, int index)
{
    unsigned int bucket[ALPHABET_SIZE];

    memset(bucket, 0, ALPHABET_SIZE * sizeof(unsigned int));

    for (int i = 0; i < index; ++i)
    {
        unsigned char c = T[i];
        P[i] = ((bucket[c]++) << 8) | c;
    }
    for (int i = index; i < n; ++i)
    {
        unsigned char c = T[i];
        P[i + 1] = ((bucket[c]++) << 8) | c;
    }

    for (int sum = 1, i = 0; i < ALPHABET_SIZE; ++i)
    {
        int tmp = sum; sum += bucket[i]; bucket[i] = tmp;
    }

    for (int p = 0, i = n - 1; i >= 0; --i)
    {
        unsigned int  u = P[p];
        unsigned char c = u & 0xff;
        T[i] = c; p = (u >> 8) + bucket[c];
    }

    return LIBBSC_NO_ERROR;
}

#define BWT_NUM_FASTBITS (17)

static int bsc_unbwt_biPSI_serial(unsigned char * RESTRICT T, unsigned int * RESTRICT P, int n, int index)
{
    if (int * RESTRICT bucket = (int *)bsc_zero_malloc(ALPHABET_SIZE * ALPHABET_SIZE * sizeof(int)))
    {
        if (unsigned short * RESTRICT fastbits = (unsigned short *)bsc_malloc((1 + (1 << BWT_NUM_FASTBITS)) * sizeof(unsigned short)))
        {
            int count[ALPHABET_SIZE]; memset(count, 0, ALPHABET_SIZE * sizeof(int));

            int shift = 0; while ((n >> shift) > (1 << BWT_NUM_FASTBITS)) shift++;

            for (int i = 0; i < n; ++i) count[T[i]]++;

            for (int sum = 1, c = 0; c < ALPHABET_SIZE; ++c)
            {
                int tmp = sum; sum += count[c]; count[c] = tmp;
                if (count[c] != sum)
                {
                    int * RESTRICT bucket_p = &bucket[c << 8];

                    int hi = index; if (sum < hi) hi = sum;
                    for (int i = count[c]; i < hi; ++i) bucket_p[T[i]]++;

                    int lo = index + 1; if (count[c] > lo) lo = count[c];
                    for (int i = lo; i < sum; ++i) bucket_p[T[i - 1]]++;
                }
            }

            int lastc = T[0];
            for (int v = 0, sum = 1, c = 0; c < ALPHABET_SIZE; ++c)
            {
                if (c == lastc) sum++;

                int * RESTRICT bucket_p = &bucket[c];
                for (int d = 0; d < ALPHABET_SIZE; ++d)
                {
                    int tmp = sum; sum += bucket_p[d << 8]; bucket_p[d << 8] = tmp;
                    if (bucket_p[d << 8] != sum)
                    {
                        for (; v <= ((sum - 1) >> shift); ++v) fastbits[v] = (c << 8) | d;
                    }
                }
            }

            for (int i = 0; i < index; ++i)
            {
                unsigned char c = T[i];
                int           p = count[c]++;

                if (p < index) P[bucket[(c << 8) | T[p    ]]++] = i; else
                if (p > index) P[bucket[(c << 8) | T[p - 1]]++] = i;
            }

            for (int i = index; i < n; ++i)
            {
                unsigned char c = T[i];
                int           p = count[c]++;

                if (p < index) P[bucket[(c << 8) | T[p    ]]++] = i + 1; else
                if (p > index) P[bucket[(c << 8) | T[p - 1]]++] = i + 1;
            }

            for (int c = 0; c < ALPHABET_SIZE; ++c)
            {
                for (int d = 0; d < c; ++d)
                {
                    int tmp = bucket[(d << 8) | c]; bucket[(d << 8) | c] = bucket[(c << 8) | d]; bucket[(c << 8) | d] = tmp;
                }
            }

            for (int p = index, i = 1; i < n; i += 2)
            {
                int c = fastbits[p >> shift]; while (bucket[c] <= p) c++;
                T[i - 1] = (unsigned char)(c >> 8); T[i] = (unsigned char)(c & 0xff);
                p = P[p];
            }

            T[n - 1] = (unsigned char)lastc;

            bsc_free(fastbits); bsc_free(bucket);
            return LIBBSC_NO_ERROR;
        }
        bsc_free(bucket);
    };
    return LIBBSC_NOT_ENOUGH_MEMORY;
}

static int bsc_unbwt_reconstruct_serial(unsigned char * T, unsigned int * P, int n, int index)
{
    if (n < 3 * 1024 * 1024) return bsc_unbwt_mergedTL_serial(T, P, n, index);
    return bsc_unbwt_biPSI_serial(T, P, n, index);
}

#ifdef LIBBSC_OPENMP

static int bsc_unbwt_mergedTL_parallel(unsigned char * RESTRICT T, unsigned int * RESTRICT P, int n, int index, int * RESTRICT indexes)
{
    unsigned int bucket[ALPHABET_SIZE];

    memset(bucket, 0, ALPHABET_SIZE * sizeof(unsigned int));

    for (int i = 0; i < index; ++i)
    {
        unsigned char c = T[i];
        P[i] = ((bucket[c]++) << 8) | c;
    }
    for (int i = index; i < n; ++i)
    {
        unsigned char c = T[i];
        P[i + 1] = ((bucket[c]++) << 8) | c;
    }

    for (int sum = 1, i = 0; i < ALPHABET_SIZE; ++i)
    {
        int tmp = sum; sum += bucket[i]; bucket[i] = tmp;
    }

    int mod = n / 8;
    {
        mod |= mod >> 1;  mod |= mod >> 2;
        mod |= mod >> 4;  mod |= mod >> 8;
        mod |= mod >> 16; mod >>= 1; mod++;
    }

    int nBlocks = 1 + (n - 1) / mod;

    #pragma omp parallel for schedule(dynamic)
    for (int blockId = 0; blockId < nBlocks; ++blockId)
    {
        int p           = (blockId < nBlocks - 1) ? indexes[blockId] + 1    : 0;
        int blockStart  = (blockId < nBlocks - 1) ? mod * blockId + mod - 1 : n - 1;
        int blockEnd    = mod * blockId;

        for (int i = blockStart; i >= blockEnd; --i)
        {
            unsigned int  u = P[p];
            unsigned char c = u & 0xff;
            T[i] = c; p = (u >> 8) + bucket[c];
        }
    }

    return LIBBSC_NO_ERROR;
}

static int bsc_unbwt_biPSI_parallel(unsigned char * RESTRICT T, unsigned int * RESTRICT P, int n, int index, int * RESTRICT indexes)
{
    if (int * RESTRICT bucket = (int *)bsc_zero_malloc(ALPHABET_SIZE * ALPHABET_SIZE * sizeof(int)))
    {
        if (unsigned short * RESTRICT fastbits = (unsigned short *)bsc_malloc((1 + (1 << BWT_NUM_FASTBITS)) * sizeof(unsigned short)))
        {
            int count[ALPHABET_SIZE]; memset(count, 0, ALPHABET_SIZE * sizeof(int));

            int shift = 0; while ((n >> shift) > (1 << BWT_NUM_FASTBITS)) shift++;

            #pragma omp parallel
            {
                unsigned int count_local[ALPHABET_SIZE];

                memset(count_local, 0, ALPHABET_SIZE * sizeof(unsigned int));

                #pragma omp for schedule(static) nowait
                for (int i = 0; i < n; ++i) count_local[T[i]]++;

                #pragma omp critical
                for (int c = 0; c < ALPHABET_SIZE; ++c) count[c] += count_local[c];
            }

            for (int sum = 1, c = 0; c < ALPHABET_SIZE; ++c)
            {
                int tmp = sum; sum += count[c]; count[c] = tmp;
            }

            #pragma omp parallel for schedule(static, 1)
            for (int c = 0; c < ALPHABET_SIZE; ++c)
            {
                int start = count[c], end = (c + 1 < ALPHABET_SIZE) ? count[c + 1] : n + 1;
                if (start != end)
                {
                    int * RESTRICT bucket_p = &bucket[c << 8];

                    int hi = index; if (end < hi) hi = end;
                    for (int i = start; i < hi; ++i) bucket_p[T[i]]++;

                    int lo = index + 1; if (start > lo) lo = start;
                    for (int i = lo; i < end; ++i) bucket_p[T[i - 1]]++;
                }
            }

            int lastc = T[0];
            for (int v = 0, sum = 1, c = 0; c < ALPHABET_SIZE; ++c)
            {
                if (c == lastc) sum++;

                int * RESTRICT bucket_p = &bucket[c];
                for (int d = 0; d < ALPHABET_SIZE; ++d)
                {
                    int tmp = sum; sum += bucket_p[d << 8]; bucket_p[d << 8] = tmp;
                    if (bucket_p[d << 8] != sum)
                    {
                        for (; v <= ((sum - 1) >> shift); ++v) fastbits[v] = (c << 8) | d;
                    }
                }
            }

            for (int i = 0; i < index; ++i)
            {
                unsigned char c = T[i];
                int           p = count[c]++;

                if (p < index) P[bucket[(c << 8) | T[p    ]]++] = i; else
                if (p > index) P[bucket[(c << 8) | T[p - 1]]++] = i;
            }

            for (int i = index; i < n; ++i)
            {
                unsigned char c = T[i];
                int           p = count[c]++;

                if (p < index) P[bucket[(c << 8) | T[p    ]]++] = i + 1; else
                if (p > index) P[bucket[(c << 8) | T[p - 1]]++] = i + 1;
            }

            for (int c = 0; c < ALPHABET_SIZE; ++c)
            {
                for (int d = 0; d < c; ++d)
                {
                    int tmp = bucket[(d << 8) | c]; bucket[(d << 8) | c] = bucket[(c << 8) | d]; bucket[(c << 8) | d] = tmp;
                }
            }

            int mod = n / 8;
            {
                mod |= mod >> 1;  mod |= mod >> 2;
                mod |= mod >> 4;  mod |= mod >> 8;
                mod |= mod >> 16; mod >>= 1; mod++;
            }

            int nBlocks = 1 + (n - 1) / mod;

            #pragma omp parallel for schedule(dynamic)
            for (int blockId = 0; blockId < nBlocks; ++blockId)
            {
                int p           = (blockId > 0          ) ? indexes[blockId - 1] + 1    : index;
                int blockEnd    = (blockId < nBlocks - 1) ? mod * blockId + mod         : n;
                int blockStart  = mod * blockId;

                if (blockEnd != n) blockEnd++; else T[n - 1] = (unsigned char)lastc;

                for (int i = blockStart + 1; i < blockEnd; i += 2)
                {
                    int c = fastbits[p >> shift]; while (bucket[c] <= p) c++;
                    T[i - 1] = (unsigned char)(c >> 8); T[i] = (unsigned char)(c & 0xff);
                    p = P[p];
                }
            }

            bsc_free(fastbits); bsc_free(bucket);
            return LIBBSC_NO_ERROR;
        }
        bsc_free(bucket);
    };
    return LIBBSC_NOT_ENOUGH_MEMORY;
}

static int bsc_unbwt_reconstruct_parallel(unsigned char * T, unsigned int * P, int n, int index, int * indexes)
{
    if (n < 3 * 1024 * 1024) return bsc_unbwt_mergedTL_parallel(T, P, n, index, indexes);
    return bsc_unbwt_biPSI_parallel(T, P, n, index, indexes);
}

#endif

int bsc_bwt_decode(unsigned char * T, int n, int index, unsigned char num_indexes, int * indexes, int features)
{
    if ((T == NULL) || (n < 0) || (index <= 0) || (index > n))
    {
        return LIBBSC_BAD_PARAMETER;
    }
    if (n <= 1)
    {
        return LIBBSC_NO_ERROR;
    }
    if (unsigned int * P = (unsigned int *)bsc_malloc((n + 1) * sizeof(unsigned int)))
    {
        int result = LIBBSC_NO_ERROR;

#ifdef LIBBSC_OPENMP

        int mod = n / 8;
        {
            mod |= mod >> 1;  mod |= mod >> 2;
            mod |= mod >> 4;  mod |= mod >> 8;
            mod |= mod >> 16; mod >>= 1;
        }

        if ((features & LIBBSC_FEATURE_MULTITHREADING) && (n >= 64 * 1024) && (num_indexes == (unsigned char)((n - 1) / (mod + 1))) && (indexes != NULL))
        {
            result = bsc_unbwt_reconstruct_parallel(T, P, n, index, indexes);
        }
        else

#endif

        {
            result = bsc_unbwt_reconstruct_serial(T, P, n, index);
        }

        bsc_free(P);
        return result;
    };
    return LIBBSC_NOT_ENOUGH_MEMORY;
}

/*-----------------------------------------------------------*/
/* End                                               bwt.cpp */
/*-----------------------------------------------------------*/
