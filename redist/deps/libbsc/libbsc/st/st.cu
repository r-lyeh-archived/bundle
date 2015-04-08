/*-----------------------------------------------------------*/
/* Block Sorting, Lossless Data Compression Library.         */
/* Sort Transform (GPU version)                              */
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

/*--

Sort Transform is patented by Michael Schindler under US patent 6,199,064.
However for research purposes this algorithm is included in this software.
So if you are of the type who should worry about this (making money) worry away.
The author shall have no liability with respect to the infringement of
copyrights, trade secrets or any patents by this software. In no event will
the author be liable for any lost revenue or profits or other special,
indirect and consequential damages.

Sort Transform is disabled by default and can be enabled by defining the
preprocessor macro LIBBSC_SORT_TRANSFORM_SUPPORT at compile time.

--*/

#if defined(LIBBSC_SORT_TRANSFORM_SUPPORT) && defined(LIBBSC_CUDA_SUPPORT)

#if defined(_MSC_VER)
  #pragma warning(disable : 4267)
#endif

#include <stdlib.h>
#include <memory.h>

#include "st.cuh"

#include "../libbsc.h"
#include "../platform/platform.h"

#include <cuda_runtime_api.h>
#include <device_functions.h>

#include "b40c/radix_sort/enactor.cuh"
#include "b40c/util/multi_buffer.cuh"

#ifdef LIBBSC_OPENMP

omp_lock_t cuda_lock;
int bsc_st_cuda_init(int features)
{
    omp_init_lock(&cuda_lock);
    return LIBBSC_NO_ERROR;
}

#else

int bsc_st_cuda_init(int features)
{
    return LIBBSC_NO_ERROR;
}

#endif

#ifndef __CUDA_ARCH__
  #define CUDA_DEVICE_ARCH              0
#else
  #define CUDA_DEVICE_ARCH              __CUDA_ARCH__
#endif

#define CUDA_DEVICE_PADDING             1024
#define CUDA_NUM_THREADS_IN_BLOCK       192

#define CUDA_CTA_OCCUPANCY_SM30         10
#define CUDA_CTA_OCCUPANCY_SM20         8
#define CUDA_CTA_OCCUPANCY_SM12         5
#define CUDA_CTA_OCCUPANCY_SM10         4
#define CUDA_CTA_OCCUPANCY(v)           (v >= 300 ? CUDA_CTA_OCCUPANCY_SM30 : v >= 200 ? CUDA_CTA_OCCUPANCY_SM20 : v >= 120 ? CUDA_CTA_OCCUPANCY_SM12 : CUDA_CTA_OCCUPANCY_SM10)

cudaError_t bsc_cuda_safe_call(const char * filename, int line, cudaError_t result, cudaError_t status = cudaSuccess)
{
    if (result != cudaSuccess)
    {
		fprintf(stderr, "\n%s(%d): bsc_cuda_safe_call failed %d: '%s'.", filename, line, result, cudaGetErrorString(result));
		fflush(stderr);
    }

    return result != cudaSuccess ? result : status;
}

__global__ __launch_bounds__(CUDA_NUM_THREADS_IN_BLOCK, CUDA_CTA_OCCUPANCY(CUDA_DEVICE_ARCH))
void bsc_st567_encode_cuda_presort(unsigned char * RESTRICT T_device, unsigned long long * RESTRICT K_device, int n)
{
    __shared__ unsigned int staging[1 + CUDA_NUM_THREADS_IN_BLOCK + 7];

    unsigned int * RESTRICT thread_staging = &staging[threadIdx.x];
    for (int grid_size = gridDim.x * CUDA_NUM_THREADS_IN_BLOCK, block_start = blockIdx.x * CUDA_NUM_THREADS_IN_BLOCK; block_start < n; block_start += grid_size)
    {
        int index = block_start + threadIdx.x;

        {
                                  thread_staging[1                            ] = T_device[index                            ];
            if (threadIdx.x < 7 ) thread_staging[1 + CUDA_NUM_THREADS_IN_BLOCK] = T_device[index + CUDA_NUM_THREADS_IN_BLOCK]; else
            if (threadIdx.x == 7) thread_staging[-7                           ] = T_device[index - 8                        ];

            syncthreads();
        }

        {
            #if CUDA_DEVICE_ARCH >= 200
                unsigned int lo = __byte_perm(thread_staging[4], thread_staging[5], 0x0411) | __byte_perm(thread_staging[6], thread_staging[7], 0x1104);
                unsigned int hi = __byte_perm(thread_staging[0], thread_staging[1], 0x0411) | __byte_perm(thread_staging[2], thread_staging[3], 0x1104);
            #else
                unsigned int lo = (thread_staging[4] << 24) | (thread_staging[5] << 16) | (thread_staging[6] << 8) | thread_staging[7];
                unsigned int hi = (thread_staging[0] << 24) | (thread_staging[1] << 16) | (thread_staging[2] << 8) | thread_staging[3];
            #endif

            K_device[index] = (((unsigned long long)hi) << 32) | ((unsigned long long)lo);

            syncthreads();
        }
    }
}

__global__ __launch_bounds__(CUDA_NUM_THREADS_IN_BLOCK, CUDA_CTA_OCCUPANCY(CUDA_DEVICE_ARCH))
void bsc_st8_encode_cuda_presort(unsigned char * RESTRICT T_device, unsigned long long * RESTRICT K_device, unsigned char * RESTRICT V_device, int n)
{
    __shared__ unsigned int staging[1 + CUDA_NUM_THREADS_IN_BLOCK + 8];

    unsigned int * RESTRICT thread_staging = &staging[threadIdx.x];
    for (int grid_size = gridDim.x * CUDA_NUM_THREADS_IN_BLOCK, block_start = blockIdx.x * CUDA_NUM_THREADS_IN_BLOCK; block_start < n; block_start += grid_size)
    {
        int index = block_start + threadIdx.x;

        {
                                  thread_staging[1                            ] = T_device[index                            ];
            if (threadIdx.x < 8 ) thread_staging[1 + CUDA_NUM_THREADS_IN_BLOCK] = T_device[index + CUDA_NUM_THREADS_IN_BLOCK]; else
            if (threadIdx.x == 8) thread_staging[-8                           ] = T_device[index - 9                        ];

            syncthreads();
        }

        {
            #if CUDA_DEVICE_ARCH >= 200
                unsigned int lo = __byte_perm(thread_staging[5], thread_staging[6], 0x0411) | __byte_perm(thread_staging[7], thread_staging[8], 0x1104);
                unsigned int hi = __byte_perm(thread_staging[1], thread_staging[2], 0x0411) | __byte_perm(thread_staging[3], thread_staging[4], 0x1104);
            #else
                unsigned int lo = (thread_staging[5] << 24) | (thread_staging[6] << 16) | (thread_staging[7] << 8) | thread_staging[8];
                unsigned int hi = (thread_staging[1] << 24) | (thread_staging[2] << 16) | (thread_staging[3] << 8) | thread_staging[4];
            #endif

            K_device[index] = (((unsigned long long)hi) << 32) | ((unsigned long long)lo); V_device[index] = thread_staging[0];

            syncthreads();
        }
    }
}

__global__ __launch_bounds__(CUDA_NUM_THREADS_IN_BLOCK, CUDA_CTA_OCCUPANCY(CUDA_DEVICE_ARCH))
void bsc_st567_encode_cuda_postsort(unsigned char * RESTRICT T_device, unsigned long long * RESTRICT K_device, int n, unsigned long long lookup, int * RESTRICT I_device)
{
    int min_index = n;
    for (int grid_size = gridDim.x * CUDA_NUM_THREADS_IN_BLOCK, block_start = blockIdx.x * CUDA_NUM_THREADS_IN_BLOCK; block_start < n; block_start += grid_size)
    {
        int index = block_start + threadIdx.x;
        {
            unsigned long long value = K_device[index];
            {
                if (value == lookup && index < min_index) min_index = index;
                T_device[index] = (unsigned char)(value >> 56);
            }
        }
    }

    syncthreads(); if (min_index != n) atomicMin(I_device, min_index);
}

__global__ __launch_bounds__(CUDA_NUM_THREADS_IN_BLOCK, CUDA_CTA_OCCUPANCY(CUDA_DEVICE_ARCH))
void bsc_st8_encode_cuda_postsort(unsigned long long * RESTRICT K_device, int n, unsigned long long lookup, int * RESTRICT I_device)
{
    int min_index = n;
    for (int grid_size = gridDim.x * CUDA_NUM_THREADS_IN_BLOCK, block_start = blockIdx.x * CUDA_NUM_THREADS_IN_BLOCK; block_start < n; block_start += grid_size)
    {
        int index = block_start + threadIdx.x;
        {
            if (K_device[index] == lookup && index < min_index) min_index = index;
        }
    }

    syncthreads(); if (min_index != n) atomicMin(I_device, min_index);
}

int bsc_st567_encode_cuda(unsigned char * T, unsigned char * T_device, int n, int num_blocks, int k)
{
    int index = LIBBSC_GPU_NOT_ENOUGH_MEMORY;
    {
        unsigned long long * K_device = NULL;
        unsigned long long * K_device_sorted = NULL;

        if (bsc_cuda_safe_call(__FILE__, __LINE__, cudaMalloc((void **)&K_device, 2 * (n + 2 * CUDA_DEVICE_PADDING) * sizeof(unsigned long long))) == cudaSuccess)
        {
            index = LIBBSC_GPU_ERROR;

            bsc_st567_encode_cuda_presort<<<num_blocks, CUDA_NUM_THREADS_IN_BLOCK>>>(T_device, K_device, n);

            cudaError_t status = cudaSuccess;
            {
                b40c::util::DoubleBuffer<unsigned long long> storage;
                {
                    storage.d_keys[storage.selector ^ 0] = K_device;
                    storage.d_keys[storage.selector ^ 1] = K_device + ((n + 2 * CUDA_DEVICE_PADDING) / CUDA_DEVICE_PADDING) * CUDA_DEVICE_PADDING;
                }

                {
                    b40c::radix_sort::Enactor enactor;
                    if (k == 5) status = enactor.Sort<b40c::radix_sort::LARGE_PROBLEM, 40, 16>(storage, n);
                    if (k == 6) status = enactor.Sort<b40c::radix_sort::LARGE_PROBLEM, 48,  8>(storage, n);
                    if (k == 7) status = enactor.Sort<b40c::radix_sort::LARGE_PROBLEM, 56,  0>(storage, n);

                    K_device_sorted = storage.d_keys[storage.selector];
                }
            }

            if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
            {
                unsigned long long lookup;
                {
                    unsigned int lo = (T[3    ] << 24) | (T[4] << 16) | (T[5] << 8) | T[6];
                    unsigned int hi = (T[n - 1] << 24) | (T[0] << 16) | (T[1] << 8) | T[2];

                    lookup = (((unsigned long long)hi) << 32) | ((unsigned long long)lo);

                    status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(T_device - sizeof(int), &n, sizeof(int), cudaMemcpyHostToDevice), status);
                }

                if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
                {
                    bsc_st567_encode_cuda_postsort<<<num_blocks, CUDA_NUM_THREADS_IN_BLOCK>>>(T_device, K_device_sorted, n, lookup, (int *)(T_device - sizeof(int)));

                    status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaFree(K_device), status);

                    if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
                    {
                        status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(T_device + n, T_device - sizeof(int), sizeof(int), cudaMemcpyDeviceToDevice), status);
                        status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(T, T_device, n + sizeof(int), cudaMemcpyDeviceToHost), status);
                    }

                    if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
                    {
                        index = *(int *)(T + n);
                    }

                    return index;
                }
            }
            cudaFree(K_device);
        }
    }

    return index;
}

int bsc_st8_encode_cuda(unsigned char * T, unsigned char * T_device, int n, int num_blocks)
{
    int index = LIBBSC_GPU_NOT_ENOUGH_MEMORY;
    {
        unsigned char * V_device = NULL;
        unsigned char * V_device_sorted = NULL;

        if (bsc_cuda_safe_call(__FILE__, __LINE__, cudaMalloc((void **)&V_device, 2 * (n + 2 * CUDA_DEVICE_PADDING) * sizeof(unsigned char))) == cudaSuccess)
        {
            unsigned long long * K_device = NULL;
            unsigned long long * K_device_sorted = NULL;

            if (bsc_cuda_safe_call(__FILE__, __LINE__, cudaMalloc((void **)&K_device, 2 * (n + 2 * CUDA_DEVICE_PADDING) * sizeof(unsigned long long))) == cudaSuccess)
            {
                index = LIBBSC_GPU_ERROR;

                bsc_st8_encode_cuda_presort<<<num_blocks, CUDA_NUM_THREADS_IN_BLOCK>>>(T_device, K_device, V_device, n);

                cudaError_t status = cudaSuccess;
                {
                    b40c::util::DoubleBuffer<unsigned long long, unsigned char> storage;
                    {
                        storage.d_keys  [storage.selector ^ 0] = K_device;
                        storage.d_keys  [storage.selector ^ 1] = K_device + ((n + 2 * CUDA_DEVICE_PADDING) / CUDA_DEVICE_PADDING) * CUDA_DEVICE_PADDING;
                        storage.d_values[storage.selector ^ 0] = V_device;
                        storage.d_values[storage.selector ^ 1] = V_device + ((n + 2 * CUDA_DEVICE_PADDING) / CUDA_DEVICE_PADDING) * CUDA_DEVICE_PADDING;
                    }

                    {
                        b40c::radix_sort::Enactor enactor;
                        
                        status = enactor.Sort(storage, n);

                        K_device_sorted = storage.d_keys[storage.selector];
                        V_device_sorted = storage.d_values[storage.selector];
                    }
                }

                if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
                {
                    unsigned long long lookup;
                    {
                        unsigned int lo = (T[4] << 24) | (T[5] << 16) | (T[6] << 8) | T[7];
                        unsigned int hi = (T[0] << 24) | (T[1] << 16) | (T[2] << 8) | T[3];

                        lookup = (((unsigned long long)hi) << 32) | ((unsigned long long)lo);

                        status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(V_device_sorted + ((n + sizeof(int) - 1) / sizeof(int)) * sizeof(int), &n, sizeof(int), cudaMemcpyHostToDevice), status);
                    }

                    if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
                    {
                        bsc_st8_encode_cuda_postsort<<<num_blocks, CUDA_NUM_THREADS_IN_BLOCK>>>(K_device_sorted, n, lookup, (int *)(V_device_sorted + ((n + sizeof(int) - 1) / sizeof(int)) * sizeof(int)));

                        if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
                        {
                            status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(T, V_device_sorted, n + 2 * sizeof(int), cudaMemcpyDeviceToHost), status);
                        }

                        status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaFree(K_device), status);
                        status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaFree(V_device), status);

                        if (bsc_cuda_safe_call(__FILE__, __LINE__, status) == cudaSuccess)
                        {
                            index = *(int *)(T + ((n + sizeof(int) - 1) / sizeof(int)) * sizeof(int));
                        }

                        return index;
                    }
                }
                cudaFree(K_device);
            }
            cudaFree(V_device);
        }
    }

    return index;
}

int bsc_st_encode_cuda(unsigned char * T, int n, int k, int features)
{
    if ((T == NULL) || (n < 0)) return LIBBSC_BAD_PARAMETER;
    if ((k < 5) || (k > 8))     return LIBBSC_BAD_PARAMETER;
    if (n <= 1)                 return 0;

    int num_blocks = 1;
    {
        cudaDeviceProp deviceProperties;
        {
            int deviceId; if (cudaGetDevice(&deviceId) != cudaSuccess || cudaGetDeviceProperties(&deviceProperties, deviceId) != cudaSuccess)
            {
                return LIBBSC_GPU_NOT_SUPPORTED;
            }
        }

        if (deviceProperties.major * 10 + deviceProperties.minor <= 10) return LIBBSC_GPU_NOT_SUPPORTED;
        num_blocks = CUDA_CTA_OCCUPANCY(deviceProperties.major * 100 + deviceProperties.minor * 10) * deviceProperties.multiProcessorCount;

        if (num_blocks > ((n + CUDA_NUM_THREADS_IN_BLOCK - 1) / CUDA_NUM_THREADS_IN_BLOCK)) num_blocks = (n + CUDA_NUM_THREADS_IN_BLOCK - 1) / CUDA_NUM_THREADS_IN_BLOCK;
        if (num_blocks <= 0) num_blocks = 1;
    }

    #ifdef LIBBSC_OPENMP
        omp_set_lock(&cuda_lock);
    #endif

    int index = LIBBSC_GPU_NOT_ENOUGH_MEMORY;
    {
        unsigned char * T_device = NULL;
        if (cudaMalloc((void **)&T_device, n + 2 * CUDA_DEVICE_PADDING) == cudaSuccess)
        {
            index = LIBBSC_GPU_ERROR;

            cudaError_t status = cudaSuccess;
            status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(T_device + CUDA_DEVICE_PADDING    , T                             , n                  , cudaMemcpyHostToDevice  ), status);
            status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(T_device + CUDA_DEVICE_PADDING + n, T_device + CUDA_DEVICE_PADDING, CUDA_DEVICE_PADDING, cudaMemcpyDeviceToDevice), status);
            status = bsc_cuda_safe_call(__FILE__, __LINE__, cudaMemcpy(T_device                          , T_device + n                  , CUDA_DEVICE_PADDING, cudaMemcpyDeviceToDevice), status);

            if (status == cudaSuccess)
            {
                if (k >= 5 && k <= 7) index = bsc_st567_encode_cuda(T, T_device + CUDA_DEVICE_PADDING, n, num_blocks, k);
                if (k == 8)           index = bsc_st8_encode_cuda  (T, T_device + CUDA_DEVICE_PADDING, n, num_blocks   );
            }

            cudaFree(T_device);
        }
    }

    #ifdef LIBBSC_OPENMP
        omp_unset_lock(&cuda_lock);
    #endif

    return index;
}

#endif

/*-----------------------------------------------------------*/
/* End                                                 st.cu */
/*-----------------------------------------------------------*/
