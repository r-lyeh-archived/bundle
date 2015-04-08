/*-----------------------------------------------------------*/
/* Block Sorting, Lossless Data Compression Library.         */
/* Interface to platform specific functions and constants    */
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

#ifndef _LIBBSC_PLATFORM_H
#define _LIBBSC_PLATFORM_H

#if defined(_OPENMP) && defined(LIBBSC_OPENMP_SUPPORT)
    #include <omp.h>
    #define LIBBSC_OPENMP
#endif

#if defined(__GNUC__)
    #define INLINE __inline__
#elif defined(_MSC_VER)
    #define INLINE __forceinline
#elif defined(__IBMC__)
    #define INLINE _Inline
#elif defined(__cplusplus)
    #define INLINE inline
#else
    #define INLINE /* */
#endif

#if defined(__GNUC__) && (__GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
    #define RESTRICT __restrict__
#elif defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)
    #define RESTRICT __restrict
#elif defined(_MSC_VER) && (_MSC_VER >= 1400)
    #define RESTRICT __restrict
#elif defined(__CUDACC__) && (CUDA_VERSION >= 3000)
    #define RESTRICT __restrict__
#else
    #define RESTRICT /* */
#endif

#define ALPHABET_SIZE (256)

#ifdef __cplusplus
extern "C" {
#endif

    /**
    * You should call this function before you call any of the other platform specific functions.
    * @param features   - the set of additional features.
    * @return LIBBSC_NO_ERROR if no error occurred, error code otherwise.
    */
    int bsc_platform_init(int features);

    /**
    * Allocates memory blocks.
    * @param size       - bytes to allocate.
    * @return a pointer to allocated space or NULL if there is insufficient memory available.
    */
    void * bsc_malloc(size_t size);

    /**
    * Allocates memory blocks and initializes all its bits to zero.
    * @param size       - bytes to allocate.
    * @return a pointer to allocated space or NULL if there is insufficient memory available.
    */
    void * bsc_zero_malloc(size_t size);

    /**
    * Deallocates or frees a memory block.
    * @param address    - previously allocated memory block to be freed.
    */
    void bsc_free(void * address);

#ifdef __cplusplus
}
#endif

#endif

/*-----------------------------------------------------------*/
/* End                                            platform.h */
/*-----------------------------------------------------------*/
