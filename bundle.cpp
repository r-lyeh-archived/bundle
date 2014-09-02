/** this is an amalgamated file. do not edit.
 */

#if 0
// lzham
#define LZHAM_NO_ZLIB_COMPATIBLE_NAMES
#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif
#include "deps/lzham/src/lzham_core.h"
#include "deps/lzham/src/lzham_lzbase.cpp"
#include "deps/lzham/src/lzham_lzcomp.cpp"
#include "deps/lzham/src/lzham_lzcomp_internal.cpp"
#include "deps/lzham/src/lzham_lzcomp_state.cpp"
#include "deps/lzham/src/lzham_match_accel.cpp"
#include "deps/lzham/src/lzham_pthreads_threading.cpp"
#include "deps/lzham/src/lzham_win32_threading.cpp"
#include "deps/lzham/src/lzham_assert.cpp"
#include "deps/lzham/src/lzham_checksum.cpp"
#include "deps/lzham/src/lzham_huffman_codes.cpp"
#include "deps/lzham/src/lzham_lzdecomp.cpp"
#include "deps/lzham/src/lzham_lzdecompbase.cpp"
#include "deps/lzham/src/lzham_platform.cpp"
#define sym_freq sym_freq2
#include "deps/lzham/src/lzham_polar_codes.cpp"
#undef sym_freq
#include "deps/lzham/src/lzham_prefix_coding.cpp"
#include "deps/lzham/src/lzham_symbol_codec.cpp"
#include "deps/lzham/src/lzham_timer.cpp"
#include "deps/lzham/src/lzham_vector.cpp"
#include "deps/lzham/lzhamlib/lzham_lib.cpp"
#include "deps/lzham/src/lzham_mem.cpp"
#endif

// headers
#include "bundle.hpp"
#if 0
#include "deps/lzp1/lzp1.hpp"
#endif

#line 3 "lz4.h"
#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

/**************************************
   Version
**************************************/
#define LZ4_VERSION_MAJOR    1    /* for major interface/format changes  */
#define LZ4_VERSION_MINOR    2    /* for minor interface/format changes  */
#define LZ4_VERSION_RELEASE  0    /* for tweaks, bug-fixes, or development */

/**************************************
   Tuning parameter
**************************************/
/*
 * LZ4_MEMORY_USAGE :
 * Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
 * Increasing memory usage improves compression ratio
 * Reduced memory usage can improve speed, due to cache effect
 * Default value is 14, for 16KB, which nicely fits into Intel x86 L1 cache
 */
#define LZ4_MEMORY_USAGE 14

/**************************************
   Simple Functions
**************************************/

int LZ4_compress        (const char* source, char* dest, int inputSize);
int LZ4_decompress_safe (const char* source, char* dest, int compressedSize, int maxOutputSize);

/*
LZ4_compress() :
	Compresses 'inputSize' bytes from 'source' into 'dest'.
	Destination buffer must be already allocated,
	and must be sized to handle worst cases situations (input data not compressible)
	Worst case size evaluation is provided by function LZ4_compressBound()
	inputSize : Max supported value is LZ4_MAX_INPUT_VALUE
	return : the number of bytes written in buffer dest
			 or 0 if the compression fails

LZ4_decompress_safe() :
	compressedSize : is obviously the source size
	maxOutputSize : is the size of the destination buffer, which must be already allocated.
	return : the number of bytes decoded in the destination buffer (necessarily <= maxOutputSize)
			 If the destination buffer is not large enough, decoding will stop and output an error code (<0).
			 If the source stream is detected malformed, the function will stop decoding and return a negative result.
			 This function is protected against buffer overflow exploits :
			 it never writes outside of output buffer, and never reads outside of input buffer.
			 Therefore, it is protected against malicious data packets.
*/

/*
Note :
	Should you prefer to explicitly allocate compression-table memory using your own allocation method,
	use the streaming functions provided below, simply reset the memory area between each call to LZ4_compress_continue()
*/

/**************************************
   Advanced Functions
**************************************/
#define LZ4_MAX_INPUT_SIZE        0x7E000000   /* 2 113 929 216 bytes */
#define LZ4_COMPRESSBOUND(isize)  ((unsigned int)(isize) > (unsigned int)LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize)/255) + 16)

/*
LZ4_compressBound() :
	Provides the maximum size that LZ4 may output in a "worst case" scenario (input data not compressible)
	primarily useful for memory allocation of output buffer.
	macro is also provided when result needs to be evaluated at compilation (such as stack memory allocation).

	isize  : is the input size. Max supported value is LZ4_MAX_INPUT_SIZE
	return : maximum output size in a "worst case" scenario
			 or 0, if input size is too large ( > LZ4_MAX_INPUT_SIZE)
*/
int LZ4_compressBound(int isize);

/*
LZ4_compress_limitedOutput() :
	Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
	If it cannot achieve it, compression will stop, and result of the function will be zero.
	This function never writes outside of provided output buffer.

	inputSize  : Max supported value is LZ4_MAX_INPUT_VALUE
	maxOutputSize : is the size of the destination buffer (which must be already allocated)
	return : the number of bytes written in buffer 'dest'
			 or 0 if the compression fails
*/
int LZ4_compress_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);

/*
LZ4_decompress_fast() :
	originalSize : is the original and therefore uncompressed size
	return : the number of bytes read from the source buffer (in other words, the compressed size)
			 If the source stream is malformed, the function will stop decoding and return a negative result.
			 Destination buffer must be already allocated. Its size must be a minimum of 'originalSize' bytes.
	note : This function is a bit faster than LZ4_decompress_safe()
		   It provides fast decompression and fully respect memory boundaries for properly formed compressed data.
		   It does not provide full protection against intentionnally modified data stream.
		   Use this function in a trusted environment (data to decode comes from a trusted source).
*/
int LZ4_decompress_fast (const char* source, char* dest, int originalSize);

/*
LZ4_decompress_safe_partial() :
	This function decompress a compressed block of size 'compressedSize' at position 'source'
	into output buffer 'dest' of size 'maxOutputSize'.
	The function tries to stop decompressing operation as soon as 'targetOutputSize' has been reached,
	reducing decompression time.
	return : the number of bytes decoded in the destination buffer (necessarily <= maxOutputSize)
	   Note : this number can be < 'targetOutputSize' should the compressed block to decode be smaller.
			 Always control how many bytes were decoded.
			 If the source stream is detected malformed, the function will stop decoding and return a negative result.
			 This function never writes outside of output buffer, and never reads outside of input buffer. It is therefore protected against malicious data packets
*/
int LZ4_decompress_safe_partial (const char* source, char* dest, int compressedSize, int targetOutputSize, int maxOutputSize);

/***********************************************
   Experimental Streaming Compression Functions
***********************************************/

#define LZ4_STREAMSIZE_U32 ((1 << (LZ4_MEMORY_USAGE-2)) + 8)
#define LZ4_STREAMSIZE     (LZ4_STREAMSIZE_U32 * sizeof(unsigned int))
/*
 * LZ4_stream_t
 * information structure to track an LZ4 stream.
 * important : set this structure content to zero before first use !
 */
typedef struct { unsigned int table[LZ4_STREAMSIZE_U32]; } LZ4_stream_t;

/*
 * If you prefer dynamic allocation methods,
 * LZ4_createStream
 * provides a pointer (void*) towards an initialized LZ4_stream_t structure.
 * LZ4_free just frees it.
 */
void* LZ4_createStream();
int   LZ4_free (void* LZ4_stream);

/*
 * LZ4_loadDict
 * Use this function to load a static dictionary into LZ4_stream.
 * Any previous data will be forgotten, only 'dictionary' will remain in memory.
 * Loading a size of 0 is allowed (same effect as init).
 * Return : 1 if OK, 0 if error
 */
int LZ4_loadDict (void* LZ4_stream, const char* dictionary, int dictSize);

/*
 * LZ4_compress_continue
 * Compress data block 'source', using blocks compressed before as dictionary to improve compression ratio
 * Previous data blocks are assumed to still be present at their previous location.
 */
int LZ4_compress_continue (void* LZ4_stream, const char* source, char* dest, int inputSize);

/*
 * LZ4_compress_limitedOutput_continue
 * Same as before, but also specify a maximum target compressed size (maxOutputSize)
 * If objective cannot be met, compression exits, and returns a zero.
 */
int LZ4_compress_limitedOutput_continue (void* LZ4_stream, const char* source, char* dest, int inputSize, int maxOutputSize);

/*
 * LZ4_saveDict
 * If previously compressed data block is not guaranteed to remain at its previous memory location
 * save it into a safe place (char* safeBuffer)
 * Note : you don't need to call LZ4_loadDict() afterwards,
 *        dictionary is immediately usable, you can therefore call again LZ4_compress_continue()
 * Return : 1 if OK, 0 if error
 * Note : any dictSize > 64 KB will be interpreted as 64KB.
 */
int LZ4_saveDict (void* LZ4_stream, char* safeBuffer, int dictSize);

/************************************************
  Experimental Streaming Decompression Functions
************************************************/

#define LZ4_STREAMDECODESIZE_U32 4
#define LZ4_STREAMDECODESIZE     (LZ4_STREAMDECODESIZE_U32 * sizeof(unsigned int))
/*
 * LZ4_streamDecode_t
 * information structure to track an LZ4 stream.
 * important : set this structure content to zero before first use !
 */
typedef struct { unsigned int table[LZ4_STREAMDECODESIZE_U32]; } LZ4_streamDecode_t;

/*
 * If you prefer dynamic allocation methods,
 * LZ4_createStreamDecode()
 * provides a pointer (void*) towards an initialized LZ4_streamDecode_t structure.
 * LZ4_free just frees it.
 */
void* LZ4_createStreamDecode();
int   LZ4_free (void* LZ4_stream);   /* yes, it's the same one as for compression */

/*
*_continue() :
	These decoding functions allow decompression of multiple blocks in "streaming" mode.
	Previously decoded blocks must still be available at the memory position where they were decoded.
	If it's not possible, save the relevant part of decoded data into a safe buffer,
	and indicate where it stands using LZ4_setDictDecode()
*/
int LZ4_decompress_safe_continue (void* LZ4_streamDecode, const char* source, char* dest, int compressedSize, int maxOutputSize);
int LZ4_decompress_fast_continue (void* LZ4_streamDecode, const char* source, char* dest, int originalSize);

/*
 * LZ4_setDictDecode
 * Use this function to instruct where to find the dictionary.
 * This function can be used to specify a static dictionary,
 * or to instruct where to find some previously decoded data saved into a different memory space.
 * Setting a size of 0 is allowed (same effect as no dictionary).
 * Return : 1 if OK, 0 if error
 */
int LZ4_setDictDecode (void* LZ4_streamDecode, const char* dictionary, int dictSize);

/*
Advanced decoding functions :
*_usingDict() :
	These decoding functions work the same as
	a combination of LZ4_setDictDecode() followed by LZ4_decompress_x_continue()
	all together into a single function call.
	It doesn't use nor update an LZ4_streamDecode_t structure.
*/
int LZ4_decompress_safe_usingDict (const char* source, char* dest, int compressedSize, int maxOutputSize, const char* dictStart, int dictSize);
int LZ4_decompress_fast_usingDict (const char* source, char* dest, int originalSize, const char* dictStart, int dictSize);

/**************************************
   Obsolete Functions
**************************************/
/*
Obsolete decompression functions
These function names are deprecated and should no longer be used.
They are only provided here for compatibility with older user programs.
- LZ4_uncompress is the same as LZ4_decompress_fast
- LZ4_uncompress_unknownOutputSize is the same as LZ4_decompress_safe
*/
int LZ4_uncompress (const char* source, char* dest, int outputSize);
int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize);

/* Obsolete functions for externally allocated state; use streaming interface instead */
int LZ4_sizeofState(void);
int LZ4_compress_withState               (void* state, const char* source, char* dest, int inputSize);
int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);

/* Obsolete streaming functions; use new streaming interface whenever possible */
void* LZ4_create (const char* inputBuffer);
int   LZ4_sizeofStreamState(void);
int   LZ4_resetStreamState(void* state, const char* inputBuffer);
char* LZ4_slideInputBuffer (void* state);

/* Obsolete streaming decoding functions */
int LZ4_decompress_safe_withPrefix64k (const char* source, char* dest, int compressedSize, int maxOutputSize);
int LZ4_decompress_fast_withPrefix64k (const char* source, char* dest, int originalSize);

#if defined (__cplusplus)
}
#endif



#line 3 "lz4hc.h"
#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

int LZ4_compressHC (const char* source, char* dest, int inputSize);
/*
LZ4_compressHC :
	return : the number of bytes in compressed buffer dest
			 or 0 if compression fails.
	note : destination buffer must be already allocated.
		To avoid any problem, size it to handle worst cases situations (input data not compressible)
		Worst case size evaluation is provided by function LZ4_compressBound() (see "lz4.h")
*/

int LZ4_compressHC_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);
/*
LZ4_compress_limitedOutput() :
	Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
	If it cannot achieve it, compression will stop, and result of the function will be zero.
	This function never writes outside of provided output buffer.

	inputSize  : Max supported value is 1 GB
	maxOutputSize : is maximum allowed size into the destination buffer (which must be already allocated)
	return : the number of output bytes written in buffer 'dest'
			 or 0 if compression fails.
*/

int LZ4_compressHC2 (const char* source, char* dest, int inputSize, int compressionLevel);
int LZ4_compressHC2_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);
/*
	Same functions as above, but with programmable 'compressionLevel'.
	Recommended values are between 4 and 9, although any value between 0 and 16 will work.
	'compressionLevel'==0 means use default 'compressionLevel' value.
	Values above 16 behave the same as 16.
	Equivalent variants exist for all other compression functions below.
*/

/* Note :
Decompression functions are provided within LZ4 source code (see "lz4.h") (BSD license)
*/

/**************************************
   Using an external allocation
**************************************/
int LZ4_sizeofStateHC(void);
int LZ4_compressHC_withStateHC               (void* state, const char* source, char* dest, int inputSize);
int LZ4_compressHC_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);

int LZ4_compressHC2_withStateHC              (void* state, const char* source, char* dest, int inputSize, int compressionLevel);
int LZ4_compressHC2_limitedOutput_withStateHC(void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);

/*
These functions are provided should you prefer to allocate memory for compression tables with your own allocation methods.
To know how much memory must be allocated for the compression tables, use :
int LZ4_sizeofStateHC();

Note that tables must be aligned for pointer (32 or 64 bits), otherwise compression will fail (return code 0).

The allocated memory can be provided to the compressions functions using 'void* state' parameter.
LZ4_compress_withStateHC() and LZ4_compress_limitedOutput_withStateHC() are equivalent to previously described functions.
They just use the externally allocated memory area instead of allocating their own (on stack, or on heap).
*/

/**************************************
   Streaming Functions
**************************************/
/* Note : these streaming functions still follows the older model */
void* LZ4_createHC (const char* inputBuffer);
int   LZ4_compressHC_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize);
int   LZ4_compressHC_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize);
char* LZ4_slideInputBufferHC (void* LZ4HC_Data);
int   LZ4_freeHC (void* LZ4HC_Data);

int   LZ4_compressHC2_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel);
int   LZ4_compressHC2_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);

/*
These functions allow the compression of dependent blocks, where each block benefits from prior 64 KB within preceding blocks.
In order to achieve this, it is necessary to start creating the LZ4HC Data Structure, thanks to the function :

void* LZ4_createHC (const char* inputBuffer);
The result of the function is the (void*) pointer on the LZ4HC Data Structure.
This pointer will be needed in all other functions.
If the pointer returned is NULL, then the allocation has failed, and compression must be aborted.
The only parameter 'const char* inputBuffer' must, obviously, point at the beginning of input buffer.
The input buffer must be already allocated, and size at least 192KB.
'inputBuffer' will also be the 'const char* source' of the first block.

All blocks are expected to lay next to each other within the input buffer, starting from 'inputBuffer'.
To compress each block, use either LZ4_compressHC_continue() or LZ4_compressHC_limitedOutput_continue().
Their behavior are identical to LZ4_compressHC() or LZ4_compressHC_limitedOutput(),
but require the LZ4HC Data Structure as their first argument, and check that each block starts right after the previous one.
If next block does not begin immediately after the previous one, the compression will fail (return 0).

When it's no longer possible to lay the next block after the previous one (not enough space left into input buffer), a call to :
char* LZ4_slideInputBufferHC(void* LZ4HC_Data);
must be performed. It will typically copy the latest 64KB of input at the beginning of input buffer.
Note that, for this function to work properly, minimum size of an input buffer must be 192KB.
==> The memory position where the next input data block must start is provided as the result of the function.

Compression can then resume, using LZ4_compressHC_continue() or LZ4_compressHC_limitedOutput_continue(), as usual.

When compression is completed, a call to LZ4_freeHC() will release the memory used by the LZ4HC Data Structure.
*/

int LZ4_sizeofStreamStateHC(void);
int LZ4_resetStreamStateHC(void* state, const char* inputBuffer);

/*
These functions achieve the same result as :
void* LZ4_createHC (const char* inputBuffer);

They are provided here to allow the user program to allocate memory using its own routines.

To know how much space must be allocated, use LZ4_sizeofStreamStateHC();
Note also that space must be aligned for pointers (32 or 64 bits).

Once space is allocated, you must initialize it using : LZ4_resetStreamStateHC(void* state, const char* inputBuffer);
void* state is a pointer to the space allocated.
It must be aligned for pointers (32 or 64 bits), and be large enough.
The parameter 'const char* inputBuffer' must, obviously, point at the beginning of input buffer.
The input buffer must be already allocated, and size at least 192KB.
'inputBuffer' will also be the 'const char* source' of the first block.

The same space can be re-used multiple times, just by initializing it each time with LZ4_resetStreamState().
return value of LZ4_resetStreamStateHC() must be 0 is OK.
Any other value means there was an error (typically, state is not aligned for pointers (32 or 64 bits)).
*/

#if defined (__cplusplus)
}
#endif


#line 3 "shoco.h"
#pragma once

#include <stddef.h>

size_t shoco_compress(const char * const in, size_t len, char * const out, size_t bufsize);
size_t shoco_decompress(const char * const in, size_t len, char * const out, size_t bufsize);


#line 3 "compress.h"
#ifndef __EASYLZMACOMPRESS_H__
#define __EASYLZMACOMPRESS_H__


#line 3 "common.h"
#ifndef __EASYLZMACOMMON_H__
#define __EASYLZMACOMMON_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* msft dll export gunk.  To build a DLL on windows, you
 * must define WIN32, EASYLZMA_SHARED, and EASYLZMA_BUILD.  To use a
 * DLL, you must define EASYLZMA_SHARED and WIN32 */
#if defined(WIN32) && defined(EASYLZMA_SHARED)
#  ifdef EASYLZMA_BUILD
#    define EASYLZMA_API __declspec(dllexport)
#  else
#    define EASYLZMA_API __declspec(dllimport)
#  endif
#else
#  define EASYLZMA_API
#endif

/** error codes */

/** no error */
#define ELZMA_E_OK                               0
/** bad parameters passed to an ELZMA function */
#define ELZMA_E_BAD_PARAMS                      10
/** could not initialize the encode with configured parameters. */
#define ELZMA_E_ENCODING_PROPERTIES_ERROR       11
/** an error occured during compression (XXX: be more specific) */
#define ELZMA_E_COMPRESS_ERROR                  12
/** currently unsupported lzma file format was specified*/
#define ELZMA_E_UNSUPPORTED_FORMAT              13
/** an error occured when reading input */
#define ELZMA_E_INPUT_ERROR                     14
/** an error occured when writing output */
#define ELZMA_E_OUTPUT_ERROR                    15
/** LZMA header couldn't be parsed */
#define ELZMA_E_CORRUPT_HEADER                  16
/** an error occured during decompression (XXX: be more specific) */
#define ELZMA_E_DECOMPRESS_ERROR                17
/** the input stream returns EOF before the decompression could complete */
#define ELZMA_E_INSUFFICIENT_INPUT              18
/** for formats which have an emebedded crc, this error would indicated that
 *  what came out was not what went in, i.e. data corruption */
#define ELZMA_E_CRC32_MISMATCH                  19
/** for formats which have an emebedded uncompressed content length,
 *  this error indicates that the amount we read was not what we expected */
#define ELZMA_E_SIZE_MISMATCH                   20

/** Supported file formats */
typedef enum {
	ELZMA_lzip, /**< the lzip format which includes a magic number and
				 *   CRC check */
	ELZMA_lzma  /**< the LZMA-Alone format, originally designed by
				 *   Igor Pavlov and in widespread use due to lzmautils,
				 *   lacking both aforementioned features of lzip */
/* XXX: future, potentially   ,
	ELZMA_xz
*/
} elzma_file_format;

/**
 * A callback invoked during elzma_[de]compress_run when the [de]compression
 * process has generated [de]compressed output.
 *
 * the size parameter indicates how much data is in buf to be written.
 * it is required that the write callback consume all data, and a return
 * value not equal to input size indicates and error.
 */
typedef size_t (*elzma_write_callback)(void *ctx, const void *buf,
									   size_t size);

/**
 * A callback invoked during elzma_[de]compress_run when the [de]compression
 * process requires more [un]compressed input.
 *
 * the size parameter is an in/out argument.  on input it indicates
 * the buffer size.  on output it indicates the amount of data read into
 * buf.  when *size is zero on output it indicates EOF.
 *
 * \returns the read callback should return nonzero on failure.
 */
typedef int (*elzma_read_callback)(void *ctx, void *buf,
								   size_t *size);

/**
 * A callback invoked during elzma_[de]compress_run to report progress
 * on the [de]compression.
 *
 * \returns the read callback should return nonzero on failure.
 */
typedef void (*elzma_progress_callback)(void *ctx, size_t complete,
										size_t total);

/** pointer to a malloc function, supporting client overriding memory
 *  allocation routines */
typedef void * (*elzma_malloc)(void *ctx, unsigned int sz);

/** pointer to a free function, supporting client overriding memory
 *  allocation routines */
typedef void (*elzma_free)(void *ctx, void * ptr);

#ifdef __cplusplus
};
#endif

#endif

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** suggested default values */
#define ELZMA_LC_DEFAULT 3
#define ELZMA_LP_DEFAULT 0
#define ELZMA_PB_DEFAULT 2
#define ELZMA_DICT_SIZE_DEFAULT_MAX (1 << 24)

/** an opaque handle to an lzma compressor */
typedef struct _elzma_compress_handle * elzma_compress_handle;

/**
 * Allocate a handle to an LZMA compressor object.
 */
elzma_compress_handle EASYLZMA_API elzma_compress_alloc();

/**
 * set allocation routines (optional, if not called malloc & free will
 * be used)
 */
void EASYLZMA_API elzma_compress_set_allocation_callbacks(
	elzma_compress_handle hand,
	elzma_malloc mallocFunc, void * mallocFuncContext,
	elzma_free freeFunc, void * freeFuncContext);

/**
 * Free all data associated with an LZMA compressor object.
 */
void EASYLZMA_API elzma_compress_free(elzma_compress_handle * hand);

/**
 * Set configuration paramters for a compression run.  If not called,
 * reasonable defaults will be used.
 */
int EASYLZMA_API elzma_compress_config(elzma_compress_handle hand,
									   unsigned char lc,
									   unsigned char lp,
									   unsigned char pb,
									   unsigned char level,
									   unsigned int dictionarySize,
									   elzma_file_format format,
									   unsigned long long uncompressedSize);

/**
 * Run compression
 */
int EASYLZMA_API elzma_compress_run(
	elzma_compress_handle hand,
	elzma_read_callback inputStream, void * inputContext,
	elzma_write_callback outputStream, void * outputContext,
	elzma_progress_callback progressCallback, void * progressContext);

/**
 * a heuristic utility routine to guess a dictionary size that gets near
 * optimal compression while reducing memory usage.
 * accepts a size in bytes, returns a proposed dictionary size
 */
unsigned int EASYLZMA_API elzma_get_dict_size(unsigned long long size);

#ifdef __cplusplus
};
#endif

#endif


#line 3 "decompress.h"
#ifndef __EASYLZMADECOMPRESS_H__
#define __EASYLZMADECOMPRESS_H__

#ifdef __cplusplus
extern "C" {
#endif

/** an opaque handle to an lzma decompressor */
typedef struct _elzma_decompress_handle * elzma_decompress_handle;

/**
 * Allocate a handle to an LZMA decompressor object.
 */
elzma_decompress_handle EASYLZMA_API elzma_decompress_alloc();

/**
 * set allocation routines (optional, if not called malloc & free will
 * be used)
 */
void EASYLZMA_API elzma_decompress_set_allocation_callbacks(
	elzma_decompress_handle hand,
	elzma_malloc mallocFunc, void * mallocFuncContext,
	elzma_free freeFunc, void * freeFuncContext);

/**
 * Free all data associated with an LZMA decompressor object.
 */
void EASYLZMA_API elzma_decompress_free(elzma_decompress_handle * hand);

/**
 * Perform decompression
 *
 * XXX: should the library automatically detect format by reading stream?
 *      currently it's based on data external to stream (such as extension
 *      or convention)
 */
int EASYLZMA_API elzma_decompress_run(
	elzma_decompress_handle hand,
	elzma_read_callback inputStream, void * inputContext,
	elzma_write_callback outputStream, void * outputContext,
	elzma_file_format format);

#ifdef __cplusplus
};
#endif

#endif

#ifdef NDEBUG
#undef NDEBUG
#endif

#line 3 "libzpaq.h"
#ifndef LIBZPAQ_H
#define LIBZPAQ_H

#ifndef DEBUG
#define NDEBUG 1
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

namespace libzpaq {

// 1, 2, 4, 8 byte unsigned integers
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

// Tables for parsing ZPAQL source code
extern const char* compname[256];    // list of ZPAQL component types
extern const int compsize[256];      // number of bytes to encode a component
extern const char* opcodelist[272];  // list of ZPAQL instructions

// Callback for error handling
extern void error(const char* msg);

// Virtual base classes for input and output
// get() and put() must be overridden to read or write 1 byte.
// read() and write() may be overridden to read or write n bytes more
// efficiently than calling get() or put() n times.
class Reader {
public:
  virtual int get() = 0;  // should return 0..255, or -1 at EOF
  virtual int read(char* buf, int n); // read to buf[n], return no. read
  virtual ~Reader() {}
};

class Writer {
public:
  virtual void put(int c) = 0;  // should output low 8 bits of c
  virtual void write(const char* buf, int n);  // write buf[n]
  virtual ~Writer() {}
};

// Read 16 bit little-endian number
int toU16(const char* p);

// An Array of T is cleared and aligned on a 64 byte address
//   with no constructors called. No copy or assignment.
// Array<T> a(n, ex=0);  - creates n<<ex elements of type T
// a[i] - index
// a(i) - index mod n, n must be a power of 2
// a.size() - gets n
template <typename T>
class Array {
  T *data;     // user location of [0] on a 64 byte boundary
  size_t n;    // user size
  int offset;  // distance back in bytes to start of actual allocation
  void operator=(const Array&);  // no assignment
  Array(const Array&);  // no copy
public:
  Array(size_t sz=0, int ex=0): data(0), n(0), offset(0) {
	resize(sz, ex);} // [0..sz-1] = 0
  void resize(size_t sz, int ex=0); // change size, erase content to zeros
  ~Array() {resize(0);}  // free memory
  size_t size() const {return n;}  // get size
  int isize() const {return int(n);}  // get size as an int
  T& operator[](size_t i) {assert(n>0 && i<n); return data[i];}
  T& operator()(size_t i) {assert(n>0 && (n&(n-1))==0); return data[i&(n-1)];}
};

// Change size to sz<<ex elements of 0
template<typename T>
void Array<T>::resize(size_t sz, int ex) {
  assert(size_t(-1)>0);  // unsigned type?
  while (ex>0) {
	if (sz>sz*2) error("Array too big");
	sz*=2, --ex;
  }
  if (n>0) {
	assert(offset>0 && offset<=64);
	assert((char*)data-offset);
	::free((char*)data-offset);
  }
  n=0;
  if (sz==0) return;
  n=sz;
  const size_t nb=128+n*sizeof(T);  // test for overflow
  if (nb<=128 || (nb-128)/sizeof(T)!=n) error("Array too big");
  data=(T*)::calloc(nb, 1);
  if (!data) n=0, error("Out of memory");
  offset=64-(((char*)data-(char*)0)&63);
  assert(offset>0 && offset<=64);
  data=(T*)((char*)data+offset);
}

//////////////////////////// SHA1 ////////////////////////////

// For computing SHA-1 checksums
class SHA1 {
public:
  void put(int c) {  // hash 1 byte
	U32& r=w[len0>>5&15];
	r=(r<<8)|(c&255);
	if (!(len0+=8)) ++len1;
	if ((len0&511)==0) process();
  }
  double size() const {return len0/8+len1*536870912.0;} // size in bytes
  uint64_t usize() const {return len0/8+(U64(len1)<<29);} // size in bytes
  const char* result();  // get hash and reset
  SHA1() {init();}
private:
  void init();      // reset, but don't clear hbuf
  U32 len0, len1;   // length in bits (low, high)
  U32 h[5];         // hash state
  U32 w[16];        // input buffer
  char hbuf[20];    // result
  void process();   // hash 1 block
};

//////////////////////////// SHA256 //////////////////////////

// For computing SHA-256 checksums
// http://en.wikipedia.org/wiki/SHA-2
class SHA256 {
public:
  void put(int c) {  // hash 1 byte
	unsigned& r=w[len0>>5&15];
	r=(r<<8)|(c&255);
	if (!(len0+=8)) ++len1;
	if ((len0&511)==0) process();
  }
  double size() const {return len0/8+len1*536870912.0;} // size in bytes
  uint64_t usize() const {return len0/8+(U64(len1)<<29);} //size in bytes
  const char* result();  // get hash and reset
  SHA256() {init();}
private:
  void init();           // reset, but don't clear hbuf
  unsigned len0, len1;   // length in bits (low, high)
  unsigned s[8];         // hash state
  unsigned w[16];        // input buffer
  char hbuf[32];         // result
  void process();        // hash 1 block
};

//////////////////////////// AES /////////////////////////////

// For encrypting with AES in CTR mode.
// The i'th 16 byte block is encrypted by XOR with AES(i)
// (i is big endian or MSB first, starting with 0).
class AES_CTR {
  U32 Te0[256], Te1[256], Te2[256], Te3[256], Te4[256]; // encryption tables
  U32 ek[60];  // round key
  int Nr;  // number of rounds (10, 12, 14 for AES 128, 192, 256)
  U32 iv0, iv1;  // first 8 bytes in CTR mode
public:
  AES_CTR(const char* key, int keylen, const char* iv=0);
	// Schedule: keylen is 16, 24, or 32, iv is 8 bytes or NULL
  void encrypt(U32 s0, U32 s1, U32 s2, U32 s3, unsigned char* ct);
  void encrypt(char* buf, int n, U64 offset);  // encrypt n bytes of buf
};

//////////////////////////// stretchKey //////////////////////

// Strengthen password pw[0..pwlen-1] and salt[0..saltlen-1]
// to produce key buf[0..buflen-1]. Uses O(n*r*p) time and 128*r*n bytes
// of memory. n must be a power of 2 and r <= 8.
void scrypt(const char* pw, int pwlen,
			const char* salt, int saltlen,
			int n, int r, int p, char* buf, int buflen);

// Generate a strong key out[0..31] key[0..31] and salt[0..31].
// Calls scrypt(key, 32, salt, 32, 16384, 8, 1, out, 32);
void stretchKey(char* out, const char* key, const char* salt);

//////////////////////////// ZPAQL ///////////////////////////

// Symbolic constants, instruction size, and names
typedef enum {NONE,CONS,CM,ICM,MATCH,AVG,MIX2,MIX,ISSE,SSE} CompType;
extern const int compsize[256];

// A ZPAQL machine COMP+HCOMP or PCOMP.
class ZPAQL {
public:
  ZPAQL();
  ~ZPAQL();
  void clear();           // Free memory, erase program, reset machine state
  void inith();           // Initialize as HCOMP to run
  void initp();           // Initialize as PCOMP to run
  double memory();        // Return memory requirement in bytes
  void run(U32 input);    // Execute with input
  int read(Reader* in2);  // Read header
  bool write(Writer* out2, bool pp); // If pp write PCOMP else HCOMP header
  int step(U32 input, int mode);  // Trace execution (defined externally)

  Writer* output;         // Destination for OUT instruction, or 0 to suppress
  SHA1* sha1;             // Points to checksum computer
  U32 H(int i) {return h(i);}  // get element of h

  void flush();           // write outbuf[0..bufptr-1] to output and sha1
  void outc(int c) {      // output byte c (0..255) or -1 at EOS
	if (c<0 || (outbuf[bufptr]=c, ++bufptr==outbuf.isize())) flush();
  }

  // ZPAQ1 block header
  Array<U8> header;   // hsize[2] hh hm ph pm n COMP (guard) HCOMP (guard)
  int cend;           // COMP in header[7...cend-1]
  int hbegin, hend;   // HCOMP/PCOMP in header[hbegin...hend-1]

private:
  // Machine state for executing HCOMP
  Array<U8> m;        // memory array M for HCOMP
  Array<U32> h;       // hash array H for HCOMP
  Array<U32> r;       // 256 element register array
  Array<char> outbuf; // output buffer
  int bufptr;         // number of bytes in outbuf
  U32 a, b, c, d;     // machine registers
  int f;              // condition flag
  int pc;             // program counter
  int rcode_size;     // length of rcode
  U8* rcode;          // JIT code for run()

  // Support code
  int assemble();  // put JIT code in rcode
  void init(int hbits, int mbits);  // initialize H and M sizes
  int execute();  // interpret 1 instruction, return 0 after HALT, else 1
  void run0(U32 input);  // default run() if not JIT
  void div(U32 x) {if (x) a/=x; else a=0;}
  void mod(U32 x) {if (x) a%=x; else a=0;}
  void swap(U32& x) {a^=x; x^=a; a^=x;}
  void swap(U8& x)  {a^=x; x^=a; a^=x;}
  void err();  // exit with run time error
};

///////////////////////// Component //////////////////////////

// A Component is a context model, indirect context model, match model,
// fixed weight mixer, adaptive 2 input mixer without or with current
// partial byte as context, adaptive m input mixer (without or with),
// or SSE (without or with).

struct Component {
  size_t limit;   // max count for cm
  size_t cxt;     // saved context
  size_t a, b, c; // multi-purpose variables
  Array<U32> cm;  // cm[cxt] -> p in bits 31..10, n in 9..0; MATCH index
  Array<U8> ht;   // ICM/ISSE hash table[0..size1][0..15] and MATCH buf
  Array<U16> a16; // MIX weights
  void init();    // initialize to all 0
  Component() {init();}
};

////////////////////////// StateTable ////////////////////////

// Next state table
class StateTable {
public:
  U8 ns[1024]; // state*4 -> next state if 0, if 1, n0, n1
  int next(int state, int y) {  // next state for bit y
	assert(state>=0 && state<256);
	assert(y>=0 && y<4);
	return ns[state*4+y];
  }
  int cminit(int state) {  // initial probability of 1 * 2^23
	assert(state>=0 && state<256);
	return ((ns[state*4+3]*2+1)<<22)/(ns[state*4+2]+ns[state*4+3]+1);
  }
  StateTable();
};

///////////////////////// Predictor //////////////////////////

// A predictor guesses the next bit
class Predictor {
public:
  Predictor(ZPAQL&);
  ~Predictor();
  void init();          // build model
  int predict();        // probability that next bit is a 1 (0..4095)
  void update(int y);   // train on bit y (0..1)
  int stat(int);        // Defined externally
  bool isModeled() {    // n>0 components?
	assert(z.header.isize()>6);
	return z.header[6]!=0;
  }
private:

  // Predictor state
  int c8;               // last 0...7 bits.
  int hmap4;            // c8 split into nibbles
  int p[256];           // predictions
  U32 h[256];           // unrolled copy of z.h
  ZPAQL& z;             // VM to compute context hashes, includes H, n
  Component comp[256];  // the model, includes P

  // Modeling support functions
  int predict0();       // default
  void update0(int y);  // default
  int dt2k[256];        // division table for match: dt2k[i] = 2^12/i
  int dt[1024];         // division table for cm: dt[i] = 2^16/(i+1.5)
  U16 squasht[4096];    // squash() lookup table
  short stretcht[32768];// stretch() lookup table
  StateTable st;        // next, cminit functions
  U8* pcode;            // JIT code for predict() and update()
  int pcode_size;       // length of pcode

  // reduce prediction error in cr.cm
  void train(Component& cr, int y) {
	assert(y==0 || y==1);
	U32& pn=cr.cm(cr.cxt);
	U32 count=pn&0x3ff;
	int error=y*32767-(cr.cm(cr.cxt)>>17);
	pn+=(error*dt[count]&-1024)+(count<cr.limit);
  }

  // x -> floor(32768/(1+exp(-x/64)))
  int squash(int x) {
	assert(x>=-2048 && x<=2047);
	return squasht[x+2048];
  }

  // x -> round(64*log((x+0.5)/(32767.5-x))), approx inverse of squash
  int stretch(int x) {
	assert(x>=0 && x<=32767);
	return stretcht[x];
  }

  // bound x to a 12 bit signed int
  int clamp2k(int x) {
	if (x<-2048) return -2048;
	else if (x>2047) return 2047;
	else return x;
  }

  // bound x to a 20 bit signed int
  int clamp512k(int x) {
	if (x<-(1<<19)) return -(1<<19);
	else if (x>=(1<<19)) return (1<<19)-1;
	else return x;
  }

  // Get cxt in ht, creating a new row if needed
  size_t find(Array<U8>& ht, int sizebits, U32 cxt);

  // Put JIT code in pcode
  int assemble_p();
};

//////////////////////////// Decoder /////////////////////////

// Decoder decompresses using an arithmetic code
class Decoder {
public:
  Reader* in;        // destination
  Decoder(ZPAQL& z);
  int decompress();  // return a byte or EOF
  int skip();        // skip to the end of the segment, return next byte
  void init();       // initialize at start of block
  int stat(int x) {return pr.stat(x);}
private:
  U32 low, high;     // range
  U32 curr;          // last 4 bytes of archive
  Predictor pr;      // to get p
  enum {BUFSIZE=1<<16};
  Array<char> buf;   // input buffer of size BUFSIZE bytes
	// of unmodeled data. buf[low..high-1] is input with curr
	// remaining in sub-block.
  int decode(int p); // return decoded bit (0..1) with prob. p (0..65535)
  void loadbuf();    // read unmodeled data into buf to EOS
};

/////////////////////////// PostProcessor ////////////////////

class PostProcessor {
  int state;   // input parse state: 0=INIT, 1=PASS, 2..4=loading, 5=POST
  int hsize;   // header size
  int ph, pm;  // sizes of H and M in z
public:
  ZPAQL z;     // holds PCOMP
  PostProcessor(): state(0), hsize(0), ph(0), pm(0) {}
  void init(int h, int m);  // ph, pm sizes of H and M
  int write(int c);  // Input a byte, return state
  int getState() const {return state;}
  void setOutput(Writer* out) {z.output=out;}
  void setSHA1(SHA1* sha1ptr) {z.sha1=sha1ptr;}
};

//////////////////////// Decompresser ////////////////////////

// For decompression and listing archive contents
class Decompresser {
public:
  Decompresser(): z(), dec(z), pp(), state(BLOCK), decode_state(FIRSTSEG) {}
  void setInput(Reader* in) {dec.in=in;}
  bool findBlock(double* memptr = 0);
  void hcomp(Writer* out2) {z.write(out2, false);}
  bool findFilename(Writer* = 0);
  void readComment(Writer* = 0);
  void setOutput(Writer* out) {pp.setOutput(out);}
  void setSHA1(SHA1* sha1ptr) {pp.setSHA1(sha1ptr);}
  bool decompress(int n = -1);  // n bytes, -1=all, return true until done
  bool pcomp(Writer* out2) {return pp.z.write(out2, true);}
  void readSegmentEnd(char* sha1string = 0);
  int stat(int x) {return dec.stat(x);}
private:
  ZPAQL z;
  Decoder dec;
  PostProcessor pp;
  enum {BLOCK, FILENAME, COMMENT, DATA, SEGEND} state;  // expected next
  enum {FIRSTSEG, SEG, SKIP} decode_state;  // which segment in block?
};

/////////////////////////// decompress() /////////////////////

void decompress(Reader* in, Writer* out);

//////////////////////////// Encoder /////////////////////////

// Encoder compresses using an arithmetic code
class Encoder {
public:
  Encoder(ZPAQL& z, int size=0):
	out(0), low(1), high(0xFFFFFFFF), pr(z) {}
  void init();
  void compress(int c);  // c is 0..255 or EOF
  int stat(int x) {return pr.stat(x);}
  Writer* out;  // destination
private:
  U32 low, high; // range
  Predictor pr;  // to get p
  Array<char> buf; // unmodeled input
  void encode(int y, int p); // encode bit y (0..1) with prob. p (0..65535)
};

//////////////////////////// Compiler ////////////////////////

// Input ZPAQL source code with args and store the compiled code
// in hz and pz and write pcomp_cmd to out2.

class Compiler {
public:
  Compiler(const char* in, int* args, ZPAQL& hz, ZPAQL& pz, Writer* out2);
private:
  const char* in;  // ZPAQL source code
  int* args;       // Array of up to 9 args, default NULL = all 0
  ZPAQL& hz;       // Output of COMP and HCOMP sections
  ZPAQL& pz;       // Output of PCOMP section
  Writer* out2;    // Output ... of "PCOMP ... ;"
  int line;        // Input line number for reporting errors
  int state;       // parse state: 0=space -1=word >0 (nest level)

  // Symbolic constants
  typedef enum {NONE,CONS,CM,ICM,MATCH,AVG,MIX2,MIX,ISSE,SSE,
	JT=39,JF=47,JMP=63,LJ=255,
	POST=256,PCOMP,END,IF,IFNOT,ELSE,ENDIF,DO,
	WHILE,UNTIL,FOREVER,IFL,IFNOTL,ELSEL,SEMICOLON} CompType;

  void syntaxError(const char* msg, const char* expected=0); // error()
  void next();                     // advance in to next token
  bool matchToken(const char* tok);// in==token?
  int rtoken(int low, int high);   // return token which must be in range
  int rtoken(const char* list[]);  // return token by position in list
  void rtoken(const char* s);      // return token which must be s
  int compile_comp(ZPAQL& z);      // compile either HCOMP or PCOMP

  // Stack of n elements
  class Stack {
	libzpaq::Array<U16> s;
	size_t top;
  public:
	Stack(int n): s(n), top(0) {}
	void push(const U16& x) {
	  if (top>=s.size()) error("IF or DO nested too deep");
	  s[top++]=x;
	}
	U16 pop() {
	  if (top<=0) error("unmatched IF or DO");
	  return s[--top];
	}
  };

  Stack if_stack, do_stack;
};

//////////////////////// Compressor //////////////////////////

class Compressor {
public:
  Compressor(): enc(z), in(0), state(INIT), verify(false) {}
  void setOutput(Writer* out) {enc.out=out;}
  void writeTag();
  void startBlock(int level);  // level=1,2,3
  void startBlock(const char* hcomp);     // ZPAQL byte code
  void startBlock(const char* config,     // ZPAQL source code
				  int* args,              // NULL or int[9] arguments
				  Writer* pcomp_cmd = 0); // retrieve preprocessor command
  void setVerify(bool v) {verify = v;}    // check postprocessing?
  void hcomp(Writer* out2) {z.write(out2, false);}
  bool pcomp(Writer* out2) {return pz.write(out2, true);}
  void startSegment(const char* filename = 0, const char* comment = 0);
  void setInput(Reader* i) {in=i;}
  void postProcess(const char* pcomp = 0, int len = 0);  // byte code
  bool compress(int n = -1);  // n bytes, -1=all, return true until done
  void endSegment(const char* sha1string = 0);
  char* endSegmentChecksum(int64_t* size = 0);
  int64_t getSize() {return sha1.usize();}
  const char* getChecksum() {return sha1.result();}
  void endBlock();
  int stat(int x) {return enc.stat(x);}
private:
  ZPAQL z, pz;  // model and test postprocessor
  Encoder enc;  // arithmetic encoder containing predictor
  Reader* in;   // input source
  SHA1 sha1;    // to test pz output
  char sha1result[20];  // sha1 output
  enum {INIT, BLOCK1, SEG1, BLOCK2, SEG2} state;
  bool verify;  // if true then test by postprocessing
};

/////////////////////////// compress() ///////////////////////

void compress(Reader* in, Writer* out, int level);

}  // namespace libzpaq

#endif  // LIBZPAQ_H


#if 1
// miniz
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
//#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
//#define MINIZ_HAS_64BIT_REGISTERS 1

#line 3 "miniz.c"
#ifndef MINIZ_HEADER_INCLUDED
#define MINIZ_HEADER_INCLUDED

#include <stdlib.h>

// Defines to completely disable specific portions of miniz.c:
// If all macros here are defined the only functionality remaining will be CRC-32, adler-32, tinfl, and tdefl.

// Define MINIZ_NO_STDIO to disable all usage and any functions which rely on stdio for file I/O.
//#define MINIZ_NO_STDIO

// If MINIZ_NO_TIME is specified then the ZIP archive functions will not be able to get the current time, or
// get/set file times, and the C run-time funcs that get/set times won't be called.
// The current downside is the times written to your archives will be from 1979.
//#define MINIZ_NO_TIME

// Define MINIZ_NO_ARCHIVE_APIS to disable all ZIP archive API's.
//#define MINIZ_NO_ARCHIVE_APIS

// Define MINIZ_NO_ARCHIVE_APIS to disable all writing related ZIP archive API's.
//#define MINIZ_NO_ARCHIVE_WRITING_APIS

// Define MINIZ_NO_ZLIB_APIS to remove all ZLIB-style compression/decompression API's.
//#define MINIZ_NO_ZLIB_APIS

// Define MINIZ_NO_ZLIB_COMPATIBLE_NAME to disable zlib names, to prevent conflicts against stock zlib.
//#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES

// Define MINIZ_NO_MALLOC to disable all calls to malloc, free, and realloc.
// Note if MINIZ_NO_MALLOC is defined then the user must always provide custom user alloc/free/realloc
// callbacks to the zlib and archive API's, and a few stand-alone helper API's which don't provide custom user
// functions (such as tdefl_compress_mem_to_heap() and tinfl_decompress_mem_to_heap()) won't work.
//#define MINIZ_NO_MALLOC

#if defined(__TINYC__) && (defined(__linux) || defined(__linux__))
  // TODO: Work around "error: include file 'sys\utime.h' when compiling with tcc on Linux
  #define MINIZ_NO_TIME
#endif

#if !defined(MINIZ_NO_TIME) && !defined(MINIZ_NO_ARCHIVE_APIS)
  #include <time.h>
#endif

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__i386) || defined(__i486__) || defined(__i486) || defined(i386) || defined(__ia64__) || defined(__x86_64__)
// MINIZ_X86_OR_X64_CPU is only used to help set the below macros.
#define MINIZ_X86_OR_X64_CPU 1
#endif

#if (__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__) || MINIZ_X86_OR_X64_CPU
// Set MINIZ_LITTLE_ENDIAN to 1 if the processor is little endian.
#define MINIZ_LITTLE_ENDIAN 1
#endif

#if MINIZ_X86_OR_X64_CPU
// Set MINIZ_USE_UNALIGNED_LOADS_AND_STORES to 1 on CPU's that permit efficient integer loads and stores from unaligned addresses.
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
#endif

#if defined(_M_X64) || defined(_WIN64) || defined(__MINGW64__) || defined(_LP64) || defined(__LP64__) || defined(__ia64__) || defined(__x86_64__)
// Set MINIZ_HAS_64BIT_REGISTERS to 1 if operations on 64-bit integers are reasonably fast (and don't involve compiler generated calls to helper functions).
#define MINIZ_HAS_64BIT_REGISTERS 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ------------------- zlib-style API Definitions.

// For more compatibility with zlib, miniz.c uses unsigned long for some parameters/struct members. Beware: mz_ulong can be either 32 or 64-bits!
typedef unsigned long mz_ulong;

// mz_free() internally uses the MZ_FREE() macro (which by default calls free() unless you've modified the MZ_MALLOC macro) to release a block allocated from the heap.
void mz_free(void *p);

#define MZ_ADLER32_INIT (1)
// mz_adler32() returns the initial adler-32 value to use when called with ptr==NULL.
mz_ulong mz_adler32(mz_ulong adler, const unsigned char *ptr, size_t buf_len);

#define MZ_CRC32_INIT (0)
// mz_crc32() returns the initial CRC-32 value to use when called with ptr==NULL.
mz_ulong mz_crc32(mz_ulong crc, const unsigned char *ptr, size_t buf_len);

// Compression strategies.
enum { MZ_DEFAULT_STRATEGY = 0, MZ_FILTERED = 1, MZ_HUFFMAN_ONLY = 2, MZ_RLE = 3, MZ_FIXED = 4 };

// Method
#define MZ_DEFLATED 8

#ifndef MINIZ_NO_ZLIB_APIS

// Heap allocation callbacks.
// Note that mz_alloc_func parameter types purpsosely differ from zlib's: items/size is size_t, not unsigned long.
typedef void *(*mz_alloc_func)(void *opaque, size_t items, size_t size);
typedef void (*mz_free_func)(void *opaque, void *address);
typedef void *(*mz_realloc_func)(void *opaque, void *address, size_t items, size_t size);

#define MZ_VERSION          "9.1.15"
#define MZ_VERNUM           0x91F0
#define MZ_VER_MAJOR        9
#define MZ_VER_MINOR        1
#define MZ_VER_REVISION     15
#define MZ_VER_SUBREVISION  0

// Flush values. For typical usage you only need MZ_NO_FLUSH and MZ_FINISH. The other values are for advanced use (refer to the zlib docs).
enum { MZ_NO_FLUSH = 0, MZ_PARTIAL_FLUSH = 1, MZ_SYNC_FLUSH = 2, MZ_FULL_FLUSH = 3, MZ_FINISH = 4, MZ_BLOCK = 5 };

// Return status codes. MZ_PARAM_ERROR is non-standard.
enum { MZ_OK = 0, MZ_STREAM_END = 1, MZ_NEED_DICT = 2, MZ_ERRNO = -1, MZ_STREAM_ERROR = -2, MZ_DATA_ERROR = -3, MZ_MEM_ERROR = -4, MZ_BUF_ERROR = -5, MZ_VERSION_ERROR = -6, MZ_PARAM_ERROR = -10000 };

// Compression levels: 0-9 are the standard zlib-style levels, 10 is best possible compression (not zlib compatible, and may be very slow), MZ_DEFAULT_COMPRESSION=MZ_DEFAULT_LEVEL.
enum { MZ_NO_COMPRESSION = 0, MZ_BEST_SPEED = 1, MZ_BEST_COMPRESSION = 9, MZ_UBER_COMPRESSION = 10, MZ_DEFAULT_LEVEL = 6, MZ_DEFAULT_COMPRESSION = -1 };

// Window bits
#define MZ_DEFAULT_WINDOW_BITS 15

struct mz_internal_state;

// Compression/decompression stream struct.
typedef struct mz_stream_s
{
  const unsigned char *next_in;     // pointer to next byte to read
  unsigned int avail_in;            // number of bytes available at next_in
  mz_ulong total_in;                // total number of bytes consumed so far

  unsigned char *next_out;          // pointer to next byte to write
  unsigned int avail_out;           // number of bytes that can be written to next_out
  mz_ulong total_out;               // total number of bytes produced so far

  char *msg;                        // error msg (unused)
  struct mz_internal_state *state;  // internal state, allocated by zalloc/zfree

  mz_alloc_func zalloc;             // optional heap allocation function (defaults to malloc)
  mz_free_func zfree;               // optional heap free function (defaults to free)
  void *opaque;                     // heap alloc function user pointer

  int data_type;                    // data_type (unused)
  mz_ulong adler;                   // adler32 of the source or uncompressed data
  mz_ulong reserved;                // not used
} mz_stream;

typedef mz_stream *mz_streamp;

// Returns the version string of miniz.c.
const char *mz_version(void);

// mz_deflateInit() initializes a compressor with default options:
// Parameters:
//  pStream must point to an initialized mz_stream struct.
//  level must be between [MZ_NO_COMPRESSION, MZ_BEST_COMPRESSION].
//  level 1 enables a specially optimized compression function that's been optimized purely for performance, not ratio.
//  (This special func. is currently only enabled when MINIZ_USE_UNALIGNED_LOADS_AND_STORES and MINIZ_LITTLE_ENDIAN are defined.)
// Return values:
//  MZ_OK on success.
//  MZ_STREAM_ERROR if the stream is bogus.
//  MZ_PARAM_ERROR if the input parameters are bogus.
//  MZ_MEM_ERROR on out of memory.
int mz_deflateInit(mz_streamp pStream, int level);

// mz_deflateInit2() is like mz_deflate(), except with more control:
// Additional parameters:
//   method must be MZ_DEFLATED
//   window_bits must be MZ_DEFAULT_WINDOW_BITS (to wrap the deflate stream with zlib header/adler-32 footer) or -MZ_DEFAULT_WINDOW_BITS (raw deflate/no header or footer)
//   mem_level must be between [1, 9] (it's checked but ignored by miniz.c)
int mz_deflateInit2(mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy);

// Quickly resets a compressor without having to reallocate anything. Same as calling mz_deflateEnd() followed by mz_deflateInit()/mz_deflateInit2().
int mz_deflateReset(mz_streamp pStream);

// mz_deflate() compresses the input to output, consuming as much of the input and producing as much output as possible.
// Parameters:
//   pStream is the stream to read from and write to. You must initialize/update the next_in, avail_in, next_out, and avail_out members.
//   flush may be MZ_NO_FLUSH, MZ_PARTIAL_FLUSH/MZ_SYNC_FLUSH, MZ_FULL_FLUSH, or MZ_FINISH.
// Return values:
//   MZ_OK on success (when flushing, or if more input is needed but not available, and/or there's more output to be written but the output buffer is full).
//   MZ_STREAM_END if all input has been consumed and all output bytes have been written. Don't call mz_deflate() on the stream anymore.
//   MZ_STREAM_ERROR if the stream is bogus.
//   MZ_PARAM_ERROR if one of the parameters is invalid.
//   MZ_BUF_ERROR if no forward progress is possible because the input and/or output buffers are empty. (Fill up the input buffer or free up some output space and try again.)
int mz_deflate(mz_streamp pStream, int flush);

// mz_deflateEnd() deinitializes a compressor:
// Return values:
//  MZ_OK on success.
//  MZ_STREAM_ERROR if the stream is bogus.
int mz_deflateEnd(mz_streamp pStream);

// mz_deflateBound() returns a (very) conservative upper bound on the amount of data that could be generated by deflate(), assuming flush is set to only MZ_NO_FLUSH or MZ_FINISH.
mz_ulong mz_deflateBound(mz_streamp pStream, mz_ulong source_len);

// Single-call compression functions mz_compress() and mz_compress2():
// Returns MZ_OK on success, or one of the error codes from mz_deflate() on failure.
int mz_compress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);

// mz_compressBound() returns a (very) conservative upper bound on the amount of data that could be generated by calling mz_compress().
mz_ulong mz_compressBound(mz_ulong source_len);

// Initializes a decompressor.
int mz_inflateInit(mz_streamp pStream);

// mz_inflateInit2() is like mz_inflateInit() with an additional option that controls the window size and whether or not the stream has been wrapped with a zlib header/footer:
// window_bits must be MZ_DEFAULT_WINDOW_BITS (to parse zlib header/footer) or -MZ_DEFAULT_WINDOW_BITS (raw deflate).
int mz_inflateInit2(mz_streamp pStream, int window_bits);

// Decompresses the input stream to the output, consuming only as much of the input as needed, and writing as much to the output as possible.
// Parameters:
//   pStream is the stream to read from and write to. You must initialize/update the next_in, avail_in, next_out, and avail_out members.
//   flush may be MZ_NO_FLUSH, MZ_SYNC_FLUSH, or MZ_FINISH.
//   On the first call, if flush is MZ_FINISH it's assumed the input and output buffers are both sized large enough to decompress the entire stream in a single call (this is slightly faster).
//   MZ_FINISH implies that there are no more source bytes available beside what's already in the input buffer, and that the output buffer is large enough to hold the rest of the decompressed data.
// Return values:
//   MZ_OK on success. Either more input is needed but not available, and/or there's more output to be written but the output buffer is full.
//   MZ_STREAM_END if all needed input has been consumed and all output bytes have been written. For zlib streams, the adler-32 of the decompressed data has also been verified.
//   MZ_STREAM_ERROR if the stream is bogus.
//   MZ_DATA_ERROR if the deflate stream is invalid.
//   MZ_PARAM_ERROR if one of the parameters is invalid.
//   MZ_BUF_ERROR if no forward progress is possible because the input buffer is empty but the inflater needs more input to continue, or if the output buffer is not large enough. Call mz_inflate() again
//   with more input data, or with more room in the output buffer (except when using single call decompression, described above).
int mz_inflate(mz_streamp pStream, int flush);

// Deinitializes a decompressor.
int mz_inflateEnd(mz_streamp pStream);

// Single-call decompression.
// Returns MZ_OK on success, or one of the error codes from mz_inflate() on failure.
int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);

// Returns a string description of the specified error code, or NULL if the error code is invalid.
const char *mz_error(int err);

// Redefine zlib-compatible names to miniz equivalents, so miniz.c can be used as a drop-in replacement for the subset of zlib that miniz.c supports.
// Define MINIZ_NO_ZLIB_COMPATIBLE_NAMES to disable zlib-compatibility if you use zlib in the same project.
#ifndef MINIZ_NO_ZLIB_COMPATIBLE_NAMES
  typedef unsigned char Byte;
  typedef unsigned int uInt;
  typedef mz_ulong uLong;
  typedef Byte Bytef;
  typedef uInt uIntf;
  typedef char charf;
  typedef int intf;
  typedef void *voidpf;
  typedef uLong uLongf;
  typedef void *voidp;
  typedef void *const voidpc;
  #define Z_NULL                0
  #define Z_NO_FLUSH            MZ_NO_FLUSH
  #define Z_PARTIAL_FLUSH       MZ_PARTIAL_FLUSH
  #define Z_SYNC_FLUSH          MZ_SYNC_FLUSH
  #define Z_FULL_FLUSH          MZ_FULL_FLUSH
  #define Z_FINISH              MZ_FINISH
  #define Z_BLOCK               MZ_BLOCK
  #define Z_OK                  MZ_OK
  #define Z_STREAM_END          MZ_STREAM_END
  #define Z_NEED_DICT           MZ_NEED_DICT
  #define Z_ERRNO               MZ_ERRNO
  #define Z_STREAM_ERROR        MZ_STREAM_ERROR
  #define Z_DATA_ERROR          MZ_DATA_ERROR
  #define Z_MEM_ERROR           MZ_MEM_ERROR
  #define Z_BUF_ERROR           MZ_BUF_ERROR
  #define Z_VERSION_ERROR       MZ_VERSION_ERROR
  #define Z_PARAM_ERROR         MZ_PARAM_ERROR
  #define Z_NO_COMPRESSION      MZ_NO_COMPRESSION
  #define Z_BEST_SPEED          MZ_BEST_SPEED
  #define Z_BEST_COMPRESSION    MZ_BEST_COMPRESSION
  #define Z_DEFAULT_COMPRESSION MZ_DEFAULT_COMPRESSION
  #define Z_DEFAULT_STRATEGY    MZ_DEFAULT_STRATEGY
  #define Z_FILTERED            MZ_FILTERED
  #define Z_HUFFMAN_ONLY        MZ_HUFFMAN_ONLY
  #define Z_RLE                 MZ_RLE
  #define Z_FIXED               MZ_FIXED
  #define Z_DEFLATED            MZ_DEFLATED
  #define Z_DEFAULT_WINDOW_BITS MZ_DEFAULT_WINDOW_BITS
  #define alloc_func            mz_alloc_func
  #define free_func             mz_free_func
  #define internal_state        mz_internal_state
  #define z_stream              mz_stream
  #define deflateInit           mz_deflateInit
  #define deflateInit2          mz_deflateInit2
  #define deflateReset          mz_deflateReset
  #define deflate               mz_deflate
  #define deflateEnd            mz_deflateEnd
  #define deflateBound          mz_deflateBound
  #define compress              mz_compress
  #define compress2             mz_compress2
  #define compressBound         mz_compressBound
  #define inflateInit           mz_inflateInit
  #define inflateInit2          mz_inflateInit2
  #define inflate               mz_inflate
  #define inflateEnd            mz_inflateEnd
  #define uncompress            mz_uncompress
  #define crc32                 mz_crc32
  #define adler32               mz_adler32
  #define MAX_WBITS             15
  #define MAX_MEM_LEVEL         9
  #define zError                mz_error
  #define ZLIB_VERSION          MZ_VERSION
  #define ZLIB_VERNUM           MZ_VERNUM
  #define ZLIB_VER_MAJOR        MZ_VER_MAJOR
  #define ZLIB_VER_MINOR        MZ_VER_MINOR
  #define ZLIB_VER_REVISION     MZ_VER_REVISION
  #define ZLIB_VER_SUBREVISION  MZ_VER_SUBREVISION
  #define zlibVersion           mz_version
  #define zlib_version          mz_version()
#endif // #ifndef MINIZ_NO_ZLIB_COMPATIBLE_NAMES

#endif // MINIZ_NO_ZLIB_APIS

// ------------------- Types and macros

typedef unsigned char mz_uint8;
typedef signed short mz_int16;
typedef unsigned short mz_uint16;
typedef unsigned int mz_uint32;
typedef unsigned int mz_uint;
typedef long long mz_int64;
typedef unsigned long long mz_uint64;
typedef int mz_bool;

#define MZ_FALSE (0)
#define MZ_TRUE (1)

// An attempt to work around MSVC's spammy "warning C4127: conditional expression is constant" message.
#ifdef _MSC_VER
   #define MZ_MACRO_END while (0, 0)
#else
   #define MZ_MACRO_END while (0)
#endif

// ------------------- ZIP archive reading/writing

#ifndef MINIZ_NO_ARCHIVE_APIS

enum
{
  MZ_ZIP_MAX_IO_BUF_SIZE = 64*1024,
  MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE = 260,
  MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE = 256
};

typedef struct
{
  mz_uint32 m_file_index;
  mz_uint32 m_central_dir_ofs;
  mz_uint16 m_version_made_by;
  mz_uint16 m_version_needed;
  mz_uint16 m_bit_flag;
  mz_uint16 m_method;
#ifndef MINIZ_NO_TIME
  time_t m_time;
#endif
  mz_uint32 m_crc32;
  mz_uint64 m_comp_size;
  mz_uint64 m_uncomp_size;
  mz_uint16 m_internal_attr;
  mz_uint32 m_external_attr;
  mz_uint64 m_local_header_ofs;
  mz_uint32 m_comment_size;
  char m_filename[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
  char m_comment[MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE];
} mz_zip_archive_file_stat;

typedef size_t (*mz_file_read_func)(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n);
typedef size_t (*mz_file_write_func)(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n);

struct mz_zip_internal_state_tag;
typedef struct mz_zip_internal_state_tag mz_zip_internal_state;

typedef enum
{
  MZ_ZIP_MODE_INVALID = 0,
  MZ_ZIP_MODE_READING = 1,
  MZ_ZIP_MODE_WRITING = 2,
  MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED = 3
} mz_zip_mode;

typedef struct mz_zip_archive_tag
{
  mz_uint64 m_archive_size;
  mz_uint64 m_central_directory_file_ofs;
  mz_uint m_total_files;
  mz_zip_mode m_zip_mode;

  mz_uint m_file_offset_alignment;

  mz_alloc_func m_pAlloc;
  mz_free_func m_pFree;
  mz_realloc_func m_pRealloc;
  void *m_pAlloc_opaque;

  mz_file_read_func m_pRead;
  mz_file_write_func m_pWrite;
  void *m_pIO_opaque;

  mz_zip_internal_state *m_pState;

} mz_zip_archive;

typedef enum
{
  MZ_ZIP_FLAG_CASE_SENSITIVE                = 0x0100,
  MZ_ZIP_FLAG_IGNORE_PATH                   = 0x0200,
  MZ_ZIP_FLAG_COMPRESSED_DATA               = 0x0400,
  MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY = 0x0800
} mz_zip_flags;

// ZIP archive reading

// Inits a ZIP archive reader.
// These functions read and validate the archive's central directory.
mz_bool mz_zip_reader_init(mz_zip_archive *pZip, mz_uint64 size, mz_uint32 flags);
mz_bool mz_zip_reader_init_mem(mz_zip_archive *pZip, const void *pMem, size_t size, mz_uint32 flags);

#ifndef MINIZ_NO_STDIO
mz_bool mz_zip_reader_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint32 flags);
#endif

// Returns the total number of files in the archive.
mz_uint mz_zip_reader_get_num_files(mz_zip_archive *pZip);

// Returns detailed information about an archive file entry.
mz_bool mz_zip_reader_file_stat(mz_zip_archive *pZip, mz_uint file_index, mz_zip_archive_file_stat *pStat);

// Determines if an archive file entry is a directory entry.
mz_bool mz_zip_reader_is_file_a_directory(mz_zip_archive *pZip, mz_uint file_index);
mz_bool mz_zip_reader_is_file_encrypted(mz_zip_archive *pZip, mz_uint file_index);

// Retrieves the filename of an archive file entry.
// Returns the number of bytes written to pFilename, or if filename_buf_size is 0 this function returns the number of bytes needed to fully store the filename.
mz_uint mz_zip_reader_get_filename(mz_zip_archive *pZip, mz_uint file_index, char *pFilename, mz_uint filename_buf_size);

// Attempts to locates a file in the archive's central directory.
// Valid flags: MZ_ZIP_FLAG_CASE_SENSITIVE, MZ_ZIP_FLAG_IGNORE_PATH
// Returns -1 if the file cannot be found.
int mz_zip_reader_locate_file(mz_zip_archive *pZip, const char *pName, const char *pComment, mz_uint flags);

// Extracts a archive file to a memory buffer using no memory allocation.
mz_bool mz_zip_reader_extract_to_mem_no_alloc(mz_zip_archive *pZip, mz_uint file_index, void *pBuf, size_t buf_size, mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size);
mz_bool mz_zip_reader_extract_file_to_mem_no_alloc(mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size);

// Extracts a archive file to a memory buffer.
mz_bool mz_zip_reader_extract_to_mem(mz_zip_archive *pZip, mz_uint file_index, void *pBuf, size_t buf_size, mz_uint flags);
mz_bool mz_zip_reader_extract_file_to_mem(mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, mz_uint flags);

// Extracts a archive file to a dynamically allocated heap buffer.
void *mz_zip_reader_extract_to_heap(mz_zip_archive *pZip, mz_uint file_index, size_t *pSize, mz_uint flags);
void *mz_zip_reader_extract_file_to_heap(mz_zip_archive *pZip, const char *pFilename, size_t *pSize, mz_uint flags);

// Extracts a archive file using a callback function to output the file's data.
mz_bool mz_zip_reader_extract_to_callback(mz_zip_archive *pZip, mz_uint file_index, mz_file_write_func pCallback, void *pOpaque, mz_uint flags);
mz_bool mz_zip_reader_extract_file_to_callback(mz_zip_archive *pZip, const char *pFilename, mz_file_write_func pCallback, void *pOpaque, mz_uint flags);

#ifndef MINIZ_NO_STDIO
// Extracts a archive file to a disk file and sets its last accessed and modified times.
// This function only extracts files, not archive directory records.
mz_bool mz_zip_reader_extract_to_file(mz_zip_archive *pZip, mz_uint file_index, const char *pDst_filename, mz_uint flags);
mz_bool mz_zip_reader_extract_file_to_file(mz_zip_archive *pZip, const char *pArchive_filename, const char *pDst_filename, mz_uint flags);
#endif

// Ends archive reading, freeing all allocations, and closing the input archive file if mz_zip_reader_init_file() was used.
mz_bool mz_zip_reader_end(mz_zip_archive *pZip);

// ZIP archive writing

#ifndef MINIZ_NO_ARCHIVE_WRITING_APIS

// Inits a ZIP archive writer.
mz_bool mz_zip_writer_init(mz_zip_archive *pZip, mz_uint64 existing_size);
mz_bool mz_zip_writer_init_heap(mz_zip_archive *pZip, size_t size_to_reserve_at_beginning, size_t initial_allocation_size);

#ifndef MINIZ_NO_STDIO
mz_bool mz_zip_writer_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint64 size_to_reserve_at_beginning);
#endif

// Converts a ZIP archive reader object into a writer object, to allow efficient in-place file appends to occur on an existing archive.
// For archives opened using mz_zip_reader_init_file, pFilename must be the archive's filename so it can be reopened for writing. If the file can't be reopened, mz_zip_reader_end() will be called.
// For archives opened using mz_zip_reader_init_mem, the memory block must be growable using the realloc callback (which defaults to realloc unless you've overridden it).
// Finally, for archives opened using mz_zip_reader_init, the mz_zip_archive's user provided m_pWrite function cannot be NULL.
// Note: In-place archive modification is not recommended unless you know what you're doing, because if execution stops or something goes wrong before
// the archive is finalized the file's central directory will be hosed.
mz_bool mz_zip_writer_init_from_reader(mz_zip_archive *pZip, const char *pFilename);

// Adds the contents of a memory buffer to an archive. These functions record the current local time into the archive.
// To add a directory entry, call this method with an archive name ending in a forwardslash with empty buffer.
// level_and_flags - compression level (0-10, see MZ_BEST_SPEED, MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to MZ_DEFAULT_COMPRESSION.
mz_bool mz_zip_writer_add_mem(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, mz_uint level_and_flags);
mz_bool mz_zip_writer_add_mem_ex(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags, mz_uint64 uncomp_size, mz_uint32 uncomp_crc32);

#ifndef MINIZ_NO_STDIO
// Adds the contents of a disk file to an archive. This function also records the disk file's modified time into the archive.
// level_and_flags - compression level (0-10, see MZ_BEST_SPEED, MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to MZ_DEFAULT_COMPRESSION.
mz_bool mz_zip_writer_add_file(mz_zip_archive *pZip, const char *pArchive_name, const char *pSrc_filename, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags);
#endif

// Adds a file to an archive by fully cloning the data from another archive.
// This function fully clones the source file's compressed data (no recompression), along with its full filename, extra data, and comment fields.
mz_bool mz_zip_writer_add_from_zip_reader(mz_zip_archive *pZip, mz_zip_archive *pSource_zip, mz_uint file_index);

// Finalizes the archive by writing the central directory records followed by the end of central directory record.
// After an archive is finalized, the only valid call on the mz_zip_archive struct is mz_zip_writer_end().
// An archive must be manually finalized by calling this function for it to be valid.
mz_bool mz_zip_writer_finalize_archive(mz_zip_archive *pZip);
mz_bool mz_zip_writer_finalize_heap_archive(mz_zip_archive *pZip, void **pBuf, size_t *pSize);

// Ends archive writing, freeing all allocations, and closing the output file if mz_zip_writer_init_file() was used.
// Note for the archive to be valid, it must have been finalized before ending.
mz_bool mz_zip_writer_end(mz_zip_archive *pZip);

// Misc. high-level helper functions:

// mz_zip_add_mem_to_archive_file_in_place() efficiently (but not atomically) appends a memory blob to a ZIP archive.
// level_and_flags - compression level (0-10, see MZ_BEST_SPEED, MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to MZ_DEFAULT_COMPRESSION.
mz_bool mz_zip_add_mem_to_archive_file_in_place(const char *pZip_filename, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags);

// Reads a single file from an archive into a heap block.
// Returns NULL on failure.
void *mz_zip_extract_archive_file_to_heap(const char *pZip_filename, const char *pArchive_name, size_t *pSize, mz_uint zip_flags);

#endif // #ifndef MINIZ_NO_ARCHIVE_WRITING_APIS

#endif // #ifndef MINIZ_NO_ARCHIVE_APIS

// ------------------- Low-level Decompression API Definitions

// Decompression flags used by tinfl_decompress().
// TINFL_FLAG_PARSE_ZLIB_HEADER: If set, the input has a valid zlib header and ends with an adler32 checksum (it's a valid zlib stream). Otherwise, the input is a raw deflate stream.
// TINFL_FLAG_HAS_MORE_INPUT: If set, there are more input bytes available beyond the end of the supplied input buffer. If clear, the input buffer contains all remaining input.
// TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF: If set, the output buffer is large enough to hold the entire decompressed stream. If clear, the output buffer is at least the size of the dictionary (typically 32KB).
// TINFL_FLAG_COMPUTE_ADLER32: Force adler-32 checksum computation of the decompressed bytes.
enum
{
  TINFL_FLAG_PARSE_ZLIB_HEADER = 1,
  TINFL_FLAG_HAS_MORE_INPUT = 2,
  TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4,
  TINFL_FLAG_COMPUTE_ADLER32 = 8
};

// High level decompression functions:
// tinfl_decompress_mem_to_heap() decompresses a block in memory to a heap block allocated via malloc().
// On entry:
//  pSrc_buf, src_buf_len: Pointer and size of the Deflate or zlib source data to decompress.
// On return:
//  Function returns a pointer to the decompressed data, or NULL on failure.
//  *pOut_len will be set to the decompressed data's size, which could be larger than src_buf_len on uncompressible data.
//  The caller must call mz_free() on the returned block when it's no longer needed.
void *tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

// tinfl_decompress_mem_to_mem() decompresses a block in memory to another block in memory.
// Returns TINFL_DECOMPRESS_MEM_TO_MEM_FAILED on failure, or the number of bytes written on success.
#define TINFL_DECOMPRESS_MEM_TO_MEM_FAILED ((size_t)(-1))
size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);

// tinfl_decompress_mem_to_callback() decompresses a block in memory to an internal 32KB buffer, and a user provided callback function will be called to flush the buffer.
// Returns 1 on success or 0 on failure.
typedef int (*tinfl_put_buf_func_ptr)(const void* pBuf, int len, void *pUser);
int tinfl_decompress_mem_to_callback(const void *pIn_buf, size_t *pIn_buf_size, tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags);

struct tinfl_decompressor_tag; typedef struct tinfl_decompressor_tag tinfl_decompressor;

// Max size of LZ dictionary.
#define TINFL_LZ_DICT_SIZE 32768

// Return status.
typedef enum
{
  TINFL_STATUS_BAD_PARAM = -3,
  TINFL_STATUS_ADLER32_MISMATCH = -2,
  TINFL_STATUS_FAILED = -1,
  TINFL_STATUS_DONE = 0,
  TINFL_STATUS_NEEDS_MORE_INPUT = 1,
  TINFL_STATUS_HAS_MORE_OUTPUT = 2
} tinfl_status;

// Initializes the decompressor to its initial state.
#define tinfl_init(r) do { (r)->m_state = 0; } MZ_MACRO_END
#define tinfl_get_adler32(r) (r)->m_check_adler32

// Main low-level decompressor coroutine function. This is the only function actually needed for decompression. All the other functions are just high-level helpers for improved usability.
// This is a universal API, i.e. it can be used as a building block to build any desired higher level decompression API. In the limit case, it can be called once per every byte input or output.
tinfl_status tinfl_decompress(tinfl_decompressor *r, const mz_uint8 *pIn_buf_next, size_t *pIn_buf_size, mz_uint8 *pOut_buf_start, mz_uint8 *pOut_buf_next, size_t *pOut_buf_size, const mz_uint32 decomp_flags);

// Internal/private bits follow.
enum
{
  TINFL_MAX_HUFF_TABLES = 3, TINFL_MAX_HUFF_SYMBOLS_0 = 288, TINFL_MAX_HUFF_SYMBOLS_1 = 32, TINFL_MAX_HUFF_SYMBOLS_2 = 19,
  TINFL_FAST_LOOKUP_BITS = 10, TINFL_FAST_LOOKUP_SIZE = 1 << TINFL_FAST_LOOKUP_BITS
};

typedef struct
{
  mz_uint8 m_code_size[TINFL_MAX_HUFF_SYMBOLS_0];
  mz_int16 m_look_up[TINFL_FAST_LOOKUP_SIZE], m_tree[TINFL_MAX_HUFF_SYMBOLS_0 * 2];
} tinfl_huff_table;

#if MINIZ_HAS_64BIT_REGISTERS
  #define TINFL_USE_64BIT_BITBUF 1
#endif

#if TINFL_USE_64BIT_BITBUF
  typedef mz_uint64 tinfl_bit_buf_t;
  #define TINFL_BITBUF_SIZE (64)
#else
  typedef mz_uint32 tinfl_bit_buf_t;
  #define TINFL_BITBUF_SIZE (32)
#endif

struct tinfl_decompressor_tag
{
  mz_uint32 m_state, m_num_bits, m_zhdr0, m_zhdr1, m_z_adler32, m_final, m_type, m_check_adler32, m_dist, m_counter, m_num_extra, m_table_sizes[TINFL_MAX_HUFF_TABLES];
  tinfl_bit_buf_t m_bit_buf;
  size_t m_dist_from_out_buf_start;
  tinfl_huff_table m_tables[TINFL_MAX_HUFF_TABLES];
  mz_uint8 m_raw_header[4], m_len_codes[TINFL_MAX_HUFF_SYMBOLS_0 + TINFL_MAX_HUFF_SYMBOLS_1 + 137];
};

// ------------------- Low-level Compression API Definitions

// Set TDEFL_LESS_MEMORY to 1 to use less memory (compression will be slightly slower, and raw/dynamic blocks will be output more frequently).
#define TDEFL_LESS_MEMORY 0

// tdefl_init() compression flags logically OR'd together (low 12 bits contain the max. number of probes per dictionary search):
// TDEFL_DEFAULT_MAX_PROBES: The compressor defaults to 128 dictionary probes per dictionary search. 0=Huffman only, 1=Huffman+LZ (fastest/crap compression), 4095=Huffman+LZ (slowest/best compression).
enum
{
  TDEFL_HUFFMAN_ONLY = 0, TDEFL_DEFAULT_MAX_PROBES = 128, TDEFL_MAX_PROBES_MASK = 0xFFF
};

// TDEFL_WRITE_ZLIB_HEADER: If set, the compressor outputs a zlib header before the deflate data, and the Adler-32 of the source data at the end. Otherwise, you'll get raw deflate data.
// TDEFL_COMPUTE_ADLER32: Always compute the adler-32 of the input data (even when not writing zlib headers).
// TDEFL_GREEDY_PARSING_FLAG: Set to use faster greedy parsing, instead of more efficient lazy parsing.
// TDEFL_NONDETERMINISTIC_PARSING_FLAG: Enable to decrease the compressor's initialization time to the minimum, but the output may vary from run to run given the same input (depending on the contents of memory).
// TDEFL_RLE_MATCHES: Only look for RLE matches (matches with a distance of 1)
// TDEFL_FILTER_MATCHES: Discards matches <= 5 chars if enabled.
// TDEFL_FORCE_ALL_STATIC_BLOCKS: Disable usage of optimized Huffman tables.
// TDEFL_FORCE_ALL_RAW_BLOCKS: Only use raw (uncompressed) deflate blocks.
// The low 12 bits are reserved to control the max # of hash probes per dictionary lookup (see TDEFL_MAX_PROBES_MASK).
enum
{
  TDEFL_WRITE_ZLIB_HEADER             = 0x01000,
  TDEFL_COMPUTE_ADLER32               = 0x02000,
  TDEFL_GREEDY_PARSING_FLAG           = 0x04000,
  TDEFL_NONDETERMINISTIC_PARSING_FLAG = 0x08000,
  TDEFL_RLE_MATCHES                   = 0x10000,
  TDEFL_FILTER_MATCHES                = 0x20000,
  TDEFL_FORCE_ALL_STATIC_BLOCKS       = 0x40000,
  TDEFL_FORCE_ALL_RAW_BLOCKS          = 0x80000
};

// High level compression functions:
// tdefl_compress_mem_to_heap() compresses a block in memory to a heap block allocated via malloc().
// On entry:
//  pSrc_buf, src_buf_len: Pointer and size of source block to compress.
//  flags: The max match finder probes (default is 128) logically OR'd against the above flags. Higher probes are slower but improve compression.
// On return:
//  Function returns a pointer to the compressed data, or NULL on failure.
//  *pOut_len will be set to the compressed data's size, which could be larger than src_buf_len on uncompressible data.
//  The caller must free() the returned block when it's no longer needed.
void *tdefl_compress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

// tdefl_compress_mem_to_mem() compresses a block in memory to another block in memory.
// Returns 0 on failure.
size_t tdefl_compress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);

// Compresses an image to a compressed PNG file in memory.
// On entry:
//  pImage, w, h, and num_chans describe the image to compress. num_chans may be 1, 2, 3, or 4.
//  The image pitch in bytes per scanline will be w*num_chans. The leftmost pixel on the top scanline is stored first in memory.
//  level may range from [0,10], use MZ_NO_COMPRESSION, MZ_BEST_SPEED, MZ_BEST_COMPRESSION, etc. or a decent default is MZ_DEFAULT_LEVEL
//  If flip is true, the image will be flipped on the Y axis (useful for OpenGL apps).
// On return:
//  Function returns a pointer to the compressed data, or NULL on failure.
//  *pLen_out will be set to the size of the PNG image file.
//  The caller must mz_free() the returned heap block (which will typically be larger than *pLen_out) when it's no longer needed.
void *tdefl_write_image_to_png_file_in_memory_ex(const void *pImage, int w, int h, int num_chans, size_t *pLen_out, mz_uint level, mz_bool flip);
void *tdefl_write_image_to_png_file_in_memory(const void *pImage, int w, int h, int num_chans, size_t *pLen_out);

// Output stream interface. The compressor uses this interface to write compressed data. It'll typically be called TDEFL_OUT_BUF_SIZE at a time.
typedef mz_bool (*tdefl_put_buf_func_ptr)(const void* pBuf, int len, void *pUser);

// tdefl_compress_mem_to_output() compresses a block to an output stream. The above helpers use this function internally.
mz_bool tdefl_compress_mem_to_output(const void *pBuf, size_t buf_len, tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags);

enum { TDEFL_MAX_HUFF_TABLES = 3, TDEFL_MAX_HUFF_SYMBOLS_0 = 288, TDEFL_MAX_HUFF_SYMBOLS_1 = 32, TDEFL_MAX_HUFF_SYMBOLS_2 = 19, TDEFL_LZ_DICT_SIZE = 32768, TDEFL_LZ_DICT_SIZE_MASK = TDEFL_LZ_DICT_SIZE - 1, TDEFL_MIN_MATCH_LEN = 3, TDEFL_MAX_MATCH_LEN = 258 };

// TDEFL_OUT_BUF_SIZE MUST be large enough to hold a single entire compressed output block (using static/fixed Huffman codes).
#if TDEFL_LESS_MEMORY
enum { TDEFL_LZ_CODE_BUF_SIZE = 24 * 1024, TDEFL_OUT_BUF_SIZE = (TDEFL_LZ_CODE_BUF_SIZE * 13 ) / 10, TDEFL_MAX_HUFF_SYMBOLS = 288, TDEFL_LZ_HASH_BITS = 12, TDEFL_LEVEL1_HASH_SIZE_MASK = 4095, TDEFL_LZ_HASH_SHIFT = (TDEFL_LZ_HASH_BITS + 2) / 3, TDEFL_LZ_HASH_SIZE = 1 << TDEFL_LZ_HASH_BITS };
#else
enum { TDEFL_LZ_CODE_BUF_SIZE = 64 * 1024, TDEFL_OUT_BUF_SIZE = (TDEFL_LZ_CODE_BUF_SIZE * 13 ) / 10, TDEFL_MAX_HUFF_SYMBOLS = 288, TDEFL_LZ_HASH_BITS = 15, TDEFL_LEVEL1_HASH_SIZE_MASK = 4095, TDEFL_LZ_HASH_SHIFT = (TDEFL_LZ_HASH_BITS + 2) / 3, TDEFL_LZ_HASH_SIZE = 1 << TDEFL_LZ_HASH_BITS };
#endif

// The low-level tdefl functions below may be used directly if the above helper functions aren't flexible enough. The low-level functions don't make any heap allocations, unlike the above helper functions.
typedef enum
{
  TDEFL_STATUS_BAD_PARAM = -2,
  TDEFL_STATUS_PUT_BUF_FAILED = -1,
  TDEFL_STATUS_OKAY = 0,
  TDEFL_STATUS_DONE = 1,
} tdefl_status;

// Must map to MZ_NO_FLUSH, MZ_SYNC_FLUSH, etc. enums
typedef enum
{
  TDEFL_NO_FLUSH = 0,
  TDEFL_SYNC_FLUSH = 2,
  TDEFL_FULL_FLUSH = 3,
  TDEFL_FINISH = 4
} tdefl_flush;

// tdefl's compression state structure.
typedef struct
{
  tdefl_put_buf_func_ptr m_pPut_buf_func;
  void *m_pPut_buf_user;
  mz_uint m_flags, m_max_probes[2];
  int m_greedy_parsing;
  mz_uint m_adler32, m_lookahead_pos, m_lookahead_size, m_dict_size;
  mz_uint8 *m_pLZ_code_buf, *m_pLZ_flags, *m_pOutput_buf, *m_pOutput_buf_end;
  mz_uint m_num_flags_left, m_total_lz_bytes, m_lz_code_buf_dict_pos, m_bits_in, m_bit_buffer;
  mz_uint m_saved_match_dist, m_saved_match_len, m_saved_lit, m_output_flush_ofs, m_output_flush_remaining, m_finished, m_block_index, m_wants_to_finish;
  tdefl_status m_prev_return_status;
  const void *m_pIn_buf;
  void *m_pOut_buf;
  size_t *m_pIn_buf_size, *m_pOut_buf_size;
  tdefl_flush m_flush;
  const mz_uint8 *m_pSrc;
  size_t m_src_buf_left, m_out_buf_ofs;
  mz_uint8 m_dict[TDEFL_LZ_DICT_SIZE + TDEFL_MAX_MATCH_LEN - 1];
  mz_uint16 m_huff_count[TDEFL_MAX_HUFF_TABLES][TDEFL_MAX_HUFF_SYMBOLS];
  mz_uint16 m_huff_codes[TDEFL_MAX_HUFF_TABLES][TDEFL_MAX_HUFF_SYMBOLS];
  mz_uint8 m_huff_code_sizes[TDEFL_MAX_HUFF_TABLES][TDEFL_MAX_HUFF_SYMBOLS];
  mz_uint8 m_lz_code_buf[TDEFL_LZ_CODE_BUF_SIZE];
  mz_uint16 m_next[TDEFL_LZ_DICT_SIZE];
  mz_uint16 m_hash[TDEFL_LZ_HASH_SIZE];
  mz_uint8 m_output_buf[TDEFL_OUT_BUF_SIZE];
} tdefl_compressor;

// Initializes the compressor.
// There is no corresponding deinit() function because the tdefl API's do not dynamically allocate memory.
// pBut_buf_func: If NULL, output data will be supplied to the specified callback. In this case, the user should call the tdefl_compress_buffer() API for compression.
// If pBut_buf_func is NULL the user should always call the tdefl_compress() API.
// flags: See the above enums (TDEFL_HUFFMAN_ONLY, TDEFL_WRITE_ZLIB_HEADER, etc.)
tdefl_status tdefl_init(tdefl_compressor *d, tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags);

// Compresses a block of data, consuming as much of the specified input buffer as possible, and writing as much compressed data to the specified output buffer as possible.
tdefl_status tdefl_compress(tdefl_compressor *d, const void *pIn_buf, size_t *pIn_buf_size, void *pOut_buf, size_t *pOut_buf_size, tdefl_flush flush);

// tdefl_compress_buffer() is only usable when the tdefl_init() is called with a non-NULL tdefl_put_buf_func_ptr.
// tdefl_compress_buffer() always consumes the entire input buffer.
tdefl_status tdefl_compress_buffer(tdefl_compressor *d, const void *pIn_buf, size_t in_buf_size, tdefl_flush flush);

tdefl_status tdefl_get_prev_return_status(tdefl_compressor *d);
mz_uint32 tdefl_get_adler32(tdefl_compressor *d);

// Can't use tdefl_create_comp_flags_from_zip_params if MINIZ_NO_ZLIB_APIS isn't defined, because it uses some of its macros.
#ifndef MINIZ_NO_ZLIB_APIS
// Create tdefl_compress() flags given zlib-style compression parameters.
// level may range from [0,10] (where 10 is absolute max compression, but may be much slower on some files)
// window_bits may be -15 (raw deflate) or 15 (zlib)
// strategy may be either MZ_DEFAULT_STRATEGY, MZ_FILTERED, MZ_HUFFMAN_ONLY, MZ_RLE, or MZ_FIXED
mz_uint tdefl_create_comp_flags_from_zip_params(int level, int window_bits, int strategy);
#endif // #ifndef MINIZ_NO_ZLIB_APIS

#ifdef __cplusplus
}
#endif

#endif // MINIZ_HEADER_INCLUDED

// ------------------- End of Header: Implementation follows. (If you only want the header, define MINIZ_HEADER_FILE_ONLY.)

#ifndef MINIZ_HEADER_FILE_ONLY

typedef unsigned char mz_validate_uint16[sizeof(mz_uint16)==2 ? 1 : -1];
typedef unsigned char mz_validate_uint32[sizeof(mz_uint32)==4 ? 1 : -1];
typedef unsigned char mz_validate_uint64[sizeof(mz_uint64)==8 ? 1 : -1];

#include <string.h>
#include <assert.h>

#define MZ_ASSERT(x) assert(x)

#ifdef MINIZ_NO_MALLOC
  #define MZ_MALLOC(x) NULL
  #define MZ_FREE(x) (void)x, ((void)0)
  #define MZ_REALLOC(p, x) NULL
#else
  #define MZ_MALLOC(x) malloc(x)
  #define MZ_FREE(x) free(x)
  #define MZ_REALLOC(p, x) realloc(p, x)
#endif

#define MZ_MAX(a,b) (((a)>(b))?(a):(b))
#define MZ_MIN(a,b) (((a)<(b))?(a):(b))
#define MZ_CLEAR_OBJ(obj) memset(&(obj), 0, sizeof(obj))

#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN
  #define MZ_READ_LE16(p) *((const mz_uint16 *)(p))
  #define MZ_READ_LE32(p) *((const mz_uint32 *)(p))
#else
  #define MZ_READ_LE16(p) ((mz_uint32)(((const mz_uint8 *)(p))[0]) | ((mz_uint32)(((const mz_uint8 *)(p))[1]) << 8U))
  #define MZ_READ_LE32(p) ((mz_uint32)(((const mz_uint8 *)(p))[0]) | ((mz_uint32)(((const mz_uint8 *)(p))[1]) << 8U) | ((mz_uint32)(((const mz_uint8 *)(p))[2]) << 16U) | ((mz_uint32)(((const mz_uint8 *)(p))[3]) << 24U))
#endif

#ifdef _MSC_VER
  #define MZ_FORCEINLINE __forceinline
#elif defined(__GNUC__)
  #define MZ_FORCEINLINE inline __attribute__((__always_inline__))
#else
  #define MZ_FORCEINLINE inline
#endif

#ifdef __cplusplus
  extern "C" {
#endif

// ------------------- zlib-style API's

mz_ulong mz_adler32(mz_ulong adler, const unsigned char *ptr, size_t buf_len)
{
  mz_uint32 i, s1 = (mz_uint32)(adler & 0xffff), s2 = (mz_uint32)(adler >> 16); size_t block_len = buf_len % 5552;
  if (!ptr) return MZ_ADLER32_INIT;
  while (buf_len) {
	for (i = 0; i + 7 < block_len; i += 8, ptr += 8) {
	  s1 += ptr[0], s2 += s1; s1 += ptr[1], s2 += s1; s1 += ptr[2], s2 += s1; s1 += ptr[3], s2 += s1;
	  s1 += ptr[4], s2 += s1; s1 += ptr[5], s2 += s1; s1 += ptr[6], s2 += s1; s1 += ptr[7], s2 += s1;
	}
	for ( ; i < block_len; ++i) s1 += *ptr++, s2 += s1;
	s1 %= 65521U, s2 %= 65521U; buf_len -= block_len; block_len = 5552;
  }
  return (s2 << 16) + s1;
}

// Karl Malbrain's compact CRC-32. See "A compact CCITT crc16 and crc32 C implementation that balances processor cache usage against speed": http://www.geocities.com/malbrain/
mz_ulong mz_crc32(mz_ulong crc, const mz_uint8 *ptr, size_t buf_len)
{
  static const mz_uint32 s_crc32[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
	0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
  mz_uint32 crcu32 = (mz_uint32)crc;
  if (!ptr) return MZ_CRC32_INIT;
  crcu32 = ~crcu32; while (buf_len--) { mz_uint8 b = *ptr++; crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)]; crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)]; }
  return ~crcu32;
}

void mz_free(void *p)
{
  MZ_FREE(p);
}

#ifndef MINIZ_NO_ZLIB_APIS

static void *def_alloc_func(void *opaque, size_t items, size_t size) { (void)opaque, (void)items, (void)size; return MZ_MALLOC(items * size); }
static void def_free_func(void *opaque, void *address) { (void)opaque, (void)address; MZ_FREE(address); }
static void *def_realloc_func(void *opaque, void *address, size_t items, size_t size) { (void)opaque, (void)address, (void)items, (void)size; return MZ_REALLOC(address, items * size); }

const char *mz_version(void)
{
  return MZ_VERSION;
}

int mz_deflateInit(mz_streamp pStream, int level)
{
  return mz_deflateInit2(pStream, level, MZ_DEFLATED, MZ_DEFAULT_WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY);
}

int mz_deflateInit2(mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy)
{
  tdefl_compressor *pComp;
  mz_uint comp_flags = TDEFL_COMPUTE_ADLER32 | tdefl_create_comp_flags_from_zip_params(level, window_bits, strategy);

  if (!pStream) return MZ_STREAM_ERROR;
  if ((method != MZ_DEFLATED) || ((mem_level < 1) || (mem_level > 9)) || ((window_bits != MZ_DEFAULT_WINDOW_BITS) && (-window_bits != MZ_DEFAULT_WINDOW_BITS))) return MZ_PARAM_ERROR;

  pStream->data_type = 0;
  pStream->adler = MZ_ADLER32_INIT;
  pStream->msg = NULL;
  pStream->reserved = 0;
  pStream->total_in = 0;
  pStream->total_out = 0;
  if (!pStream->zalloc) pStream->zalloc = def_alloc_func;
  if (!pStream->zfree) pStream->zfree = def_free_func;

  pComp = (tdefl_compressor *)pStream->zalloc(pStream->opaque, 1, sizeof(tdefl_compressor));
  if (!pComp)
	return MZ_MEM_ERROR;

  pStream->state = (struct mz_internal_state *)pComp;

  if (tdefl_init(pComp, NULL, NULL, comp_flags) != TDEFL_STATUS_OKAY)
  {
	mz_deflateEnd(pStream);
	return MZ_PARAM_ERROR;
  }

  return MZ_OK;
}

int mz_deflateReset(mz_streamp pStream)
{
  if ((!pStream) || (!pStream->state) || (!pStream->zalloc) || (!pStream->zfree)) return MZ_STREAM_ERROR;
  pStream->total_in = pStream->total_out = 0;
  tdefl_init((tdefl_compressor*)pStream->state, NULL, NULL, ((tdefl_compressor*)pStream->state)->m_flags);
  return MZ_OK;
}

int mz_deflate(mz_streamp pStream, int flush)
{
  size_t in_bytes, out_bytes;
  mz_ulong orig_total_in, orig_total_out;
  int mz_status = MZ_OK;

  if ((!pStream) || (!pStream->state) || (flush < 0) || (flush > MZ_FINISH) || (!pStream->next_out)) return MZ_STREAM_ERROR;
  if (!pStream->avail_out) return MZ_BUF_ERROR;

  if (flush == MZ_PARTIAL_FLUSH) flush = MZ_SYNC_FLUSH;

  if (((tdefl_compressor*)pStream->state)->m_prev_return_status == TDEFL_STATUS_DONE)
	return (flush == MZ_FINISH) ? MZ_STREAM_END : MZ_BUF_ERROR;

  orig_total_in = pStream->total_in; orig_total_out = pStream->total_out;
  for ( ; ; )
  {
	tdefl_status defl_status;
	in_bytes = pStream->avail_in; out_bytes = pStream->avail_out;

	defl_status = tdefl_compress((tdefl_compressor*)pStream->state, pStream->next_in, &in_bytes, pStream->next_out, &out_bytes, (tdefl_flush)flush);
	pStream->next_in += (mz_uint)in_bytes; pStream->avail_in -= (mz_uint)in_bytes;
	pStream->total_in += (mz_uint)in_bytes; pStream->adler = tdefl_get_adler32((tdefl_compressor*)pStream->state);

	pStream->next_out += (mz_uint)out_bytes; pStream->avail_out -= (mz_uint)out_bytes;
	pStream->total_out += (mz_uint)out_bytes;

	if (defl_status < 0)
	{
	  mz_status = MZ_STREAM_ERROR;
	  break;
	}
	else if (defl_status == TDEFL_STATUS_DONE)
	{
	  mz_status = MZ_STREAM_END;
	  break;
	}
	else if (!pStream->avail_out)
	  break;
	else if ((!pStream->avail_in) && (flush != MZ_FINISH))
	{
	  if ((flush) || (pStream->total_in != orig_total_in) || (pStream->total_out != orig_total_out))
		break;
	  return MZ_BUF_ERROR; // Can't make forward progress without some input.
	}
  }
  return mz_status;
}

int mz_deflateEnd(mz_streamp pStream)
{
  if (!pStream) return MZ_STREAM_ERROR;
  if (pStream->state)
  {
	pStream->zfree(pStream->opaque, pStream->state);
	pStream->state = NULL;
  }
  return MZ_OK;
}

mz_ulong mz_deflateBound(mz_streamp pStream, mz_ulong source_len)
{
  (void)pStream;
  // This is really over conservative. (And lame, but it's actually pretty tricky to compute a true upper bound given the way tdefl's blocking works.)
  return MZ_MAX(128 + (source_len * 110) / 100, 128 + source_len + ((source_len / (31 * 1024)) + 1) * 5);
}

int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level)
{
  int status;
  mz_stream stream;
  memset(&stream, 0, sizeof(stream));

  // In case mz_ulong is 64-bits (argh I hate longs).
  if ((source_len | *pDest_len) > 0xFFFFFFFFU) return MZ_PARAM_ERROR;

  stream.next_in = pSource;
  stream.avail_in = (mz_uint32)source_len;
  stream.next_out = pDest;
  stream.avail_out = (mz_uint32)*pDest_len;

  status = mz_deflateInit(&stream, level);
  if (status != MZ_OK) return status;

  status = mz_deflate(&stream, MZ_FINISH);
  if (status != MZ_STREAM_END)
  {
	mz_deflateEnd(&stream);
	return (status == MZ_OK) ? MZ_BUF_ERROR : status;
  }

  *pDest_len = stream.total_out;
  return mz_deflateEnd(&stream);
}

int mz_compress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len)
{
  return mz_compress2(pDest, pDest_len, pSource, source_len, MZ_DEFAULT_COMPRESSION);
}

mz_ulong mz_compressBound(mz_ulong source_len)
{
  return mz_deflateBound(NULL, source_len);
}

typedef struct
{
  tinfl_decompressor m_decomp;
  mz_uint m_dict_ofs, m_dict_avail, m_first_call, m_has_flushed; int m_window_bits;
  mz_uint8 m_dict[TINFL_LZ_DICT_SIZE];
  tinfl_status m_last_status;
} inflate_state;

int mz_inflateInit2(mz_streamp pStream, int window_bits)
{
  inflate_state *pDecomp;
  if (!pStream) return MZ_STREAM_ERROR;
  if ((window_bits != MZ_DEFAULT_WINDOW_BITS) && (-window_bits != MZ_DEFAULT_WINDOW_BITS)) return MZ_PARAM_ERROR;

  pStream->data_type = 0;
  pStream->adler = 0;
  pStream->msg = NULL;
  pStream->total_in = 0;
  pStream->total_out = 0;
  pStream->reserved = 0;
  if (!pStream->zalloc) pStream->zalloc = def_alloc_func;
  if (!pStream->zfree) pStream->zfree = def_free_func;

  pDecomp = (inflate_state*)pStream->zalloc(pStream->opaque, 1, sizeof(inflate_state));
  if (!pDecomp) return MZ_MEM_ERROR;

  pStream->state = (struct mz_internal_state *)pDecomp;

  tinfl_init(&pDecomp->m_decomp);
  pDecomp->m_dict_ofs = 0;
  pDecomp->m_dict_avail = 0;
  pDecomp->m_last_status = TINFL_STATUS_NEEDS_MORE_INPUT;
  pDecomp->m_first_call = 1;
  pDecomp->m_has_flushed = 0;
  pDecomp->m_window_bits = window_bits;

  return MZ_OK;
}

int mz_inflateInit(mz_streamp pStream)
{
   return mz_inflateInit2(pStream, MZ_DEFAULT_WINDOW_BITS);
}

int mz_inflate(mz_streamp pStream, int flush)
{
  inflate_state* pState;
  mz_uint n, first_call, decomp_flags = TINFL_FLAG_COMPUTE_ADLER32;
  size_t in_bytes, out_bytes, orig_avail_in;
  tinfl_status status;

  if ((!pStream) || (!pStream->state)) return MZ_STREAM_ERROR;
  if (flush == MZ_PARTIAL_FLUSH) flush = MZ_SYNC_FLUSH;
  if ((flush) && (flush != MZ_SYNC_FLUSH) && (flush != MZ_FINISH)) return MZ_STREAM_ERROR;

  pState = (inflate_state*)pStream->state;
  if (pState->m_window_bits > 0) decomp_flags |= TINFL_FLAG_PARSE_ZLIB_HEADER;
  orig_avail_in = pStream->avail_in;

  first_call = pState->m_first_call; pState->m_first_call = 0;
  if (pState->m_last_status < 0) return MZ_DATA_ERROR;

  if (pState->m_has_flushed && (flush != MZ_FINISH)) return MZ_STREAM_ERROR;
  pState->m_has_flushed |= (flush == MZ_FINISH);

  if ((flush == MZ_FINISH) && (first_call))
  {
	// MZ_FINISH on the first call implies that the input and output buffers are large enough to hold the entire compressed/decompressed file.
	decomp_flags |= TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF;
	in_bytes = pStream->avail_in; out_bytes = pStream->avail_out;
	status = tinfl_decompress(&pState->m_decomp, pStream->next_in, &in_bytes, pStream->next_out, pStream->next_out, &out_bytes, decomp_flags);
	pState->m_last_status = status;
	pStream->next_in += (mz_uint)in_bytes; pStream->avail_in -= (mz_uint)in_bytes; pStream->total_in += (mz_uint)in_bytes;
	pStream->adler = tinfl_get_adler32(&pState->m_decomp);
	pStream->next_out += (mz_uint)out_bytes; pStream->avail_out -= (mz_uint)out_bytes; pStream->total_out += (mz_uint)out_bytes;

	if (status < 0)
	  return MZ_DATA_ERROR;
	else if (status != TINFL_STATUS_DONE)
	{
	  pState->m_last_status = TINFL_STATUS_FAILED;
	  return MZ_BUF_ERROR;
	}
	return MZ_STREAM_END;
  }
  // flush != MZ_FINISH then we must assume there's more input.
  if (flush != MZ_FINISH) decomp_flags |= TINFL_FLAG_HAS_MORE_INPUT;

  if (pState->m_dict_avail)
  {
	n = MZ_MIN(pState->m_dict_avail, pStream->avail_out);
	memcpy(pStream->next_out, pState->m_dict + pState->m_dict_ofs, n);
	pStream->next_out += n; pStream->avail_out -= n; pStream->total_out += n;
	pState->m_dict_avail -= n; pState->m_dict_ofs = (pState->m_dict_ofs + n) & (TINFL_LZ_DICT_SIZE - 1);
	return ((pState->m_last_status == TINFL_STATUS_DONE) && (!pState->m_dict_avail)) ? MZ_STREAM_END : MZ_OK;
  }

  for ( ; ; )
  {
	in_bytes = pStream->avail_in;
	out_bytes = TINFL_LZ_DICT_SIZE - pState->m_dict_ofs;

	status = tinfl_decompress(&pState->m_decomp, pStream->next_in, &in_bytes, pState->m_dict, pState->m_dict + pState->m_dict_ofs, &out_bytes, decomp_flags);
	pState->m_last_status = status;

	pStream->next_in += (mz_uint)in_bytes; pStream->avail_in -= (mz_uint)in_bytes;
	pStream->total_in += (mz_uint)in_bytes; pStream->adler = tinfl_get_adler32(&pState->m_decomp);

	pState->m_dict_avail = (mz_uint)out_bytes;

	n = MZ_MIN(pState->m_dict_avail, pStream->avail_out);
	memcpy(pStream->next_out, pState->m_dict + pState->m_dict_ofs, n);
	pStream->next_out += n; pStream->avail_out -= n; pStream->total_out += n;
	pState->m_dict_avail -= n; pState->m_dict_ofs = (pState->m_dict_ofs + n) & (TINFL_LZ_DICT_SIZE - 1);

	if (status < 0)
	   return MZ_DATA_ERROR; // Stream is corrupted (there could be some uncompressed data left in the output dictionary - oh well).
	else if ((status == TINFL_STATUS_NEEDS_MORE_INPUT) && (!orig_avail_in))
	  return MZ_BUF_ERROR; // Signal caller that we can't make forward progress without supplying more input or by setting flush to MZ_FINISH.
	else if (flush == MZ_FINISH)
	{
	   // The output buffer MUST be large to hold the remaining uncompressed data when flush==MZ_FINISH.
	   if (status == TINFL_STATUS_DONE)
		  return pState->m_dict_avail ? MZ_BUF_ERROR : MZ_STREAM_END;
	   // status here must be TINFL_STATUS_HAS_MORE_OUTPUT, which means there's at least 1 more byte on the way. If there's no more room left in the output buffer then something is wrong.
	   else if (!pStream->avail_out)
		  return MZ_BUF_ERROR;
	}
	else if ((status == TINFL_STATUS_DONE) || (!pStream->avail_in) || (!pStream->avail_out) || (pState->m_dict_avail))
	  break;
  }

  return ((status == TINFL_STATUS_DONE) && (!pState->m_dict_avail)) ? MZ_STREAM_END : MZ_OK;
}

int mz_inflateEnd(mz_streamp pStream)
{
  if (!pStream)
	return MZ_STREAM_ERROR;
  if (pStream->state)
  {
	pStream->zfree(pStream->opaque, pStream->state);
	pStream->state = NULL;
  }
  return MZ_OK;
}

int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len)
{
  mz_stream stream;
  int status;
  memset(&stream, 0, sizeof(stream));

  // In case mz_ulong is 64-bits (argh I hate longs).
  if ((source_len | *pDest_len) > 0xFFFFFFFFU) return MZ_PARAM_ERROR;

  stream.next_in = pSource;
  stream.avail_in = (mz_uint32)source_len;
  stream.next_out = pDest;
  stream.avail_out = (mz_uint32)*pDest_len;

  status = mz_inflateInit(&stream);
  if (status != MZ_OK)
	return status;

  status = mz_inflate(&stream, MZ_FINISH);
  if (status != MZ_STREAM_END)
  {
	mz_inflateEnd(&stream);
	return ((status == MZ_BUF_ERROR) && (!stream.avail_in)) ? MZ_DATA_ERROR : status;
  }
  *pDest_len = stream.total_out;

  return mz_inflateEnd(&stream);
}

const char *mz_error(int err)
{
  static struct { int m_err; const char *m_pDesc; } s_error_descs[] =
  {
	{ MZ_OK, "" }, { MZ_STREAM_END, "stream end" }, { MZ_NEED_DICT, "need dictionary" }, { MZ_ERRNO, "file error" }, { MZ_STREAM_ERROR, "stream error" },
	{ MZ_DATA_ERROR, "data error" }, { MZ_MEM_ERROR, "out of memory" }, { MZ_BUF_ERROR, "buf error" }, { MZ_VERSION_ERROR, "version error" }, { MZ_PARAM_ERROR, "parameter error" }
  };
  mz_uint i; for (i = 0; i < sizeof(s_error_descs) / sizeof(s_error_descs[0]); ++i) if (s_error_descs[i].m_err == err) return s_error_descs[i].m_pDesc;
  return NULL;
}

#endif //MINIZ_NO_ZLIB_APIS

// ------------------- Low-level Decompression (completely independent from all compression API's)

#define TINFL_MEMCPY(d, s, l) memcpy(d, s, l)
#define TINFL_MEMSET(p, c, l) memset(p, c, l)

#define TINFL_CR_BEGIN switch(r->m_state) { case 0:
#define TINFL_CR_RETURN(state_index, result) do { status = result; r->m_state = state_index; goto common_exit; case state_index:; } MZ_MACRO_END
#define TINFL_CR_RETURN_FOREVER(state_index, result) do { for ( ; ; ) { TINFL_CR_RETURN(state_index, result); } } MZ_MACRO_END
#define TINFL_CR_FINISH }

// TODO: If the caller has indicated that there's no more input, and we attempt to read beyond the input buf, then something is wrong with the input because the inflator never
// reads ahead more than it needs to. Currently TINFL_GET_BYTE() pads the end of the stream with 0's in this scenario.
#define TINFL_GET_BYTE(state_index, c) do { \
  if (pIn_buf_cur >= pIn_buf_end) { \
	for ( ; ; ) { \
	  if (decomp_flags & TINFL_FLAG_HAS_MORE_INPUT) { \
		TINFL_CR_RETURN(state_index, TINFL_STATUS_NEEDS_MORE_INPUT); \
		if (pIn_buf_cur < pIn_buf_end) { \
		  c = *pIn_buf_cur++; \
		  break; \
		} \
	  } else { \
		c = 0; \
		break; \
	  } \
	} \
  } else c = *pIn_buf_cur++; } MZ_MACRO_END

#define TINFL_NEED_BITS(state_index, n) do { mz_uint c; TINFL_GET_BYTE(state_index, c); bit_buf |= (((tinfl_bit_buf_t)c) << num_bits); num_bits += 8; } while (num_bits < (mz_uint)(n))
#define TINFL_SKIP_BITS(state_index, n) do { if (num_bits < (mz_uint)(n)) { TINFL_NEED_BITS(state_index, n); } bit_buf >>= (n); num_bits -= (n); } MZ_MACRO_END
#define TINFL_GET_BITS(state_index, b, n) do { if (num_bits < (mz_uint)(n)) { TINFL_NEED_BITS(state_index, n); } b = bit_buf & ((1 << (n)) - 1); bit_buf >>= (n); num_bits -= (n); } MZ_MACRO_END

// TINFL_HUFF_BITBUF_FILL() is only used rarely, when the number of bytes remaining in the input buffer falls below 2.
// It reads just enough bytes from the input stream that are needed to decode the next Huffman code (and absolutely no more). It works by trying to fully decode a
// Huffman code by using whatever bits are currently present in the bit buffer. If this fails, it reads another byte, and tries again until it succeeds or until the
// bit buffer contains >=15 bits (deflate's max. Huffman code size).
#define TINFL_HUFF_BITBUF_FILL(state_index, pHuff) \
  do { \
	temp = (pHuff)->m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]; \
	if (temp >= 0) { \
	  code_len = temp >> 9; \
	  if ((code_len) && (num_bits >= code_len)) \
	  break; \
	} else if (num_bits > TINFL_FAST_LOOKUP_BITS) { \
	   code_len = TINFL_FAST_LOOKUP_BITS; \
	   do { \
		  temp = (pHuff)->m_tree[~temp + ((bit_buf >> code_len++) & 1)]; \
	   } while ((temp < 0) && (num_bits >= (code_len + 1))); if (temp >= 0) break; \
	} TINFL_GET_BYTE(state_index, c); bit_buf |= (((tinfl_bit_buf_t)c) << num_bits); num_bits += 8; \
  } while (num_bits < 15);

// TINFL_HUFF_DECODE() decodes the next Huffman coded symbol. It's more complex than you would initially expect because the zlib API expects the decompressor to never read
// beyond the final byte of the deflate stream. (In other words, when this macro wants to read another byte from the input, it REALLY needs another byte in order to fully
// decode the next Huffman code.) Handling this properly is particularly important on raw deflate (non-zlib) streams, which aren't followed by a byte aligned adler-32.
// The slow path is only executed at the very end of the input buffer.
#define TINFL_HUFF_DECODE(state_index, sym, pHuff) do { \
  int temp; mz_uint code_len, c; \
  if (num_bits < 15) { \
	if ((pIn_buf_end - pIn_buf_cur) < 2) { \
	   TINFL_HUFF_BITBUF_FILL(state_index, pHuff); \
	} else { \
	   bit_buf |= (((tinfl_bit_buf_t)pIn_buf_cur[0]) << num_bits) | (((tinfl_bit_buf_t)pIn_buf_cur[1]) << (num_bits + 8)); pIn_buf_cur += 2; num_bits += 16; \
	} \
  } \
  if ((temp = (pHuff)->m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0) \
	code_len = temp >> 9, temp &= 511; \
  else { \
	code_len = TINFL_FAST_LOOKUP_BITS; do { temp = (pHuff)->m_tree[~temp + ((bit_buf >> code_len++) & 1)]; } while (temp < 0); \
  } sym = temp; bit_buf >>= code_len; num_bits -= code_len; } MZ_MACRO_END

tinfl_status tinfl_decompress(tinfl_decompressor *r, const mz_uint8 *pIn_buf_next, size_t *pIn_buf_size, mz_uint8 *pOut_buf_start, mz_uint8 *pOut_buf_next, size_t *pOut_buf_size, const mz_uint32 decomp_flags)
{
  static const int s_length_base[31] = { 3,4,5,6,7,8,9,10,11,13, 15,17,19,23,27,31,35,43,51,59, 67,83,99,115,131,163,195,227,258,0,0 };
  static const int s_length_extra[31]= { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };
  static const int s_dist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193, 257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};
  static const int s_dist_extra[32] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
  static const mz_uint8 s_length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
  static const int s_min_table_sizes[3] = { 257, 1, 4 };

  tinfl_status status = TINFL_STATUS_FAILED; mz_uint32 num_bits, dist, counter, num_extra; tinfl_bit_buf_t bit_buf;
  const mz_uint8 *pIn_buf_cur = pIn_buf_next, *const pIn_buf_end = pIn_buf_next + *pIn_buf_size;
  mz_uint8 *pOut_buf_cur = pOut_buf_next, *const pOut_buf_end = pOut_buf_next + *pOut_buf_size;
  size_t out_buf_size_mask = (decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF) ? (size_t)-1 : ((pOut_buf_next - pOut_buf_start) + *pOut_buf_size) - 1, dist_from_out_buf_start;

  // Ensure the output buffer's size is a power of 2, unless the output buffer is large enough to hold the entire output file (in which case it doesn't matter).
  if (((out_buf_size_mask + 1) & out_buf_size_mask) || (pOut_buf_next < pOut_buf_start)) { *pIn_buf_size = *pOut_buf_size = 0; return TINFL_STATUS_BAD_PARAM; }

  num_bits = r->m_num_bits; bit_buf = r->m_bit_buf; dist = r->m_dist; counter = r->m_counter; num_extra = r->m_num_extra; dist_from_out_buf_start = r->m_dist_from_out_buf_start;
  TINFL_CR_BEGIN

  bit_buf = num_bits = dist = counter = num_extra = r->m_zhdr0 = r->m_zhdr1 = 0; r->m_z_adler32 = r->m_check_adler32 = 1;
  if (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER)
  {
	TINFL_GET_BYTE(1, r->m_zhdr0); TINFL_GET_BYTE(2, r->m_zhdr1);
	counter = (((r->m_zhdr0 * 256 + r->m_zhdr1) % 31 != 0) || (r->m_zhdr1 & 32) || ((r->m_zhdr0 & 15) != 8));
	if (!(decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)) counter |= (((1U << (8U + (r->m_zhdr0 >> 4))) > 32768U) || ((out_buf_size_mask + 1) < (size_t)(1U << (8U + (r->m_zhdr0 >> 4)))));
	if (counter) { TINFL_CR_RETURN_FOREVER(36, TINFL_STATUS_FAILED); }
  }

  do
  {
	TINFL_GET_BITS(3, r->m_final, 3); r->m_type = r->m_final >> 1;
	if (r->m_type == 0)
	{
	  TINFL_SKIP_BITS(5, num_bits & 7);
	  for (counter = 0; counter < 4; ++counter) { if (num_bits) TINFL_GET_BITS(6, r->m_raw_header[counter], 8); else TINFL_GET_BYTE(7, r->m_raw_header[counter]); }
	  if ((counter = (r->m_raw_header[0] | (r->m_raw_header[1] << 8))) != (mz_uint)(0xFFFF ^ (r->m_raw_header[2] | (r->m_raw_header[3] << 8)))) { TINFL_CR_RETURN_FOREVER(39, TINFL_STATUS_FAILED); }
	  while ((counter) && (num_bits))
	  {
		TINFL_GET_BITS(51, dist, 8);
		while (pOut_buf_cur >= pOut_buf_end) { TINFL_CR_RETURN(52, TINFL_STATUS_HAS_MORE_OUTPUT); }
		*pOut_buf_cur++ = (mz_uint8)dist;
		counter--;
	  }
	  while (counter)
	  {
		size_t n; while (pOut_buf_cur >= pOut_buf_end) { TINFL_CR_RETURN(9, TINFL_STATUS_HAS_MORE_OUTPUT); }
		while (pIn_buf_cur >= pIn_buf_end)
		{
		  if (decomp_flags & TINFL_FLAG_HAS_MORE_INPUT)
		  {
			TINFL_CR_RETURN(38, TINFL_STATUS_NEEDS_MORE_INPUT);
		  }
		  else
		  {
			TINFL_CR_RETURN_FOREVER(40, TINFL_STATUS_FAILED);
		  }
		}
		n = MZ_MIN(MZ_MIN((size_t)(pOut_buf_end - pOut_buf_cur), (size_t)(pIn_buf_end - pIn_buf_cur)), counter);
		TINFL_MEMCPY(pOut_buf_cur, pIn_buf_cur, n); pIn_buf_cur += n; pOut_buf_cur += n; counter -= (mz_uint)n;
	  }
	}
	else if (r->m_type == 3)
	{
	  TINFL_CR_RETURN_FOREVER(10, TINFL_STATUS_FAILED);
	}
	else
	{
	  if (r->m_type == 1)
	  {
		mz_uint8 *p = r->m_tables[0].m_code_size; mz_uint i;
		r->m_table_sizes[0] = 288; r->m_table_sizes[1] = 32; TINFL_MEMSET(r->m_tables[1].m_code_size, 5, 32);
		for ( i = 0; i <= 143; ++i) *p++ = 8; for ( ; i <= 255; ++i) *p++ = 9; for ( ; i <= 279; ++i) *p++ = 7; for ( ; i <= 287; ++i) *p++ = 8;
	  }
	  else
	  {
		for (counter = 0; counter < 3; counter++) { TINFL_GET_BITS(11, r->m_table_sizes[counter], "\05\05\04"[counter]); r->m_table_sizes[counter] += s_min_table_sizes[counter]; }
		MZ_CLEAR_OBJ(r->m_tables[2].m_code_size); for (counter = 0; counter < r->m_table_sizes[2]; counter++) { mz_uint s; TINFL_GET_BITS(14, s, 3); r->m_tables[2].m_code_size[s_length_dezigzag[counter]] = (mz_uint8)s; }
		r->m_table_sizes[2] = 19;
	  }
	  for ( ; (int)r->m_type >= 0; r->m_type--)
	  {
		int tree_next, tree_cur; tinfl_huff_table *pTable;
		mz_uint i, j, used_syms, total, sym_index, next_code[17], total_syms[16]; pTable = &r->m_tables[r->m_type]; MZ_CLEAR_OBJ(total_syms); MZ_CLEAR_OBJ(pTable->m_look_up); MZ_CLEAR_OBJ(pTable->m_tree);
		for (i = 0; i < r->m_table_sizes[r->m_type]; ++i) total_syms[pTable->m_code_size[i]]++;
		used_syms = 0, total = 0; next_code[0] = next_code[1] = 0;
		for (i = 1; i <= 15; ++i) { used_syms += total_syms[i]; next_code[i + 1] = (total = ((total + total_syms[i]) << 1)); }
		if ((65536 != total) && (used_syms > 1))
		{
		  TINFL_CR_RETURN_FOREVER(35, TINFL_STATUS_FAILED);
		}
		for (tree_next = -1, sym_index = 0; sym_index < r->m_table_sizes[r->m_type]; ++sym_index)
		{
		  mz_uint rev_code = 0, l, cur_code, code_size = pTable->m_code_size[sym_index]; if (!code_size) continue;
		  cur_code = next_code[code_size]++; for (l = code_size; l > 0; l--, cur_code >>= 1) rev_code = (rev_code << 1) | (cur_code & 1);
		  if (code_size <= TINFL_FAST_LOOKUP_BITS) { mz_int16 k = (mz_int16)((code_size << 9) | sym_index); while (rev_code < TINFL_FAST_LOOKUP_SIZE) { pTable->m_look_up[rev_code] = k; rev_code += (1 << code_size); } continue; }
		  if (0 == (tree_cur = pTable->m_look_up[rev_code & (TINFL_FAST_LOOKUP_SIZE - 1)])) { pTable->m_look_up[rev_code & (TINFL_FAST_LOOKUP_SIZE - 1)] = (mz_int16)tree_next; tree_cur = tree_next; tree_next -= 2; }
		  rev_code >>= (TINFL_FAST_LOOKUP_BITS - 1);
		  for (j = code_size; j > (TINFL_FAST_LOOKUP_BITS + 1); j--)
		  {
			tree_cur -= ((rev_code >>= 1) & 1);
			if (!pTable->m_tree[-tree_cur - 1]) { pTable->m_tree[-tree_cur - 1] = (mz_int16)tree_next; tree_cur = tree_next; tree_next -= 2; } else tree_cur = pTable->m_tree[-tree_cur - 1];
		  }
		  tree_cur -= ((rev_code >>= 1) & 1); pTable->m_tree[-tree_cur - 1] = (mz_int16)sym_index;
		}
		if (r->m_type == 2)
		{
		  for (counter = 0; counter < (r->m_table_sizes[0] + r->m_table_sizes[1]); )
		  {
			mz_uint s; TINFL_HUFF_DECODE(16, dist, &r->m_tables[2]); if (dist < 16) { r->m_len_codes[counter++] = (mz_uint8)dist; continue; }
			if ((dist == 16) && (!counter))
			{
			  TINFL_CR_RETURN_FOREVER(17, TINFL_STATUS_FAILED);
			}
			num_extra = "\02\03\07"[dist - 16]; TINFL_GET_BITS(18, s, num_extra); s += "\03\03\013"[dist - 16];
			TINFL_MEMSET(r->m_len_codes + counter, (dist == 16) ? r->m_len_codes[counter - 1] : 0, s); counter += s;
		  }
		  if ((r->m_table_sizes[0] + r->m_table_sizes[1]) != counter)
		  {
			TINFL_CR_RETURN_FOREVER(21, TINFL_STATUS_FAILED);
		  }
		  TINFL_MEMCPY(r->m_tables[0].m_code_size, r->m_len_codes, r->m_table_sizes[0]); TINFL_MEMCPY(r->m_tables[1].m_code_size, r->m_len_codes + r->m_table_sizes[0], r->m_table_sizes[1]);
		}
	  }
	  for ( ; ; )
	  {
		mz_uint8 *pSrc;
		for ( ; ; )
		{
		  if (((pIn_buf_end - pIn_buf_cur) < 4) || ((pOut_buf_end - pOut_buf_cur) < 2))
		  {
			TINFL_HUFF_DECODE(23, counter, &r->m_tables[0]);
			if (counter >= 256)
			  break;
			while (pOut_buf_cur >= pOut_buf_end) { TINFL_CR_RETURN(24, TINFL_STATUS_HAS_MORE_OUTPUT); }
			*pOut_buf_cur++ = (mz_uint8)counter;
		  }
		  else
		  {
			int sym2; mz_uint code_len;
#if TINFL_USE_64BIT_BITBUF
			if (num_bits < 30) { bit_buf |= (((tinfl_bit_buf_t)MZ_READ_LE32(pIn_buf_cur)) << num_bits); pIn_buf_cur += 4; num_bits += 32; }
#else
			if (num_bits < 15) { bit_buf |= (((tinfl_bit_buf_t)MZ_READ_LE16(pIn_buf_cur)) << num_bits); pIn_buf_cur += 2; num_bits += 16; }
#endif
			if ((sym2 = r->m_tables[0].m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0)
			  code_len = sym2 >> 9;
			else
			{
			  code_len = TINFL_FAST_LOOKUP_BITS; do { sym2 = r->m_tables[0].m_tree[~sym2 + ((bit_buf >> code_len++) & 1)]; } while (sym2 < 0);
			}
			counter = sym2; bit_buf >>= code_len; num_bits -= code_len;
			if (counter & 256)
			  break;

#if !TINFL_USE_64BIT_BITBUF
			if (num_bits < 15) { bit_buf |= (((tinfl_bit_buf_t)MZ_READ_LE16(pIn_buf_cur)) << num_bits); pIn_buf_cur += 2; num_bits += 16; }
#endif
			if ((sym2 = r->m_tables[0].m_look_up[bit_buf & (TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0)
			  code_len = sym2 >> 9;
			else
			{
			  code_len = TINFL_FAST_LOOKUP_BITS; do { sym2 = r->m_tables[0].m_tree[~sym2 + ((bit_buf >> code_len++) & 1)]; } while (sym2 < 0);
			}
			bit_buf >>= code_len; num_bits -= code_len;

			pOut_buf_cur[0] = (mz_uint8)counter;
			if (sym2 & 256)
			{
			  pOut_buf_cur++;
			  counter = sym2;
			  break;
			}
			pOut_buf_cur[1] = (mz_uint8)sym2;
			pOut_buf_cur += 2;
		  }
		}
		if ((counter &= 511) == 256) break;

		num_extra = s_length_extra[counter - 257]; counter = s_length_base[counter - 257];
		if (num_extra) { mz_uint extra_bits; TINFL_GET_BITS(25, extra_bits, num_extra); counter += extra_bits; }

		TINFL_HUFF_DECODE(26, dist, &r->m_tables[1]);
		num_extra = s_dist_extra[dist]; dist = s_dist_base[dist];
		if (num_extra) { mz_uint extra_bits; TINFL_GET_BITS(27, extra_bits, num_extra); dist += extra_bits; }

		dist_from_out_buf_start = pOut_buf_cur - pOut_buf_start;
		if ((dist > dist_from_out_buf_start) && (decomp_flags & TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF))
		{
		  TINFL_CR_RETURN_FOREVER(37, TINFL_STATUS_FAILED);
		}

		pSrc = pOut_buf_start + ((dist_from_out_buf_start - dist) & out_buf_size_mask);

		if ((MZ_MAX(pOut_buf_cur, pSrc) + counter) > pOut_buf_end)
		{
		  while (counter--)
		  {
			while (pOut_buf_cur >= pOut_buf_end) { TINFL_CR_RETURN(53, TINFL_STATUS_HAS_MORE_OUTPUT); }
			*pOut_buf_cur++ = pOut_buf_start[(dist_from_out_buf_start++ - dist) & out_buf_size_mask];
		  }
		  continue;
		}
#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES
		else if ((counter >= 9) && (counter <= dist))
		{
		  const mz_uint8 *pSrc_end = pSrc + (counter & ~7);
		  do
		  {
			((mz_uint32 *)pOut_buf_cur)[0] = ((const mz_uint32 *)pSrc)[0];
			((mz_uint32 *)pOut_buf_cur)[1] = ((const mz_uint32 *)pSrc)[1];
			pOut_buf_cur += 8;
		  } while ((pSrc += 8) < pSrc_end);
		  if ((counter &= 7) < 3)
		  {
			if (counter)
			{
			  pOut_buf_cur[0] = pSrc[0];
			  if (counter > 1)
				pOut_buf_cur[1] = pSrc[1];
			  pOut_buf_cur += counter;
			}
			continue;
		  }
		}
#endif
		do
		{
		  pOut_buf_cur[0] = pSrc[0];
		  pOut_buf_cur[1] = pSrc[1];
		  pOut_buf_cur[2] = pSrc[2];
		  pOut_buf_cur += 3; pSrc += 3;
		} while ((int)(counter -= 3) > 2);
		if ((int)counter > 0)
		{
		  pOut_buf_cur[0] = pSrc[0];
		  if ((int)counter > 1)
			pOut_buf_cur[1] = pSrc[1];
		  pOut_buf_cur += counter;
		}
	  }
	}
  } while (!(r->m_final & 1));
  if (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER)
  {
	TINFL_SKIP_BITS(32, num_bits & 7); for (counter = 0; counter < 4; ++counter) { mz_uint s; if (num_bits) TINFL_GET_BITS(41, s, 8); else TINFL_GET_BYTE(42, s); r->m_z_adler32 = (r->m_z_adler32 << 8) | s; }
  }
  TINFL_CR_RETURN_FOREVER(34, TINFL_STATUS_DONE);
  TINFL_CR_FINISH

common_exit:
  r->m_num_bits = num_bits; r->m_bit_buf = bit_buf; r->m_dist = dist; r->m_counter = counter; r->m_num_extra = num_extra; r->m_dist_from_out_buf_start = dist_from_out_buf_start;
  *pIn_buf_size = pIn_buf_cur - pIn_buf_next; *pOut_buf_size = pOut_buf_cur - pOut_buf_next;
  if ((decomp_flags & (TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_COMPUTE_ADLER32)) && (status >= 0))
  {
	const mz_uint8 *ptr = pOut_buf_next; size_t buf_len = *pOut_buf_size;
	mz_uint32 i, s1 = r->m_check_adler32 & 0xffff, s2 = r->m_check_adler32 >> 16; size_t block_len = buf_len % 5552;
	while (buf_len)
	{
	  for (i = 0; i + 7 < block_len; i += 8, ptr += 8)
	  {
		s1 += ptr[0], s2 += s1; s1 += ptr[1], s2 += s1; s1 += ptr[2], s2 += s1; s1 += ptr[3], s2 += s1;
		s1 += ptr[4], s2 += s1; s1 += ptr[5], s2 += s1; s1 += ptr[6], s2 += s1; s1 += ptr[7], s2 += s1;
	  }
	  for ( ; i < block_len; ++i) s1 += *ptr++, s2 += s1;
	  s1 %= 65521U, s2 %= 65521U; buf_len -= block_len; block_len = 5552;
	}
	r->m_check_adler32 = (s2 << 16) + s1; if ((status == TINFL_STATUS_DONE) && (decomp_flags & TINFL_FLAG_PARSE_ZLIB_HEADER) && (r->m_check_adler32 != r->m_z_adler32)) status = TINFL_STATUS_ADLER32_MISMATCH;
  }
  return status;
}

// Higher level helper functions.
void *tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags)
{
  tinfl_decompressor decomp; void *pBuf = NULL, *pNew_buf; size_t src_buf_ofs = 0, out_buf_capacity = 0;
  *pOut_len = 0;
  tinfl_init(&decomp);
  for ( ; ; )
  {
	size_t src_buf_size = src_buf_len - src_buf_ofs, dst_buf_size = out_buf_capacity - *pOut_len, new_out_buf_capacity;
	tinfl_status status = tinfl_decompress(&decomp, (const mz_uint8*)pSrc_buf + src_buf_ofs, &src_buf_size, (mz_uint8*)pBuf, pBuf ? (mz_uint8*)pBuf + *pOut_len : NULL, &dst_buf_size,
	  (flags & ~TINFL_FLAG_HAS_MORE_INPUT) | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
	if ((status < 0) || (status == TINFL_STATUS_NEEDS_MORE_INPUT))
	{
	  MZ_FREE(pBuf); *pOut_len = 0; return NULL;
	}
	src_buf_ofs += src_buf_size;
	*pOut_len += dst_buf_size;
	if (status == TINFL_STATUS_DONE) break;
	new_out_buf_capacity = out_buf_capacity * 2; if (new_out_buf_capacity < 128) new_out_buf_capacity = 128;
	pNew_buf = MZ_REALLOC(pBuf, new_out_buf_capacity);
	if (!pNew_buf)
	{
	  MZ_FREE(pBuf); *pOut_len = 0; return NULL;
	}
	pBuf = pNew_buf; out_buf_capacity = new_out_buf_capacity;
  }
  return pBuf;
}

size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags)
{
  tinfl_decompressor decomp; tinfl_status status; tinfl_init(&decomp);
  status = tinfl_decompress(&decomp, (const mz_uint8*)pSrc_buf, &src_buf_len, (mz_uint8*)pOut_buf, (mz_uint8*)pOut_buf, &out_buf_len, (flags & ~TINFL_FLAG_HAS_MORE_INPUT) | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  return (status != TINFL_STATUS_DONE) ? TINFL_DECOMPRESS_MEM_TO_MEM_FAILED : out_buf_len;
}

int tinfl_decompress_mem_to_callback(const void *pIn_buf, size_t *pIn_buf_size, tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
  int result = 0;
  tinfl_decompressor decomp;
  mz_uint8 *pDict = (mz_uint8*)MZ_MALLOC(TINFL_LZ_DICT_SIZE); size_t in_buf_ofs = 0, dict_ofs = 0;
  if (!pDict)
	return TINFL_STATUS_FAILED;
  tinfl_init(&decomp);
  for ( ; ; )
  {
	size_t in_buf_size = *pIn_buf_size - in_buf_ofs, dst_buf_size = TINFL_LZ_DICT_SIZE - dict_ofs;
	tinfl_status status = tinfl_decompress(&decomp, (const mz_uint8*)pIn_buf + in_buf_ofs, &in_buf_size, pDict, pDict + dict_ofs, &dst_buf_size,
	  (flags & ~(TINFL_FLAG_HAS_MORE_INPUT | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)));
	in_buf_ofs += in_buf_size;
	if ((dst_buf_size) && (!(*pPut_buf_func)(pDict + dict_ofs, (int)dst_buf_size, pPut_buf_user)))
	  break;
	if (status != TINFL_STATUS_HAS_MORE_OUTPUT)
	{
	  result = (status == TINFL_STATUS_DONE);
	  break;
	}
	dict_ofs = (dict_ofs + dst_buf_size) & (TINFL_LZ_DICT_SIZE - 1);
  }
  MZ_FREE(pDict);
  *pIn_buf_size = in_buf_ofs;
  return result;
}

// ------------------- Low-level Compression (independent from all decompression API's)

// Purposely making these tables static for faster init and thread safety.
static const mz_uint16 s_tdefl_len_sym[256] = {
  257,258,259,260,261,262,263,264,265,265,266,266,267,267,268,268,269,269,269,269,270,270,270,270,271,271,271,271,272,272,272,272,
  273,273,273,273,273,273,273,273,274,274,274,274,274,274,274,274,275,275,275,275,275,275,275,275,276,276,276,276,276,276,276,276,
  277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
  279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,280,280,280,280,280,280,280,280,280,280,280,280,280,280,280,280,
  281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,
  282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,
  283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,
  284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,285 };

static const mz_uint8 s_tdefl_len_extra[256] = {
  0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,0 };

static const mz_uint8 s_tdefl_small_dist_sym[512] = {
  0,1,2,3,4,4,5,5,6,6,6,6,7,7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,
  11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,
  13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,14,14,14,14,
  14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
  14,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17 };

static const mz_uint8 s_tdefl_small_dist_extra[512] = {
  0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7 };

static const mz_uint8 s_tdefl_large_dist_sym[128] = {
  0,0,18,19,20,20,21,21,22,22,22,22,23,23,23,23,24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,
  26,26,26,26,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
  28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29 };

static const mz_uint8 s_tdefl_large_dist_extra[128] = {
  0,0,8,8,9,9,9,9,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
  12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
  13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13 };

// Radix sorts tdefl_sym_freq[] array by 16-bit key m_key. Returns ptr to sorted values.
typedef struct { mz_uint16 m_key, m_sym_index; } tdefl_sym_freq;
static tdefl_sym_freq* tdefl_radix_sort_syms(mz_uint num_syms, tdefl_sym_freq* pSyms0, tdefl_sym_freq* pSyms1)
{
  mz_uint32 total_passes = 2, pass_shift, pass, i, hist[256 * 2]; tdefl_sym_freq* pCur_syms = pSyms0, *pNew_syms = pSyms1; MZ_CLEAR_OBJ(hist);
  for (i = 0; i < num_syms; i++) { mz_uint freq = pSyms0[i].m_key; hist[freq & 0xFF]++; hist[256 + ((freq >> 8) & 0xFF)]++; }
  while ((total_passes > 1) && (num_syms == hist[(total_passes - 1) * 256])) total_passes--;
  for (pass_shift = 0, pass = 0; pass < total_passes; pass++, pass_shift += 8)
  {
	const mz_uint32* pHist = &hist[pass << 8];
	mz_uint offsets[256], cur_ofs = 0;
	for (i = 0; i < 256; i++) { offsets[i] = cur_ofs; cur_ofs += pHist[i]; }
	for (i = 0; i < num_syms; i++) pNew_syms[offsets[(pCur_syms[i].m_key >> pass_shift) & 0xFF]++] = pCur_syms[i];
	{ tdefl_sym_freq* t = pCur_syms; pCur_syms = pNew_syms; pNew_syms = t; }
  }
  return pCur_syms;
}

// tdefl_calculate_minimum_redundancy() originally written by: Alistair Moffat, alistair@cs.mu.oz.au, Jyrki Katajainen, jyrki@diku.dk, November 1996.
static void tdefl_calculate_minimum_redundancy(tdefl_sym_freq *A, int n)
{
  int root, leaf, next, avbl, used, dpth;
  if (n==0) return; else if (n==1) { A[0].m_key = 1; return; }
  A[0].m_key += A[1].m_key; root = 0; leaf = 2;
  for (next=1; next < n-1; next++)
  {
	if (leaf>=n || A[root].m_key<A[leaf].m_key) { A[next].m_key = A[root].m_key; A[root++].m_key = (mz_uint16)next; } else A[next].m_key = A[leaf++].m_key;
	if (leaf>=n || (root<next && A[root].m_key<A[leaf].m_key)) { A[next].m_key = (mz_uint16)(A[next].m_key + A[root].m_key); A[root++].m_key = (mz_uint16)next; } else A[next].m_key = (mz_uint16)(A[next].m_key + A[leaf++].m_key);
  }
  A[n-2].m_key = 0; for (next=n-3; next>=0; next--) A[next].m_key = A[A[next].m_key].m_key+1;
  avbl = 1; used = dpth = 0; root = n-2; next = n-1;
  while (avbl>0)
  {
	while (root>=0 && (int)A[root].m_key==dpth) { used++; root--; }
	while (avbl>used) { A[next--].m_key = (mz_uint16)(dpth); avbl--; }
	avbl = 2*used; dpth++; used = 0;
  }
}

// Limits canonical Huffman code table's max code size.
enum { TDEFL_MAX_SUPPORTED_HUFF_CODESIZE = 32 };
static void tdefl_huffman_enforce_max_code_size(int *pNum_codes, int code_list_len, int max_code_size)
{
  int i; mz_uint32 total = 0; if (code_list_len <= 1) return;
  for (i = max_code_size + 1; i <= TDEFL_MAX_SUPPORTED_HUFF_CODESIZE; i++) pNum_codes[max_code_size] += pNum_codes[i];
  for (i = max_code_size; i > 0; i--) total += (((mz_uint32)pNum_codes[i]) << (max_code_size - i));
  while (total != (1UL << max_code_size))
  {
	pNum_codes[max_code_size]--;
	for (i = max_code_size - 1; i > 0; i--) if (pNum_codes[i]) { pNum_codes[i]--; pNum_codes[i + 1] += 2; break; }
	total--;
  }
}

static void tdefl_optimize_huffman_table(tdefl_compressor *d, int table_num, int table_len, int code_size_limit, int static_table)
{
  int i, j, l, num_codes[1 + TDEFL_MAX_SUPPORTED_HUFF_CODESIZE]; mz_uint next_code[TDEFL_MAX_SUPPORTED_HUFF_CODESIZE + 1]; MZ_CLEAR_OBJ(num_codes);
  if (static_table)
  {
	for (i = 0; i < table_len; i++) num_codes[d->m_huff_code_sizes[table_num][i]]++;
  }
  else
  {
	tdefl_sym_freq syms0[TDEFL_MAX_HUFF_SYMBOLS], syms1[TDEFL_MAX_HUFF_SYMBOLS], *pSyms;
	int num_used_syms = 0;
	const mz_uint16 *pSym_count = &d->m_huff_count[table_num][0];
	for (i = 0; i < table_len; i++) if (pSym_count[i]) { syms0[num_used_syms].m_key = (mz_uint16)pSym_count[i]; syms0[num_used_syms++].m_sym_index = (mz_uint16)i; }

	pSyms = tdefl_radix_sort_syms(num_used_syms, syms0, syms1); tdefl_calculate_minimum_redundancy(pSyms, num_used_syms);

	for (i = 0; i < num_used_syms; i++) num_codes[pSyms[i].m_key]++;

	tdefl_huffman_enforce_max_code_size(num_codes, num_used_syms, code_size_limit);

	MZ_CLEAR_OBJ(d->m_huff_code_sizes[table_num]); MZ_CLEAR_OBJ(d->m_huff_codes[table_num]);
	for (i = 1, j = num_used_syms; i <= code_size_limit; i++)
	  for (l = num_codes[i]; l > 0; l--) d->m_huff_code_sizes[table_num][pSyms[--j].m_sym_index] = (mz_uint8)(i);
  }

  next_code[1] = 0; for (j = 0, i = 2; i <= code_size_limit; i++) next_code[i] = j = ((j + num_codes[i - 1]) << 1);

  for (i = 0; i < table_len; i++)
  {
	mz_uint rev_code = 0, code, code_size; if ((code_size = d->m_huff_code_sizes[table_num][i]) == 0) continue;
	code = next_code[code_size]++; for (l = code_size; l > 0; l--, code >>= 1) rev_code = (rev_code << 1) | (code & 1);
	d->m_huff_codes[table_num][i] = (mz_uint16)rev_code;
  }
}

#define TDEFL_PUT_BITS(b, l) do { \
  mz_uint bits = b; mz_uint len = l; MZ_ASSERT(bits <= ((1U << len) - 1U)); \
  d->m_bit_buffer |= (bits << d->m_bits_in); d->m_bits_in += len; \
  while (d->m_bits_in >= 8) { \
	if (d->m_pOutput_buf < d->m_pOutput_buf_end) \
	  *d->m_pOutput_buf++ = (mz_uint8)(d->m_bit_buffer); \
	  d->m_bit_buffer >>= 8; \
	  d->m_bits_in -= 8; \
  } \
} MZ_MACRO_END

#define TDEFL_RLE_PREV_CODE_SIZE() { if (rle_repeat_count) { \
  if (rle_repeat_count < 3) { \
	d->m_huff_count[2][prev_code_size] = (mz_uint16)(d->m_huff_count[2][prev_code_size] + rle_repeat_count); \
	while (rle_repeat_count--) packed_code_sizes[num_packed_code_sizes++] = prev_code_size; \
  } else { \
	d->m_huff_count[2][16] = (mz_uint16)(d->m_huff_count[2][16] + 1); packed_code_sizes[num_packed_code_sizes++] = 16; packed_code_sizes[num_packed_code_sizes++] = (mz_uint8)(rle_repeat_count - 3); \
} rle_repeat_count = 0; } }

#define TDEFL_RLE_ZERO_CODE_SIZE() { if (rle_z_count) { \
  if (rle_z_count < 3) { \
	d->m_huff_count[2][0] = (mz_uint16)(d->m_huff_count[2][0] + rle_z_count); while (rle_z_count--) packed_code_sizes[num_packed_code_sizes++] = 0; \
  } else if (rle_z_count <= 10) { \
	d->m_huff_count[2][17] = (mz_uint16)(d->m_huff_count[2][17] + 1); packed_code_sizes[num_packed_code_sizes++] = 17; packed_code_sizes[num_packed_code_sizes++] = (mz_uint8)(rle_z_count - 3); \
  } else { \
	d->m_huff_count[2][18] = (mz_uint16)(d->m_huff_count[2][18] + 1); packed_code_sizes[num_packed_code_sizes++] = 18; packed_code_sizes[num_packed_code_sizes++] = (mz_uint8)(rle_z_count - 11); \
} rle_z_count = 0; } }

static mz_uint8 s_tdefl_packed_code_size_syms_swizzle[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static void tdefl_start_dynamic_block(tdefl_compressor *d)
{
  int num_lit_codes, num_dist_codes, num_bit_lengths; mz_uint i, total_code_sizes_to_pack, num_packed_code_sizes, rle_z_count, rle_repeat_count, packed_code_sizes_index;
  mz_uint8 code_sizes_to_pack[TDEFL_MAX_HUFF_SYMBOLS_0 + TDEFL_MAX_HUFF_SYMBOLS_1], packed_code_sizes[TDEFL_MAX_HUFF_SYMBOLS_0 + TDEFL_MAX_HUFF_SYMBOLS_1], prev_code_size = 0xFF;

  d->m_huff_count[0][256] = 1;

  tdefl_optimize_huffman_table(d, 0, TDEFL_MAX_HUFF_SYMBOLS_0, 15, MZ_FALSE);
  tdefl_optimize_huffman_table(d, 1, TDEFL_MAX_HUFF_SYMBOLS_1, 15, MZ_FALSE);

  for (num_lit_codes = 286; num_lit_codes > 257; num_lit_codes--) if (d->m_huff_code_sizes[0][num_lit_codes - 1]) break;
  for (num_dist_codes = 30; num_dist_codes > 1; num_dist_codes--) if (d->m_huff_code_sizes[1][num_dist_codes - 1]) break;

  memcpy(code_sizes_to_pack, &d->m_huff_code_sizes[0][0], num_lit_codes);
  memcpy(code_sizes_to_pack + num_lit_codes, &d->m_huff_code_sizes[1][0], num_dist_codes);
  total_code_sizes_to_pack = num_lit_codes + num_dist_codes; num_packed_code_sizes = 0; rle_z_count = 0; rle_repeat_count = 0;

  memset(&d->m_huff_count[2][0], 0, sizeof(d->m_huff_count[2][0]) * TDEFL_MAX_HUFF_SYMBOLS_2);
  for (i = 0; i < total_code_sizes_to_pack; i++)
  {
	mz_uint8 code_size = code_sizes_to_pack[i];
	if (!code_size)
	{
	  TDEFL_RLE_PREV_CODE_SIZE();
	  if (++rle_z_count == 138) { TDEFL_RLE_ZERO_CODE_SIZE(); }
	}
	else
	{
	  TDEFL_RLE_ZERO_CODE_SIZE();
	  if (code_size != prev_code_size)
	  {
		TDEFL_RLE_PREV_CODE_SIZE();
		d->m_huff_count[2][code_size] = (mz_uint16)(d->m_huff_count[2][code_size] + 1); packed_code_sizes[num_packed_code_sizes++] = code_size;
	  }
	  else if (++rle_repeat_count == 6)
	  {
		TDEFL_RLE_PREV_CODE_SIZE();
	  }
	}
	prev_code_size = code_size;
  }
  if (rle_repeat_count) { TDEFL_RLE_PREV_CODE_SIZE(); } else { TDEFL_RLE_ZERO_CODE_SIZE(); }

  tdefl_optimize_huffman_table(d, 2, TDEFL_MAX_HUFF_SYMBOLS_2, 7, MZ_FALSE);

  TDEFL_PUT_BITS(2, 2);

  TDEFL_PUT_BITS(num_lit_codes - 257, 5);
  TDEFL_PUT_BITS(num_dist_codes - 1, 5);

  for (num_bit_lengths = 18; num_bit_lengths >= 0; num_bit_lengths--) if (d->m_huff_code_sizes[2][s_tdefl_packed_code_size_syms_swizzle[num_bit_lengths]]) break;
  num_bit_lengths = MZ_MAX(4, (num_bit_lengths + 1)); TDEFL_PUT_BITS(num_bit_lengths - 4, 4);
  for (i = 0; (int)i < num_bit_lengths; i++) TDEFL_PUT_BITS(d->m_huff_code_sizes[2][s_tdefl_packed_code_size_syms_swizzle[i]], 3);

  for (packed_code_sizes_index = 0; packed_code_sizes_index < num_packed_code_sizes; )
  {
	mz_uint code = packed_code_sizes[packed_code_sizes_index++]; MZ_ASSERT(code < TDEFL_MAX_HUFF_SYMBOLS_2);
	TDEFL_PUT_BITS(d->m_huff_codes[2][code], d->m_huff_code_sizes[2][code]);
	if (code >= 16) TDEFL_PUT_BITS(packed_code_sizes[packed_code_sizes_index++], "\02\03\07"[code - 16]);
  }
}

static void tdefl_start_static_block(tdefl_compressor *d)
{
  mz_uint i;
  mz_uint8 *p = &d->m_huff_code_sizes[0][0];

  for (i = 0; i <= 143; ++i) *p++ = 8;
  for ( ; i <= 255; ++i) *p++ = 9;
  for ( ; i <= 279; ++i) *p++ = 7;
  for ( ; i <= 287; ++i) *p++ = 8;

  memset(d->m_huff_code_sizes[1], 5, 32);

  tdefl_optimize_huffman_table(d, 0, 288, 15, MZ_TRUE);
  tdefl_optimize_huffman_table(d, 1, 32, 15, MZ_TRUE);

  TDEFL_PUT_BITS(1, 2);
}

static const mz_uint mz_bitmasks[17] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN && MINIZ_HAS_64BIT_REGISTERS
static mz_bool tdefl_compress_lz_codes(tdefl_compressor *d)
{
  mz_uint flags;
  mz_uint8 *pLZ_codes;
  mz_uint8 *pOutput_buf = d->m_pOutput_buf;
  mz_uint8 *pLZ_code_buf_end = d->m_pLZ_code_buf;
  mz_uint64 bit_buffer = d->m_bit_buffer;
  mz_uint bits_in = d->m_bits_in;

#define TDEFL_PUT_BITS_FAST(b, l) { bit_buffer |= (((mz_uint64)(b)) << bits_in); bits_in += (l); }

  flags = 1;
  for (pLZ_codes = d->m_lz_code_buf; pLZ_codes < pLZ_code_buf_end; flags >>= 1)
  {
	if (flags == 1)
	  flags = *pLZ_codes++ | 0x100;

	if (flags & 1)
	{
	  mz_uint s0, s1, n0, n1, sym, num_extra_bits;
	  mz_uint match_len = pLZ_codes[0], match_dist = *(const mz_uint16 *)(pLZ_codes + 1); pLZ_codes += 3;

	  MZ_ASSERT(d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
	  TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][s_tdefl_len_sym[match_len]], d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
	  TDEFL_PUT_BITS_FAST(match_len & mz_bitmasks[s_tdefl_len_extra[match_len]], s_tdefl_len_extra[match_len]);

	  // This sequence coaxes MSVC into using cmov's vs. jmp's.
	  s0 = s_tdefl_small_dist_sym[match_dist & 511];
	  n0 = s_tdefl_small_dist_extra[match_dist & 511];
	  s1 = s_tdefl_large_dist_sym[match_dist >> 8];
	  n1 = s_tdefl_large_dist_extra[match_dist >> 8];
	  sym = (match_dist < 512) ? s0 : s1;
	  num_extra_bits = (match_dist < 512) ? n0 : n1;

	  MZ_ASSERT(d->m_huff_code_sizes[1][sym]);
	  TDEFL_PUT_BITS_FAST(d->m_huff_codes[1][sym], d->m_huff_code_sizes[1][sym]);
	  TDEFL_PUT_BITS_FAST(match_dist & mz_bitmasks[num_extra_bits], num_extra_bits);
	}
	else
	{
	  mz_uint lit = *pLZ_codes++;
	  MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
	  TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);

	  if (((flags & 2) == 0) && (pLZ_codes < pLZ_code_buf_end))
	  {
		flags >>= 1;
		lit = *pLZ_codes++;
		MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
		TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);

		if (((flags & 2) == 0) && (pLZ_codes < pLZ_code_buf_end))
		{
		  flags >>= 1;
		  lit = *pLZ_codes++;
		  MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
		  TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);
		}
	  }
	}

	if (pOutput_buf >= d->m_pOutput_buf_end)
	  return MZ_FALSE;

	*(mz_uint64*)pOutput_buf = bit_buffer;
	pOutput_buf += (bits_in >> 3);
	bit_buffer >>= (bits_in & ~7);
	bits_in &= 7;
  }

#undef TDEFL_PUT_BITS_FAST

  d->m_pOutput_buf = pOutput_buf;
  d->m_bits_in = 0;
  d->m_bit_buffer = 0;

  while (bits_in)
  {
	mz_uint32 n = MZ_MIN(bits_in, 16);
	TDEFL_PUT_BITS((mz_uint)bit_buffer & mz_bitmasks[n], n);
	bit_buffer >>= n;
	bits_in -= n;
  }

  TDEFL_PUT_BITS(d->m_huff_codes[0][256], d->m_huff_code_sizes[0][256]);

  return (d->m_pOutput_buf < d->m_pOutput_buf_end);
}
#else
static mz_bool tdefl_compress_lz_codes(tdefl_compressor *d)
{
  mz_uint flags;
  mz_uint8 *pLZ_codes;

  flags = 1;
  for (pLZ_codes = d->m_lz_code_buf; pLZ_codes < d->m_pLZ_code_buf; flags >>= 1)
  {
	if (flags == 1)
	  flags = *pLZ_codes++ | 0x100;
	if (flags & 1)
	{
	  mz_uint sym, num_extra_bits;
	  mz_uint match_len = pLZ_codes[0], match_dist = (pLZ_codes[1] | (pLZ_codes[2] << 8)); pLZ_codes += 3;

	  MZ_ASSERT(d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
	  TDEFL_PUT_BITS(d->m_huff_codes[0][s_tdefl_len_sym[match_len]], d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
	  TDEFL_PUT_BITS(match_len & mz_bitmasks[s_tdefl_len_extra[match_len]], s_tdefl_len_extra[match_len]);

	  if (match_dist < 512)
	  {
		sym = s_tdefl_small_dist_sym[match_dist]; num_extra_bits = s_tdefl_small_dist_extra[match_dist];
	  }
	  else
	  {
		sym = s_tdefl_large_dist_sym[match_dist >> 8]; num_extra_bits = s_tdefl_large_dist_extra[match_dist >> 8];
	  }
	  MZ_ASSERT(d->m_huff_code_sizes[1][sym]);
	  TDEFL_PUT_BITS(d->m_huff_codes[1][sym], d->m_huff_code_sizes[1][sym]);
	  TDEFL_PUT_BITS(match_dist & mz_bitmasks[num_extra_bits], num_extra_bits);
	}
	else
	{
	  mz_uint lit = *pLZ_codes++;
	  MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
	  TDEFL_PUT_BITS(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);
	}
  }

  TDEFL_PUT_BITS(d->m_huff_codes[0][256], d->m_huff_code_sizes[0][256]);

  return (d->m_pOutput_buf < d->m_pOutput_buf_end);
}
#endif // MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN && MINIZ_HAS_64BIT_REGISTERS

static mz_bool tdefl_compress_block(tdefl_compressor *d, mz_bool static_block)
{
  if (static_block)
	tdefl_start_static_block(d);
  else
	tdefl_start_dynamic_block(d);
  return tdefl_compress_lz_codes(d);
}

static int tdefl_flush_block(tdefl_compressor *d, int flush)
{
  mz_uint saved_bit_buf, saved_bits_in;
  mz_uint8 *pSaved_output_buf;
  mz_bool comp_block_succeeded = MZ_FALSE;
  int n, use_raw_block = ((d->m_flags & TDEFL_FORCE_ALL_RAW_BLOCKS) != 0) && (d->m_lookahead_pos - d->m_lz_code_buf_dict_pos) <= d->m_dict_size;
  mz_uint8 *pOutput_buf_start = ((d->m_pPut_buf_func == NULL) && ((*d->m_pOut_buf_size - d->m_out_buf_ofs) >= TDEFL_OUT_BUF_SIZE)) ? ((mz_uint8 *)d->m_pOut_buf + d->m_out_buf_ofs) : d->m_output_buf;

  d->m_pOutput_buf = pOutput_buf_start;
  d->m_pOutput_buf_end = d->m_pOutput_buf + TDEFL_OUT_BUF_SIZE - 16;

  MZ_ASSERT(!d->m_output_flush_remaining);
  d->m_output_flush_ofs = 0;
  d->m_output_flush_remaining = 0;

  *d->m_pLZ_flags = (mz_uint8)(*d->m_pLZ_flags >> d->m_num_flags_left);
  d->m_pLZ_code_buf -= (d->m_num_flags_left == 8);

  if ((d->m_flags & TDEFL_WRITE_ZLIB_HEADER) && (!d->m_block_index))
  {
	TDEFL_PUT_BITS(0x78, 8); TDEFL_PUT_BITS(0x01, 8);
  }

  TDEFL_PUT_BITS(flush == TDEFL_FINISH, 1);

  pSaved_output_buf = d->m_pOutput_buf; saved_bit_buf = d->m_bit_buffer; saved_bits_in = d->m_bits_in;

  if (!use_raw_block)
	comp_block_succeeded = tdefl_compress_block(d, (d->m_flags & TDEFL_FORCE_ALL_STATIC_BLOCKS) || (d->m_total_lz_bytes < 48));

  // If the block gets expanded, forget the current contents of the output buffer and send a raw block instead.
  if ( ((use_raw_block) || ((d->m_total_lz_bytes) && ((d->m_pOutput_buf - pSaved_output_buf + 1U) >= d->m_total_lz_bytes))) &&
	   ((d->m_lookahead_pos - d->m_lz_code_buf_dict_pos) <= d->m_dict_size) )
  {
	mz_uint i; d->m_pOutput_buf = pSaved_output_buf; d->m_bit_buffer = saved_bit_buf, d->m_bits_in = saved_bits_in;
	TDEFL_PUT_BITS(0, 2);
	if (d->m_bits_in) { TDEFL_PUT_BITS(0, 8 - d->m_bits_in); }
	for (i = 2; i; --i, d->m_total_lz_bytes ^= 0xFFFF)
	{
	  TDEFL_PUT_BITS(d->m_total_lz_bytes & 0xFFFF, 16);
	}
	for (i = 0; i < d->m_total_lz_bytes; ++i)
	{
	  TDEFL_PUT_BITS(d->m_dict[(d->m_lz_code_buf_dict_pos + i) & TDEFL_LZ_DICT_SIZE_MASK], 8);
	}
  }
  // Check for the extremely unlikely (if not impossible) case of the compressed block not fitting into the output buffer when using dynamic codes.
  else if (!comp_block_succeeded)
  {
	d->m_pOutput_buf = pSaved_output_buf; d->m_bit_buffer = saved_bit_buf, d->m_bits_in = saved_bits_in;
	tdefl_compress_block(d, MZ_TRUE);
  }

  if (flush)
  {
	if (flush == TDEFL_FINISH)
	{
	  if (d->m_bits_in) { TDEFL_PUT_BITS(0, 8 - d->m_bits_in); }
	  if (d->m_flags & TDEFL_WRITE_ZLIB_HEADER) { mz_uint i, a = d->m_adler32; for (i = 0; i < 4; i++) { TDEFL_PUT_BITS((a >> 24) & 0xFF, 8); a <<= 8; } }
	}
	else
	{
	  mz_uint i, z = 0; TDEFL_PUT_BITS(0, 3); if (d->m_bits_in) { TDEFL_PUT_BITS(0, 8 - d->m_bits_in); } for (i = 2; i; --i, z ^= 0xFFFF) { TDEFL_PUT_BITS(z & 0xFFFF, 16); }
	}
  }

  MZ_ASSERT(d->m_pOutput_buf < d->m_pOutput_buf_end);

  memset(&d->m_huff_count[0][0], 0, sizeof(d->m_huff_count[0][0]) * TDEFL_MAX_HUFF_SYMBOLS_0);
  memset(&d->m_huff_count[1][0], 0, sizeof(d->m_huff_count[1][0]) * TDEFL_MAX_HUFF_SYMBOLS_1);

  d->m_pLZ_code_buf = d->m_lz_code_buf + 1; d->m_pLZ_flags = d->m_lz_code_buf; d->m_num_flags_left = 8; d->m_lz_code_buf_dict_pos += d->m_total_lz_bytes; d->m_total_lz_bytes = 0; d->m_block_index++;

  if ((n = (int)(d->m_pOutput_buf - pOutput_buf_start)) != 0)
  {
	if (d->m_pPut_buf_func)
	{
	  *d->m_pIn_buf_size = d->m_pSrc - (const mz_uint8 *)d->m_pIn_buf;
	  if (!(*d->m_pPut_buf_func)(d->m_output_buf, n, d->m_pPut_buf_user))
		return (d->m_prev_return_status = TDEFL_STATUS_PUT_BUF_FAILED);
	}
	else if (pOutput_buf_start == d->m_output_buf)
	{
	  int bytes_to_copy = (int)MZ_MIN((size_t)n, (size_t)(*d->m_pOut_buf_size - d->m_out_buf_ofs));
	  memcpy((mz_uint8 *)d->m_pOut_buf + d->m_out_buf_ofs, d->m_output_buf, bytes_to_copy);
	  d->m_out_buf_ofs += bytes_to_copy;
	  if ((n -= bytes_to_copy) != 0)
	  {
		d->m_output_flush_ofs = bytes_to_copy;
		d->m_output_flush_remaining = n;
	  }
	}
	else
	{
	  d->m_out_buf_ofs += n;
	}
  }

  return d->m_output_flush_remaining;
}

#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES
#define TDEFL_READ_UNALIGNED_WORD(p) *(const mz_uint16*)(p)
static MZ_FORCEINLINE void tdefl_find_match(tdefl_compressor *d, mz_uint lookahead_pos, mz_uint max_dist, mz_uint max_match_len, mz_uint *pMatch_dist, mz_uint *pMatch_len)
{
  mz_uint dist, pos = lookahead_pos & TDEFL_LZ_DICT_SIZE_MASK, match_len = *pMatch_len, probe_pos = pos, next_probe_pos, probe_len;
  mz_uint num_probes_left = d->m_max_probes[match_len >= 32];
  const mz_uint16 *s = (const mz_uint16*)(d->m_dict + pos), *p, *q;
  mz_uint16 c01 = TDEFL_READ_UNALIGNED_WORD(&d->m_dict[pos + match_len - 1]), s01 = TDEFL_READ_UNALIGNED_WORD(s);
  MZ_ASSERT(max_match_len <= TDEFL_MAX_MATCH_LEN); if (max_match_len <= match_len) return;
  for ( ; ; )
  {
	for ( ; ; )
	{
	  if (--num_probes_left == 0) return;
	  #define TDEFL_PROBE \
		next_probe_pos = d->m_next[probe_pos]; \
		if ((!next_probe_pos) || ((dist = (mz_uint16)(lookahead_pos - next_probe_pos)) > max_dist)) return; \
		probe_pos = next_probe_pos & TDEFL_LZ_DICT_SIZE_MASK; \
		if (TDEFL_READ_UNALIGNED_WORD(&d->m_dict[probe_pos + match_len - 1]) == c01) break;
	  TDEFL_PROBE; TDEFL_PROBE; TDEFL_PROBE;
	}
	if (!dist) break; q = (const mz_uint16*)(d->m_dict + probe_pos); if (TDEFL_READ_UNALIGNED_WORD(q) != s01) continue; p = s; probe_len = 32;
	do { } while ( (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) && (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) &&
				   (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) && (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) && (--probe_len > 0) );
	if (!probe_len)
	{
	  *pMatch_dist = dist; *pMatch_len = MZ_MIN(max_match_len, TDEFL_MAX_MATCH_LEN); break;
	}
	else if ((probe_len = ((mz_uint)(p - s) * 2) + (mz_uint)(*(const mz_uint8*)p == *(const mz_uint8*)q)) > match_len)
	{
	  *pMatch_dist = dist; if ((*pMatch_len = match_len = MZ_MIN(max_match_len, probe_len)) == max_match_len) break;
	  c01 = TDEFL_READ_UNALIGNED_WORD(&d->m_dict[pos + match_len - 1]);
	}
  }
}
#else
static MZ_FORCEINLINE void tdefl_find_match(tdefl_compressor *d, mz_uint lookahead_pos, mz_uint max_dist, mz_uint max_match_len, mz_uint *pMatch_dist, mz_uint *pMatch_len)
{
  mz_uint dist, pos = lookahead_pos & TDEFL_LZ_DICT_SIZE_MASK, match_len = *pMatch_len, probe_pos = pos, next_probe_pos, probe_len;
  mz_uint num_probes_left = d->m_max_probes[match_len >= 32];
  const mz_uint8 *s = d->m_dict + pos, *p, *q;
  mz_uint8 c0 = d->m_dict[pos + match_len], c1 = d->m_dict[pos + match_len - 1];
  MZ_ASSERT(max_match_len <= TDEFL_MAX_MATCH_LEN); if (max_match_len <= match_len) return;
  for ( ; ; )
  {
	for ( ; ; )
	{
	  if (--num_probes_left == 0) return;
	  #define TDEFL_PROBE \
		next_probe_pos = d->m_next[probe_pos]; \
		if ((!next_probe_pos) || ((dist = (mz_uint16)(lookahead_pos - next_probe_pos)) > max_dist)) return; \
		probe_pos = next_probe_pos & TDEFL_LZ_DICT_SIZE_MASK; \
		if ((d->m_dict[probe_pos + match_len] == c0) && (d->m_dict[probe_pos + match_len - 1] == c1)) break;
	  TDEFL_PROBE; TDEFL_PROBE; TDEFL_PROBE;
	}
	if (!dist) break; p = s; q = d->m_dict + probe_pos; for (probe_len = 0; probe_len < max_match_len; probe_len++) if (*p++ != *q++) break;
	if (probe_len > match_len)
	{
	  *pMatch_dist = dist; if ((*pMatch_len = match_len = probe_len) == max_match_len) return;
	  c0 = d->m_dict[pos + match_len]; c1 = d->m_dict[pos + match_len - 1];
	}
  }
}
#endif // #if MINIZ_USE_UNALIGNED_LOADS_AND_STORES

#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN
static mz_bool tdefl_compress_fast(tdefl_compressor *d)
{
  // Faster, minimally featured LZRW1-style match+parse loop with better register utilization. Intended for applications where raw throughput is valued more highly than ratio.
  mz_uint lookahead_pos = d->m_lookahead_pos, lookahead_size = d->m_lookahead_size, dict_size = d->m_dict_size, total_lz_bytes = d->m_total_lz_bytes, num_flags_left = d->m_num_flags_left;
  mz_uint8 *pLZ_code_buf = d->m_pLZ_code_buf, *pLZ_flags = d->m_pLZ_flags;
  mz_uint cur_pos = lookahead_pos & TDEFL_LZ_DICT_SIZE_MASK;

  while ((d->m_src_buf_left) || ((d->m_flush) && (lookahead_size)))
  {
	const mz_uint TDEFL_COMP_FAST_LOOKAHEAD_SIZE = 4096;
	mz_uint dst_pos = (lookahead_pos + lookahead_size) & TDEFL_LZ_DICT_SIZE_MASK;
	mz_uint num_bytes_to_process = (mz_uint)MZ_MIN(d->m_src_buf_left, TDEFL_COMP_FAST_LOOKAHEAD_SIZE - lookahead_size);
	d->m_src_buf_left -= num_bytes_to_process;
	lookahead_size += num_bytes_to_process;

	while (num_bytes_to_process)
	{
	  mz_uint32 n = MZ_MIN(TDEFL_LZ_DICT_SIZE - dst_pos, num_bytes_to_process);
	  memcpy(d->m_dict + dst_pos, d->m_pSrc, n);
	  if (dst_pos < (TDEFL_MAX_MATCH_LEN - 1))
		memcpy(d->m_dict + TDEFL_LZ_DICT_SIZE + dst_pos, d->m_pSrc, MZ_MIN(n, (TDEFL_MAX_MATCH_LEN - 1) - dst_pos));
	  d->m_pSrc += n;
	  dst_pos = (dst_pos + n) & TDEFL_LZ_DICT_SIZE_MASK;
	  num_bytes_to_process -= n;
	}

	dict_size = MZ_MIN(TDEFL_LZ_DICT_SIZE - lookahead_size, dict_size);
	if ((!d->m_flush) && (lookahead_size < TDEFL_COMP_FAST_LOOKAHEAD_SIZE)) break;

	while (lookahead_size >= 4)
	{
	  mz_uint cur_match_dist, cur_match_len = 1;
	  mz_uint8 *pCur_dict = d->m_dict + cur_pos;
	  mz_uint first_trigram = (*(const mz_uint32 *)pCur_dict) & 0xFFFFFF;
	  mz_uint hash = (first_trigram ^ (first_trigram >> (24 - (TDEFL_LZ_HASH_BITS - 8)))) & TDEFL_LEVEL1_HASH_SIZE_MASK;
	  mz_uint probe_pos = d->m_hash[hash];
	  d->m_hash[hash] = (mz_uint16)lookahead_pos;

	  if (((cur_match_dist = (mz_uint16)(lookahead_pos - probe_pos)) <= dict_size) && ((*(const mz_uint32 *)(d->m_dict + (probe_pos &= TDEFL_LZ_DICT_SIZE_MASK)) & 0xFFFFFF) == first_trigram))
	  {
		const mz_uint16 *p = (const mz_uint16 *)pCur_dict;
		const mz_uint16 *q = (const mz_uint16 *)(d->m_dict + probe_pos);
		mz_uint32 probe_len = 32;
		do { } while ( (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) && (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) &&
		  (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) && (TDEFL_READ_UNALIGNED_WORD(++p) == TDEFL_READ_UNALIGNED_WORD(++q)) && (--probe_len > 0) );
		cur_match_len = ((mz_uint)(p - (const mz_uint16 *)pCur_dict) * 2) + (mz_uint)(*(const mz_uint8 *)p == *(const mz_uint8 *)q);
		if (!probe_len)
		  cur_match_len = cur_match_dist ? TDEFL_MAX_MATCH_LEN : 0;

		if ((cur_match_len < TDEFL_MIN_MATCH_LEN) || ((cur_match_len == TDEFL_MIN_MATCH_LEN) && (cur_match_dist >= 8U*1024U)))
		{
		  cur_match_len = 1;
		  *pLZ_code_buf++ = (mz_uint8)first_trigram;
		  *pLZ_flags = (mz_uint8)(*pLZ_flags >> 1);
		  d->m_huff_count[0][(mz_uint8)first_trigram]++;
		}
		else
		{
		  mz_uint32 s0, s1;
		  cur_match_len = MZ_MIN(cur_match_len, lookahead_size);

		  MZ_ASSERT((cur_match_len >= TDEFL_MIN_MATCH_LEN) && (cur_match_dist >= 1) && (cur_match_dist <= TDEFL_LZ_DICT_SIZE));

		  cur_match_dist--;

		  pLZ_code_buf[0] = (mz_uint8)(cur_match_len - TDEFL_MIN_MATCH_LEN);
		  *(mz_uint16 *)(&pLZ_code_buf[1]) = (mz_uint16)cur_match_dist;
		  pLZ_code_buf += 3;
		  *pLZ_flags = (mz_uint8)((*pLZ_flags >> 1) | 0x80);

		  s0 = s_tdefl_small_dist_sym[cur_match_dist & 511];
		  s1 = s_tdefl_large_dist_sym[cur_match_dist >> 8];
		  d->m_huff_count[1][(cur_match_dist < 512) ? s0 : s1]++;

		  d->m_huff_count[0][s_tdefl_len_sym[cur_match_len - TDEFL_MIN_MATCH_LEN]]++;
		}
	  }
	  else
	  {
		*pLZ_code_buf++ = (mz_uint8)first_trigram;
		*pLZ_flags = (mz_uint8)(*pLZ_flags >> 1);
		d->m_huff_count[0][(mz_uint8)first_trigram]++;
	  }

	  if (--num_flags_left == 0) { num_flags_left = 8; pLZ_flags = pLZ_code_buf++; }

	  total_lz_bytes += cur_match_len;
	  lookahead_pos += cur_match_len;
	  dict_size = MZ_MIN(dict_size + cur_match_len, TDEFL_LZ_DICT_SIZE);
	  cur_pos = (cur_pos + cur_match_len) & TDEFL_LZ_DICT_SIZE_MASK;
	  MZ_ASSERT(lookahead_size >= cur_match_len);
	  lookahead_size -= cur_match_len;

	  if (pLZ_code_buf > &d->m_lz_code_buf[TDEFL_LZ_CODE_BUF_SIZE - 8])
	  {
		int n;
		d->m_lookahead_pos = lookahead_pos; d->m_lookahead_size = lookahead_size; d->m_dict_size = dict_size;
		d->m_total_lz_bytes = total_lz_bytes; d->m_pLZ_code_buf = pLZ_code_buf; d->m_pLZ_flags = pLZ_flags; d->m_num_flags_left = num_flags_left;
		if ((n = tdefl_flush_block(d, 0)) != 0)
		  return (n < 0) ? MZ_FALSE : MZ_TRUE;
		total_lz_bytes = d->m_total_lz_bytes; pLZ_code_buf = d->m_pLZ_code_buf; pLZ_flags = d->m_pLZ_flags; num_flags_left = d->m_num_flags_left;
	  }
	}

	while (lookahead_size)
	{
	  mz_uint8 lit = d->m_dict[cur_pos];

	  total_lz_bytes++;
	  *pLZ_code_buf++ = lit;
	  *pLZ_flags = (mz_uint8)(*pLZ_flags >> 1);
	  if (--num_flags_left == 0) { num_flags_left = 8; pLZ_flags = pLZ_code_buf++; }

	  d->m_huff_count[0][lit]++;

	  lookahead_pos++;
	  dict_size = MZ_MIN(dict_size + 1, TDEFL_LZ_DICT_SIZE);
	  cur_pos = (cur_pos + 1) & TDEFL_LZ_DICT_SIZE_MASK;
	  lookahead_size--;

	  if (pLZ_code_buf > &d->m_lz_code_buf[TDEFL_LZ_CODE_BUF_SIZE - 8])
	  {
		int n;
		d->m_lookahead_pos = lookahead_pos; d->m_lookahead_size = lookahead_size; d->m_dict_size = dict_size;
		d->m_total_lz_bytes = total_lz_bytes; d->m_pLZ_code_buf = pLZ_code_buf; d->m_pLZ_flags = pLZ_flags; d->m_num_flags_left = num_flags_left;
		if ((n = tdefl_flush_block(d, 0)) != 0)
		  return (n < 0) ? MZ_FALSE : MZ_TRUE;
		total_lz_bytes = d->m_total_lz_bytes; pLZ_code_buf = d->m_pLZ_code_buf; pLZ_flags = d->m_pLZ_flags; num_flags_left = d->m_num_flags_left;
	  }
	}
  }

  d->m_lookahead_pos = lookahead_pos; d->m_lookahead_size = lookahead_size; d->m_dict_size = dict_size;
  d->m_total_lz_bytes = total_lz_bytes; d->m_pLZ_code_buf = pLZ_code_buf; d->m_pLZ_flags = pLZ_flags; d->m_num_flags_left = num_flags_left;
  return MZ_TRUE;
}
#endif // MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN

static MZ_FORCEINLINE void tdefl_record_literal(tdefl_compressor *d, mz_uint8 lit)
{
  d->m_total_lz_bytes++;
  *d->m_pLZ_code_buf++ = lit;
  *d->m_pLZ_flags = (mz_uint8)(*d->m_pLZ_flags >> 1); if (--d->m_num_flags_left == 0) { d->m_num_flags_left = 8; d->m_pLZ_flags = d->m_pLZ_code_buf++; }
  d->m_huff_count[0][lit]++;
}

static MZ_FORCEINLINE void tdefl_record_match(tdefl_compressor *d, mz_uint match_len, mz_uint match_dist)
{
  mz_uint32 s0, s1;

  MZ_ASSERT((match_len >= TDEFL_MIN_MATCH_LEN) && (match_dist >= 1) && (match_dist <= TDEFL_LZ_DICT_SIZE));

  d->m_total_lz_bytes += match_len;

  d->m_pLZ_code_buf[0] = (mz_uint8)(match_len - TDEFL_MIN_MATCH_LEN);

  match_dist -= 1;
  d->m_pLZ_code_buf[1] = (mz_uint8)(match_dist & 0xFF);
  d->m_pLZ_code_buf[2] = (mz_uint8)(match_dist >> 8); d->m_pLZ_code_buf += 3;

  *d->m_pLZ_flags = (mz_uint8)((*d->m_pLZ_flags >> 1) | 0x80); if (--d->m_num_flags_left == 0) { d->m_num_flags_left = 8; d->m_pLZ_flags = d->m_pLZ_code_buf++; }

  s0 = s_tdefl_small_dist_sym[match_dist & 511]; s1 = s_tdefl_large_dist_sym[(match_dist >> 8) & 127];
  d->m_huff_count[1][(match_dist < 512) ? s0 : s1]++;

  if (match_len >= TDEFL_MIN_MATCH_LEN) d->m_huff_count[0][s_tdefl_len_sym[match_len - TDEFL_MIN_MATCH_LEN]]++;
}

static mz_bool tdefl_compress_normal(tdefl_compressor *d)
{
  const mz_uint8 *pSrc = d->m_pSrc; size_t src_buf_left = d->m_src_buf_left;
  tdefl_flush flush = d->m_flush;

  while ((src_buf_left) || ((flush) && (d->m_lookahead_size)))
  {
	mz_uint len_to_move, cur_match_dist, cur_match_len, cur_pos;
	// Update dictionary and hash chains. Keeps the lookahead size equal to TDEFL_MAX_MATCH_LEN.
	if ((d->m_lookahead_size + d->m_dict_size) >= (TDEFL_MIN_MATCH_LEN - 1))
	{
	  mz_uint dst_pos = (d->m_lookahead_pos + d->m_lookahead_size) & TDEFL_LZ_DICT_SIZE_MASK, ins_pos = d->m_lookahead_pos + d->m_lookahead_size - 2;
	  mz_uint hash = (d->m_dict[ins_pos & TDEFL_LZ_DICT_SIZE_MASK] << TDEFL_LZ_HASH_SHIFT) ^ d->m_dict[(ins_pos + 1) & TDEFL_LZ_DICT_SIZE_MASK];
	  mz_uint num_bytes_to_process = (mz_uint)MZ_MIN(src_buf_left, TDEFL_MAX_MATCH_LEN - d->m_lookahead_size);
	  const mz_uint8 *pSrc_end = pSrc + num_bytes_to_process;
	  src_buf_left -= num_bytes_to_process;
	  d->m_lookahead_size += num_bytes_to_process;
	  while (pSrc != pSrc_end)
	  {
		mz_uint8 c = *pSrc++; d->m_dict[dst_pos] = c; if (dst_pos < (TDEFL_MAX_MATCH_LEN - 1)) d->m_dict[TDEFL_LZ_DICT_SIZE + dst_pos] = c;
		hash = ((hash << TDEFL_LZ_HASH_SHIFT) ^ c) & (TDEFL_LZ_HASH_SIZE - 1);
		d->m_next[ins_pos & TDEFL_LZ_DICT_SIZE_MASK] = d->m_hash[hash]; d->m_hash[hash] = (mz_uint16)(ins_pos);
		dst_pos = (dst_pos + 1) & TDEFL_LZ_DICT_SIZE_MASK; ins_pos++;
	  }
	}
	else
	{
	  while ((src_buf_left) && (d->m_lookahead_size < TDEFL_MAX_MATCH_LEN))
	  {
		mz_uint8 c = *pSrc++;
		mz_uint dst_pos = (d->m_lookahead_pos + d->m_lookahead_size) & TDEFL_LZ_DICT_SIZE_MASK;
		src_buf_left--;
		d->m_dict[dst_pos] = c;
		if (dst_pos < (TDEFL_MAX_MATCH_LEN - 1))
		  d->m_dict[TDEFL_LZ_DICT_SIZE + dst_pos] = c;
		if ((++d->m_lookahead_size + d->m_dict_size) >= TDEFL_MIN_MATCH_LEN)
		{
		  mz_uint ins_pos = d->m_lookahead_pos + (d->m_lookahead_size - 1) - 2;
		  mz_uint hash = ((d->m_dict[ins_pos & TDEFL_LZ_DICT_SIZE_MASK] << (TDEFL_LZ_HASH_SHIFT * 2)) ^ (d->m_dict[(ins_pos + 1) & TDEFL_LZ_DICT_SIZE_MASK] << TDEFL_LZ_HASH_SHIFT) ^ c) & (TDEFL_LZ_HASH_SIZE - 1);
		  d->m_next[ins_pos & TDEFL_LZ_DICT_SIZE_MASK] = d->m_hash[hash]; d->m_hash[hash] = (mz_uint16)(ins_pos);
		}
	  }
	}
	d->m_dict_size = MZ_MIN(TDEFL_LZ_DICT_SIZE - d->m_lookahead_size, d->m_dict_size);
	if ((!flush) && (d->m_lookahead_size < TDEFL_MAX_MATCH_LEN))
	  break;

	// Simple lazy/greedy parsing state machine.
	len_to_move = 1; cur_match_dist = 0; cur_match_len = d->m_saved_match_len ? d->m_saved_match_len : (TDEFL_MIN_MATCH_LEN - 1); cur_pos = d->m_lookahead_pos & TDEFL_LZ_DICT_SIZE_MASK;
	if (d->m_flags & (TDEFL_RLE_MATCHES | TDEFL_FORCE_ALL_RAW_BLOCKS))
	{
	  if ((d->m_dict_size) && (!(d->m_flags & TDEFL_FORCE_ALL_RAW_BLOCKS)))
	  {
		mz_uint8 c = d->m_dict[(cur_pos - 1) & TDEFL_LZ_DICT_SIZE_MASK];
		cur_match_len = 0; while (cur_match_len < d->m_lookahead_size) { if (d->m_dict[cur_pos + cur_match_len] != c) break; cur_match_len++; }
		if (cur_match_len < TDEFL_MIN_MATCH_LEN) cur_match_len = 0; else cur_match_dist = 1;
	  }
	}
	else
	{
	  tdefl_find_match(d, d->m_lookahead_pos, d->m_dict_size, d->m_lookahead_size, &cur_match_dist, &cur_match_len);
	}
	if (((cur_match_len == TDEFL_MIN_MATCH_LEN) && (cur_match_dist >= 8U*1024U)) || (cur_pos == cur_match_dist) || ((d->m_flags & TDEFL_FILTER_MATCHES) && (cur_match_len <= 5)))
	{
	  cur_match_dist = cur_match_len = 0;
	}
	if (d->m_saved_match_len)
	{
	  if (cur_match_len > d->m_saved_match_len)
	  {
		tdefl_record_literal(d, (mz_uint8)d->m_saved_lit);
		if (cur_match_len >= 128)
		{
		  tdefl_record_match(d, cur_match_len, cur_match_dist);
		  d->m_saved_match_len = 0; len_to_move = cur_match_len;
		}
		else
		{
		  d->m_saved_lit = d->m_dict[cur_pos]; d->m_saved_match_dist = cur_match_dist; d->m_saved_match_len = cur_match_len;
		}
	  }
	  else
	  {
		tdefl_record_match(d, d->m_saved_match_len, d->m_saved_match_dist);
		len_to_move = d->m_saved_match_len - 1; d->m_saved_match_len = 0;
	  }
	}
	else if (!cur_match_dist)
	  tdefl_record_literal(d, d->m_dict[MZ_MIN(cur_pos, sizeof(d->m_dict) - 1)]);
	else if ((d->m_greedy_parsing) || (d->m_flags & TDEFL_RLE_MATCHES) || (cur_match_len >= 128))
	{
	  tdefl_record_match(d, cur_match_len, cur_match_dist);
	  len_to_move = cur_match_len;
	}
	else
	{
	  d->m_saved_lit = d->m_dict[MZ_MIN(cur_pos, sizeof(d->m_dict) - 1)]; d->m_saved_match_dist = cur_match_dist; d->m_saved_match_len = cur_match_len;
	}
	// Move the lookahead forward by len_to_move bytes.
	d->m_lookahead_pos += len_to_move;
	MZ_ASSERT(d->m_lookahead_size >= len_to_move);
	d->m_lookahead_size -= len_to_move;
	d->m_dict_size = MZ_MIN(d->m_dict_size + len_to_move, TDEFL_LZ_DICT_SIZE);
	// Check if it's time to flush the current LZ codes to the internal output buffer.
	if ( (d->m_pLZ_code_buf > &d->m_lz_code_buf[TDEFL_LZ_CODE_BUF_SIZE - 8]) ||
		 ( (d->m_total_lz_bytes > 31*1024) && (((((mz_uint)(d->m_pLZ_code_buf - d->m_lz_code_buf) * 115) >> 7) >= d->m_total_lz_bytes) || (d->m_flags & TDEFL_FORCE_ALL_RAW_BLOCKS))) )
	{
	  int n;
	  d->m_pSrc = pSrc; d->m_src_buf_left = src_buf_left;
	  if ((n = tdefl_flush_block(d, 0)) != 0)
		return (n < 0) ? MZ_FALSE : MZ_TRUE;
	}
  }

  d->m_pSrc = pSrc; d->m_src_buf_left = src_buf_left;
  return MZ_TRUE;
}

static tdefl_status tdefl_flush_output_buffer(tdefl_compressor *d)
{
  if (d->m_pIn_buf_size)
  {
	*d->m_pIn_buf_size = d->m_pSrc - (const mz_uint8 *)d->m_pIn_buf;
  }

  if (d->m_pOut_buf_size)
  {
	size_t n = MZ_MIN(*d->m_pOut_buf_size - d->m_out_buf_ofs, d->m_output_flush_remaining);
	memcpy((mz_uint8 *)d->m_pOut_buf + d->m_out_buf_ofs, d->m_output_buf + d->m_output_flush_ofs, n);
	d->m_output_flush_ofs += (mz_uint)n;
	d->m_output_flush_remaining -= (mz_uint)n;
	d->m_out_buf_ofs += n;

	*d->m_pOut_buf_size = d->m_out_buf_ofs;
  }

  return (d->m_finished && !d->m_output_flush_remaining) ? TDEFL_STATUS_DONE : TDEFL_STATUS_OKAY;
}

tdefl_status tdefl_compress(tdefl_compressor *d, const void *pIn_buf, size_t *pIn_buf_size, void *pOut_buf, size_t *pOut_buf_size, tdefl_flush flush)
{
  if (!d)
  {
	if (pIn_buf_size) *pIn_buf_size = 0;
	if (pOut_buf_size) *pOut_buf_size = 0;
	return TDEFL_STATUS_BAD_PARAM;
  }

  d->m_pIn_buf = pIn_buf; d->m_pIn_buf_size = pIn_buf_size;
  d->m_pOut_buf = pOut_buf; d->m_pOut_buf_size = pOut_buf_size;
  d->m_pSrc = (const mz_uint8 *)(pIn_buf); d->m_src_buf_left = pIn_buf_size ? *pIn_buf_size : 0;
  d->m_out_buf_ofs = 0;
  d->m_flush = flush;

  if ( ((d->m_pPut_buf_func != NULL) == ((pOut_buf != NULL) || (pOut_buf_size != NULL))) || (d->m_prev_return_status != TDEFL_STATUS_OKAY) ||
		(d->m_wants_to_finish && (flush != TDEFL_FINISH)) || (pIn_buf_size && *pIn_buf_size && !pIn_buf) || (pOut_buf_size && *pOut_buf_size && !pOut_buf) )
  {
	if (pIn_buf_size) *pIn_buf_size = 0;
	if (pOut_buf_size) *pOut_buf_size = 0;
	return (d->m_prev_return_status = TDEFL_STATUS_BAD_PARAM);
  }
  d->m_wants_to_finish |= (flush == TDEFL_FINISH);

  if ((d->m_output_flush_remaining) || (d->m_finished))
	return (d->m_prev_return_status = tdefl_flush_output_buffer(d));

#if MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN
  if (((d->m_flags & TDEFL_MAX_PROBES_MASK) == 1) &&
	  ((d->m_flags & TDEFL_GREEDY_PARSING_FLAG) != 0) &&
	  ((d->m_flags & (TDEFL_FILTER_MATCHES | TDEFL_FORCE_ALL_RAW_BLOCKS | TDEFL_RLE_MATCHES)) == 0))
  {
	if (!tdefl_compress_fast(d))
	  return d->m_prev_return_status;
  }
  else
#endif // #if MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN
  {
	if (!tdefl_compress_normal(d))
	  return d->m_prev_return_status;
  }

  if ((d->m_flags & (TDEFL_WRITE_ZLIB_HEADER | TDEFL_COMPUTE_ADLER32)) && (pIn_buf))
	d->m_adler32 = (mz_uint32)mz_adler32(d->m_adler32, (const mz_uint8 *)pIn_buf, d->m_pSrc - (const mz_uint8 *)pIn_buf);

  if ((flush) && (!d->m_lookahead_size) && (!d->m_src_buf_left) && (!d->m_output_flush_remaining))
  {
	if (tdefl_flush_block(d, flush) < 0)
	  return d->m_prev_return_status;
	d->m_finished = (flush == TDEFL_FINISH);
	if (flush == TDEFL_FULL_FLUSH) { MZ_CLEAR_OBJ(d->m_hash); MZ_CLEAR_OBJ(d->m_next); d->m_dict_size = 0; }
  }

  return (d->m_prev_return_status = tdefl_flush_output_buffer(d));
}

tdefl_status tdefl_compress_buffer(tdefl_compressor *d, const void *pIn_buf, size_t in_buf_size, tdefl_flush flush)
{
  MZ_ASSERT(d->m_pPut_buf_func); return tdefl_compress(d, pIn_buf, &in_buf_size, NULL, NULL, flush);
}

tdefl_status tdefl_init(tdefl_compressor *d, tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
  d->m_pPut_buf_func = pPut_buf_func; d->m_pPut_buf_user = pPut_buf_user;
  d->m_flags = (mz_uint)(flags); d->m_max_probes[0] = 1 + ((flags & 0xFFF) + 2) / 3; d->m_greedy_parsing = (flags & TDEFL_GREEDY_PARSING_FLAG) != 0;
  d->m_max_probes[1] = 1 + (((flags & 0xFFF) >> 2) + 2) / 3;
  if (!(flags & TDEFL_NONDETERMINISTIC_PARSING_FLAG)) MZ_CLEAR_OBJ(d->m_hash);
  d->m_lookahead_pos = d->m_lookahead_size = d->m_dict_size = d->m_total_lz_bytes = d->m_lz_code_buf_dict_pos = d->m_bits_in = 0;
  d->m_output_flush_ofs = d->m_output_flush_remaining = d->m_finished = d->m_block_index = d->m_bit_buffer = d->m_wants_to_finish = 0;
  d->m_pLZ_code_buf = d->m_lz_code_buf + 1; d->m_pLZ_flags = d->m_lz_code_buf; d->m_num_flags_left = 8;
  d->m_pOutput_buf = d->m_output_buf; d->m_pOutput_buf_end = d->m_output_buf; d->m_prev_return_status = TDEFL_STATUS_OKAY;
  d->m_saved_match_dist = d->m_saved_match_len = d->m_saved_lit = 0; d->m_adler32 = 1;
  d->m_pIn_buf = NULL; d->m_pOut_buf = NULL;
  d->m_pIn_buf_size = NULL; d->m_pOut_buf_size = NULL;
  d->m_flush = TDEFL_NO_FLUSH; d->m_pSrc = NULL; d->m_src_buf_left = 0; d->m_out_buf_ofs = 0;
  memset(&d->m_huff_count[0][0], 0, sizeof(d->m_huff_count[0][0]) * TDEFL_MAX_HUFF_SYMBOLS_0);
  memset(&d->m_huff_count[1][0], 0, sizeof(d->m_huff_count[1][0]) * TDEFL_MAX_HUFF_SYMBOLS_1);
  return TDEFL_STATUS_OKAY;
}

tdefl_status tdefl_get_prev_return_status(tdefl_compressor *d)
{
  return d->m_prev_return_status;
}

mz_uint32 tdefl_get_adler32(tdefl_compressor *d)
{
  return d->m_adler32;
}

mz_bool tdefl_compress_mem_to_output(const void *pBuf, size_t buf_len, tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
  tdefl_compressor *pComp; mz_bool succeeded; if (((buf_len) && (!pBuf)) || (!pPut_buf_func)) return MZ_FALSE;
  pComp = (tdefl_compressor*)MZ_MALLOC(sizeof(tdefl_compressor)); if (!pComp) return MZ_FALSE;
  succeeded = (tdefl_init(pComp, pPut_buf_func, pPut_buf_user, flags) == TDEFL_STATUS_OKAY);
  succeeded = succeeded && (tdefl_compress_buffer(pComp, pBuf, buf_len, TDEFL_FINISH) == TDEFL_STATUS_DONE);
  MZ_FREE(pComp); return succeeded;
}

typedef struct
{
  size_t m_size, m_capacity;
  mz_uint8 *m_pBuf;
  mz_bool m_expandable;
} tdefl_output_buffer;

static mz_bool tdefl_output_buffer_putter(const void *pBuf, int len, void *pUser)
{
  tdefl_output_buffer *p = (tdefl_output_buffer *)pUser;
  size_t new_size = p->m_size + len;
  if (new_size > p->m_capacity)
  {
	size_t new_capacity = p->m_capacity; mz_uint8 *pNew_buf; if (!p->m_expandable) return MZ_FALSE;
	do { new_capacity = MZ_MAX(128U, new_capacity << 1U); } while (new_size > new_capacity);
	pNew_buf = (mz_uint8*)MZ_REALLOC(p->m_pBuf, new_capacity); if (!pNew_buf) return MZ_FALSE;
	p->m_pBuf = pNew_buf; p->m_capacity = new_capacity;
  }
  memcpy((mz_uint8*)p->m_pBuf + p->m_size, pBuf, len); p->m_size = new_size;
  return MZ_TRUE;
}

void *tdefl_compress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags)
{
  tdefl_output_buffer out_buf; MZ_CLEAR_OBJ(out_buf);
  if (!pOut_len) return MZ_FALSE; else *pOut_len = 0;
  out_buf.m_expandable = MZ_TRUE;
  if (!tdefl_compress_mem_to_output(pSrc_buf, src_buf_len, tdefl_output_buffer_putter, &out_buf, flags)) return NULL;
  *pOut_len = out_buf.m_size; return out_buf.m_pBuf;
}

size_t tdefl_compress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags)
{
  tdefl_output_buffer out_buf; MZ_CLEAR_OBJ(out_buf);
  if (!pOut_buf) return 0;
  out_buf.m_pBuf = (mz_uint8*)pOut_buf; out_buf.m_capacity = out_buf_len;
  if (!tdefl_compress_mem_to_output(pSrc_buf, src_buf_len, tdefl_output_buffer_putter, &out_buf, flags)) return 0;
  return out_buf.m_size;
}

#ifndef MINIZ_NO_ZLIB_APIS
static const mz_uint s_tdefl_num_probes[11] = { 0, 1, 6, 32,  16, 32, 128, 256,  512, 768, 1500 };

// level may actually range from [0,10] (10 is a "hidden" max level, where we want a bit more compression and it's fine if throughput to fall off a cliff on some files).
mz_uint tdefl_create_comp_flags_from_zip_params(int level, int window_bits, int strategy)
{
  mz_uint comp_flags = s_tdefl_num_probes[(level >= 0) ? MZ_MIN(10, level) : MZ_DEFAULT_LEVEL] | ((level <= 3) ? TDEFL_GREEDY_PARSING_FLAG : 0);
  if (window_bits > 0) comp_flags |= TDEFL_WRITE_ZLIB_HEADER;

  if (!level) comp_flags |= TDEFL_FORCE_ALL_RAW_BLOCKS;
  else if (strategy == MZ_FILTERED) comp_flags |= TDEFL_FILTER_MATCHES;
  else if (strategy == MZ_HUFFMAN_ONLY) comp_flags &= ~TDEFL_MAX_PROBES_MASK;
  else if (strategy == MZ_FIXED) comp_flags |= TDEFL_FORCE_ALL_STATIC_BLOCKS;
  else if (strategy == MZ_RLE) comp_flags |= TDEFL_RLE_MATCHES;

  return comp_flags;
}
#endif //MINIZ_NO_ZLIB_APIS

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4204) // nonstandard extension used : non-constant aggregate initializer (also supported by GNU C and C99, so no big deal)
#endif

// Simple PNG writer function by Alex Evans, 2011. Released into the public domain: https://gist.github.com/908299, more context at
// http://altdevblogaday.org/2011/04/06/a-smaller-jpg-encoder/.
// This is actually a modification of Alex's original code so PNG files generated by this function pass pngcheck.
void *tdefl_write_image_to_png_file_in_memory_ex(const void *pImage, int w, int h, int num_chans, size_t *pLen_out, mz_uint level, mz_bool flip)
{
  // Using a local copy of this array here in case MINIZ_NO_ZLIB_APIS was defined.
  static const mz_uint s_tdefl_png_num_probes[11] = { 0, 1, 6, 32,  16, 32, 128, 256,  512, 768, 1500 };
  tdefl_compressor *pComp = (tdefl_compressor *)MZ_MALLOC(sizeof(tdefl_compressor)); tdefl_output_buffer out_buf; int i, bpl = w * num_chans, y, z; mz_uint32 c; *pLen_out = 0;
  if (!pComp) return NULL;
  MZ_CLEAR_OBJ(out_buf); out_buf.m_expandable = MZ_TRUE; out_buf.m_capacity = 57+MZ_MAX(64, (1+bpl)*h); if (NULL == (out_buf.m_pBuf = (mz_uint8*)MZ_MALLOC(out_buf.m_capacity))) { MZ_FREE(pComp); return NULL; }
  // write dummy header
  for (z = 41; z; --z) tdefl_output_buffer_putter(&z, 1, &out_buf);
  // compress image data
  tdefl_init(pComp, tdefl_output_buffer_putter, &out_buf, s_tdefl_png_num_probes[MZ_MIN(10, level)] | TDEFL_WRITE_ZLIB_HEADER);
  for (y = 0; y < h; ++y) { tdefl_compress_buffer(pComp, &z, 1, TDEFL_NO_FLUSH); tdefl_compress_buffer(pComp, (mz_uint8*)pImage + (flip ? (h - 1 - y) : y) * bpl, bpl, TDEFL_NO_FLUSH); }
  if (tdefl_compress_buffer(pComp, NULL, 0, TDEFL_FINISH) != TDEFL_STATUS_DONE) { MZ_FREE(pComp); MZ_FREE(out_buf.m_pBuf); return NULL; }
  // write real header
  *pLen_out = out_buf.m_size-41;
  {
	static const mz_uint8 chans[] = {0x00, 0x00, 0x04, 0x02, 0x06};
	mz_uint8 pnghdr[41]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
	  0,0,(mz_uint8)(w>>8),(mz_uint8)w,0,0,(mz_uint8)(h>>8),(mz_uint8)h,8,chans[num_chans],0,0,0,0,0,0,0,
	  (mz_uint8)(*pLen_out>>24),(mz_uint8)(*pLen_out>>16),(mz_uint8)(*pLen_out>>8),(mz_uint8)*pLen_out,0x49,0x44,0x41,0x54};
	c=(mz_uint32)mz_crc32(MZ_CRC32_INIT,pnghdr+12,17); for (i=0; i<4; ++i, c<<=8) ((mz_uint8*)(pnghdr+29))[i]=(mz_uint8)(c>>24);
	memcpy(out_buf.m_pBuf, pnghdr, 41);
  }
  // write footer (IDAT CRC-32, followed by IEND chunk)
  if (!tdefl_output_buffer_putter("\0\0\0\0\0\0\0\0\x49\x45\x4e\x44\xae\x42\x60\x82", 16, &out_buf)) { *pLen_out = 0; MZ_FREE(pComp); MZ_FREE(out_buf.m_pBuf); return NULL; }
  c = (mz_uint32)mz_crc32(MZ_CRC32_INIT,out_buf.m_pBuf+41-4, *pLen_out+4); for (i=0; i<4; ++i, c<<=8) (out_buf.m_pBuf+out_buf.m_size-16)[i] = (mz_uint8)(c >> 24);
  // compute final size of file, grab compressed data buffer and return
  *pLen_out += 57; MZ_FREE(pComp); return out_buf.m_pBuf;
}
void *tdefl_write_image_to_png_file_in_memory(const void *pImage, int w, int h, int num_chans, size_t *pLen_out)
{
  // Level 6 corresponds to TDEFL_DEFAULT_MAX_PROBES or MZ_DEFAULT_LEVEL (but we can't depend on MZ_DEFAULT_LEVEL being available in case the zlib API's where #defined out)
  return tdefl_write_image_to_png_file_in_memory_ex(pImage, w, h, num_chans, pLen_out, 6, MZ_FALSE);
}

#ifdef _MSC_VER
#pragma warning (pop)
#endif

// ------------------- .ZIP archive reading

#ifndef MINIZ_NO_ARCHIVE_APIS

#ifdef MINIZ_NO_STDIO
  #define MZ_FILE void *
#else
  #include <stdio.h>
  #include <sys/stat.h>

  #if defined(_MSC_VER) || defined(__MINGW64__)
	static FILE *mz_fopen(const char *pFilename, const char *pMode)
	{
	  FILE* pFile = NULL;
	  fopen_s(&pFile, pFilename, pMode);
	  return pFile;
	}
	static FILE *mz_freopen(const char *pPath, const char *pMode, FILE *pStream)
	{
	  FILE* pFile = NULL;
	  if (freopen_s(&pFile, pPath, pMode, pStream))
		return NULL;
	  return pFile;
	}
	#ifndef MINIZ_NO_TIME
	  #include <sys/utime.h>
	#endif
	#define MZ_FILE FILE
	#define MZ_FOPEN mz_fopen
	#define MZ_FCLOSE fclose
	#define MZ_FREAD fread
	#define MZ_FWRITE fwrite
	#define MZ_FTELL64 _ftelli64
	#define MZ_FSEEK64 _fseeki64
	#define MZ_FILE_STAT_STRUCT _stat
	#define MZ_FILE_STAT _stat
	#define MZ_FFLUSH fflush
	#define MZ_FREOPEN mz_freopen
	#define MZ_DELETE_FILE remove
  #elif defined(__MINGW32__)
	#ifndef MINIZ_NO_TIME
	  #include <sys/utime.h>
	#endif
	#define MZ_FILE FILE
	#define MZ_FOPEN(f, m) fopen(f, m)
	#define MZ_FCLOSE fclose
	#define MZ_FREAD fread
	#define MZ_FWRITE fwrite
	#define MZ_FTELL64 ftello64
	#define MZ_FSEEK64 fseeko64
	#define MZ_FILE_STAT_STRUCT _stat
	#define MZ_FILE_STAT _stat
	#define MZ_FFLUSH fflush
	#define MZ_FREOPEN(f, m, s) freopen(f, m, s)
	#define MZ_DELETE_FILE remove
  #elif defined(__TINYC__)
	#ifndef MINIZ_NO_TIME
	  #include <sys/utime.h>
	#endif
	#define MZ_FILE FILE
	#define MZ_FOPEN(f, m) fopen(f, m)
	#define MZ_FCLOSE fclose
	#define MZ_FREAD fread
	#define MZ_FWRITE fwrite
	#define MZ_FTELL64 ftell
	#define MZ_FSEEK64 fseek
	#define MZ_FILE_STAT_STRUCT stat
	#define MZ_FILE_STAT stat
	#define MZ_FFLUSH fflush
	#define MZ_FREOPEN(f, m, s) freopen(f, m, s)
	#define MZ_DELETE_FILE remove
  #elif defined(__GNUC__) && _LARGEFILE64_SOURCE
	#ifndef MINIZ_NO_TIME
	  #include <utime.h>
	#endif
	#define MZ_FILE FILE
	#define MZ_FOPEN(f, m) fopen64(f, m)
	#define MZ_FCLOSE fclose
	#define MZ_FREAD fread
	#define MZ_FWRITE fwrite
	#define MZ_FTELL64 ftello64
	#define MZ_FSEEK64 fseeko64
	#define MZ_FILE_STAT_STRUCT stat64
	#define MZ_FILE_STAT stat64
	#define MZ_FFLUSH fflush
	#define MZ_FREOPEN(p, m, s) freopen64(p, m, s)
	#define MZ_DELETE_FILE remove
  #else
	#ifndef MINIZ_NO_TIME
	  #include <utime.h>
	#endif
	#define MZ_FILE FILE
	#define MZ_FOPEN(f, m) fopen(f, m)
	#define MZ_FCLOSE fclose
	#define MZ_FREAD fread
	#define MZ_FWRITE fwrite
	#define MZ_FTELL64 ftello
	#define MZ_FSEEK64 fseeko
	#define MZ_FILE_STAT_STRUCT stat
	#define MZ_FILE_STAT stat
	#define MZ_FFLUSH fflush
	#define MZ_FREOPEN(f, m, s) freopen(f, m, s)
	#define MZ_DELETE_FILE remove
  #endif // #ifdef _MSC_VER
#endif // #ifdef MINIZ_NO_STDIO

#define MZ_TOLOWER(c) ((((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c))

#ifndef _0x06054b50
#define _0x06054b50 0x06054b50
#endif

#ifndef _0x02014b50
#define _0x02014b50 0x02014b50
#endif

#ifndef _0x04034b50
#define _0x04034b50 0x04034b50
#endif

#ifndef _0x08074b50
#define _0x08074b50 0x08074b50
#endif

// Various ZIP archive enums. To completely avoid cross platform compiler alignment and platform endian issues, miniz.c doesn't use structs for any of this stuff.
enum
{
  // ZIP archive identifiers and record sizes
  MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG = _0x06054b50, MZ_ZIP_CENTRAL_DIR_HEADER_SIG = _0x02014b50, MZ_ZIP_LOCAL_DIR_HEADER_SIG = _0x04034b50,
  MZ_ZIP_LOCAL_DIR_HEADER_SIZE = 30, MZ_ZIP_CENTRAL_DIR_HEADER_SIZE = 46, MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE = 22,
  // Central directory header record offsets
  MZ_ZIP_CDH_SIG_OFS = 0, MZ_ZIP_CDH_VERSION_MADE_BY_OFS = 4, MZ_ZIP_CDH_VERSION_NEEDED_OFS = 6, MZ_ZIP_CDH_BIT_FLAG_OFS = 8,
  MZ_ZIP_CDH_METHOD_OFS = 10, MZ_ZIP_CDH_FILE_TIME_OFS = 12, MZ_ZIP_CDH_FILE_DATE_OFS = 14, MZ_ZIP_CDH_CRC32_OFS = 16,
  MZ_ZIP_CDH_COMPRESSED_SIZE_OFS = 20, MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS = 24, MZ_ZIP_CDH_FILENAME_LEN_OFS = 28, MZ_ZIP_CDH_EXTRA_LEN_OFS = 30,
  MZ_ZIP_CDH_COMMENT_LEN_OFS = 32, MZ_ZIP_CDH_DISK_START_OFS = 34, MZ_ZIP_CDH_INTERNAL_ATTR_OFS = 36, MZ_ZIP_CDH_EXTERNAL_ATTR_OFS = 38, MZ_ZIP_CDH_LOCAL_HEADER_OFS = 42,
  // Local directory header offsets
  MZ_ZIP_LDH_SIG_OFS = 0, MZ_ZIP_LDH_VERSION_NEEDED_OFS = 4, MZ_ZIP_LDH_BIT_FLAG_OFS = 6, MZ_ZIP_LDH_METHOD_OFS = 8, MZ_ZIP_LDH_FILE_TIME_OFS = 10,
  MZ_ZIP_LDH_FILE_DATE_OFS = 12, MZ_ZIP_LDH_CRC32_OFS = 14, MZ_ZIP_LDH_COMPRESSED_SIZE_OFS = 18, MZ_ZIP_LDH_DECOMPRESSED_SIZE_OFS = 22,
  MZ_ZIP_LDH_FILENAME_LEN_OFS = 26, MZ_ZIP_LDH_EXTRA_LEN_OFS = 28,
  // End of central directory offsets
  MZ_ZIP_ECDH_SIG_OFS = 0, MZ_ZIP_ECDH_NUM_THIS_DISK_OFS = 4, MZ_ZIP_ECDH_NUM_DISK_CDIR_OFS = 6, MZ_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS = 8,
  MZ_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS = 10, MZ_ZIP_ECDH_CDIR_SIZE_OFS = 12, MZ_ZIP_ECDH_CDIR_OFS_OFS = 16, MZ_ZIP_ECDH_COMMENT_SIZE_OFS = 20,
};

typedef struct
{
  void *m_p;
  size_t m_size, m_capacity;
  mz_uint m_element_size;
} mz_zip_array;

struct mz_zip_internal_state_tag
{
  mz_zip_array m_central_dir;
  mz_zip_array m_central_dir_offsets;
  mz_zip_array m_sorted_central_dir_offsets;
  MZ_FILE *m_pFile;
  void *m_pMem;
  size_t m_mem_size;
  size_t m_mem_capacity;
};

#define MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(array_ptr, element_size) (array_ptr)->m_element_size = element_size
#define MZ_ZIP_ARRAY_ELEMENT(array_ptr, element_type, index) ((element_type *)((array_ptr)->m_p))[index]

static MZ_FORCEINLINE void mz_zip_array_clear(mz_zip_archive *pZip, mz_zip_array *pArray)
{
  pZip->m_pFree(pZip->m_pAlloc_opaque, pArray->m_p);
  memset(pArray, 0, sizeof(mz_zip_array));
}

static mz_bool mz_zip_array_ensure_capacity(mz_zip_archive *pZip, mz_zip_array *pArray, size_t min_new_capacity, mz_uint growing)
{
  void *pNew_p; size_t new_capacity = min_new_capacity; MZ_ASSERT(pArray->m_element_size); if (pArray->m_capacity >= min_new_capacity) return MZ_TRUE;
  if (growing) { new_capacity = MZ_MAX(1, pArray->m_capacity); while (new_capacity < min_new_capacity) new_capacity *= 2; }
  if (NULL == (pNew_p = pZip->m_pRealloc(pZip->m_pAlloc_opaque, pArray->m_p, pArray->m_element_size, new_capacity))) return MZ_FALSE;
  pArray->m_p = pNew_p; pArray->m_capacity = new_capacity;
  return MZ_TRUE;
}

static MZ_FORCEINLINE mz_bool mz_zip_array_reserve(mz_zip_archive *pZip, mz_zip_array *pArray, size_t new_capacity, mz_uint growing)
{
  if (new_capacity > pArray->m_capacity) { if (!mz_zip_array_ensure_capacity(pZip, pArray, new_capacity, growing)) return MZ_FALSE; }
  return MZ_TRUE;
}

static MZ_FORCEINLINE mz_bool mz_zip_array_resize(mz_zip_archive *pZip, mz_zip_array *pArray, size_t new_size, mz_uint growing)
{
  if (new_size > pArray->m_capacity) { if (!mz_zip_array_ensure_capacity(pZip, pArray, new_size, growing)) return MZ_FALSE; }
  pArray->m_size = new_size;
  return MZ_TRUE;
}

static MZ_FORCEINLINE mz_bool mz_zip_array_ensure_room(mz_zip_archive *pZip, mz_zip_array *pArray, size_t n)
{
  return mz_zip_array_reserve(pZip, pArray, pArray->m_size + n, MZ_TRUE);
}

static MZ_FORCEINLINE mz_bool mz_zip_array_push_back(mz_zip_archive *pZip, mz_zip_array *pArray, const void *pElements, size_t n)
{
  size_t orig_size = pArray->m_size; if (!mz_zip_array_resize(pZip, pArray, orig_size + n, MZ_TRUE)) return MZ_FALSE;
  memcpy((mz_uint8*)pArray->m_p + orig_size * pArray->m_element_size, pElements, n * pArray->m_element_size);
  return MZ_TRUE;
}

#ifndef MINIZ_NO_TIME
static time_t mz_zip_dos_to_time_t(int dos_time, int dos_date)
{
  struct tm tm;
  memset(&tm, 0, sizeof(tm)); tm.tm_isdst = -1;
  tm.tm_year = ((dos_date >> 9) & 127) + 1980 - 1900; tm.tm_mon = ((dos_date >> 5) & 15) - 1; tm.tm_mday = dos_date & 31;
  tm.tm_hour = (dos_time >> 11) & 31; tm.tm_min = (dos_time >> 5) & 63; tm.tm_sec = (dos_time << 1) & 62;
  return mktime(&tm);
}

static void mz_zip_time_to_dos_time(time_t time, mz_uint16 *pDOS_time, mz_uint16 *pDOS_date)
{
#ifdef _MSC_VER
  struct tm tm_struct;
  struct tm *tm = &tm_struct;
  errno_t err = localtime_s(tm, &time);
  if (err)
  {
	*pDOS_date = 0; *pDOS_time = 0;
	return;
  }
#else
  struct tm *tm = localtime(&time);
#endif
  *pDOS_time = (mz_uint16)(((tm->tm_hour) << 11) + ((tm->tm_min) << 5) + ((tm->tm_sec) >> 1));
  *pDOS_date = (mz_uint16)(((tm->tm_year + 1900 - 1980) << 9) + ((tm->tm_mon + 1) << 5) + tm->tm_mday);
}
#endif

#ifndef MINIZ_NO_STDIO
static mz_bool mz_zip_get_file_modified_time(const char *pFilename, mz_uint16 *pDOS_time, mz_uint16 *pDOS_date)
{
#ifdef MINIZ_NO_TIME
  (void)pFilename; *pDOS_date = *pDOS_time = 0;
#else
  struct MZ_FILE_STAT_STRUCT file_stat;
  // On Linux with x86 glibc, this call will fail on large files (>= 0x80000000 bytes) unless you compiled with _LARGEFILE64_SOURCE. Argh.
  if (MZ_FILE_STAT(pFilename, &file_stat) != 0)
	return MZ_FALSE;
  mz_zip_time_to_dos_time(file_stat.st_mtime, pDOS_time, pDOS_date);
#endif // #ifdef MINIZ_NO_TIME
  return MZ_TRUE;
}

#ifndef MINIZ_NO_TIME
static mz_bool mz_zip_set_file_times(const char *pFilename, time_t access_time, time_t modified_time)
{
  struct utimbuf t; t.actime = access_time; t.modtime = modified_time;
  return !utime(pFilename, &t);
}
#endif // #ifndef MINIZ_NO_TIME
#endif // #ifndef MINIZ_NO_STDIO

static mz_bool mz_zip_reader_init_internal(mz_zip_archive *pZip, mz_uint32 flags)
{
  (void)flags;
  if ((!pZip) || (pZip->m_pState) || (pZip->m_zip_mode != MZ_ZIP_MODE_INVALID))
	return MZ_FALSE;

  if (!pZip->m_pAlloc) pZip->m_pAlloc = def_alloc_func;
  if (!pZip->m_pFree) pZip->m_pFree = def_free_func;
  if (!pZip->m_pRealloc) pZip->m_pRealloc = def_realloc_func;

  pZip->m_zip_mode = MZ_ZIP_MODE_READING;
  pZip->m_archive_size = 0;
  pZip->m_central_directory_file_ofs = 0;
  pZip->m_total_files = 0;

  if (NULL == (pZip->m_pState = (mz_zip_internal_state *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(mz_zip_internal_state))))
	return MZ_FALSE;
  memset(pZip->m_pState, 0, sizeof(mz_zip_internal_state));
  MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir, sizeof(mz_uint8));
  MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir_offsets, sizeof(mz_uint32));
  MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_sorted_central_dir_offsets, sizeof(mz_uint32));
  return MZ_TRUE;
}

static MZ_FORCEINLINE mz_bool mz_zip_reader_filename_less(const mz_zip_array *pCentral_dir_array, const mz_zip_array *pCentral_dir_offsets, mz_uint l_index, mz_uint r_index)
{
  const mz_uint8 *pL = &MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_array, mz_uint8, MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_offsets, mz_uint32, l_index)), *pE;
  const mz_uint8 *pR = &MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_array, mz_uint8, MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_offsets, mz_uint32, r_index));
  mz_uint l_len = MZ_READ_LE16(pL + MZ_ZIP_CDH_FILENAME_LEN_OFS), r_len = MZ_READ_LE16(pR + MZ_ZIP_CDH_FILENAME_LEN_OFS);
  mz_uint8 l = 0, r = 0;
  pL += MZ_ZIP_CENTRAL_DIR_HEADER_SIZE; pR += MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;
  pE = pL + MZ_MIN(l_len, r_len);
  while (pL < pE)
  {
	if ((l = MZ_TOLOWER(*pL)) != (r = MZ_TOLOWER(*pR)))
	  break;
	pL++; pR++;
  }
  return (pL == pE) ? (l_len < r_len) : (l < r);
}

#define MZ_SWAP_UINT32(a, b) do { mz_uint32 t = a; a = b; b = t; } MZ_MACRO_END

// Heap sort of lowercased filenames, used to help accelerate plain central directory searches by mz_zip_reader_locate_file(). (Could also use qsort(), but it could allocate memory.)
static void mz_zip_reader_sort_central_dir_offsets_by_filename(mz_zip_archive *pZip)
{
  mz_zip_internal_state *pState = pZip->m_pState;
  const mz_zip_array *pCentral_dir_offsets = &pState->m_central_dir_offsets;
  const mz_zip_array *pCentral_dir = &pState->m_central_dir;
  mz_uint32 *pIndices = &MZ_ZIP_ARRAY_ELEMENT(&pState->m_sorted_central_dir_offsets, mz_uint32, 0);
  const int size = pZip->m_total_files;
  int start = (size - 2) >> 1, end;
  while (start >= 0)
  {
	int child, root = start;
	for ( ; ; )
	{
	  if ((child = (root << 1) + 1) >= size)
		break;
	  child += (((child + 1) < size) && (mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[child], pIndices[child + 1])));
	  if (!mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[root], pIndices[child]))
		break;
	  MZ_SWAP_UINT32(pIndices[root], pIndices[child]); root = child;
	}
	start--;
  }

  end = size - 1;
  while (end > 0)
  {
	int child, root = 0;
	MZ_SWAP_UINT32(pIndices[end], pIndices[0]);
	for ( ; ; )
	{
	  if ((child = (root << 1) + 1) >= end)
		break;
	  child += (((child + 1) < end) && mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[child], pIndices[child + 1]));
	  if (!mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[root], pIndices[child]))
		break;
	  MZ_SWAP_UINT32(pIndices[root], pIndices[child]); root = child;
	}
	end--;
  }
}

static mz_bool mz_zip_reader_read_central_dir(mz_zip_archive *pZip, mz_uint32 flags)
{
  mz_uint cdir_size, num_this_disk, cdir_disk_index;
  mz_uint64 cdir_ofs;
  mz_int64 cur_file_ofs;
  const mz_uint8 *p;
  mz_uint32 buf_u32[4096 / sizeof(mz_uint32)]; mz_uint8 *pBuf = (mz_uint8 *)buf_u32;
  mz_bool sort_central_dir = ((flags & MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY) == 0);
  // Basic sanity checks - reject files which are too small, and check the first 4 bytes of the file to make sure a local header is there.
  if (pZip->m_archive_size < MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)
	return MZ_FALSE;
  // Find the end of central directory record by scanning the file from the end towards the beginning.
  cur_file_ofs = MZ_MAX((mz_int64)pZip->m_archive_size - (mz_int64)sizeof(buf_u32), 0);
  for ( ; ; )
  {
	int i, n = (int)MZ_MIN(sizeof(buf_u32), pZip->m_archive_size - cur_file_ofs);
	if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pBuf, n) != (mz_uint)n)
	  return MZ_FALSE;
	for (i = n - 4; i >= 0; --i)
	  if (MZ_READ_LE32(pBuf + i) == MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG)
		break;
	if (i >= 0)
	{
	  cur_file_ofs += i;
	  break;
	}
	if ((!cur_file_ofs) || ((pZip->m_archive_size - cur_file_ofs) >= (0xFFFF + MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)))
	  return MZ_FALSE;
	cur_file_ofs = MZ_MAX(cur_file_ofs - (sizeof(buf_u32) - 3), 0);
  }
  // Read and verify the end of central directory record.
  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pBuf, MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE) != MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)
	return MZ_FALSE;
  if ((MZ_READ_LE32(pBuf + MZ_ZIP_ECDH_SIG_OFS) != MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG) ||
	  ((pZip->m_total_files = MZ_READ_LE16(pBuf + MZ_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS)) != MZ_READ_LE16(pBuf + MZ_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS)))
	return MZ_FALSE;

  num_this_disk = MZ_READ_LE16(pBuf + MZ_ZIP_ECDH_NUM_THIS_DISK_OFS);
  cdir_disk_index = MZ_READ_LE16(pBuf + MZ_ZIP_ECDH_NUM_DISK_CDIR_OFS);
  if (((num_this_disk | cdir_disk_index) != 0) && ((num_this_disk != 1) || (cdir_disk_index != 1)))
	return MZ_FALSE;

  if ((cdir_size = MZ_READ_LE32(pBuf + MZ_ZIP_ECDH_CDIR_SIZE_OFS)) < pZip->m_total_files * MZ_ZIP_CENTRAL_DIR_HEADER_SIZE)
	return MZ_FALSE;

  cdir_ofs = MZ_READ_LE32(pBuf + MZ_ZIP_ECDH_CDIR_OFS_OFS);
  if ((cdir_ofs + (mz_uint64)cdir_size) > pZip->m_archive_size)
	return MZ_FALSE;

  pZip->m_central_directory_file_ofs = cdir_ofs;

  if (pZip->m_total_files)
  {
	 mz_uint i, n;

	// Read the entire central directory into a heap block, and allocate another heap block to hold the unsorted central dir file record offsets, and another to hold the sorted indices.
	if ((!mz_zip_array_resize(pZip, &pZip->m_pState->m_central_dir, cdir_size, MZ_FALSE)) ||
		(!mz_zip_array_resize(pZip, &pZip->m_pState->m_central_dir_offsets, pZip->m_total_files, MZ_FALSE)))
	  return MZ_FALSE;

	if (sort_central_dir)
	{
	  if (!mz_zip_array_resize(pZip, &pZip->m_pState->m_sorted_central_dir_offsets, pZip->m_total_files, MZ_FALSE))
		return MZ_FALSE;
	}

	if (pZip->m_pRead(pZip->m_pIO_opaque, cdir_ofs, pZip->m_pState->m_central_dir.m_p, cdir_size) != cdir_size)
	  return MZ_FALSE;

	// Now create an index into the central directory file records, do some basic sanity checking on each record, and check for zip64 entries (which are not yet supported).
	p = (const mz_uint8 *)pZip->m_pState->m_central_dir.m_p;
	for (n = cdir_size, i = 0; i < pZip->m_total_files; ++i)
	{
	  mz_uint total_header_size, comp_size, decomp_size, disk_index;
	  if ((n < MZ_ZIP_CENTRAL_DIR_HEADER_SIZE) || (MZ_READ_LE32(p) != MZ_ZIP_CENTRAL_DIR_HEADER_SIG))
		return MZ_FALSE;
	  MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, mz_uint32, i) = (mz_uint32)(p - (const mz_uint8 *)pZip->m_pState->m_central_dir.m_p);
	  if (sort_central_dir)
		MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_sorted_central_dir_offsets, mz_uint32, i) = i;
	  comp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
	  decomp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
	  if (((!MZ_READ_LE32(p + MZ_ZIP_CDH_METHOD_OFS)) && (decomp_size != comp_size)) || (decomp_size && !comp_size) || (decomp_size == 0xFFFFFFFF) || (comp_size == 0xFFFFFFFF))
		return MZ_FALSE;
	  disk_index = MZ_READ_LE16(p + MZ_ZIP_CDH_DISK_START_OFS);
	  if ((disk_index != num_this_disk) && (disk_index != 1))
		return MZ_FALSE;
	  if (((mz_uint64)MZ_READ_LE32(p + MZ_ZIP_CDH_LOCAL_HEADER_OFS) + MZ_ZIP_LOCAL_DIR_HEADER_SIZE + comp_size) > pZip->m_archive_size)
		return MZ_FALSE;
	  if ((total_header_size = MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + MZ_READ_LE16(p + MZ_ZIP_CDH_FILENAME_LEN_OFS) + MZ_READ_LE16(p + MZ_ZIP_CDH_EXTRA_LEN_OFS) + MZ_READ_LE16(p + MZ_ZIP_CDH_COMMENT_LEN_OFS)) > n)
		return MZ_FALSE;
	  n -= total_header_size; p += total_header_size;
	}
  }

  if (sort_central_dir)
	mz_zip_reader_sort_central_dir_offsets_by_filename(pZip);

  return MZ_TRUE;
}

mz_bool mz_zip_reader_init(mz_zip_archive *pZip, mz_uint64 size, mz_uint32 flags)
{
  if ((!pZip) || (!pZip->m_pRead))
	return MZ_FALSE;
  if (!mz_zip_reader_init_internal(pZip, flags))
	return MZ_FALSE;
  pZip->m_archive_size = size;
  if (!mz_zip_reader_read_central_dir(pZip, flags))
  {
	mz_zip_reader_end(pZip);
	return MZ_FALSE;
  }
  return MZ_TRUE;
}

static size_t mz_zip_mem_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n)
{
  mz_zip_archive *pZip = (mz_zip_archive *)pOpaque;
  size_t s = (file_ofs >= pZip->m_archive_size) ? 0 : (size_t)MZ_MIN(pZip->m_archive_size - file_ofs, n);
  memcpy(pBuf, (const mz_uint8 *)pZip->m_pState->m_pMem + file_ofs, s);
  return s;
}

mz_bool mz_zip_reader_init_mem(mz_zip_archive *pZip, const void *pMem, size_t size, mz_uint32 flags)
{
  if (!mz_zip_reader_init_internal(pZip, flags))
	return MZ_FALSE;
  pZip->m_archive_size = size;
  pZip->m_pRead = mz_zip_mem_read_func;
  pZip->m_pIO_opaque = pZip;
#ifdef __cplusplus
  pZip->m_pState->m_pMem = const_cast<void *>(pMem);
#else
  pZip->m_pState->m_pMem = (void *)pMem;
#endif
  pZip->m_pState->m_mem_size = size;
  if (!mz_zip_reader_read_central_dir(pZip, flags))
  {
	mz_zip_reader_end(pZip);
	return MZ_FALSE;
  }
  return MZ_TRUE;
}

#ifndef MINIZ_NO_STDIO
static size_t mz_zip_file_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n)
{
  mz_zip_archive *pZip = (mz_zip_archive *)pOpaque;
  mz_int64 cur_ofs = MZ_FTELL64(pZip->m_pState->m_pFile);
  if (((mz_int64)file_ofs < 0) || (((cur_ofs != (mz_int64)file_ofs)) && (MZ_FSEEK64(pZip->m_pState->m_pFile, (mz_int64)file_ofs, SEEK_SET))))
	return 0;
  return MZ_FREAD(pBuf, 1, n, pZip->m_pState->m_pFile);
}

mz_bool mz_zip_reader_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint32 flags)
{
  mz_uint64 file_size;
  MZ_FILE *pFile = MZ_FOPEN(pFilename, "rb");
  if (!pFile)
	return MZ_FALSE;
  if (MZ_FSEEK64(pFile, 0, SEEK_END))
  {
	MZ_FCLOSE(pFile);
	return MZ_FALSE;
  }
  file_size = MZ_FTELL64(pFile);
  if (!mz_zip_reader_init_internal(pZip, flags))
  {
	MZ_FCLOSE(pFile);
	return MZ_FALSE;
  }
  pZip->m_pRead = mz_zip_file_read_func;
  pZip->m_pIO_opaque = pZip;
  pZip->m_pState->m_pFile = pFile;
  pZip->m_archive_size = file_size;
  if (!mz_zip_reader_read_central_dir(pZip, flags))
  {
	mz_zip_reader_end(pZip);
	return MZ_FALSE;
  }
  return MZ_TRUE;
}
#endif // #ifndef MINIZ_NO_STDIO

mz_uint mz_zip_reader_get_num_files(mz_zip_archive *pZip)
{
  return pZip ? pZip->m_total_files : 0;
}

static MZ_FORCEINLINE const mz_uint8 *mz_zip_reader_get_cdh(mz_zip_archive *pZip, mz_uint file_index)
{
  if ((!pZip) || (!pZip->m_pState) || (file_index >= pZip->m_total_files) || (pZip->m_zip_mode != MZ_ZIP_MODE_READING))
	return NULL;
  return &MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir, mz_uint8, MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, mz_uint32, file_index));
}

mz_bool mz_zip_reader_is_file_encrypted(mz_zip_archive *pZip, mz_uint file_index)
{
  mz_uint m_bit_flag;
  const mz_uint8 *p = mz_zip_reader_get_cdh(pZip, file_index);
  if (!p)
	return MZ_FALSE;
  m_bit_flag = MZ_READ_LE16(p + MZ_ZIP_CDH_BIT_FLAG_OFS);
  return (m_bit_flag & 1);
}

mz_bool mz_zip_reader_is_file_a_directory(mz_zip_archive *pZip, mz_uint file_index)
{
  mz_uint filename_len, external_attr;
  const mz_uint8 *p = mz_zip_reader_get_cdh(pZip, file_index);
  if (!p)
	return MZ_FALSE;

  // First see if the filename ends with a '/' character.
  filename_len = MZ_READ_LE16(p + MZ_ZIP_CDH_FILENAME_LEN_OFS);
  if (filename_len)
  {
	if (*(p + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + filename_len - 1) == '/')
	  return MZ_TRUE;
  }

  // Bugfix: This code was also checking if the internal attribute was non-zero, which wasn't correct.
  // Most/all zip writers (hopefully) set DOS file/directory attributes in the low 16-bits, so check for the DOS directory flag and ignore the source OS ID in the created by field.
  // FIXME: Remove this check? Is it necessary - we already check the filename.
  external_attr = MZ_READ_LE32(p + MZ_ZIP_CDH_EXTERNAL_ATTR_OFS);
  if ((external_attr & 0x10) != 0)
	return MZ_TRUE;

  return MZ_FALSE;
}

mz_bool mz_zip_reader_file_stat(mz_zip_archive *pZip, mz_uint file_index, mz_zip_archive_file_stat *pStat)
{
  mz_uint n;
  const mz_uint8 *p = mz_zip_reader_get_cdh(pZip, file_index);
  if ((!p) || (!pStat))
	return MZ_FALSE;

  // Unpack the central directory record.
  pStat->m_file_index = file_index;
  pStat->m_central_dir_ofs = MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, mz_uint32, file_index);
  pStat->m_version_made_by = MZ_READ_LE16(p + MZ_ZIP_CDH_VERSION_MADE_BY_OFS);
  pStat->m_version_needed = MZ_READ_LE16(p + MZ_ZIP_CDH_VERSION_NEEDED_OFS);
  pStat->m_bit_flag = MZ_READ_LE16(p + MZ_ZIP_CDH_BIT_FLAG_OFS);
  pStat->m_method = MZ_READ_LE16(p + MZ_ZIP_CDH_METHOD_OFS);
#ifndef MINIZ_NO_TIME
  pStat->m_time = mz_zip_dos_to_time_t(MZ_READ_LE16(p + MZ_ZIP_CDH_FILE_TIME_OFS), MZ_READ_LE16(p + MZ_ZIP_CDH_FILE_DATE_OFS));
#endif
  pStat->m_crc32 = MZ_READ_LE32(p + MZ_ZIP_CDH_CRC32_OFS);
  pStat->m_comp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
  pStat->m_uncomp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
  pStat->m_internal_attr = MZ_READ_LE16(p + MZ_ZIP_CDH_INTERNAL_ATTR_OFS);
  pStat->m_external_attr = MZ_READ_LE32(p + MZ_ZIP_CDH_EXTERNAL_ATTR_OFS);
  pStat->m_local_header_ofs = MZ_READ_LE32(p + MZ_ZIP_CDH_LOCAL_HEADER_OFS);

  // Copy as much of the filename and comment as possible.
  n = MZ_READ_LE16(p + MZ_ZIP_CDH_FILENAME_LEN_OFS); n = MZ_MIN(n, MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE - 1);
  memcpy(pStat->m_filename, p + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE, n); pStat->m_filename[n] = '\0';

  n = MZ_READ_LE16(p + MZ_ZIP_CDH_COMMENT_LEN_OFS); n = MZ_MIN(n, MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE - 1);
  pStat->m_comment_size = n;
  memcpy(pStat->m_comment, p + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + MZ_READ_LE16(p + MZ_ZIP_CDH_FILENAME_LEN_OFS) + MZ_READ_LE16(p + MZ_ZIP_CDH_EXTRA_LEN_OFS), n); pStat->m_comment[n] = '\0';

  return MZ_TRUE;
}

mz_uint mz_zip_reader_get_filename(mz_zip_archive *pZip, mz_uint file_index, char *pFilename, mz_uint filename_buf_size)
{
  mz_uint n;
  const mz_uint8 *p = mz_zip_reader_get_cdh(pZip, file_index);
  if (!p) { if (filename_buf_size) pFilename[0] = '\0'; return 0; }
  n = MZ_READ_LE16(p + MZ_ZIP_CDH_FILENAME_LEN_OFS);
  if (filename_buf_size)
  {
	n = MZ_MIN(n, filename_buf_size - 1);
	memcpy(pFilename, p + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE, n);
	pFilename[n] = '\0';
  }
  return n + 1;
}

static MZ_FORCEINLINE mz_bool mz_zip_reader_string_equal(const char *pA, const char *pB, mz_uint len, mz_uint flags)
{
  mz_uint i;
  if (flags & MZ_ZIP_FLAG_CASE_SENSITIVE)
	return 0 == memcmp(pA, pB, len);
  for (i = 0; i < len; ++i)
	if (MZ_TOLOWER(pA[i]) != MZ_TOLOWER(pB[i]))
	  return MZ_FALSE;
  return MZ_TRUE;
}

static MZ_FORCEINLINE int mz_zip_reader_filename_compare(const mz_zip_array *pCentral_dir_array, const mz_zip_array *pCentral_dir_offsets, mz_uint l_index, const char *pR, mz_uint r_len)
{
  const mz_uint8 *pL = &MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_array, mz_uint8, MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_offsets, mz_uint32, l_index)), *pE;
  mz_uint l_len = MZ_READ_LE16(pL + MZ_ZIP_CDH_FILENAME_LEN_OFS);
  mz_uint8 l = 0, r = 0;
  pL += MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;
  pE = pL + MZ_MIN(l_len, r_len);
  while (pL < pE)
  {
	if ((l = MZ_TOLOWER(*pL)) != (r = MZ_TOLOWER(*pR)))
	  break;
	pL++; pR++;
  }
  return (pL == pE) ? (int)(l_len - r_len) : (l - r);
}

static int mz_zip_reader_locate_file_binary_search(mz_zip_archive *pZip, const char *pFilename)
{
  mz_zip_internal_state *pState = pZip->m_pState;
  const mz_zip_array *pCentral_dir_offsets = &pState->m_central_dir_offsets;
  const mz_zip_array *pCentral_dir = &pState->m_central_dir;
  mz_uint32 *pIndices = &MZ_ZIP_ARRAY_ELEMENT(&pState->m_sorted_central_dir_offsets, mz_uint32, 0);
  const int size = pZip->m_total_files;
  const mz_uint filename_len = (mz_uint)strlen(pFilename);
  int l = 0, h = size - 1;
  while (l <= h)
  {
	int m = (l + h) >> 1, file_index = pIndices[m], comp = mz_zip_reader_filename_compare(pCentral_dir, pCentral_dir_offsets, file_index, pFilename, filename_len);
	if (!comp)
	  return file_index;
	else if (comp < 0)
	  l = m + 1;
	else
	  h = m - 1;
  }
  return -1;
}

int mz_zip_reader_locate_file(mz_zip_archive *pZip, const char *pName, const char *pComment, mz_uint flags)
{
  mz_uint file_index; size_t name_len, comment_len;
  if ((!pZip) || (!pZip->m_pState) || (!pName) || (pZip->m_zip_mode != MZ_ZIP_MODE_READING))
	return -1;
  if (((flags & (MZ_ZIP_FLAG_IGNORE_PATH | MZ_ZIP_FLAG_CASE_SENSITIVE)) == 0) && (!pComment) && (pZip->m_pState->m_sorted_central_dir_offsets.m_size))
	return mz_zip_reader_locate_file_binary_search(pZip, pName);
  name_len = strlen(pName); if (name_len > 0xFFFF) return -1;
  comment_len = pComment ? strlen(pComment) : 0; if (comment_len > 0xFFFF) return -1;
  for (file_index = 0; file_index < pZip->m_total_files; file_index++)
  {
	const mz_uint8 *pHeader = &MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir, mz_uint8, MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, mz_uint32, file_index));
	mz_uint filename_len = MZ_READ_LE16(pHeader + MZ_ZIP_CDH_FILENAME_LEN_OFS);
	const char *pFilename = (const char *)pHeader + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;
	if (filename_len < name_len)
	  continue;
	if (comment_len)
	{
	  mz_uint file_extra_len = MZ_READ_LE16(pHeader + MZ_ZIP_CDH_EXTRA_LEN_OFS), file_comment_len = MZ_READ_LE16(pHeader + MZ_ZIP_CDH_COMMENT_LEN_OFS);
	  const char *pFile_comment = pFilename + filename_len + file_extra_len;
	  if ((file_comment_len != comment_len) || (!mz_zip_reader_string_equal(pComment, pFile_comment, file_comment_len, flags)))
		continue;
	}
	if ((flags & MZ_ZIP_FLAG_IGNORE_PATH) && (filename_len))
	{
	  int ofs = filename_len - 1;
	  do
	  {
		if ((pFilename[ofs] == '/') || (pFilename[ofs] == '\\') || (pFilename[ofs] == ':'))
		  break;
	  } while (--ofs >= 0);
	  ofs++;
	  pFilename += ofs; filename_len -= ofs;
	}
	if ((filename_len == name_len) && (mz_zip_reader_string_equal(pName, pFilename, filename_len, flags)))
	  return file_index;
  }
  return -1;
}

mz_bool mz_zip_reader_extract_to_mem_no_alloc(mz_zip_archive *pZip, mz_uint file_index, void *pBuf, size_t buf_size, mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size)
{
  int status = TINFL_STATUS_DONE;
  mz_uint64 needed_size, cur_file_ofs, comp_remaining, out_buf_ofs = 0, read_buf_size, read_buf_ofs = 0, read_buf_avail;
  mz_zip_archive_file_stat file_stat;
  void *pRead_buf;
  mz_uint32 local_header_u32[(MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(mz_uint32) - 1) / sizeof(mz_uint32)]; mz_uint8 *pLocal_header = (mz_uint8 *)local_header_u32;
  tinfl_decompressor inflator;

  if ((buf_size) && (!pBuf))
	return MZ_FALSE;

  if (!mz_zip_reader_file_stat(pZip, file_index, &file_stat))
	return MZ_FALSE;

  // Empty file, or a directory (but not always a directory - I've seen odd zips with directories that have compressed data which inflates to 0 bytes)
  if (!file_stat.m_comp_size)
	return MZ_TRUE;

  // Entry is a subdirectory (I've seen old zips with dir entries which have compressed deflate data which inflates to 0 bytes, but these entries claim to uncompress to 512 bytes in the headers).
  // I'm torn how to handle this case - should it fail instead?
  if (mz_zip_reader_is_file_a_directory(pZip, file_index))
	return MZ_TRUE;

  // Encryption and patch files are not supported.
  if (file_stat.m_bit_flag & (1 | 32))
	return MZ_FALSE;

  // This function only supports stored and deflate.
  if ((!(flags & MZ_ZIP_FLAG_COMPRESSED_DATA)) && (file_stat.m_method != 0) && (file_stat.m_method != MZ_DEFLATED))
	return MZ_FALSE;

  // Ensure supplied output buffer is large enough.
  needed_size = (flags & MZ_ZIP_FLAG_COMPRESSED_DATA) ? file_stat.m_comp_size : file_stat.m_uncomp_size;
  if (buf_size < needed_size)
	return MZ_FALSE;

  // Read and parse the local directory entry.
  cur_file_ofs = file_stat.m_local_header_ofs;
  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pLocal_header, MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
	return MZ_FALSE;
  if (MZ_READ_LE32(pLocal_header) != MZ_ZIP_LOCAL_DIR_HEADER_SIG)
	return MZ_FALSE;

  cur_file_ofs += MZ_ZIP_LOCAL_DIR_HEADER_SIZE + MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_FILENAME_LEN_OFS) + MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_EXTRA_LEN_OFS);
  if ((cur_file_ofs + file_stat.m_comp_size) > pZip->m_archive_size)
	return MZ_FALSE;

  if ((flags & MZ_ZIP_FLAG_COMPRESSED_DATA) || (!file_stat.m_method))
  {
	// The file is stored or the caller has requested the compressed data.
	if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pBuf, (size_t)needed_size) != needed_size)
	  return MZ_FALSE;
	return ((flags & MZ_ZIP_FLAG_COMPRESSED_DATA) != 0) || (mz_crc32(MZ_CRC32_INIT, (const mz_uint8 *)pBuf, (size_t)file_stat.m_uncomp_size) == file_stat.m_crc32);
  }

  // Decompress the file either directly from memory or from a file input buffer.
  tinfl_init(&inflator);

  if (pZip->m_pState->m_pMem)
  {
	// Read directly from the archive in memory.
	pRead_buf = (mz_uint8 *)pZip->m_pState->m_pMem + cur_file_ofs;
	read_buf_size = read_buf_avail = file_stat.m_comp_size;
	comp_remaining = 0;
  }
  else if (pUser_read_buf)
  {
	// Use a user provided read buffer.
	if (!user_read_buf_size)
	  return MZ_FALSE;
	pRead_buf = (mz_uint8 *)pUser_read_buf;
	read_buf_size = user_read_buf_size;
	read_buf_avail = 0;
	comp_remaining = file_stat.m_comp_size;
  }
  else
  {
	// Temporarily allocate a read buffer.
	read_buf_size = MZ_MIN(file_stat.m_comp_size, MZ_ZIP_MAX_IO_BUF_SIZE);
#ifdef _MSC_VER
	if (((0, sizeof(size_t) == sizeof(mz_uint32))) && (read_buf_size > 0x7FFFFFFF))
#else
	if (((sizeof(size_t) == sizeof(mz_uint32))) && (read_buf_size > 0x7FFFFFFF))
#endif
	  return MZ_FALSE;
	if (NULL == (pRead_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)read_buf_size)))
	  return MZ_FALSE;
	read_buf_avail = 0;
	comp_remaining = file_stat.m_comp_size;
  }

  do
  {
	size_t in_buf_size, out_buf_size = (size_t)(file_stat.m_uncomp_size - out_buf_ofs);
	if ((!read_buf_avail) && (!pZip->m_pState->m_pMem))
	{
	  read_buf_avail = MZ_MIN(read_buf_size, comp_remaining);
	  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
	  {
		status = TINFL_STATUS_FAILED;
		break;
	  }
	  cur_file_ofs += read_buf_avail;
	  comp_remaining -= read_buf_avail;
	  read_buf_ofs = 0;
	}
	in_buf_size = (size_t)read_buf_avail;
	status = tinfl_decompress(&inflator, (mz_uint8 *)pRead_buf + read_buf_ofs, &in_buf_size, (mz_uint8 *)pBuf, (mz_uint8 *)pBuf + out_buf_ofs, &out_buf_size, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF | (comp_remaining ? TINFL_FLAG_HAS_MORE_INPUT : 0));
	read_buf_avail -= in_buf_size;
	read_buf_ofs += in_buf_size;
	out_buf_ofs += out_buf_size;
  } while (status == TINFL_STATUS_NEEDS_MORE_INPUT);

  if (status == TINFL_STATUS_DONE)
  {
	// Make sure the entire file was decompressed, and check its CRC.
	if ((out_buf_ofs != file_stat.m_uncomp_size) || (mz_crc32(MZ_CRC32_INIT, (const mz_uint8 *)pBuf, (size_t)file_stat.m_uncomp_size) != file_stat.m_crc32))
	  status = TINFL_STATUS_FAILED;
  }

  if ((!pZip->m_pState->m_pMem) && (!pUser_read_buf))
	pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);

  return status == TINFL_STATUS_DONE;
}

mz_bool mz_zip_reader_extract_file_to_mem_no_alloc(mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size)
{
  int file_index = mz_zip_reader_locate_file(pZip, pFilename, NULL, flags);
  if (file_index < 0)
	return MZ_FALSE;
  return mz_zip_reader_extract_to_mem_no_alloc(pZip, file_index, pBuf, buf_size, flags, pUser_read_buf, user_read_buf_size);
}

mz_bool mz_zip_reader_extract_to_mem(mz_zip_archive *pZip, mz_uint file_index, void *pBuf, size_t buf_size, mz_uint flags)
{
  return mz_zip_reader_extract_to_mem_no_alloc(pZip, file_index, pBuf, buf_size, flags, NULL, 0);
}

mz_bool mz_zip_reader_extract_file_to_mem(mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, mz_uint flags)
{
  return mz_zip_reader_extract_file_to_mem_no_alloc(pZip, pFilename, pBuf, buf_size, flags, NULL, 0);
}

void *mz_zip_reader_extract_to_heap(mz_zip_archive *pZip, mz_uint file_index, size_t *pSize, mz_uint flags)
{
  mz_uint64 comp_size, uncomp_size, alloc_size;
  const mz_uint8 *p = mz_zip_reader_get_cdh(pZip, file_index);
  void *pBuf;

  if (pSize)
	*pSize = 0;
  if (!p)
	return NULL;

  comp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
  uncomp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);

  alloc_size = (flags & MZ_ZIP_FLAG_COMPRESSED_DATA) ? comp_size : uncomp_size;
#ifdef _MSC_VER
  if (((0, sizeof(size_t) == sizeof(mz_uint32))) && (alloc_size > 0x7FFFFFFF))
#else
  if (((sizeof(size_t) == sizeof(mz_uint32))) && (alloc_size > 0x7FFFFFFF))
#endif
	return NULL;
  if (NULL == (pBuf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)alloc_size)))
	return NULL;

  if (!mz_zip_reader_extract_to_mem(pZip, file_index, pBuf, (size_t)alloc_size, flags))
  {
	pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
	return NULL;
  }

  if (pSize) *pSize = (size_t)alloc_size;
  return pBuf;
}

void *mz_zip_reader_extract_file_to_heap(mz_zip_archive *pZip, const char *pFilename, size_t *pSize, mz_uint flags)
{
  int file_index = mz_zip_reader_locate_file(pZip, pFilename, NULL, flags);
  if (file_index < 0)
  {
	if (pSize) *pSize = 0;
	return MZ_FALSE;
  }
  return mz_zip_reader_extract_to_heap(pZip, file_index, pSize, flags);
}

mz_bool mz_zip_reader_extract_to_callback(mz_zip_archive *pZip, mz_uint file_index, mz_file_write_func pCallback, void *pOpaque, mz_uint flags)
{
  int status = TINFL_STATUS_DONE; mz_uint file_crc32 = MZ_CRC32_INIT;
  mz_uint64 read_buf_size, read_buf_ofs = 0, read_buf_avail, comp_remaining, out_buf_ofs = 0, cur_file_ofs;
  mz_zip_archive_file_stat file_stat;
  void *pRead_buf = NULL; void *pWrite_buf = NULL;
  mz_uint32 local_header_u32[(MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(mz_uint32) - 1) / sizeof(mz_uint32)]; mz_uint8 *pLocal_header = (mz_uint8 *)local_header_u32;

  if (!mz_zip_reader_file_stat(pZip, file_index, &file_stat))
	return MZ_FALSE;

  // Empty file, or a directory (but not always a directory - I've seen odd zips with directories that have compressed data which inflates to 0 bytes)
  if (!file_stat.m_comp_size)
	return MZ_TRUE;

  // Entry is a subdirectory (I've seen old zips with dir entries which have compressed deflate data which inflates to 0 bytes, but these entries claim to uncompress to 512 bytes in the headers).
  // I'm torn how to handle this case - should it fail instead?
  if (mz_zip_reader_is_file_a_directory(pZip, file_index))
	return MZ_TRUE;

  // Encryption and patch files are not supported.
  if (file_stat.m_bit_flag & (1 | 32))
	return MZ_FALSE;

  // This function only supports stored and deflate.
  if ((!(flags & MZ_ZIP_FLAG_COMPRESSED_DATA)) && (file_stat.m_method != 0) && (file_stat.m_method != MZ_DEFLATED))
	return MZ_FALSE;

  // Read and parse the local directory entry.
  cur_file_ofs = file_stat.m_local_header_ofs;
  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pLocal_header, MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
	return MZ_FALSE;
  if (MZ_READ_LE32(pLocal_header) != MZ_ZIP_LOCAL_DIR_HEADER_SIG)
	return MZ_FALSE;

  cur_file_ofs += MZ_ZIP_LOCAL_DIR_HEADER_SIZE + MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_FILENAME_LEN_OFS) + MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_EXTRA_LEN_OFS);
  if ((cur_file_ofs + file_stat.m_comp_size) > pZip->m_archive_size)
	return MZ_FALSE;

  // Decompress the file either directly from memory or from a file input buffer.
  if (pZip->m_pState->m_pMem)
  {
	pRead_buf = (mz_uint8 *)pZip->m_pState->m_pMem + cur_file_ofs;
	read_buf_size = read_buf_avail = file_stat.m_comp_size;
	comp_remaining = 0;
  }
  else
  {
	read_buf_size = MZ_MIN(file_stat.m_comp_size, MZ_ZIP_MAX_IO_BUF_SIZE);
	if (NULL == (pRead_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)read_buf_size)))
	  return MZ_FALSE;
	read_buf_avail = 0;
	comp_remaining = file_stat.m_comp_size;
  }

  if ((flags & MZ_ZIP_FLAG_COMPRESSED_DATA) || (!file_stat.m_method))
  {
	// The file is stored or the caller has requested the compressed data.
	if (pZip->m_pState->m_pMem)
	{
#ifdef _MSC_VER
	  if (((0, sizeof(size_t) == sizeof(mz_uint32))) && (file_stat.m_comp_size > 0xFFFFFFFF))
#else
	  if (((sizeof(size_t) == sizeof(mz_uint32))) && (file_stat.m_comp_size > 0xFFFFFFFF))
#endif
		return MZ_FALSE;
	  if (pCallback(pOpaque, out_buf_ofs, pRead_buf, (size_t)file_stat.m_comp_size) != file_stat.m_comp_size)
		status = TINFL_STATUS_FAILED;
	  else if (!(flags & MZ_ZIP_FLAG_COMPRESSED_DATA))
		file_crc32 = (mz_uint32)mz_crc32(file_crc32, (const mz_uint8 *)pRead_buf, (size_t)file_stat.m_comp_size);
	  cur_file_ofs += file_stat.m_comp_size;
	  out_buf_ofs += file_stat.m_comp_size;
	  comp_remaining = 0;
	}
	else
	{
	  while (comp_remaining)
	  {
		read_buf_avail = MZ_MIN(read_buf_size, comp_remaining);
		if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
		{
		  status = TINFL_STATUS_FAILED;
		  break;
		}

		if (!(flags & MZ_ZIP_FLAG_COMPRESSED_DATA))
		  file_crc32 = (mz_uint32)mz_crc32(file_crc32, (const mz_uint8 *)pRead_buf, (size_t)read_buf_avail);

		if (pCallback(pOpaque, out_buf_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
		{
		  status = TINFL_STATUS_FAILED;
		  break;
		}
		cur_file_ofs += read_buf_avail;
		out_buf_ofs += read_buf_avail;
		comp_remaining -= read_buf_avail;
	  }
	}
  }
  else
  {
	tinfl_decompressor inflator;
	tinfl_init(&inflator);

	if (NULL == (pWrite_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, TINFL_LZ_DICT_SIZE)))
	  status = TINFL_STATUS_FAILED;
	else
	{
	  do
	  {
		mz_uint8 *pWrite_buf_cur = (mz_uint8 *)pWrite_buf + (out_buf_ofs & (TINFL_LZ_DICT_SIZE - 1));
		size_t in_buf_size, out_buf_size = TINFL_LZ_DICT_SIZE - (out_buf_ofs & (TINFL_LZ_DICT_SIZE - 1));
		if ((!read_buf_avail) && (!pZip->m_pState->m_pMem))
		{
		  read_buf_avail = MZ_MIN(read_buf_size, comp_remaining);
		  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
		  {
			status = TINFL_STATUS_FAILED;
			break;
		  }
		  cur_file_ofs += read_buf_avail;
		  comp_remaining -= read_buf_avail;
		  read_buf_ofs = 0;
		}

		in_buf_size = (size_t)read_buf_avail;
		status = tinfl_decompress(&inflator, (const mz_uint8 *)pRead_buf + read_buf_ofs, &in_buf_size, (mz_uint8 *)pWrite_buf, pWrite_buf_cur, &out_buf_size, comp_remaining ? TINFL_FLAG_HAS_MORE_INPUT : 0);
		read_buf_avail -= in_buf_size;
		read_buf_ofs += in_buf_size;

		if (out_buf_size)
		{
		  if (pCallback(pOpaque, out_buf_ofs, pWrite_buf_cur, out_buf_size) != out_buf_size)
		  {
			status = TINFL_STATUS_FAILED;
			break;
		  }
		  file_crc32 = (mz_uint32)mz_crc32(file_crc32, pWrite_buf_cur, out_buf_size);
		  if ((out_buf_ofs += out_buf_size) > file_stat.m_uncomp_size)
		  {
			status = TINFL_STATUS_FAILED;
			break;
		  }
		}
	  } while ((status == TINFL_STATUS_NEEDS_MORE_INPUT) || (status == TINFL_STATUS_HAS_MORE_OUTPUT));
	}
  }

  if ((status == TINFL_STATUS_DONE) && (!(flags & MZ_ZIP_FLAG_COMPRESSED_DATA)))
  {
	// Make sure the entire file was decompressed, and check its CRC.
	if ((out_buf_ofs != file_stat.m_uncomp_size) || (file_crc32 != file_stat.m_crc32))
	  status = TINFL_STATUS_FAILED;
  }

  if (!pZip->m_pState->m_pMem)
	pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
  if (pWrite_buf)
	pZip->m_pFree(pZip->m_pAlloc_opaque, pWrite_buf);

  return status == TINFL_STATUS_DONE;
}

mz_bool mz_zip_reader_extract_file_to_callback(mz_zip_archive *pZip, const char *pFilename, mz_file_write_func pCallback, void *pOpaque, mz_uint flags)
{
  int file_index = mz_zip_reader_locate_file(pZip, pFilename, NULL, flags);
  if (file_index < 0)
	return MZ_FALSE;
  return mz_zip_reader_extract_to_callback(pZip, file_index, pCallback, pOpaque, flags);
}

#ifndef MINIZ_NO_STDIO
static size_t mz_zip_file_write_callback(void *pOpaque, mz_uint64 ofs, const void *pBuf, size_t n)
{
  (void)ofs; return MZ_FWRITE(pBuf, 1, n, (MZ_FILE*)pOpaque);
}

mz_bool mz_zip_reader_extract_to_file(mz_zip_archive *pZip, mz_uint file_index, const char *pDst_filename, mz_uint flags)
{
  mz_bool status;
  mz_zip_archive_file_stat file_stat;
  MZ_FILE *pFile;
  if (!mz_zip_reader_file_stat(pZip, file_index, &file_stat))
	return MZ_FALSE;
  pFile = MZ_FOPEN(pDst_filename, "wb");
  if (!pFile)
	return MZ_FALSE;
  status = mz_zip_reader_extract_to_callback(pZip, file_index, mz_zip_file_write_callback, pFile, flags);
  if (MZ_FCLOSE(pFile) == EOF)
	return MZ_FALSE;
#ifndef MINIZ_NO_TIME
  if (status)
	mz_zip_set_file_times(pDst_filename, file_stat.m_time, file_stat.m_time);
#endif
  return status;
}
#endif // #ifndef MINIZ_NO_STDIO

mz_bool mz_zip_reader_end(mz_zip_archive *pZip)
{
  if ((!pZip) || (!pZip->m_pState) || (!pZip->m_pAlloc) || (!pZip->m_pFree) || (pZip->m_zip_mode != MZ_ZIP_MODE_READING))
	return MZ_FALSE;

  if (pZip->m_pState)
  {
	mz_zip_internal_state *pState = pZip->m_pState; pZip->m_pState = NULL;
	mz_zip_array_clear(pZip, &pState->m_central_dir);
	mz_zip_array_clear(pZip, &pState->m_central_dir_offsets);
	mz_zip_array_clear(pZip, &pState->m_sorted_central_dir_offsets);

#ifndef MINIZ_NO_STDIO
	if (pState->m_pFile)
	{
	  MZ_FCLOSE(pState->m_pFile);
	  pState->m_pFile = NULL;
	}
#endif // #ifndef MINIZ_NO_STDIO

	pZip->m_pFree(pZip->m_pAlloc_opaque, pState);
  }
  pZip->m_zip_mode = MZ_ZIP_MODE_INVALID;

  return MZ_TRUE;
}

#ifndef MINIZ_NO_STDIO
mz_bool mz_zip_reader_extract_file_to_file(mz_zip_archive *pZip, const char *pArchive_filename, const char *pDst_filename, mz_uint flags)
{
  int file_index = mz_zip_reader_locate_file(pZip, pArchive_filename, NULL, flags);
  if (file_index < 0)
	return MZ_FALSE;
  return mz_zip_reader_extract_to_file(pZip, file_index, pDst_filename, flags);
}
#endif

// ------------------- .ZIP archive writing

#ifndef MINIZ_NO_ARCHIVE_WRITING_APIS

static void mz_write_le16(mz_uint8 *p, mz_uint16 v) { p[0] = (mz_uint8)v; p[1] = (mz_uint8)(v >> 8); }
static void mz_write_le32(mz_uint8 *p, mz_uint32 v) { p[0] = (mz_uint8)v; p[1] = (mz_uint8)(v >> 8); p[2] = (mz_uint8)(v >> 16); p[3] = (mz_uint8)(v >> 24); }
#define MZ_WRITE_LE16(p, v) mz_write_le16((mz_uint8 *)(p), (mz_uint16)(v))
#define MZ_WRITE_LE32(p, v) mz_write_le32((mz_uint8 *)(p), (mz_uint32)(v))

mz_bool mz_zip_writer_init(mz_zip_archive *pZip, mz_uint64 existing_size)
{
  if ((!pZip) || (pZip->m_pState) || (!pZip->m_pWrite) || (pZip->m_zip_mode != MZ_ZIP_MODE_INVALID))
	return MZ_FALSE;

  if (pZip->m_file_offset_alignment)
  {
	// Ensure user specified file offset alignment is a power of 2.
	if (pZip->m_file_offset_alignment & (pZip->m_file_offset_alignment - 1))
	  return MZ_FALSE;
  }

  if (!pZip->m_pAlloc) pZip->m_pAlloc = def_alloc_func;
  if (!pZip->m_pFree) pZip->m_pFree = def_free_func;
  if (!pZip->m_pRealloc) pZip->m_pRealloc = def_realloc_func;

  pZip->m_zip_mode = MZ_ZIP_MODE_WRITING;
  pZip->m_archive_size = existing_size;
  pZip->m_central_directory_file_ofs = 0;
  pZip->m_total_files = 0;

  if (NULL == (pZip->m_pState = (mz_zip_internal_state *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(mz_zip_internal_state))))
	return MZ_FALSE;
  memset(pZip->m_pState, 0, sizeof(mz_zip_internal_state));
  MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir, sizeof(mz_uint8));
  MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir_offsets, sizeof(mz_uint32));
  MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_sorted_central_dir_offsets, sizeof(mz_uint32));
  return MZ_TRUE;
}

static size_t mz_zip_heap_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n)
{
  mz_zip_archive *pZip = (mz_zip_archive *)pOpaque;
  mz_zip_internal_state *pState = pZip->m_pState;
  mz_uint64 new_size = MZ_MAX(file_ofs + n, pState->m_mem_size);
#ifdef _MSC_VER
  if ((!n) || ((0, sizeof(size_t) == sizeof(mz_uint32)) && (new_size > 0x7FFFFFFF)))
#else
  if ((!n) || ((sizeof(size_t) == sizeof(mz_uint32)) && (new_size > 0x7FFFFFFF)))
#endif
	return 0;
  if (new_size > pState->m_mem_capacity)
  {
	void *pNew_block;
	size_t new_capacity = MZ_MAX(64, pState->m_mem_capacity); while (new_capacity < new_size) new_capacity *= 2;
	if (NULL == (pNew_block = pZip->m_pRealloc(pZip->m_pAlloc_opaque, pState->m_pMem, 1, new_capacity)))
	  return 0;
	pState->m_pMem = pNew_block; pState->m_mem_capacity = new_capacity;
  }
  memcpy((mz_uint8 *)pState->m_pMem + file_ofs, pBuf, n);
  pState->m_mem_size = (size_t)new_size;
  return n;
}

mz_bool mz_zip_writer_init_heap(mz_zip_archive *pZip, size_t size_to_reserve_at_beginning, size_t initial_allocation_size)
{
  pZip->m_pWrite = mz_zip_heap_write_func;
  pZip->m_pIO_opaque = pZip;
  if (!mz_zip_writer_init(pZip, size_to_reserve_at_beginning))
	return MZ_FALSE;
  if (0 != (initial_allocation_size = MZ_MAX(initial_allocation_size, size_to_reserve_at_beginning)))
  {
	if (NULL == (pZip->m_pState->m_pMem = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, initial_allocation_size)))
	{
	  mz_zip_writer_end(pZip);
	  return MZ_FALSE;
	}
	pZip->m_pState->m_mem_capacity = initial_allocation_size;
  }
  return MZ_TRUE;
}

#ifndef MINIZ_NO_STDIO
static size_t mz_zip_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n)
{
  mz_zip_archive *pZip = (mz_zip_archive *)pOpaque;
  mz_int64 cur_ofs = MZ_FTELL64(pZip->m_pState->m_pFile);
  if (((mz_int64)file_ofs < 0) || (((cur_ofs != (mz_int64)file_ofs)) && (MZ_FSEEK64(pZip->m_pState->m_pFile, (mz_int64)file_ofs, SEEK_SET))))
	return 0;
  return MZ_FWRITE(pBuf, 1, n, pZip->m_pState->m_pFile);
}

mz_bool mz_zip_writer_init_file(mz_zip_archive *pZip, const char *pFilename, mz_uint64 size_to_reserve_at_beginning)
{
  MZ_FILE *pFile;
  pZip->m_pWrite = mz_zip_file_write_func;
  pZip->m_pIO_opaque = pZip;
  if (!mz_zip_writer_init(pZip, size_to_reserve_at_beginning))
	return MZ_FALSE;
  if (NULL == (pFile = MZ_FOPEN(pFilename, "wb")))
  {
	mz_zip_writer_end(pZip);
	return MZ_FALSE;
  }
  pZip->m_pState->m_pFile = pFile;
  if (size_to_reserve_at_beginning)
  {
	mz_uint64 cur_ofs = 0; char buf[4096]; MZ_CLEAR_OBJ(buf);
	do
	{
	  size_t n = (size_t)MZ_MIN(sizeof(buf), size_to_reserve_at_beginning);
	  if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_ofs, buf, n) != n)
	  {
		mz_zip_writer_end(pZip);
		return MZ_FALSE;
	  }
	  cur_ofs += n; size_to_reserve_at_beginning -= n;
	} while (size_to_reserve_at_beginning);
  }
  return MZ_TRUE;
}
#endif // #ifndef MINIZ_NO_STDIO

mz_bool mz_zip_writer_init_from_reader(mz_zip_archive *pZip, const char *pFilename)
{
  mz_zip_internal_state *pState;
  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != MZ_ZIP_MODE_READING))
	return MZ_FALSE;
  // No sense in trying to write to an archive that's already at the support max size
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + MZ_ZIP_LOCAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
	return MZ_FALSE;

  pState = pZip->m_pState;

  if (pState->m_pFile)
  {
#ifdef MINIZ_NO_STDIO
	pFilename; return MZ_FALSE;
#else
	// Archive is being read from stdio - try to reopen as writable.
	if (pZip->m_pIO_opaque != pZip)
	  return MZ_FALSE;
	if (!pFilename)
	  return MZ_FALSE;
	pZip->m_pWrite = mz_zip_file_write_func;
	if (NULL == (pState->m_pFile = MZ_FREOPEN(pFilename, "r+b", pState->m_pFile)))
	{
	  // The mz_zip_archive is now in a bogus state because pState->m_pFile is NULL, so just close it.
	  mz_zip_reader_end(pZip);
	  return MZ_FALSE;
	}
#endif // #ifdef MINIZ_NO_STDIO
  }
  else if (pState->m_pMem)
  {
	// Archive lives in a memory block. Assume it's from the heap that we can resize using the realloc callback.
	if (pZip->m_pIO_opaque != pZip)
	  return MZ_FALSE;
	pState->m_mem_capacity = pState->m_mem_size;
	pZip->m_pWrite = mz_zip_heap_write_func;
  }
  // Archive is being read via a user provided read function - make sure the user has specified a write function too.
  else if (!pZip->m_pWrite)
	return MZ_FALSE;

  // Start writing new files at the archive's current central directory location.
  pZip->m_archive_size = pZip->m_central_directory_file_ofs;
  pZip->m_zip_mode = MZ_ZIP_MODE_WRITING;
  pZip->m_central_directory_file_ofs = 0;

  return MZ_TRUE;
}

mz_bool mz_zip_writer_add_mem(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, mz_uint level_and_flags)
{
  return mz_zip_writer_add_mem_ex(pZip, pArchive_name, pBuf, buf_size, NULL, 0, level_and_flags, 0, 0);
}

typedef struct
{
  mz_zip_archive *m_pZip;
  mz_uint64 m_cur_archive_file_ofs;
  mz_uint64 m_comp_size;
} mz_zip_writer_add_state;

static mz_bool mz_zip_writer_add_put_buf_callback(const void* pBuf, int len, void *pUser)
{
  mz_zip_writer_add_state *pState = (mz_zip_writer_add_state *)pUser;
  if ((int)pState->m_pZip->m_pWrite(pState->m_pZip->m_pIO_opaque, pState->m_cur_archive_file_ofs, pBuf, len) != len)
	return MZ_FALSE;
  pState->m_cur_archive_file_ofs += len;
  pState->m_comp_size += len;
  return MZ_TRUE;
}

static mz_bool mz_zip_writer_create_local_dir_header(mz_zip_archive *pZip, mz_uint8 *pDst, mz_uint16 filename_size, mz_uint16 extra_size, mz_uint64 uncomp_size, mz_uint64 comp_size, mz_uint32 uncomp_crc32, mz_uint16 method, mz_uint16 bit_flags, mz_uint16 dos_time, mz_uint16 dos_date)
{
  (void)pZip;
  memset(pDst, 0, MZ_ZIP_LOCAL_DIR_HEADER_SIZE);
  MZ_WRITE_LE32(pDst + MZ_ZIP_LDH_SIG_OFS, MZ_ZIP_LOCAL_DIR_HEADER_SIG);
  MZ_WRITE_LE16(pDst + MZ_ZIP_LDH_VERSION_NEEDED_OFS, method ? 20 : 0);
  MZ_WRITE_LE16(pDst + MZ_ZIP_LDH_BIT_FLAG_OFS, bit_flags);
  MZ_WRITE_LE16(pDst + MZ_ZIP_LDH_METHOD_OFS, method);
  MZ_WRITE_LE16(pDst + MZ_ZIP_LDH_FILE_TIME_OFS, dos_time);
  MZ_WRITE_LE16(pDst + MZ_ZIP_LDH_FILE_DATE_OFS, dos_date);
  MZ_WRITE_LE32(pDst + MZ_ZIP_LDH_CRC32_OFS, uncomp_crc32);
  MZ_WRITE_LE32(pDst + MZ_ZIP_LDH_COMPRESSED_SIZE_OFS, comp_size);
  MZ_WRITE_LE32(pDst + MZ_ZIP_LDH_DECOMPRESSED_SIZE_OFS, uncomp_size);
  MZ_WRITE_LE16(pDst + MZ_ZIP_LDH_FILENAME_LEN_OFS, filename_size);
  MZ_WRITE_LE16(pDst + MZ_ZIP_LDH_EXTRA_LEN_OFS, extra_size);
  return MZ_TRUE;
}

static mz_bool mz_zip_writer_create_central_dir_header(mz_zip_archive *pZip, mz_uint8 *pDst, mz_uint16 filename_size, mz_uint16 extra_size, mz_uint16 comment_size, mz_uint64 uncomp_size, mz_uint64 comp_size, mz_uint32 uncomp_crc32, mz_uint16 method, mz_uint16 bit_flags, mz_uint16 dos_time, mz_uint16 dos_date, mz_uint64 local_header_ofs, mz_uint32 ext_attributes)
{
  (void)pZip;
  memset(pDst, 0, MZ_ZIP_CENTRAL_DIR_HEADER_SIZE);
  MZ_WRITE_LE32(pDst + MZ_ZIP_CDH_SIG_OFS, MZ_ZIP_CENTRAL_DIR_HEADER_SIG);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_VERSION_NEEDED_OFS, method ? 20 : 0);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_BIT_FLAG_OFS, bit_flags);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_METHOD_OFS, method);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_FILE_TIME_OFS, dos_time);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_FILE_DATE_OFS, dos_date);
  MZ_WRITE_LE32(pDst + MZ_ZIP_CDH_CRC32_OFS, uncomp_crc32);
  MZ_WRITE_LE32(pDst + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS, comp_size);
  MZ_WRITE_LE32(pDst + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS, uncomp_size);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_FILENAME_LEN_OFS, filename_size);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_EXTRA_LEN_OFS, extra_size);
  MZ_WRITE_LE16(pDst + MZ_ZIP_CDH_COMMENT_LEN_OFS, comment_size);
  MZ_WRITE_LE32(pDst + MZ_ZIP_CDH_EXTERNAL_ATTR_OFS, ext_attributes);
  MZ_WRITE_LE32(pDst + MZ_ZIP_CDH_LOCAL_HEADER_OFS, local_header_ofs);
  return MZ_TRUE;
}

static mz_bool mz_zip_writer_add_to_central_dir(mz_zip_archive *pZip, const char *pFilename, mz_uint16 filename_size, const void *pExtra, mz_uint16 extra_size, const void *pComment, mz_uint16 comment_size, mz_uint64 uncomp_size, mz_uint64 comp_size, mz_uint32 uncomp_crc32, mz_uint16 method, mz_uint16 bit_flags, mz_uint16 dos_time, mz_uint16 dos_date, mz_uint64 local_header_ofs, mz_uint32 ext_attributes)
{
  mz_zip_internal_state *pState = pZip->m_pState;
  mz_uint32 central_dir_ofs = (mz_uint32)pState->m_central_dir.m_size;
  size_t orig_central_dir_size = pState->m_central_dir.m_size;
  mz_uint8 central_dir_header[MZ_ZIP_CENTRAL_DIR_HEADER_SIZE];

  // No zip64 support yet
  if ((local_header_ofs > 0xFFFFFFFF) || (((mz_uint64)pState->m_central_dir.m_size + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + filename_size + extra_size + comment_size) > 0xFFFFFFFF))
	return MZ_FALSE;

  if (!mz_zip_writer_create_central_dir_header(pZip, central_dir_header, filename_size, extra_size, comment_size, uncomp_size, comp_size, uncomp_crc32, method, bit_flags, dos_time, dos_date, local_header_ofs, ext_attributes))
	return MZ_FALSE;

  if ((!mz_zip_array_push_back(pZip, &pState->m_central_dir, central_dir_header, MZ_ZIP_CENTRAL_DIR_HEADER_SIZE)) ||
	  (!mz_zip_array_push_back(pZip, &pState->m_central_dir, pFilename, filename_size)) ||
	  (!mz_zip_array_push_back(pZip, &pState->m_central_dir, pExtra, extra_size)) ||
	  (!mz_zip_array_push_back(pZip, &pState->m_central_dir, pComment, comment_size)) ||
	  (!mz_zip_array_push_back(pZip, &pState->m_central_dir_offsets, &central_dir_ofs, 1)))
  {
	// Try to push the central directory array back into its original state.
	mz_zip_array_resize(pZip, &pState->m_central_dir, orig_central_dir_size, MZ_FALSE);
	return MZ_FALSE;
  }

  return MZ_TRUE;
}

static mz_bool mz_zip_writer_validate_archive_name(const char *pArchive_name)
{
  // Basic ZIP archive filename validity checks: Valid filenames cannot start with a forward slash, cannot contain a drive letter, and cannot use DOS-style backward slashes.
  if (*pArchive_name == '/')
	return MZ_FALSE;
  while (*pArchive_name)
  {
	if ((*pArchive_name == '\\') || (*pArchive_name == ':'))
	  return MZ_FALSE;
	pArchive_name++;
  }
  return MZ_TRUE;
}

static mz_uint mz_zip_writer_compute_padding_needed_for_file_alignment(mz_zip_archive *pZip)
{
  mz_uint32 n;
  if (!pZip->m_file_offset_alignment)
	return 0;
  n = (mz_uint32)(pZip->m_archive_size & (pZip->m_file_offset_alignment - 1));
  return (pZip->m_file_offset_alignment - n) & (pZip->m_file_offset_alignment - 1);
}

static mz_bool mz_zip_writer_write_zeros(mz_zip_archive *pZip, mz_uint64 cur_file_ofs, mz_uint32 n)
{
  char buf[4096];
  memset(buf, 0, MZ_MIN(sizeof(buf), n));
  while (n)
  {
	mz_uint32 s = MZ_MIN(sizeof(buf), n);
	if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_file_ofs, buf, s) != s)
	  return MZ_FALSE;
	cur_file_ofs += s; n -= s;
  }
  return MZ_TRUE;
}

mz_bool mz_zip_writer_add_mem_ex(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags, mz_uint64 uncomp_size, mz_uint32 uncomp_crc32)
{
  mz_uint16 method = 0, dos_time = 0, dos_date = 0;
  mz_uint level, ext_attributes = 0, num_alignment_padding_bytes;
  mz_uint64 local_dir_header_ofs = pZip->m_archive_size, cur_archive_file_ofs = pZip->m_archive_size, comp_size = 0;
  size_t archive_name_size;
  mz_uint8 local_dir_header[MZ_ZIP_LOCAL_DIR_HEADER_SIZE];
  tdefl_compressor *pComp = NULL;
  mz_bool store_data_uncompressed;
  mz_zip_internal_state *pState;

  if ((int)level_and_flags < 0)
	level_and_flags = MZ_DEFAULT_LEVEL;
  level = level_and_flags & 0xF;
  store_data_uncompressed = ((!level) || (level_and_flags & MZ_ZIP_FLAG_COMPRESSED_DATA));

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != MZ_ZIP_MODE_WRITING) || ((buf_size) && (!pBuf)) || (!pArchive_name) || ((comment_size) && (!pComment)) || (pZip->m_total_files == 0xFFFF) || (level > MZ_UBER_COMPRESSION))
	return MZ_FALSE;

  pState = pZip->m_pState;

  if ((!(level_and_flags & MZ_ZIP_FLAG_COMPRESSED_DATA)) && (uncomp_size))
	return MZ_FALSE;
  // No zip64 support yet
  if ((buf_size > 0xFFFFFFFF) || (uncomp_size > 0xFFFFFFFF))
	return MZ_FALSE;
  if (!mz_zip_writer_validate_archive_name(pArchive_name))
	return MZ_FALSE;

#ifndef MINIZ_NO_TIME
  {
	time_t cur_time; time(&cur_time);
	mz_zip_time_to_dos_time(cur_time, &dos_time, &dos_date);
  }
#endif // #ifndef MINIZ_NO_TIME

  archive_name_size = strlen(pArchive_name);
  if (archive_name_size > 0xFFFF)
	return MZ_FALSE;

  num_alignment_padding_bytes = mz_zip_writer_compute_padding_needed_for_file_alignment(pZip);

  // no zip64 support yet
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + num_alignment_padding_bytes + MZ_ZIP_LOCAL_DIR_HEADER_SIZE + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + comment_size + archive_name_size) > 0xFFFFFFFF))
	return MZ_FALSE;

  if ((archive_name_size) && (pArchive_name[archive_name_size - 1] == '/'))
  {
	// Set DOS Subdirectory attribute bit.
	ext_attributes |= 0x10;
	// Subdirectories cannot contain data.
	if ((buf_size) || (uncomp_size))
	  return MZ_FALSE;
  }

  // Try to do any allocations before writing to the archive, so if an allocation fails the file remains unmodified. (A good idea if we're doing an in-place modification.)
  if ((!mz_zip_array_ensure_room(pZip, &pState->m_central_dir, MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + archive_name_size + comment_size)) || (!mz_zip_array_ensure_room(pZip, &pState->m_central_dir_offsets, 1)))
	return MZ_FALSE;

  if ((!store_data_uncompressed) && (buf_size))
  {
	if (NULL == (pComp = (tdefl_compressor *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(tdefl_compressor))))
	  return MZ_FALSE;
  }

  /* align file offset (@r-lyeh) { */
  {
	mz_uint offset = cur_archive_file_ofs + sizeof(local_dir_header) + archive_name_size + comment_size;
	num_alignment_padding_bytes = (pZip->m_file_offset_alignment - (offset & (pZip->m_file_offset_alignment - 1))) % pZip->m_file_offset_alignment;
  }
  /* } align file offset (@r-lyeh) */

  if (!mz_zip_writer_write_zeros(pZip, cur_archive_file_ofs, num_alignment_padding_bytes + sizeof(local_dir_header)))
  {
	pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
	return MZ_FALSE;
  }
  local_dir_header_ofs += num_alignment_padding_bytes;
  /* if (pZip->m_file_offset_alignment) { MZ_ASSERT((local_dir_header_ofs & (pZip->m_file_offset_alignment - 1)) == 0); } */
  cur_archive_file_ofs += num_alignment_padding_bytes + sizeof(local_dir_header);

  MZ_CLEAR_OBJ(local_dir_header);
  if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pArchive_name, archive_name_size) != archive_name_size)
  {
	pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
	return MZ_FALSE;
  }
  cur_archive_file_ofs += archive_name_size;

  if (!(level_and_flags & MZ_ZIP_FLAG_COMPRESSED_DATA))
  {
	uncomp_crc32 = (mz_uint32)mz_crc32(MZ_CRC32_INIT, (const mz_uint8*)pBuf, buf_size);
	uncomp_size = buf_size;
	if (uncomp_size <= 3)
	{
	  level = 0;
	  store_data_uncompressed = MZ_TRUE;
	}
  }

  if (store_data_uncompressed)
  {
	if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pBuf, buf_size) != buf_size)
	{
	  pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
	  return MZ_FALSE;
	}

	cur_archive_file_ofs += buf_size;
	comp_size = buf_size;

	if (level_and_flags & MZ_ZIP_FLAG_COMPRESSED_DATA)
	  method = MZ_DEFLATED;
  }
  else if (buf_size)
  {
	mz_zip_writer_add_state state;

	state.m_pZip = pZip;
	state.m_cur_archive_file_ofs = cur_archive_file_ofs;
	state.m_comp_size = 0;

	if ((tdefl_init(pComp, mz_zip_writer_add_put_buf_callback, &state, tdefl_create_comp_flags_from_zip_params(level, -15, MZ_DEFAULT_STRATEGY)) != TDEFL_STATUS_OKAY) ||
		(tdefl_compress_buffer(pComp, pBuf, buf_size, TDEFL_FINISH) != TDEFL_STATUS_DONE))
	{
	  pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
	  return MZ_FALSE;
	}

	comp_size = state.m_comp_size;
	cur_archive_file_ofs = state.m_cur_archive_file_ofs;

	method = MZ_DEFLATED;
  }

  pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
  pComp = NULL;

  // no zip64 support yet
  if ((comp_size > 0xFFFFFFFF) || (cur_archive_file_ofs > 0xFFFFFFFF))
	return MZ_FALSE;

  if (!mz_zip_writer_create_local_dir_header(pZip, local_dir_header, (mz_uint16)archive_name_size, 0, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date))
	return MZ_FALSE;

  if (pZip->m_pWrite(pZip->m_pIO_opaque, local_dir_header_ofs, local_dir_header, sizeof(local_dir_header)) != sizeof(local_dir_header))
	return MZ_FALSE;

  if (!mz_zip_writer_add_to_central_dir(pZip, pArchive_name, (mz_uint16)archive_name_size, NULL, 0, pComment, comment_size, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date, local_dir_header_ofs, ext_attributes))
	return MZ_FALSE;

  pZip->m_total_files++;
  pZip->m_archive_size = cur_archive_file_ofs;

  return MZ_TRUE;
}

#ifndef MINIZ_NO_STDIO
mz_bool mz_zip_writer_add_file(mz_zip_archive *pZip, const char *pArchive_name, const char *pSrc_filename, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags)
{
  mz_uint uncomp_crc32 = MZ_CRC32_INIT, level, num_alignment_padding_bytes;
  mz_uint16 method = 0, dos_time = 0, dos_date = 0, ext_attributes = 0;
  mz_uint64 local_dir_header_ofs = pZip->m_archive_size, cur_archive_file_ofs = pZip->m_archive_size, uncomp_size = 0, comp_size = 0;
  size_t archive_name_size;
  mz_uint8 local_dir_header[MZ_ZIP_LOCAL_DIR_HEADER_SIZE];
  MZ_FILE *pSrc_file = NULL;

  if ((int)level_and_flags < 0)
	level_and_flags = MZ_DEFAULT_LEVEL;
  level = level_and_flags & 0xF;

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != MZ_ZIP_MODE_WRITING) || (!pArchive_name) || ((comment_size) && (!pComment)) || (level > MZ_UBER_COMPRESSION))
	return MZ_FALSE;
  if (level_and_flags & MZ_ZIP_FLAG_COMPRESSED_DATA)
	return MZ_FALSE;
  if (!mz_zip_writer_validate_archive_name(pArchive_name))
	return MZ_FALSE;

  archive_name_size = strlen(pArchive_name);
  if (archive_name_size > 0xFFFF)
	return MZ_FALSE;

  num_alignment_padding_bytes = mz_zip_writer_compute_padding_needed_for_file_alignment(pZip);

  // no zip64 support yet
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + num_alignment_padding_bytes + MZ_ZIP_LOCAL_DIR_HEADER_SIZE + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + comment_size + archive_name_size) > 0xFFFFFFFF))
	return MZ_FALSE;

  if (!mz_zip_get_file_modified_time(pSrc_filename, &dos_time, &dos_date))
	return MZ_FALSE;

  pSrc_file = MZ_FOPEN(pSrc_filename, "rb");
  if (!pSrc_file)
	return MZ_FALSE;
  MZ_FSEEK64(pSrc_file, 0, SEEK_END);
  uncomp_size = MZ_FTELL64(pSrc_file);
  MZ_FSEEK64(pSrc_file, 0, SEEK_SET);

  if (uncomp_size > 0xFFFFFFFF)
  {
	// No zip64 support yet
	MZ_FCLOSE(pSrc_file);
	return MZ_FALSE;
  }
  if (uncomp_size <= 3)
	level = 0;

  if (!mz_zip_writer_write_zeros(pZip, cur_archive_file_ofs, num_alignment_padding_bytes + sizeof(local_dir_header)))
  {
	MZ_FCLOSE(pSrc_file);
	return MZ_FALSE;
  }
  local_dir_header_ofs += num_alignment_padding_bytes;
  if (pZip->m_file_offset_alignment) { MZ_ASSERT((local_dir_header_ofs & (pZip->m_file_offset_alignment - 1)) == 0); }
  cur_archive_file_ofs += num_alignment_padding_bytes + sizeof(local_dir_header);

  MZ_CLEAR_OBJ(local_dir_header);
  if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pArchive_name, archive_name_size) != archive_name_size)
  {
	MZ_FCLOSE(pSrc_file);
	return MZ_FALSE;
  }
  cur_archive_file_ofs += archive_name_size;

  if (uncomp_size)
  {
	mz_uint64 uncomp_remaining = uncomp_size;
	void *pRead_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, MZ_ZIP_MAX_IO_BUF_SIZE);
	if (!pRead_buf)
	{
	  MZ_FCLOSE(pSrc_file);
	  return MZ_FALSE;
	}

	if (!level)
	{
	  while (uncomp_remaining)
	  {
		mz_uint n = (mz_uint)MZ_MIN(MZ_ZIP_MAX_IO_BUF_SIZE, uncomp_remaining);
		if ((MZ_FREAD(pRead_buf, 1, n, pSrc_file) != n) || (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pRead_buf, n) != n))
		{
		  pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
		  MZ_FCLOSE(pSrc_file);
		  return MZ_FALSE;
		}
		uncomp_crc32 = (mz_uint32)mz_crc32(uncomp_crc32, (const mz_uint8 *)pRead_buf, n);
		uncomp_remaining -= n;
		cur_archive_file_ofs += n;
	  }
	  comp_size = uncomp_size;
	}
	else
	{
	  mz_bool result = MZ_FALSE;
	  mz_zip_writer_add_state state;
	  tdefl_compressor *pComp = (tdefl_compressor *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(tdefl_compressor));
	  if (!pComp)
	  {
		pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
		MZ_FCLOSE(pSrc_file);
		return MZ_FALSE;
	  }

	  state.m_pZip = pZip;
	  state.m_cur_archive_file_ofs = cur_archive_file_ofs;
	  state.m_comp_size = 0;

	  if (tdefl_init(pComp, mz_zip_writer_add_put_buf_callback, &state, tdefl_create_comp_flags_from_zip_params(level, -15, MZ_DEFAULT_STRATEGY)) != TDEFL_STATUS_OKAY)
	  {
		pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
		pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
		MZ_FCLOSE(pSrc_file);
		return MZ_FALSE;
	  }

	  for ( ; ; )
	  {
		size_t in_buf_size = (mz_uint32)MZ_MIN(uncomp_remaining, MZ_ZIP_MAX_IO_BUF_SIZE);
		tdefl_status status;

		if (MZ_FREAD(pRead_buf, 1, in_buf_size, pSrc_file) != in_buf_size)
		  break;

		uncomp_crc32 = (mz_uint32)mz_crc32(uncomp_crc32, (const mz_uint8 *)pRead_buf, in_buf_size);
		uncomp_remaining -= in_buf_size;

		status = tdefl_compress_buffer(pComp, pRead_buf, in_buf_size, uncomp_remaining ? TDEFL_NO_FLUSH : TDEFL_FINISH);
		if (status == TDEFL_STATUS_DONE)
		{
		  result = MZ_TRUE;
		  break;
		}
		else if (status != TDEFL_STATUS_OKAY)
		  break;
	  }

	  pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);

	  if (!result)
	  {
		pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
		MZ_FCLOSE(pSrc_file);
		return MZ_FALSE;
	  }

	  comp_size = state.m_comp_size;
	  cur_archive_file_ofs = state.m_cur_archive_file_ofs;

	  method = MZ_DEFLATED;
	}

	pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
  }

  MZ_FCLOSE(pSrc_file); pSrc_file = NULL;

  // no zip64 support yet
  if ((comp_size > 0xFFFFFFFF) || (cur_archive_file_ofs > 0xFFFFFFFF))
	return MZ_FALSE;

  if (!mz_zip_writer_create_local_dir_header(pZip, local_dir_header, (mz_uint16)archive_name_size, 0, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date))
	return MZ_FALSE;

  if (pZip->m_pWrite(pZip->m_pIO_opaque, local_dir_header_ofs, local_dir_header, sizeof(local_dir_header)) != sizeof(local_dir_header))
	return MZ_FALSE;

  if (!mz_zip_writer_add_to_central_dir(pZip, pArchive_name, (mz_uint16)archive_name_size, NULL, 0, pComment, comment_size, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date, local_dir_header_ofs, ext_attributes))
	return MZ_FALSE;

  pZip->m_total_files++;
  pZip->m_archive_size = cur_archive_file_ofs;

  return MZ_TRUE;
}
#endif // #ifndef MINIZ_NO_STDIO

mz_bool mz_zip_writer_add_from_zip_reader(mz_zip_archive *pZip, mz_zip_archive *pSource_zip, mz_uint file_index)
{
  mz_uint n, bit_flags, num_alignment_padding_bytes;
  mz_uint64 comp_bytes_remaining, local_dir_header_ofs;
  mz_uint64 cur_src_file_ofs, cur_dst_file_ofs;
  mz_uint32 local_header_u32[(MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(mz_uint32) - 1) / sizeof(mz_uint32)]; mz_uint8 *pLocal_header = (mz_uint8 *)local_header_u32;
  mz_uint8 central_header[MZ_ZIP_CENTRAL_DIR_HEADER_SIZE];
  size_t orig_central_dir_size;
  mz_zip_internal_state *pState;
  void *pBuf; const mz_uint8 *pSrc_central_header;

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != MZ_ZIP_MODE_WRITING))
	return MZ_FALSE;
  if (NULL == (pSrc_central_header = mz_zip_reader_get_cdh(pSource_zip, file_index)))
	return MZ_FALSE;
  pState = pZip->m_pState;

  num_alignment_padding_bytes = mz_zip_writer_compute_padding_needed_for_file_alignment(pZip);

  // no zip64 support yet
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + num_alignment_padding_bytes + MZ_ZIP_LOCAL_DIR_HEADER_SIZE + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
	return MZ_FALSE;

  cur_src_file_ofs = MZ_READ_LE32(pSrc_central_header + MZ_ZIP_CDH_LOCAL_HEADER_OFS);
  cur_dst_file_ofs = pZip->m_archive_size;

  if (pSource_zip->m_pRead(pSource_zip->m_pIO_opaque, cur_src_file_ofs, pLocal_header, MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
	return MZ_FALSE;
  if (MZ_READ_LE32(pLocal_header) != MZ_ZIP_LOCAL_DIR_HEADER_SIG)
	return MZ_FALSE;
  cur_src_file_ofs += MZ_ZIP_LOCAL_DIR_HEADER_SIZE;

  if (!mz_zip_writer_write_zeros(pZip, cur_dst_file_ofs, num_alignment_padding_bytes))
	return MZ_FALSE;
  cur_dst_file_ofs += num_alignment_padding_bytes;
  local_dir_header_ofs = cur_dst_file_ofs;
  if (pZip->m_file_offset_alignment) { MZ_ASSERT((local_dir_header_ofs & (pZip->m_file_offset_alignment - 1)) == 0); }

  if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_dst_file_ofs, pLocal_header, MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
	return MZ_FALSE;
  cur_dst_file_ofs += MZ_ZIP_LOCAL_DIR_HEADER_SIZE;

  n = MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_FILENAME_LEN_OFS) + MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_EXTRA_LEN_OFS);
  comp_bytes_remaining = n + MZ_READ_LE32(pSrc_central_header + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);

  if (NULL == (pBuf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)MZ_MAX(sizeof(mz_uint32) * 4, MZ_MIN(MZ_ZIP_MAX_IO_BUF_SIZE, comp_bytes_remaining)))))
	return MZ_FALSE;

  while (comp_bytes_remaining)
  {
	n = (mz_uint)MZ_MIN(MZ_ZIP_MAX_IO_BUF_SIZE, comp_bytes_remaining);
	if (pSource_zip->m_pRead(pSource_zip->m_pIO_opaque, cur_src_file_ofs, pBuf, n) != n)
	{
	  pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
	  return MZ_FALSE;
	}
	cur_src_file_ofs += n;

	if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_dst_file_ofs, pBuf, n) != n)
	{
	  pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
	  return MZ_FALSE;
	}
	cur_dst_file_ofs += n;

	comp_bytes_remaining -= n;
  }

  bit_flags = MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_BIT_FLAG_OFS);
  if (bit_flags & 8)
  {
	// Copy data descriptor
	if (pSource_zip->m_pRead(pSource_zip->m_pIO_opaque, cur_src_file_ofs, pBuf, sizeof(mz_uint32) * 4) != sizeof(mz_uint32) * 4)
	{
	  pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
	  return MZ_FALSE;
	}

	n = sizeof(mz_uint32) * ((MZ_READ_LE32(pBuf) == _0x08074b50) ? 4 : 3);
	if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_dst_file_ofs, pBuf, n) != n)
	{
	  pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
	  return MZ_FALSE;
	}

	cur_src_file_ofs += n;
	cur_dst_file_ofs += n;
  }
  pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);

  // no zip64 support yet
  if (cur_dst_file_ofs > 0xFFFFFFFF)
	return MZ_FALSE;

  orig_central_dir_size = pState->m_central_dir.m_size;

  memcpy(central_header, pSrc_central_header, MZ_ZIP_CENTRAL_DIR_HEADER_SIZE);
  MZ_WRITE_LE32(central_header + MZ_ZIP_CDH_LOCAL_HEADER_OFS, local_dir_header_ofs);
  if (!mz_zip_array_push_back(pZip, &pState->m_central_dir, central_header, MZ_ZIP_CENTRAL_DIR_HEADER_SIZE))
	return MZ_FALSE;

  n = MZ_READ_LE16(pSrc_central_header + MZ_ZIP_CDH_FILENAME_LEN_OFS) + MZ_READ_LE16(pSrc_central_header + MZ_ZIP_CDH_EXTRA_LEN_OFS) + MZ_READ_LE16(pSrc_central_header + MZ_ZIP_CDH_COMMENT_LEN_OFS);
  if (!mz_zip_array_push_back(pZip, &pState->m_central_dir, pSrc_central_header + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE, n))
  {
	mz_zip_array_resize(pZip, &pState->m_central_dir, orig_central_dir_size, MZ_FALSE);
	return MZ_FALSE;
  }

  if (pState->m_central_dir.m_size > 0xFFFFFFFF)
	return MZ_FALSE;
  n = (mz_uint32)orig_central_dir_size;
  if (!mz_zip_array_push_back(pZip, &pState->m_central_dir_offsets, &n, 1))
  {
	mz_zip_array_resize(pZip, &pState->m_central_dir, orig_central_dir_size, MZ_FALSE);
	return MZ_FALSE;
  }

  pZip->m_total_files++;
  pZip->m_archive_size = cur_dst_file_ofs;

  return MZ_TRUE;
}

mz_bool mz_zip_writer_finalize_archive(mz_zip_archive *pZip)
{
  mz_zip_internal_state *pState;
  mz_uint64 central_dir_ofs, central_dir_size;
  mz_uint8 hdr[MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE];

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != MZ_ZIP_MODE_WRITING))
	return MZ_FALSE;

  pState = pZip->m_pState;

  // no zip64 support yet
  if ((pZip->m_total_files > 0xFFFF) || ((pZip->m_archive_size + pState->m_central_dir.m_size + MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
	return MZ_FALSE;

  central_dir_ofs = 0;
  central_dir_size = 0;
  if (pZip->m_total_files)
  {
	// Write central directory
	central_dir_ofs = pZip->m_archive_size;
	central_dir_size = pState->m_central_dir.m_size;
	pZip->m_central_directory_file_ofs = central_dir_ofs;
	if (pZip->m_pWrite(pZip->m_pIO_opaque, central_dir_ofs, pState->m_central_dir.m_p, (size_t)central_dir_size) != central_dir_size)
	  return MZ_FALSE;
	pZip->m_archive_size += central_dir_size;
  }

  // Write end of central directory record
  MZ_CLEAR_OBJ(hdr);
  MZ_WRITE_LE32(hdr + MZ_ZIP_ECDH_SIG_OFS, MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG);
  MZ_WRITE_LE16(hdr + MZ_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS, pZip->m_total_files);
  MZ_WRITE_LE16(hdr + MZ_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS, pZip->m_total_files);
  MZ_WRITE_LE32(hdr + MZ_ZIP_ECDH_CDIR_SIZE_OFS, central_dir_size);
  MZ_WRITE_LE32(hdr + MZ_ZIP_ECDH_CDIR_OFS_OFS, central_dir_ofs);

  if (pZip->m_pWrite(pZip->m_pIO_opaque, pZip->m_archive_size, hdr, sizeof(hdr)) != sizeof(hdr))
	return MZ_FALSE;
#ifndef MINIZ_NO_STDIO
  if ((pState->m_pFile) && (MZ_FFLUSH(pState->m_pFile) == EOF))
	return MZ_FALSE;
#endif // #ifndef MINIZ_NO_STDIO

  pZip->m_archive_size += sizeof(hdr);

  pZip->m_zip_mode = MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED;
  return MZ_TRUE;
}

mz_bool mz_zip_writer_finalize_heap_archive(mz_zip_archive *pZip, void **pBuf, size_t *pSize)
{
  if ((!pZip) || (!pZip->m_pState) || (!pBuf) || (!pSize))
	return MZ_FALSE;
  if (pZip->m_pWrite != mz_zip_heap_write_func)
	return MZ_FALSE;
  if (!mz_zip_writer_finalize_archive(pZip))
	return MZ_FALSE;

  *pBuf = pZip->m_pState->m_pMem;
  *pSize = pZip->m_pState->m_mem_size;
  pZip->m_pState->m_pMem = NULL;
  pZip->m_pState->m_mem_size = pZip->m_pState->m_mem_capacity = 0;
  return MZ_TRUE;
}

mz_bool mz_zip_writer_end(mz_zip_archive *pZip)
{
  mz_zip_internal_state *pState;
  mz_bool status = MZ_TRUE;
  if ((!pZip) || (!pZip->m_pState) || (!pZip->m_pAlloc) || (!pZip->m_pFree) || ((pZip->m_zip_mode != MZ_ZIP_MODE_WRITING) && (pZip->m_zip_mode != MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED)))
	return MZ_FALSE;

  pState = pZip->m_pState;
  pZip->m_pState = NULL;
  mz_zip_array_clear(pZip, &pState->m_central_dir);
  mz_zip_array_clear(pZip, &pState->m_central_dir_offsets);
  mz_zip_array_clear(pZip, &pState->m_sorted_central_dir_offsets);

#ifndef MINIZ_NO_STDIO
  if (pState->m_pFile)
  {
	MZ_FCLOSE(pState->m_pFile);
	pState->m_pFile = NULL;
  }
#endif // #ifndef MINIZ_NO_STDIO

  if ((pZip->m_pWrite == mz_zip_heap_write_func) && (pState->m_pMem))
  {
	pZip->m_pFree(pZip->m_pAlloc_opaque, pState->m_pMem);
	pState->m_pMem = NULL;
  }

  pZip->m_pFree(pZip->m_pAlloc_opaque, pState);
  pZip->m_zip_mode = MZ_ZIP_MODE_INVALID;
  return status;
}

#ifndef MINIZ_NO_STDIO
mz_bool mz_zip_add_mem_to_archive_file_in_place(const char *pZip_filename, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, mz_uint16 comment_size, mz_uint level_and_flags)
{
  mz_bool status, created_new_archive = MZ_FALSE;
  mz_zip_archive zip_archive;
  struct MZ_FILE_STAT_STRUCT file_stat;
  MZ_CLEAR_OBJ(zip_archive);
  if ((int)level_and_flags < 0)
	 level_and_flags = MZ_DEFAULT_LEVEL;
  if ((!pZip_filename) || (!pArchive_name) || ((buf_size) && (!pBuf)) || ((comment_size) && (!pComment)) || ((level_and_flags & 0xF) > MZ_UBER_COMPRESSION))
	return MZ_FALSE;
  if (!mz_zip_writer_validate_archive_name(pArchive_name))
	return MZ_FALSE;
  if (MZ_FILE_STAT(pZip_filename, &file_stat) != 0)
  {
	// Create a new archive.
	if (!mz_zip_writer_init_file(&zip_archive, pZip_filename, 0))
	  return MZ_FALSE;
	created_new_archive = MZ_TRUE;
  }
  else
  {
	// Append to an existing archive.
	if (!mz_zip_reader_init_file(&zip_archive, pZip_filename, level_and_flags | MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
	  return MZ_FALSE;
	if (!mz_zip_writer_init_from_reader(&zip_archive, pZip_filename))
	{
	  mz_zip_reader_end(&zip_archive);
	  return MZ_FALSE;
	}
  }
  status = mz_zip_writer_add_mem_ex(&zip_archive, pArchive_name, pBuf, buf_size, pComment, comment_size, level_and_flags, 0, 0);
  // Always finalize, even if adding failed for some reason, so we have a valid central directory. (This may not always succeed, but we can try.)
  if (!mz_zip_writer_finalize_archive(&zip_archive))
	status = MZ_FALSE;
  if (!mz_zip_writer_end(&zip_archive))
	status = MZ_FALSE;
  if ((!status) && (created_new_archive))
  {
	// It's a new archive and something went wrong, so just delete it.
	int ignoredStatus = MZ_DELETE_FILE(pZip_filename);
	(void)ignoredStatus;
  }
  return status;
}

void *mz_zip_extract_archive_file_to_heap(const char *pZip_filename, const char *pArchive_name, size_t *pSize, mz_uint flags)
{
  int file_index;
  mz_zip_archive zip_archive;
  void *p = NULL;

  if (pSize)
	*pSize = 0;

  if ((!pZip_filename) || (!pArchive_name))
	return NULL;

  MZ_CLEAR_OBJ(zip_archive);
  if (!mz_zip_reader_init_file(&zip_archive, pZip_filename, flags | MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
	return NULL;

  if ((file_index = mz_zip_reader_locate_file(&zip_archive, pArchive_name, NULL, flags)) >= 0)
	p = mz_zip_reader_extract_to_heap(&zip_archive, file_index, pSize, flags);

  mz_zip_reader_end(&zip_archive);
  return p;
}

#endif // #ifndef MINIZ_NO_STDIO

#endif // #ifndef MINIZ_NO_ARCHIVE_WRITING_APIS

#endif // #ifndef MINIZ_NO_ARCHIVE_APIS

#ifdef __cplusplus
}
#endif

#endif // MINIZ_HEADER_FILE_ONLY

/*
  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org/>
*/


#endif

#if 1
// lz4, which defines 'inline' and 'restrict' which is later required by shoco

#line 3 "lz4.c"
/**************************************
   Tuning parameters
**************************************/
/*
 * HEAPMODE :
 * Select how default compression functions will allocate memory for their hash table,
 * in memory stack (0:default, fastest), or in memory heap (1:requires memory allocation (malloc)).
 */
#define HEAPMODE 0

/**************************************
   CPU Feature Detection
**************************************/
/* 32 or 64 bits ? */
#if (defined(__x86_64__) || defined(_M_X64) || defined(_WIN64) \
  || defined(__powerpc64__) || defined(__powerpc64le__) \
  || defined(__ppc64__) || defined(__ppc64le__) \
  || defined(__PPC64__) || defined(__PPC64LE__) \
  || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) )   /* Detects 64 bits mode */
#  define LZ4_ARCH64 1
#else
#  define LZ4_ARCH64 0
#endif

/*
 * Little Endian or Big Endian ?
 * Overwrite the #define below if you know your architecture endianess
 */
#include <stdlib.h>   /* Apparently required to detect endianess */
#if defined (__GLIBC__)
#  include <endian.h>
#  if (__BYTE_ORDER == __BIG_ENDIAN)
#     define LZ4_BIG_ENDIAN 1
#  endif
#elif (defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined(_BIG_ENDIAN)) && !(defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN))
#  define LZ4_BIG_ENDIAN 1
#elif defined(__sparc) || defined(__sparc__) \
   || defined(__powerpc__) || defined(__ppc__) || defined(__PPC__) \
   || defined(__hpux)  || defined(__hppa) \
   || defined(_MIPSEB) || defined(__s390__)
#  define LZ4_BIG_ENDIAN 1
#else
/* Little Endian assumed. PDP Endian and other very rare endian format are unsupported. */
#endif

/*
 * Unaligned memory access is automatically enabled for "common" CPU, such as x86.
 * For others CPU, such as ARM, the compiler may be more cautious, inserting unnecessary extra code to ensure aligned access property
 * If you know your target CPU supports unaligned memory access, you want to force this option manually to improve performance
 */
#if defined(__ARM_FEATURE_UNALIGNED)
#  define LZ4_FORCE_UNALIGNED_ACCESS 1
#endif

/* Define this parameter if your target system or compiler does not support hardware bit count */
#if defined(_MSC_VER) && defined(_WIN32_WCE)   /* Visual Studio for Windows CE does not support Hardware bit count */
#  define LZ4_FORCE_SW_BITCOUNT
#endif

/*
 * BIG_ENDIAN_NATIVE_BUT_INCOMPATIBLE :
 * This option may provide a small boost to performance for some big endian cpu, although probably modest.
 * You may set this option to 1 if data will remain within closed environment.
 * This option is useless on Little_Endian CPU (such as x86)
 */

/* #define BIG_ENDIAN_NATIVE_BUT_INCOMPATIBLE 1 */

/**************************************
 Compiler Options
**************************************/
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
/* "restrict" is a known keyword */
#else
#  define restrict /* Disable restrict */
#endif

#ifdef _MSC_VER    /* Visual Studio */
#  define FORCE_INLINE static __forceinline
#  include <intrin.h>                    /* For Visual 2005 */
#  if LZ4_ARCH64   /* 64-bits */
#    pragma intrinsic(_BitScanForward64) /* For Visual 2005 */
#    pragma intrinsic(_BitScanReverse64) /* For Visual 2005 */
#  else            /* 32-bits */
#    pragma intrinsic(_BitScanForward)   /* For Visual 2005 */
#    pragma intrinsic(_BitScanReverse)   /* For Visual 2005 */
#  endif
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#else
#  ifdef __GNUC__
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif
#endif

#ifdef _MSC_VER  /* Visual Studio */
#  define lz4_bswap16(x) _byteswap_ushort(x)
#else
#  define lz4_bswap16(x) ((unsigned short int) ((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8)))
#endif

#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

#if (GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
#  define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
#  define expect(expr,value)    (expr)
#endif

#define likely(expr)     expect((expr) != 0, 1)
#define unlikely(expr)   expect((expr) != 0, 0)

/**************************************
   Memory routines
**************************************/
#include <stdlib.h>   /* malloc, calloc, free */
#define ALLOCATOR(n,s) calloc(n,s)
#define FREEMEM        free
#include <string.h>   /* memset, memcpy */
#define MEM_INIT       memset

/**************************************
   Includes
**************************************/

#line 3 "lz4.h"

/**************************************
   Basic Types
**************************************/
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
# include <stdint.h>
  typedef  uint8_t BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      U16;
  typedef unsigned int        U32;
  typedef   signed int        S32;
  typedef unsigned long long  U64;
#endif

#if defined(__GNUC__)  && !defined(LZ4_FORCE_UNALIGNED_ACCESS)
#  define _PACKED __attribute__ ((packed))
#else
#  define _PACKED
#endif

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  if defined(__IBMC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    pragma pack(1)
#  else
#    pragma pack(push, 1)
#  endif
#endif

typedef struct { U16 v; }  _PACKED U16_S;
typedef struct { U32 v; }  _PACKED U32_S;
typedef struct { U64 v; }  _PACKED U64_S;
typedef struct {size_t v;} _PACKED size_t_S;

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    pragma pack(0)
#  else
#    pragma pack(pop)
#  endif
#endif

#define A16(x)   (((U16_S *)(x))->v)
#define A32(x)   (((U32_S *)(x))->v)
#define A64(x)   (((U64_S *)(x))->v)
#define AARCH(x) (((size_t_S *)(x))->v)

/**************************************
   Constants
**************************************/
#define LZ4_HASHLOG   (LZ4_MEMORY_USAGE-2)
#define HASHTABLESIZE (1 << LZ4_MEMORY_USAGE)
#define HASH_SIZE_U32 (1 << LZ4_HASHLOG)

#define MINMATCH 4

#define COPYLENGTH 8
#define LASTLITERALS 5
#define MFLIMIT (COPYLENGTH+MINMATCH)
static const int LZ4_minLength = (MFLIMIT+1);

#define KB *(1U<<10)
#define MB *(1U<<20)
#define GB *(1U<<30)

#define LZ4_64KLIMIT ((64 KB) + (MFLIMIT-1))
#define SKIPSTRENGTH 6   /* Increasing this value will make the compression run slower on incompressible data */

#define MAXD_LOG 16
#define MAX_DISTANCE ((1 << MAXD_LOG) - 1)

#define ML_BITS  4
#define ML_MASK  ((1U<<ML_BITS)-1)
#define RUN_BITS (8-ML_BITS)
#define RUN_MASK ((1U<<RUN_BITS)-1)

/**************************************
   Structures and local types
**************************************/
typedef struct {
	U32  hashTable[HASH_SIZE_U32];
	U32  currentOffset;
	U32  initCheck;
	const BYTE* dictionary;
	const BYTE* bufferStart;
	U32  dictSize;
} LZ4_stream_t_internal;

typedef enum { notLimited = 0, limitedOutput = 1 } limitedOutput_directive;
typedef enum { byPtr, byU32, byU16 } tableType_t;

typedef enum { noDict = 0, withPrefix64k, usingExtDict } dict_directive;
typedef enum { noDictIssue = 0, dictSmall } dictIssue_directive;

typedef enum { endOnOutputSize = 0, endOnInputSize = 1 } endCondition_directive;
typedef enum { full = 0, partial = 1 } earlyEnd_directive;

/**************************************
   Architecture-specific macros
**************************************/
#define STEPSIZE                  sizeof(size_t)
#define LZ4_COPYSTEP(d,s)         { AARCH(d) = AARCH(s); d+=STEPSIZE; s+=STEPSIZE; }
#define LZ4_COPY8(d,s)            { LZ4_COPYSTEP(d,s); if (STEPSIZE<8) LZ4_COPYSTEP(d,s); }

#if (defined(LZ4_BIG_ENDIAN) && !defined(BIG_ENDIAN_NATIVE_BUT_INCOMPATIBLE))
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { U16 v = A16(p); v = lz4_bswap16(v); d = (s) - v; }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,i)  { U16 v = (U16)(i); v = lz4_bswap16(v); A16(p) = v; p+=2; }
#else      /* Little Endian */
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { d = (s) - A16(p); }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,v)  { A16(p) = v; p+=2; }
#endif

/**************************************
   Macros
**************************************/
#define LZ4_STATIC_ASSERT(c)    { enum { LZ4_static_assert = 1/(!!(c)) }; }   /* use only *after* variable declarations */
#if LZ4_ARCH64 || !defined(__GNUC__)
#  define LZ4_WILDCOPY(d,s,e)   { do { LZ4_COPY8(d,s) } while (d<e); }        /* at the end, d>=e; */
#else
#  define LZ4_WILDCOPY(d,s,e)   { if (likely(e-d <= 8)) LZ4_COPY8(d,s) else do { LZ4_COPY8(d,s) } while (d<e); }
#endif

/****************************
   Private local functions
****************************/
#if LZ4_ARCH64

int LZ4_NbCommonBytes (register U64 val)
{
# if defined(LZ4_BIG_ENDIAN)
#   if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r = 0;
	_BitScanReverse64( &r, val );
	return (int)(r>>3);
#   elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_clzll(val) >> 3);
#   else
	int r;
	if (!(val>>32)) { r=4; } else { r=0; val>>=32; }
	if (!(val>>16)) { r+=2; val>>=8; } else { val>>=24; }
	r += (!val);
	return r;
#   endif
# else
#   if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r = 0;
	_BitScanForward64( &r, val );
	return (int)(r>>3);
#   elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_ctzll(val) >> 3);
#   else
	static const int DeBruijnBytePos[64] = { 0, 0, 0, 0, 0, 1, 1, 2, 0, 3, 1, 3, 1, 4, 2, 7, 0, 2, 3, 6, 1, 5, 3, 5, 1, 3, 4, 4, 2, 5, 6, 7, 7, 0, 1, 2, 3, 3, 4, 6, 2, 6, 5, 5, 3, 4, 5, 6, 7, 1, 2, 4, 6, 4, 4, 5, 7, 2, 6, 5, 7, 6, 7, 7 };
	return DeBruijnBytePos[((U64)((val & -(long long)val) * 0x0218A392CDABBD3FULL)) >> 58];
#   endif
# endif
}

#else

int LZ4_NbCommonBytes (register U32 val)
{
# if defined(LZ4_BIG_ENDIAN)
#   if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r = 0;
	_BitScanReverse( &r, val );
	return (int)(r>>3);
#   elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_clz(val) >> 3);
#   else
	int r;
	if (!(val>>16)) { r=2; val>>=8; } else { r=0; val>>=24; }
	r += (!val);
	return r;
#   endif
# else
#   if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r;
	_BitScanForward( &r, val );
	return (int)(r>>3);
#   elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_ctz(val) >> 3);
#   else
	static const int DeBruijnBytePos[32] = { 0, 0, 3, 0, 3, 1, 3, 0, 3, 2, 2, 1, 3, 2, 0, 1, 3, 3, 1, 2, 2, 2, 2, 0, 3, 1, 2, 0, 1, 0, 1, 1 };
	return DeBruijnBytePos[((U32)((val & -(S32)val) * 0x077CB531U)) >> 27];
#   endif
# endif
}

#endif

/********************************
   Compression functions
********************************/
int LZ4_compressBound(int isize)  { return LZ4_COMPRESSBOUND(isize); }

static int LZ4_hashSequence(U32 sequence, tableType_t tableType)
{
	if (tableType == byU16)
		return (((sequence) * 2654435761U) >> ((MINMATCH*8)-(LZ4_HASHLOG+1)));
	else
		return (((sequence) * 2654435761U) >> ((MINMATCH*8)-LZ4_HASHLOG));
}

static int LZ4_hashPosition(const BYTE* p, tableType_t tableType) { return LZ4_hashSequence(A32(p), tableType); }

static void LZ4_putPositionOnHash(const BYTE* p, U32 h, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	switch (tableType)
	{
	case byPtr: { const BYTE** hashTable = (const BYTE**) tableBase; hashTable[h] = p; break; }
	case byU32: { U32* hashTable = (U32*) tableBase; hashTable[h] = (U32)(p-srcBase); break; }
	case byU16: { U16* hashTable = (U16*) tableBase; hashTable[h] = (U16)(p-srcBase); break; }
	}
}

static void LZ4_putPosition(const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	U32 h = LZ4_hashPosition(p, tableType);
	LZ4_putPositionOnHash(p, h, tableBase, tableType, srcBase);
}

static const BYTE* LZ4_getPositionOnHash(U32 h, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	if (tableType == byPtr) { const BYTE** hashTable = (const BYTE**) tableBase; return hashTable[h]; }
	if (tableType == byU32) { U32* hashTable = (U32*) tableBase; return hashTable[h] + srcBase; }
	{ U16* hashTable = (U16*) tableBase; return hashTable[h] + srcBase; }   /* default, to ensure a return */
}

static const BYTE* LZ4_getPosition(const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	U32 h = LZ4_hashPosition(p, tableType);
	return LZ4_getPositionOnHash(h, tableBase, tableType, srcBase);
}

static unsigned LZ4_count(const BYTE* pIn, const BYTE* pRef, const BYTE* pInLimit)
{
	const BYTE* const pStart = pIn;

	while (likely(pIn<pInLimit-(STEPSIZE-1)))
	{
		size_t diff = AARCH(pRef) ^ AARCH(pIn);
		if (!diff) { pIn+=STEPSIZE; pRef+=STEPSIZE; continue; }
		pIn += LZ4_NbCommonBytes(diff);
		return (unsigned)(pIn - pStart);
	}
	if (sizeof(void*)==8) if ((pIn<(pInLimit-3)) && (A32(pRef) == A32(pIn))) { pIn+=4; pRef+=4; }
	if ((pIn<(pInLimit-1)) && (A16(pRef) == A16(pIn))) { pIn+=2; pRef+=2; }
	if ((pIn<pInLimit) && (*pRef == *pIn)) pIn++;

	return (unsigned)(pIn - pStart);
}

static int LZ4_compress_generic(
				 void* ctx,
				 const char* source,
				 char* dest,
				 int inputSize,
				 int maxOutputSize,

				 limitedOutput_directive outputLimited,
				 tableType_t tableType,
				 dict_directive dict,
				 dictIssue_directive dictIssue)
{
	LZ4_stream_t_internal* const dictPtr = (LZ4_stream_t_internal*)ctx;

	const BYTE* ip = (const BYTE*) source;
	const BYTE* base;
	const BYTE* lowLimit;
	const BYTE* const lowRefLimit = ip - dictPtr->dictSize;
	const BYTE* const dictionary = dictPtr->dictionary;
	const BYTE* const dictEnd = dictionary + dictPtr->dictSize;
	const size_t dictDelta = dictEnd - (const BYTE*)source;
	const BYTE* anchor = (const BYTE*) source;
	const BYTE* const iend = ip + inputSize;
	const BYTE* const mflimit = iend - MFLIMIT;
	const BYTE* const matchlimit = iend - LASTLITERALS;

	BYTE* op = (BYTE*) dest;
	BYTE* const olimit = op + maxOutputSize;

	const int skipStrength = SKIPSTRENGTH;
	U32 forwardH;
	size_t refDelta=0;

	/* Init conditions */
	if ((U32)inputSize > (U32)LZ4_MAX_INPUT_SIZE) return 0;          /* Unsupported input size, too large (or negative) */
	switch(dict)
	{
	case noDict:
	default:
		base = (const BYTE*)source;
		lowLimit = (const BYTE*)source;
		break;
	case withPrefix64k:
		base = (const BYTE*)source - dictPtr->currentOffset;
		lowLimit = (const BYTE*)source - dictPtr->dictSize;
		break;
	case usingExtDict:
		base = (const BYTE*)source - dictPtr->currentOffset;
		lowLimit = (const BYTE*)source;
		break;
	}
	if ((tableType == byU16) && (inputSize>=(int)LZ4_64KLIMIT)) return 0;   /* Size too large (not within 64K limit) */
	if (inputSize<LZ4_minLength) goto _last_literals;                       /* Input too small, no compression (all literals) */

	/* First Byte */
	LZ4_putPosition(ip, ctx, tableType, base);
	ip++; forwardH = LZ4_hashPosition(ip, tableType);

	/* Main Loop */
	for ( ; ; )
	{
		const BYTE* ref;
		BYTE* token;
		{
			const BYTE* forwardIp = ip;
			unsigned step=1;
			unsigned searchMatchNb = (1U << skipStrength);

			/* Find a match */
			do {
				U32 h = forwardH;
				ip = forwardIp;
				forwardIp += step;
				step = searchMatchNb++ >> skipStrength;
				//if (step>8) step=8;   // required for valid forwardIp ; slows down uncompressible data a bit

				if (unlikely(forwardIp > mflimit)) goto _last_literals;

				ref = LZ4_getPositionOnHash(h, ctx, tableType, base);
				if (dict==usingExtDict)
				{
					if (ref<(const BYTE*)source)
					{
						refDelta = dictDelta;
						lowLimit = dictionary;
					}
					else
					{
						refDelta = 0;
						lowLimit = (const BYTE*)source;
					}
				}
				forwardH = LZ4_hashPosition(forwardIp, tableType);
				LZ4_putPositionOnHash(ip, h, ctx, tableType, base);

			} while ( ((dictIssue==dictSmall) ? (ref < lowRefLimit) : 0)
				|| ((tableType==byU16) ? 0 : (ref + MAX_DISTANCE < ip))
				|| (A32(ref+refDelta) != A32(ip)) );
		}

		/* Catch up */
		while ((ip>anchor) && (ref+refDelta > lowLimit) && (unlikely(ip[-1]==ref[refDelta-1]))) { ip--; ref--; }

		{
			/* Encode Literal length */
			unsigned litLength = (unsigned)(ip - anchor);
			token = op++;
			if ((outputLimited) && (unlikely(op + litLength + (2 + 1 + LASTLITERALS) + (litLength/255) > olimit)))
				return 0;   /* Check output limit */
			if (litLength>=RUN_MASK)
			{
				int len = (int)litLength-RUN_MASK;
				*token=(RUN_MASK<<ML_BITS);
				for(; len >= 255 ; len-=255) *op++ = 255;
				*op++ = (BYTE)len;
			}
			else *token = (BYTE)(litLength<<ML_BITS);

			/* Copy Literals */
			{ BYTE* end = op+litLength; LZ4_WILDCOPY(op,anchor,end); op=end; }
		}

_next_match:
		/* Encode Offset */
		LZ4_WRITE_LITTLEENDIAN_16(op, (U16)(ip-ref));

		/* Encode MatchLength */
		{
			unsigned matchLength;

			if ((dict==usingExtDict) && (lowLimit==dictionary))
			{
				const BYTE* limit;
				ref += refDelta;
				limit = ip + (dictEnd-ref);
				if (limit > matchlimit) limit = matchlimit;
				matchLength = LZ4_count(ip+MINMATCH, ref+MINMATCH, limit);
				ip += MINMATCH + matchLength;
				if (ip==limit)
				{
					unsigned more = LZ4_count(ip, (const BYTE*)source, matchlimit);
					matchLength += more;
					ip += more;
				}
			}
			else
			{
				matchLength = LZ4_count(ip+MINMATCH, ref+MINMATCH, matchlimit);
				ip += MINMATCH + matchLength;
			}

			if (matchLength>=ML_MASK)
			{
				if ((outputLimited) && (unlikely(op + (1 + LASTLITERALS) + (matchLength>>8) > olimit)))
					return 0;    /* Check output limit */
				*token += ML_MASK;
				matchLength -= ML_MASK;
				for (; matchLength >= 510 ; matchLength-=510) { *op++ = 255; *op++ = 255; }
				if (matchLength >= 255) { matchLength-=255; *op++ = 255; }
				*op++ = (BYTE)matchLength;
			}
			else *token += (BYTE)(matchLength);
		}

		anchor = ip;

		/* Test end of chunk */
		if (ip > mflimit) break;

		/* Fill table */
		LZ4_putPosition(ip-2, ctx, tableType, base);

		/* Test next position */
		ref = LZ4_getPosition(ip, ctx, tableType, base);
		if (dict==usingExtDict)
		{
			if (ref<(const BYTE*)source)
			{
				refDelta = dictDelta;
				lowLimit = dictionary;
			}
			else
			{
				refDelta = 0;
				lowLimit = (const BYTE*)source;
			}
		}
		LZ4_putPosition(ip, ctx, tableType, base);
		if ( ((dictIssue==dictSmall) ? (ref>=lowRefLimit) : 1)
			&& (ref+MAX_DISTANCE>=ip)
			&& (A32(ref+refDelta)==A32(ip)) )
		{ token=op++; *token=0; goto _next_match; }

		/* Prepare next loop */
		forwardH = LZ4_hashPosition(++ip, tableType);
	}

_last_literals:
	/* Encode Last Literals */
	{
		int lastRun = (int)(iend - anchor);
		if ((outputLimited) && (((char*)op - dest) + lastRun + 1 + ((lastRun+255-RUN_MASK)/255) > (U32)maxOutputSize))
			return 0;   /* Check output limit */
		if (lastRun>=(int)RUN_MASK) { *op++=(RUN_MASK<<ML_BITS); lastRun-=RUN_MASK; for(; lastRun >= 255 ; lastRun-=255) *op++ = 255; *op++ = (BYTE) lastRun; }
		else *op++ = (BYTE)(lastRun<<ML_BITS);
		memcpy(op, anchor, iend - anchor);
		op += iend-anchor;
	}

	/* End */
	return (int) (((char*)op)-dest);
}

int LZ4_compress(const char* source, char* dest, int inputSize)
{
#if (HEAPMODE)
	void* ctx = ALLOCATOR(LZ4_STREAMSIZE_U32, 4);   /* Aligned on 4-bytes boundaries */
#else
	U32 ctx[LZ4_STREAMSIZE_U32] = {0};      /* Ensure data is aligned on 4-bytes boundaries */
#endif
	int result;

	if (inputSize < (int)LZ4_64KLIMIT)
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, 0, notLimited, byU16, noDict, noDictIssue);
	else
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, 0, notLimited, (sizeof(void*)==8) ? byU32 : byPtr, noDict, noDictIssue);

#if (HEAPMODE)
	FREEMEM(ctx);
#endif
	return result;
}

int LZ4_compress_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize)
{
#if (HEAPMODE)
	void* ctx = ALLOCATOR(LZ4_STREAMSIZE_U32, 4);   /* Aligned on 4-bytes boundaries */
#else
	U32 ctx[LZ4_STREAMSIZE_U32] = {0};      /* Ensure data is aligned on 4-bytes boundaries */
#endif
	int result;

	if (inputSize < (int)LZ4_64KLIMIT)
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, maxOutputSize, limitedOutput, byU16, noDict, noDictIssue);
	else
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, maxOutputSize, limitedOutput, (sizeof(void*)==8) ? byU32 : byPtr, noDict, noDictIssue);

#if (HEAPMODE)
	FREEMEM(ctx);
#endif
	return result;
}

/*****************************************
   Experimental : Streaming functions
*****************************************/

void* LZ4_createStream()
{
	void* lz4s = ALLOCATOR(4, LZ4_STREAMSIZE_U32);
	MEM_INIT(lz4s, 0, LZ4_STREAMSIZE);
	return lz4s;
}

int LZ4_free (void* LZ4_stream)
{
	FREEMEM(LZ4_stream);
	return (0);
}

int LZ4_loadDict (void* LZ4_dict, const char* dictionary, int dictSize)
{
	LZ4_stream_t_internal* dict = (LZ4_stream_t_internal*) LZ4_dict;
	const BYTE* p = (const BYTE*)dictionary;
	const BYTE* const dictEnd = p + dictSize;
	const BYTE* base;

	LZ4_STATIC_ASSERT(LZ4_STREAMSIZE >= sizeof(LZ4_stream_t_internal));      /* A compilation error here means LZ4_STREAMSIZE is not large enough */
	if (dict->initCheck) MEM_INIT(dict, 0, sizeof(LZ4_stream_t_internal));   /* Uninitialized structure detected */

	if (dictSize < MINMATCH)
	{
		dict->dictionary = NULL;
		dict->dictSize = 0;
		return 1;
	}

	if (p <= dictEnd - 64 KB) p = dictEnd - 64 KB;
	base = p - dict->currentOffset;
	dict->dictionary = p;
	dict->dictSize = (U32)(dictEnd - p);
	dict->currentOffset += dict->dictSize;

	while (p <= dictEnd-MINMATCH)
	{
		LZ4_putPosition(p, dict, byU32, base);
		p+=3;
	}

	return 1;
}

void LZ4_renormDictT(LZ4_stream_t_internal* LZ4_dict, const BYTE* src)
{
	if ((LZ4_dict->currentOffset > 0x80000000) ||
		((size_t)LZ4_dict->currentOffset > (size_t)src))   /* address space overflow */
	{
		/* rescale hash table */
		U32 delta = LZ4_dict->currentOffset - 64 KB;
		const BYTE* dictEnd = LZ4_dict->dictionary + LZ4_dict->dictSize;
		int i;
		for (i=0; i<HASH_SIZE_U32; i++)
		{
			if (LZ4_dict->hashTable[i] < delta) LZ4_dict->hashTable[i]=0;
			else LZ4_dict->hashTable[i] -= delta;
		}
		LZ4_dict->currentOffset = 64 KB;
		if (LZ4_dict->dictSize > 64 KB) LZ4_dict->dictSize = 64 KB;
		LZ4_dict->dictionary = dictEnd - LZ4_dict->dictSize;
	}
}

FORCE_INLINE int LZ4_compress_continue_generic (void* LZ4_stream, const char* source, char* dest, int inputSize,
												int maxOutputSize, limitedOutput_directive limit)
{
	LZ4_stream_t_internal* streamPtr = (LZ4_stream_t_internal*)LZ4_stream;
	const BYTE* const dictEnd = streamPtr->dictionary + streamPtr->dictSize;

	const BYTE* smallest = (const BYTE*) source;
	if (streamPtr->initCheck) return 0;   /* Uninitialized structure detected */
	if ((streamPtr->dictSize>0) && (smallest>dictEnd)) smallest = dictEnd;
	LZ4_renormDictT(streamPtr, smallest);

	/* Check overlapping input/dictionary space */
	{
		const BYTE* sourceEnd = (const BYTE*) source + inputSize;
		if ((sourceEnd > streamPtr->dictionary) && (sourceEnd < dictEnd))
		{
			streamPtr->dictSize = (U32)(dictEnd - sourceEnd);
			if (streamPtr->dictSize > 64 KB) streamPtr->dictSize = 64 KB;
			if (streamPtr->dictSize < 4) streamPtr->dictSize = 0;
			streamPtr->dictionary = dictEnd - streamPtr->dictSize;
		}
	}

	/* prefix mode : source data follows dictionary */
	if (dictEnd == (const BYTE*)source)
	{
		int result;
		if ((streamPtr->dictSize < 64 KB) && (streamPtr->dictSize < streamPtr->currentOffset))
			result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, withPrefix64k, dictSmall);
		else
			result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, withPrefix64k, noDictIssue);
		streamPtr->dictSize += (U32)inputSize;
		streamPtr->currentOffset += (U32)inputSize;
		return result;
	}

	/* external dictionary mode */
	{
		int result;
		if ((streamPtr->dictSize < 64 KB) && (streamPtr->dictSize < streamPtr->currentOffset))
			result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, usingExtDict, dictSmall);
		else
			result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, usingExtDict, noDictIssue);
		streamPtr->dictionary = (const BYTE*)source;
		streamPtr->dictSize = (U32)inputSize;
		streamPtr->currentOffset += (U32)inputSize;
		return result;
	}
}

int LZ4_compress_continue (void* LZ4_stream, const char* source, char* dest, int inputSize)
{
	return LZ4_compress_continue_generic(LZ4_stream, source, dest, inputSize, 0, notLimited);
}

int LZ4_compress_limitedOutput_continue (void* LZ4_stream, const char* source, char* dest, int inputSize, int maxOutputSize)
{
	return LZ4_compress_continue_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limitedOutput);
}

// Hidden debug function, to force separate dictionary mode
int LZ4_compress_forceExtDict (LZ4_stream_t* LZ4_dict, const char* source, char* dest, int inputSize)
{
	LZ4_stream_t_internal* streamPtr = (LZ4_stream_t_internal*)LZ4_dict;
	int result;
	const BYTE* const dictEnd = streamPtr->dictionary + streamPtr->dictSize;

	const BYTE* smallest = dictEnd;
	if (smallest > (const BYTE*) source) smallest = (const BYTE*) source;
	LZ4_renormDictT((LZ4_stream_t_internal*)LZ4_dict, smallest);

	result = LZ4_compress_generic(LZ4_dict, source, dest, inputSize, 0, notLimited, byU32, usingExtDict, noDictIssue);

	streamPtr->dictionary = (const BYTE*)source;
	streamPtr->dictSize = (U32)inputSize;
	streamPtr->currentOffset += (U32)inputSize;

	return result;
}

int LZ4_saveDict (void* LZ4_dict, char* safeBuffer, int dictSize)
{
	LZ4_stream_t_internal* dict = (LZ4_stream_t_internal*) LZ4_dict;
	const BYTE* previousDictEnd = dict->dictionary + dict->dictSize;

	if ((U32)dictSize > 64 KB) dictSize = 64 KB;   /* useless to define a dictionary > 64 KB */
	if ((U32)dictSize > dict->dictSize) dictSize = dict->dictSize;

	memcpy(safeBuffer, previousDictEnd - dictSize, dictSize);

	dict->dictionary = (const BYTE*)safeBuffer;
	dict->dictSize = (U32)dictSize;

	return 1;
}

/****************************
   Decompression functions
****************************/
/*
 * This generic decompression function cover all use cases.
 * It shall be instanciated several times, using different sets of directives
 * Note that it is essential this generic function is really inlined,
 * in order to remove useless branches during compilation optimisation.
 */
FORCE_INLINE int LZ4_decompress_generic(
				 const char* source,
				 char* dest,
				 int inputSize,
				 int outputSize,         /* If endOnInput==endOnInputSize, this value is the max size of Output Buffer. */

				 int endOnInput,         /* endOnOutputSize, endOnInputSize */
				 int partialDecoding,    /* full, partial */
				 int targetOutputSize,   /* only used if partialDecoding==partial */
				 int dict,               /* noDict, withPrefix64k, usingExtDict */
				 const char* dictStart,  /* only if dict==usingExtDict */
				 int dictSize            /* note : = 0 if noDict */
				 )
{
	/* Local Variables */
	const BYTE* restrict ip = (const BYTE*) source;
	const BYTE* ref;
	const BYTE* const iend = ip + inputSize;

	BYTE* op = (BYTE*) dest;
	BYTE* const oend = op + outputSize;
	BYTE* cpy;
	BYTE* oexit = op + targetOutputSize;
	const BYTE* const lowLimit = (const BYTE*)dest - dictSize;

	const BYTE* const dictEnd = (const BYTE*)dictStart + dictSize;
//#define OLD
#ifdef OLD
	const size_t dec32table[] = {0, 3, 2, 3, 0, 0, 0, 0};   /* static reduces speed for LZ4_decompress_safe() on GCC64 */
#else
	const size_t dec32table[] = {4-0, 4-3, 4-2, 4-3, 4-0, 4-0, 4-0, 4-0};   /* static reduces speed for LZ4_decompress_safe() on GCC64 */
#endif
	static const size_t dec64table[] = {0, 0, 0, (size_t)-1, 0, 1, 2, 3};

	const int checkOffset = (endOnInput) && (dictSize < (int)(64 KB));

	/* Special cases */
	if ((partialDecoding) && (oexit> oend-MFLIMIT)) oexit = oend-MFLIMIT;                        /* targetOutputSize too high => decode everything */
	if ((endOnInput) && (unlikely(outputSize==0))) return ((inputSize==1) && (*ip==0)) ? 0 : -1;   /* Empty output buffer */
	if ((!endOnInput) && (unlikely(outputSize==0))) return (*ip==0?1:-1);

	/* Main Loop */
	while (1)
	{
		unsigned token;
		size_t length;

		/* get runlength */
		token = *ip++;
		if ((length=(token>>ML_BITS)) == RUN_MASK)
		{
			unsigned s;
			do
			{
				s = *ip++;
				length += s;
			}
			while (likely((endOnInput)?ip<iend-RUN_MASK:1) && (s==255));
			if ((sizeof(void*)==4) && unlikely(length>LZ4_MAX_INPUT_SIZE)) goto _output_error;   /* overflow detection */
		}

		/* copy literals */
		cpy = op+length;
		if (((endOnInput) && ((cpy>(partialDecoding?oexit:oend-MFLIMIT)) || (ip+length>iend-(2+1+LASTLITERALS))) )
			|| ((!endOnInput) && (cpy>oend-COPYLENGTH)))
		{
			if (partialDecoding)
			{
				if (cpy > oend) goto _output_error;                           /* Error : write attempt beyond end of output buffer */
				if ((endOnInput) && (ip+length > iend)) goto _output_error;   /* Error : read attempt beyond end of input buffer */
			}
			else
			{
				if ((!endOnInput) && (cpy != oend)) goto _output_error;       /* Error : block decoding must stop exactly there */
				if ((endOnInput) && ((ip+length != iend) || (cpy > oend))) goto _output_error;   /* Error : input must be consumed */
			}
			memcpy(op, ip, length);
			ip += length;
			op += length;
			break;                                       /* Necessarily EOF, due to parsing restrictions */
		}
		LZ4_WILDCOPY(op, ip, cpy); ip -= (op-cpy); op = cpy;

		/* get offset */
		LZ4_READ_LITTLEENDIAN_16(ref,cpy,ip); ip+=2;
		if ((checkOffset) && (unlikely(ref < lowLimit))) goto _output_error;   /* Error : offset outside destination buffer */

		/* get matchlength */
		if ((length=(token&ML_MASK)) == ML_MASK)
		{
			unsigned s;
			do
			{
				if (endOnInput && (ip > iend-LASTLITERALS)) goto _output_error;
				s = *ip++;
				length += s;
			} while (s==255);
			if ((sizeof(void*)==4) && unlikely(length>LZ4_MAX_INPUT_SIZE)) goto _output_error;   /* overflow detection */
		}

		/* check external dictionary */
		if ((dict==usingExtDict) && (ref < (BYTE* const)dest))
		{
			if (unlikely(op+length+MINMATCH > oend-LASTLITERALS)) goto _output_error;

			if (length+MINMATCH <= (size_t)(dest-(char*)ref))
			{
				ref = dictEnd - (dest-(char*)ref);
				memcpy(op, ref, length+MINMATCH);
				op += length+MINMATCH;
			}
			else
			{
				size_t copySize = (size_t)(dest-(char*)ref);
				memcpy(op, dictEnd - copySize, copySize);
				op += copySize;
				copySize = length+MINMATCH - copySize;
				if (copySize > (size_t)((char*)op-dest))   /* overlap */
				{
					BYTE* const cpy = op + copySize;
					const BYTE* ref = (BYTE*)dest;
					while (op < cpy) *op++ = *ref++;
				}
				else
				{
					memcpy(op, dest, copySize);
					op += copySize;
				}
			}
			continue;
		}

		/* copy repeated sequence */
		if (unlikely((op-ref)<(int)STEPSIZE))
		{
			const size_t dec64 = dec64table[(sizeof(void*)==4) ? 0 : op-ref];
			op[0] = ref[0];
			op[1] = ref[1];
			op[2] = ref[2];
			op[3] = ref[3];
#ifdef OLD
			op += 4, ref += 4; ref -= dec32table[op-ref];
			A32(op) = A32(ref);
			op += STEPSIZE-4; ref -= dec64;
#else
			ref += dec32table[op-ref];
			A32(op+4) = A32(ref);
			op += STEPSIZE; ref -= dec64;
#endif
		} else { LZ4_COPYSTEP(op,ref); }
		cpy = op + length - (STEPSIZE-4);

		if (unlikely(cpy>oend-COPYLENGTH-(STEPSIZE-4)))
		{
			if (cpy > oend-LASTLITERALS) goto _output_error;    /* Error : last 5 bytes must be literals */
			if (op<oend-COPYLENGTH) LZ4_WILDCOPY(op, ref, (oend-COPYLENGTH));
			while(op<cpy) *op++=*ref++;
			op=cpy;
			continue;
		}
		LZ4_WILDCOPY(op, ref, cpy);
		op=cpy;   /* correction */
	}

	/* end of decoding */
	if (endOnInput)
	   return (int) (((char*)op)-dest);     /* Nb of output bytes decoded */
	else
	   return (int) (((char*)ip)-source);   /* Nb of input bytes read */

	/* Overflow error detected */
_output_error:
	return (int) (-(((char*)ip)-source))-1;
}

int LZ4_decompress_safe(const char* source, char* dest, int compressedSize, int maxOutputSize)
{
	return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, noDict, NULL, 0);
}

int LZ4_decompress_safe_partial(const char* source, char* dest, int compressedSize, int targetOutputSize, int maxOutputSize)
{
	return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, endOnInputSize, partial, targetOutputSize, noDict, NULL, 0);
}

int LZ4_decompress_fast(const char* source, char* dest, int originalSize)
{
	return LZ4_decompress_generic(source, dest, 0, originalSize, endOnOutputSize, full, 0, withPrefix64k, NULL, 0);
}

/* streaming decompression functions */

//#define LZ4_STREAMDECODESIZE_U32 4
//#define LZ4_STREAMDECODESIZE     (LZ4_STREAMDECODESIZE_U32 * sizeof(unsigned int))
//typedef struct { unsigned int table[LZ4_STREAMDECODESIZE_U32]; } LZ4_streamDecode_t;
typedef struct
{
	const char* dictionary;
	int dictSize;
} LZ4_streamDecode_t_internal;

/*
 * If you prefer dynamic allocation methods,
 * LZ4_createStreamDecode()
 * provides a pointer (void*) towards an initialized LZ4_streamDecode_t structure.
 */
void* LZ4_createStreamDecode()
{
	void* lz4s = ALLOCATOR(sizeof(U32), LZ4_STREAMDECODESIZE_U32);
	MEM_INIT(lz4s, 0, LZ4_STREAMDECODESIZE);
	return lz4s;
}

/*
 * LZ4_setDictDecode
 * Use this function to instruct where to find the dictionary
 * This function is not necessary if previous data is still available where it was decoded.
 * Loading a size of 0 is allowed (same effect as no dictionary).
 * Return : 1 if OK, 0 if error
 */
int LZ4_setDictDecode (void* LZ4_streamDecode, const char* dictionary, int dictSize)
{
	LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*) LZ4_streamDecode;
	lz4sd->dictionary = dictionary;
	lz4sd->dictSize = dictSize;
	return 1;
}

/*
*_continue() :
	These decoding functions allow decompression of multiple blocks in "streaming" mode.
	Previously decoded blocks must still be available at the memory position where they were decoded.
	If it's not possible, save the relevant part of decoded data into a safe buffer,
	and indicate where it stands using LZ4_setDictDecode()
*/
int LZ4_decompress_safe_continue (void* LZ4_streamDecode, const char* source, char* dest, int compressedSize, int maxOutputSize)
{
	LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*) LZ4_streamDecode;
	int result;

	result = LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, usingExtDict, lz4sd->dictionary, lz4sd->dictSize);
	if (result <= 0) return result;
	if (lz4sd->dictionary + lz4sd->dictSize == dest)
	{
		lz4sd->dictSize += result;
	}
	else
	{
		lz4sd->dictionary = dest;
		lz4sd->dictSize = result;
	}

	return result;
}

int LZ4_decompress_fast_continue (void* LZ4_streamDecode, const char* source, char* dest, int originalSize)
{
	LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*) LZ4_streamDecode;
	int result;

	result = LZ4_decompress_generic(source, dest, 0, originalSize, endOnOutputSize, full, 0, usingExtDict, lz4sd->dictionary, lz4sd->dictSize);
	if (result <= 0) return result;
	if (lz4sd->dictionary + lz4sd->dictSize == dest)
	{
		lz4sd->dictSize += result;
	}
	else
	{
		lz4sd->dictionary = dest;
		lz4sd->dictSize = result;
	}

	return result;
}

/*
Advanced decoding functions :
*_usingDict() :
	These decoding functions work the same as "_continue" ones,
	the dictionary must be explicitly provided within parameters
*/

int LZ4_decompress_safe_usingDict(const char* source, char* dest, int compressedSize, int maxOutputSize, const char* dictStart, int dictSize)
{
	return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, usingExtDict, dictStart, dictSize);
}

int LZ4_decompress_fast_usingDict(const char* source, char* dest, int originalSize, const char* dictStart, int dictSize)
{
	return LZ4_decompress_generic(source, dest, 0, originalSize, endOnOutputSize, full, 0, usingExtDict, dictStart, dictSize);
}

/***************************************************
	Obsolete Functions
***************************************************/
/*
These function names are deprecated and should no longer be used.
They are only provided here for compatibility with older user programs.
- LZ4_uncompress is totally equivalent to LZ4_decompress_fast
- LZ4_uncompress_unknownOutputSize is totally equivalent to LZ4_decompress_safe
*/
int LZ4_uncompress (const char* source, char* dest, int outputSize) { return LZ4_decompress_fast(source, dest, outputSize); }
int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize) { return LZ4_decompress_safe(source, dest, isize, maxOutputSize); }

/* Obsolete Streaming functions */

int LZ4_sizeofStreamState() { return LZ4_STREAMSIZE; }

void LZ4_init(LZ4_stream_t_internal* lz4ds, const BYTE* base)
{
	MEM_INIT(lz4ds, 0, LZ4_STREAMSIZE);
	lz4ds->bufferStart = base;
}

int LZ4_resetStreamState(void* state, const char* inputBuffer)
{
	if ((((size_t)state) & 3) != 0) return 1;   /* Error : pointer is not aligned on 4-bytes boundary */
	LZ4_init((LZ4_stream_t_internal*)state, (const BYTE*)inputBuffer);
	return 0;
}

void* LZ4_create (const char* inputBuffer)
{
	void* lz4ds = ALLOCATOR(4, LZ4_STREAMSIZE_U32);
	LZ4_init ((LZ4_stream_t_internal*)lz4ds, (const BYTE*)inputBuffer);
	return lz4ds;
}

char* LZ4_slideInputBuffer (void* LZ4_Data)
{
	LZ4_stream_t_internal* lz4ds = (LZ4_stream_t_internal*)LZ4_Data;

	LZ4_saveDict((LZ4_stream_t*)LZ4_Data, (char*)lz4ds->bufferStart, 64 KB);

	return (char*)(lz4ds->bufferStart + 64 KB);
}

/*  Obsolete compresson functions using User-allocated state */

int LZ4_sizeofState() { return LZ4_STREAMSIZE; }

int LZ4_compress_withState (void* state, const char* source, char* dest, int inputSize)
{
	if (((size_t)(state)&3) != 0) return 0;   /* Error : state is not aligned on 4-bytes boundary */
	MEM_INIT(state, 0, LZ4_STREAMSIZE);

	if (inputSize < (int)LZ4_64KLIMIT)
		return LZ4_compress_generic(state, source, dest, inputSize, 0, notLimited, byU16, noDict, noDictIssue);
	else
		return LZ4_compress_generic(state, source, dest, inputSize, 0, notLimited, (sizeof(void*)==8) ? byU32 : byPtr, noDict, noDictIssue);
}

int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize)
{
	if (((size_t)(state)&3) != 0) return 0;   /* Error : state is not aligned on 4-bytes boundary */
	MEM_INIT(state, 0, LZ4_STREAMSIZE);

	if (inputSize < (int)LZ4_64KLIMIT)
		return LZ4_compress_generic(state, source, dest, inputSize, maxOutputSize, limitedOutput, byU16, noDict, noDictIssue);
	else
		return LZ4_compress_generic(state, source, dest, inputSize, maxOutputSize, limitedOutput, (sizeof(void*)==8) ? byU32 : byPtr, noDict, noDictIssue);
}

/* Obsolete streaming decompression functions */

int LZ4_decompress_safe_withPrefix64k(const char* source, char* dest, int compressedSize, int maxOutputSize)
{
	return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, withPrefix64k, NULL, 64 KB);
}

int LZ4_decompress_fast_withPrefix64k(const char* source, char* dest, int originalSize)
{
	return LZ4_decompress_generic(source, dest, 0, originalSize, endOnOutputSize, full, 0, withPrefix64k, NULL, 64 KB);
}


#undef ALLOCATOR
#define U16_S U16_S2
#define U32_S U32_S2
#define U64_S U64_S2
#undef AARCH
#undef HASHTABLESIZE
#undef LZ4_COPYSTEP
#undef LZ4_WILDCOPY
#undef MAX_DISTANCE
#undef ML_MASK
#undef STEPSIZE
#define LZ4_NbCommonBytes LZ4_NbCommonBytes2
#define limitedOutput limitedOutput2
#define limitedOutput_directive limitedOutput_directive2

#line 3 "lz4hc.c"
/**************************************
   Tuning Parameter
**************************************/
#define LZ4HC_DEFAULT_COMPRESSIONLEVEL 8

/**************************************
   Memory routines
**************************************/
#include <stdlib.h>   /* calloc, free */
#define ALLOCATOR(s)  calloc(1,s)
#define FREEMEM       free
#include <string.h>   /* memset, memcpy */
#define MEM_INIT      memset

/**************************************
   CPU Feature Detection
**************************************/
/* 32 or 64 bits ? */
#if (defined(__x86_64__) || defined(_M_X64) || defined(_WIN64) \
  || defined(__powerpc64__) || defined(__powerpc64le__) \
  || defined(__ppc64__) || defined(__ppc64le__) \
  || defined(__PPC64__) || defined(__PPC64LE__) \
  || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) )   /* Detects 64 bits mode */
#  define LZ4_ARCH64 1
#else
#  define LZ4_ARCH64 0
#endif

/*
 * Little Endian or Big Endian ?
 * Overwrite the #define below if you know your architecture endianess
 */
#include <stdlib.h>   /* Apparently required to detect endianess */
#if defined (__GLIBC__)
#  include <endian.h>
#  if (__BYTE_ORDER == __BIG_ENDIAN)
#     define LZ4_BIG_ENDIAN 1
#  endif
#elif (defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined(_BIG_ENDIAN)) && !(defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN))
#  define LZ4_BIG_ENDIAN 1
#elif defined(__sparc) || defined(__sparc__) \
   || defined(__powerpc__) || defined(__ppc__) || defined(__PPC__) \
   || defined(__hpux)  || defined(__hppa) \
   || defined(_MIPSEB) || defined(__s390__)
#  define LZ4_BIG_ENDIAN 1
#else
/* Little Endian assumed. PDP Endian and other very rare endian format are unsupported. */
#endif

/*
 * Unaligned memory access is automatically enabled for "common" CPU, such as x86.
 * For others CPU, the compiler will be more cautious, and insert extra code to ensure aligned access is respected
 * If you know your target CPU supports unaligned memory access, you want to force this option manually to improve performance
 */
#if defined(__ARM_FEATURE_UNALIGNED)
#  define LZ4_FORCE_UNALIGNED_ACCESS 1
#endif

/* Define this parameter if your target system or compiler does not support hardware bit count */
#if defined(_MSC_VER) && defined(_WIN32_WCE)            /* Visual Studio for Windows CE does not support Hardware bit count */
#  define LZ4_FORCE_SW_BITCOUNT
#endif

/**************************************
 Compiler Options
**************************************/
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
/* "restrict" is a known keyword */
#else
#  define restrict /* Disable restrict */
#endif

#ifdef _MSC_VER    /* Visual Studio */
#  define FORCE_INLINE static __forceinline
#  include <intrin.h>                    /* For Visual 2005 */
#  if LZ4_ARCH64   /* 64-bits */
#    pragma intrinsic(_BitScanForward64) /* For Visual 2005 */
#    pragma intrinsic(_BitScanReverse64) /* For Visual 2005 */
#  else            /* 32-bits */
#    pragma intrinsic(_BitScanForward)   /* For Visual 2005 */
#    pragma intrinsic(_BitScanReverse)   /* For Visual 2005 */
#  endif
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#  pragma warning(disable : 4701)        /* disable: C4701: potentially uninitialized local variable used */
#else
#  ifdef __GNUC__
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif
#endif

#ifdef _MSC_VER  /* Visual Studio */
#  define lz4_bswap16(x) _byteswap_ushort(x)
#else
#  define lz4_bswap16(x)  ((unsigned short int) ((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8)))
#endif

/**************************************
   Includes
**************************************/

#line 3 "lz4hc.h"


#line 3 "lz4.h"
/**************************************
   Basic Types
**************************************/
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
# include <stdint.h>
  typedef uint8_t  BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      U16;
  typedef unsigned int        U32;
  typedef   signed int        S32;
  typedef unsigned long long  U64;
#endif

#if defined(__GNUC__)  && !defined(LZ4_FORCE_UNALIGNED_ACCESS)
#  define _PACKED __attribute__ ((packed))
#else
#  define _PACKED
#endif

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  ifdef __IBMC__
#    pragma pack(1)
#  else
#    pragma pack(push, 1)
#  endif
#endif

typedef struct _U16_S { U16 v; } _PACKED U16_S;
typedef struct _U32_S { U32 v; } _PACKED U32_S;
typedef struct _U64_S { U64 v; } _PACKED U64_S;

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(pop)
#endif

#define A64(x) (((U64_S *)(x))->v)
#define A32(x) (((U32_S *)(x))->v)
#define A16(x) (((U16_S *)(x))->v)

/**************************************
   Constants
**************************************/
#define MINMATCH 4

#define DICTIONARY_LOGSIZE 16
#define MAXD (1<<DICTIONARY_LOGSIZE)
#define MAXD_MASK ((U32)(MAXD - 1))
#define MAX_DISTANCE (MAXD - 1)

#define HASH_LOG (DICTIONARY_LOGSIZE-1)
#define HASHTABLESIZE (1 << HASH_LOG)
#define HASH_MASK (HASHTABLESIZE - 1)

#define ML_BITS  4
#define ML_MASK  (size_t)((1U<<ML_BITS)-1)
#define RUN_BITS (8-ML_BITS)
#define RUN_MASK ((1U<<RUN_BITS)-1)

#define COPYLENGTH 8
#define LASTLITERALS 5
#define MFLIMIT (COPYLENGTH+MINMATCH)
#define MINLENGTH (MFLIMIT+1)
#define OPTIMAL_ML (int)((ML_MASK-1)+MINMATCH)

#define KB *(1U<<10)
#define MB *(1U<<20)
#define GB *(1U<<30)

/**************************************
   Architecture-specific macros
**************************************/
#if LZ4_ARCH64   /* 64-bit */
#  define STEPSIZE 8
#  define LZ4_COPYSTEP(s,d)     A64(d) = A64(s); d+=8; s+=8;
#  define LZ4_COPYPACKET(s,d)   LZ4_COPYSTEP(s,d)
#  define AARCH A64
#  define HTYPE                 U32
#  define INITBASE(b,s)         const BYTE* const b = s
#else            /* 32-bit */
#  define STEPSIZE 4
#  define LZ4_COPYSTEP(s,d)     A32(d) = A32(s); d+=4; s+=4;
#  define LZ4_COPYPACKET(s,d)   LZ4_COPYSTEP(s,d); LZ4_COPYSTEP(s,d);
#  define AARCH A32
#  define HTYPE                 U32
#  define INITBASE(b,s)         const BYTE* const b = s
#endif

#if defined(LZ4_BIG_ENDIAN)
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { U16 v = A16(p); v = lz4_bswap16(v); d = (s) - v; }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,i)  { U16 v = (U16)(i); v = lz4_bswap16(v); A16(p) = v; p+=2; }
#else      /* Little Endian */
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { d = (s) - A16(p); }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,v)  { A16(p) = v; p+=2; }
#endif

/**************************************
   Local Types
**************************************/
typedef struct
{
	const BYTE* inputBuffer;
	const BYTE* base;
	const BYTE* end;
	HTYPE hashTable[HASHTABLESIZE];
	U16 chainTable[MAXD];
	const BYTE* nextToUpdate;
} LZ4HC_Data_Structure;

/**************************************
   Macros
**************************************/
#define LZ4_WILDCOPY(s,d,e)    do { LZ4_COPYPACKET(s,d) } while (d<e);
#define LZ4_BLINDCOPY(s,d,l)   { BYTE* e=d+l; LZ4_WILDCOPY(s,d,e); d=e; }
#define HASH_FUNCTION(i)       (((i) * 2654435761U) >> ((MINMATCH*8)-HASH_LOG))
#define HASH_VALUE(p)          HASH_FUNCTION(A32(p))
#define HASH_POINTER(p)        (HashTable[HASH_VALUE(p)] + base)
#define DELTANEXT(p)           chainTable[(size_t)(p) & MAXD_MASK]
#define GETNEXT(p)             ((p) - (size_t)DELTANEXT(p))

/**************************************
 Private functions
**************************************/
#if LZ4_ARCH64

FORCE_INLINE int LZ4_NbCommonBytes (register U64 val)
{
#if defined(LZ4_BIG_ENDIAN)
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r = 0;
	_BitScanReverse64( &r, val );
	return (int)(r>>3);
#  elif defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_clzll(val) >> 3);
#  else
	int r;
	if (!(val>>32)) { r=4; } else { r=0; val>>=32; }
	if (!(val>>16)) { r+=2; val>>=8; } else { val>>=24; }
	r += (!val);
	return r;
#  endif
#else
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r = 0;
	_BitScanForward64( &r, val );
	return (int)(r>>3);
#  elif defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_ctzll(val) >> 3);
#  else
	static const int DeBruijnBytePos[64] = { 0, 0, 0, 0, 0, 1, 1, 2, 0, 3, 1, 3, 1, 4, 2, 7, 0, 2, 3, 6, 1, 5, 3, 5, 1, 3, 4, 4, 2, 5, 6, 7, 7, 0, 1, 2, 3, 3, 4, 6, 2, 6, 5, 5, 3, 4, 5, 6, 7, 1, 2, 4, 6, 4, 4, 5, 7, 2, 6, 5, 7, 6, 7, 7 };
	return DeBruijnBytePos[((U64)((val & -val) * 0x0218A392CDABBD3F)) >> 58];
#  endif
#endif
}

#else

FORCE_INLINE int LZ4_NbCommonBytes (register U32 val)
{
#if defined(LZ4_BIG_ENDIAN)
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r;
	_BitScanReverse( &r, val );
	return (int)(r>>3);
#  elif defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_clz(val) >> 3);
#  else
	int r;
	if (!(val>>16)) { r=2; val>>=8; } else { r=0; val>>=24; }
	r += (!val);
	return r;
#  endif
#else
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
	unsigned long r;
	_BitScanForward( &r, val );
	return (int)(r>>3);
#  elif defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return (__builtin_ctz(val) >> 3);
#  else
	static const int DeBruijnBytePos[32] = { 0, 0, 3, 0, 3, 1, 3, 0, 3, 2, 2, 1, 3, 2, 0, 1, 3, 3, 1, 2, 2, 2, 2, 0, 3, 1, 2, 0, 1, 0, 1, 1 };
	return DeBruijnBytePos[((U32)((val & -(S32)val) * 0x077CB531U)) >> 27];
#  endif
#endif
}

#endif

int LZ4_sizeofStreamStateHC()
{
	return sizeof(LZ4HC_Data_Structure);
}

FORCE_INLINE void LZ4_initHC (LZ4HC_Data_Structure* hc4, const BYTE* base)
{
	MEM_INIT((void*)hc4->hashTable, 0, sizeof(hc4->hashTable));
	MEM_INIT(hc4->chainTable, 0xFF, sizeof(hc4->chainTable));
	hc4->nextToUpdate = base + 1;
	hc4->base = base;
	hc4->inputBuffer = base;
	hc4->end = base;
}

int LZ4_resetStreamStateHC(void* state, const char* inputBuffer)
{
	if ((((size_t)state) & (sizeof(void*)-1)) != 0) return 1;   /* Error : pointer is not aligned for pointer (32 or 64 bits) */
	LZ4_initHC((LZ4HC_Data_Structure*)state, (const BYTE*)inputBuffer);
	return 0;
}

void* LZ4_createHC (const char* inputBuffer)
{
	void* hc4 = ALLOCATOR(sizeof(LZ4HC_Data_Structure));
	LZ4_initHC ((LZ4HC_Data_Structure*)hc4, (const BYTE*)inputBuffer);
	return hc4;
}

int LZ4_freeHC (void* LZ4HC_Data)
{
	FREEMEM(LZ4HC_Data);
	return (0);
}

/* Update chains up to ip (excluded) */
FORCE_INLINE void LZ4HC_Insert (LZ4HC_Data_Structure* hc4, const BYTE* ip)
{
	U16*   chainTable = hc4->chainTable;
	HTYPE* HashTable  = hc4->hashTable;
	INITBASE(base,hc4->base);

	while(hc4->nextToUpdate < ip)
	{
		const BYTE* const p = hc4->nextToUpdate;
		size_t delta = (p) - HASH_POINTER(p);
		if (delta>MAX_DISTANCE) delta = MAX_DISTANCE;
		DELTANEXT(p) = (U16)delta;
		HashTable[HASH_VALUE(p)] = (HTYPE)((p) - base);
		hc4->nextToUpdate++;
	}
}

char* LZ4_slideInputBufferHC(void* LZ4HC_Data)
{
	LZ4HC_Data_Structure* hc4 = (LZ4HC_Data_Structure*)LZ4HC_Data;
	U32 distance = (U32)(hc4->end - hc4->inputBuffer) - 64 KB;
	distance = (distance >> 16) << 16;   /* Must be a multiple of 64 KB */
	LZ4HC_Insert(hc4, hc4->end - MINMATCH);
	memcpy((void*)(hc4->end - 64 KB - distance), (const void*)(hc4->end - 64 KB), 64 KB);
	hc4->nextToUpdate -= distance;
	hc4->base -= distance;
	if ((U32)(hc4->inputBuffer - hc4->base) > 1 GB + 64 KB)   /* Avoid overflow */
	{
		int i;
		hc4->base += 1 GB;
		for (i=0; i<HASHTABLESIZE; i++) hc4->hashTable[i] -= 1 GB;
	}
	hc4->end -= distance;
	return (char*)(hc4->end);
}

FORCE_INLINE size_t LZ4HC_CommonLength (const BYTE* p1, const BYTE* p2, const BYTE* const matchlimit)
{
	const BYTE* p1t = p1;

	while (p1t<matchlimit-(STEPSIZE-1))
	{
		size_t diff = AARCH(p2) ^ AARCH(p1t);
		if (!diff) { p1t+=STEPSIZE; p2+=STEPSIZE; continue; }
		p1t += LZ4_NbCommonBytes(diff);
		return (p1t - p1);
	}
	if (LZ4_ARCH64) if ((p1t<(matchlimit-3)) && (A32(p2) == A32(p1t))) { p1t+=4; p2+=4; }
	if ((p1t<(matchlimit-1)) && (A16(p2) == A16(p1t))) { p1t+=2; p2+=2; }
	if ((p1t<matchlimit) && (*p2 == *p1t)) p1t++;
	return (p1t - p1);
}

FORCE_INLINE int LZ4HC_InsertAndFindBestMatch (LZ4HC_Data_Structure* hc4, const BYTE* ip, const BYTE* const matchlimit, const BYTE** matchpos, const int maxNbAttempts)
{
	U16* const chainTable = hc4->chainTable;
	HTYPE* const HashTable = hc4->hashTable;
	const BYTE* ref;
	INITBASE(base,hc4->base);
	int nbAttempts=maxNbAttempts;
	size_t repl=0, ml=0;
	U16 delta=0;  /* useless assignment, to remove an uninitialization warning */

	/* HC4 match finder */
	LZ4HC_Insert(hc4, ip);
	ref = HASH_POINTER(ip);

#define REPEAT_OPTIMIZATION
#ifdef REPEAT_OPTIMIZATION
	/* Detect repetitive sequences of length <= 4 */
	if ((U32)(ip-ref) <= 4)        /* potential repetition */
	{
		if (A32(ref) == A32(ip))   /* confirmed */
		{
			delta = (U16)(ip-ref);
			repl = ml  = LZ4HC_CommonLength(ip+MINMATCH, ref+MINMATCH, matchlimit) + MINMATCH;
			*matchpos = ref;
		}
		ref = GETNEXT(ref);
	}
#endif

	while (((U32)(ip-ref) <= MAX_DISTANCE) && (nbAttempts))
	{
		nbAttempts--;
		if (*(ref+ml) == *(ip+ml))
		if (A32(ref) == A32(ip))
		{
			size_t mlt = LZ4HC_CommonLength(ip+MINMATCH, ref+MINMATCH, matchlimit) + MINMATCH;
			if (mlt > ml) { ml = mlt; *matchpos = ref; }
		}
		ref = GETNEXT(ref);
	}

#ifdef REPEAT_OPTIMIZATION
	/* Complete table */
	if (repl)
	{
		const BYTE* ptr = ip;
		const BYTE* end;

		end = ip + repl - (MINMATCH-1);
		while(ptr < end-delta)
		{
			DELTANEXT(ptr) = delta;    /* Pre-Load */
			ptr++;
		}
		do
		{
			DELTANEXT(ptr) = delta;
			HashTable[HASH_VALUE(ptr)] = (HTYPE)((ptr) - base);     /* Head of chain */
			ptr++;
		} while(ptr < end);
		hc4->nextToUpdate = end;
	}
#endif

	return (int)ml;
}

FORCE_INLINE int LZ4HC_InsertAndGetWiderMatch (LZ4HC_Data_Structure* hc4, const BYTE* ip, const BYTE* startLimit, const BYTE* matchlimit, int longest, const BYTE** matchpos, const BYTE** startpos, const int maxNbAttempts)
{
	U16* const  chainTable = hc4->chainTable;
	HTYPE* const HashTable = hc4->hashTable;
	INITBASE(base,hc4->base);
	const BYTE*  ref;
	int nbAttempts = maxNbAttempts;
	int delta = (int)(ip-startLimit);

	/* First Match */
	LZ4HC_Insert(hc4, ip);
	ref = HASH_POINTER(ip);

	while (((U32)(ip-ref) <= MAX_DISTANCE) && (nbAttempts))
	{
		nbAttempts--;
		if (*(startLimit + longest) == *(ref - delta + longest))
		if (A32(ref) == A32(ip))
		{
#if 1
			const BYTE* reft = ref+MINMATCH;
			const BYTE* ipt = ip+MINMATCH;
			const BYTE* startt = ip;

			while (ipt<matchlimit-(STEPSIZE-1))
			{
				size_t diff = AARCH(reft) ^ AARCH(ipt);
				if (!diff) { ipt+=STEPSIZE; reft+=STEPSIZE; continue; }
				ipt += LZ4_NbCommonBytes(diff);
				goto _endCount;
			}
			if (LZ4_ARCH64) if ((ipt<(matchlimit-3)) && (A32(reft) == A32(ipt))) { ipt+=4; reft+=4; }
			if ((ipt<(matchlimit-1)) && (A16(reft) == A16(ipt))) { ipt+=2; reft+=2; }
			if ((ipt<matchlimit) && (*reft == *ipt)) ipt++;
_endCount:
			reft = ref;
#else
			/* Easier for code maintenance, but unfortunately slower too */
			const BYTE* startt = ip;
			const BYTE* reft = ref;
			const BYTE* ipt = ip + MINMATCH + LZ4HC_CommonLength(ip+MINMATCH, ref+MINMATCH, matchlimit);
#endif

			while ((startt>startLimit) && (reft > hc4->inputBuffer) && (startt[-1] == reft[-1])) {startt--; reft--;}

			if ((ipt-startt) > longest)
			{
				longest = (int)(ipt-startt);
				*matchpos = reft;
				*startpos = startt;
			}
		}
		ref = GETNEXT(ref);
	}

	return longest;
}

typedef enum { noLimit = 0, limitedOutput = 1 } limitedOutput_directive;

FORCE_INLINE int LZ4HC_encodeSequence (
					   const BYTE** ip,
					   BYTE** op,
					   const BYTE** anchor,
					   int matchLength,
					   const BYTE* ref,
					   limitedOutput_directive limitedOutputBuffer,
					   BYTE* oend)
{
	int length;
	BYTE* token;

	/* Encode Literal length */
	length = (int)(*ip - *anchor);
	token = (*op)++;
	if ((limitedOutputBuffer) && ((*op + length + (2 + 1 + LASTLITERALS) + (length>>8)) > oend)) return 1;   /* Check output limit */
	if (length>=(int)RUN_MASK) { int len; *token=(RUN_MASK<<ML_BITS); len = length-RUN_MASK; for(; len > 254 ; len-=255) *(*op)++ = 255;  *(*op)++ = (BYTE)len; }
	else *token = (BYTE)(length<<ML_BITS);

	/* Copy Literals */
	LZ4_BLINDCOPY(*anchor, *op, length);

	/* Encode Offset */
	LZ4_WRITE_LITTLEENDIAN_16(*op,(U16)(*ip-ref));

	/* Encode MatchLength */
	length = (int)(matchLength-MINMATCH);
	if ((limitedOutputBuffer) && (*op + (1 + LASTLITERALS) + (length>>8) > oend)) return 1;   /* Check output limit */
	if (length>=(int)ML_MASK) { *token+=ML_MASK; length-=ML_MASK; for(; length > 509 ; length-=510) { *(*op)++ = 255; *(*op)++ = 255; } if (length > 254) { length-=255; *(*op)++ = 255; } *(*op)++ = (BYTE)length; }
	else *token += (BYTE)(length);

	/* Prepare next loop */
	*ip += matchLength;
	*anchor = *ip;

	return 0;
}

#define MAX_COMPRESSION_LEVEL 16
static int LZ4HC_compress_generic (
				 void* ctxvoid,
				 const char* source,
				 char* dest,
				 int inputSize,
				 int maxOutputSize,
				 int compressionLevel,
				 limitedOutput_directive limit
				)
{
	LZ4HC_Data_Structure* ctx = (LZ4HC_Data_Structure*) ctxvoid;
	const BYTE* ip = (const BYTE*) source;
	const BYTE* anchor = ip;
	const BYTE* const iend = ip + inputSize;
	const BYTE* const mflimit = iend - MFLIMIT;
	const BYTE* const matchlimit = (iend - LASTLITERALS);

	BYTE* op = (BYTE*) dest;
	BYTE* const oend = op + maxOutputSize;

	const int maxNbAttempts = compressionLevel > MAX_COMPRESSION_LEVEL ? 1 << MAX_COMPRESSION_LEVEL : compressionLevel ? 1<<(compressionLevel-1) : 1<<LZ4HC_DEFAULT_COMPRESSIONLEVEL;
	int   ml, ml2, ml3, ml0;
	const BYTE* ref=NULL;
	const BYTE* start2=NULL;
	const BYTE* ref2=NULL;
	const BYTE* start3=NULL;
	const BYTE* ref3=NULL;
	const BYTE* start0;
	const BYTE* ref0;

	/* Ensure blocks follow each other */
	if (ip != ctx->end) return 0;
	ctx->end += inputSize;

	ip++;

	/* Main Loop */
	while (ip < mflimit)
	{
		ml = LZ4HC_InsertAndFindBestMatch (ctx, ip, matchlimit, (&ref), maxNbAttempts);
		if (!ml) { ip++; continue; }

		/* saved, in case we would skip too much */
		start0 = ip;
		ref0 = ref;
		ml0 = ml;

_Search2:
		if (ip+ml < mflimit)
			ml2 = LZ4HC_InsertAndGetWiderMatch(ctx, ip + ml - 2, ip + 1, matchlimit, ml, &ref2, &start2, maxNbAttempts);
		else ml2 = ml;

		if (ml2 == ml)  /* No better match */
		{
			if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;
			continue;
		}

		if (start0 < ip)
		{
			if (start2 < ip + ml0)   /* empirical */
			{
				ip = start0;
				ref = ref0;
				ml = ml0;
			}
		}

		/* Here, start0==ip */
		if ((start2 - ip) < 3)   /* First Match too small : removed */
		{
			ml = ml2;
			ip = start2;
			ref =ref2;
			goto _Search2;
		}

_Search3:
		/*
		 * Currently we have :
		 * ml2 > ml1, and
		 * ip1+3 <= ip2 (usually < ip1+ml1)
		 */
		if ((start2 - ip) < OPTIMAL_ML)
		{
			int correction;
			int new_ml = ml;
			if (new_ml > OPTIMAL_ML) new_ml = OPTIMAL_ML;
			if (ip+new_ml > start2 + ml2 - MINMATCH) new_ml = (int)(start2 - ip) + ml2 - MINMATCH;
			correction = new_ml - (int)(start2 - ip);
			if (correction > 0)
			{
				start2 += correction;
				ref2 += correction;
				ml2 -= correction;
			}
		}
		/* Now, we have start2 = ip+new_ml, with new_ml = min(ml, OPTIMAL_ML=18) */

		if (start2 + ml2 < mflimit)
			ml3 = LZ4HC_InsertAndGetWiderMatch(ctx, start2 + ml2 - 3, start2, matchlimit, ml2, &ref3, &start3, maxNbAttempts);
		else ml3 = ml2;

		if (ml3 == ml2) /* No better match : 2 sequences to encode */
		{
			/* ip & ref are known; Now for ml */
			if (start2 < ip+ml)  ml = (int)(start2 - ip);
			/* Now, encode 2 sequences */
			if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;
			ip = start2;
			if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml2, ref2, limit, oend)) return 0;
			continue;
		}

		if (start3 < ip+ml+3) /* Not enough space for match 2 : remove it */
		{
			if (start3 >= (ip+ml)) /* can write Seq1 immediately ==> Seq2 is removed, so Seq3 becomes Seq1 */
			{
				if (start2 < ip+ml)
				{
					int correction = (int)(ip+ml - start2);
					start2 += correction;
					ref2 += correction;
					ml2 -= correction;
					if (ml2 < MINMATCH)
					{
						start2 = start3;
						ref2 = ref3;
						ml2 = ml3;
					}
				}

				if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;
				ip  = start3;
				ref = ref3;
				ml  = ml3;

				start0 = start2;
				ref0 = ref2;
				ml0 = ml2;
				goto _Search2;
			}

			start2 = start3;
			ref2 = ref3;
			ml2 = ml3;
			goto _Search3;
		}

		/*
		 * OK, now we have 3 ascending matches; let's write at least the first one
		 * ip & ref are known; Now for ml
		 */
		if (start2 < ip+ml)
		{
			if ((start2 - ip) < (int)ML_MASK)
			{
				int correction;
				if (ml > OPTIMAL_ML) ml = OPTIMAL_ML;
				if (ip + ml > start2 + ml2 - MINMATCH) ml = (int)(start2 - ip) + ml2 - MINMATCH;
				correction = ml - (int)(start2 - ip);
				if (correction > 0)
				{
					start2 += correction;
					ref2 += correction;
					ml2 -= correction;
				}
			}
			else
			{
				ml = (int)(start2 - ip);
			}
		}
		if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;

		ip = start2;
		ref = ref2;
		ml = ml2;

		start2 = start3;
		ref2 = ref3;
		ml2 = ml3;

		goto _Search3;

	}

	/* Encode Last Literals */
	{
		int lastRun = (int)(iend - anchor);
		if ((limit) && (((char*)op - dest) + lastRun + 1 + ((lastRun+255-RUN_MASK)/255) > (U32)maxOutputSize)) return 0;  /* Check output limit */
		if (lastRun>=(int)RUN_MASK) { *op++=(RUN_MASK<<ML_BITS); lastRun-=RUN_MASK; for(; lastRun > 254 ; lastRun-=255) *op++ = 255; *op++ = (BYTE) lastRun; }
		else *op++ = (BYTE)(lastRun<<ML_BITS);
		memcpy(op, anchor, iend - anchor);
		op += iend-anchor;
	}

	/* End */
	return (int) (((char*)op)-dest);
}

int LZ4_compressHC2(const char* source, char* dest, int inputSize, int compressionLevel)
{
	void* ctx = LZ4_createHC(source);
	int result;
	if (ctx==NULL) return 0;

	result = LZ4HC_compress_generic (ctx, source, dest, inputSize, 0, compressionLevel, noLimit);

	LZ4_freeHC(ctx);
	return result;
}

int LZ4_compressHC(const char* source, char* dest, int inputSize) { return LZ4_compressHC2(source, dest, inputSize, 0); }

int LZ4_compressHC2_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel)
{
	void* ctx = LZ4_createHC(source);
	int result;
	if (ctx==NULL) return 0;

	result = LZ4HC_compress_generic (ctx, source, dest, inputSize, maxOutputSize, compressionLevel, limitedOutput);

	LZ4_freeHC(ctx);
	return result;
}

int LZ4_compressHC_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize)
{
	return LZ4_compressHC2_limitedOutput(source, dest, inputSize, maxOutputSize, 0);
}

/*****************************
   Using external allocation
*****************************/
int LZ4_sizeofStateHC() { return sizeof(LZ4HC_Data_Structure); }

int LZ4_compressHC2_withStateHC (void* state, const char* source, char* dest, int inputSize, int compressionLevel)
{
	if (((size_t)(state)&(sizeof(void*)-1)) != 0) return 0;   /* Error : state is not aligned for pointers (32 or 64 bits) */
	LZ4_initHC ((LZ4HC_Data_Structure*)state, (const BYTE*)source);
	return LZ4HC_compress_generic (state, source, dest, inputSize, 0, compressionLevel, noLimit);
}

int LZ4_compressHC_withStateHC (void* state, const char* source, char* dest, int inputSize)
{ return LZ4_compressHC2_withStateHC (state, source, dest, inputSize, 0); }

int LZ4_compressHC2_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel)
{
	if (((size_t)(state)&(sizeof(void*)-1)) != 0) return 0;   /* Error : state is not aligned for pointers (32 or 64 bits) */
	LZ4_initHC ((LZ4HC_Data_Structure*)state, (const BYTE*)source);
	return LZ4HC_compress_generic (state, source, dest, inputSize, maxOutputSize, compressionLevel, limitedOutput);
}

int LZ4_compressHC_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize)
{ return LZ4_compressHC2_limitedOutput_withStateHC (state, source, dest, inputSize, maxOutputSize, 0); }

/****************************
   Stream functions
****************************/

int LZ4_compressHC_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize)
{
	return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, 0, 0, noLimit);
}

int LZ4_compressHC2_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel)
{
	return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, 0, compressionLevel, noLimit);
}

int LZ4_compressHC_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize)
{
	return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, maxOutputSize, 0, limitedOutput);
}

int LZ4_compressHC2_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel)
{
	return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, maxOutputSize, compressionLevel, limitedOutput);
}


#undef KB
#undef MB
#undef MAX_DISTANCE
#endif

#if 1
// shoco

#line 3 "shoco.c"
// #include <stdint.h>

#if (defined (__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || __BIG_ENDIAN__)
  #define swap(x) (x)
#else
  #if defined(_MSC_VER)
	#include <stdlib.h>
	#define swap(x) _byteswap_ulong(x)
  #elif defined (__GNUC__)
	#define swap(x) __builtin_bswap32(x)
  #else
	#include <byteswap.h>
	#define swap(x) bswap_32(x)
  #endif
#endif

#if defined(_MSC_VER)
  #define _ALIGNED __declspec(align(16))
#elif defined(__GNUC__)
  #define _ALIGNED __attribute__ ((aligned(16)))
#else
  #define _ALIGNED
#endif

#if defined(_M_X64) || defined (_M_AMD64) || defined (__x86_64__)
  #include "emmintrin.h"
  #define HAVE_SSE2
#endif


#line 3 "shoco.h"
#define _SHOCO_INTERNAL

#line 3 "shoco_model.h"
#ifndef _SHOCO_INTERNAL
#error This header file is only to be included by 'shoco.c'.
#endif
#pragma once
/*
This file was generated by 'generate_compressor_model.py'
so don't edit this by hand. Also, do not include this file
anywhere. It is internal to 'shoco.c'. Include 'shoco.h'
if you want to use shoco in your project.
*/

#define MIN_CHR 39
#define MAX_CHR 122

static const char chrs_by_chr_id[32] = {
  'e', 'a', 'i', 'o', 't', 'h', 'n', 'r', 's', 'l', 'u', 'c', 'w', 'm', 'd', 'b', 'p', 'f', 'g', 'v', 'y', 'k', '-', 'H', 'M', 'T', '\'', 'B', 'x', 'I', 'W', 'L'
};

static const int8_t chr_ids_by_chr[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 27, -1, -1, -1, -1, -1, 23, 29, -1, -1, 31, 24, -1, -1, -1, -1, -1, -1, 25, -1, -1, 30, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 15, 11, 14, 0, 17, 18, 5, 2, -1, 21, 9, 13, 6, 3, 16, -1, 7, 8, 4, 10, 19, 12, 28, 20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static const int8_t successor_ids_by_chr_id_and_chr_id[32][32] = {
  {7, 4, 12, -1, 6, -1, 1, 0, 3, 5, -1, 9, -1, 8, 2, -1, 15, 14, -1, 10, 11, -1, -1, -1, -1, -1, -1, -1, 13, -1, -1, -1},
  {-1, -1, 6, -1, 1, -1, 0, 3, 2, 4, 15, 11, -1, 9, 5, 10, 13, -1, 12, 8, 7, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {9, 11, -1, 4, 2, -1, 0, 8, 1, 5, -1, 6, -1, 3, 7, 15, -1, 12, 10, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, 14, 7, 5, -1, 1, 2, 8, 9, 0, 15, 6, 4, 11, -1, 12, 3, -1, 10, -1, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {2, 4, 3, 1, 5, 0, -1, 6, 10, 9, 7, 12, 11, -1, -1, -1, -1, 13, -1, -1, 8, -1, 15, -1, -1, -1, 14, -1, -1, -1, -1, -1},
  {0, 1, 2, 3, 4, -1, -1, 5, 9, 10, 6, -1, -1, 8, 15, 11, -1, 14, -1, -1, 7, -1, 13, -1, -1, -1, 12, -1, -1, -1, -1, -1},
  {2, 8, 7, 4, 3, -1, 9, -1, 6, 11, -1, 5, -1, -1, 0, -1, -1, 14, 1, 15, 10, 12, -1, -1, -1, -1, 13, -1, -1, -1, -1, -1},
  {0, 3, 1, 2, 6, -1, 9, 8, 4, 12, 13, 10, -1, 11, 7, -1, -1, 15, 14, -1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0, 6, 3, 4, 1, 2, -1, -1, 5, 10, 7, 9, 11, 12, -1, -1, 8, 14, -1, -1, 15, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0, 6, 2, 5, 9, -1, -1, -1, 10, 1, 8, -1, 12, 14, 4, -1, 15, 7, -1, 13, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {8, 10, 9, 15, 1, -1, 4, 0, 3, 2, -1, 6, -1, 12, 11, 13, 7, 14, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {1, 3, 6, 0, 4, 2, -1, 7, 13, 8, 9, 11, -1, -1, 15, -1, -1, -1, -1, -1, 10, 5, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {3, 0, 1, 4, -1, 2, 5, 6, 7, 8, -1, 14, -1, -1, 9, 15, -1, 12, -1, -1, -1, 10, 11, -1, -1, -1, 13, -1, -1, -1, -1, -1},
  {0, 1, 3, 2, 15, -1, 12, -1, 7, 14, 4, -1, -1, 9, -1, 8, 5, 10, -1, -1, 6, -1, 13, -1, -1, -1, 11, -1, -1, -1, -1, -1},
  {0, 3, 1, 2, -1, -1, 12, 6, 4, 9, 7, -1, -1, 14, 8, -1, -1, 15, 11, 13, 5, -1, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0, 5, 7, 2, 10, 13, -1, 6, 8, 1, 3, -1, -1, 14, 15, 11, -1, -1, -1, 12, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0, 2, 6, 3, 7, 10, -1, 1, 9, 4, 8, -1, -1, 15, -1, 12, 5, -1, -1, -1, 11, -1, 13, -1, -1, -1, 14, -1, -1, -1, -1, -1},
  {1, 3, 4, 0, 7, -1, 12, 2, 11, 8, 6, 13, -1, -1, -1, -1, -1, 5, -1, -1, 10, 15, 9, -1, -1, -1, 14, -1, -1, -1, -1, -1},
  {1, 3, 5, 2, 13, 0, 9, 4, 7, 6, 8, -1, -1, 15, -1, 11, -1, -1, 10, -1, 14, -1, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0, 2, 1, 3, -1, -1, -1, 6, -1, -1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {1, 11, 4, 0, 3, -1, 13, 12, 2, 7, -1, -1, 15, 10, 5, 8, 14, -1, -1, -1, -1, -1, 9, -1, -1, -1, 6, -1, -1, -1, -1, -1},
  {0, 9, 2, 14, 15, 4, 1, 13, 3, 5, -1, -1, 10, -1, -1, -1, -1, 6, 12, -1, 7, -1, 8, -1, -1, -1, 11, -1, -1, -1, -1, -1},
  {-1, 2, 14, -1, 1, 5, 8, 7, 4, 12, -1, 6, 9, 11, 13, 3, 10, 15, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0, 1, 3, 2, -1, -1, -1, -1, -1, -1, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {4, 3, 1, 5, -1, -1, -1, 0, -1, -1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {2, 8, 4, 1, -1, 0, -1, 6, -1, -1, 5, -1, 7, -1, -1, -1, -1, -1, -1, -1, 10, -1, -1, 9, -1, -1, -1, -1, -1, -1, -1, -1},
  {12, 5, -1, -1, 1, -1, -1, 7, 0, 3, -1, 2, -1, 4, 6, -1, -1, -1, -1, 8, -1, -1, 15, -1, 13, 9, -1, -1, -1, -1, -1, 11},
  {1, 3, 2, 4, -1, -1, -1, 5, -1, 7, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 6, -1, -1, -1, -1, -1, -1, -1, -1, 8, -1, -1},
  {5, 3, 4, 12, 1, 6, -1, -1, -1, -1, 8, 2, -1, -1, -1, -1, 0, 9, -1, -1, 11, -1, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, 0, -1, 1, 12, 3, -1, -1, -1, -1, 5, -1, -1, -1, 2, -1, -1, -1, -1, -1, -1, -1, -1, 4, -1, -1, 6, -1, 10},
  {2, 3, 1, 4, -1, 0, -1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, 6, -1, -1},
  {5, 1, 3, 0, -1, -1, -1, -1, -1, -1, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, -1, 9, -1, -1, 6, -1, 7}
};

static const int8_t chrs_by_chr_and_successor_id[MAX_CHR - MIN_CHR][16] = {
  {'s', 't', 'c', 'l', 'm', 'a', 'd', 'r', 'v', 'T', 'A', 'L', 'e', 'M', 'Y', '-'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'-', 't', 'a', 'b', 's', 'h', 'c', 'r', 'n', 'w', 'p', 'm', 'l', 'd', 'i', 'f'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'u', 'e', 'i', 'a', 'o', 'r', 'y', 'l', 'I', 'E', 'R', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'e', 'a', 'o', 'i', 'u', 'A', 'y', 'E', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'t', 'n', 'f', 's', '\'', 'm', 'I', 'N', 'A', 'E', 'L', 'Z', 'r', 'V', 'R', 'C'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'o', 'a', 'y', 'i', 'u', 'e', 'I', 'L', 'D', '\'', 'E', 'Y', '\x00', '\x00', '\x00', '\x00'},
  {'r', 'i', 'y', 'a', 'e', 'o', 'u', 'Y', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'h', 'o', 'e', 'E', 'i', 'u', 'r', 'w', 'a', 'H', 'y', 'R', 'Z', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'h', 'i', 'e', 'a', 'o', 'r', 'I', 'y', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'n', 't', 's', 'r', 'l', 'd', 'i', 'y', 'v', 'm', 'b', 'c', 'g', 'p', 'k', 'u'},
  {'e', 'l', 'o', 'u', 'y', 'a', 'r', 'i', 's', 'j', 't', 'b', 'v', 'h', 'm', 'd'},
  {'o', 'e', 'h', 'a', 't', 'k', 'i', 'r', 'l', 'u', 'y', 'c', 'q', 's', '-', 'd'},
  {'e', 'i', 'o', 'a', 's', 'y', 'r', 'u', 'd', 'l', '-', 'g', 'n', 'v', 'm', 'f'},
  {'r', 'n', 'd', 's', 'a', 'l', 't', 'e', 'm', 'c', 'v', 'y', 'i', 'x', 'f', 'p'},
  {'o', 'e', 'r', 'a', 'i', 'f', 'u', 't', 'l', '-', 'y', 's', 'n', 'c', '\'', 'k'},
  {'h', 'e', 'o', 'a', 'r', 'i', 'l', 's', 'u', 'n', 'g', 'b', '-', 't', 'y', 'm'},
  {'e', 'a', 'i', 'o', 't', 'r', 'u', 'y', 'm', 's', 'l', 'b', '\'', '-', 'f', 'd'},
  {'n', 's', 't', 'm', 'o', 'l', 'c', 'd', 'r', 'e', 'g', 'a', 'f', 'v', 'z', 'b'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'e', 'n', 'i', 's', 'h', 'l', 'f', 'y', '-', 'a', 'w', '\'', 'g', 'r', 'o', 't'},
  {'e', 'l', 'i', 'y', 'd', 'o', 'a', 'f', 'u', 't', 's', 'k', 'w', 'v', 'm', 'p'},
  {'e', 'a', 'o', 'i', 'u', 'p', 'y', 's', 'b', 'm', 'f', '\'', 'n', '-', 'l', 't'},
  {'d', 'g', 'e', 't', 'o', 'c', 's', 'i', 'a', 'n', 'y', 'l', 'k', '\'', 'f', 'v'},
  {'u', 'n', 'r', 'f', 'm', 't', 'w', 'o', 's', 'l', 'v', 'd', 'p', 'k', 'i', 'c'},
  {'e', 'r', 'a', 'o', 'l', 'p', 'i', 't', 'u', 's', 'h', 'y', 'b', '-', '\'', 'm'},
  {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'e', 'i', 'o', 'a', 's', 'y', 't', 'd', 'r', 'n', 'c', 'm', 'l', 'u', 'g', 'f'},
  {'e', 't', 'h', 'i', 'o', 's', 'a', 'u', 'p', 'c', 'l', 'w', 'm', 'k', 'f', 'y'},
  {'h', 'o', 'e', 'i', 'a', 't', 'r', 'u', 'y', 'l', 's', 'w', 'c', 'f', '\'', '-'},
  {'r', 't', 'l', 's', 'n', 'g', 'c', 'p', 'e', 'i', 'a', 'd', 'm', 'b', 'f', 'o'},
  {'e', 'i', 'a', 'o', 'y', 'u', 'r', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'},
  {'a', 'i', 'h', 'e', 'o', 'n', 'r', 's', 'l', 'd', 'k', '-', 'f', '\'', 'c', 'b'},
  {'p', 't', 'c', 'a', 'i', 'e', 'h', 'q', 'u', 'f', '-', 'y', 'o', '\x00', '\x00', '\x00'},
  {'o', 'e', 's', 't', 'i', 'd', '\'', 'l', 'b', '-', 'm', 'a', 'r', 'n', 'p', 'w'}
};

typedef struct Pack {
  const uint32_t word;
  const unsigned int bytes_packed;
  const unsigned int bytes_unpacked;
  const unsigned int offsets[8];
  const int16_t _ALIGNED masks[8];
  const char header_mask;
  const char header;
} Pack;

#define PACK_COUNT 3
#define MAX_SUCCESSOR_N 7

static const Pack packs[PACK_COUNT] = {
  { 0x80000000, 1, 2, { 26, 24, 24, 24, 24, 24, 24, 24 }, { 15, 3, 0, 0, 0, 0, 0, 0 }, char(0xc0), char(0x80) },
  { 0xc0000000, 2, 4, { 25, 22, 19, 16, 16, 16, 16, 16 }, { 15, 7, 7, 7, 0, 0, 0, 0 }, char(0xe0), char(0xc0) },
  { 0xe0000000, 4, 8, { 23, 19, 15, 11, 8, 5, 2, 0 }, { 31, 15, 15, 15, 7, 7, 7, 3 }, char(0xf0), char(0xe0) }
};


static inline int decode_header(unsigned char val) {
  int i = -1;
  while ((signed char)val < 0) {
	val <<= 1;
	++i;
  }
  return i;
}

union Code {
  uint32_t word;
  char bytes[4];
};

#ifdef HAVE_SSE2
static inline int check_indices(const int16_t * restrict indices, int pack_n) {
  __m128i zero = _mm_setzero_si128();
  __m128i indis = _mm_load_si128 ((__m128i *)indices);
  __m128i masks = _mm_load_si128 ((__m128i *)packs[pack_n].masks);
  __m128i cmp = _mm_cmpgt_epi16 (indis, masks);
  __m128i mmask = _mm_cmpgt_epi16 (masks, zero);
  cmp = _mm_and_si128 (cmp, mmask);
  int result = _mm_movemask_epi8 (cmp);
  return (result == 0);
}
#else
static inline int check_indices(const int16_t * restrict indices, int pack_n) {
  for (int i = 0; i < packs[pack_n].bytes_unpacked; ++i)
	if (indices[i] > packs[pack_n].masks[i])
	  return 0;
  return 1;
}
#endif

static inline int find_best_encoding(const int16_t * restrict indices, int n_consecutive) {
  for (int p = PACK_COUNT - 1; p >= 0; --p)
	if ((n_consecutive >= packs[p].bytes_unpacked) && (check_indices(indices, p)))
	  return p;
  return -1;
}

size_t shoco_compress(const char * const restrict original, size_t strlen, char * const restrict out, size_t bufsize) {
  char *o = out;
  char * const out_end = out + bufsize;
  const char *in = original;
  int16_t _ALIGNED indices[MAX_SUCCESSOR_N + 1] = { 0 };
  int last_chr_index;
  int current_index;
  int successor_index;
  int n_consecutive;
  union Code code;
  int pack_n;
  int rest;
  const char * const in_end = original + strlen;

  while ((*in != '\0')) {
	if (strlen && (in > in_end))
	  break;

	// find the longest string of known successors
	indices[0] = chr_ids_by_chr[(unsigned char)in[0]];
	last_chr_index = indices[0];
	if (last_chr_index < 0)
	  goto last_resort;

	rest = in_end - in;
	for (n_consecutive = 1; n_consecutive <= MAX_SUCCESSOR_N; ++n_consecutive) {
	  if (strlen && (n_consecutive > rest))
		break;

	  current_index = chr_ids_by_chr[(unsigned char)in[n_consecutive]];
	  if (current_index < 0)  // '\0' is always -1
		break;

	  successor_index = successor_ids_by_chr_id_and_chr_id[last_chr_index][current_index];
	  if (successor_index < 0)
		break;

	  indices[n_consecutive] = successor_index;
	  last_chr_index = current_index;
	}
	if (n_consecutive < 2)
	  goto last_resort;

	pack_n = find_best_encoding(indices, n_consecutive);
	if (pack_n >= 0) {
	  if (o + packs[pack_n].bytes_packed > out_end)
		return bufsize + 1;

	  code.word = packs[pack_n].word;
	  for (int i = 0; i < packs[pack_n].bytes_unpacked; ++i)
		code.word |= indices[i] << packs[pack_n].offsets[i];

	  // In the little-endian world, we need to swap what's
	  // in the register to match the memory representation.
	  // On big-endian systems, this is a dummy.
	  code.word = swap(code.word);

	  // if we'd just copy the word, we might write over the end
	  // of the output string
	  for (int i = 0; i < packs[pack_n].bytes_packed; ++i)
		o[i] = code.bytes[i];

	  o += packs[pack_n].bytes_packed;
	  in += packs[pack_n].bytes_unpacked;
	} else {
last_resort:
	  if (*in & 0x80) {
		// non-ascii case
		if (o + 2 > out_end)
		  return bufsize + 1;
		// put in a sentinel byte
		*o++ = 0x00;
	  } else {
		// an ascii byte
		if (o + 1 > out_end)
		  return bufsize + 1;
	  }
	  *o++ = *in++;
	}
  }

  return o - out;
}

size_t shoco_decompress(const char * const restrict original, size_t complen, char * const restrict out, size_t bufsize) {
  char *o = out;
  char * const out_end = out + bufsize;
  const char *in = original;
  char last_chr;
  union Code code;
  int offset;
  int mask;
  int mark;
  const char * const in_end = original + complen;

  while (in < in_end) {
	mark = decode_header(*in);
	if (mark < 0) {
	  if (o >= out_end)
		return bufsize + 1;

	  // ignore the sentinel value for non-ascii chars
	  if (*in == 0x00)
		++in;

	  *o++ = *in++;
	} else {
	  if (o + packs[mark].bytes_unpacked > out_end)
		return bufsize + 1;

	  // This should be OK as well, but it fails with emscripten.
	  // Test this with new versions of emcc.
	  //code.word = swap(*(uint32_t *)in);
	  for (int i = 0; i < packs[mark].bytes_packed; ++i)
		code.bytes[i] = in[i];
	  code.word = swap(code.word);

	  // unpack the leading char
	  offset = packs[mark].offsets[0];
	  mask = packs[mark].masks[0];
	  last_chr = o[0] = chrs_by_chr_id[(code.word >> offset) & mask];

	  // unpack the successor chars
	  for (int i = 1; i < packs[mark].bytes_unpacked; ++i) {
		offset = packs[mark].offsets[i];
		mask = packs[mark].masks[i];
		last_chr = o[i] = chrs_by_chr_and_successor_id[(unsigned char)last_chr - MIN_CHR][(code.word >> offset) & mask];
	  }

	  o += packs[mark].bytes_unpacked;
	  in += packs[mark].bytes_packed;
	}
  }

  // append a 0-terminator if it fits
  if (o < out_end)
	*o = '\0';

  return o - out;
}


#endif

#if 1
// easylzma

#line 3 "common_internal.c"

#line 3 "common_internal.h"
#ifndef __ELZMA_COMMON_INTERNAL_H__
#define __ELZMA_COMMON_INTERNAL_H__

/** a structure which may be cast and passed into Igor's allocate
 *  routines */
struct elzma_alloc_struct {
	void *(*Alloc)(void *p, size_t size);
	void (*Free)(void *p, void *address); /* address can be 0 */

	elzma_malloc clientMallocFunc;
	void * clientMallocContext;

	elzma_free clientFreeFunc;
	void * clientFreeContext;
};

/* initialize an allocation structure, may be called safely multiple
 * times */
void init_alloc_struct(struct elzma_alloc_struct * allocStruct,
					   elzma_malloc clientMallocFunc,
					   void * clientMallocContext,
					   elzma_free clientFreeFunc,
					   void * clientFreeContext);

/** superset representation of a compressed file header */
struct elzma_file_header {
	unsigned char pb;
	unsigned char lp;
	unsigned char lc;
	unsigned char isStreamed;
	long long unsigned int uncompressedSize;
	unsigned int dictSize;
};

/** superset representation of a compressed file footer */
struct elzma_file_footer {
	unsigned int crc32;
	long long unsigned int uncompressedSize;
};

/** a structure which encapsulates information about the particular
 *  file header and footer in use (lzip vs lzma vs (eventually) xz.
 *  The intention of this structure is to simplify compression and
 *  decompression logic by abstracting the file format details a bit.  */
struct elzma_format_handler
{
	unsigned int header_size;
	void (*init_header)(struct elzma_file_header * hdr);
	int (*parse_header)(const unsigned char * hdrBuf,
						struct elzma_file_header * hdr);
	int (*serialize_header)(unsigned char * hdrBuf,
							const struct elzma_file_header * hdr);

	unsigned int footer_size;
	int (*serialize_footer)(struct elzma_file_footer * ftr,
							unsigned char * ftrBuf);
	int (*parse_footer)(const unsigned char * ftrBuf,
						struct elzma_file_footer * ftr);
};

#endif

static void *elzmaAlloc(void *p, size_t size) {
	struct elzma_alloc_struct * as = (struct elzma_alloc_struct *) p;
	if (as->clientMallocFunc) {
		return as->clientMallocFunc(as->clientMallocContext, size);
	}
	return malloc(size);
}

static void elzmaFree(void *p, void *address) {
	struct elzma_alloc_struct * as = (struct elzma_alloc_struct *) p;
	if (as->clientFreeFunc) {
		as->clientFreeFunc(as->clientMallocContext, address);
	} else {
		free(address);
	}
}

void
init_alloc_struct(struct elzma_alloc_struct * as,
				  elzma_malloc clientMallocFunc,
				  void * clientMallocContext,
				  elzma_free clientFreeFunc,
				  void * clientFreeContext)
{
	as->Alloc = elzmaAlloc;
	as->Free = elzmaFree;
	as->clientMallocFunc = clientMallocFunc;
	as->clientMallocContext = clientMallocContext;
	as->clientFreeFunc = clientFreeFunc;
	as->clientFreeContext = clientFreeContext;
}



#line 3 "compress.c"

#line 3 "lzma_header.h"
#ifndef __EASYLZMA_LZMA_HEADER__
#define __EASYLZMA_LZMA_HEADER__

/* LZMA-Alone header format gleaned from reading Igor's code */

void initializeLZMAFormatHandler(struct elzma_format_handler * hand);

#endif


#line 3 "lzip_header.h"
#ifndef __EASYLZMA_LZIP_HEADER__
#define __EASYLZMA_LZIP_HEADER__

/* lzip file format documented here:
 * http://download.savannah.gnu.org/releases-noredirect/lzip/manual/ */

void initializeLZIPFormatHandler(struct elzma_format_handler * hand);

#endif


#line 3 "Types.h"
#ifndef __7Z_TYPES_H
#define __7Z_TYPES_H

#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef EXTERN_C_BEGIN
#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif
#endif

EXTERN_C_BEGIN

#define SZ_OK 0

#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
#define SZ_ERROR_OUTPUT_EOF 7
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_PROGRESS 10
#define SZ_ERROR_FAIL 11
#define SZ_ERROR_THREAD 12

#define SZ_ERROR_ARCHIVE 16
#define SZ_ERROR_NO_ARCHIVE 17

typedef int SRes;

#ifdef _WIN32
typedef DWORD WRes;
#else
typedef int WRes;
#endif

#ifndef RINOK
#define RINOK(x) { int __result__ = (x); if (__result__ != 0) return __result__; }
#endif

typedef unsigned char Byte;
typedef short Int16;
typedef unsigned short UInt16;

#ifdef _LZMA_UINT32_IS_ULONG
typedef long Int32;
typedef unsigned long UInt32;
#else
typedef int Int32;
typedef unsigned int UInt32;
#endif

#ifdef _SZ_NO_INT_64

/* define _SZ_NO_INT_64, if your compiler doesn't support 64-bit integers.
   NOTES: Some code will work incorrectly in that case! */

typedef long Int64;
typedef unsigned long UInt64;

#else

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 Int64;
typedef unsigned __int64 UInt64;
#define UINT64_CONST(n) n
#else
typedef long long int Int64;
typedef unsigned long long int UInt64;
#define UINT64_CONST(n) n ## ULL
#endif

#endif

#ifdef _LZMA_NO_SYSTEM_SIZE_T
typedef UInt32 SizeT;
#else
typedef size_t SizeT;
#endif

typedef int Bool;
#define True 1
#define False 0

#ifdef _WIN32
#define MY_STD_CALL __stdcall
#else
#define MY_STD_CALL
#endif

#ifdef _MSC_VER

#if _MSC_VER >= 1300
#define MY_NO_INLINE __declspec(noinline)
#else
#define MY_NO_INLINE
#endif

#define MY_CDECL __cdecl
#define MY_FAST_CALL __fastcall

#else

#define MY_CDECL
#define MY_FAST_CALL

#endif

/* The following interfaces use first parameter as pointer to structure */

typedef struct
{
  Byte (*Read)(void *p); /* reads one byte, returns 0 in case of EOF or error */
} IByteIn;

typedef struct
{
  void (*Write)(void *p, Byte b);
} IByteOut;

typedef struct
{
  SRes (*Read)(void *p, void *buf, size_t *size);
	/* if (input(*size) != 0 && output(*size) == 0) means end_of_stream.
	   (output(*size) < input(*size)) is allowed */
} ISeqInStream;

/* it can return SZ_ERROR_INPUT_EOF */
SRes SeqInStream_Read(ISeqInStream *stream, void *buf, size_t size);
SRes SeqInStream_Read2(ISeqInStream *stream, void *buf, size_t size, SRes errorType);
SRes SeqInStream_ReadByte(ISeqInStream *stream, Byte *buf);

typedef struct
{
  size_t (*Write)(void *p, const void *buf, size_t size);
	/* Returns: result - the number of actually written bytes.
	   (result < size) means error */
} ISeqOutStream;

typedef enum
{
  SZ_SEEK_SET = 0,
  SZ_SEEK_CUR = 1,
  SZ_SEEK_END = 2
} ESzSeek;

typedef struct
{
  SRes (*Read)(void *p, void *buf, size_t *size);  /* same as ISeqInStream::Read */
  SRes (*Seek)(void *p, Int64 *pos, ESzSeek origin);
} ISeekInStream;

typedef struct
{
  SRes (*Look)(void *p, const void **buf, size_t *size);
	/* if (input(*size) != 0 && output(*size) == 0) means end_of_stream.
	   (output(*size) > input(*size)) is not allowed
	   (output(*size) < input(*size)) is allowed */
  SRes (*Skip)(void *p, size_t offset);
	/* offset must be <= output(*size) of Look */

  SRes (*Read)(void *p, void *buf, size_t *size);
	/* reads directly (without buffer). It's same as ISeqInStream::Read */
  SRes (*Seek)(void *p, Int64 *pos, ESzSeek origin);
} ILookInStream;

SRes LookInStream_LookRead(ILookInStream *stream, void *buf, size_t *size);
SRes LookInStream_SeekTo(ILookInStream *stream, UInt64 offset);

/* reads via ILookInStream::Read */
SRes LookInStream_Read2(ILookInStream *stream, void *buf, size_t size, SRes errorType);
SRes LookInStream_Read(ILookInStream *stream, void *buf, size_t size);

#define LookToRead_BUF_SIZE (1 << 14)

typedef struct
{
  ILookInStream s;
  ISeekInStream *realStream;
  size_t pos;
  size_t size;
  Byte buf[LookToRead_BUF_SIZE];
} CLookToRead;

void LookToRead_CreateVTable(CLookToRead *p, int lookahead);
void LookToRead_Init(CLookToRead *p);

typedef struct
{
  ISeqInStream s;
  ILookInStream *realStream;
} CSecToLook;

void SecToLook_CreateVTable(CSecToLook *p);

typedef struct
{
  ISeqInStream s;
  ILookInStream *realStream;
} CSecToRead;

void SecToRead_CreateVTable(CSecToRead *p);

typedef struct
{
  SRes (*Progress)(void *p, UInt64 inSize, UInt64 outSize);
	/* Returns: result. (result != SZ_OK) means break.
	   Value (UInt64)(Int64)-1 for size means unknown value. */
} ICompressProgress;

typedef struct
{
  void *(*Alloc)(void *p, size_t size);
  void (*Free)(void *p, void *address); /* address can be 0 */
} ISzAlloc;

#define IAlloc_Alloc(p, size) (p)->Alloc((p), size)
#define IAlloc_Free(p, a) (p)->Free((p), a)

#ifdef _WIN32

#define CHAR_PATH_SEPARATOR '\\'
#define WCHAR_PATH_SEPARATOR L'\\'
#define STRING_PATH_SEPARATOR "\\"
#define WSTRING_PATH_SEPARATOR L"\\"

#else

#define CHAR_PATH_SEPARATOR '/'
#define WCHAR_PATH_SEPARATOR L'/'
#define STRING_PATH_SEPARATOR "/"
#define WSTRING_PATH_SEPARATOR L"/"

#endif

EXTERN_C_END

#endif


#line 3 "LzmaEnc.h"
#ifndef __LZMA_ENC_H
#define __LZMA_ENC_H

EXTERN_C_BEGIN

#define LZMA_PROPS_SIZE 5

typedef struct _CLzmaEncProps
{
  int level;       /*  0 <= level <= 9 */
  UInt32 dictSize; /* (1 << 12) <= dictSize <= (1 << 27) for 32-bit version
					  (1 << 12) <= dictSize <= (1 << 30) for 64-bit version
					   default = (1 << 24) */
  UInt32 reduceSize; /* estimated size of data that will be compressed. default = 0xFFFFFFFF.
						Encoder uses this value to reduce dictionary size */
  int lc;          /* 0 <= lc <= 8, default = 3 */
  int lp;          /* 0 <= lp <= 4, default = 0 */
  int pb;          /* 0 <= pb <= 4, default = 2 */
  int algo;        /* 0 - fast, 1 - normal, default = 1 */
  int fb;          /* 5 <= fb <= 273, default = 32 */
  int btMode;      /* 0 - hashChain Mode, 1 - binTree mode - normal, default = 1 */
  int numHashBytes; /* 2, 3 or 4, default = 4 */
  UInt32 mc;        /* 1 <= mc <= (1 << 30), default = 32 */
  unsigned writeEndMark;  /* 0 - do not write EOPM, 1 - write EOPM, default = 0 */
  int numThreads;  /* 1 or 2, default = 2 */
} CLzmaEncProps;

void LzmaEncProps_Init(CLzmaEncProps *p);
void LzmaEncProps_Normalize(CLzmaEncProps *p);
UInt32 LzmaEncProps_GetDictSize(const CLzmaEncProps *props2);

/* ---------- CLzmaEncHandle Interface ---------- */

/* LzmaEnc_* functions can return the following exit codes:
Returns:
  SZ_OK           - OK
  SZ_ERROR_MEM    - Memory allocation error
  SZ_ERROR_PARAM  - Incorrect paramater in props
  SZ_ERROR_WRITE  - Write callback error.
  SZ_ERROR_PROGRESS - some break from progress callback
  SZ_ERROR_THREAD - errors in multithreading functions (only for Mt version)
*/

typedef void * CLzmaEncHandle;

CLzmaEncHandle LzmaEnc_Create(ISzAlloc *alloc);
void LzmaEnc_Destroy(CLzmaEncHandle p, ISzAlloc *alloc, ISzAlloc *allocBig);
SRes LzmaEnc_SetProps(CLzmaEncHandle p, const CLzmaEncProps *props);
SRes LzmaEnc_WriteProperties(CLzmaEncHandle p, Byte *properties, SizeT *size);
SRes LzmaEnc_Encode(CLzmaEncHandle p, ISeqOutStream *outStream, ISeqInStream *inStream,
	ICompressProgress *progress, ISzAlloc *alloc, ISzAlloc *allocBig);
SRes LzmaEnc_MemEncode(CLzmaEncHandle p, Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
	int writeEndMark, ICompressProgress *progress, ISzAlloc *alloc, ISzAlloc *allocBig);

/* ---------- One Call Interface ---------- */

/* LzmaEncode
Return code:
  SZ_OK               - OK
  SZ_ERROR_MEM        - Memory allocation error
  SZ_ERROR_PARAM      - Incorrect paramater
  SZ_ERROR_OUTPUT_EOF - output buffer overflow
  SZ_ERROR_THREAD     - errors in multithreading functions (only for Mt version)
*/

SRes LzmaEncode(Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
	const CLzmaEncProps *props, Byte *propsEncoded, SizeT *propsSize, int writeEndMark,
	ICompressProgress *progress, ISzAlloc *alloc, ISzAlloc *allocBig);

EXTERN_C_END

#endif


#line 3 "7zCrc.h"
#ifndef __7Z_CRC_H
#define __7Z_CRC_H

#include <stddef.h>

extern UInt32 g_CrcTable[];

void MY_FAST_CALL CrcGenerateTable(void);

#define CRC_INIT_VAL 0xFFFFFFFF
#define CRC_GET_DIGEST(crc) ((crc) ^ 0xFFFFFFFF)
#define CRC_UPDATE_BYTE(crc, b) (g_CrcTable[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

UInt32 MY_FAST_CALL CrcUpdate(UInt32 crc, const void *data, size_t size);
UInt32 MY_FAST_CALL CrcCalc(const void *data, size_t size);

#endif

#include <string.h>

struct _elzma_compress_handle {
	CLzmaEncProps props;
	CLzmaEncHandle encHand;
	unsigned long long uncompressedSize;
	elzma_file_format format;
	struct elzma_alloc_struct allocStruct;
	struct elzma_format_handler formatHandler;
};

elzma_compress_handle
elzma_compress_alloc()
{
	elzma_compress_handle hand = (elzma_compress_handle)malloc(sizeof(struct _elzma_compress_handle));
	memset((void *) hand, 0, sizeof(struct _elzma_compress_handle));

/*  info

  -d{N}:  set dictionary - [0,28], default: 23 (2^23 = 8MB)

  -fb{N}: set number of fast bytes - [5, 255], default: 128
		  Usually big number gives a little bit better compression ratio
		  and slower compression process.

  -lc{N}: set number of literal context bits - [0, 8], default: 3
		  Sometimes lc=4 gives gain for big files.

  -lp{N}: set number of literal pos bits - [0, 4], default: 0
		  lp switch is intended for periodical data when period is
		  equal 2^value (where lp=value). For example, for 32-bit (4 bytes)
		  periodical data you can use lp=2. Often it's better to set lc=0,
		  if you change lp switch.

  -pb{N}: set number of pos bits - [0, 4], default: 2
		  pb switch is intended for periodical data
		  when period is equal 2^value (where lp=value).

  -eos:   write End Of Stream marker

*/

	/* "reasonable" defaults for props */
	LzmaEncProps_Init(&(hand->props));
	hand->props.lc = 3;
	hand->props.lp = 0;
	hand->props.pb = 2;
	hand->props.level = 9; // default: 5, max: 9
	hand->props.algo = 1;
	hand->props.fb = 32; // default: 32, max: 273
	hand->props.dictSize = 1 << 20; // default: 1<<24 16MiB
	hand->props.btMode = 1;
	hand->props.numHashBytes = 4;
	hand->props.mc = 32;
	hand->props.numThreads = 1;
	hand->props.writeEndMark = 1;

	init_alloc_struct(&(hand->allocStruct), NULL, NULL, NULL, NULL);

	/* default format is LZMA-Alone */
	initializeLZMAFormatHandler(&(hand->formatHandler));

	return hand;
}

void
elzma_compress_free(elzma_compress_handle * hand)
{
	if (hand && *hand) {
		if ((*hand)->encHand) {
			LzmaEnc_Destroy((*hand)->encHand,
							(ISzAlloc *) &((*hand)->allocStruct),
							(ISzAlloc *) &((*hand)->allocStruct));
		}

	}
	*hand = NULL;
}

int
elzma_compress_config(elzma_compress_handle hand,
					  unsigned char lc,
					  unsigned char lp,
					  unsigned char pb,
					  unsigned char level,
					  unsigned int dictionarySize,
					  elzma_file_format format,
					  unsigned long long uncompressedSize)
{
	/* XXX: validate arguments are in valid ranges */

	hand->props.lc = lc;
	hand->props.lp = lp;
	hand->props.pb = pb;
	hand->props.level = level;
	hand->props.dictSize = dictionarySize;
	hand->uncompressedSize = uncompressedSize;
	hand->format = format;

	/* default of LZMA-Alone is set at alloc time, and there are only
	 * two possible formats */
	if (format == ELZMA_lzip) {
		initializeLZIPFormatHandler(&(hand->formatHandler));
	}

	return ELZMA_E_OK;
}

/* use Igor's stream hooks for compression. */
struct elzmaInStream
{
	SRes (*ReadPtr)(void *p, void *buf, size_t *size);
	elzma_read_callback inputStream;
	void * inputContext;
	unsigned int crc32;
	unsigned int crc32a;
	unsigned int crc32b;
	unsigned int crc32c;
	int calculateCRC;
};

static SRes elzmaReadFunc(void *p, void *buf, size_t *size)
{
	int rv;
	struct elzmaInStream * is = (struct elzmaInStream *) p;
	rv = is->inputStream(is->inputContext, buf, size);
	if (rv == 0 && *size > 0 && is->calculateCRC) {
		is->crc32 = CrcUpdate(is->crc32, buf, *size);
	}
	return rv;
}

struct elzmaOutStream {
	size_t (*WritePtr)(void *p, const void *buf, size_t size);
	elzma_write_callback outputStream;
	void * outputContext;
};

static size_t elzmaWriteFunc(void *p, const void *buf, size_t size)
{
	struct elzmaOutStream * os = (struct elzmaOutStream *) p;
	return os->outputStream(os->outputContext, buf, size);
}

/* use Igor's stream hooks for compression. */
struct elzmaProgressStruct
{
	SRes (*Progress)(void *p, UInt64 inSize, UInt64 outSize);
	long long unsigned int uncompressedSize;
	elzma_progress_callback progressCallback;
	void * progressContext;

};

#include <stdio.h>
static SRes elzmaProgress(void *p, UInt64 inSize, UInt64 outSize)
{
	struct elzmaProgressStruct * ps = (struct elzmaProgressStruct *) p;
	if (ps->progressCallback) {
		ps->progressCallback(ps->progressContext, inSize,
							 ps->uncompressedSize);
	}
	return SZ_OK;
}

void elzma_compress_set_allocation_callbacks(
	elzma_compress_handle hand,
	elzma_malloc mallocFunc, void * mallocFuncContext,
	elzma_free freeFunc, void * freeFuncContext)
{
	if (hand) {
		init_alloc_struct(&(hand->allocStruct),
						  mallocFunc, mallocFuncContext,
						  freeFunc, freeFuncContext);
	}
}

int
elzma_compress_run(elzma_compress_handle hand,
				   elzma_read_callback inputStream, void * inputContext,
				   elzma_write_callback outputStream, void * outputContext,
				   elzma_progress_callback progressCallback,
				   void * progressContext)
{
	struct elzmaInStream inStreamStruct;
	struct elzmaOutStream outStreamStruct;
	struct elzmaProgressStruct progressStruct;
	SRes r;

	CrcGenerateTable();

	if (hand == NULL || inputStream == NULL) return ELZMA_E_BAD_PARAMS;

	/* initialize stream structrures */
	inStreamStruct.ReadPtr = elzmaReadFunc;
	inStreamStruct.inputStream = inputStream;
	inStreamStruct.inputContext = inputContext;
	inStreamStruct.crc32 = CRC_INIT_VAL;
	inStreamStruct.calculateCRC =
		(hand->formatHandler.serialize_footer != NULL);

	outStreamStruct.WritePtr = elzmaWriteFunc;
	outStreamStruct.outputStream = outputStream;
	outStreamStruct.outputContext = outputContext;

	progressStruct.Progress = elzmaProgress;
	progressStruct.uncompressedSize = hand->uncompressedSize;
	progressStruct.progressCallback = progressCallback;
	progressStruct.progressContext = progressContext;

	/* create an encoding object */
	hand->encHand = LzmaEnc_Create((ISzAlloc *) &(hand->allocStruct));

	if (hand->encHand == NULL) {
		return ELZMA_E_COMPRESS_ERROR;
	}

	/* inintialize with compression parameters */
	if (SZ_OK != LzmaEnc_SetProps(hand->encHand, &(hand->props)))
	{
		return ELZMA_E_BAD_PARAMS;
	}

	/* verify format is sane */
	if (ELZMA_lzma != hand->format && ELZMA_lzip != hand->format) {
		return ELZMA_E_UNSUPPORTED_FORMAT;
	}

	/* now write the compression header header */
	{
		unsigned char * hdr = (unsigned char *)
			hand->allocStruct.Alloc(&(hand->allocStruct),
									hand->formatHandler.header_size);

		struct elzma_file_header h;
		size_t wt;

		hand->formatHandler.init_header(&h);
		h.pb = (unsigned char) hand->props.pb;
		h.lp = (unsigned char) hand->props.lp;
		h.lc = (unsigned char) hand->props.lc;
		h.dictSize = hand->props.dictSize;
		h.isStreamed = (unsigned char) (hand->uncompressedSize == 0);
		h.uncompressedSize = hand->uncompressedSize;

		hand->formatHandler.serialize_header(hdr, &h);

		wt = outputStream(outputContext, (void *) hdr,
						  hand->formatHandler.header_size);

		hand->allocStruct.Free(&(hand->allocStruct), hdr);

		if (wt != hand->formatHandler.header_size) {
			return ELZMA_E_OUTPUT_ERROR;
		}
	}

	/* begin LZMA encoding */
	/* XXX: expose encoding progress */
	r = LzmaEnc_Encode(hand->encHand,
					   (ISeqOutStream *) &outStreamStruct,
					   (ISeqInStream *) &inStreamStruct,
					   (ICompressProgress *) &progressStruct,
					   (ISzAlloc *) &(hand->allocStruct),
					   (ISzAlloc *) &(hand->allocStruct));

	if (r != SZ_OK) return ELZMA_E_COMPRESS_ERROR;

	/* support a footer! (lzip) */
	if (hand->formatHandler.serialize_footer != NULL &&
		hand->formatHandler.footer_size > 0)
	{
		size_t wt;
		unsigned char * ftrBuf = (unsigned char *)
			hand->allocStruct.Alloc(&(hand->allocStruct),
									hand->formatHandler.footer_size);
		struct elzma_file_footer ftr;
		ftr.crc32 = inStreamStruct.crc32 ^ 0xFFFFFFFF;
		ftr.uncompressedSize = hand->uncompressedSize;

		hand->formatHandler.serialize_footer(&ftr, ftrBuf);

		wt = outputStream(outputContext, (void *) ftrBuf,
						  hand->formatHandler.footer_size);

		hand->allocStruct.Free(&(hand->allocStruct), ftrBuf);

		if (wt != hand->formatHandler.footer_size) {
			return ELZMA_E_OUTPUT_ERROR;
		}
	}

	return ELZMA_E_OK;
}

unsigned int
elzma_get_dict_size(unsigned long long size)
{
	int i = 13; /* 16k dict is minimum */

	/* now we'll find the closes power of two with a max at 16< *
	 * if the size is greater than 8m, we'll divide by two, all of this
	 * is based on a quick set of emperical tests on hopefully
	 * representative sample data */
	if ( size > ( 1 << 23 ) ) size >>= 1;

	while (size >> i) i++;

	if (i > 23) return 1 << 23;

	/* now 1 << i is greater than size, let's return either 1<<i or 1<<(i-1),
	 * whichever is closer to size */
	return 1 << ((((1 << i) - size) > (size - (1 << (i-1)))) ? i-1 : i);
}


#line 3 "decompress.c"

#line 3 "LzmaDec.h"
#ifndef __LZMA_DEC_H
#define __LZMA_DEC_H

#ifdef __cplusplus
extern "C" {
#endif

/* #define _LZMA_PROB32 */
/* _LZMA_PROB32 can increase the speed on some CPUs,
   but memory usage for CLzmaDec::probs will be doubled in that case */

#ifdef _LZMA_PROB32
#define CLzmaProb UInt32
#else
#define CLzmaProb UInt16
#endif

/* ---------- LZMA Properties ---------- */

#define LZMA_PROPS_SIZE 5

typedef struct _CLzmaProps
{
  unsigned lc, lp, pb;
  UInt32 dicSize;
} CLzmaProps;

/* LzmaProps_Decode - decodes properties
Returns:
  SZ_OK
  SZ_ERROR_UNSUPPORTED - Unsupported properties
*/

SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size);

/* ---------- LZMA Decoder state ---------- */

/* LZMA_REQUIRED_INPUT_MAX = number of required input bytes for worst case.
   Num bits = log2((2^11 / 31) ^ 22) + 26 < 134 + 26 = 160; */

#define LZMA_REQUIRED_INPUT_MAX 20

typedef struct
{
  CLzmaProps prop;
  CLzmaProb *probs;
  Byte *dic;
  const Byte *buf;
  UInt32 range, code;
  SizeT dicPos;
  SizeT dicBufSize;
  UInt32 processedPos;
  UInt32 checkDicSize;
  unsigned state;
  UInt32 reps[4];
  unsigned remainLen;
  int needFlush;
  int needInitState;
  UInt32 numProbs;
  unsigned tempBufSize;
  Byte tempBuf[LZMA_REQUIRED_INPUT_MAX];
} CLzmaDec;

#define LzmaDec_Construct(p) { (p)->dic = 0; (p)->probs = 0; }

void LzmaDec_Init(CLzmaDec *p);

/* There are two types of LZMA streams:
	 0) Stream with end mark. That end mark adds about 6 bytes to compressed size.
	 1) Stream without end mark. You must know exact uncompressed size to decompress such stream. */

typedef enum
{
  LZMA_FINISH_ANY,   /* finish at any point */
  LZMA_FINISH_END    /* block must be finished at the end */
} ELzmaFinishMode;

/* ELzmaFinishMode has meaning only if the decoding reaches output limit !!!

   You must use LZMA_FINISH_END, when you know that current output buffer
   covers last bytes of block. In other cases you must use LZMA_FINISH_ANY.

   If LZMA decoder sees end marker before reaching output limit, it returns SZ_OK,
   and output value of destLen will be less than output buffer size limit.
   You can check status result also.

   You can use multiple checks to test data integrity after full decompression:
	 1) Check Result and "status" variable.
	 2) Check that output(destLen) = uncompressedSize, if you know real uncompressedSize.
	 3) Check that output(srcLen) = compressedSize, if you know real compressedSize.
		You must use correct finish mode in that case. */

typedef enum
{
  LZMA_STATUS_NOT_SPECIFIED,               /* use main error code instead */
  LZMA_STATUS_FINISHED_WITH_MARK,          /* stream was finished with end mark. */
  LZMA_STATUS_NOT_FINISHED,                /* stream was not finished */
  LZMA_STATUS_NEEDS_MORE_INPUT,            /* you must provide more input bytes */
  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK  /* there is probability that stream was finished without end mark */
} ELzmaStatus;

/* ELzmaStatus is used only as output value for function call */

/* ---------- Interfaces ---------- */

/* There are 3 levels of interfaces:
	 1) Dictionary Interface
	 2) Buffer Interface
	 3) One Call Interface
   You can select any of these interfaces, but don't mix functions from different
   groups for same object. */

/* There are two variants to allocate state for Dictionary Interface:
	 1) LzmaDec_Allocate / LzmaDec_Free
	 2) LzmaDec_AllocateProbs / LzmaDec_FreeProbs
   You can use variant 2, if you set dictionary buffer manually.
   For Buffer Interface you must always use variant 1.

LzmaDec_Allocate* can return:
  SZ_OK
  SZ_ERROR_MEM         - Memory allocation error
  SZ_ERROR_UNSUPPORTED - Unsupported properties
*/

SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAlloc *alloc);
void LzmaDec_FreeProbs(CLzmaDec *p, ISzAlloc *alloc);

SRes LzmaDec_Allocate(CLzmaDec *state, const Byte *prop, unsigned propsSize, ISzAlloc *alloc);
void LzmaDec_Free(CLzmaDec *state, ISzAlloc *alloc);

/* ---------- Dictionary Interface ---------- */

/* You can use it, if you want to eliminate the overhead for data copying from
   dictionary to some other external buffer.
   You must work with CLzmaDec variables directly in this interface.

   STEPS:
	 LzmaDec_Constr()
	 LzmaDec_Allocate()
	 for (each new stream)
	 {
	   LzmaDec_Init()
	   while (it needs more decompression)
	   {
		 LzmaDec_DecodeToDic()
		 use data from CLzmaDec::dic and update CLzmaDec::dicPos
	   }
	 }
	 LzmaDec_Free()
*/

/* LzmaDec_DecodeToDic

   The decoding to internal dictionary buffer (CLzmaDec::dic).
   You must manually update CLzmaDec::dicPos, if it reaches CLzmaDec::dicBufSize !!!

finishMode:
  It has meaning only if the decoding reaches output limit (dicLimit).
  LZMA_FINISH_ANY - Decode just dicLimit bytes.
  LZMA_FINISH_END - Stream must be finished after dicLimit.

Returns:
  SZ_OK
	status:
	  LZMA_STATUS_FINISHED_WITH_MARK
	  LZMA_STATUS_NOT_FINISHED
	  LZMA_STATUS_NEEDS_MORE_INPUT
	  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
  SZ_ERROR_DATA - Data error
*/

SRes LzmaDec_DecodeToDic(CLzmaDec *p, SizeT dicLimit,
	const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);

/* ---------- Buffer Interface ---------- */

/* It's zlib-like interface.
   See LzmaDec_DecodeToDic description for information about STEPS and return results,
   but you must use LzmaDec_DecodeToBuf instead of LzmaDec_DecodeToDic and you don't need
   to work with CLzmaDec variables manually.

finishMode:
  It has meaning only if the decoding reaches output limit (*destLen).
  LZMA_FINISH_ANY - Decode just destLen bytes.
  LZMA_FINISH_END - Stream must be finished after (*destLen).
*/

SRes LzmaDec_DecodeToBuf(CLzmaDec *p, Byte *dest, SizeT *destLen,
	const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);

/* ---------- One Call Interface ---------- */

/* LzmaDecode

finishMode:
  It has meaning only if the decoding reaches output limit (*destLen).
  LZMA_FINISH_ANY - Decode just destLen bytes.
  LZMA_FINISH_END - Stream must be finished after (*destLen).

Returns:
  SZ_OK
	status:
	  LZMA_STATUS_FINISHED_WITH_MARK
	  LZMA_STATUS_NOT_FINISHED
	  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
  SZ_ERROR_DATA - Data error
  SZ_ERROR_MEM  - Memory allocation error
  SZ_ERROR_UNSUPPORTED - Unsupported properties
  SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).
*/

SRes LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
	const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode,
	ELzmaStatus *status, ISzAlloc *alloc);

#ifdef __cplusplus
}
#endif

#endif

#include <string.h>
#include <assert.h>

#define ELZMA_DECOMPRESS_INPUT_BUFSIZE (1024 * 64)
#define ELZMA_DECOMPRESS_OUTPUT_BUFSIZE (1024 * 256)

/** an opaque handle to an lzma decompressor */
struct _elzma_decompress_handle {
	char inbuf[ELZMA_DECOMPRESS_INPUT_BUFSIZE];
	char outbuf[ELZMA_DECOMPRESS_OUTPUT_BUFSIZE];
	struct elzma_alloc_struct allocStruct;
};

elzma_decompress_handle
elzma_decompress_alloc()
{
	elzma_decompress_handle hand = (elzma_decompress_handle)
		malloc(sizeof(struct _elzma_decompress_handle));
	memset((void *) hand, 0, sizeof(struct _elzma_decompress_handle));
	init_alloc_struct(&(hand->allocStruct), NULL, NULL, NULL, NULL);
	return hand;
}

void elzma_decompress_set_allocation_callbacks(
	elzma_decompress_handle hand,
	elzma_malloc mallocFunc, void * mallocFuncContext,
	elzma_free freeFunc, void * freeFuncContext)
{
	if (hand) {
		init_alloc_struct(&(hand->allocStruct),
						  mallocFunc, mallocFuncContext,
						  freeFunc, freeFuncContext);
	}
}

void
elzma_decompress_free(elzma_decompress_handle * hand)
{
	if (*hand) free(*hand);
	*hand = NULL;
}

int
elzma_decompress_run(elzma_decompress_handle hand,
					 elzma_read_callback inputStream, void * inputContext,
					 elzma_write_callback outputStream, void * outputContext,
					 elzma_file_format format)
{
	unsigned long long int totalRead = 0; /* total amount read from stream */
	unsigned int crc32 = CRC_INIT_VAL; /* running crc32 (lzip case) */
	CLzmaDec dec;
	unsigned int errorCode = ELZMA_E_OK;
	struct elzma_format_handler formatHandler;
	struct elzma_file_header h;
	struct elzma_file_footer f;

	/* switch between supported formats */
	if (format == ELZMA_lzma) {
		initializeLZMAFormatHandler(&formatHandler);
	} else if (format == ELZMA_lzip) {
		CrcGenerateTable();
		initializeLZIPFormatHandler(&formatHandler);
	} else {
		return ELZMA_E_BAD_PARAMS;
	}

	/* initialize footer */
	f.crc32 = 0;
	f.uncompressedSize = 0;

	/* initialize decoder memory */
	memset((void *) &dec, 0, sizeof(dec));
	LzmaDec_Init(&dec);

	/* decode the header. */
	{
		unsigned char * hdr = (unsigned char *)
			hand->allocStruct.Alloc(&(hand->allocStruct),
									formatHandler.header_size);

		size_t sz = formatHandler.header_size;

		formatHandler.init_header(&h);

		if (inputStream(inputContext, hdr, &sz) != 0 ||
			sz != formatHandler.header_size)
		{
			hand->allocStruct.Free(&(hand->allocStruct), hdr);
			return ELZMA_E_INPUT_ERROR;
		}

		if (0 != formatHandler.parse_header(hdr, &h)) {
			hand->allocStruct.Free(&(hand->allocStruct), hdr);
			return ELZMA_E_CORRUPT_HEADER;
		}

		/* the LzmaDec_Allocate call requires 5 bytes which have
		 * compression properties encoded in them.  In the case of
		 * lzip, the header format does not already contain what
		 * LzmaDec_Allocate expects, so we must craft it, silly */
		{
			unsigned char propsBuf[13];
			const unsigned char * propsPtr = hdr;

			if (format == ELZMA_lzip) {
				struct elzma_format_handler lzmaHand;
				initializeLZMAFormatHandler(&lzmaHand);
				lzmaHand.serialize_header(propsBuf, &h);
				propsPtr = propsBuf;
			}

			/* now we're ready to allocate the decoder */
			LzmaDec_Allocate(&dec, propsPtr, 5,
							 (ISzAlloc *) &(hand->allocStruct));
		}

		hand->allocStruct.Free(&(hand->allocStruct), hdr);
	}

	/* perform the decoding */
	for (;;)
	{
		size_t dstLen = ELZMA_DECOMPRESS_OUTPUT_BUFSIZE;
		size_t srcLen = ELZMA_DECOMPRESS_INPUT_BUFSIZE;
		size_t amt = 0;
		size_t bufOff = 0;
		ELzmaStatus stat;

		if (0 != inputStream(inputContext, hand->inbuf, &srcLen))
		{
			errorCode = ELZMA_E_INPUT_ERROR;
			goto decompressEnd;
		}

		/* handle the case where the input prematurely finishes */
		if (srcLen == 0) {
			errorCode = ELZMA_E_INSUFFICIENT_INPUT;
			goto decompressEnd;
		}

		amt = srcLen;

		/* handle the case where a single read buffer of compressed bytes
		 * will translate into multiple buffers of uncompressed bytes,
		 * with this inner loop */
		stat = LZMA_STATUS_NOT_SPECIFIED;

		while (bufOff < srcLen) {
			SRes r = LzmaDec_DecodeToBuf(&dec, (Byte *) hand->outbuf, &dstLen,
										 ((Byte *) hand->inbuf + bufOff), &amt,
										 LZMA_FINISH_ANY, &stat);

			/* XXX deal with result code more granularly*/
			if (r != SZ_OK) {
				errorCode = ELZMA_E_DECOMPRESS_ERROR;
				goto decompressEnd;
			}

			/* write what we've read */
			{
				size_t wt;

				/* if decoding lzip, update our crc32 value */
				if (format == ELZMA_lzip && dstLen > 0) {
					crc32 = CrcUpdate(crc32, hand->outbuf, dstLen);

				}
				totalRead += dstLen;

				wt = outputStream(outputContext, hand->outbuf, dstLen);
				if (wt != dstLen) {
					errorCode = ELZMA_E_OUTPUT_ERROR;
					goto decompressEnd;
				}
			}

			/* do we have more data on the input buffer? */
			bufOff += amt;
			assert( bufOff <= srcLen );
			if (bufOff >= srcLen) break;
			amt = srcLen - bufOff;

			/* with lzip, we will have the footer left on the buffer! */
			if (stat == LZMA_STATUS_FINISHED_WITH_MARK) {
				break;
			}
		}

		/* now check status */
		if (stat == LZMA_STATUS_FINISHED_WITH_MARK) {
			/* read a footer if one is expected and
			 * present */
			if (formatHandler.footer_size > 0 &&
				amt >= formatHandler.footer_size &&
				formatHandler.parse_footer != NULL)
			{
				formatHandler.parse_footer(
					(unsigned char *) hand->inbuf + bufOff, &f);
			}

			break;
		}
		/* for LZMA utils,  we don't always have a finished mark */
		if (!h.isStreamed && totalRead >= h.uncompressedSize) {
			break;
		}
	}

	/* finish the calculated crc32 */
	crc32 ^= 0xFFFFFFFF;

	/* if we have a footer, check that the calculated crc32 matches
	 * the encoded crc32, and that the sizes match */
	if (formatHandler.footer_size)
	{
		if (f.crc32 != crc32) {
			errorCode = ELZMA_E_CRC32_MISMATCH;
		} else if (f.uncompressedSize != totalRead) {
			errorCode = ELZMA_E_SIZE_MISMATCH;
		}
	}
	else if (!h.isStreamed)
	{
		/* if the format does not support a footer and has an uncompressed
		 * size in the header, let's compare that with how much we actually
		 * read */
		if (h.uncompressedSize != totalRead) {
			errorCode = ELZMA_E_SIZE_MISMATCH;
		}
	}

  decompressEnd:
	LzmaDec_Free(&dec, (ISzAlloc *) &(hand->allocStruct));

	return errorCode;
}


#line 3 "lzip_header.c"
#include <string.h>

#define ELZMA_LZIP_HEADER_SIZE 6
#define ELZMA_LZIP_FOOTER_SIZE 12

static
void initLzipHeader(struct elzma_file_header * hdr)
{
	memset((void *) hdr, 0, sizeof(struct elzma_file_header));
}

static
int parseLzipHeader(const unsigned char * hdrBuf,
					struct elzma_file_header * hdr)
{
	if (0 != strncmp("LZIP", (char *) hdrBuf, 4)) return 1;
	/* XXX: ignore version for now */
	hdr->pb = 2;
	hdr->lp = 0;
	hdr->lc = 3;
	/* unknown at this point */
	hdr->isStreamed = 1;
	hdr->uncompressedSize = 0;
	hdr->dictSize = 1 << (hdrBuf[5] & 0x1F);
	return 0;
}

static int
serializeLzipHeader(unsigned char * hdrBuf,
					const struct elzma_file_header * hdr)
{
	hdrBuf[0] = 'L';
	hdrBuf[1] = 'Z';
	hdrBuf[2] = 'I';
	hdrBuf[3] = 'P';
	hdrBuf[4] = 0;
	{
		int r = 0;
		while ((hdr->dictSize >> r) != 0) r++;
		hdrBuf[5] = (unsigned char) (r-1) & 0x1F;
	}
	return 0;
}

static int
serializeLzipFooter(struct elzma_file_footer * ftr,
					unsigned char * ftrBuf)
{
	unsigned int i = 0;

	/* first crc32 */
	for (i = 0; i < 4; i++) {
		*(ftrBuf++) = (unsigned char) (ftr->crc32 >> (i * 8));
	}

	/* next data size */
	for (i = 0; i < 8; i++) {
		*(ftrBuf++) = (unsigned char) (ftr->uncompressedSize >> (i * 8));
	}

	/* write version 0 files, omit member length for now*/

	return 0;
}

static int
parseLzipFooter(const unsigned char * ftrBuf,
				struct elzma_file_footer * ftr)
{
	unsigned int i = 0;
	ftr->crc32 = 0;
	ftr->uncompressedSize = 0;

	/* first crc32 */
	for (i = 0; i < 4; i++)
	{
		ftr->crc32 += ((unsigned int) *(ftrBuf++) << (i * 8));
	}

	/* next data size */
	for (i = 0; i < 8; i++) {
		ftr->uncompressedSize +=
			(unsigned long long) *(ftrBuf++) << (i * 8);
	}
	/* read version 0 files, omit member length for now*/

	return 0;
}

void
initializeLZIPFormatHandler(struct elzma_format_handler * hand)
{
	hand->header_size = ELZMA_LZIP_HEADER_SIZE;
	hand->init_header = initLzipHeader;
	hand->parse_header = parseLzipHeader;
	hand->serialize_header = serializeLzipHeader;
	hand->footer_size = ELZMA_LZIP_FOOTER_SIZE;
	hand->serialize_footer = serializeLzipFooter;
	hand->parse_footer = parseLzipFooter;
}


#line 3 "lzma_header.c"
/* XXX: clean this up, it's mostly lifted from pavel */

#include <string.h>
#include <assert.h>

#define ELZMA_LZMA_HEADER_SIZE 13
#define ELZMA_LZMA_PROPSBUF_SIZE 5

/****************
  Header parsing
 ****************/

#ifndef UINT64_MAX
#define UINT64_MAX ((unsigned long long) -1)
#endif

/* Parse the properties byte */
static char
lzmadec_header_properties (
	unsigned char *pb, unsigned char *lp, unsigned char *lc, const unsigned char c)
{
/*	 pb, lp and lc are encoded into a single byte.  */
	if (c > (9 * 5 * 5))
		return -1;
	*pb = c / (9 * 5);        /* 0 <= pb <= 4 */
	*lp = (c % (9 * 5)) / 9;  /* 0 <= lp <= 4 */
	*lc = c % 9;              /* 0 <= lc <= 8 */

	assert (*pb < 5 && *lp < 5 && *lc < 9);
	return 0;
}

/* Parse the dictionary size (4 bytes, little endian) */
static char
lzmadec_header_dictionary (unsigned int *size, const unsigned char *buffer)
{
	unsigned int i;
	*size = 0;
	for (i = 0; i < 4; i++)
		*size += (unsigned int)(*buffer++) << (i * 8);
	/* The dictionary size is limited to 256 MiB (checked from
	 * LZMA SDK 4.30) */
	if (*size > (1 << 28))
		return -1;
	return 0;
}

/* Parse the uncompressed size field (8 bytes, little endian) */
static void
lzmadec_header_uncompressed (unsigned long long *size,
							 unsigned char *is_streamed,
							 const unsigned char *buffer)
{
	unsigned int i;

	/* Streamed files have all 64 bits set in the size field.
	 * We don't know the uncompressed size beforehand. */
	*is_streamed = 1;  /* Assume streamed. */
	*size = 0;
	for (i = 0; i < 8; i++) {
		*size += (unsigned long long)buffer[i] << (i * 8);
		if (buffer[i] != 255)
			*is_streamed = 0;
	}
	assert ((*is_streamed == 1 && *size == UINT64_MAX)
			|| (*is_streamed == 0 && *size < UINT64_MAX));
}

static void
initLzmaHeader(struct elzma_file_header * hdr)
{
	memset((void *) hdr, 0, sizeof(struct elzma_file_header));
}

static int
parseLzmaHeader(const unsigned char * hdrBuf,
				struct elzma_file_header * hdr)
{
	if (lzmadec_header_properties(&(hdr->pb), &(hdr->lp), &(hdr->lc),
								  *hdrBuf) ||
		lzmadec_header_dictionary(&(hdr->dictSize), hdrBuf + 1))
	{
		return 1;
	}
	lzmadec_header_uncompressed(&(hdr->uncompressedSize),
								&(hdr->isStreamed),
								hdrBuf + 5);

	return 0;
}

static int
serializeLzmaHeader(unsigned char * hdrBuf,
					const struct elzma_file_header * hdr)
{
	unsigned int i;

	memset((void *) hdrBuf, 0, ELZMA_LZMA_HEADER_SIZE);

	/* encode lc, pb, and lp */
	*hdrBuf++ = hdr->lc + (hdr->pb * 45) + (hdr->lp * 45 * 9);

	/* encode dictionary size */
	for (i = 0; i < 4; i++) {
		*(hdrBuf++) = (unsigned char) (hdr->dictSize >> (i * 8));
	}

	/* encode uncompressed size */
	for (i = 0; i < 8; i++) {
		if (hdr->isStreamed) {
			*(hdrBuf++) = 0xff;
		} else {
			*(hdrBuf++) = (unsigned char) (hdr->uncompressedSize >> (i * 8));
		}
	}

	return 0;
}

void
initializeLZMAFormatHandler(struct elzma_format_handler * hand)
{
	hand->header_size = ELZMA_LZMA_HEADER_SIZE;
	hand->init_header = initLzmaHeader;
	hand->parse_header = parseLzmaHeader;
	hand->serialize_header = serializeLzmaHeader;
	hand->footer_size = 0;
	hand->serialize_footer = NULL;
}

// lzma sdk
#define _7ZIP_ST

#line 3 "7zCrc.c"
#define kCrcPoly 0xEDB88320
UInt32 g_CrcTable[256];

void MY_FAST_CALL CrcGenerateTable(void)
{
  UInt32 i;
  for (i = 0; i < 256; i++)
  {
	UInt32 r = i;
	int j;
	for (j = 0; j < 8; j++)
	  r = (r >> 1) ^ (kCrcPoly & ~((r & 1) - 1));
	g_CrcTable[i] = r;
  }
}

UInt32 MY_FAST_CALL CrcUpdate(UInt32 v, const void *data, size_t size)
{
  const Byte *p = (const Byte *)data;
  for (; size > 0 ; size--, p++)
	v = CRC_UPDATE_BYTE(v, *p);
  return v;
}

UInt32 MY_FAST_CALL CrcCalc(const void *data, size_t size)
{
  return CrcUpdate(CRC_INIT_VAL, data, size) ^ 0xFFFFFFFF;
}



#line 3 "Alloc.c"
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>


#line 3 "Alloc.h"
#ifndef __COMMON_ALLOC_H
#define __COMMON_ALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *MyAlloc(size_t size);
void MyFree(void *address);

#ifdef _WIN32

void SetLargePageSize();

void *MidAlloc(size_t size);
void MidFree(void *address);
void *BigAlloc(size_t size);
void BigFree(void *address);

#else

#define MidAlloc(size) MyAlloc(size)
#define MidFree(address) MyFree(address)
#define BigAlloc(size) MyAlloc(size)
#define BigFree(address) MyFree(address)

#endif

#ifdef __cplusplus
}
#endif

#endif

/* #define _SZ_ALLOC_DEBUG */

/* use _SZ_ALLOC_DEBUG to debug alloc/free operations */
#ifdef _SZ_ALLOC_DEBUG
#include <stdio.h>
int g_allocCount = 0;
int g_allocCountMid = 0;
int g_allocCountBig = 0;
#endif

void *MyAlloc(size_t size)
{
  if (size == 0)
	return 0;
  #ifdef _SZ_ALLOC_DEBUG
  {
	void *p = malloc(size);
	fprintf(stderr, "\nAlloc %10d bytes, count = %10d,  addr = %8X", size, g_allocCount++, (unsigned)p);
	return p;
  }
  #else
  return malloc(size);
  #endif
}

void MyFree(void *address)
{
  #ifdef _SZ_ALLOC_DEBUG
  if (address != 0)
	fprintf(stderr, "\nFree; count = %10d,  addr = %8X", --g_allocCount, (unsigned)address);
  #endif
  free(address);
}

#ifdef _WIN32

void *MidAlloc(size_t size)
{
  if (size == 0)
	return 0;
  #ifdef _SZ_ALLOC_DEBUG
  fprintf(stderr, "\nAlloc_Mid %10d bytes;  count = %10d", size, g_allocCountMid++);
  #endif
  return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
}

void MidFree(void *address)
{
  #ifdef _SZ_ALLOC_DEBUG
  if (address != 0)
	fprintf(stderr, "\nFree_Mid; count = %10d", --g_allocCountMid);
  #endif
  if (address == 0)
	return;
  VirtualFree(address, 0, MEM_RELEASE);
}

#ifndef MEM_LARGE_PAGES
#undef _7ZIP_LARGE_PAGES
#endif

#ifdef _7ZIP_LARGE_PAGES
SIZE_T g_LargePageSize = 0;
typedef SIZE_T (WINAPI *GetLargePageMinimumP)();
#endif

void SetLargePageSize()
{
  #ifdef _7ZIP_LARGE_PAGES
  SIZE_T size = 0;
  GetLargePageMinimumP largePageMinimum = (GetLargePageMinimumP)
		GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetLargePageMinimum");
  if (largePageMinimum == 0)
	return;
  size = largePageMinimum();
  if (size == 0 || (size & (size - 1)) != 0)
	return;
  g_LargePageSize = size;
  #endif
}

void *BigAlloc(size_t size)
{
  if (size == 0)
	return 0;
  #ifdef _SZ_ALLOC_DEBUG
  fprintf(stderr, "\nAlloc_Big %10d bytes;  count = %10d", size, g_allocCountBig++);
  #endif

  #ifdef _7ZIP_LARGE_PAGES
  if (g_LargePageSize != 0 && g_LargePageSize <= (1 << 30) && size >= (1 << 18))
  {
	void *res = VirtualAlloc(0, (size + g_LargePageSize - 1) & (~(g_LargePageSize - 1)),
		MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
	if (res != 0)
	  return res;
  }
  #endif
  return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
}

void BigFree(void *address)
{
  #ifdef _SZ_ALLOC_DEBUG
  if (address != 0)
	fprintf(stderr, "\nFree_Big; count = %10d", --g_allocCountBig);
  #endif

  if (address == 0)
	return;
  VirtualFree(address, 0, MEM_RELEASE);
}

#endif


#line 3 "Bra.c"

#line 3 "Bra.h"
#ifndef __BRA_H
#define __BRA_H

#ifdef __cplusplus
extern "C" {
#endif

/*
These functions convert relative addresses to absolute addresses
in CALL instructions to increase the compression ratio.

  In:
	data     - data buffer
	size     - size of data
	ip       - current virtual Instruction Pinter (IP) value
	state    - state variable for x86 converter
	encoding - 0 (for decoding), 1 (for encoding)

  Out:
	state    - state variable for x86 converter

  Returns:
	The number of processed bytes. If you call these functions with multiple calls,
	you must start next call with first byte after block of processed bytes.

  Type   Endian  Alignment  LookAhead

  x86    little      1          4
  ARMT   little      2          2
  ARM    little      4          0
  PPC     big        4          0
  SPARC   big        4          0
  IA64   little     16          0

  size must be >= Alignment + LookAhead, if it's not last block.
  If (size < Alignment + LookAhead), converter returns 0.

  Example:

	UInt32 ip = 0;
	for ()
	{
	  ; size must be >= Alignment + LookAhead, if it's not last block
	  SizeT processed = Convert(data, size, ip, 1);
	  data += processed;
	  size -= processed;
	  ip += processed;
	}
*/

#define x86_Convert_Init(state) { state = 0; }
SizeT x86_Convert(Byte *data, SizeT size, UInt32 ip, UInt32 *state, int encoding);
SizeT ARM_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT ARMT_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT PPC_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT SPARC_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT IA64_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);

#ifdef __cplusplus
}
#endif

#endif

SizeT ARM_Convert(Byte *data, SizeT size, UInt32 ip, int encoding)
{
  SizeT i;
  if (size < 4)
	return 0;
  size -= 4;
  ip += 8;
  for (i = 0; i <= size; i += 4)
  {
	if (data[i + 3] == 0xEB)
	{
	  UInt32 dest;
	  UInt32 src = ((UInt32)data[i + 2] << 16) | ((UInt32)data[i + 1] << 8) | (data[i + 0]);
	  src <<= 2;
	  if (encoding)
		dest = ip + (UInt32)i + src;
	  else
		dest = src - (ip + (UInt32)i);
	  dest >>= 2;
	  data[i + 2] = (Byte)(dest >> 16);
	  data[i + 1] = (Byte)(dest >> 8);
	  data[i + 0] = (Byte)dest;
	}
  }
  return i;
}

SizeT ARMT_Convert(Byte *data, SizeT size, UInt32 ip, int encoding)
{
  SizeT i;
  if (size < 4)
	return 0;
  size -= 4;
  ip += 4;
  for (i = 0; i <= size; i += 2)
  {
	if ((data[i + 1] & 0xF8) == 0xF0 &&
		(data[i + 3] & 0xF8) == 0xF8)
	{
	  UInt32 dest;
	  UInt32 src =
		(((UInt32)data[i + 1] & 0x7) << 19) |
		((UInt32)data[i + 0] << 11) |
		(((UInt32)data[i + 3] & 0x7) << 8) |
		(data[i + 2]);

	  src <<= 1;
	  if (encoding)
		dest = ip + (UInt32)i + src;
	  else
		dest = src - (ip + (UInt32)i);
	  dest >>= 1;

	  data[i + 1] = (Byte)(0xF0 | ((dest >> 19) & 0x7));
	  data[i + 0] = (Byte)(dest >> 11);
	  data[i + 3] = (Byte)(0xF8 | ((dest >> 8) & 0x7));
	  data[i + 2] = (Byte)dest;
	  i += 2;
	}
  }
  return i;
}

SizeT PPC_Convert(Byte *data, SizeT size, UInt32 ip, int encoding)
{
  SizeT i;
  if (size < 4)
	return 0;
  size -= 4;
  for (i = 0; i <= size; i += 4)
  {
	if ((data[i] >> 2) == 0x12 && (data[i + 3] & 3) == 1)
	{
	  UInt32 src = ((UInt32)(data[i + 0] & 3) << 24) |
		((UInt32)data[i + 1] << 16) |
		((UInt32)data[i + 2] << 8) |
		((UInt32)data[i + 3] & (~3));

	  UInt32 dest;
	  if (encoding)
		dest = ip + (UInt32)i + src;
	  else
		dest = src - (ip + (UInt32)i);
	  data[i + 0] = (Byte)(0x48 | ((dest >> 24) &  0x3));
	  data[i + 1] = (Byte)(dest >> 16);
	  data[i + 2] = (Byte)(dest >> 8);
	  data[i + 3] &= 0x3;
	  data[i + 3] |= dest;
	}
  }
  return i;
}

SizeT SPARC_Convert(Byte *data, SizeT size, UInt32 ip, int encoding)
{
  UInt32 i;
  if (size < 4)
	return 0;
  size -= 4;
  for (i = 0; i <= size; i += 4)
  {
	if ((data[i] == 0x40 && (data[i + 1] & 0xC0) == 0x00) ||
		(data[i] == 0x7F && (data[i + 1] & 0xC0) == 0xC0))
	{
	  UInt32 src =
		((UInt32)data[i + 0] << 24) |
		((UInt32)data[i + 1] << 16) |
		((UInt32)data[i + 2] << 8) |
		((UInt32)data[i + 3]);
	  UInt32 dest;

	  src <<= 2;
	  if (encoding)
		dest = ip + i + src;
	  else
		dest = src - (ip + i);
	  dest >>= 2;

	  dest = (((0 - ((dest >> 22) & 1)) << 22) & 0x3FFFFFFF) | (dest & 0x3FFFFF) | 0x40000000;

	  data[i + 0] = (Byte)(dest >> 24);
	  data[i + 1] = (Byte)(dest >> 16);
	  data[i + 2] = (Byte)(dest >> 8);
	  data[i + 3] = (Byte)dest;
	}
  }
  return i;
}


#line 3 "Bra86.c"
#define Test86MSByte(b) ((b) == 0 || (b) == 0xFF)

const Byte kMaskToAllowedStatus[8] = {1, 1, 1, 0, 1, 0, 0, 0};
const Byte kMaskToBitNumber[8] = {0, 1, 2, 2, 3, 3, 3, 3};

SizeT x86_Convert(Byte *data, SizeT size, UInt32 ip, UInt32 *state, int encoding)
{
  SizeT bufferPos = 0, prevPosT;
  UInt32 prevMask = *state & 0x7;
  if (size < 5)
	return 0;
  ip += 5;
  prevPosT = (SizeT)0 - 1;

  for (;;)
  {
	Byte *p = data + bufferPos;
	Byte *limit = data + size - 4;
	for (; p < limit; p++)
	  if ((*p & 0xFE) == 0xE8)
		break;
	bufferPos = (SizeT)(p - data);
	if (p >= limit)
	  break;
	prevPosT = bufferPos - prevPosT;
	if (prevPosT > 3)
	  prevMask = 0;
	else
	{
	  prevMask = (prevMask << ((int)prevPosT - 1)) & 0x7;
	  if (prevMask != 0)
	  {
		Byte b = p[4 - kMaskToBitNumber[prevMask]];
		if (!kMaskToAllowedStatus[prevMask] || Test86MSByte(b))
		{
		  prevPosT = bufferPos;
		  prevMask = ((prevMask << 1) & 0x7) | 1;
		  bufferPos++;
		  continue;
		}
	  }
	}
	prevPosT = bufferPos;

	if (Test86MSByte(p[4]))
	{
	  UInt32 src = ((UInt32)p[4] << 24) | ((UInt32)p[3] << 16) | ((UInt32)p[2] << 8) | ((UInt32)p[1]);
	  UInt32 dest;
	  for (;;)
	  {
		Byte b;
		int index;
		if (encoding)
		  dest = (ip + (UInt32)bufferPos) + src;
		else
		  dest = src - (ip + (UInt32)bufferPos);
		if (prevMask == 0)
		  break;
		index = kMaskToBitNumber[prevMask] * 8;
		b = (Byte)(dest >> (24 - index));
		if (!Test86MSByte(b))
		  break;
		src = dest ^ ((1 << (32 - index)) - 1);
	  }
	  p[4] = (Byte)(~(((dest >> 24) & 1) - 1));
	  p[3] = (Byte)(dest >> 16);
	  p[2] = (Byte)(dest >> 8);
	  p[1] = (Byte)dest;
	  bufferPos += 5;
	}
	else
	{
	  prevMask = ((prevMask << 1) & 0x7) | 1;
	  bufferPos++;
	}
  }
  prevPosT = bufferPos - prevPosT;
  *state = ((prevPosT > 3) ? 0 : ((prevMask << ((int)prevPosT - 1)) & 0x7));
  return bufferPos;
}


#line 3 "BraIA64.c"
static const Byte kBranchTable[32] =
{
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  4, 4, 6, 6, 0, 0, 7, 7,
  4, 4, 0, 0, 4, 4, 0, 0
};

SizeT IA64_Convert(Byte *data, SizeT size, UInt32 ip, int encoding)
{
  SizeT i;
  if (size < 16)
	return 0;
  size -= 16;
  for (i = 0; i <= size; i += 16)
  {
	UInt32 instrTemplate = data[i] & 0x1F;
	UInt32 mask = kBranchTable[instrTemplate];
	UInt32 bitPos = 5;
	int slot;
	for (slot = 0; slot < 3; slot++, bitPos += 41)
	{
	  UInt32 bytePos, bitRes;
	  UInt64 instruction, instNorm;
	  int j;
	  if (((mask >> slot) & 1) == 0)
		continue;
	  bytePos = (bitPos >> 3);
	  bitRes = bitPos & 0x7;
	  instruction = 0;
	  for (j = 0; j < 6; j++)
		instruction += (UInt64)data[i + j + bytePos] << (8 * j);

	  instNorm = instruction >> bitRes;
	  if (((instNorm >> 37) & 0xF) == 0x5 && ((instNorm >> 9) & 0x7) == 0)
	  {
		UInt32 src = (UInt32)((instNorm >> 13) & 0xFFFFF);
		UInt32 dest;
		src |= ((UInt32)(instNorm >> 36) & 1) << 20;

		src <<= 4;

		if (encoding)
		  dest = ip + (UInt32)i + src;
		else
		  dest = src - (ip + (UInt32)i);

		dest >>= 4;

		instNorm &= ~((UInt64)(0x8FFFFF) << 13);
		instNorm |= ((UInt64)(dest & 0xFFFFF) << 13);
		instNorm |= ((UInt64)(dest & 0x100000) << (36 - 20));

		instruction &= (1 << bitRes) - 1;
		instruction |= (instNorm << bitRes);
		for (j = 0; j < 6; j++)
		  data[i + j + bytePos] = (Byte)(instruction >> (8 * j));
	  }
	}
  }
  return i;
}


#line 3 "LzFind.c"
#include <string.h>


#line 3 "LzFind.h"
#ifndef __LZ_FIND_H
#define __LZ_FIND_H

#ifdef __cplusplus
extern "C" {
#endif

typedef UInt32 CLzRef;

typedef struct _CMatchFinder
{
  Byte *buffer;
  UInt32 pos;
  UInt32 posLimit;
  UInt32 streamPos;
  UInt32 lenLimit;

  UInt32 cyclicBufferPos;
  UInt32 cyclicBufferSize; /* it must be = (historySize + 1) */

  UInt32 matchMaxLen;
  CLzRef *hash;
  CLzRef *son;
  UInt32 hashMask;
  UInt32 cutValue;

  Byte *bufferBase;
  ISeqInStream *stream;
  int streamEndWasReached;

  UInt32 blockSize;
  UInt32 keepSizeBefore;
  UInt32 keepSizeAfter;

  UInt32 numHashBytes;
  int directInput;
  size_t directInputRem;
  int btMode;
  int bigHash;
  UInt32 historySize;
  UInt32 fixedHashSize;
  UInt32 hashSizeSum;
  UInt32 numSons;
  SRes result;
  UInt32 crc[256];
} CMatchFinder;

#define Inline_MatchFinder_GetPointerToCurrentPos(p) ((p)->buffer)
#define Inline_MatchFinder_GetIndexByte(p, index) ((p)->buffer[(Int32)(index)])

#define Inline_MatchFinder_GetNumAvailableBytes(p) ((p)->streamPos - (p)->pos)

int MatchFinder_NeedMove(CMatchFinder *p);
Byte *MatchFinder_GetPointerToCurrentPos(CMatchFinder *p);
void MatchFinder_MoveBlock(CMatchFinder *p);
void MatchFinder_ReadIfRequired(CMatchFinder *p);

void MatchFinder_Construct(CMatchFinder *p);

/* Conditions:
	 historySize <= 3 GB
	 keepAddBufferBefore + matchMaxLen + keepAddBufferAfter < 511MB
*/
int MatchFinder_Create(CMatchFinder *p, UInt32 historySize,
	UInt32 keepAddBufferBefore, UInt32 matchMaxLen, UInt32 keepAddBufferAfter,
	ISzAlloc *alloc);
void MatchFinder_Free(CMatchFinder *p, ISzAlloc *alloc);
void MatchFinder_Normalize3(UInt32 subValue, CLzRef *items, UInt32 numItems);
void MatchFinder_ReduceOffsets(CMatchFinder *p, UInt32 subValue);

UInt32 * GetMatchesSpec1(UInt32 lenLimit, UInt32 curMatch, UInt32 pos, const Byte *buffer, CLzRef *son,
	UInt32 _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 _cutValue,
	UInt32 *distances, UInt32 maxLen);

/*
Conditions:
  Mf_GetNumAvailableBytes_Func must be called before each Mf_GetMatchLen_Func.
  Mf_GetPointerToCurrentPos_Func's result must be used only before any other function
*/

typedef void (*Mf_Init_Func)(void *object);
typedef Byte (*Mf_GetIndexByte_Func)(void *object, Int32 index);
typedef UInt32 (*Mf_GetNumAvailableBytes_Func)(void *object);
typedef const Byte * (*Mf_GetPointerToCurrentPos_Func)(void *object);
typedef UInt32 (*Mf_GetMatches_Func)(void *object, UInt32 *distances);
typedef void (*Mf_Skip_Func)(void *object, UInt32);

typedef struct _IMatchFinder
{
  Mf_Init_Func Init;
  Mf_GetIndexByte_Func GetIndexByte;
  Mf_GetNumAvailableBytes_Func GetNumAvailableBytes;
  Mf_GetPointerToCurrentPos_Func GetPointerToCurrentPos;
  Mf_GetMatches_Func GetMatches;
  Mf_Skip_Func Skip;
} IMatchFinder;

void MatchFinder_CreateVTable(CMatchFinder *p, IMatchFinder *vTable);

void MatchFinder_Init(CMatchFinder *p);
UInt32 Bt3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances);
UInt32 Hc3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances);
void Bt3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num);
void Hc3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num);

#ifdef __cplusplus
}
#endif

#endif


#line 3 "LzHash.h"
#ifndef __LZ_HASH_H
#define __LZ_HASH_H

#define kHash2Size (1 << 10)
#define kHash3Size (1 << 16)
#define kHash4Size (1 << 20)

#define kFix3HashSize (kHash2Size)
#define kFix4HashSize (kHash2Size + kHash3Size)
#define kFix5HashSize (kHash2Size + kHash3Size + kHash4Size)

#define HASH2_CALC hashValue = cur[0] | ((UInt32)cur[1] << 8);

#define HASH3_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  hash2Value = temp & (kHash2Size - 1); \
  hashValue = (temp ^ ((UInt32)cur[2] << 8)) & p->hashMask; }

#define HASH4_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  hash2Value = temp & (kHash2Size - 1); \
  hash3Value = (temp ^ ((UInt32)cur[2] << 8)) & (kHash3Size - 1); \
  hashValue = (temp ^ ((UInt32)cur[2] << 8) ^ (p->crc[cur[3]] << 5)) & p->hashMask; }

#define HASH5_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  hash2Value = temp & (kHash2Size - 1); \
  hash3Value = (temp ^ ((UInt32)cur[2] << 8)) & (kHash3Size - 1); \
  hash4Value = (temp ^ ((UInt32)cur[2] << 8) ^ (p->crc[cur[3]] << 5)); \
  hashValue = (hash4Value ^ (p->crc[cur[4]] << 3)) & p->hashMask; \
  hash4Value &= (kHash4Size - 1); }

/* #define HASH_ZIP_CALC hashValue = ((cur[0] | ((UInt32)cur[1] << 8)) ^ p->crc[cur[2]]) & 0xFFFF; */
#define HASH_ZIP_CALC hashValue = ((cur[2] | ((UInt32)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;

#define MT_HASH2_CALC \
  hash2Value = (p->crc[cur[0]] ^ cur[1]) & (kHash2Size - 1);

#define MT_HASH3_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  hash2Value = temp & (kHash2Size - 1); \
  hash3Value = (temp ^ ((UInt32)cur[2] << 8)) & (kHash3Size - 1); }

#define MT_HASH4_CALC { \
  UInt32 temp = p->crc[cur[0]] ^ cur[1]; \
  hash2Value = temp & (kHash2Size - 1); \
  hash3Value = (temp ^ ((UInt32)cur[2] << 8)) & (kHash3Size - 1); \
  hash4Value = (temp ^ ((UInt32)cur[2] << 8) ^ (p->crc[cur[3]] << 5)) & (kHash4Size - 1); }

#endif

#define kEmptyHashValue 0
#define kMaxValForNormalize ((UInt32)0xFFFFFFFF)
#define kNormalizeStepMin (1 << 10) /* it must be power of 2 */
#define kNormalizeMask (~(kNormalizeStepMin - 1))
#define kMaxHistorySize ((UInt32)3 << 30)

#define kStartMaxLen 3

static void LzInWindow_Free(CMatchFinder *p, ISzAlloc *alloc)
{
  if (!p->directInput)
  {
	alloc->Free(alloc, p->bufferBase);
	p->bufferBase = 0;
  }
}

/* keepSizeBefore + keepSizeAfter + keepSizeReserv must be < 4G) */

static int LzInWindow_Create(CMatchFinder *p, UInt32 keepSizeReserv, ISzAlloc *alloc)
{
  UInt32 blockSize = p->keepSizeBefore + p->keepSizeAfter + keepSizeReserv;
  if (p->directInput)
  {
	p->blockSize = blockSize;
	return 1;
  }
  if (p->bufferBase == 0 || p->blockSize != blockSize)
  {
	LzInWindow_Free(p, alloc);
	p->blockSize = blockSize;
	p->bufferBase = (Byte *)alloc->Alloc(alloc, (size_t)blockSize);
  }
  return (p->bufferBase != 0);
}

Byte *MatchFinder_GetPointerToCurrentPos(CMatchFinder *p) { return p->buffer; }
Byte MatchFinder_GetIndexByte(CMatchFinder *p, Int32 index) { return p->buffer[index]; }

UInt32 MatchFinder_GetNumAvailableBytes(CMatchFinder *p) { return p->streamPos - p->pos; }

void MatchFinder_ReduceOffsets(CMatchFinder *p, UInt32 subValue)
{
  p->posLimit -= subValue;
  p->pos -= subValue;
  p->streamPos -= subValue;
}

static void MatchFinder_ReadBlock(CMatchFinder *p)
{
  if (p->streamEndWasReached || p->result != SZ_OK)
	return;
  if (p->directInput)
  {
	UInt32 curSize = 0xFFFFFFFF - p->streamPos;
	if (curSize > p->directInputRem)
	  curSize = (UInt32)p->directInputRem;
	p->directInputRem -= curSize;
	p->streamPos += curSize;
	if (p->directInputRem == 0)
	  p->streamEndWasReached = 1;
	return;
  }
  for (;;)
  {
	Byte *dest = p->buffer + (p->streamPos - p->pos);
	size_t size = (p->bufferBase + p->blockSize - dest);
	if (size == 0)
	  return;
	p->result = p->stream->Read(p->stream, dest, &size);
	if (p->result != SZ_OK)
	  return;
	if (size == 0)
	{
	  p->streamEndWasReached = 1;
	  return;
	}
	p->streamPos += (UInt32)size;
	if (p->streamPos - p->pos > p->keepSizeAfter)
	  return;
  }
}

void MatchFinder_MoveBlock(CMatchFinder *p)
{
  memmove(p->bufferBase,
	p->buffer - p->keepSizeBefore,
	(size_t)(p->streamPos - p->pos + p->keepSizeBefore));
  p->buffer = p->bufferBase + p->keepSizeBefore;
}

int MatchFinder_NeedMove(CMatchFinder *p)
{
  if (p->directInput)
	return 0;
  /* if (p->streamEndWasReached) return 0; */
  return ((size_t)(p->bufferBase + p->blockSize - p->buffer) <= p->keepSizeAfter);
}

void MatchFinder_ReadIfRequired(CMatchFinder *p)
{
  if (p->streamEndWasReached)
	return;
  if (p->keepSizeAfter >= p->streamPos - p->pos)
	MatchFinder_ReadBlock(p);
}

static void MatchFinder_CheckAndMoveAndRead(CMatchFinder *p)
{
  if (MatchFinder_NeedMove(p))
	MatchFinder_MoveBlock(p);
  MatchFinder_ReadBlock(p);
}

static void MatchFinder_SetDefaultSettings(CMatchFinder *p)
{
  p->cutValue = 32;
  p->btMode = 1;
  p->numHashBytes = 4;
  p->bigHash = 0;
}

#define kCrcPoly 0xEDB88320

void MatchFinder_Construct(CMatchFinder *p)
{
  UInt32 i;
  p->bufferBase = 0;
  p->directInput = 0;
  p->hash = 0;
  MatchFinder_SetDefaultSettings(p);

  for (i = 0; i < 256; i++)
  {
	UInt32 r = i;
	int j;
	for (j = 0; j < 8; j++)
	  r = (r >> 1) ^ (kCrcPoly & ~((r & 1) - 1));
	p->crc[i] = r;
  }
}

static void MatchFinder_FreeThisClassMemory(CMatchFinder *p, ISzAlloc *alloc)
{
  alloc->Free(alloc, p->hash);
  p->hash = 0;
}

void MatchFinder_Free(CMatchFinder *p, ISzAlloc *alloc)
{
  MatchFinder_FreeThisClassMemory(p, alloc);
  LzInWindow_Free(p, alloc);
}

static CLzRef* AllocRefs(UInt32 num, ISzAlloc *alloc)
{
  size_t sizeInBytes = (size_t)num * sizeof(CLzRef);
  if (sizeInBytes / sizeof(CLzRef) != num)
	return 0;
  return (CLzRef *)alloc->Alloc(alloc, sizeInBytes);
}

int MatchFinder_Create(CMatchFinder *p, UInt32 historySize,
	UInt32 keepAddBufferBefore, UInt32 matchMaxLen, UInt32 keepAddBufferAfter,
	ISzAlloc *alloc)
{
  UInt32 sizeReserv;
  if (historySize > kMaxHistorySize)
  {
	MatchFinder_Free(p, alloc);
	return 0;
  }
  sizeReserv = historySize >> 1;
  if (historySize > ((UInt32)2 << 30))
	sizeReserv = historySize >> 2;
  sizeReserv += (keepAddBufferBefore + matchMaxLen + keepAddBufferAfter) / 2 + (1 << 19);

  p->keepSizeBefore = historySize + keepAddBufferBefore + 1;
  p->keepSizeAfter = matchMaxLen + keepAddBufferAfter;
  /* we need one additional byte, since we use MoveBlock after pos++ and before dictionary using */
  if (LzInWindow_Create(p, sizeReserv, alloc))
  {
	UInt32 newCyclicBufferSize = historySize + 1;
	UInt32 hs;
	p->matchMaxLen = matchMaxLen;
	{
	  p->fixedHashSize = 0;
	  if (p->numHashBytes == 2)
		hs = (1 << 16) - 1;
	  else
	  {
		hs = historySize - 1;
		hs |= (hs >> 1);
		hs |= (hs >> 2);
		hs |= (hs >> 4);
		hs |= (hs >> 8);
		hs >>= 1;
		hs |= 0xFFFF; /* don't change it! It's required for Deflate */
		if (hs > (1 << 24))
		{
		  if (p->numHashBytes == 3)
			hs = (1 << 24) - 1;
		  else
			hs >>= 1;
		}
	  }
	  p->hashMask = hs;
	  hs++;
	  if (p->numHashBytes > 2) p->fixedHashSize += kHash2Size;
	  if (p->numHashBytes > 3) p->fixedHashSize += kHash3Size;
	  if (p->numHashBytes > 4) p->fixedHashSize += kHash4Size;
	  hs += p->fixedHashSize;
	}

	{
	  UInt32 prevSize = p->hashSizeSum + p->numSons;
	  UInt32 newSize;
	  p->historySize = historySize;
	  p->hashSizeSum = hs;
	  p->cyclicBufferSize = newCyclicBufferSize;
	  p->numSons = (p->btMode ? newCyclicBufferSize * 2 : newCyclicBufferSize);
	  newSize = p->hashSizeSum + p->numSons;
	  if (p->hash != 0 && prevSize == newSize)
		return 1;
	  MatchFinder_FreeThisClassMemory(p, alloc);
	  p->hash = AllocRefs(newSize, alloc);
	  if (p->hash != 0)
	  {
		p->son = p->hash + p->hashSizeSum;
		return 1;
	  }
	}
  }
  MatchFinder_Free(p, alloc);
  return 0;
}

static void MatchFinder_SetLimits(CMatchFinder *p)
{
  UInt32 limit = kMaxValForNormalize - p->pos;
  UInt32 limit2 = p->cyclicBufferSize - p->cyclicBufferPos;
  if (limit2 < limit)
	limit = limit2;
  limit2 = p->streamPos - p->pos;
  if (limit2 <= p->keepSizeAfter)
  {
	if (limit2 > 0)
	  limit2 = 1;
  }
  else
	limit2 -= p->keepSizeAfter;
  if (limit2 < limit)
	limit = limit2;
  {
	UInt32 lenLimit = p->streamPos - p->pos;
	if (lenLimit > p->matchMaxLen)
	  lenLimit = p->matchMaxLen;
	p->lenLimit = lenLimit;
  }
  p->posLimit = p->pos + limit;
}

void MatchFinder_Init(CMatchFinder *p)
{
  UInt32 i;
  for (i = 0; i < p->hashSizeSum; i++)
	p->hash[i] = kEmptyHashValue;
  p->cyclicBufferPos = 0;
  p->buffer = p->bufferBase;
  p->pos = p->streamPos = p->cyclicBufferSize;
  p->result = SZ_OK;
  p->streamEndWasReached = 0;
  MatchFinder_ReadBlock(p);
  MatchFinder_SetLimits(p);
}

static UInt32 MatchFinder_GetSubValue(CMatchFinder *p)
{
  return (p->pos - p->historySize - 1) & kNormalizeMask;
}

void MatchFinder_Normalize3(UInt32 subValue, CLzRef *items, UInt32 numItems)
{
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
	UInt32 value = items[i];
	if (value <= subValue)
	  value = kEmptyHashValue;
	else
	  value -= subValue;
	items[i] = value;
  }
}

static void MatchFinder_Normalize(CMatchFinder *p)
{
  UInt32 subValue = MatchFinder_GetSubValue(p);
  MatchFinder_Normalize3(subValue, p->hash, p->hashSizeSum + p->numSons);
  MatchFinder_ReduceOffsets(p, subValue);
}

static void MatchFinder_CheckLimits(CMatchFinder *p)
{
  if (p->pos == kMaxValForNormalize)
	MatchFinder_Normalize(p);
  if (!p->streamEndWasReached && p->keepSizeAfter == p->streamPos - p->pos)
	MatchFinder_CheckAndMoveAndRead(p);
  if (p->cyclicBufferPos == p->cyclicBufferSize)
	p->cyclicBufferPos = 0;
  MatchFinder_SetLimits(p);
}

static UInt32 * Hc_GetMatchesSpec(UInt32 lenLimit, UInt32 curMatch, UInt32 pos, const Byte *cur, CLzRef *son,
	UInt32 _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 cutValue,
	UInt32 *distances, UInt32 maxLen)
{
  son[_cyclicBufferPos] = curMatch;
  for (;;)
  {
	UInt32 delta = pos - curMatch;
	if (cutValue-- == 0 || delta >= _cyclicBufferSize)
	  return distances;
	{
	  const Byte *pb = cur - delta;
	  curMatch = son[_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)];
	  if (pb[maxLen] == cur[maxLen] && *pb == *cur)
	  {
		UInt32 len = 0;
		while (++len != lenLimit)
		  if (pb[len] != cur[len])
			break;
		if (maxLen < len)
		{
		  *distances++ = maxLen = len;
		  *distances++ = delta - 1;
		  if (len == lenLimit)
			return distances;
		}
	  }
	}
  }
}

UInt32 * GetMatchesSpec1(UInt32 lenLimit, UInt32 curMatch, UInt32 pos, const Byte *cur, CLzRef *son,
	UInt32 _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 cutValue,
	UInt32 *distances, UInt32 maxLen)
{
  CLzRef *ptr0 = son + (_cyclicBufferPos << 1) + 1;
  CLzRef *ptr1 = son + (_cyclicBufferPos << 1);
  UInt32 len0 = 0, len1 = 0;
  for (;;)
  {
	UInt32 delta = pos - curMatch;
	if (cutValue-- == 0 || delta >= _cyclicBufferSize)
	{
	  *ptr0 = *ptr1 = kEmptyHashValue;
	  return distances;
	}
	{
	  CLzRef *pair = son + ((_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)) << 1);
	  const Byte *pb = cur - delta;
	  UInt32 len = (len0 < len1 ? len0 : len1);
	  if (pb[len] == cur[len])
	  {
		if (++len != lenLimit && pb[len] == cur[len])
		  while (++len != lenLimit)
			if (pb[len] != cur[len])
			  break;
		if (maxLen < len)
		{
		  *distances++ = maxLen = len;
		  *distances++ = delta - 1;
		  if (len == lenLimit)
		  {
			*ptr1 = pair[0];
			*ptr0 = pair[1];
			return distances;
		  }
		}
	  }
	  if (pb[len] < cur[len])
	  {
		*ptr1 = curMatch;
		ptr1 = pair + 1;
		curMatch = *ptr1;
		len1 = len;
	  }
	  else
	  {
		*ptr0 = curMatch;
		ptr0 = pair;
		curMatch = *ptr0;
		len0 = len;
	  }
	}
  }
}

static void SkipMatchesSpec(UInt32 lenLimit, UInt32 curMatch, UInt32 pos, const Byte *cur, CLzRef *son,
	UInt32 _cyclicBufferPos, UInt32 _cyclicBufferSize, UInt32 cutValue)
{
  CLzRef *ptr0 = son + (_cyclicBufferPos << 1) + 1;
  CLzRef *ptr1 = son + (_cyclicBufferPos << 1);
  UInt32 len0 = 0, len1 = 0;
  for (;;)
  {
	UInt32 delta = pos - curMatch;
	if (cutValue-- == 0 || delta >= _cyclicBufferSize)
	{
	  *ptr0 = *ptr1 = kEmptyHashValue;
	  return;
	}
	{
	  CLzRef *pair = son + ((_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)) << 1);
	  const Byte *pb = cur - delta;
	  UInt32 len = (len0 < len1 ? len0 : len1);
	  if (pb[len] == cur[len])
	  {
		while (++len != lenLimit)
		  if (pb[len] != cur[len])
			break;
		{
		  if (len == lenLimit)
		  {
			*ptr1 = pair[0];
			*ptr0 = pair[1];
			return;
		  }
		}
	  }
	  if (pb[len] < cur[len])
	  {
		*ptr1 = curMatch;
		ptr1 = pair + 1;
		curMatch = *ptr1;
		len1 = len;
	  }
	  else
	  {
		*ptr0 = curMatch;
		ptr0 = pair;
		curMatch = *ptr0;
		len0 = len;
	  }
	}
  }
}

#define MOVE_POS \
  ++p->cyclicBufferPos; \
  p->buffer++; \
  if (++p->pos == p->posLimit) MatchFinder_CheckLimits(p);

#define MOVE_POS_RET MOVE_POS return offset;

static void MatchFinder_MovePos(CMatchFinder *p) { MOVE_POS; }

#define GET_MATCHES_HEADER2(minLen, ret_op) \
  UInt32 lenLimit; UInt32 hashValue; const Byte *cur; UInt32 curMatch; \
  lenLimit = p->lenLimit; { if (lenLimit < minLen) { MatchFinder_MovePos(p); ret_op; }} \
  cur = p->buffer;

#define GET_MATCHES_HEADER(minLen) GET_MATCHES_HEADER2(minLen, return 0)
#define SKIP_HEADER(minLen)        GET_MATCHES_HEADER2(minLen, continue)

#define MF_PARAMS(p) p->pos, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, p->cutValue

#define GET_MATCHES_FOOTER(offset, maxLen) \
  offset = (UInt32)(GetMatchesSpec1(lenLimit, curMatch, MF_PARAMS(p), \
  distances + offset, maxLen) - distances); MOVE_POS_RET;

#define SKIP_FOOTER \
  SkipMatchesSpec(lenLimit, curMatch, MF_PARAMS(p)); MOVE_POS;

static UInt32 Bt2_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  UInt32 offset;
  GET_MATCHES_HEADER(2)
  HASH2_CALC;
  curMatch = p->hash[hashValue];
  p->hash[hashValue] = p->pos;
  offset = 0;
  GET_MATCHES_FOOTER(offset, 1)
}

UInt32 Bt3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  UInt32 offset;
  GET_MATCHES_HEADER(3)
  HASH_ZIP_CALC;
  curMatch = p->hash[hashValue];
  p->hash[hashValue] = p->pos;
  offset = 0;
  GET_MATCHES_FOOTER(offset, 2)
}

static UInt32 Bt3_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  UInt32 hash2Value, delta2, maxLen, offset;
  GET_MATCHES_HEADER(3)

  HASH3_CALC;

  delta2 = p->pos - p->hash[hash2Value];
  curMatch = p->hash[kFix3HashSize + hashValue];

  p->hash[hash2Value] =
  p->hash[kFix3HashSize + hashValue] = p->pos;

  maxLen = 2;
  offset = 0;
  if (delta2 < p->cyclicBufferSize && *(cur - delta2) == *cur)
  {
	for (; maxLen != lenLimit; maxLen++)
	  if (cur[(ptrdiff_t)maxLen - delta2] != cur[maxLen])
		break;
	distances[0] = maxLen;
	distances[1] = delta2 - 1;
	offset = 2;
	if (maxLen == lenLimit)
	{
	  SkipMatchesSpec(lenLimit, curMatch, MF_PARAMS(p));
	  MOVE_POS_RET;
	}
  }
  GET_MATCHES_FOOTER(offset, maxLen)
}

static UInt32 Bt4_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  UInt32 hash2Value, hash3Value, delta2, delta3, maxLen, offset;
  GET_MATCHES_HEADER(4)

  HASH4_CALC;

  delta2 = p->pos - p->hash[                hash2Value];
  delta3 = p->pos - p->hash[kFix3HashSize + hash3Value];
  curMatch = p->hash[kFix4HashSize + hashValue];

  p->hash[                hash2Value] =
  p->hash[kFix3HashSize + hash3Value] =
  p->hash[kFix4HashSize + hashValue] = p->pos;

  maxLen = 1;
  offset = 0;
  if (delta2 < p->cyclicBufferSize && *(cur - delta2) == *cur)
  {
	distances[0] = maxLen = 2;
	distances[1] = delta2 - 1;
	offset = 2;
  }
  if (delta2 != delta3 && delta3 < p->cyclicBufferSize && *(cur - delta3) == *cur)
  {
	maxLen = 3;
	distances[offset + 1] = delta3 - 1;
	offset += 2;
	delta2 = delta3;
  }
  if (offset != 0)
  {
	for (; maxLen != lenLimit; maxLen++)
	  if (cur[(ptrdiff_t)maxLen - delta2] != cur[maxLen])
		break;
	distances[offset - 2] = maxLen;
	if (maxLen == lenLimit)
	{
	  SkipMatchesSpec(lenLimit, curMatch, MF_PARAMS(p));
	  MOVE_POS_RET;
	}
  }
  if (maxLen < 3)
	maxLen = 3;
  GET_MATCHES_FOOTER(offset, maxLen)
}

static UInt32 Hc4_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  UInt32 hash2Value, hash3Value, delta2, delta3, maxLen, offset;
  GET_MATCHES_HEADER(4)

  HASH4_CALC;

  delta2 = p->pos - p->hash[                hash2Value];
  delta3 = p->pos - p->hash[kFix3HashSize + hash3Value];
  curMatch = p->hash[kFix4HashSize + hashValue];

  p->hash[                hash2Value] =
  p->hash[kFix3HashSize + hash3Value] =
  p->hash[kFix4HashSize + hashValue] = p->pos;

  maxLen = 1;
  offset = 0;
  if (delta2 < p->cyclicBufferSize && *(cur - delta2) == *cur)
  {
	distances[0] = maxLen = 2;
	distances[1] = delta2 - 1;
	offset = 2;
  }
  if (delta2 != delta3 && delta3 < p->cyclicBufferSize && *(cur - delta3) == *cur)
  {
	maxLen = 3;
	distances[offset + 1] = delta3 - 1;
	offset += 2;
	delta2 = delta3;
  }
  if (offset != 0)
  {
	for (; maxLen != lenLimit; maxLen++)
	  if (cur[(ptrdiff_t)maxLen - delta2] != cur[maxLen])
		break;
	distances[offset - 2] = maxLen;
	if (maxLen == lenLimit)
	{
	  p->son[p->cyclicBufferPos] = curMatch;
	  MOVE_POS_RET;
	}
  }
  if (maxLen < 3)
	maxLen = 3;
  offset = (UInt32)(Hc_GetMatchesSpec(lenLimit, curMatch, MF_PARAMS(p),
	distances + offset, maxLen) - (distances));
  MOVE_POS_RET
}

UInt32 Hc3Zip_MatchFinder_GetMatches(CMatchFinder *p, UInt32 *distances)
{
  UInt32 offset;
  GET_MATCHES_HEADER(3)
  HASH_ZIP_CALC;
  curMatch = p->hash[hashValue];
  p->hash[hashValue] = p->pos;
  offset = (UInt32)(Hc_GetMatchesSpec(lenLimit, curMatch, MF_PARAMS(p),
	distances, 2) - (distances));
  MOVE_POS_RET
}

static void Bt2_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  do
  {
	SKIP_HEADER(2)
	HASH2_CALC;
	curMatch = p->hash[hashValue];
	p->hash[hashValue] = p->pos;
	SKIP_FOOTER
  }
  while (--num != 0);
}

void Bt3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  do
  {
	SKIP_HEADER(3)
	HASH_ZIP_CALC;
	curMatch = p->hash[hashValue];
	p->hash[hashValue] = p->pos;
	SKIP_FOOTER
  }
  while (--num != 0);
}

static void Bt3_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  do
  {
	UInt32 hash2Value;
	SKIP_HEADER(3)
	HASH3_CALC;
	curMatch = p->hash[kFix3HashSize + hashValue];
	p->hash[hash2Value] =
	p->hash[kFix3HashSize + hashValue] = p->pos;
	SKIP_FOOTER
  }
  while (--num != 0);
}

static void Bt4_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  do
  {
	UInt32 hash2Value, hash3Value;
	SKIP_HEADER(4)
	HASH4_CALC;
	curMatch = p->hash[kFix4HashSize + hashValue];
	p->hash[                hash2Value] =
	p->hash[kFix3HashSize + hash3Value] = p->pos;
	p->hash[kFix4HashSize + hashValue] = p->pos;
	SKIP_FOOTER
  }
  while (--num != 0);
}

static void Hc4_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  do
  {
	UInt32 hash2Value, hash3Value;
	SKIP_HEADER(4)
	HASH4_CALC;
	curMatch = p->hash[kFix4HashSize + hashValue];
	p->hash[                hash2Value] =
	p->hash[kFix3HashSize + hash3Value] =
	p->hash[kFix4HashSize + hashValue] = p->pos;
	p->son[p->cyclicBufferPos] = curMatch;
	MOVE_POS
  }
  while (--num != 0);
}

void Hc3Zip_MatchFinder_Skip(CMatchFinder *p, UInt32 num)
{
  do
  {
	SKIP_HEADER(3)
	HASH_ZIP_CALC;
	curMatch = p->hash[hashValue];
	p->hash[hashValue] = p->pos;
	p->son[p->cyclicBufferPos] = curMatch;
	MOVE_POS
  }
  while (--num != 0);
}

void MatchFinder_CreateVTable(CMatchFinder *p, IMatchFinder *vTable)
{
  vTable->Init = (Mf_Init_Func)MatchFinder_Init;
  vTable->GetIndexByte = (Mf_GetIndexByte_Func)MatchFinder_GetIndexByte;
  vTable->GetNumAvailableBytes = (Mf_GetNumAvailableBytes_Func)MatchFinder_GetNumAvailableBytes;
  vTable->GetPointerToCurrentPos = (Mf_GetPointerToCurrentPos_Func)MatchFinder_GetPointerToCurrentPos;
  if (!p->btMode)
  {
	vTable->GetMatches = (Mf_GetMatches_Func)Hc4_MatchFinder_GetMatches;
	vTable->Skip = (Mf_Skip_Func)Hc4_MatchFinder_Skip;
  }
  else if (p->numHashBytes == 2)
  {
	vTable->GetMatches = (Mf_GetMatches_Func)Bt2_MatchFinder_GetMatches;
	vTable->Skip = (Mf_Skip_Func)Bt2_MatchFinder_Skip;
  }
  else if (p->numHashBytes == 3)
  {
	vTable->GetMatches = (Mf_GetMatches_Func)Bt3_MatchFinder_GetMatches;
	vTable->Skip = (Mf_Skip_Func)Bt3_MatchFinder_Skip;
  }
  else
  {
	vTable->GetMatches = (Mf_GetMatches_Func)Bt4_MatchFinder_GetMatches;
	vTable->Skip = (Mf_Skip_Func)Bt4_MatchFinder_Skip;
  }
}


#line 3 "LzmaDec.c"
#include <string.h>

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_INIT_SIZE 5

#define NORMALIZE if (range < kTopValue) { range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0(p) ttt = *(p); NORMALIZE; bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1) IF_BIT_0(p) \
  { UPDATE_0(p); i = (i + i); A0; } else \
  { UPDATE_1(p); i = (i + i) + 1; A1; }
#define GET_BIT(p, i) GET_BIT2(p, i, ; , ;)

#define TREE_GET_BIT(probs, i) { GET_BIT((probs + i), i); }
#define TREE_DECODE(probs, limit, i) \
  { i = 1; do { TREE_GET_BIT(probs, i); } while (i < limit); i -= limit; }

/* #define _LZMA_SIZE_OPT */

#ifdef _LZMA_SIZE_OPT
#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)
#else
#define TREE_6_DECODE(probs, i) \
  { i = 1; \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  i -= 0x40; }
#endif

#define NORMALIZE_CHECK if (range < kTopValue) { if (buf >= bufLimit) return DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0_CHECK(p) ttt = *(p); NORMALIZE_CHECK; bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK range -= bound; code -= bound;
#define GET_BIT2_CHECK(p, i, A0, A1) IF_BIT_0_CHECK(p) \
  { UPDATE_0_CHECK; i = (i + i); A0; } else \
  { UPDATE_1_CHECK; i = (i + i) + 1; A1; }
#define GET_BIT_CHECK(p, i) GET_BIT2_CHECK(p, i, ; , ;)
#define TREE_DECODE_CHECK(probs, limit, i) \
  { i = 1; do { GET_BIT_CHECK(probs + i, i) } while (i < limit); i -= limit; }

#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenChoice 0
#define LenChoice2 (LenChoice + 1)
#define LenLow (LenChoice2 + 1)
#define LenMid (LenLow + (kNumPosStatesMax << kLenNumLowBits))
#define LenHigh (LenMid + (kNumPosStatesMax << kLenNumMidBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)

#define kNumStates 12
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)

#define IsMatch 0
#define IsRep (IsMatch + (kNumStates << kNumPosBitsMax))
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define IsRep0Long (IsRepG2 + kNumStates)
#define PosSlot (IsRep0Long + (kNumStates << kNumPosBitsMax))
#define SpecPos (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define Align (SpecPos + kNumFullDistances - kEndPosModelIndex)
#define LenCoder (Align + kAlignTableSize)
#define RepLenCoder (LenCoder + kNumLenProbs)
#define Literal (RepLenCoder + kNumLenProbs)

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768

#define LzmaProps_GetNumProbs(p) ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))

#if Literal != LZMA_BASE_SIZE
StopCompilingDueBUG
#endif

#define LZMA_DIC_MIN (1 << 12)

/* First LZMA-symbol is always decoded.
And it decodes new LZMA-symbols while (buf < bufLimit), but "buf" is without last normalization
Out:
  Result:
	SZ_OK - OK
	SZ_ERROR_DATA - Error
  p->remainLen:
	< kMatchSpecLenStart : normal remain
	= kMatchSpecLenStart : finished
	= kMatchSpecLenStart + 1 : Flush marker
	= kMatchSpecLenStart + 2 : State Init Marker
*/

static int MY_FAST_CALL LzmaDec_DecodeReal(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
  CLzmaProb *probs = p->probs;

  unsigned state = p->state;
  UInt32 rep0 = p->reps[0], rep1 = p->reps[1], rep2 = p->reps[2], rep3 = p->reps[3];
  unsigned pbMask = ((unsigned)1 << (p->prop.pb)) - 1;
  unsigned lpMask = ((unsigned)1 << (p->prop.lp)) - 1;
  unsigned lc = p->prop.lc;

  Byte *dic = p->dic;
  SizeT dicBufSize = p->dicBufSize;
  SizeT dicPos = p->dicPos;

  UInt32 processedPos = p->processedPos;
  UInt32 checkDicSize = p->checkDicSize;
  unsigned len = 0;

  const Byte *buf = p->buf;
  UInt32 range = p->range;
  UInt32 code = p->code;

  do
  {
	CLzmaProb *prob;
	UInt32 bound;
	unsigned ttt;
	unsigned posState = processedPos & pbMask;

	prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
	IF_BIT_0(prob)
	{
	  unsigned symbol;
	  UPDATE_0(prob);
	  prob = probs + Literal;
	  if (checkDicSize != 0 || processedPos != 0)
		prob += (LZMA_LIT_SIZE * (((processedPos & lpMask) << lc) +
		(dic[(dicPos == 0 ? dicBufSize : dicPos) - 1] >> (8 - lc))));

	  if (state < kNumLitStates)
	  {
		state -= (state < 4) ? state : 3;
		symbol = 1;
		do { GET_BIT(prob + symbol, symbol) } while (symbol < 0x100);
	  }
	  else
	  {
		unsigned matchByte = p->dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
		unsigned offs = 0x100;
		state -= (state < 10) ? 3 : 6;
		symbol = 1;
		do
		{
		  unsigned bit;
		  CLzmaProb *probLit;
		  matchByte <<= 1;
		  bit = (matchByte & offs);
		  probLit = prob + offs + bit + symbol;
		  GET_BIT2(probLit, symbol, offs &= ~bit, offs &= bit)
		}
		while (symbol < 0x100);
	  }
	  dic[dicPos++] = (Byte)symbol;
	  processedPos++;
	  continue;
	}
	else
	{
	  UPDATE_1(prob);
	  prob = probs + IsRep + state;
	  IF_BIT_0(prob)
	  {
		UPDATE_0(prob);
		state += kNumStates;
		prob = probs + LenCoder;
	  }
	  else
	  {
		UPDATE_1(prob);
		if (checkDicSize == 0 && processedPos == 0)
		  return SZ_ERROR_DATA;
		prob = probs + IsRepG0 + state;
		IF_BIT_0(prob)
		{
		  UPDATE_0(prob);
		  prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
		  IF_BIT_0(prob)
		  {
			UPDATE_0(prob);
			dic[dicPos] = dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
			dicPos++;
			processedPos++;
			state = state < kNumLitStates ? 9 : 11;
			continue;
		  }
		  UPDATE_1(prob);
		}
		else
		{
		  UInt32 distance;
		  UPDATE_1(prob);
		  prob = probs + IsRepG1 + state;
		  IF_BIT_0(prob)
		  {
			UPDATE_0(prob);
			distance = rep1;
		  }
		  else
		  {
			UPDATE_1(prob);
			prob = probs + IsRepG2 + state;
			IF_BIT_0(prob)
			{
			  UPDATE_0(prob);
			  distance = rep2;
			}
			else
			{
			  UPDATE_1(prob);
			  distance = rep3;
			  rep3 = rep2;
			}
			rep2 = rep1;
		  }
		  rep1 = rep0;
		  rep0 = distance;
		}
		state = state < kNumLitStates ? 8 : 11;
		prob = probs + RepLenCoder;
	  }
	  {
		unsigned limit, offset;
		CLzmaProb *probLen = prob + LenChoice;
		IF_BIT_0(probLen)
		{
		  UPDATE_0(probLen);
		  probLen = prob + LenLow + (posState << kLenNumLowBits);
		  offset = 0;
		  limit = (1 << kLenNumLowBits);
		}
		else
		{
		  UPDATE_1(probLen);
		  probLen = prob + LenChoice2;
		  IF_BIT_0(probLen)
		  {
			UPDATE_0(probLen);
			probLen = prob + LenMid + (posState << kLenNumMidBits);
			offset = kLenNumLowSymbols;
			limit = (1 << kLenNumMidBits);
		  }
		  else
		  {
			UPDATE_1(probLen);
			probLen = prob + LenHigh;
			offset = kLenNumLowSymbols + kLenNumMidSymbols;
			limit = (1 << kLenNumHighBits);
		  }
		}
		TREE_DECODE(probLen, limit, len);
		len += offset;
	  }

	  if (state >= kNumStates)
	  {
		UInt32 distance;
		prob = probs + PosSlot +
			((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
		TREE_6_DECODE(prob, distance);
		if (distance >= kStartPosModelIndex)
		{
		  unsigned posSlot = (unsigned)distance;
		  int numDirectBits = (int)(((distance >> 1) - 1));
		  distance = (2 | (distance & 1));
		  if (posSlot < kEndPosModelIndex)
		  {
			distance <<= numDirectBits;
			prob = probs + SpecPos + distance - posSlot - 1;
			{
			  UInt32 mask = 1;
			  unsigned i = 1;
			  do
			  {
				GET_BIT2(prob + i, i, ; , distance |= mask);
				mask <<= 1;
			  }
			  while (--numDirectBits != 0);
			}
		  }
		  else
		  {
			numDirectBits -= kNumAlignBits;
			do
			{
			  NORMALIZE
			  range >>= 1;

			  {
				UInt32 t;
				code -= range;
				t = (0 - ((UInt32)code >> 31)); /* (UInt32)((Int32)code >> 31) */
				distance = (distance << 1) + (t + 1);
				code += range & t;
			  }
			  /*
			  distance <<= 1;
			  if (code >= range)
			  {
				code -= range;
				distance |= 1;
			  }
			  */
			}
			while (--numDirectBits != 0);
			prob = probs + Align;
			distance <<= kNumAlignBits;
			{
			  unsigned i = 1;
			  GET_BIT2(prob + i, i, ; , distance |= 1);
			  GET_BIT2(prob + i, i, ; , distance |= 2);
			  GET_BIT2(prob + i, i, ; , distance |= 4);
			  GET_BIT2(prob + i, i, ; , distance |= 8);
			}
			if (distance == (UInt32)0xFFFFFFFF)
			{
			  len += kMatchSpecLenStart;
			  state -= kNumStates;
			  break;
			}
		  }
		}
		rep3 = rep2;
		rep2 = rep1;
		rep1 = rep0;
		rep0 = distance + 1;
		if (checkDicSize == 0)
		{
		  if (distance >= processedPos)
			return SZ_ERROR_DATA;
		}
		else if (distance >= checkDicSize)
		  return SZ_ERROR_DATA;
		state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
	  }

	  len += kMatchMinLen;

	  if (limit == dicPos)
		return SZ_ERROR_DATA;
	  {
		SizeT rem = limit - dicPos;
		unsigned curLen = ((rem < len) ? (unsigned)rem : len);
		SizeT pos = (dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0);

		processedPos += curLen;

		len -= curLen;
		if (pos + curLen <= dicBufSize)
		{
		  Byte *dest = dic + dicPos;
		  ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dicPos;
		  const Byte *lim = dest + curLen;
		  dicPos += curLen;
		  do
			*(dest) = (Byte)*(dest + src);
		  while (++dest != lim);
		}
		else
		{
		  do
		  {
			dic[dicPos++] = dic[pos];
			if (++pos == dicBufSize)
			  pos = 0;
		  }
		  while (--curLen != 0);
		}
	  }
	}
  }
  while (dicPos < limit && buf < bufLimit);
  NORMALIZE;
  p->buf = buf;
  p->range = range;
  p->code = code;
  p->remainLen = len;
  p->dicPos = dicPos;
  p->processedPos = processedPos;
  p->reps[0] = rep0;
  p->reps[1] = rep1;
  p->reps[2] = rep2;
  p->reps[3] = rep3;
  p->state = state;

  return SZ_OK;
}

static void MY_FAST_CALL LzmaDec_WriteRem(CLzmaDec *p, SizeT limit)
{
  if (p->remainLen != 0 && p->remainLen < kMatchSpecLenStart)
  {
	Byte *dic = p->dic;
	SizeT dicPos = p->dicPos;
	SizeT dicBufSize = p->dicBufSize;
	unsigned len = p->remainLen;
	UInt32 rep0 = p->reps[0];
	if (limit - dicPos < len)
	  len = (unsigned)(limit - dicPos);

	if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= len)
	  p->checkDicSize = p->prop.dicSize;

	p->processedPos += len;
	p->remainLen -= len;
	while (len != 0)
	{
	  len--;
	  dic[dicPos] = dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
	  dicPos++;
	}
	p->dicPos = dicPos;
  }
}

static int MY_FAST_CALL LzmaDec_DecodeReal2(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
  do
  {
	SizeT limit2 = limit;
	if (p->checkDicSize == 0)
	{
	  UInt32 rem = p->prop.dicSize - p->processedPos;
	  if (limit - p->dicPos > rem)
		limit2 = p->dicPos + rem;
	}
	RINOK(LzmaDec_DecodeReal(p, limit2, bufLimit));
	if (p->processedPos >= p->prop.dicSize)
	  p->checkDicSize = p->prop.dicSize;
	LzmaDec_WriteRem(p, limit);
  }
  while (p->dicPos < limit && p->buf < bufLimit && p->remainLen < kMatchSpecLenStart);

  if (p->remainLen > kMatchSpecLenStart)
  {
	p->remainLen = kMatchSpecLenStart;
  }
  return 0;
}

typedef enum
{
  DUMMY_ERROR, /* unexpected end of input stream */
  DUMMY_LIT,
  DUMMY_MATCH,
  DUMMY_REP
} ELzmaDummy;

static ELzmaDummy LzmaDec_TryDummy(const CLzmaDec *p, const Byte *buf, SizeT inSize)
{
  UInt32 range = p->range;
  UInt32 code = p->code;
  const Byte *bufLimit = buf + inSize;
  CLzmaProb *probs = p->probs;
  unsigned state = p->state;
  ELzmaDummy res;

  {
	CLzmaProb *prob;
	UInt32 bound;
	unsigned ttt;
	unsigned posState = (p->processedPos) & ((1 << p->prop.pb) - 1);

	prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
	IF_BIT_0_CHECK(prob)
	{
	  UPDATE_0_CHECK

	  /* if (bufLimit - buf >= 7) return DUMMY_LIT; */

	  prob = probs + Literal;
	  if (p->checkDicSize != 0 || p->processedPos != 0)
		prob += (LZMA_LIT_SIZE *
		  ((((p->processedPos) & ((1 << (p->prop.lp)) - 1)) << p->prop.lc) +
		  (p->dic[(p->dicPos == 0 ? p->dicBufSize : p->dicPos) - 1] >> (8 - p->prop.lc))));

	  if (state < kNumLitStates)
	  {
		unsigned symbol = 1;
		do { GET_BIT_CHECK(prob + symbol, symbol) } while (symbol < 0x100);
	  }
	  else
	  {
		unsigned matchByte = p->dic[p->dicPos - p->reps[0] +
			((p->dicPos < p->reps[0]) ? p->dicBufSize : 0)];
		unsigned offs = 0x100;
		unsigned symbol = 1;
		do
		{
		  unsigned bit;
		  CLzmaProb *probLit;
		  matchByte <<= 1;
		  bit = (matchByte & offs);
		  probLit = prob + offs + bit + symbol;
		  GET_BIT2_CHECK(probLit, symbol, offs &= ~bit, offs &= bit)
		}
		while (symbol < 0x100);
	  }
	  res = DUMMY_LIT;
	}
	else
	{
	  unsigned len;
	  UPDATE_1_CHECK;

	  prob = probs + IsRep + state;
	  IF_BIT_0_CHECK(prob)
	  {
		UPDATE_0_CHECK;
		state = 0;
		prob = probs + LenCoder;
		res = DUMMY_MATCH;
	  }
	  else
	  {
		UPDATE_1_CHECK;
		res = DUMMY_REP;
		prob = probs + IsRepG0 + state;
		IF_BIT_0_CHECK(prob)
		{
		  UPDATE_0_CHECK;
		  prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
		  IF_BIT_0_CHECK(prob)
		  {
			UPDATE_0_CHECK;
			NORMALIZE_CHECK;
			return DUMMY_REP;
		  }
		  else
		  {
			UPDATE_1_CHECK;
		  }
		}
		else
		{
		  UPDATE_1_CHECK;
		  prob = probs + IsRepG1 + state;
		  IF_BIT_0_CHECK(prob)
		  {
			UPDATE_0_CHECK;
		  }
		  else
		  {
			UPDATE_1_CHECK;
			prob = probs + IsRepG2 + state;
			IF_BIT_0_CHECK(prob)
			{
			  UPDATE_0_CHECK;
			}
			else
			{
			  UPDATE_1_CHECK;
			}
		  }
		}
		state = kNumStates;
		prob = probs + RepLenCoder;
	  }
	  {
		unsigned limit, offset;
		CLzmaProb *probLen = prob + LenChoice;
		IF_BIT_0_CHECK(probLen)
		{
		  UPDATE_0_CHECK;
		  probLen = prob + LenLow + (posState << kLenNumLowBits);
		  offset = 0;
		  limit = 1 << kLenNumLowBits;
		}
		else
		{
		  UPDATE_1_CHECK;
		  probLen = prob + LenChoice2;
		  IF_BIT_0_CHECK(probLen)
		  {
			UPDATE_0_CHECK;
			probLen = prob + LenMid + (posState << kLenNumMidBits);
			offset = kLenNumLowSymbols;
			limit = 1 << kLenNumMidBits;
		  }
		  else
		  {
			UPDATE_1_CHECK;
			probLen = prob + LenHigh;
			offset = kLenNumLowSymbols + kLenNumMidSymbols;
			limit = 1 << kLenNumHighBits;
		  }
		}
		TREE_DECODE_CHECK(probLen, limit, len);
		len += offset;
	  }

	  if (state < 4)
	  {
		unsigned posSlot;
		prob = probs + PosSlot +
			((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) <<
			kNumPosSlotBits);
		TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot);
		if (posSlot >= kStartPosModelIndex)
		{
		  int numDirectBits = ((posSlot >> 1) - 1);

		  /* if (bufLimit - buf >= 8) return DUMMY_MATCH; */

		  if (posSlot < kEndPosModelIndex)
		  {
			prob = probs + SpecPos + ((2 | (posSlot & 1)) << numDirectBits) - posSlot - 1;
		  }
		  else
		  {
			numDirectBits -= kNumAlignBits;
			do
			{
			  NORMALIZE_CHECK
			  range >>= 1;
			  code -= range & (((code - range) >> 31) - 1);
			  /* if (code >= range) code -= range; */
			}
			while (--numDirectBits != 0);
			prob = probs + Align;
			numDirectBits = kNumAlignBits;
		  }
		  {
			unsigned i = 1;
			do
			{
			  GET_BIT_CHECK(prob + i, i);
			}
			while (--numDirectBits != 0);
		  }
		}
	  }
	}
  }
  NORMALIZE_CHECK;
  return res;
}

static void LzmaDec_InitRc(CLzmaDec *p, const Byte *data)
{
  p->code = ((UInt32)data[1] << 24) | ((UInt32)data[2] << 16) | ((UInt32)data[3] << 8) | ((UInt32)data[4]);
  p->range = 0xFFFFFFFF;
  p->needFlush = 0;
}

void LzmaDec_InitDicAndState(CLzmaDec *p, Bool initDic, Bool initState)
{
  p->needFlush = 1;
  p->remainLen = 0;
  p->tempBufSize = 0;

  if (initDic)
  {
	p->processedPos = 0;
	p->checkDicSize = 0;
	p->needInitState = 1;
  }
  if (initState)
	p->needInitState = 1;
}

void LzmaDec_Init(CLzmaDec *p)
{
  p->dicPos = 0;
  LzmaDec_InitDicAndState(p, True, True);
}

static void LzmaDec_InitStateReal(CLzmaDec *p)
{
  UInt32 numProbs = Literal + ((UInt32)LZMA_LIT_SIZE << (p->prop.lc + p->prop.lp));
  UInt32 i;
  CLzmaProb *probs = p->probs;
  for (i = 0; i < numProbs; i++)
	probs[i] = kBitModelTotal >> 1;
  p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = 1;
  p->state = 0;
  p->needInitState = 0;
}

SRes LzmaDec_DecodeToDic(CLzmaDec *p, SizeT dicLimit, const Byte *src, SizeT *srcLen,
	ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT inSize = *srcLen;
  (*srcLen) = 0;
  LzmaDec_WriteRem(p, dicLimit);

  *status = LZMA_STATUS_NOT_SPECIFIED;

  while (p->remainLen != kMatchSpecLenStart)
  {
	  int checkEndMarkNow;

	  if (p->needFlush != 0)
	  {
		for (; inSize > 0 && p->tempBufSize < RC_INIT_SIZE; (*srcLen)++, inSize--)
		  p->tempBuf[p->tempBufSize++] = *src++;
		if (p->tempBufSize < RC_INIT_SIZE)
		{
		  *status = LZMA_STATUS_NEEDS_MORE_INPUT;
		  return SZ_OK;
		}
		if (p->tempBuf[0] != 0)
		  return SZ_ERROR_DATA;

		LzmaDec_InitRc(p, p->tempBuf);
		p->tempBufSize = 0;
	  }

	  checkEndMarkNow = 0;
	  if (p->dicPos >= dicLimit)
	  {
		if (p->remainLen == 0 && p->code == 0)
		{
		  *status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
		  return SZ_OK;
		}
		if (finishMode == LZMA_FINISH_ANY)
		{
		  *status = LZMA_STATUS_NOT_FINISHED;
		  return SZ_OK;
		}
		if (p->remainLen != 0)
		{
		  *status = LZMA_STATUS_NOT_FINISHED;
		  return SZ_ERROR_DATA;
		}
		checkEndMarkNow = 1;
	  }

	  if (p->needInitState)
		LzmaDec_InitStateReal(p);

	  if (p->tempBufSize == 0)
	  {
		SizeT processed;
		const Byte *bufLimit;
		if (inSize < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
		{
		  int dummyRes = LzmaDec_TryDummy(p, src, inSize);
		  if (dummyRes == DUMMY_ERROR)
		  {
			memcpy(p->tempBuf, src, inSize);
			p->tempBufSize = (unsigned)inSize;
			(*srcLen) += inSize;
			*status = LZMA_STATUS_NEEDS_MORE_INPUT;
			return SZ_OK;
		  }
		  if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
		  {
			*status = LZMA_STATUS_NOT_FINISHED;
			return SZ_ERROR_DATA;
		  }
		  bufLimit = src;
		}
		else
		  bufLimit = src + inSize - LZMA_REQUIRED_INPUT_MAX;
		p->buf = src;
		if (LzmaDec_DecodeReal2(p, dicLimit, bufLimit) != 0)
		  return SZ_ERROR_DATA;
		processed = (SizeT)(p->buf - src);
		(*srcLen) += processed;
		src += processed;
		inSize -= processed;
	  }
	  else
	  {
		unsigned rem = p->tempBufSize, lookAhead = 0;
		while (rem < LZMA_REQUIRED_INPUT_MAX && lookAhead < inSize)
		  p->tempBuf[rem++] = src[lookAhead++];
		p->tempBufSize = rem;
		if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
		{
		  int dummyRes = LzmaDec_TryDummy(p, p->tempBuf, rem);
		  if (dummyRes == DUMMY_ERROR)
		  {
			(*srcLen) += lookAhead;
			*status = LZMA_STATUS_NEEDS_MORE_INPUT;
			return SZ_OK;
		  }
		  if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
		  {
			*status = LZMA_STATUS_NOT_FINISHED;
			return SZ_ERROR_DATA;
		  }
		}
		p->buf = p->tempBuf;
		if (LzmaDec_DecodeReal2(p, dicLimit, p->buf) != 0)
		  return SZ_ERROR_DATA;
		lookAhead -= (rem - (unsigned)(p->buf - p->tempBuf));
		(*srcLen) += lookAhead;
		src += lookAhead;
		inSize -= lookAhead;
		p->tempBufSize = 0;
	  }
  }
  if (p->code == 0)
	*status = LZMA_STATUS_FINISHED_WITH_MARK;
  return (p->code == 0) ? SZ_OK : SZ_ERROR_DATA;
}

SRes LzmaDec_DecodeToBuf(CLzmaDec *p, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT outSize = *destLen;
  SizeT inSize = *srcLen;
  *srcLen = *destLen = 0;
  for (;;)
  {
	SizeT inSizeCur = inSize, outSizeCur, dicPos;
	ELzmaFinishMode curFinishMode;
	SRes res;
	if (p->dicPos == p->dicBufSize)
	  p->dicPos = 0;
	dicPos = p->dicPos;
	if (outSize > p->dicBufSize - dicPos)
	{
	  outSizeCur = p->dicBufSize;
	  curFinishMode = LZMA_FINISH_ANY;
	}
	else
	{
	  outSizeCur = dicPos + outSize;
	  curFinishMode = finishMode;
	}

	res = LzmaDec_DecodeToDic(p, outSizeCur, src, &inSizeCur, curFinishMode, status);
	src += inSizeCur;
	inSize -= inSizeCur;
	*srcLen += inSizeCur;
	outSizeCur = p->dicPos - dicPos;
	memcpy(dest, p->dic + dicPos, outSizeCur);
	dest += outSizeCur;
	outSize -= outSizeCur;
	*destLen += outSizeCur;
	if (res != 0)
	  return res;
	if (outSizeCur == 0 || outSize == 0)
	  return SZ_OK;
  }
}

void LzmaDec_FreeProbs(CLzmaDec *p, ISzAlloc *alloc)
{
  alloc->Free(alloc, p->probs);
  p->probs = 0;
}

static void LzmaDec_FreeDict(CLzmaDec *p, ISzAlloc *alloc)
{
  alloc->Free(alloc, p->dic);
  p->dic = 0;
}

void LzmaDec_Free(CLzmaDec *p, ISzAlloc *alloc)
{
  LzmaDec_FreeProbs(p, alloc);
  LzmaDec_FreeDict(p, alloc);
}

SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size)
{
  UInt32 dicSize;
  Byte d;

  if (size < LZMA_PROPS_SIZE)
	return SZ_ERROR_UNSUPPORTED;
  else
	dicSize = data[1] | ((UInt32)data[2] << 8) | ((UInt32)data[3] << 16) | ((UInt32)data[4] << 24);

  if (dicSize < LZMA_DIC_MIN)
	dicSize = LZMA_DIC_MIN;
  p->dicSize = dicSize;

  d = data[0];
  if (d >= (9 * 5 * 5))
	return SZ_ERROR_UNSUPPORTED;

  p->lc = d % 9;
  d /= 9;
  p->pb = d / 5;
  p->lp = d % 5;

  return SZ_OK;
}

static SRes LzmaDec_AllocateProbs2(CLzmaDec *p, const CLzmaProps *propNew, ISzAlloc *alloc)
{
  UInt32 numProbs = LzmaProps_GetNumProbs(propNew);
  if (p->probs == 0 || numProbs != p->numProbs)
  {
	LzmaDec_FreeProbs(p, alloc);
	p->probs = (CLzmaProb *)alloc->Alloc(alloc, numProbs * sizeof(CLzmaProb));
	p->numProbs = numProbs;
	if (p->probs == 0)
	  return SZ_ERROR_MEM;
  }
  return SZ_OK;
}

SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAlloc *alloc)
{
  CLzmaProps propNew;
  RINOK(LzmaProps_Decode(&propNew, props, propsSize));
  RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc));
  p->prop = propNew;
  return SZ_OK;
}

SRes LzmaDec_Allocate(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAlloc *alloc)
{
  CLzmaProps propNew;
  SizeT dicBufSize;
  RINOK(LzmaProps_Decode(&propNew, props, propsSize));
  RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc));
  dicBufSize = propNew.dicSize;
  if (p->dic == 0 || dicBufSize != p->dicBufSize)
  {
	LzmaDec_FreeDict(p, alloc);
	p->dic = (Byte *)alloc->Alloc(alloc, dicBufSize);
	if (p->dic == 0)
	{
	  LzmaDec_FreeProbs(p, alloc);
	  return SZ_ERROR_MEM;
	}
  }
  p->dicBufSize = dicBufSize;
  p->prop = propNew;
  return SZ_OK;
}

SRes LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
	const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode,
	ELzmaStatus *status, ISzAlloc *alloc)
{
  CLzmaDec p;
  SRes res;
  SizeT outSize = *destLen, inSize = *srcLen;
  *destLen = *srcLen = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;
  if (inSize < RC_INIT_SIZE)
	return SZ_ERROR_INPUT_EOF;
  LzmaDec_Construct(&p);
  RINOK(LzmaDec_AllocateProbs(&p, propData, propSize, alloc));
  p.dic = dest;
  p.dicBufSize = outSize;
  LzmaDec_Init(&p);
  *srcLen = inSize;
  res = LzmaDec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
  *destLen = p.dicPos;
  if (res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
	res = SZ_ERROR_INPUT_EOF;
  LzmaDec_FreeProbs(&p, alloc);
  return res;
}


#line 3 "LzmaEnc.c"
#include <string.h>

/* #define SHOW_STAT */
/* #define SHOW_STAT2 */

#if defined(SHOW_STAT) || defined(SHOW_STAT2)
#include <stdio.h>
#endif

#define _7ZIP_ST
#ifndef _7ZIP_ST
#include "LzFindMt.h"
#endif

#ifdef SHOW_STAT
static int ttt = 0;
#endif

#define kBlockSizeMax ((1 << LZMA_NUM_BLOCK_SIZE_BITS) - 1)

#define kBlockSize (9 << 10)
#define kUnpackBlockSize (1 << 18)
#define kMatchArraySize (1 << 21)
#define kMatchRecordMaxSize ((LZMA_MATCH_LEN_MAX * 2 + 3) * LZMA_MATCH_LEN_MAX)

#define kNumMaxDirectBits (31)

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5
#define kProbInitValue (kBitModelTotal >> 1)

#define kNumMoveReducingBits 4
#define kNumBitPriceShiftBits 4
#define kBitPrice (1 << kNumBitPriceShiftBits)

void LzmaEncProps_Init(CLzmaEncProps *p)
{
  p->level = 5;
  p->dictSize = p->mc = 0;
  p->reduceSize = (UInt32)(Int32)-1;
  p->lc = p->lp = p->pb = p->algo = p->fb = p->btMode = p->numHashBytes = p->numThreads = -1;
  p->writeEndMark = 0;
}

void LzmaEncProps_Normalize(CLzmaEncProps *p)
{
  int level = p->level;
  if (level < 0) level = 5;
  p->level = level;
  if (p->dictSize == 0) p->dictSize = (level <= 5 ? (1 << (level * 2 + 14)) : (level == 6 ? (1 << 25) : (1 << 26)));
  if (p->dictSize > p->reduceSize)
  {
	unsigned i;
	for (i = 15; i <= 30; i++)
	{
	  if (p->reduceSize <= ((UInt32)2 << i)) { p->dictSize = ((UInt32)2 << i); break; }
	  if (p->reduceSize <= ((UInt32)3 << i)) { p->dictSize = ((UInt32)3 << i); break; }
	}
  }
  if (p->lc < 0) p->lc = 3;
  if (p->lp < 0) p->lp = 0;
  if (p->pb < 0) p->pb = 2;
  if (p->algo < 0) p->algo = (level < 5 ? 0 : 1);
  if (p->fb < 0) p->fb = (level < 7 ? 32 : 64);
  if (p->btMode < 0) p->btMode = (p->algo == 0 ? 0 : 1);
  if (p->numHashBytes < 0) p->numHashBytes = 4;
  if (p->mc == 0)  p->mc = (16 + (p->fb >> 1)) >> (p->btMode ? 0 : 1);
  if (p->numThreads < 0)
	p->numThreads =
	  #ifndef _7ZIP_ST
	  ((p->btMode && p->algo) ? 2 : 1);
	  #else
	  1;
	  #endif
}

UInt32 LzmaEncProps_GetDictSize(const CLzmaEncProps *props2)
{
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);
  return props.dictSize;
}

/* #define LZMA_LOG_BSR */
/* Define it for Intel's CPU */

#ifdef LZMA_LOG_BSR

#define kDicLogSizeMaxCompress 30

#define BSR2_RET(pos, res) { unsigned long i; _BitScanReverse(&i, (pos)); res = (i + i) + ((pos >> (i - 1)) & 1); }

UInt32 GetPosSlot1(UInt32 pos)
{
  UInt32 res;
  BSR2_RET(pos, res);
  return res;
}
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if (pos < 2) res = pos; else BSR2_RET(pos, res); }

#else

#define kNumLogBits (9 + (int)sizeof(size_t) / 2)
#define kDicLogSizeMaxCompress ((kNumLogBits - 1) * 2 + 7)

void LzmaEnc_FastPosInit(Byte *g_FastPos)
{
  int c = 2, slotFast;
  g_FastPos[0] = 0;
  g_FastPos[1] = 1;

  for (slotFast = 2; slotFast < kNumLogBits * 2; slotFast++)
  {
	UInt32 k = (1 << ((slotFast >> 1) - 1));
	UInt32 j;
	for (j = 0; j < k; j++, c++)
	  g_FastPos[c] = (Byte)slotFast;
  }
}

#define BSR2_RET(pos, res) { UInt32 i = 6 + ((kNumLogBits - 1) & \
  (0 - (((((UInt32)1 << (kNumLogBits + 6)) - 1) - pos) >> 31))); \
  res = p->g_FastPos[pos >> i] + (i * 2); }
/*
#define BSR2_RET(pos, res) { res = (pos < (1 << (kNumLogBits + 6))) ? \
  p->g_FastPos[pos >> 6] + 12 : \
  p->g_FastPos[pos >> (6 + kNumLogBits - 1)] + (6 + (kNumLogBits - 1)) * 2; }
*/

#define GetPosSlot1(pos) p->g_FastPos[pos]
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if (pos < kNumFullDistances) res = p->g_FastPos[pos]; else BSR2_RET(pos, res); }

#endif

#define LZMA_NUM_REPS 4

typedef unsigned CState;

typedef struct
{
  UInt32 price;

  CState state;
  int prev1IsChar;
  int prev2;

  UInt32 posPrev2;
  UInt32 backPrev2;

  UInt32 posPrev;
  UInt32 backPrev;
  UInt32 backs[LZMA_NUM_REPS];
} COptimal;

#define kNumOpts (1 << 12)

#define kNumLenToPosStates 4
#define kNumPosSlotBits 6
#define kDicLogSizeMin 0
#define kDicLogSizeMax 32
#define kDistTableSizeMax (kDicLogSizeMax * 2)

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)
#define kAlignMask (kAlignTableSize - 1)

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumPosModels (kEndPosModelIndex - kStartPosModelIndex)

#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#ifdef _LZMA_PROB32
#define CLzmaProb UInt32
#else
#define CLzmaProb UInt16
#endif

#define LZMA_PB_MAX 4
#define LZMA_LC_MAX 8
#define LZMA_LP_MAX 4

#define LZMA_NUM_PB_STATES_MAX (1 << LZMA_PB_MAX)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define kLenNumSymbolsTotal (kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)

#define LZMA_MATCH_LEN_MIN 2
#define LZMA_MATCH_LEN_MAX (LZMA_MATCH_LEN_MIN + kLenNumSymbolsTotal - 1)

#define kNumStates 12

typedef struct
{
  CLzmaProb choice;
  CLzmaProb choice2;
  CLzmaProb low[LZMA_NUM_PB_STATES_MAX << kLenNumLowBits];
  CLzmaProb mid[LZMA_NUM_PB_STATES_MAX << kLenNumMidBits];
  CLzmaProb high[kLenNumHighSymbols];
} CLenEnc;

typedef struct
{
  CLenEnc p;
  UInt32 prices[LZMA_NUM_PB_STATES_MAX][kLenNumSymbolsTotal];
  UInt32 tableSize;
  UInt32 counters[LZMA_NUM_PB_STATES_MAX];
} CLenPriceEnc;

typedef struct
{
  UInt32 range;
  Byte cache;
  UInt64 low;
  UInt64 cacheSize;
  Byte *buf;
  Byte *bufLim;
  Byte *bufBase;
  ISeqOutStream *outStream;
  UInt64 processed;
  SRes res;
} CRangeEnc;

typedef struct
{
  CLzmaProb *litProbs;

  CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb isRep[kNumStates];
  CLzmaProb isRepG0[kNumStates];
  CLzmaProb isRepG1[kNumStates];
  CLzmaProb isRepG2[kNumStates];
  CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];

  CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  CLzmaProb posEncoders[kNumFullDistances - kEndPosModelIndex];
  CLzmaProb posAlignEncoder[1 << kNumAlignBits];

  CLenPriceEnc lenEnc;
  CLenPriceEnc repLenEnc;

  UInt32 reps[LZMA_NUM_REPS];
  UInt32 state;
} CSaveState;

typedef struct
{
  IMatchFinder matchFinder;
  void *matchFinderObj;

  #ifndef _7ZIP_ST
  Bool mtMode;
  CMatchFinderMt matchFinderMt;
  #endif

  CMatchFinder matchFinderBase;

  #ifndef _7ZIP_ST
  Byte pad[128];
  #endif

  UInt32 optimumEndIndex;
  UInt32 optimumCurrentIndex;

  UInt32 longestMatchLength;
  UInt32 numPairs;
  UInt32 numAvail;
  COptimal opt[kNumOpts];

  #ifndef LZMA_LOG_BSR
  Byte g_FastPos[1 << kNumLogBits];
  #endif

  UInt32 ProbPrices[kBitModelTotal >> kNumMoveReducingBits];
  UInt32 matches[LZMA_MATCH_LEN_MAX * 2 + 2 + 1];
  UInt32 numFastBytes;
  UInt32 additionalOffset;
  UInt32 reps[LZMA_NUM_REPS];
  UInt32 state;

  UInt32 posSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  UInt32 distancesPrices[kNumLenToPosStates][kNumFullDistances];
  UInt32 alignPrices[kAlignTableSize];
  UInt32 alignPriceCount;

  UInt32 distTableSize;

  unsigned lc, lp, pb;
  unsigned lpMask, pbMask;

  CLzmaProb *litProbs;

  CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb isRep[kNumStates];
  CLzmaProb isRepG0[kNumStates];
  CLzmaProb isRepG1[kNumStates];
  CLzmaProb isRepG2[kNumStates];
  CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];

  CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  CLzmaProb posEncoders[kNumFullDistances - kEndPosModelIndex];
  CLzmaProb posAlignEncoder[1 << kNumAlignBits];

  CLenPriceEnc lenEnc;
  CLenPriceEnc repLenEnc;

  unsigned lclp;

  Bool fastMode;

  CRangeEnc rc;

  Bool writeEndMark;
  UInt64 nowPos64;
  UInt32 matchPriceCount;
  Bool finished;
  Bool multiThread;

  SRes result;
  UInt32 dictSize;

  int needInit;

  CSaveState saveState;
} CLzmaEnc;

void LzmaEnc_SaveState(CLzmaEncHandle pp)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  CSaveState *dest = &p->saveState;
  int i;
  dest->lenEnc = p->lenEnc;
  dest->repLenEnc = p->repLenEnc;
  dest->state = p->state;

  for (i = 0; i < kNumStates; i++)
  {
	memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
	memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
  }
  for (i = 0; i < kNumLenToPosStates; i++)
	memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));
  memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
  memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
  memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
  memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
  memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
  memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
  memcpy(dest->reps, p->reps, sizeof(p->reps));
  memcpy(dest->litProbs, p->litProbs, (0x300 << p->lclp) * sizeof(CLzmaProb));
}

void LzmaEnc_RestoreState(CLzmaEncHandle pp)
{
  CLzmaEnc *dest = (CLzmaEnc *)pp;
  const CSaveState *p = &dest->saveState;
  int i;
  dest->lenEnc = p->lenEnc;
  dest->repLenEnc = p->repLenEnc;
  dest->state = p->state;

  for (i = 0; i < kNumStates; i++)
  {
	memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
	memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
  }
  for (i = 0; i < kNumLenToPosStates; i++)
	memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));
  memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
  memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
  memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
  memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
  memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
  memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
  memcpy(dest->reps, p->reps, sizeof(p->reps));
  memcpy(dest->litProbs, p->litProbs, (0x300 << dest->lclp) * sizeof(CLzmaProb));
}

SRes LzmaEnc_SetProps(CLzmaEncHandle pp, const CLzmaEncProps *props2)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);

  if (props.lc > LZMA_LC_MAX || props.lp > LZMA_LP_MAX || props.pb > LZMA_PB_MAX ||
	  props.dictSize > ((UInt32)1 << kDicLogSizeMaxCompress) || props.dictSize > ((UInt32)1 << 30))
	return SZ_ERROR_PARAM;
  p->dictSize = props.dictSize;
  {
	unsigned fb = props.fb;
	if (fb < 5)
	  fb = 5;
	if (fb > LZMA_MATCH_LEN_MAX)
	  fb = LZMA_MATCH_LEN_MAX;
	p->numFastBytes = fb;
  }
  p->lc = props.lc;
  p->lp = props.lp;
  p->pb = props.pb;
  p->fastMode = (props.algo == 0);
  p->matchFinderBase.btMode = props.btMode;
  {
	UInt32 numHashBytes = 4;
	if (props.btMode)
	{
	  if (props.numHashBytes < 2)
		numHashBytes = 2;
	  else if (props.numHashBytes < 4)
		numHashBytes = props.numHashBytes;
	}
	p->matchFinderBase.numHashBytes = numHashBytes;
  }

  p->matchFinderBase.cutValue = props.mc;

  p->writeEndMark = props.writeEndMark;

  #ifndef _7ZIP_ST
  /*
  if (newMultiThread != _multiThread)
  {
	ReleaseMatchFinder();
	_multiThread = newMultiThread;
  }
  */
  p->multiThread = (props.numThreads > 1);
  #endif

  return SZ_OK;
}

static const int kLiteralNextStates[kNumStates] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5};
static const int kMatchNextStates[kNumStates]   = {7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10};
static const int kRepNextStates[kNumStates]     = {8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11};
static const int kShortRepNextStates[kNumStates]= {9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11};

#define IsCharState(s) ((s) < 7)

#define GetLenToPosState(len) (((len) < kNumLenToPosStates + 1) ? (len) - 2 : kNumLenToPosStates - 1)

#define kInfinityPrice (1 << 30)

static void RangeEnc_Construct(CRangeEnc *p)
{
  p->outStream = 0;
  p->bufBase = 0;
}

#define RangeEnc_GetProcessed(p) ((p)->processed + ((p)->buf - (p)->bufBase) + (p)->cacheSize)

#define RC_BUF_SIZE (1 << 16)
static int RangeEnc_Alloc(CRangeEnc *p, ISzAlloc *alloc)
{
  if (p->bufBase == 0)
  {
	p->bufBase = (Byte *)alloc->Alloc(alloc, RC_BUF_SIZE);
	if (p->bufBase == 0)
	  return 0;
	p->bufLim = p->bufBase + RC_BUF_SIZE;
  }
  return 1;
}

static void RangeEnc_Free(CRangeEnc *p, ISzAlloc *alloc)
{
  alloc->Free(alloc, p->bufBase);
  p->bufBase = 0;
}

static void RangeEnc_Init(CRangeEnc *p)
{
  /* Stream.Init(); */
  p->low = 0;
  p->range = 0xFFFFFFFF;
  p->cacheSize = 1;
  p->cache = 0;

  p->buf = p->bufBase;

  p->processed = 0;
  p->res = SZ_OK;
}

static void RangeEnc_FlushStream(CRangeEnc *p)
{
  size_t num;
  if (p->res != SZ_OK)
	return;
  num = p->buf - p->bufBase;
  if (num != p->outStream->Write(p->outStream, p->bufBase, num))
	p->res = SZ_ERROR_WRITE;
  p->processed += num;
  p->buf = p->bufBase;
}

static void MY_FAST_CALL RangeEnc_ShiftLow(CRangeEnc *p)
{
  if ((UInt32)p->low < (UInt32)0xFF000000 || (int)(p->low >> 32) != 0)
  {
	Byte temp = p->cache;
	do
	{
	  Byte *buf = p->buf;
	  *buf++ = (Byte)(temp + (Byte)(p->low >> 32));
	  p->buf = buf;
	  if (buf == p->bufLim)
		RangeEnc_FlushStream(p);
	  temp = 0xFF;
	}
	while (--p->cacheSize != 0);
	p->cache = (Byte)((UInt32)p->low >> 24);
  }
  p->cacheSize++;
  p->low = (UInt32)p->low << 8;
}

static void RangeEnc_FlushData(CRangeEnc *p)
{
  int i;
  for (i = 0; i < 5; i++)
	RangeEnc_ShiftLow(p);
}

static void RangeEnc_EncodeDirectBits(CRangeEnc *p, UInt32 value, int numBits)
{
  do
  {
	p->range >>= 1;
	p->low += p->range & (0 - ((value >> --numBits) & 1));
	if (p->range < kTopValue)
	{
	  p->range <<= 8;
	  RangeEnc_ShiftLow(p);
	}
  }
  while (numBits != 0);
}

static void RangeEnc_EncodeBit(CRangeEnc *p, CLzmaProb *prob, UInt32 symbol)
{
  UInt32 ttt = *prob;
  UInt32 newBound = (p->range >> kNumBitModelTotalBits) * ttt;
  if (symbol == 0)
  {
	p->range = newBound;
	ttt += (kBitModelTotal - ttt) >> kNumMoveBits;
  }
  else
  {
	p->low += newBound;
	p->range -= newBound;
	ttt -= ttt >> kNumMoveBits;
  }
  *prob = (CLzmaProb)ttt;
  if (p->range < kTopValue)
  {
	p->range <<= 8;
	RangeEnc_ShiftLow(p);
  }
}

static void LitEnc_Encode(CRangeEnc *p, CLzmaProb *probs, UInt32 symbol)
{
  symbol |= 0x100;
  do
  {
	RangeEnc_EncodeBit(p, probs + (symbol >> 8), (symbol >> 7) & 1);
	symbol <<= 1;
  }
  while (symbol < 0x10000);
}

static void LitEnc_EncodeMatched(CRangeEnc *p, CLzmaProb *probs, UInt32 symbol, UInt32 matchByte)
{
  UInt32 offs = 0x100;
  symbol |= 0x100;
  do
  {
	matchByte <<= 1;
	RangeEnc_EncodeBit(p, probs + (offs + (matchByte & offs) + (symbol >> 8)), (symbol >> 7) & 1);
	symbol <<= 1;
	offs &= ~(matchByte ^ symbol);
  }
  while (symbol < 0x10000);
}

void LzmaEnc_InitPriceTables(UInt32 *ProbPrices)
{
  UInt32 i;
  for (i = (1 << kNumMoveReducingBits) / 2; i < kBitModelTotal; i += (1 << kNumMoveReducingBits))
  {
	const int kCyclesBits = kNumBitPriceShiftBits;
	UInt32 w = i;
	UInt32 bitCount = 0;
	int j;
	for (j = 0; j < kCyclesBits; j++)
	{
	  w = w * w;
	  bitCount <<= 1;
	  while (w >= ((UInt32)1 << 16))
	  {
		w >>= 1;
		bitCount++;
	  }
	}
	ProbPrices[i >> kNumMoveReducingBits] = ((kNumBitModelTotalBits << kCyclesBits) - 15 - bitCount);
  }
}

#define GET_PRICE(prob, symbol) \
  p->ProbPrices[((prob) ^ (((-(int)(symbol))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];

#define GET_PRICEa(prob, symbol) \
  ProbPrices[((prob) ^ ((-((int)(symbol))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];

#define GET_PRICE_0(prob) p->ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1(prob) p->ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]

#define GET_PRICE_0a(prob) ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1a(prob) ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]

static UInt32 LitEnc_GetPrice(const CLzmaProb *probs, UInt32 symbol, UInt32 *ProbPrices)
{
  UInt32 price = 0;
  symbol |= 0x100;
  do
  {
	price += GET_PRICEa(probs[symbol >> 8], (symbol >> 7) & 1);
	symbol <<= 1;
  }
  while (symbol < 0x10000);
  return price;
}

static UInt32 LitEnc_GetPriceMatched(const CLzmaProb *probs, UInt32 symbol, UInt32 matchByte, UInt32 *ProbPrices)
{
  UInt32 price = 0;
  UInt32 offs = 0x100;
  symbol |= 0x100;
  do
  {
	matchByte <<= 1;
	price += GET_PRICEa(probs[offs + (matchByte & offs) + (symbol >> 8)], (symbol >> 7) & 1);
	symbol <<= 1;
	offs &= ~(matchByte ^ symbol);
  }
  while (symbol < 0x10000);
  return price;
}

static void RcTree_Encode(CRangeEnc *rc, CLzmaProb *probs, int numBitLevels, UInt32 symbol)
{
  UInt32 m = 1;
  int i;
  for (i = numBitLevels; i != 0;)
  {
	UInt32 bit;
	i--;
	bit = (symbol >> i) & 1;
	RangeEnc_EncodeBit(rc, probs + m, bit);
	m = (m << 1) | bit;
  }
}

static void RcTree_ReverseEncode(CRangeEnc *rc, CLzmaProb *probs, int numBitLevels, UInt32 symbol)
{
  UInt32 m = 1;
  int i;
  for (i = 0; i < numBitLevels; i++)
  {
	UInt32 bit = symbol & 1;
	RangeEnc_EncodeBit(rc, probs + m, bit);
	m = (m << 1) | bit;
	symbol >>= 1;
  }
}

static UInt32 RcTree_GetPrice(const CLzmaProb *probs, int numBitLevels, UInt32 symbol, UInt32 *ProbPrices)
{
  UInt32 price = 0;
  symbol |= (1 << numBitLevels);
  while (symbol != 1)
  {
	price += GET_PRICEa(probs[symbol >> 1], symbol & 1);
	symbol >>= 1;
  }
  return price;
}

static UInt32 RcTree_ReverseGetPrice(const CLzmaProb *probs, int numBitLevels, UInt32 symbol, UInt32 *ProbPrices)
{
  UInt32 price = 0;
  UInt32 m = 1;
  int i;
  for (i = numBitLevels; i != 0; i--)
  {
	UInt32 bit = symbol & 1;
	symbol >>= 1;
	price += GET_PRICEa(probs[m], bit);
	m = (m << 1) | bit;
  }
  return price;
}

static void LenEnc_Init(CLenEnc *p)
{
  unsigned i;
  p->choice = p->choice2 = kProbInitValue;
  for (i = 0; i < (LZMA_NUM_PB_STATES_MAX << kLenNumLowBits); i++)
	p->low[i] = kProbInitValue;
  for (i = 0; i < (LZMA_NUM_PB_STATES_MAX << kLenNumMidBits); i++)
	p->mid[i] = kProbInitValue;
  for (i = 0; i < kLenNumHighSymbols; i++)
	p->high[i] = kProbInitValue;
}

static void LenEnc_Encode(CLenEnc *p, CRangeEnc *rc, UInt32 symbol, UInt32 posState)
{
  if (symbol < kLenNumLowSymbols)
  {
	RangeEnc_EncodeBit(rc, &p->choice, 0);
	RcTree_Encode(rc, p->low + (posState << kLenNumLowBits), kLenNumLowBits, symbol);
  }
  else
  {
	RangeEnc_EncodeBit(rc, &p->choice, 1);
	if (symbol < kLenNumLowSymbols + kLenNumMidSymbols)
	{
	  RangeEnc_EncodeBit(rc, &p->choice2, 0);
	  RcTree_Encode(rc, p->mid + (posState << kLenNumMidBits), kLenNumMidBits, symbol - kLenNumLowSymbols);
	}
	else
	{
	  RangeEnc_EncodeBit(rc, &p->choice2, 1);
	  RcTree_Encode(rc, p->high, kLenNumHighBits, symbol - kLenNumLowSymbols - kLenNumMidSymbols);
	}
  }
}

static void LenEnc_SetPrices(CLenEnc *p, UInt32 posState, UInt32 numSymbols, UInt32 *prices, UInt32 *ProbPrices)
{
  UInt32 a0 = GET_PRICE_0a(p->choice);
  UInt32 a1 = GET_PRICE_1a(p->choice);
  UInt32 b0 = a1 + GET_PRICE_0a(p->choice2);
  UInt32 b1 = a1 + GET_PRICE_1a(p->choice2);
  UInt32 i = 0;
  for (i = 0; i < kLenNumLowSymbols; i++)
  {
	if (i >= numSymbols)
	  return;
	prices[i] = a0 + RcTree_GetPrice(p->low + (posState << kLenNumLowBits), kLenNumLowBits, i, ProbPrices);
  }
  for (; i < kLenNumLowSymbols + kLenNumMidSymbols; i++)
  {
	if (i >= numSymbols)
	  return;
	prices[i] = b0 + RcTree_GetPrice(p->mid + (posState << kLenNumMidBits), kLenNumMidBits, i - kLenNumLowSymbols, ProbPrices);
  }
  for (; i < numSymbols; i++)
	prices[i] = b1 + RcTree_GetPrice(p->high, kLenNumHighBits, i - kLenNumLowSymbols - kLenNumMidSymbols, ProbPrices);
}

static void MY_FAST_CALL LenPriceEnc_UpdateTable(CLenPriceEnc *p, UInt32 posState, UInt32 *ProbPrices)
{
  LenEnc_SetPrices(&p->p, posState, p->tableSize, p->prices[posState], ProbPrices);
  p->counters[posState] = p->tableSize;
}

static void LenPriceEnc_UpdateTables(CLenPriceEnc *p, UInt32 numPosStates, UInt32 *ProbPrices)
{
  UInt32 posState;
  for (posState = 0; posState < numPosStates; posState++)
	LenPriceEnc_UpdateTable(p, posState, ProbPrices);
}

static void LenEnc_Encode2(CLenPriceEnc *p, CRangeEnc *rc, UInt32 symbol, UInt32 posState, Bool updatePrice, UInt32 *ProbPrices)
{
  LenEnc_Encode(&p->p, rc, symbol, posState);
  if (updatePrice)
	if (--p->counters[posState] == 0)
	  LenPriceEnc_UpdateTable(p, posState, ProbPrices);
}

static void MovePos(CLzmaEnc *p, UInt32 num)
{
  #ifdef SHOW_STAT
  ttt += num;
  printf("\n MovePos %d", num);
  #endif
  if (num != 0)
  {
	p->additionalOffset += num;
	p->matchFinder.Skip(p->matchFinderObj, num);
  }
}

static UInt32 ReadMatchDistances(CLzmaEnc *p, UInt32 *numDistancePairsRes)
{
  UInt32 lenRes = 0, numPairs;
  p->numAvail = p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
  numPairs = p->matchFinder.GetMatches(p->matchFinderObj, p->matches);
  #ifdef SHOW_STAT
  printf("\n i = %d numPairs = %d    ", ttt, numPairs / 2);
  ttt++;
  {
	UInt32 i;
	for (i = 0; i < numPairs; i += 2)
	  printf("%2d %6d   | ", p->matches[i], p->matches[i + 1]);
  }
  #endif
  if (numPairs > 0)
  {
	lenRes = p->matches[numPairs - 2];
	if (lenRes == p->numFastBytes)
	{
	  const Byte *pby = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
	  UInt32 distance = p->matches[numPairs - 1] + 1;
	  UInt32 numAvail = p->numAvail;
	  if (numAvail > LZMA_MATCH_LEN_MAX)
		numAvail = LZMA_MATCH_LEN_MAX;
	  {
		const Byte *pby2 = pby - distance;
		for (; lenRes < numAvail && pby[lenRes] == pby2[lenRes]; lenRes++);
	  }
	}
  }
  p->additionalOffset++;
  *numDistancePairsRes = numPairs;
  return lenRes;
}

#define MakeAsChar(p) (p)->backPrev = (UInt32)(-1); (p)->prev1IsChar = False;
#define MakeAsShortRep(p) (p)->backPrev = 0; (p)->prev1IsChar = False;
#define IsShortRep(p) ((p)->backPrev == 0)

static UInt32 GetRepLen1Price(CLzmaEnc *p, UInt32 state, UInt32 posState)
{
  return
	GET_PRICE_0(p->isRepG0[state]) +
	GET_PRICE_0(p->isRep0Long[state][posState]);
}

static UInt32 GetPureRepPrice(CLzmaEnc *p, UInt32 repIndex, UInt32 state, UInt32 posState)
{
  UInt32 price;
  if (repIndex == 0)
  {
	price = GET_PRICE_0(p->isRepG0[state]);
	price += GET_PRICE_1(p->isRep0Long[state][posState]);
  }
  else
  {
	price = GET_PRICE_1(p->isRepG0[state]);
	if (repIndex == 1)
	  price += GET_PRICE_0(p->isRepG1[state]);
	else
	{
	  price += GET_PRICE_1(p->isRepG1[state]);
	  price += GET_PRICE(p->isRepG2[state], repIndex - 2);
	}
  }
  return price;
}

static UInt32 GetRepPrice(CLzmaEnc *p, UInt32 repIndex, UInt32 len, UInt32 state, UInt32 posState)
{
  return p->repLenEnc.prices[posState][len - LZMA_MATCH_LEN_MIN] +
	GetPureRepPrice(p, repIndex, state, posState);
}

static UInt32 Backward(CLzmaEnc *p, UInt32 *backRes, UInt32 cur)
{
  UInt32 posMem = p->opt[cur].posPrev;
  UInt32 backMem = p->opt[cur].backPrev;
  p->optimumEndIndex = cur;
  do
  {
	if (p->opt[cur].prev1IsChar)
	{
	  MakeAsChar(&p->opt[posMem])
	  p->opt[posMem].posPrev = posMem - 1;
	  if (p->opt[cur].prev2)
	  {
		p->opt[posMem - 1].prev1IsChar = False;
		p->opt[posMem - 1].posPrev = p->opt[cur].posPrev2;
		p->opt[posMem - 1].backPrev = p->opt[cur].backPrev2;
	  }
	}
	{
	  UInt32 posPrev = posMem;
	  UInt32 backCur = backMem;

	  backMem = p->opt[posPrev].backPrev;
	  posMem = p->opt[posPrev].posPrev;

	  p->opt[posPrev].backPrev = backCur;
	  p->opt[posPrev].posPrev = cur;
	  cur = posPrev;
	}
  }
  while (cur != 0);
  *backRes = p->opt[0].backPrev;
  p->optimumCurrentIndex  = p->opt[0].posPrev;
  return p->optimumCurrentIndex;
}

#define LIT_PROBS(pos, prevByte) (p->litProbs + ((((pos) & p->lpMask) << p->lc) + ((prevByte) >> (8 - p->lc))) * 0x300)

static UInt32 GetOptimum(CLzmaEnc *p, UInt32 position, UInt32 *backRes)
{
  UInt32 numAvail, mainLen, numPairs, repMaxIndex, i, posState, lenEnd, len, cur;
  UInt32 matchPrice, repMatchPrice, normalMatchPrice;
  UInt32 reps[LZMA_NUM_REPS], repLens[LZMA_NUM_REPS];
  UInt32 *matches;
  const Byte *data;
  Byte curByte, matchByte;
  if (p->optimumEndIndex != p->optimumCurrentIndex)
  {
	const COptimal *opt = &p->opt[p->optimumCurrentIndex];
	UInt32 lenRes = opt->posPrev - p->optimumCurrentIndex;
	*backRes = opt->backPrev;
	p->optimumCurrentIndex = opt->posPrev;
	return lenRes;
  }
  p->optimumCurrentIndex = p->optimumEndIndex = 0;

  if (p->additionalOffset == 0)
	mainLen = ReadMatchDistances(p, &numPairs);
  else
  {
	mainLen = p->longestMatchLength;
	numPairs = p->numPairs;
  }

  numAvail = p->numAvail;
  if (numAvail < 2)
  {
	*backRes = (UInt32)(-1);
	return 1;
  }
  if (numAvail > LZMA_MATCH_LEN_MAX)
	numAvail = LZMA_MATCH_LEN_MAX;

  data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
  repMaxIndex = 0;
  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
	UInt32 lenTest;
	const Byte *data2;
	reps[i] = p->reps[i];
	data2 = data - (reps[i] + 1);
	if (data[0] != data2[0] || data[1] != data2[1])
	{
	  repLens[i] = 0;
	  continue;
	}
	for (lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++);
	repLens[i] = lenTest;
	if (lenTest > repLens[repMaxIndex])
	  repMaxIndex = i;
  }
  if (repLens[repMaxIndex] >= p->numFastBytes)
  {
	UInt32 lenRes;
	*backRes = repMaxIndex;
	lenRes = repLens[repMaxIndex];
	MovePos(p, lenRes - 1);
	return lenRes;
  }

  matches = p->matches;
  if (mainLen >= p->numFastBytes)
  {
	*backRes = matches[numPairs - 1] + LZMA_NUM_REPS;
	MovePos(p, mainLen - 1);
	return mainLen;
  }
  curByte = *data;
  matchByte = *(data - (reps[0] + 1));

  if (mainLen < 2 && curByte != matchByte && repLens[repMaxIndex] < 2)
  {
	*backRes = (UInt32)-1;
	return 1;
  }

  p->opt[0].state = (CState)p->state;

  posState = (position & p->pbMask);

  {
	const CLzmaProb *probs = LIT_PROBS(position, *(data - 1));
	p->opt[1].price = GET_PRICE_0(p->isMatch[p->state][posState]) +
		(!IsCharState(p->state) ?
		  LitEnc_GetPriceMatched(probs, curByte, matchByte, p->ProbPrices) :
		  LitEnc_GetPrice(probs, curByte, p->ProbPrices));
  }

  MakeAsChar(&p->opt[1]);

  matchPrice = GET_PRICE_1(p->isMatch[p->state][posState]);
  repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[p->state]);

  if (matchByte == curByte)
  {
	UInt32 shortRepPrice = repMatchPrice + GetRepLen1Price(p, p->state, posState);
	if (shortRepPrice < p->opt[1].price)
	{
	  p->opt[1].price = shortRepPrice;
	  MakeAsShortRep(&p->opt[1]);
	}
  }
  lenEnd = ((mainLen >= repLens[repMaxIndex]) ? mainLen : repLens[repMaxIndex]);

  if (lenEnd < 2)
  {
	*backRes = p->opt[1].backPrev;
	return 1;
  }

  p->opt[1].posPrev = 0;
  for (i = 0; i < LZMA_NUM_REPS; i++)
	p->opt[0].backs[i] = reps[i];

  len = lenEnd;
  do
	p->opt[len--].price = kInfinityPrice;
  while (len >= 2);

  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
	UInt32 repLen = repLens[i];
	UInt32 price;
	if (repLen < 2)
	  continue;
	price = repMatchPrice + GetPureRepPrice(p, i, p->state, posState);
	do
	{
	  UInt32 curAndLenPrice = price + p->repLenEnc.prices[posState][repLen - 2];
	  COptimal *opt = &p->opt[repLen];
	  if (curAndLenPrice < opt->price)
	  {
		opt->price = curAndLenPrice;
		opt->posPrev = 0;
		opt->backPrev = i;
		opt->prev1IsChar = False;
	  }
	}
	while (--repLen >= 2);
  }

  normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[p->state]);

  len = ((repLens[0] >= 2) ? repLens[0] + 1 : 2);
  if (len <= mainLen)
  {
	UInt32 offs = 0;
	while (len > matches[offs])
	  offs += 2;
	for (; ; len++)
	{
	  COptimal *opt;
	  UInt32 distance = matches[offs + 1];

	  UInt32 curAndLenPrice = normalMatchPrice + p->lenEnc.prices[posState][len - LZMA_MATCH_LEN_MIN];
	  UInt32 lenToPosState = GetLenToPosState(len);
	  if (distance < kNumFullDistances)
		curAndLenPrice += p->distancesPrices[lenToPosState][distance];
	  else
	  {
		UInt32 slot;
		GetPosSlot2(distance, slot);
		curAndLenPrice += p->alignPrices[distance & kAlignMask] + p->posSlotPrices[lenToPosState][slot];
	  }
	  opt = &p->opt[len];
	  if (curAndLenPrice < opt->price)
	  {
		opt->price = curAndLenPrice;
		opt->posPrev = 0;
		opt->backPrev = distance + LZMA_NUM_REPS;
		opt->prev1IsChar = False;
	  }
	  if (len == matches[offs])
	  {
		offs += 2;
		if (offs == numPairs)
		  break;
	  }
	}
  }

  cur = 0;

	#ifdef SHOW_STAT2
	if (position >= 0)
	{
	  unsigned i;
	  printf("\n pos = %4X", position);
	  for (i = cur; i <= lenEnd; i++)
	  printf("\nprice[%4X] = %d", position - cur + i, p->opt[i].price);
	}
	#endif

  for (;;)
  {
	UInt32 numAvailFull, newLen, numPairs, posPrev, state, posState, startLen;
	UInt32 curPrice, curAnd1Price, matchPrice, repMatchPrice;
	Bool nextIsChar;
	Byte curByte, matchByte;
	const Byte *data;
	COptimal *curOpt;
	COptimal *nextOpt;

	cur++;
	if (cur == lenEnd)
	  return Backward(p, backRes, cur);

	newLen = ReadMatchDistances(p, &numPairs);
	if (newLen >= p->numFastBytes)
	{
	  p->numPairs = numPairs;
	  p->longestMatchLength = newLen;
	  return Backward(p, backRes, cur);
	}
	position++;
	curOpt = &p->opt[cur];
	posPrev = curOpt->posPrev;
	if (curOpt->prev1IsChar)
	{
	  posPrev--;
	  if (curOpt->prev2)
	  {
		state = p->opt[curOpt->posPrev2].state;
		if (curOpt->backPrev2 < LZMA_NUM_REPS)
		  state = kRepNextStates[state];
		else
		  state = kMatchNextStates[state];
	  }
	  else
		state = p->opt[posPrev].state;
	  state = kLiteralNextStates[state];
	}
	else
	  state = p->opt[posPrev].state;
	if (posPrev == cur - 1)
	{
	  if (IsShortRep(curOpt))
		state = kShortRepNextStates[state];
	  else
		state = kLiteralNextStates[state];
	}
	else
	{
	  UInt32 pos;
	  const COptimal *prevOpt;
	  if (curOpt->prev1IsChar && curOpt->prev2)
	  {
		posPrev = curOpt->posPrev2;
		pos = curOpt->backPrev2;
		state = kRepNextStates[state];
	  }
	  else
	  {
		pos = curOpt->backPrev;
		if (pos < LZMA_NUM_REPS)
		  state = kRepNextStates[state];
		else
		  state = kMatchNextStates[state];
	  }
	  prevOpt = &p->opt[posPrev];
	  if (pos < LZMA_NUM_REPS)
	  {
		UInt32 i;
		reps[0] = prevOpt->backs[pos];
		for (i = 1; i <= pos; i++)
		  reps[i] = prevOpt->backs[i - 1];
		for (; i < LZMA_NUM_REPS; i++)
		  reps[i] = prevOpt->backs[i];
	  }
	  else
	  {
		UInt32 i;
		reps[0] = (pos - LZMA_NUM_REPS);
		for (i = 1; i < LZMA_NUM_REPS; i++)
		  reps[i] = prevOpt->backs[i - 1];
	  }
	}
	curOpt->state = (CState)state;

	curOpt->backs[0] = reps[0];
	curOpt->backs[1] = reps[1];
	curOpt->backs[2] = reps[2];
	curOpt->backs[3] = reps[3];

	curPrice = curOpt->price;
	nextIsChar = False;
	data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
	curByte = *data;
	matchByte = *(data - (reps[0] + 1));

	posState = (position & p->pbMask);

	curAnd1Price = curPrice + GET_PRICE_0(p->isMatch[state][posState]);
	{
	  const CLzmaProb *probs = LIT_PROBS(position, *(data - 1));
	  curAnd1Price +=
		(!IsCharState(state) ?
		  LitEnc_GetPriceMatched(probs, curByte, matchByte, p->ProbPrices) :
		  LitEnc_GetPrice(probs, curByte, p->ProbPrices));
	}

	nextOpt = &p->opt[cur + 1];

	if (curAnd1Price < nextOpt->price)
	{
	  nextOpt->price = curAnd1Price;
	  nextOpt->posPrev = cur;
	  MakeAsChar(nextOpt);
	  nextIsChar = True;
	}

	matchPrice = curPrice + GET_PRICE_1(p->isMatch[state][posState]);
	repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[state]);

	if (matchByte == curByte && !(nextOpt->posPrev < cur && nextOpt->backPrev == 0))
	{
	  UInt32 shortRepPrice = repMatchPrice + GetRepLen1Price(p, state, posState);
	  if (shortRepPrice <= nextOpt->price)
	  {
		nextOpt->price = shortRepPrice;
		nextOpt->posPrev = cur;
		MakeAsShortRep(nextOpt);
		nextIsChar = True;
	  }
	}
	numAvailFull = p->numAvail;
	{
	  UInt32 temp = kNumOpts - 1 - cur;
	  if (temp < numAvailFull)
		numAvailFull = temp;
	}

	if (numAvailFull < 2)
	  continue;
	numAvail = (numAvailFull <= p->numFastBytes ? numAvailFull : p->numFastBytes);

	if (!nextIsChar && matchByte != curByte) /* speed optimization */
	{
	  /* try Literal + rep0 */
	  UInt32 temp;
	  UInt32 lenTest2;
	  const Byte *data2 = data - (reps[0] + 1);
	  UInt32 limit = p->numFastBytes + 1;
	  if (limit > numAvailFull)
		limit = numAvailFull;

	  for (temp = 1; temp < limit && data[temp] == data2[temp]; temp++);
	  lenTest2 = temp - 1;
	  if (lenTest2 >= 2)
	  {
		UInt32 state2 = kLiteralNextStates[state];
		UInt32 posStateNext = (position + 1) & p->pbMask;
		UInt32 nextRepMatchPrice = curAnd1Price +
			GET_PRICE_1(p->isMatch[state2][posStateNext]) +
			GET_PRICE_1(p->isRep[state2]);
		/* for (; lenTest2 >= 2; lenTest2--) */
		{
		  UInt32 curAndLenPrice;
		  COptimal *opt;
		  UInt32 offset = cur + 1 + lenTest2;
		  while (lenEnd < offset)
			p->opt[++lenEnd].price = kInfinityPrice;
		  curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
		  opt = &p->opt[offset];
		  if (curAndLenPrice < opt->price)
		  {
			opt->price = curAndLenPrice;
			opt->posPrev = cur + 1;
			opt->backPrev = 0;
			opt->prev1IsChar = True;
			opt->prev2 = False;
		  }
		}
	  }
	}

	startLen = 2; /* speed optimization */
	{
	UInt32 repIndex;
	for (repIndex = 0; repIndex < LZMA_NUM_REPS; repIndex++)
	{
	  UInt32 lenTest;
	  UInt32 lenTestTemp;
	  UInt32 price;
	  const Byte *data2 = data - (reps[repIndex] + 1);
	  if (data[0] != data2[0] || data[1] != data2[1])
		continue;
	  for (lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++);
	  while (lenEnd < cur + lenTest)
		p->opt[++lenEnd].price = kInfinityPrice;
	  lenTestTemp = lenTest;
	  price = repMatchPrice + GetPureRepPrice(p, repIndex, state, posState);
	  do
	  {
		UInt32 curAndLenPrice = price + p->repLenEnc.prices[posState][lenTest - 2];
		COptimal *opt = &p->opt[cur + lenTest];
		if (curAndLenPrice < opt->price)
		{
		  opt->price = curAndLenPrice;
		  opt->posPrev = cur;
		  opt->backPrev = repIndex;
		  opt->prev1IsChar = False;
		}
	  }
	  while (--lenTest >= 2);
	  lenTest = lenTestTemp;

	  if (repIndex == 0)
		startLen = lenTest + 1;

	  /* if (_maxMode) */
		{
		  UInt32 lenTest2 = lenTest + 1;
		  UInt32 limit = lenTest2 + p->numFastBytes;
		  UInt32 nextRepMatchPrice;
		  if (limit > numAvailFull)
			limit = numAvailFull;
		  for (; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++);
		  lenTest2 -= lenTest + 1;
		  if (lenTest2 >= 2)
		  {
			UInt32 state2 = kRepNextStates[state];
			UInt32 posStateNext = (position + lenTest) & p->pbMask;
			UInt32 curAndLenCharPrice =
				price + p->repLenEnc.prices[posState][lenTest - 2] +
				GET_PRICE_0(p->isMatch[state2][posStateNext]) +
				LitEnc_GetPriceMatched(LIT_PROBS(position + lenTest, data[lenTest - 1]),
					data[lenTest], data2[lenTest], p->ProbPrices);
			state2 = kLiteralNextStates[state2];
			posStateNext = (position + lenTest + 1) & p->pbMask;
			nextRepMatchPrice = curAndLenCharPrice +
				GET_PRICE_1(p->isMatch[state2][posStateNext]) +
				GET_PRICE_1(p->isRep[state2]);

			/* for (; lenTest2 >= 2; lenTest2--) */
			{
			  UInt32 curAndLenPrice;
			  COptimal *opt;
			  UInt32 offset = cur + lenTest + 1 + lenTest2;
			  while (lenEnd < offset)
				p->opt[++lenEnd].price = kInfinityPrice;
			  curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
			  opt = &p->opt[offset];
			  if (curAndLenPrice < opt->price)
			  {
				opt->price = curAndLenPrice;
				opt->posPrev = cur + lenTest + 1;
				opt->backPrev = 0;
				opt->prev1IsChar = True;
				opt->prev2 = True;
				opt->posPrev2 = cur;
				opt->backPrev2 = repIndex;
			  }
			}
		  }
		}
	}
	}
	/* for (UInt32 lenTest = 2; lenTest <= newLen; lenTest++) */
	if (newLen > numAvail)
	{
	  newLen = numAvail;
	  for (numPairs = 0; newLen > matches[numPairs]; numPairs += 2);
	  matches[numPairs] = newLen;
	  numPairs += 2;
	}
	if (newLen >= startLen)
	{
	  UInt32 normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[state]);
	  UInt32 offs, curBack, posSlot;
	  UInt32 lenTest;
	  while (lenEnd < cur + newLen)
		p->opt[++lenEnd].price = kInfinityPrice;

	  offs = 0;
	  while (startLen > matches[offs])
		offs += 2;
	  curBack = matches[offs + 1];
	  GetPosSlot2(curBack, posSlot);
	  for (lenTest = /*2*/ startLen; ; lenTest++)
	  {
		UInt32 curAndLenPrice = normalMatchPrice + p->lenEnc.prices[posState][lenTest - LZMA_MATCH_LEN_MIN];
		UInt32 lenToPosState = GetLenToPosState(lenTest);
		COptimal *opt;
		if (curBack < kNumFullDistances)
		  curAndLenPrice += p->distancesPrices[lenToPosState][curBack];
		else
		  curAndLenPrice += p->posSlotPrices[lenToPosState][posSlot] + p->alignPrices[curBack & kAlignMask];

		opt = &p->opt[cur + lenTest];
		if (curAndLenPrice < opt->price)
		{
		  opt->price = curAndLenPrice;
		  opt->posPrev = cur;
		  opt->backPrev = curBack + LZMA_NUM_REPS;
		  opt->prev1IsChar = False;
		}

		if (/*_maxMode && */lenTest == matches[offs])
		{
		  /* Try Match + Literal + Rep0 */
		  const Byte *data2 = data - (curBack + 1);
		  UInt32 lenTest2 = lenTest + 1;
		  UInt32 limit = lenTest2 + p->numFastBytes;
		  UInt32 nextRepMatchPrice;
		  if (limit > numAvailFull)
			limit = numAvailFull;
		  for (; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++);
		  lenTest2 -= lenTest + 1;
		  if (lenTest2 >= 2)
		  {
			UInt32 state2 = kMatchNextStates[state];
			UInt32 posStateNext = (position + lenTest) & p->pbMask;
			UInt32 curAndLenCharPrice = curAndLenPrice +
				GET_PRICE_0(p->isMatch[state2][posStateNext]) +
				LitEnc_GetPriceMatched(LIT_PROBS(position + lenTest, data[lenTest - 1]),
					data[lenTest], data2[lenTest], p->ProbPrices);
			state2 = kLiteralNextStates[state2];
			posStateNext = (posStateNext + 1) & p->pbMask;
			nextRepMatchPrice = curAndLenCharPrice +
				GET_PRICE_1(p->isMatch[state2][posStateNext]) +
				GET_PRICE_1(p->isRep[state2]);

			/* for (; lenTest2 >= 2; lenTest2--) */
			{
			  UInt32 offset = cur + lenTest + 1 + lenTest2;
			  UInt32 curAndLenPrice;
			  COptimal *opt;
			  while (lenEnd < offset)
				p->opt[++lenEnd].price = kInfinityPrice;
			  curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
			  opt = &p->opt[offset];
			  if (curAndLenPrice < opt->price)
			  {
				opt->price = curAndLenPrice;
				opt->posPrev = cur + lenTest + 1;
				opt->backPrev = 0;
				opt->prev1IsChar = True;
				opt->prev2 = True;
				opt->posPrev2 = cur;
				opt->backPrev2 = curBack + LZMA_NUM_REPS;
			  }
			}
		  }
		  offs += 2;
		  if (offs == numPairs)
			break;
		  curBack = matches[offs + 1];
		  if (curBack >= kNumFullDistances)
			GetPosSlot2(curBack, posSlot);
		}
	  }
	}
  }
}

#define ChangePair(smallDist, bigDist) (((bigDist) >> 7) > (smallDist))

static UInt32 GetOptimumFast(CLzmaEnc *p, UInt32 *backRes)
{
  UInt32 numAvail, mainLen, mainDist, numPairs, repIndex, repLen, i;
  const Byte *data;
  const UInt32 *matches;

  if (p->additionalOffset == 0)
	mainLen = ReadMatchDistances(p, &numPairs);
  else
  {
	mainLen = p->longestMatchLength;
	numPairs = p->numPairs;
  }

  numAvail = p->numAvail;
  *backRes = (UInt32)-1;
  if (numAvail < 2)
	return 1;
  if (numAvail > LZMA_MATCH_LEN_MAX)
	numAvail = LZMA_MATCH_LEN_MAX;
  data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;

  repLen = repIndex = 0;
  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
	UInt32 len;
	const Byte *data2 = data - (p->reps[i] + 1);
	if (data[0] != data2[0] || data[1] != data2[1])
	  continue;
	for (len = 2; len < numAvail && data[len] == data2[len]; len++);
	if (len >= p->numFastBytes)
	{
	  *backRes = i;
	  MovePos(p, len - 1);
	  return len;
	}
	if (len > repLen)
	{
	  repIndex = i;
	  repLen = len;
	}
  }

  matches = p->matches;
  if (mainLen >= p->numFastBytes)
  {
	*backRes = matches[numPairs - 1] + LZMA_NUM_REPS;
	MovePos(p, mainLen - 1);
	return mainLen;
  }

  mainDist = 0; /* for GCC */
  if (mainLen >= 2)
  {
	mainDist = matches[numPairs - 1];
	while (numPairs > 2 && mainLen == matches[numPairs - 4] + 1)
	{
	  if (!ChangePair(matches[numPairs - 3], mainDist))
		break;
	  numPairs -= 2;
	  mainLen = matches[numPairs - 2];
	  mainDist = matches[numPairs - 1];
	}
	if (mainLen == 2 && mainDist >= 0x80)
	  mainLen = 1;
  }

  if (repLen >= 2 && (
		(repLen + 1 >= mainLen) ||
		(repLen + 2 >= mainLen && mainDist >= (1 << 9)) ||
		(repLen + 3 >= mainLen && mainDist >= (1 << 15))))
  {
	*backRes = repIndex;
	MovePos(p, repLen - 1);
	return repLen;
  }

  if (mainLen < 2 || numAvail <= 2)
	return 1;

  p->longestMatchLength = ReadMatchDistances(p, &p->numPairs);
  if (p->longestMatchLength >= 2)
  {
	UInt32 newDistance = matches[p->numPairs - 1];
	if ((p->longestMatchLength >= mainLen && newDistance < mainDist) ||
		(p->longestMatchLength == mainLen + 1 && !ChangePair(mainDist, newDistance)) ||
		(p->longestMatchLength > mainLen + 1) ||
		(p->longestMatchLength + 1 >= mainLen && mainLen >= 3 && ChangePair(newDistance, mainDist)))
	  return 1;
  }

  data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
	UInt32 len, limit;
	const Byte *data2 = data - (p->reps[i] + 1);
	if (data[0] != data2[0] || data[1] != data2[1])
	  continue;
	limit = mainLen - 1;
	for (len = 2; len < limit && data[len] == data2[len]; len++);
	if (len >= limit)
	  return 1;
  }
  *backRes = mainDist + LZMA_NUM_REPS;
  MovePos(p, mainLen - 2);
  return mainLen;
}

static void WriteEndMarker(CLzmaEnc *p, UInt32 posState)
{
  UInt32 len;
  RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 1);
  RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
  p->state = kMatchNextStates[p->state];
  len = LZMA_MATCH_LEN_MIN;
  LenEnc_Encode2(&p->lenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, !p->fastMode, p->ProbPrices);
  RcTree_Encode(&p->rc, p->posSlotEncoder[GetLenToPosState(len)], kNumPosSlotBits, (1 << kNumPosSlotBits) - 1);
  RangeEnc_EncodeDirectBits(&p->rc, (((UInt32)1 << 30) - 1) >> kNumAlignBits, 30 - kNumAlignBits);
  RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, kAlignMask);
}

static SRes CheckErrors(CLzmaEnc *p)
{
  if (p->result != SZ_OK)
	return p->result;
  if (p->rc.res != SZ_OK)
	p->result = SZ_ERROR_WRITE;
  if (p->matchFinderBase.result != SZ_OK)
	p->result = SZ_ERROR_READ;
  if (p->result != SZ_OK)
	p->finished = True;
  return p->result;
}

static SRes Flush(CLzmaEnc *p, UInt32 nowPos)
{
  /* ReleaseMFStream(); */
  p->finished = True;
  if (p->writeEndMark)
	WriteEndMarker(p, nowPos & p->pbMask);
  RangeEnc_FlushData(&p->rc);
  RangeEnc_FlushStream(&p->rc);
  return CheckErrors(p);
}

static void FillAlignPrices(CLzmaEnc *p)
{
  UInt32 i;
  for (i = 0; i < kAlignTableSize; i++)
	p->alignPrices[i] = RcTree_ReverseGetPrice(p->posAlignEncoder, kNumAlignBits, i, p->ProbPrices);
  p->alignPriceCount = 0;
}

static void FillDistancesPrices(CLzmaEnc *p)
{
  UInt32 tempPrices[kNumFullDistances];
  UInt32 i, lenToPosState;
  for (i = kStartPosModelIndex; i < kNumFullDistances; i++)
  {
	UInt32 posSlot = GetPosSlot1(i);
	UInt32 footerBits = ((posSlot >> 1) - 1);
	UInt32 base = ((2 | (posSlot & 1)) << footerBits);
	tempPrices[i] = RcTree_ReverseGetPrice(p->posEncoders + base - posSlot - 1, footerBits, i - base, p->ProbPrices);
  }

  for (lenToPosState = 0; lenToPosState < kNumLenToPosStates; lenToPosState++)
  {
	UInt32 posSlot;
	const CLzmaProb *encoder = p->posSlotEncoder[lenToPosState];
	UInt32 *posSlotPrices = p->posSlotPrices[lenToPosState];
	for (posSlot = 0; posSlot < p->distTableSize; posSlot++)
	  posSlotPrices[posSlot] = RcTree_GetPrice(encoder, kNumPosSlotBits, posSlot, p->ProbPrices);
	for (posSlot = kEndPosModelIndex; posSlot < p->distTableSize; posSlot++)
	  posSlotPrices[posSlot] += ((((posSlot >> 1) - 1) - kNumAlignBits) << kNumBitPriceShiftBits);

	{
	  UInt32 *distancesPrices = p->distancesPrices[lenToPosState];
	  UInt32 i;
	  for (i = 0; i < kStartPosModelIndex; i++)
		distancesPrices[i] = posSlotPrices[i];
	  for (; i < kNumFullDistances; i++)
		distancesPrices[i] = posSlotPrices[GetPosSlot1(i)] + tempPrices[i];
	}
  }
  p->matchPriceCount = 0;
}

void LzmaEnc_Construct(CLzmaEnc *p)
{
  RangeEnc_Construct(&p->rc);
  MatchFinder_Construct(&p->matchFinderBase);
  #ifndef _7ZIP_ST
  MatchFinderMt_Construct(&p->matchFinderMt);
  p->matchFinderMt.MatchFinder = &p->matchFinderBase;
  #endif

  {
	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	LzmaEnc_SetProps(p, &props);
  }

  #ifndef LZMA_LOG_BSR
  LzmaEnc_FastPosInit(p->g_FastPos);
  #endif

  LzmaEnc_InitPriceTables(p->ProbPrices);
  p->litProbs = 0;
  p->saveState.litProbs = 0;
}

CLzmaEncHandle LzmaEnc_Create(ISzAlloc *alloc)
{
  void *p;
  p = alloc->Alloc(alloc, sizeof(CLzmaEnc));
  if (p != 0)
	LzmaEnc_Construct((CLzmaEnc *)p);
  return p;
}

void LzmaEnc_FreeLits(CLzmaEnc *p, ISzAlloc *alloc)
{
  alloc->Free(alloc, p->litProbs);
  alloc->Free(alloc, p->saveState.litProbs);
  p->litProbs = 0;
  p->saveState.litProbs = 0;
}

void LzmaEnc_Destruct(CLzmaEnc *p, ISzAlloc *alloc, ISzAlloc *allocBig)
{
  #ifndef _7ZIP_ST
  MatchFinderMt_Destruct(&p->matchFinderMt, allocBig);
  #endif
  MatchFinder_Free(&p->matchFinderBase, allocBig);
  LzmaEnc_FreeLits(p, alloc);
  RangeEnc_Free(&p->rc, alloc);
}

void LzmaEnc_Destroy(CLzmaEncHandle p, ISzAlloc *alloc, ISzAlloc *allocBig)
{
  LzmaEnc_Destruct((CLzmaEnc *)p, alloc, allocBig);
  alloc->Free(alloc, p);
}

static SRes LzmaEnc_CodeOneBlock(CLzmaEnc *p, Bool useLimits, UInt32 maxPackSize, UInt32 maxUnpackSize)
{
  UInt32 nowPos32, startPos32;
  if (p->needInit)
  {
	p->matchFinder.Init(p->matchFinderObj);
	p->needInit = 0;
  }

  if (p->finished)
	return p->result;
  RINOK(CheckErrors(p));

  nowPos32 = (UInt32)p->nowPos64;
  startPos32 = nowPos32;

  if (p->nowPos64 == 0)
  {
	UInt32 numPairs;
	Byte curByte;
	if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
	  return Flush(p, nowPos32);
	ReadMatchDistances(p, &numPairs);
	RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][0], 0);
	p->state = kLiteralNextStates[p->state];
	curByte = p->matchFinder.GetIndexByte(p->matchFinderObj, 0 - p->additionalOffset);
	LitEnc_Encode(&p->rc, p->litProbs, curByte);
	p->additionalOffset--;
	nowPos32++;
  }

  if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) != 0)
  for (;;)
  {
	UInt32 pos, len, posState;

	if (p->fastMode)
	  len = GetOptimumFast(p, &pos);
	else
	  len = GetOptimum(p, nowPos32, &pos);

	#ifdef SHOW_STAT2
	printf("\n pos = %4X,   len = %d   pos = %d", nowPos32, len, pos);
	#endif

	posState = nowPos32 & p->pbMask;
	if (len == 1 && pos == (UInt32)-1)
	{
	  Byte curByte;
	  CLzmaProb *probs;
	  const Byte *data;

	  RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 0);
	  data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
	  curByte = *data;
	  probs = LIT_PROBS(nowPos32, *(data - 1));
	  if (IsCharState(p->state))
		LitEnc_Encode(&p->rc, probs, curByte);
	  else
		LitEnc_EncodeMatched(&p->rc, probs, curByte, *(data - p->reps[0] - 1));
	  p->state = kLiteralNextStates[p->state];
	}
	else
	{
	  RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 1);
	  if (pos < LZMA_NUM_REPS)
	  {
		RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 1);
		if (pos == 0)
		{
		  RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 0);
		  RangeEnc_EncodeBit(&p->rc, &p->isRep0Long[p->state][posState], ((len == 1) ? 0 : 1));
		}
		else
		{
		  UInt32 distance = p->reps[pos];
		  RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 1);
		  if (pos == 1)
			RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 0);
		  else
		  {
			RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 1);
			RangeEnc_EncodeBit(&p->rc, &p->isRepG2[p->state], pos - 2);
			if (pos == 3)
			  p->reps[3] = p->reps[2];
			p->reps[2] = p->reps[1];
		  }
		  p->reps[1] = p->reps[0];
		  p->reps[0] = distance;
		}
		if (len == 1)
		  p->state = kShortRepNextStates[p->state];
		else
		{
		  LenEnc_Encode2(&p->repLenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, !p->fastMode, p->ProbPrices);
		  p->state = kRepNextStates[p->state];
		}
	  }
	  else
	  {
		UInt32 posSlot;
		RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
		p->state = kMatchNextStates[p->state];
		LenEnc_Encode2(&p->lenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, !p->fastMode, p->ProbPrices);
		pos -= LZMA_NUM_REPS;
		GetPosSlot(pos, posSlot);
		RcTree_Encode(&p->rc, p->posSlotEncoder[GetLenToPosState(len)], kNumPosSlotBits, posSlot);

		if (posSlot >= kStartPosModelIndex)
		{
		  UInt32 footerBits = ((posSlot >> 1) - 1);
		  UInt32 base = ((2 | (posSlot & 1)) << footerBits);
		  UInt32 posReduced = pos - base;

		  if (posSlot < kEndPosModelIndex)
			RcTree_ReverseEncode(&p->rc, p->posEncoders + base - posSlot - 1, footerBits, posReduced);
		  else
		  {
			RangeEnc_EncodeDirectBits(&p->rc, posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
			RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, posReduced & kAlignMask);
			p->alignPriceCount++;
		  }
		}
		p->reps[3] = p->reps[2];
		p->reps[2] = p->reps[1];
		p->reps[1] = p->reps[0];
		p->reps[0] = pos;
		p->matchPriceCount++;
	  }
	}
	p->additionalOffset -= len;
	nowPos32 += len;
	if (p->additionalOffset == 0)
	{
	  UInt32 processed;
	  if (!p->fastMode)
	  {
		if (p->matchPriceCount >= (1 << 7))
		  FillDistancesPrices(p);
		if (p->alignPriceCount >= kAlignTableSize)
		  FillAlignPrices(p);
	  }
	  if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
		break;
	  processed = nowPos32 - startPos32;
	  if (useLimits)
	  {
		if (processed + kNumOpts + 300 >= maxUnpackSize ||
			RangeEnc_GetProcessed(&p->rc) + kNumOpts * 2 >= maxPackSize)
		  break;
	  }
	  else if (processed >= (1 << 15))
	  {
		p->nowPos64 += nowPos32 - startPos32;
		return CheckErrors(p);
	  }
	}
  }
  p->nowPos64 += nowPos32 - startPos32;
  return Flush(p, nowPos32);
}

#define kBigHashDicLimit ((UInt32)1 << 24)

static SRes LzmaEnc_Alloc(CLzmaEnc *p, UInt32 keepWindowSize, ISzAlloc *alloc, ISzAlloc *allocBig)
{
  UInt32 beforeSize = kNumOpts;
  Bool btMode;
  if (!RangeEnc_Alloc(&p->rc, alloc))
	return SZ_ERROR_MEM;
  btMode = (p->matchFinderBase.btMode != 0);
  #ifndef _7ZIP_ST
  p->mtMode = (p->multiThread && !p->fastMode && btMode);
  #endif

  {
	unsigned lclp = p->lc + p->lp;
	if (p->litProbs == 0 || p->saveState.litProbs == 0 || p->lclp != lclp)
	{
	  LzmaEnc_FreeLits(p, alloc);
	  p->litProbs = (CLzmaProb *)alloc->Alloc(alloc, (0x300 << lclp) * sizeof(CLzmaProb));
	  p->saveState.litProbs = (CLzmaProb *)alloc->Alloc(alloc, (0x300 << lclp) * sizeof(CLzmaProb));
	  if (p->litProbs == 0 || p->saveState.litProbs == 0)
	  {
		LzmaEnc_FreeLits(p, alloc);
		return SZ_ERROR_MEM;
	  }
	  p->lclp = lclp;
	}
  }

  p->matchFinderBase.bigHash = (p->dictSize > kBigHashDicLimit);

  if (beforeSize + p->dictSize < keepWindowSize)
	beforeSize = keepWindowSize - p->dictSize;

  #ifndef _7ZIP_ST
  if (p->mtMode)
  {
	RINOK(MatchFinderMt_Create(&p->matchFinderMt, p->dictSize, beforeSize, p->numFastBytes, LZMA_MATCH_LEN_MAX, allocBig));
	p->matchFinderObj = &p->matchFinderMt;
	MatchFinderMt_CreateVTable(&p->matchFinderMt, &p->matchFinder);
  }
  else
  #endif
  {
	if (!MatchFinder_Create(&p->matchFinderBase, p->dictSize, beforeSize, p->numFastBytes, LZMA_MATCH_LEN_MAX, allocBig))
	  return SZ_ERROR_MEM;
	p->matchFinderObj = &p->matchFinderBase;
	MatchFinder_CreateVTable(&p->matchFinderBase, &p->matchFinder);
  }
  return SZ_OK;
}

void LzmaEnc_Init(CLzmaEnc *p)
{
  UInt32 i;
  p->state = 0;
  for (i = 0 ; i < LZMA_NUM_REPS; i++)
	p->reps[i] = 0;

  RangeEnc_Init(&p->rc);

  for (i = 0; i < kNumStates; i++)
  {
	UInt32 j;
	for (j = 0; j < LZMA_NUM_PB_STATES_MAX; j++)
	{
	  p->isMatch[i][j] = kProbInitValue;
	  p->isRep0Long[i][j] = kProbInitValue;
	}
	p->isRep[i] = kProbInitValue;
	p->isRepG0[i] = kProbInitValue;
	p->isRepG1[i] = kProbInitValue;
	p->isRepG2[i] = kProbInitValue;
  }

  {
	UInt32 num = 0x300 << (p->lp + p->lc);
	for (i = 0; i < num; i++)
	  p->litProbs[i] = kProbInitValue;
  }

  {
	for (i = 0; i < kNumLenToPosStates; i++)
	{
	  CLzmaProb *probs = p->posSlotEncoder[i];
	  UInt32 j;
	  for (j = 0; j < (1 << kNumPosSlotBits); j++)
		probs[j] = kProbInitValue;
	}
  }
  {
	for (i = 0; i < kNumFullDistances - kEndPosModelIndex; i++)
	  p->posEncoders[i] = kProbInitValue;
  }

  LenEnc_Init(&p->lenEnc.p);
  LenEnc_Init(&p->repLenEnc.p);

  for (i = 0; i < (1 << kNumAlignBits); i++)
	p->posAlignEncoder[i] = kProbInitValue;

  p->optimumEndIndex = 0;
  p->optimumCurrentIndex = 0;
  p->additionalOffset = 0;

  p->pbMask = (1 << p->pb) - 1;
  p->lpMask = (1 << p->lp) - 1;
}

void LzmaEnc_InitPrices(CLzmaEnc *p)
{
  if (!p->fastMode)
  {
	FillDistancesPrices(p);
	FillAlignPrices(p);
  }

  p->lenEnc.tableSize =
  p->repLenEnc.tableSize =
	  p->numFastBytes + 1 - LZMA_MATCH_LEN_MIN;
  LenPriceEnc_UpdateTables(&p->lenEnc, 1 << p->pb, p->ProbPrices);
  LenPriceEnc_UpdateTables(&p->repLenEnc, 1 << p->pb, p->ProbPrices);
}

static SRes LzmaEnc_AllocAndInit(CLzmaEnc *p, UInt32 keepWindowSize, ISzAlloc *alloc, ISzAlloc *allocBig)
{
  UInt32 i;
  for (i = 0; i < (UInt32)kDicLogSizeMaxCompress; i++)
	if (p->dictSize <= ((UInt32)1 << i))
	  break;
  p->distTableSize = i * 2;

  p->finished = False;
  p->result = SZ_OK;
  RINOK(LzmaEnc_Alloc(p, keepWindowSize, alloc, allocBig));
  LzmaEnc_Init(p);
  LzmaEnc_InitPrices(p);
  p->nowPos64 = 0;
  return SZ_OK;
}

static SRes LzmaEnc_Prepare(CLzmaEncHandle pp, ISeqOutStream *outStream, ISeqInStream *inStream,
	ISzAlloc *alloc, ISzAlloc *allocBig)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  p->matchFinderBase.stream = inStream;
  p->needInit = 1;
  p->rc.outStream = outStream;
  return LzmaEnc_AllocAndInit(p, 0, alloc, allocBig);
}

SRes LzmaEnc_PrepareForLzma2(CLzmaEncHandle pp,
	ISeqInStream *inStream, UInt32 keepWindowSize,
	ISzAlloc *alloc, ISzAlloc *allocBig)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  p->matchFinderBase.stream = inStream;
  p->needInit = 1;
  return LzmaEnc_AllocAndInit(p, keepWindowSize, alloc, allocBig);
}

static void LzmaEnc_SetInputBuf(CLzmaEnc *p, const Byte *src, SizeT srcLen)
{
  p->matchFinderBase.directInput = 1;
  p->matchFinderBase.bufferBase = (Byte *)src;
  p->matchFinderBase.directInputRem = srcLen;
}

SRes LzmaEnc_MemPrepare(CLzmaEncHandle pp, const Byte *src, SizeT srcLen,
	UInt32 keepWindowSize, ISzAlloc *alloc, ISzAlloc *allocBig)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  LzmaEnc_SetInputBuf(p, src, srcLen);
  p->needInit = 1;

  return LzmaEnc_AllocAndInit(p, keepWindowSize, alloc, allocBig);
}

void LzmaEnc_Finish(CLzmaEncHandle pp)
{
  #ifndef _7ZIP_ST
  CLzmaEnc *p = (CLzmaEnc *)pp;
  if (p->mtMode)
	MatchFinderMt_ReleaseStream(&p->matchFinderMt);
  #else
  pp = pp;
  #endif
}

typedef struct
{
  ISeqOutStream funcTable;
  Byte *data;
  SizeT rem;
  Bool overflow;
} CSeqOutStreamBuf;

static size_t MyWrite(void *pp, const void *data, size_t size)
{
  CSeqOutStreamBuf *p = (CSeqOutStreamBuf *)pp;
  if (p->rem < size)
  {
	size = p->rem;
	p->overflow = True;
  }
  memcpy(p->data, data, size);
  p->rem -= size;
  p->data += size;
  return size;
}

UInt32 LzmaEnc_GetNumAvailableBytes(CLzmaEncHandle pp)
{
  const CLzmaEnc *p = (CLzmaEnc *)pp;
  return p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
}

const Byte *LzmaEnc_GetCurBuf(CLzmaEncHandle pp)
{
  const CLzmaEnc *p = (CLzmaEnc *)pp;
  return p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
}

SRes LzmaEnc_CodeOneMemBlock(CLzmaEncHandle pp, Bool reInit,
	Byte *dest, size_t *destLen, UInt32 desiredPackSize, UInt32 *unpackSize)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  UInt64 nowPos64;
  SRes res;
  CSeqOutStreamBuf outStream;

  outStream.funcTable.Write = MyWrite;
  outStream.data = dest;
  outStream.rem = *destLen;
  outStream.overflow = False;

  p->writeEndMark = False;
  p->finished = False;
  p->result = SZ_OK;

  if (reInit)
	LzmaEnc_Init(p);
  LzmaEnc_InitPrices(p);
  nowPos64 = p->nowPos64;
  RangeEnc_Init(&p->rc);
  p->rc.outStream = &outStream.funcTable;

  res = LzmaEnc_CodeOneBlock(p, True, desiredPackSize, *unpackSize);

  *unpackSize = (UInt32)(p->nowPos64 - nowPos64);
  *destLen -= outStream.rem;
  if (outStream.overflow)
	return SZ_ERROR_OUTPUT_EOF;

  return res;
}

static SRes LzmaEnc_Encode2(CLzmaEnc *p, ICompressProgress *progress)
{
  SRes res = SZ_OK;

  #ifndef _7ZIP_ST
  Byte allocaDummy[0x300];
  int i = 0;
  for (i = 0; i < 16; i++)
	allocaDummy[i] = (Byte)i;
  #endif

  for (;;)
  {
	res = LzmaEnc_CodeOneBlock(p, False, 0, 0);
	if (res != SZ_OK || p->finished != 0)
	  break;
	if (progress != 0)
	{
	  res = progress->Progress(progress, p->nowPos64, RangeEnc_GetProcessed(&p->rc));
	  if (res != SZ_OK)
	  {
		res = SZ_ERROR_PROGRESS;
		break;
	  }
	}
  }
  LzmaEnc_Finish(p);
  return res;
}

SRes LzmaEnc_Encode(CLzmaEncHandle pp, ISeqOutStream *outStream, ISeqInStream *inStream, ICompressProgress *progress,
	ISzAlloc *alloc, ISzAlloc *allocBig)
{
  RINOK(LzmaEnc_Prepare(pp, outStream, inStream, alloc, allocBig));
  return LzmaEnc_Encode2((CLzmaEnc *)pp, progress);
}

SRes LzmaEnc_WriteProperties(CLzmaEncHandle pp, Byte *props, SizeT *size)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  int i;
  UInt32 dictSize = p->dictSize;
  if (*size < LZMA_PROPS_SIZE)
	return SZ_ERROR_PARAM;
  *size = LZMA_PROPS_SIZE;
  props[0] = (Byte)((p->pb * 5 + p->lp) * 9 + p->lc);

  for (i = 11; i <= 30; i++)
  {
	if (dictSize <= ((UInt32)2 << i))
	{
	  dictSize = (2 << i);
	  break;
	}
	if (dictSize <= ((UInt32)3 << i))
	{
	  dictSize = (3 << i);
	  break;
	}
  }

  for (i = 0; i < 4; i++)
	props[1 + i] = (Byte)(dictSize >> (8 * i));
  return SZ_OK;
}

SRes LzmaEnc_MemEncode(CLzmaEncHandle pp, Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
	int writeEndMark, ICompressProgress *progress, ISzAlloc *alloc, ISzAlloc *allocBig)
{
  SRes res;
  CLzmaEnc *p = (CLzmaEnc *)pp;

  CSeqOutStreamBuf outStream;

  LzmaEnc_SetInputBuf(p, src, srcLen);

  outStream.funcTable.Write = MyWrite;
  outStream.data = dest;
  outStream.rem = *destLen;
  outStream.overflow = False;

  p->writeEndMark = writeEndMark;

  p->rc.outStream = &outStream.funcTable;
  res = LzmaEnc_MemPrepare(pp, src, srcLen, 0, alloc, allocBig);
  if (res == SZ_OK)
	res = LzmaEnc_Encode2(p, progress);

  *destLen -= outStream.rem;
  if (outStream.overflow)
	return SZ_ERROR_OUTPUT_EOF;
  return res;
}

SRes LzmaEncode(Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
	const CLzmaEncProps *props, Byte *propsEncoded, SizeT *propsSize, int writeEndMark,
	ICompressProgress *progress, ISzAlloc *alloc, ISzAlloc *allocBig)
{
  CLzmaEnc *p = (CLzmaEnc *)LzmaEnc_Create(alloc);
  SRes res;
  if (p == 0)
	return SZ_ERROR_MEM;

  res = LzmaEnc_SetProps(p, props);
  if (res == SZ_OK)
  {
	res = LzmaEnc_WriteProperties(p, propsEncoded, propsSize);
	if (res == SZ_OK)
	  res = LzmaEnc_MemEncode(p, dest, destLen, src, srcLen,
		  writeEndMark, progress, alloc, allocBig);
  }

  LzmaEnc_Destroy(p, alloc, allocBig);
  return res;
}


#line 3 "LzmaLib.c"

#line 3 "LzmaLib.h"
#ifndef __LZMA_LIB_H
#define __LZMA_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#define MY_STDAPI int MY_STD_CALL

#define LZMA_PROPS_SIZE 5

/*
RAM requirements for LZMA:
  for compression:   (dictSize * 11.5 + 6 MB) + state_size
  for decompression: dictSize + state_size
	state_size = (4 + (1.5 << (lc + lp))) KB
	by default (lc=3, lp=0), state_size = 16 KB.

LZMA properties (5 bytes) format
	Offset Size  Description
	  0     1    lc, lp and pb in encoded form.
	  1     4    dictSize (little endian).
*/

/*
LzmaCompress
------------

outPropsSize -
	 In:  the pointer to the size of outProps buffer; *outPropsSize = LZMA_PROPS_SIZE = 5.
	 Out: the pointer to the size of written properties in outProps buffer; *outPropsSize = LZMA_PROPS_SIZE = 5.

  LZMA Encoder will use defult values for any parameter, if it is
  -1  for any from: level, loc, lp, pb, fb, numThreads
   0  for dictSize

level - compression level: 0 <= level <= 9;

  level dictSize algo  fb
	0:    16 KB   0    32
	1:    64 KB   0    32
	2:   256 KB   0    32
	3:     1 MB   0    32
	4:     4 MB   0    32
	5:    16 MB   1    32
	6:    32 MB   1    32
	7+:   64 MB   1    64

  The default value for "level" is 5.

  algo = 0 means fast method
  algo = 1 means normal method

dictSize - The dictionary size in bytes. The maximum value is
		128 MB = (1 << 27) bytes for 32-bit version
		  1 GB = (1 << 30) bytes for 64-bit version
	 The default value is 16 MB = (1 << 24) bytes.
	 It's recommended to use the dictionary that is larger than 4 KB and
	 that can be calculated as (1 << N) or (3 << N) sizes.

lc - The number of literal context bits (high bits of previous literal).
	 It can be in the range from 0 to 8. The default value is 3.
	 Sometimes lc=4 gives the gain for big files.

lp - The number of literal pos bits (low bits of current position for literals).
	 It can be in the range from 0 to 4. The default value is 0.
	 The lp switch is intended for periodical data when the period is equal to 2^lp.
	 For example, for 32-bit (4 bytes) periodical data you can use lp=2. Often it's
	 better to set lc=0, if you change lp switch.

pb - The number of pos bits (low bits of current position).
	 It can be in the range from 0 to 4. The default value is 2.
	 The pb switch is intended for periodical data when the period is equal 2^pb.

fb - Word size (the number of fast bytes).
	 It can be in the range from 5 to 273. The default value is 32.
	 Usually, a big number gives a little bit better compression ratio and
	 slower compression process.

numThreads - The number of thereads. 1 or 2. The default value is 2.
	 Fast mode (algo = 0) can use only 1 thread.

Out:
  destLen  - processed output size
Returns:
  SZ_OK               - OK
  SZ_ERROR_MEM        - Memory allocation error
  SZ_ERROR_PARAM      - Incorrect paramater
  SZ_ERROR_OUTPUT_EOF - output buffer overflow
  SZ_ERROR_THREAD     - errors in multithreading functions (only for Mt version)
*/

MY_STDAPI LzmaCompress(unsigned char *dest, size_t *destLen, const unsigned char *src, size_t srcLen,
  unsigned char *outProps, size_t *outPropsSize, /* *outPropsSize must be = 5 */
  int level,      /* 0 <= level <= 9, default = 5 */
  unsigned dictSize,  /* default = (1 << 24) */
  int lc,        /* 0 <= lc <= 8, default = 3  */
  int lp,        /* 0 <= lp <= 4, default = 0  */
  int pb,        /* 0 <= pb <= 4, default = 2  */
  int fb,        /* 5 <= fb <= 273, default = 32 */
  int numThreads /* 1 or 2, default = 2 */
  );

/*
LzmaUncompress
--------------
In:
  dest     - output data
  destLen  - output data size
  src      - input data
  srcLen   - input data size
Out:
  destLen  - processed output size
  srcLen   - processed input size
Returns:
  SZ_OK                - OK
  SZ_ERROR_DATA        - Data error
  SZ_ERROR_MEM         - Memory allocation arror
  SZ_ERROR_UNSUPPORTED - Unsupported properties
  SZ_ERROR_INPUT_EOF   - it needs more bytes in input buffer (src)
*/

MY_STDAPI LzmaUncompress(unsigned char *dest, size_t *destLen, const unsigned char *src, SizeT *srcLen,
  const unsigned char *props, size_t propsSize);

#ifdef __cplusplus
}
#endif

#endif

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

MY_STDAPI LzmaCompress(unsigned char *dest, size_t  *destLen, const unsigned char *src, size_t  srcLen,
  unsigned char *outProps, size_t *outPropsSize,
  int level, /* 0 <= level <= 9, default = 5 */
  unsigned dictSize, /* use (1 << N) or (3 << N). 4 KB < dictSize <= 128 MB */
  int lc, /* 0 <= lc <= 8, default = 3  */
  int lp, /* 0 <= lp <= 4, default = 0  */
  int pb, /* 0 <= pb <= 4, default = 2  */
  int fb,  /* 5 <= fb <= 273, default = 32 */
  int numThreads /* 1 or 2, default = 2 */
)
{
  CLzmaEncProps props;
  LzmaEncProps_Init(&props);
  props.level = level;
  props.dictSize = dictSize;
  props.lc = lc;
  props.lp = lp;
  props.pb = pb;
  props.fb = fb;
  props.numThreads = numThreads;

  return LzmaEncode(dest, destLen, src, srcLen, &props, outProps, outPropsSize, 0,
	  NULL, &g_Alloc, &g_Alloc);
}

MY_STDAPI LzmaUncompress(unsigned char *dest, size_t  *destLen, const unsigned char *src, size_t  *srcLen,
  const unsigned char *props, size_t propsSize)
{
  ELzmaStatus status;
  return LzmaDecode(dest, destLen, src, srcLen, props, (unsigned)propsSize, LZMA_FINISH_ANY, &status, &g_Alloc);
}

#undef NORMALIZE
#undef IF_BIT_0
#undef UPDATE_0
#undef UPDATE_1
#undef kNumFullDistances
#undef kTopValue

#line 3 "Bcj2.c"

#line 3 "Bcj2.h"
#ifndef __BCJ2_H
#define __BCJ2_H

#ifdef __cplusplus
extern "C" {
#endif

/*
Conditions:
  outSize <= FullOutputSize,
  where FullOutputSize is full size of output stream of x86_2 filter.

If buf0 overlaps outBuf, there are two required conditions:
  1) (buf0 >= outBuf)
  2) (buf0 + size0 >= outBuf + FullOutputSize).

Returns:
  SZ_OK
  SZ_ERROR_DATA - Data error
*/

int Bcj2_Decode(
	const Byte *buf0, SizeT size0,
	const Byte *buf1, SizeT size1,
	const Byte *buf2, SizeT size2,
	const Byte *buf3, SizeT size3,
	Byte *outBuf, SizeT outSize);

#ifdef __cplusplus
}
#endif

#endif

#ifdef _LZMA_PROB32
#define CProb UInt32
#else
#define CProb UInt16
#endif

#define IsJcc(b0, b1) ((b0) == 0x0F && ((b1) & 0xF0) == 0x80)
#define IsJ(b0, b1) ((b1 & 0xFE) == 0xE8 || IsJcc(b0, b1))

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_READ_BYTE (*buffer++)
#define RC_TEST { if (buffer == bufferLim) return SZ_ERROR_DATA; }
#define RC_INIT2 code = 0; range = 0xFFFFFFFF; \
  { int i; for (i = 0; i < 5; i++) { RC_TEST; code = (code << 8) | RC_READ_BYTE; }}

#define NORMALIZE if (range < kTopValue) { RC_TEST; range <<= 8; code = (code << 8) | RC_READ_BYTE; }

#define IF_BIT_0(p) ttt = *(p); bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)); NORMALIZE;
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CProb)(ttt - (ttt >> kNumMoveBits)); NORMALIZE;

int Bcj2_Decode(
	const Byte *buf0, SizeT size0,
	const Byte *buf1, SizeT size1,
	const Byte *buf2, SizeT size2,
	const Byte *buf3, SizeT size3,
	Byte *outBuf, SizeT outSize)
{
  CProb p[256 + 2];
  SizeT inPos = 0, outPos = 0;

  const Byte *buffer, *bufferLim;
  UInt32 range, code;
  Byte prevByte = 0;

  unsigned int i;
  for (i = 0; i < sizeof(p) / sizeof(p[0]); i++)
	p[i] = kBitModelTotal >> 1;

  buffer = buf3;
  bufferLim = buffer + size3;
  RC_INIT2

  if (outSize == 0)
	return SZ_OK;

  for (;;)
  {
	Byte b;
	CProb *prob;
	UInt32 bound;
	UInt32 ttt;

	SizeT limit = size0 - inPos;
	if (outSize - outPos < limit)
	  limit = outSize - outPos;
	while (limit != 0)
	{
	  Byte b = buf0[inPos];
	  outBuf[outPos++] = b;
	  if (IsJ(prevByte, b))
		break;
	  inPos++;
	  prevByte = b;
	  limit--;
	}

	if (limit == 0 || outPos == outSize)
	  break;

	b = buf0[inPos++];

	if (b == 0xE8)
	  prob = p + prevByte;
	else if (b == 0xE9)
	  prob = p + 256;
	else
	  prob = p + 257;

	IF_BIT_0(prob)
	{
	  UPDATE_0(prob)
	  prevByte = b;
	}
	else
	{
	  UInt32 dest;
	  const Byte *v;
	  UPDATE_1(prob)
	  if (b == 0xE8)
	  {
		v = buf1;
		if (size1 < 4)
		  return SZ_ERROR_DATA;
		buf1 += 4;
		size1 -= 4;
	  }
	  else
	  {
		v = buf2;
		if (size2 < 4)
		  return SZ_ERROR_DATA;
		buf2 += 4;
		size2 -= 4;
	  }
	  dest = (((UInt32)v[0] << 24) | ((UInt32)v[1] << 16) |
		  ((UInt32)v[2] << 8) | ((UInt32)v[3])) - ((UInt32)outPos + 4);
	  outBuf[outPos++] = (Byte)dest;
	  if (outPos == outSize)
		break;
	  outBuf[outPos++] = (Byte)(dest >> 8);
	  if (outPos == outSize)
		break;
	  outBuf[outPos++] = (Byte)(dest >> 16);
	  if (outPos == outSize)
		break;
	  outBuf[outPos++] = prevByte = (Byte)(dest >> 24);
	}
  }
  return (outPos == outSize) ? SZ_OK : SZ_ERROR_DATA;
}


#endif

#if 1
// zpaq
#if defined __X86__ || defined __i386__ || defined i386 || defined _M_IX86 || defined __386__ || defined __x86_64__ || defined _M_X64
#   define BUNDLE_CPU_X86 1
#   if defined __x86_64__ || defined _M_X64
#       define BUNDLE_CPU_X86_64 1
#   endif
#endif
#if !defined(BUNDLE_CPU_X86)
#   define NOJIT
#endif
#if !defined(_WIN32)
#   if !defined(unix)
#       define unix
#   endif
#endif
//#include "deps/zpaq/divsufsort.h"
//#include "deps/zpaq/divsufsort.c"

#line 3 "libzpaq.cpp"
#include <string.h>

#ifndef NOJIT
#ifdef unix
#include <sys/mman.h>
#else
#include <windows.h>
#endif
#endif

namespace libzpaq {

// Read 16 bit little-endian number
int toU16(const char* p) {
  return (p[0]&255)+256*(p[1]&255);
}

// Default read() and write()
int Reader::read(char* buf, int n) {
  int i=0, c;
  while (i<n && (c=get())>=0)
	buf[i++]=c;
  return i;
}

void Writer::write(const char* buf, int n) {
  for (int i=0; i<n; ++i)
	put(U8(buf[i]));
}

///////////////////////// allocx //////////////////////

// Allocate newsize > 0 bytes of executable memory and update
// p to point to it and newsize = n. Free any previously
// allocated memory first. If newsize is 0 then free only.
// Call error in case of failure. If NOJIT, ignore newsize
// and set p=0, n=0 without allocating memory.
void allocx(U8* &p, int &n, int newsize) {
#ifdef NOJIT
  p=0;
  n=0;
#else
  if (p || n) {
	if (p)
#ifdef unix
	  munmap(p, n);
#else // Windows
	  VirtualFree(p, 0, MEM_RELEASE);
#endif
	p=0;
	n=0;
  }
  if (newsize>0) {
#ifdef unix
	p=(U8*)mmap(0, newsize, PROT_READ|PROT_WRITE|PROT_EXEC,
				MAP_PRIVATE|MAP_ANON, -1, 0);
	if ((void*)p==MAP_FAILED) p=0;
#else
	p=(U8*)VirtualAlloc(0, newsize, MEM_RESERVE|MEM_COMMIT,
						PAGE_EXECUTE_READWRITE);
#endif
	if (p)
	  n=newsize;
	else {
	  n=0;
	  error("allocx failed");
	}
  }
#endif
}

//////////////////////////// SHA1 ////////////////////////////

// SHA1 code, see http://en.wikipedia.org/wiki/SHA-1

// Start a new hash
void SHA1::init() {
  len0=len1=0;
  h[0]=0x67452301;
  h[1]=0xEFCDAB89;
  h[2]=0x98BADCFE;
  h[3]=0x10325476;
  h[4]=0xC3D2E1F0;
  memset(w, 0, sizeof(w));
}

// Return old result and start a new hash
const char* SHA1::result() {

  // pad and append length
  const U32 s1=len1, s0=len0;
  put(0x80);
  while ((len0&511)!=448)
	put(0);
  put(s1>>24);
  put(s1>>16);
  put(s1>>8);
  put(s1);
  put(s0>>24);
  put(s0>>16);
  put(s0>>8);
  put(s0);

  // copy h to hbuf
  for (int i=0; i<5; ++i) {
	hbuf[4*i]=h[i]>>24;
	hbuf[4*i+1]=h[i]>>16;
	hbuf[4*i+2]=h[i]>>8;
	hbuf[4*i+3]=h[i];
  }

  // return hash prior to clearing state
  init();
  return hbuf;
}

// Hash 1 block of 64 bytes
void SHA1::process() {
  U32 a=h[0], b=h[1], c=h[2], d=h[3], e=h[4];
  static const U32 k[4]={0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
  #define f(a,b,c,d,e,i) \
	if (i>=16) \
	  w[(i)&15]^=w[(i-3)&15]^w[(i-8)&15]^w[(i-14)&15], \
	  w[(i)&15]=w[(i)&15]<<1|w[(i)&15]>>31; \
	e+=(a<<5|a>>27)+k[(i)/20]+w[(i)&15] \
	  +((i)%40>=20 ? b^c^d : i>=40 ? (b&c)|(d&(b|c)) : d^(b&(c^d))); \
	b=b<<30|b>>2;
  #define r(i) f(a,b,c,d,e,i) f(e,a,b,c,d,i+1) f(d,e,a,b,c,i+2) \
			   f(c,d,e,a,b,i+3) f(b,c,d,e,a,i+4)
  r(0)  r(5)  r(10) r(15) r(20) r(25) r(30) r(35)
  r(40) r(45) r(50) r(55) r(60) r(65) r(70) r(75)
  #undef f
  #undef r
  h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e;
}

//////////////////////////// SHA256 //////////////////////////

void SHA256::init() {
  len0=len1=0;
  s[0]=0x6a09e667;
  s[1]=0xbb67ae85;
  s[2]=0x3c6ef372;
  s[3]=0xa54ff53a;
  s[4]=0x510e527f;
  s[5]=0x9b05688c;
  s[6]=0x1f83d9ab;
  s[7]=0x5be0cd19;
  memset(w, 0, sizeof(w));
}

void SHA256::process() {

  #define ror(a,b) ((a)>>(b)|(a<<(32-(b))))

  #define m(i) \
	 w[(i)&15]+=w[(i-7)&15] \
	   +(ror(w[(i-15)&15],7)^ror(w[(i-15)&15],18)^(w[(i-15)&15]>>3)) \
	   +(ror(w[(i-2)&15],17)^ror(w[(i-2)&15],19)^(w[(i-2)&15]>>10))

  #define r(a,b,c,d,e,f,g,h,i) { \
	unsigned t1=ror(e,14)^e; \
	t1=ror(t1,5)^e; \
	h+=ror(t1,6)+((e&f)^(~e&g))+k[i]+w[(i)&15]; } \
	d+=h; \
	{unsigned t1=ror(a,9)^a; \
	t1=ror(t1,11)^a; \
	h+=ror(t1,2)+((a&b)^(c&(a^b))); }

  #define mr(a,b,c,d,e,f,g,h,i) m(i); r(a,b,c,d,e,f,g,h,i);

  #define r8(i) \
	r(a,b,c,d,e,f,g,h,i);   \
	r(h,a,b,c,d,e,f,g,i+1); \
	r(g,h,a,b,c,d,e,f,i+2); \
	r(f,g,h,a,b,c,d,e,i+3); \
	r(e,f,g,h,a,b,c,d,i+4); \
	r(d,e,f,g,h,a,b,c,i+5); \
	r(c,d,e,f,g,h,a,b,i+6); \
	r(b,c,d,e,f,g,h,a,i+7);

  #define mr8(i) \
	mr(a,b,c,d,e,f,g,h,i);   \
	mr(h,a,b,c,d,e,f,g,i+1); \
	mr(g,h,a,b,c,d,e,f,i+2); \
	mr(f,g,h,a,b,c,d,e,i+3); \
	mr(e,f,g,h,a,b,c,d,i+4); \
	mr(d,e,f,g,h,a,b,c,i+5); \
	mr(c,d,e,f,g,h,a,b,i+6); \
	mr(b,c,d,e,f,g,h,a,i+7);

  static const unsigned k[64]={
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

  unsigned a=s[0];
  unsigned b=s[1];
  unsigned c=s[2];
  unsigned d=s[3];
  unsigned e=s[4];
  unsigned f=s[5];
  unsigned g=s[6];
  unsigned h=s[7];

  r8(0);
  r8(8);
  mr8(16);
  mr8(24);
  mr8(32);
  mr8(40);
  mr8(48);
  mr8(56);

  s[0]+=a;
  s[1]+=b;
  s[2]+=c;
  s[3]+=d;
  s[4]+=e;
  s[5]+=f;
  s[6]+=g;
  s[7]+=h;

  #undef mr8
  #undef r8
  #undef mr
  #undef r
  #undef m
  #undef ror
};

// Return old result and start a new hash
const char* SHA256::result() {

  // pad and append length
  const unsigned s1=len1, s0=len0;
  put(0x80);
  while ((len0&511)!=448) put(0);
  put(s1>>24);
  put(s1>>16);
  put(s1>>8);
  put(s1);
  put(s0>>24);
  put(s0>>16);
  put(s0>>8);
  put(s0);

  // copy s to hbuf
  for (int i=0; i<8; ++i) {
	hbuf[4*i]=s[i]>>24;
	hbuf[4*i+1]=s[i]>>16;
	hbuf[4*i+2]=s[i]>>8;
	hbuf[4*i+3]=s[i];
  }

  // return hash prior to clearing state
  init();
  return hbuf;
}

//////////////////////////// AES /////////////////////////////

// Some AES code is derived from libtomcrypt 1.17 (public domain).

#define Te4_0 0x000000FF & Te4
#define Te4_1 0x0000FF00 & Te4
#define Te4_2 0x00FF0000 & Te4
#define Te4_3 0xFF000000 & Te4

// Extract byte n of x
static inline unsigned byte(unsigned x, unsigned n) {return (x>>(8*n))&255;}

// x = y[0..3] MSB first
static inline void LOAD32H(U32& x, const char* y) {
  const unsigned char* u=(const unsigned char*)y;
  x=u[0]<<24|u[1]<<16|u[2]<<8|u[3];
}

// y[0..3] = x MSB first
static inline void STORE32H(U32& x, unsigned char* y) {
  y[0]=x>>24;
  y[1]=x>>16;
  y[2]=x>>8;
  y[3]=x;
}

#define setup_mix(temp) \
  ((Te4_3[byte(temp, 2)]) ^ (Te4_2[byte(temp, 1)]) ^ \
   (Te4_1[byte(temp, 0)]) ^ (Te4_0[byte(temp, 3)]))

// Initialize encryption tables and round key. keylen is 16, 24, or 32.
AES_CTR::AES_CTR(const char* key, int keylen, const char* iv) {
  assert(key  != NULL);
  assert(keylen==16 || keylen==24 || keylen==32);

  // Initialize IV (default 0)
  iv0=iv1=0;
  if (iv) {
	LOAD32H(iv0, iv);
	LOAD32H(iv1, iv+4);
  }

  // Initialize encryption tables
  for (int i=0; i<256; ++i) {
	unsigned s1=
	"\x63\x7c\x77\x7b\xf2\x6b\x6f\xc5\x30\x01\x67\x2b\xfe\xd7\xab\x76"
	"\xca\x82\xc9\x7d\xfa\x59\x47\xf0\xad\xd4\xa2\xaf\x9c\xa4\x72\xc0"
	"\xb7\xfd\x93\x26\x36\x3f\xf7\xcc\x34\xa5\xe5\xf1\x71\xd8\x31\x15"
	"\x04\xc7\x23\xc3\x18\x96\x05\x9a\x07\x12\x80\xe2\xeb\x27\xb2\x75"
	"\x09\x83\x2c\x1a\x1b\x6e\x5a\xa0\x52\x3b\xd6\xb3\x29\xe3\x2f\x84"
	"\x53\xd1\x00\xed\x20\xfc\xb1\x5b\x6a\xcb\xbe\x39\x4a\x4c\x58\xcf"
	"\xd0\xef\xaa\xfb\x43\x4d\x33\x85\x45\xf9\x02\x7f\x50\x3c\x9f\xa8"
	"\x51\xa3\x40\x8f\x92\x9d\x38\xf5\xbc\xb6\xda\x21\x10\xff\xf3\xd2"
	"\xcd\x0c\x13\xec\x5f\x97\x44\x17\xc4\xa7\x7e\x3d\x64\x5d\x19\x73"
	"\x60\x81\x4f\xdc\x22\x2a\x90\x88\x46\xee\xb8\x14\xde\x5e\x0b\xdb"
	"\xe0\x32\x3a\x0a\x49\x06\x24\x5c\xc2\xd3\xac\x62\x91\x95\xe4\x79"
	"\xe7\xc8\x37\x6d\x8d\xd5\x4e\xa9\x6c\x56\xf4\xea\x65\x7a\xae\x08"
	"\xba\x78\x25\x2e\x1c\xa6\xb4\xc6\xe8\xdd\x74\x1f\x4b\xbd\x8b\x8a"
	"\x70\x3e\xb5\x66\x48\x03\xf6\x0e\x61\x35\x57\xb9\x86\xc1\x1d\x9e"
	"\xe1\xf8\x98\x11\x69\xd9\x8e\x94\x9b\x1e\x87\xe9\xce\x55\x28\xdf"
	"\x8c\xa1\x89\x0d\xbf\xe6\x42\x68\x41\x99\x2d\x0f\xb0\x54\xbb\x16"
	[i]&255;
	unsigned s2=s1<<1;
	if (s2>=0x100) s2^=0x11b;
	unsigned s3=s1^s2;
	Te0[i]=s2<<24|s1<<16|s1<<8|s3;
	Te1[i]=s3<<24|s2<<16|s1<<8|s1;
	Te2[i]=s1<<24|s3<<16|s2<<8|s1;
	Te3[i]=s1<<24|s1<<16|s3<<8|s2;
	Te4[i]=s1<<24|s1<<16|s1<<8|s1;
  }

  // setup the forward key
  Nr = 10 + ((keylen/8)-2)*2;  // 10, 12, or 14 rounds
  int i = 0;
  U32* rk = &ek[0];
  U32 temp;
  static const U32 rcon[10] = {
	0x01000000UL, 0x02000000UL, 0x04000000UL, 0x08000000UL,
	0x10000000UL, 0x20000000UL, 0x40000000UL, 0x80000000UL,
	0x1B000000UL, 0x36000000UL};  // round constants

  LOAD32H(rk[0], key   );
  LOAD32H(rk[1], key +  4);
  LOAD32H(rk[2], key +  8);
  LOAD32H(rk[3], key + 12);
  if (keylen == 16) {
	for (;;) {
	  temp  = rk[3];
	  rk[4] = rk[0] ^ setup_mix(temp) ^ rcon[i];
	  rk[5] = rk[1] ^ rk[4];
	  rk[6] = rk[2] ^ rk[5];
	  rk[7] = rk[3] ^ rk[6];
	  if (++i == 10) {
		 break;
	  }
	  rk += 4;
	}
  }
  else if (keylen == 24) {
	LOAD32H(rk[4], key + 16);
	LOAD32H(rk[5], key + 20);
	for (;;) {
	  temp = rk[5];
	  rk[ 6] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
	  rk[ 7] = rk[ 1] ^ rk[ 6];
	  rk[ 8] = rk[ 2] ^ rk[ 7];
	  rk[ 9] = rk[ 3] ^ rk[ 8];
	  if (++i == 8) {
		break;
	  }
	  rk[10] = rk[ 4] ^ rk[ 9];
	  rk[11] = rk[ 5] ^ rk[10];
	  rk += 6;
	}
  }
  else if (keylen == 32) {
	LOAD32H(rk[4], key + 16);
	LOAD32H(rk[5], key + 20);
	LOAD32H(rk[6], key + 24);
	LOAD32H(rk[7], key + 28);
	for (;;) {
	  temp = rk[7];
	  rk[ 8] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
	  rk[ 9] = rk[ 1] ^ rk[ 8];
	  rk[10] = rk[ 2] ^ rk[ 9];
	  rk[11] = rk[ 3] ^ rk[10];
	  if (++i == 7) {
		break;
	  }
	  temp = rk[11];
	  rk[12] = rk[ 4] ^ setup_mix(temp<<24|temp>>8);
	  rk[13] = rk[ 5] ^ rk[12];
	  rk[14] = rk[ 6] ^ rk[13];
	  rk[15] = rk[ 7] ^ rk[14];
	  rk += 8;
	}
  }
}

// Encrypt to ct[16]
void AES_CTR::encrypt(U32 s0, U32 s1, U32 s2, U32 s3, unsigned char* ct) {
  int r = Nr >> 1;
  U32 *rk = &ek[0];
  U32 t0=0, t1=0, t2=0, t3=0;
  s0 ^= rk[0];
  s1 ^= rk[1];
  s2 ^= rk[2];
  s3 ^= rk[3];
  for (;;) {
	t0 =
	  Te0[byte(s0, 3)] ^
	  Te1[byte(s1, 2)] ^
	  Te2[byte(s2, 1)] ^
	  Te3[byte(s3, 0)] ^
	  rk[4];
	t1 =
	  Te0[byte(s1, 3)] ^
	  Te1[byte(s2, 2)] ^
	  Te2[byte(s3, 1)] ^
	  Te3[byte(s0, 0)] ^
	  rk[5];
	t2 =
	  Te0[byte(s2, 3)] ^
	  Te1[byte(s3, 2)] ^
	  Te2[byte(s0, 1)] ^
	  Te3[byte(s1, 0)] ^
	  rk[6];
	t3 =
	  Te0[byte(s3, 3)] ^
	  Te1[byte(s0, 2)] ^
	  Te2[byte(s1, 1)] ^
	  Te3[byte(s2, 0)] ^
	  rk[7];

	rk += 8;
	if (--r == 0) {
	  break;
	}

	s0 =
	  Te0[byte(t0, 3)] ^
	  Te1[byte(t1, 2)] ^
	  Te2[byte(t2, 1)] ^
	  Te3[byte(t3, 0)] ^
	  rk[0];
	s1 =
	  Te0[byte(t1, 3)] ^
	  Te1[byte(t2, 2)] ^
	  Te2[byte(t3, 1)] ^
	  Te3[byte(t0, 0)] ^
	  rk[1];
	s2 =
	  Te0[byte(t2, 3)] ^
	  Te1[byte(t3, 2)] ^
	  Te2[byte(t0, 1)] ^
	  Te3[byte(t1, 0)] ^
	  rk[2];
	s3 =
	  Te0[byte(t3, 3)] ^
	  Te1[byte(t0, 2)] ^
	  Te2[byte(t1, 1)] ^
	  Te3[byte(t2, 0)] ^
	  rk[3];
  }

  // apply last round and map cipher state to byte array block:
  s0 =
	(Te4_3[byte(t0, 3)]) ^
	(Te4_2[byte(t1, 2)]) ^
	(Te4_1[byte(t2, 1)]) ^
	(Te4_0[byte(t3, 0)]) ^
	rk[0];
  STORE32H(s0, ct);
  s1 =
	(Te4_3[byte(t1, 3)]) ^
	(Te4_2[byte(t2, 2)]) ^
	(Te4_1[byte(t3, 1)]) ^
	(Te4_0[byte(t0, 0)]) ^
	rk[1];
  STORE32H(s1, ct+4);
  s2 =
	(Te4_3[byte(t2, 3)]) ^
	(Te4_2[byte(t3, 2)]) ^
	(Te4_1[byte(t0, 1)]) ^
	(Te4_0[byte(t1, 0)]) ^
	rk[2];
  STORE32H(s2, ct+8);
  s3 =
	(Te4_3[byte(t3, 3)]) ^
	(Te4_2[byte(t0, 2)]) ^
	(Te4_1[byte(t1, 1)]) ^
	(Te4_0[byte(t2, 0)]) ^
	rk[3];
  STORE32H(s3, ct+12);
}

// Encrypt or decrypt slice buf[0..n-1] at offset by XOR with AES(i) where
// i is the 128 bit big-endian distance from the start in 16 byte blocks.
void AES_CTR::encrypt(char* buf, int n, U64 offset) {
  for (U64 i=offset/16; i<=(offset+n)/16; ++i) {
	unsigned char ct[16];
	encrypt(iv0, iv1, i>>32, i, ct);
	for (int j=0; j<16; ++j) {
	  const int k=i*16-offset+j;
	  if (k>=0 && k<n)
		buf[k]^=ct[j];
	}
  }
}

#undef setup_mix
#undef Te4_3
#undef Te4_2
#undef Te4_1
#undef Te4_0

//////////////////////////// stretchKey //////////////////////

// PBKDF2(pw[0..pwlen], salt[0..saltlen], c) to buf[0..dkLen-1]
// using HMAC-SHA256, for the special case of c = 1 iterations
// output size dkLen a multiple of 32, and pwLen <= 64.
static void pbkdf2(const char* pw, int pwLen, const char* salt, int saltLen,
				   int c, char* buf, int dkLen) {
  assert(c==1);
  assert(dkLen%32==0);
  assert(pwLen<=64);

  libzpaq::SHA256 sha256;
  char b[32];
  for (int i=1; i*32<=dkLen; ++i) {
	for (int j=0; j<pwLen; ++j) sha256.put(pw[j]^0x36);
	for (int j=pwLen; j<64; ++j) sha256.put(0x36);
	for (int j=0; j<saltLen; ++j) sha256.put(salt[j]);
	for (int j=24; j>=0; j-=8) sha256.put(i>>j);
	memcpy(b, sha256.result(), 32);
	for (int j=0; j<pwLen; ++j) sha256.put(pw[j]^0x5c);
	for (int j=pwLen; j<64; ++j) sha256.put(0x5c);
	for (int j=0; j<32; ++j) sha256.put(b[j]);
	memcpy(buf+i*32-32, sha256.result(), 32);
  }
}

// Hash b[0..15] using 8 rounds of salsa20
// Modified from http://cr.yp.to/salsa20.html (public domain) to 8 rounds
static void salsa8(U32* b) {
  unsigned x[16]={0};
  memcpy(x, b, 64);
  for (int i=0; i<4; ++i) {
	#define R(a,b) (((a)<<(b))+((a)>>(32-b)))
	x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
	x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);
	x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
	x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);
	x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
	x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);
	x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
	x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);
	x[ 1] ^= R(x[ 0]+x[ 3], 7);  x[ 2] ^= R(x[ 1]+x[ 0], 9);
	x[ 3] ^= R(x[ 2]+x[ 1],13);  x[ 0] ^= R(x[ 3]+x[ 2],18);
	x[ 6] ^= R(x[ 5]+x[ 4], 7);  x[ 7] ^= R(x[ 6]+x[ 5], 9);
	x[ 4] ^= R(x[ 7]+x[ 6],13);  x[ 5] ^= R(x[ 4]+x[ 7],18);
	x[11] ^= R(x[10]+x[ 9], 7);  x[ 8] ^= R(x[11]+x[10], 9);
	x[ 9] ^= R(x[ 8]+x[11],13);  x[10] ^= R(x[ 9]+x[ 8],18);
	x[12] ^= R(x[15]+x[14], 7);  x[13] ^= R(x[12]+x[15], 9);
	x[14] ^= R(x[13]+x[12],13);  x[15] ^= R(x[14]+x[13],18);
	#undef R
  }
  for (int i=0; i<16; ++i) b[i]+=x[i];
}

// BlockMix_{Salsa20/8, r} on b[0..128*r-1]
static void blockmix(U32* b, int r) {
  assert(r<=8);
  U32 x[16];
  U32 y[256];
  memcpy(x, b+32*r-16, 64);
  for (int i=0; i<2*r; ++i) {
	for (int j=0; j<16; ++j) x[j]^=b[i*16+j];
	salsa8(x);
	memcpy(&y[i*16], x, 64);
  }
  for (int i=0; i<r; ++i) memcpy(b+i*16, &y[i*32], 64);
  for (int i=0; i<r; ++i) memcpy(b+(i+r)*16, &y[i*32+16], 64);
}

// Mix b[0..128*r-1]. Uses 128*r*n bytes of memory and O(r*n) time
static void smix(char* b, int r, int n) {
  libzpaq::Array<U32> x(32*r), v(32*r*n);
  for (int i=0; i<r*128; ++i) x[i/4]+=(b[i]&255)<<i%4*8;
  for (int i=0; i<n; ++i) {
	memcpy(&v[i*r*32], &x[0], r*128);
	blockmix(&x[0], r);
  }
  for (int i=0; i<n; ++i) {
	U32 j=x[(2*r-1)*16]&(n-1);
	for (int k=0; k<r*32; ++k) x[k]^=v[j*r*32+k];
	blockmix(&x[0], r);
  }
  for (int i=0; i<r*128; ++i) b[i]=x[i/4]>>(i%4*8);
}

// Strengthen password pw[0..pwlen-1] and salt[0..saltlen-1]
// to produce key buf[0..buflen-1]. Uses O(n*r*p) time and 128*r*n bytes
// of memory. n must be a power of 2 and r <= 8.
void scrypt(const char* pw, int pwlen,
			const char* salt, int saltlen,
			int n, int r, int p, char* buf, int buflen) {
  assert(r<=8);
  assert(n>0 && (n&(n-1))==0);  // power of 2?
  libzpaq::Array<char> b(p*r*128);
  pbkdf2(pw, pwlen, salt, saltlen, 1, &b[0], p*r*128);
  for (int i=0; i<p; ++i) smix(&b[i*r*128], r, n);
  pbkdf2(pw, pwlen, &b[0], p*r*128, 1, buf, buflen);
}

// Stretch key in[0..31], assumed to be SHA256(password), with
// NUL terminate salt to produce new key out[0..31]
void stretchKey(char* out, const char* in, const char* salt) {
  scrypt(in, 32, salt, 32, 1<<14, 8, 1, out, 32);
}

//////////////////////////// Component ///////////////////////

// A Component is a context model, indirect context model, match model,
// fixed weight mixer, adaptive 2 input mixer without or with current
// partial byte as context, adaptive m input mixer (without or with),
// or SSE (without or with).

const int compsize[256]={0,2,3,2,3,4,6,6,3,5};

void Component::init() {
  limit=cxt=a=b=c=0;
  cm.resize(0);
  ht.resize(0);
  a16.resize(0);
}

////////////////////////// StateTable ////////////////////////

// sns[i*4] -> next state if 0, next state if 1, n0, n1
static const U8 sns[1024]={
	 1,     2,     0,     0,     3,     5,     1,     0,
	 4,     6,     0,     1,     7,     9,     2,     0,
	 8,    11,     1,     1,     8,    11,     1,     1,
	10,    12,     0,     2,    13,    15,     3,     0,
	14,    17,     2,     1,    14,    17,     2,     1,
	16,    19,     1,     2,    16,    19,     1,     2,
	18,    20,     0,     3,    21,    23,     4,     0,
	22,    25,     3,     1,    22,    25,     3,     1,
	24,    27,     2,     2,    24,    27,     2,     2,
	26,    29,     1,     3,    26,    29,     1,     3,
	28,    30,     0,     4,    31,    33,     5,     0,
	32,    35,     4,     1,    32,    35,     4,     1,
	34,    37,     3,     2,    34,    37,     3,     2,
	36,    39,     2,     3,    36,    39,     2,     3,
	38,    41,     1,     4,    38,    41,     1,     4,
	40,    42,     0,     5,    43,    33,     6,     0,
	44,    47,     5,     1,    44,    47,     5,     1,
	46,    49,     4,     2,    46,    49,     4,     2,
	48,    51,     3,     3,    48,    51,     3,     3,
	50,    53,     2,     4,    50,    53,     2,     4,
	52,    55,     1,     5,    52,    55,     1,     5,
	40,    56,     0,     6,    57,    45,     7,     0,
	58,    47,     6,     1,    58,    47,     6,     1,
	60,    63,     5,     2,    60,    63,     5,     2,
	62,    65,     4,     3,    62,    65,     4,     3,
	64,    67,     3,     4,    64,    67,     3,     4,
	66,    69,     2,     5,    66,    69,     2,     5,
	52,    71,     1,     6,    52,    71,     1,     6,
	54,    72,     0,     7,    73,    59,     8,     0,
	74,    61,     7,     1,    74,    61,     7,     1,
	76,    63,     6,     2,    76,    63,     6,     2,
	78,    81,     5,     3,    78,    81,     5,     3,
	80,    83,     4,     4,    80,    83,     4,     4,
	82,    85,     3,     5,    82,    85,     3,     5,
	66,    87,     2,     6,    66,    87,     2,     6,
	68,    89,     1,     7,    68,    89,     1,     7,
	70,    90,     0,     8,    91,    59,     9,     0,
	92,    77,     8,     1,    92,    77,     8,     1,
	94,    79,     7,     2,    94,    79,     7,     2,
	96,    81,     6,     3,    96,    81,     6,     3,
	98,   101,     5,     4,    98,   101,     5,     4,
   100,   103,     4,     5,   100,   103,     4,     5,
	82,   105,     3,     6,    82,   105,     3,     6,
	84,   107,     2,     7,    84,   107,     2,     7,
	86,   109,     1,     8,    86,   109,     1,     8,
	70,   110,     0,     9,   111,    59,    10,     0,
   112,    77,     9,     1,   112,    77,     9,     1,
   114,    97,     8,     2,   114,    97,     8,     2,
   116,    99,     7,     3,   116,    99,     7,     3,
	62,   101,     6,     4,    62,   101,     6,     4,
	80,    83,     5,     5,    80,    83,     5,     5,
   100,    67,     4,     6,   100,    67,     4,     6,
   102,   119,     3,     7,   102,   119,     3,     7,
   104,   121,     2,     8,   104,   121,     2,     8,
	86,   123,     1,     9,    86,   123,     1,     9,
	70,   124,     0,    10,   125,    59,    11,     0,
   126,    77,    10,     1,   126,    77,    10,     1,
   128,    97,     9,     2,   128,    97,     9,     2,
	60,    63,     8,     3,    60,    63,     8,     3,
	66,    69,     3,     8,    66,    69,     3,     8,
   104,   131,     2,     9,   104,   131,     2,     9,
	86,   133,     1,    10,    86,   133,     1,    10,
	70,   134,     0,    11,   135,    59,    12,     0,
   136,    77,    11,     1,   136,    77,    11,     1,
   138,    97,    10,     2,   138,    97,    10,     2,
   104,   141,     2,    10,   104,   141,     2,    10,
	86,   143,     1,    11,    86,   143,     1,    11,
	70,   144,     0,    12,   145,    59,    13,     0,
   146,    77,    12,     1,   146,    77,    12,     1,
   148,    97,    11,     2,   148,    97,    11,     2,
   104,   151,     2,    11,   104,   151,     2,    11,
	86,   153,     1,    12,    86,   153,     1,    12,
	70,   154,     0,    13,   155,    59,    14,     0,
   156,    77,    13,     1,   156,    77,    13,     1,
   158,    97,    12,     2,   158,    97,    12,     2,
   104,   161,     2,    12,   104,   161,     2,    12,
	86,   163,     1,    13,    86,   163,     1,    13,
	70,   164,     0,    14,   165,    59,    15,     0,
   166,    77,    14,     1,   166,    77,    14,     1,
   168,    97,    13,     2,   168,    97,    13,     2,
   104,   171,     2,    13,   104,   171,     2,    13,
	86,   173,     1,    14,    86,   173,     1,    14,
	70,   174,     0,    15,   175,    59,    16,     0,
   176,    77,    15,     1,   176,    77,    15,     1,
   178,    97,    14,     2,   178,    97,    14,     2,
   104,   181,     2,    14,   104,   181,     2,    14,
	86,   183,     1,    15,    86,   183,     1,    15,
	70,   184,     0,    16,   185,    59,    17,     0,
   186,    77,    16,     1,   186,    77,    16,     1,
	74,    97,    15,     2,    74,    97,    15,     2,
   104,    89,     2,    15,   104,    89,     2,    15,
	86,   187,     1,    16,    86,   187,     1,    16,
	70,   188,     0,    17,   189,    59,    18,     0,
   190,    77,    17,     1,    86,   191,     1,    17,
	70,   192,     0,    18,   193,    59,    19,     0,
   194,    77,    18,     1,    86,   195,     1,    18,
	70,   196,     0,    19,   193,    59,    20,     0,
   197,    77,    19,     1,    86,   198,     1,    19,
	70,   196,     0,    20,   199,    77,    20,     1,
	86,   200,     1,    20,   201,    77,    21,     1,
	86,   202,     1,    21,   203,    77,    22,     1,
	86,   204,     1,    22,   205,    77,    23,     1,
	86,   206,     1,    23,   207,    77,    24,     1,
	86,   208,     1,    24,   209,    77,    25,     1,
	86,   210,     1,    25,   211,    77,    26,     1,
	86,   212,     1,    26,   213,    77,    27,     1,
	86,   214,     1,    27,   215,    77,    28,     1,
	86,   216,     1,    28,   217,    77,    29,     1,
	86,   218,     1,    29,   219,    77,    30,     1,
	86,   220,     1,    30,   221,    77,    31,     1,
	86,   222,     1,    31,   223,    77,    32,     1,
	86,   224,     1,    32,   225,    77,    33,     1,
	86,   226,     1,    33,   227,    77,    34,     1,
	86,   228,     1,    34,   229,    77,    35,     1,
	86,   230,     1,    35,   231,    77,    36,     1,
	86,   232,     1,    36,   233,    77,    37,     1,
	86,   234,     1,    37,   235,    77,    38,     1,
	86,   236,     1,    38,   237,    77,    39,     1,
	86,   238,     1,    39,   239,    77,    40,     1,
	86,   240,     1,    40,   241,    77,    41,     1,
	86,   242,     1,    41,   243,    77,    42,     1,
	86,   244,     1,    42,   245,    77,    43,     1,
	86,   246,     1,    43,   247,    77,    44,     1,
	86,   248,     1,    44,   249,    77,    45,     1,
	86,   250,     1,    45,   251,    77,    46,     1,
	86,   252,     1,    46,   253,    77,    47,     1,
	86,   254,     1,    47,   253,    77,    48,     1,
	86,   254,     1,    48,     0,     0,     0,     0
};

// Initialize next state table ns[state*4] -> next if 0, next if 1, n0, n1
StateTable::StateTable() {
  memcpy(ns, sns, sizeof(ns));
}

/////////////////////////// ZPAQL //////////////////////////

// Write header to out2, return true if HCOMP/PCOMP section is present.
// If pp is true, then write only the postprocessor code.
bool ZPAQL::write(Writer* out2, bool pp) {
  if (header.size()<=6) return false;
  assert(header[0]+256*header[1]==cend-2+hend-hbegin);
  assert(cend>=7);
  assert(hbegin>=cend);
  assert(hend>=hbegin);
  assert(out2);
  if (!pp) {  // if not a postprocessor then write COMP
	for (int i=0; i<cend; ++i)
	  out2->put(header[i]);
  }
  else {  // write PCOMP size only
	out2->put((hend-hbegin)&255);
	out2->put((hend-hbegin)>>8);
  }
  for (int i=hbegin; i<hend; ++i)
	out2->put(header[i]);
  return true;
}

// Read header from in2
int ZPAQL::read(Reader* in2) {

  // Get header size and allocate
  int hsize=in2->get();
  hsize+=in2->get()*256;
  header.resize(hsize+300);
  cend=hbegin=hend=0;
  header[cend++]=hsize&255;
  header[cend++]=hsize>>8;
  while (cend<7) header[cend++]=in2->get(); // hh hm ph pm n

  // Read COMP
  int n=header[cend-1];
  for (int i=0; i<n; ++i) {
	int type=in2->get();  // component type
	if (type==-1) error("unexpected end of file");
	header[cend++]=type;  // component type
	int size=compsize[type];
	if (size<1) error("Invalid component type");
	if (cend+size>header.isize()-8) error("COMP list too big");
	for (int j=1; j<size; ++j)
	  header[cend++]=in2->get();
  }
  if ((header[cend++]=in2->get())!=0) error("missing COMP END");

  // Insert a guard gap and read HCOMP
  hbegin=hend=cend+128;
  while (hend<hsize+129) {
	assert(hend<header.isize()-8);
	int op=in2->get();
	if (op==-1) error("unexpected end of file");
	header[hend++]=op;
  }
  if ((header[hend++]=in2->get())!=0) error("missing HCOMP END");
  assert(cend>=7 && cend<header.isize());
  assert(hbegin==cend+128 && hbegin<header.isize());
  assert(hend>hbegin && hend<header.isize());
  assert(hsize==header[0]+256*header[1]);
  assert(hsize==cend-2+hend-hbegin);
  allocx(rcode, rcode_size, 0);  // clear JIT code
  return cend+hend-hbegin;
}

// Free memory, but preserve output, sha1 pointers
void ZPAQL::clear() {
  cend=hbegin=hend=0;  // COMP and HCOMP locations
  a=b=c=d=f=pc=0;      // machine state
  header.resize(0);
  h.resize(0);
  m.resize(0);
  r.resize(0);
  allocx(rcode, rcode_size, 0);
}

// Constructor
ZPAQL::ZPAQL() {
  output=0;
  sha1=0;
  rcode=0;
  rcode_size=0;
  clear();
  outbuf.resize(1<<14);
  bufptr=0;
}

ZPAQL::~ZPAQL() {
  allocx(rcode, rcode_size, 0);
}

// Initialize machine state as HCOMP
void ZPAQL::inith() {
  assert(header.isize()>6);
  assert(output==0);
  assert(sha1==0);
  init(header[2], header[3]); // hh, hm
}

// Initialize machine state as PCOMP
void ZPAQL::initp() {
  assert(header.isize()>6);
  init(header[4], header[5]); // ph, pm
}

// Flush pending output
void ZPAQL::flush() {
  if (output) output->write(&outbuf[0], bufptr);
  if (sha1) for (int i=0; i<bufptr; ++i) sha1->put(U8(outbuf[i]));
  bufptr=0;
}

// pow(2, x)
static double pow2(int x) {
  double r=1;
  for (; x>0; x--) r+=r;
  return r;
}

// Return memory requirement in bytes
double ZPAQL::memory() {
  double mem=pow2(header[2]+2)+pow2(header[3])  // hh hm
			+pow2(header[4]+2)+pow2(header[5])  // ph pm
			+header.size();
  int cp=7;  // start of comp list
  for (int i=0; i<header[6]; ++i) {  // n
	assert(cp<cend);
	double size=pow2(header[cp+1]); // sizebits
	switch(header[cp]) {
	  case CM: mem+=4*size; break;
	  case ICM: mem+=64*size+1024; break;
	  case MATCH: mem+=4*size+pow2(header[cp+2]); break; // bufbits
	  case MIX2: mem+=2*size; break;
	  case MIX: mem+=4*size*header[cp+3]; break; // m
	  case ISSE: mem+=64*size+2048; break;
	  case SSE: mem+=128*size; break;
	}
	cp+=compsize[header[cp]];
  }
  return mem;
}

// Initialize machine state to run a program.
void ZPAQL::init(int hbits, int mbits) {
  assert(header.isize()>0);
  assert(cend>=7);
  assert(hbegin>=cend+128);
  assert(hend>=hbegin);
  assert(hend<header.isize()-130);
  assert(header[0]+256*header[1]==cend-2+hend-hbegin);
  assert(bufptr==0);
  assert(outbuf.isize()>0);
  h.resize(1, hbits);
  m.resize(1, mbits);
  r.resize(256);
  a=b=c=d=pc=f=0;
}

// Run program on input by interpreting header
void ZPAQL::run0(U32 input) {
  assert(cend>6);
  assert(hbegin>=cend+128);
  assert(hend>=hbegin);
  assert(hend<header.isize()-130);
  assert(m.size()>0);
  assert(h.size()>0);
  assert(header[0]+256*header[1]==cend+hend-hbegin-2);
  pc=hbegin;
  a=input;
  while (execute()) ;
}

// Execute one instruction, return 0 after HALT else 1
int ZPAQL::execute() {
  switch(header[pc++]) {
	case 0: err(); break; // ERROR
	case 1: ++a; break; // A++
	case 2: --a; break; // A--
	case 3: a = ~a; break; // A!
	case 4: a = 0; break; // A=0
	case 7: a = r[header[pc++]]; break; // A=R N
	case 8: swap(b); break; // B<>A
	case 9: ++b; break; // B++
	case 10: --b; break; // B--
	case 11: b = ~b; break; // B!
	case 12: b = 0; break; // B=0
	case 15: b = r[header[pc++]]; break; // B=R N
	case 16: swap(c); break; // C<>A
	case 17: ++c; break; // C++
	case 18: --c; break; // C--
	case 19: c = ~c; break; // C!
	case 20: c = 0; break; // C=0
	case 23: c = r[header[pc++]]; break; // C=R N
	case 24: swap(d); break; // D<>A
	case 25: ++d; break; // D++
	case 26: --d; break; // D--
	case 27: d = ~d; break; // D!
	case 28: d = 0; break; // D=0
	case 31: d = r[header[pc++]]; break; // D=R N
	case 32: swap(m(b)); break; // *B<>A
	case 33: ++m(b); break; // *B++
	case 34: --m(b); break; // *B--
	case 35: m(b) = ~m(b); break; // *B!
	case 36: m(b) = 0; break; // *B=0
	case 39: if (f) pc+=((header[pc]+128)&255)-127; else ++pc; break; // JT N
	case 40: swap(m(c)); break; // *C<>A
	case 41: ++m(c); break; // *C++
	case 42: --m(c); break; // *C--
	case 43: m(c) = ~m(c); break; // *C!
	case 44: m(c) = 0; break; // *C=0
	case 47: if (!f) pc+=((header[pc]+128)&255)-127; else ++pc; break; // JF N
	case 48: swap(h(d)); break; // *D<>A
	case 49: ++h(d); break; // *D++
	case 50: --h(d); break; // *D--
	case 51: h(d) = ~h(d); break; // *D!
	case 52: h(d) = 0; break; // *D=0
	case 55: r[header[pc++]] = a; break; // R=A N
	case 56: return 0  ; // HALT
	case 57: outc(a&255); break; // OUT
	case 59: a = (a+m(b)+512)*773; break; // HASH
	case 60: h(d) = (h(d)+a+512)*773; break; // HASHD
	case 63: pc+=((header[pc]+128)&255)-127; break; // JMP N
	case 64: break; // A=A
	case 65: a = b; break; // A=B
	case 66: a = c; break; // A=C
	case 67: a = d; break; // A=D
	case 68: a = m(b); break; // A=*B
	case 69: a = m(c); break; // A=*C
	case 70: a = h(d); break; // A=*D
	case 71: a = header[pc++]; break; // A= N
	case 72: b = a; break; // B=A
	case 73: break; // B=B
	case 74: b = c; break; // B=C
	case 75: b = d; break; // B=D
	case 76: b = m(b); break; // B=*B
	case 77: b = m(c); break; // B=*C
	case 78: b = h(d); break; // B=*D
	case 79: b = header[pc++]; break; // B= N
	case 80: c = a; break; // C=A
	case 81: c = b; break; // C=B
	case 82: break; // C=C
	case 83: c = d; break; // C=D
	case 84: c = m(b); break; // C=*B
	case 85: c = m(c); break; // C=*C
	case 86: c = h(d); break; // C=*D
	case 87: c = header[pc++]; break; // C= N
	case 88: d = a; break; // D=A
	case 89: d = b; break; // D=B
	case 90: d = c; break; // D=C
	case 91: break; // D=D
	case 92: d = m(b); break; // D=*B
	case 93: d = m(c); break; // D=*C
	case 94: d = h(d); break; // D=*D
	case 95: d = header[pc++]; break; // D= N
	case 96: m(b) = a; break; // *B=A
	case 97: m(b) = b; break; // *B=B
	case 98: m(b) = c; break; // *B=C
	case 99: m(b) = d; break; // *B=D
	case 100: m(b) = m(b); break; // *B=*B
	case 101: m(b) = m(c); break; // *B=*C
	case 102: m(b) = h(d); break; // *B=*D
	case 103: m(b) = header[pc++]; break; // *B= N
	case 104: m(c) = a; break; // *C=A
	case 105: m(c) = b; break; // *C=B
	case 106: m(c) = c; break; // *C=C
	case 107: m(c) = d; break; // *C=D
	case 108: m(c) = m(b); break; // *C=*B
	case 109: m(c) = m(c); break; // *C=*C
	case 110: m(c) = h(d); break; // *C=*D
	case 111: m(c) = header[pc++]; break; // *C= N
	case 112: h(d) = a; break; // *D=A
	case 113: h(d) = b; break; // *D=B
	case 114: h(d) = c; break; // *D=C
	case 115: h(d) = d; break; // *D=D
	case 116: h(d) = m(b); break; // *D=*B
	case 117: h(d) = m(c); break; // *D=*C
	case 118: h(d) = h(d); break; // *D=*D
	case 119: h(d) = header[pc++]; break; // *D= N
	case 128: a += a; break; // A+=A
	case 129: a += b; break; // A+=B
	case 130: a += c; break; // A+=C
	case 131: a += d; break; // A+=D
	case 132: a += m(b); break; // A+=*B
	case 133: a += m(c); break; // A+=*C
	case 134: a += h(d); break; // A+=*D
	case 135: a += header[pc++]; break; // A+= N
	case 136: a -= a; break; // A-=A
	case 137: a -= b; break; // A-=B
	case 138: a -= c; break; // A-=C
	case 139: a -= d; break; // A-=D
	case 140: a -= m(b); break; // A-=*B
	case 141: a -= m(c); break; // A-=*C
	case 142: a -= h(d); break; // A-=*D
	case 143: a -= header[pc++]; break; // A-= N
	case 144: a *= a; break; // A*=A
	case 145: a *= b; break; // A*=B
	case 146: a *= c; break; // A*=C
	case 147: a *= d; break; // A*=D
	case 148: a *= m(b); break; // A*=*B
	case 149: a *= m(c); break; // A*=*C
	case 150: a *= h(d); break; // A*=*D
	case 151: a *= header[pc++]; break; // A*= N
	case 152: div(a); break; // A/=A
	case 153: div(b); break; // A/=B
	case 154: div(c); break; // A/=C
	case 155: div(d); break; // A/=D
	case 156: div(m(b)); break; // A/=*B
	case 157: div(m(c)); break; // A/=*C
	case 158: div(h(d)); break; // A/=*D
	case 159: div(header[pc++]); break; // A/= N
	case 160: mod(a); break; // A%=A
	case 161: mod(b); break; // A%=B
	case 162: mod(c); break; // A%=C
	case 163: mod(d); break; // A%=D
	case 164: mod(m(b)); break; // A%=*B
	case 165: mod(m(c)); break; // A%=*C
	case 166: mod(h(d)); break; // A%=*D
	case 167: mod(header[pc++]); break; // A%= N
	case 168: a &= a; break; // A&=A
	case 169: a &= b; break; // A&=B
	case 170: a &= c; break; // A&=C
	case 171: a &= d; break; // A&=D
	case 172: a &= m(b); break; // A&=*B
	case 173: a &= m(c); break; // A&=*C
	case 174: a &= h(d); break; // A&=*D
	case 175: a &= header[pc++]; break; // A&= N
	case 176: a &= ~ a; break; // A&~A
	case 177: a &= ~ b; break; // A&~B
	case 178: a &= ~ c; break; // A&~C
	case 179: a &= ~ d; break; // A&~D
	case 180: a &= ~ m(b); break; // A&~*B
	case 181: a &= ~ m(c); break; // A&~*C
	case 182: a &= ~ h(d); break; // A&~*D
	case 183: a &= ~ header[pc++]; break; // A&~ N
	case 184: a |= a; break; // A|=A
	case 185: a |= b; break; // A|=B
	case 186: a |= c; break; // A|=C
	case 187: a |= d; break; // A|=D
	case 188: a |= m(b); break; // A|=*B
	case 189: a |= m(c); break; // A|=*C
	case 190: a |= h(d); break; // A|=*D
	case 191: a |= header[pc++]; break; // A|= N
	case 192: a ^= a; break; // A^=A
	case 193: a ^= b; break; // A^=B
	case 194: a ^= c; break; // A^=C
	case 195: a ^= d; break; // A^=D
	case 196: a ^= m(b); break; // A^=*B
	case 197: a ^= m(c); break; // A^=*C
	case 198: a ^= h(d); break; // A^=*D
	case 199: a ^= header[pc++]; break; // A^= N
	case 200: a <<= (a&31); break; // A<<=A
	case 201: a <<= (b&31); break; // A<<=B
	case 202: a <<= (c&31); break; // A<<=C
	case 203: a <<= (d&31); break; // A<<=D
	case 204: a <<= (m(b)&31); break; // A<<=*B
	case 205: a <<= (m(c)&31); break; // A<<=*C
	case 206: a <<= (h(d)&31); break; // A<<=*D
	case 207: a <<= (header[pc++]&31); break; // A<<= N
	case 208: a >>= (a&31); break; // A>>=A
	case 209: a >>= (b&31); break; // A>>=B
	case 210: a >>= (c&31); break; // A>>=C
	case 211: a >>= (d&31); break; // A>>=D
	case 212: a >>= (m(b)&31); break; // A>>=*B
	case 213: a >>= (m(c)&31); break; // A>>=*C
	case 214: a >>= (h(d)&31); break; // A>>=*D
	case 215: a >>= (header[pc++]&31); break; // A>>= N
	case 216: f = 1; break; // A==A
	case 217: f = (a == b); break; // A==B
	case 218: f = (a == c); break; // A==C
	case 219: f = (a == d); break; // A==D
	case 220: f = (a == U32(m(b))); break; // A==*B
	case 221: f = (a == U32(m(c))); break; // A==*C
	case 222: f = (a == h(d)); break; // A==*D
	case 223: f = (a == U32(header[pc++])); break; // A== N
	case 224: f = 0; break; // A<A
	case 225: f = (a < b); break; // A<B
	case 226: f = (a < c); break; // A<C
	case 227: f = (a < d); break; // A<D
	case 228: f = (a < U32(m(b))); break; // A<*B
	case 229: f = (a < U32(m(c))); break; // A<*C
	case 230: f = (a < h(d)); break; // A<*D
	case 231: f = (a < U32(header[pc++])); break; // A< N
	case 232: f = 0; break; // A>A
	case 233: f = (a > b); break; // A>B
	case 234: f = (a > c); break; // A>C
	case 235: f = (a > d); break; // A>D
	case 236: f = (a > U32(m(b))); break; // A>*B
	case 237: f = (a > U32(m(c))); break; // A>*C
	case 238: f = (a > h(d)); break; // A>*D
	case 239: f = (a > U32(header[pc++])); break; // A> N
	case 255: if((pc=hbegin+header[pc]+256*header[pc+1])>=hend)err();break;//LJ
	default: err();
  }
  return 1;
}

// Print illegal instruction error message and exit
void ZPAQL::err() {
  error("ZPAQL execution error");
}

///////////////////////// Predictor /////////////////////////

// sdt2k[i]=2048/i;
static const int sdt2k[256]={
	 0,  2048,  1024,   682,   512,   409,   341,   292,
   256,   227,   204,   186,   170,   157,   146,   136,
   128,   120,   113,   107,   102,    97,    93,    89,
	85,    81,    78,    75,    73,    70,    68,    66,
	64,    62,    60,    58,    56,    55,    53,    52,
	51,    49,    48,    47,    46,    45,    44,    43,
	42,    41,    40,    40,    39,    38,    37,    37,
	36,    35,    35,    34,    34,    33,    33,    32,
	32,    31,    31,    30,    30,    29,    29,    28,
	28,    28,    27,    27,    26,    26,    26,    25,
	25,    25,    24,    24,    24,    24,    23,    23,
	23,    23,    22,    22,    22,    22,    21,    21,
	21,    21,    20,    20,    20,    20,    20,    19,
	19,    19,    19,    19,    18,    18,    18,    18,
	18,    18,    17,    17,    17,    17,    17,    17,
	17,    16,    16,    16,    16,    16,    16,    16,
	16,    15,    15,    15,    15,    15,    15,    15,
	15,    14,    14,    14,    14,    14,    14,    14,
	14,    14,    14,    13,    13,    13,    13,    13,
	13,    13,    13,    13,    13,    13,    12,    12,
	12,    12,    12,    12,    12,    12,    12,    12,
	12,    12,    12,    11,    11,    11,    11,    11,
	11,    11,    11,    11,    11,    11,    11,    11,
	11,    11,    11,    10,    10,    10,    10,    10,
	10,    10,    10,    10,    10,    10,    10,    10,
	10,    10,    10,    10,    10,     9,     9,     9,
	 9,     9,     9,     9,     9,     9,     9,     9,
	 9,     9,     9,     9,     9,     9,     9,     9,
	 9,     9,     9,     9,     8,     8,     8,     8,
	 8,     8,     8,     8,     8,     8,     8,     8,
	 8,     8,     8,     8,     8,     8,     8,     8,
	 8,     8,     8,     8,     8,     8,     8,     8
};

// sdt[i]=(1<<17)/(i*2+3)*2;
static const int sdt[1024]={
 87380, 52428, 37448, 29126, 23830, 20164, 17476, 15420,
 13796, 12482, 11396, 10484,  9708,  9038,  8456,  7942,
  7488,  7084,  6720,  6392,  6096,  5824,  5576,  5348,
  5140,  4946,  4766,  4598,  4442,  4296,  4160,  4032,
  3912,  3798,  3692,  3590,  3494,  3404,  3318,  3236,
  3158,  3084,  3012,  2944,  2880,  2818,  2758,  2702,
  2646,  2594,  2544,  2496,  2448,  2404,  2360,  2318,
  2278,  2240,  2202,  2166,  2130,  2096,  2064,  2032,
  2000,  1970,  1940,  1912,  1884,  1858,  1832,  1806,
  1782,  1758,  1736,  1712,  1690,  1668,  1648,  1628,
  1608,  1588,  1568,  1550,  1532,  1514,  1496,  1480,
  1464,  1448,  1432,  1416,  1400,  1386,  1372,  1358,
  1344,  1330,  1316,  1304,  1290,  1278,  1266,  1254,
  1242,  1230,  1218,  1208,  1196,  1186,  1174,  1164,
  1154,  1144,  1134,  1124,  1114,  1106,  1096,  1086,
  1078,  1068,  1060,  1052,  1044,  1036,  1028,  1020,
  1012,  1004,   996,   988,   980,   974,   966,   960,
   952,   946,   938,   932,   926,   918,   912,   906,
   900,   894,   888,   882,   876,   870,   864,   858,
   852,   848,   842,   836,   832,   826,   820,   816,
   810,   806,   800,   796,   790,   786,   782,   776,
   772,   768,   764,   758,   754,   750,   746,   742,
   738,   734,   730,   726,   722,   718,   714,   710,
   706,   702,   698,   694,   690,   688,   684,   680,
   676,   672,   670,   666,   662,   660,   656,   652,
   650,   646,   644,   640,   636,   634,   630,   628,
   624,   622,   618,   616,   612,   610,   608,   604,
   602,   598,   596,   594,   590,   588,   586,   582,
   580,   578,   576,   572,   570,   568,   566,   562,
   560,   558,   556,   554,   550,   548,   546,   544,
   542,   540,   538,   536,   532,   530,   528,   526,
   524,   522,   520,   518,   516,   514,   512,   510,
   508,   506,   504,   502,   500,   498,   496,   494,
   492,   490,   488,   488,   486,   484,   482,   480,
   478,   476,   474,   474,   472,   470,   468,   466,
   464,   462,   462,   460,   458,   456,   454,   454,
   452,   450,   448,   448,   446,   444,   442,   442,
   440,   438,   436,   436,   434,   432,   430,   430,
   428,   426,   426,   424,   422,   422,   420,   418,
   418,   416,   414,   414,   412,   410,   410,   408,
   406,   406,   404,   402,   402,   400,   400,   398,
   396,   396,   394,   394,   392,   390,   390,   388,
   388,   386,   386,   384,   382,   382,   380,   380,
   378,   378,   376,   376,   374,   372,   372,   370,
   370,   368,   368,   366,   366,   364,   364,   362,
   362,   360,   360,   358,   358,   356,   356,   354,
   354,   352,   352,   350,   350,   348,   348,   348,
   346,   346,   344,   344,   342,   342,   340,   340,
   340,   338,   338,   336,   336,   334,   334,   332,
   332,   332,   330,   330,   328,   328,   328,   326,
   326,   324,   324,   324,   322,   322,   320,   320,
   320,   318,   318,   316,   316,   316,   314,   314,
   312,   312,   312,   310,   310,   310,   308,   308,
   308,   306,   306,   304,   304,   304,   302,   302,
   302,   300,   300,   300,   298,   298,   298,   296,
   296,   296,   294,   294,   294,   292,   292,   292,
   290,   290,   290,   288,   288,   288,   286,   286,
   286,   284,   284,   284,   284,   282,   282,   282,
   280,   280,   280,   278,   278,   278,   276,   276,
   276,   276,   274,   274,   274,   272,   272,   272,
   272,   270,   270,   270,   268,   268,   268,   268,
   266,   266,   266,   266,   264,   264,   264,   262,
   262,   262,   262,   260,   260,   260,   260,   258,
   258,   258,   258,   256,   256,   256,   256,   254,
   254,   254,   254,   252,   252,   252,   252,   250,
   250,   250,   250,   248,   248,   248,   248,   248,
   246,   246,   246,   246,   244,   244,   244,   244,
   242,   242,   242,   242,   242,   240,   240,   240,
   240,   238,   238,   238,   238,   238,   236,   236,
   236,   236,   234,   234,   234,   234,   234,   232,
   232,   232,   232,   232,   230,   230,   230,   230,
   230,   228,   228,   228,   228,   228,   226,   226,
   226,   226,   226,   224,   224,   224,   224,   224,
   222,   222,   222,   222,   222,   220,   220,   220,
   220,   220,   220,   218,   218,   218,   218,   218,
   216,   216,   216,   216,   216,   216,   214,   214,
   214,   214,   214,   212,   212,   212,   212,   212,
   212,   210,   210,   210,   210,   210,   210,   208,
   208,   208,   208,   208,   208,   206,   206,   206,
   206,   206,   206,   204,   204,   204,   204,   204,
   204,   204,   202,   202,   202,   202,   202,   202,
   200,   200,   200,   200,   200,   200,   198,   198,
   198,   198,   198,   198,   198,   196,   196,   196,
   196,   196,   196,   196,   194,   194,   194,   194,
   194,   194,   194,   192,   192,   192,   192,   192,
   192,   192,   190,   190,   190,   190,   190,   190,
   190,   188,   188,   188,   188,   188,   188,   188,
   186,   186,   186,   186,   186,   186,   186,   186,
   184,   184,   184,   184,   184,   184,   184,   182,
   182,   182,   182,   182,   182,   182,   182,   180,
   180,   180,   180,   180,   180,   180,   180,   178,
   178,   178,   178,   178,   178,   178,   178,   176,
   176,   176,   176,   176,   176,   176,   176,   176,
   174,   174,   174,   174,   174,   174,   174,   174,
   172,   172,   172,   172,   172,   172,   172,   172,
   172,   170,   170,   170,   170,   170,   170,   170,
   170,   170,   168,   168,   168,   168,   168,   168,
   168,   168,   168,   166,   166,   166,   166,   166,
   166,   166,   166,   166,   166,   164,   164,   164,
   164,   164,   164,   164,   164,   164,   162,   162,
   162,   162,   162,   162,   162,   162,   162,   162,
   160,   160,   160,   160,   160,   160,   160,   160,
   160,   160,   158,   158,   158,   158,   158,   158,
   158,   158,   158,   158,   158,   156,   156,   156,
   156,   156,   156,   156,   156,   156,   156,   154,
   154,   154,   154,   154,   154,   154,   154,   154,
   154,   154,   152,   152,   152,   152,   152,   152,
   152,   152,   152,   152,   152,   150,   150,   150,
   150,   150,   150,   150,   150,   150,   150,   150,
   150,   148,   148,   148,   148,   148,   148,   148,
   148,   148,   148,   148,   148,   146,   146,   146,
   146,   146,   146,   146,   146,   146,   146,   146,
   146,   144,   144,   144,   144,   144,   144,   144,
   144,   144,   144,   144,   144,   142,   142,   142,
   142,   142,   142,   142,   142,   142,   142,   142,
   142,   142,   140,   140,   140,   140,   140,   140,
   140,   140,   140,   140,   140,   140,   140,   138,
   138,   138,   138,   138,   138,   138,   138,   138,
   138,   138,   138,   138,   138,   136,   136,   136,
   136,   136,   136,   136,   136,   136,   136,   136,
   136,   136,   136,   134,   134,   134,   134,   134,
   134,   134,   134,   134,   134,   134,   134,   134,
   134,   132,   132,   132,   132,   132,   132,   132,
   132,   132,   132,   132,   132,   132,   132,   132,
   130,   130,   130,   130,   130,   130,   130,   130,
   130,   130,   130,   130,   130,   130,   130,   128,
   128,   128,   128,   128,   128,   128,   128,   128,
   128,   128,   128,   128,   128,   128,   128,   126
};

// ssquasht[i]=int(32768.0/(1+exp((i-2048)*(-1.0/64))));
// Middle 1344 of 4096 entries only.
static const U16 ssquasht[1344]={
	 0,     0,     0,     0,     0,     0,     0,     1,
	 1,     1,     1,     1,     1,     1,     1,     1,
	 1,     1,     1,     1,     1,     1,     1,     1,
	 1,     1,     1,     1,     1,     1,     1,     1,
	 1,     1,     1,     1,     1,     1,     1,     1,
	 1,     1,     1,     1,     1,     1,     1,     1,
	 1,     1,     1,     2,     2,     2,     2,     2,
	 2,     2,     2,     2,     2,     2,     2,     2,
	 2,     2,     2,     2,     2,     2,     2,     2,
	 2,     2,     2,     2,     2,     3,     3,     3,
	 3,     3,     3,     3,     3,     3,     3,     3,
	 3,     3,     3,     3,     3,     3,     3,     3,
	 4,     4,     4,     4,     4,     4,     4,     4,
	 4,     4,     4,     4,     4,     4,     5,     5,
	 5,     5,     5,     5,     5,     5,     5,     5,
	 5,     5,     6,     6,     6,     6,     6,     6,
	 6,     6,     6,     6,     7,     7,     7,     7,
	 7,     7,     7,     7,     8,     8,     8,     8,
	 8,     8,     8,     8,     9,     9,     9,     9,
	 9,     9,    10,    10,    10,    10,    10,    10,
	10,    11,    11,    11,    11,    11,    12,    12,
	12,    12,    12,    13,    13,    13,    13,    13,
	14,    14,    14,    14,    15,    15,    15,    15,
	15,    16,    16,    16,    17,    17,    17,    17,
	18,    18,    18,    18,    19,    19,    19,    20,
	20,    20,    21,    21,    21,    22,    22,    22,
	23,    23,    23,    24,    24,    25,    25,    25,
	26,    26,    27,    27,    28,    28,    28,    29,
	29,    30,    30,    31,    31,    32,    32,    33,
	33,    34,    34,    35,    36,    36,    37,    37,
	38,    38,    39,    40,    40,    41,    42,    42,
	43,    44,    44,    45,    46,    46,    47,    48,
	49,    49,    50,    51,    52,    53,    54,    54,
	55,    56,    57,    58,    59,    60,    61,    62,
	63,    64,    65,    66,    67,    68,    69,    70,
	71,    72,    73,    74,    76,    77,    78,    79,
	81,    82,    83,    84,    86,    87,    88,    90,
	91,    93,    94,    96,    97,    99,   100,   102,
   103,   105,   107,   108,   110,   112,   114,   115,
   117,   119,   121,   123,   125,   127,   129,   131,
   133,   135,   137,   139,   141,   144,   146,   148,
   151,   153,   155,   158,   160,   163,   165,   168,
   171,   173,   176,   179,   182,   184,   187,   190,
   193,   196,   199,   202,   206,   209,   212,   215,
   219,   222,   226,   229,   233,   237,   240,   244,
   248,   252,   256,   260,   264,   268,   272,   276,
   281,   285,   289,   294,   299,   303,   308,   313,
   318,   323,   328,   333,   338,   343,   349,   354,
   360,   365,   371,   377,   382,   388,   394,   401,
   407,   413,   420,   426,   433,   440,   446,   453,
   460,   467,   475,   482,   490,   497,   505,   513,
   521,   529,   537,   545,   554,   562,   571,   580,
   589,   598,   607,   617,   626,   636,   646,   656,
   666,   676,   686,   697,   708,   719,   730,   741,
   752,   764,   776,   788,   800,   812,   825,   837,
   850,   863,   876,   890,   903,   917,   931,   946,
   960,   975,   990,  1005,  1020,  1036,  1051,  1067,
  1084,  1100,  1117,  1134,  1151,  1169,  1186,  1204,
  1223,  1241,  1260,  1279,  1298,  1318,  1338,  1358,
  1379,  1399,  1421,  1442,  1464,  1486,  1508,  1531,
  1554,  1577,  1600,  1624,  1649,  1673,  1698,  1724,
  1749,  1775,  1802,  1829,  1856,  1883,  1911,  1940,
  1968,  1998,  2027,  2057,  2087,  2118,  2149,  2181,
  2213,  2245,  2278,  2312,  2345,  2380,  2414,  2450,
  2485,  2521,  2558,  2595,  2633,  2671,  2709,  2748,
  2788,  2828,  2869,  2910,  2952,  2994,  3037,  3080,
  3124,  3168,  3213,  3259,  3305,  3352,  3399,  3447,
  3496,  3545,  3594,  3645,  3696,  3747,  3799,  3852,
  3906,  3960,  4014,  4070,  4126,  4182,  4240,  4298,
  4356,  4416,  4476,  4537,  4598,  4660,  4723,  4786,
  4851,  4916,  4981,  5048,  5115,  5183,  5251,  5320,
  5390,  5461,  5533,  5605,  5678,  5752,  5826,  5901,
  5977,  6054,  6131,  6210,  6289,  6369,  6449,  6530,
  6613,  6695,  6779,  6863,  6949,  7035,  7121,  7209,
  7297,  7386,  7476,  7566,  7658,  7750,  7842,  7936,
  8030,  8126,  8221,  8318,  8415,  8513,  8612,  8712,
  8812,  8913,  9015,  9117,  9221,  9324,  9429,  9534,
  9640,  9747,  9854,  9962, 10071, 10180, 10290, 10401,
 10512, 10624, 10737, 10850, 10963, 11078, 11192, 11308,
 11424, 11540, 11658, 11775, 11893, 12012, 12131, 12251,
 12371, 12491, 12612, 12734, 12856, 12978, 13101, 13224,
 13347, 13471, 13595, 13719, 13844, 13969, 14095, 14220,
 14346, 14472, 14599, 14725, 14852, 14979, 15106, 15233,
 15361, 15488, 15616, 15744, 15872, 16000, 16128, 16256,
 16384, 16511, 16639, 16767, 16895, 17023, 17151, 17279,
 17406, 17534, 17661, 17788, 17915, 18042, 18168, 18295,
 18421, 18547, 18672, 18798, 18923, 19048, 19172, 19296,
 19420, 19543, 19666, 19789, 19911, 20033, 20155, 20276,
 20396, 20516, 20636, 20755, 20874, 20992, 21109, 21227,
 21343, 21459, 21575, 21689, 21804, 21917, 22030, 22143,
 22255, 22366, 22477, 22587, 22696, 22805, 22913, 23020,
 23127, 23233, 23338, 23443, 23546, 23650, 23752, 23854,
 23955, 24055, 24155, 24254, 24352, 24449, 24546, 24641,
 24737, 24831, 24925, 25017, 25109, 25201, 25291, 25381,
 25470, 25558, 25646, 25732, 25818, 25904, 25988, 26072,
 26154, 26237, 26318, 26398, 26478, 26557, 26636, 26713,
 26790, 26866, 26941, 27015, 27089, 27162, 27234, 27306,
 27377, 27447, 27516, 27584, 27652, 27719, 27786, 27851,
 27916, 27981, 28044, 28107, 28169, 28230, 28291, 28351,
 28411, 28469, 28527, 28585, 28641, 28697, 28753, 28807,
 28861, 28915, 28968, 29020, 29071, 29122, 29173, 29222,
 29271, 29320, 29368, 29415, 29462, 29508, 29554, 29599,
 29643, 29687, 29730, 29773, 29815, 29857, 29898, 29939,
 29979, 30019, 30058, 30096, 30134, 30172, 30209, 30246,
 30282, 30317, 30353, 30387, 30422, 30455, 30489, 30522,
 30554, 30586, 30618, 30649, 30680, 30710, 30740, 30769,
 30799, 30827, 30856, 30884, 30911, 30938, 30965, 30992,
 31018, 31043, 31069, 31094, 31118, 31143, 31167, 31190,
 31213, 31236, 31259, 31281, 31303, 31325, 31346, 31368,
 31388, 31409, 31429, 31449, 31469, 31488, 31507, 31526,
 31544, 31563, 31581, 31598, 31616, 31633, 31650, 31667,
 31683, 31700, 31716, 31731, 31747, 31762, 31777, 31792,
 31807, 31821, 31836, 31850, 31864, 31877, 31891, 31904,
 31917, 31930, 31942, 31955, 31967, 31979, 31991, 32003,
 32015, 32026, 32037, 32048, 32059, 32070, 32081, 32091,
 32101, 32111, 32121, 32131, 32141, 32150, 32160, 32169,
 32178, 32187, 32196, 32205, 32213, 32222, 32230, 32238,
 32246, 32254, 32262, 32270, 32277, 32285, 32292, 32300,
 32307, 32314, 32321, 32327, 32334, 32341, 32347, 32354,
 32360, 32366, 32373, 32379, 32385, 32390, 32396, 32402,
 32407, 32413, 32418, 32424, 32429, 32434, 32439, 32444,
 32449, 32454, 32459, 32464, 32468, 32473, 32478, 32482,
 32486, 32491, 32495, 32499, 32503, 32507, 32511, 32515,
 32519, 32523, 32527, 32530, 32534, 32538, 32541, 32545,
 32548, 32552, 32555, 32558, 32561, 32565, 32568, 32571,
 32574, 32577, 32580, 32583, 32585, 32588, 32591, 32594,
 32596, 32599, 32602, 32604, 32607, 32609, 32612, 32614,
 32616, 32619, 32621, 32623, 32626, 32628, 32630, 32632,
 32634, 32636, 32638, 32640, 32642, 32644, 32646, 32648,
 32650, 32652, 32653, 32655, 32657, 32659, 32660, 32662,
 32664, 32665, 32667, 32668, 32670, 32671, 32673, 32674,
 32676, 32677, 32679, 32680, 32681, 32683, 32684, 32685,
 32686, 32688, 32689, 32690, 32691, 32693, 32694, 32695,
 32696, 32697, 32698, 32699, 32700, 32701, 32702, 32703,
 32704, 32705, 32706, 32707, 32708, 32709, 32710, 32711,
 32712, 32713, 32713, 32714, 32715, 32716, 32717, 32718,
 32718, 32719, 32720, 32721, 32721, 32722, 32723, 32723,
 32724, 32725, 32725, 32726, 32727, 32727, 32728, 32729,
 32729, 32730, 32730, 32731, 32731, 32732, 32733, 32733,
 32734, 32734, 32735, 32735, 32736, 32736, 32737, 32737,
 32738, 32738, 32739, 32739, 32739, 32740, 32740, 32741,
 32741, 32742, 32742, 32742, 32743, 32743, 32744, 32744,
 32744, 32745, 32745, 32745, 32746, 32746, 32746, 32747,
 32747, 32747, 32748, 32748, 32748, 32749, 32749, 32749,
 32749, 32750, 32750, 32750, 32750, 32751, 32751, 32751,
 32752, 32752, 32752, 32752, 32752, 32753, 32753, 32753,
 32753, 32754, 32754, 32754, 32754, 32754, 32755, 32755,
 32755, 32755, 32755, 32756, 32756, 32756, 32756, 32756,
 32757, 32757, 32757, 32757, 32757, 32757, 32757, 32758,
 32758, 32758, 32758, 32758, 32758, 32759, 32759, 32759,
 32759, 32759, 32759, 32759, 32759, 32760, 32760, 32760,
 32760, 32760, 32760, 32760, 32760, 32761, 32761, 32761,
 32761, 32761, 32761, 32761, 32761, 32761, 32761, 32762,
 32762, 32762, 32762, 32762, 32762, 32762, 32762, 32762,
 32762, 32762, 32762, 32763, 32763, 32763, 32763, 32763,
 32763, 32763, 32763, 32763, 32763, 32763, 32763, 32763,
 32763, 32764, 32764, 32764, 32764, 32764, 32764, 32764,
 32764, 32764, 32764, 32764, 32764, 32764, 32764, 32764,
 32764, 32764, 32764, 32764, 32765, 32765, 32765, 32765,
 32765, 32765, 32765, 32765, 32765, 32765, 32765, 32765,
 32765, 32765, 32765, 32765, 32765, 32765, 32765, 32765,
 32765, 32765, 32765, 32765, 32765, 32765, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32767, 32767, 32767, 32767, 32767, 32767
};

// stdt[i]=count of -i or i in botton or top of stretcht[]
static const U8 stdt[712]={
	64,   128,   128,   128,   128,   128,   127,   128,
   127,   128,   127,   127,   127,   127,   126,   126,
   126,   126,   126,   125,   125,   124,   125,   124,
   123,   123,   123,   123,   122,   122,   121,   121,
   120,   120,   119,   119,   118,   118,   118,   116,
   117,   115,   116,   114,   114,   113,   113,   112,
   112,   111,   110,   110,   109,   108,   108,   107,
   106,   106,   105,   104,   104,   102,   103,   101,
   101,   100,    99,    98,    98,    97,    96,    96,
	94,    94,    94,    92,    92,    91,    90,    89,
	89,    88,    87,    86,    86,    84,    84,    84,
	82,    82,    81,    80,    79,    79,    78,    77,
	76,    76,    75,    74,    73,    73,    72,    71,
	70,    70,    69,    68,    67,    67,    66,    65,
	65,    64,    63,    62,    62,    61,    61,    59,
	59,    59,    57,    58,    56,    56,    55,    54,
	54,    53,    52,    52,    51,    51,    50,    49,
	49,    48,    48,    47,    47,    45,    46,    44,
	45,    43,    43,    43,    42,    41,    41,    40,
	40,    40,    39,    38,    38,    37,    37,    36,
	36,    36,    35,    34,    34,    34,    33,    32,
	33,    32,    31,    31,    30,    31,    29,    30,
	28,    29,    28,    28,    27,    27,    27,    26,
	26,    25,    26,    24,    25,    24,    24,    23,
	23,    23,    23,    22,    22,    21,    22,    21,
	20,    21,    20,    19,    20,    19,    19,    19,
	18,    18,    18,    18,    17,    17,    17,    17,
	16,    16,    16,    16,    15,    15,    15,    15,
	15,    14,    14,    14,    14,    13,    14,    13,
	13,    13,    12,    13,    12,    12,    12,    11,
	12,    11,    11,    11,    11,    11,    10,    11,
	10,    10,    10,    10,     9,    10,     9,     9,
	 9,     9,     9,     8,     9,     8,     9,     8,
	 8,     8,     7,     8,     8,     7,     7,     8,
	 7,     7,     7,     6,     7,     7,     6,     6,
	 7,     6,     6,     6,     6,     6,     6,     5,
	 6,     5,     6,     5,     5,     5,     5,     5,
	 5,     5,     5,     5,     4,     5,     4,     5,
	 4,     4,     5,     4,     4,     4,     4,     4,
	 4,     3,     4,     4,     3,     4,     4,     3,
	 3,     4,     3,     3,     3,     4,     3,     3,
	 3,     3,     3,     3,     2,     3,     3,     3,
	 2,     3,     2,     3,     3,     2,     2,     3,
	 2,     2,     3,     2,     2,     2,     2,     3,
	 2,     2,     2,     2,     2,     2,     1,     2,
	 2,     2,     2,     1,     2,     2,     2,     1,
	 2,     1,     2,     2,     1,     2,     1,     2,
	 1,     1,     2,     1,     1,     2,     1,     1,
	 2,     1,     1,     1,     1,     2,     1,     1,
	 1,     1,     1,     1,     1,     1,     1,     1,
	 1,     1,     1,     1,     1,     1,     1,     1,
	 1,     1,     0,     1,     1,     1,     1,     0,
	 1,     1,     1,     0,     1,     1,     1,     0,
	 1,     1,     0,     1,     1,     0,     1,     0,
	 1,     1,     0,     1,     0,     1,     0,     1,
	 0,     1,     0,     1,     0,     1,     0,     1,
	 0,     1,     0,     1,     0,     1,     0,     0,
	 1,     0,     1,     0,     0,     1,     0,     1,
	 0,     0,     1,     0,     0,     1,     0,     0,
	 1,     0,     0,     1,     0,     0,     0,     1,
	 0,     0,     1,     0,     0,     0,     1,     0,
	 0,     0,     1,     0,     0,     0,     1,     0,
	 0,     0,     0,     1,     0,     0,     0,     0,
	 1,     0,     0,     0,     0,     1,     0,     0,
	 0,     0,     0,     1,     0,     0,     0,     0,
	 0,     1,     0,     0,     0,     0,     0,     0,
	 1,     0,     0,     0,     0,     0,     0,     0,
	 1,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     1,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     1,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     1,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     1,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     1,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     1,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     0,     0,
	 0,     0,     0,     0,     0,     0,     1,     0
};

// Initailize model-independent tables
Predictor::Predictor(ZPAQL& zr):
	c8(1), hmap4(1), z(zr) {
  assert(sizeof(U8)==1);
  assert(sizeof(U16)==2);
  assert(sizeof(U32)==4);
  assert(sizeof(U64)==8);
  assert(sizeof(short)==2);
  assert(sizeof(int)==4);

  // Initialize tables
  memcpy(dt2k, sdt2k, sizeof(dt2k));
  memcpy(dt, sdt, sizeof(dt));
  memcpy(squasht, ssquasht, sizeof(squasht));

  // ssquasht[i]=int(32768.0/(1+exp((i-2048)*(-1.0/64))));
  // Copy middle 1344 of 4096 entries.
  memset(squasht, 0, 1376*2);
  memcpy(squasht+1376, ssquasht, 1344*2);
  for (int i=2720; i<4096; ++i) squasht[i]=32767;

  // sstretcht[i]=int(log((i+0.5)/(32767.5-i))*64+0.5+100000)-100000;
  int k=16384;
  for (int i=0; i<712; ++i)
	for (int j=stdt[i]; j>0; --j)
	  stretcht[k++]=i;
  assert(k==32768);
  for (int i=0; i<16384; ++i)
	stretcht[i]=-stretcht[32767-i];

#ifndef NDEBUG
  // Verify floating point math for squash() and stretch()
  U32 sqsum=0, stsum=0;
  for (int i=32767; i>=0; --i)
	stsum=stsum*3+stretch(i);
  for (int i=4095; i>=0; --i)
	sqsum=sqsum*3+squash(i-2048);
  assert(stsum==3887533746u);
  assert(sqsum==2278286169u);
#endif

  pcode=0;
  pcode_size=0;
}

Predictor::~Predictor() {
  allocx(pcode, pcode_size, 0);  // free executable memory
}

// Initialize the predictor with a new model in z
void Predictor::init() {

  // Clear old JIT code if any
  allocx(pcode, pcode_size, 0);

  // Initialize context hash function
  z.inith();

  // Initialize predictions
  for (int i=0; i<256; ++i) h[i]=p[i]=0;

  // Initialize components
  for (int i=0; i<256; ++i)  // clear old model
	comp[i].init();
  int n=z.header[6]; // hsize[0..1] hh hm ph pm n (comp)[n] END 0[128] (hcomp) END
  const U8* cp=&z.header[7];  // start of component list
  for (int i=0; i<n; ++i) {
	assert(cp<&z.header[z.cend]);
	assert(cp>&z.header[0] && cp<&z.header[z.header.isize()-8]);
	Component& cr=comp[i];
	switch(cp[0]) {
	  case CONS:  // c
		p[i]=(cp[1]-128)*4;
		break;
	  case CM: // sizebits limit
		if (cp[1]>32) error("max size for CM is 32");
		cr.cm.resize(1, cp[1]);  // packed CM (22 bits) + CMCOUNT (10 bits)
		cr.limit=cp[2]*4;
		for (size_t j=0; j<cr.cm.size(); ++j)
		  cr.cm[j]=0x80000000;
		break;
	  case ICM: // sizebits
		if (cp[1]>26) error("max size for ICM is 26");
		cr.limit=1023;
		cr.cm.resize(256);
		cr.ht.resize(64, cp[1]);
		for (size_t j=0; j<cr.cm.size(); ++j)
		  cr.cm[j]=st.cminit(j);
		break;
	  case MATCH:  // sizebits
		if (cp[1]>32 || cp[2]>32) error("max size for MATCH is 32 32");
		cr.cm.resize(1, cp[1]);  // index
		cr.ht.resize(1, cp[2]);  // buf
		cr.ht(0)=1;
		break;
	  case AVG: // j k wt
		if (cp[1]>=i) error("AVG j >= i");
		if (cp[2]>=i) error("AVG k >= i");
		break;
	  case MIX2:  // sizebits j k rate mask
		if (cp[1]>32) error("max size for MIX2 is 32");
		if (cp[3]>=i) error("MIX2 k >= i");
		if (cp[2]>=i) error("MIX2 j >= i");
		cr.c=(size_t(1)<<cp[1]); // size (number of contexts)
		cr.a16.resize(1, cp[1]);  // wt[size][m]
		for (size_t j=0; j<cr.a16.size(); ++j)
		  cr.a16[j]=32768;
		break;
	  case MIX: {  // sizebits j m rate mask
		if (cp[1]>32) error("max size for MIX is 32");
		if (cp[2]>=i) error("MIX j >= i");
		if (cp[3]<1 || cp[3]>i-cp[2]) error("MIX m not in 1..i-j");
		int m=cp[3];  // number of inputs
		assert(m>=1);
		cr.c=(size_t(1)<<cp[1]); // size (number of contexts)
		cr.cm.resize(m, cp[1]);  // wt[size][m]
		for (size_t j=0; j<cr.cm.size(); ++j)
		  cr.cm[j]=65536/m;
		break;
	  }
	  case ISSE:  // sizebits j
		if (cp[1]>32) error("max size for ISSE is 32");
		if (cp[2]>=i) error("ISSE j >= i");
		cr.ht.resize(64, cp[1]);
		cr.cm.resize(512);
		for (int j=0; j<256; ++j) {
		  cr.cm[j*2]=1<<15;
		  cr.cm[j*2+1]=clamp512k(stretch(st.cminit(j)>>8)<<10);
		}
		break;
	  case SSE: // sizebits j start limit
		if (cp[1]>32) error("max size for SSE is 32");
		if (cp[2]>=i) error("SSE j >= i");
		if (cp[3]>cp[4]*4) error("SSE start > limit*4");
		cr.cm.resize(32, cp[1]);
		cr.limit=cp[4]*4;
		for (size_t j=0; j<cr.cm.size(); ++j)
		  cr.cm[j]=squash((j&31)*64-992)<<17|cp[3];
		break;
	  default: error("unknown component type");
	}
	assert(compsize[*cp]>0);
	cp+=compsize[*cp];
	assert(cp>=&z.header[7] && cp<&z.header[z.cend]);
  }
}

// Return next bit prediction using interpreted COMP code
int Predictor::predict0() {
  assert(c8>=1 && c8<=255);

  // Predict next bit
  int n=z.header[6];
  assert(n>0 && n<=255);
  const U8* cp=&z.header[7];
  assert(cp[-1]==n);
  for (int i=0; i<n; ++i) {
	assert(cp>&z.header[0] && cp<&z.header[z.header.isize()-8]);
	Component& cr=comp[i];
	switch(cp[0]) {
	  case CONS:  // c
		break;
	  case CM:  // sizebits limit
		cr.cxt=h[i]^hmap4;
		p[i]=stretch(cr.cm(cr.cxt)>>17);
		break;
	  case ICM: // sizebits
		assert((hmap4&15)>0);
		if (c8==1 || (c8&0xf0)==16) cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
		cr.cxt=cr.ht[cr.c+(hmap4&15)];
		p[i]=stretch(cr.cm(cr.cxt)>>8);
		break;
	  case MATCH: // sizebits bufbits: a=len, b=offset, c=bit, cxt=bitpos,
				  //                   ht=buf, limit=pos
		assert(cr.cm.size()==(size_t(1)<<cp[1]));
		assert(cr.ht.size()==(size_t(1)<<cp[2]));
		assert(cr.a<=255);
		assert(cr.c==0 || cr.c==1);
		assert(cr.cxt<8);
		assert(cr.limit<cr.ht.size());
		if (cr.a==0) p[i]=0;
		else {
		  cr.c=(cr.ht(cr.limit-cr.b)>>(7-cr.cxt))&1; // predicted bit
		  p[i]=stretch(dt2k[cr.a]*(cr.c*-2+1)&32767);
		}
		break;
	  case AVG: // j k wt
		p[i]=(p[cp[1]]*cp[3]+p[cp[2]]*(256-cp[3]))>>8;
		break;
	  case MIX2: { // sizebits j k rate mask
				   // c=size cm=wt[size] cxt=input
		cr.cxt=((h[i]+(c8&cp[5]))&(cr.c-1));
		assert(cr.cxt<cr.a16.size());
		int w=cr.a16[cr.cxt];
		assert(w>=0 && w<65536);
		p[i]=(w*p[cp[2]]+(65536-w)*p[cp[3]])>>16;
		assert(p[i]>=-2048 && p[i]<2048);
	  }
		break;
	  case MIX: {  // sizebits j m rate mask
				   // c=size cm=wt[size][m] cxt=index of wt in cm
		int m=cp[3];
		assert(m>=1 && m<=i);
		cr.cxt=h[i]+(c8&cp[5]);
		cr.cxt=(cr.cxt&(cr.c-1))*m; // pointer to row of weights
		assert(cr.cxt<=cr.cm.size()-m);
		int* wt=(int*)&cr.cm[cr.cxt];
		p[i]=0;
		for (int j=0; j<m; ++j)
		  p[i]+=(wt[j]>>8)*p[cp[2]+j];
		p[i]=clamp2k(p[i]>>8);
	  }
		break;
	  case ISSE: { // sizebits j -- c=hi, cxt=bh
		assert((hmap4&15)>0);
		if (c8==1 || (c8&0xf0)==16)
		  cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
		cr.cxt=cr.ht[cr.c+(hmap4&15)];  // bit history
		int *wt=(int*)&cr.cm[cr.cxt*2];
		p[i]=clamp2k((wt[0]*p[cp[2]]+wt[1]*64)>>16);
	  }
		break;
	  case SSE: { // sizebits j start limit
		cr.cxt=(h[i]+c8)*32;
		int pq=p[cp[2]]+992;
		if (pq<0) pq=0;
		if (pq>1983) pq=1983;
		int wt=pq&63;
		pq>>=6;
		assert(pq>=0 && pq<=30);
		cr.cxt+=pq;
		p[i]=stretch(((cr.cm(cr.cxt)>>10)*(64-wt)+(cr.cm(cr.cxt+1)>>10)*wt)>>13);
		cr.cxt+=wt>>5;
	  }
		break;
	  default:
		error("component predict not implemented");
	}
	cp+=compsize[cp[0]];
	assert(cp<&z.header[z.cend]);
	assert(p[i]>=-2048 && p[i]<2048);
  }
  assert(cp[0]==NONE);
  return squash(p[n-1]);
}

// Update model with decoded bit y (0...1)
void Predictor::update0(int y) {
  assert(y==0 || y==1);
  assert(c8>=1 && c8<=255);
  assert(hmap4>=1 && hmap4<=511);

  // Update components
  const U8* cp=&z.header[7];
  int n=z.header[6];
  assert(n>=1 && n<=255);
  assert(cp[-1]==n);
  for (int i=0; i<n; ++i) {
	Component& cr=comp[i];
	switch(cp[0]) {
	  case CONS:  // c
		break;
	  case CM:  // sizebits limit
		train(cr, y);
		break;
	  case ICM: { // sizebits: cxt=ht[b]=bh, ht[c][0..15]=bh row, cxt=bh
		cr.ht[cr.c+(hmap4&15)]=st.next(cr.ht[cr.c+(hmap4&15)], y);
		U32& pn=cr.cm(cr.cxt);
		pn+=int(y*32767-(pn>>8))>>2;
	  }
		break;
	  case MATCH: // sizebits bufbits:
				  //   a=len, b=offset, c=bit, cm=index, cxt=bitpos
				  //   ht=buf, limit=pos
	  {
		assert(cr.a<=255);
		assert(cr.c==0 || cr.c==1);
		assert(cr.cxt<8);
		assert(cr.cm.size()==(size_t(1)<<cp[1]));
		assert(cr.ht.size()==(size_t(1)<<cp[2]));
		assert(cr.limit<cr.ht.size());
		if (int(cr.c)!=y) cr.a=0;  // mismatch?
		cr.ht(cr.limit)+=cr.ht(cr.limit)+y;
		if (++cr.cxt==8) {
		  cr.cxt=0;
		  ++cr.limit;
		  cr.limit&=(1<<cp[2])-1;
		  if (cr.a==0) {  // look for a match
			cr.b=cr.limit-cr.cm(h[i]);
			if (cr.b&(cr.ht.size()-1))
			  while (cr.a<255
					 && cr.ht(cr.limit-cr.a-1)==cr.ht(cr.limit-cr.a-cr.b-1))
				++cr.a;
		  }
		  else cr.a+=cr.a<255;
		  cr.cm(h[i])=cr.limit;
		}
	  }
		break;
	  case AVG:  // j k wt
		break;
	  case MIX2: { // sizebits j k rate mask
				   // cm=wt[size], cxt=input
		assert(cr.a16.size()==cr.c);
		assert(cr.cxt<cr.a16.size());
		int err=(y*32767-squash(p[i]))*cp[4]>>5;
		int w=cr.a16[cr.cxt];
		w+=(err*(p[cp[2]]-p[cp[3]])+(1<<12))>>13;
		if (w<0) w=0;
		if (w>65535) w=65535;
		cr.a16[cr.cxt]=w;
	  }
		break;
	  case MIX: {   // sizebits j m rate mask
					// cm=wt[size][m], cxt=input
		int m=cp[3];
		assert(m>0 && m<=i);
		assert(cr.cm.size()==m*cr.c);
		assert(cr.cxt+m<=cr.cm.size());
		int err=(y*32767-squash(p[i]))*cp[4]>>4;
		int* wt=(int*)&cr.cm[cr.cxt];
		for (int j=0; j<m; ++j)
		  wt[j]=clamp512k(wt[j]+((err*p[cp[2]+j]+(1<<12))>>13));
	  }
		break;
	  case ISSE: { // sizebits j  -- c=hi, cxt=bh
		assert(cr.cxt==cr.ht[cr.c+(hmap4&15)]);
		int err=y*32767-squash(p[i]);
		int *wt=(int*)&cr.cm[cr.cxt*2];
		wt[0]=clamp512k(wt[0]+((err*p[cp[2]]+(1<<12))>>13));
		wt[1]=clamp512k(wt[1]+((err+16)>>5));
		cr.ht[cr.c+(hmap4&15)]=st.next(cr.cxt, y);
	  }
		break;
	  case SSE:  // sizebits j start limit
		train(cr, y);
		break;
	  default:
		assert(0);
	}
	cp+=compsize[cp[0]];
	assert(cp>=&z.header[7] && cp<&z.header[z.cend]
		   && cp<&z.header[z.header.isize()-8]);
  }
  assert(cp[0]==NONE);

  // Save bit y in c8, hmap4
  c8+=c8+y;
  if (c8>=256) {
	z.run(c8-256);
	hmap4=1;
	c8=1;
	for (int i=0; i<n; ++i) h[i]=z.H(i);
  }
  else if (c8>=16 && c8<32)
	hmap4=(hmap4&0xf)<<5|y<<4|1;
  else
	hmap4=(hmap4&0x1f0)|(((hmap4&0xf)*2+y)&0xf);
}

// Find cxt row in hash table ht. ht has rows of 16 indexed by the
// low sizebits of cxt with element 0 having the next higher 8 bits for
// collision detection. If not found after 3 adjacent tries, replace the
// row with lowest element 1 as priority. Return index of row.
size_t Predictor::find(Array<U8>& ht, int sizebits, U32 cxt) {
  assert(ht.size()==size_t(16)<<sizebits);
  int chk=cxt>>sizebits&255;
  size_t h0=(cxt*16)&(ht.size()-16);
  if (ht[h0]==chk) return h0;
  size_t h1=h0^16;
  if (ht[h1]==chk) return h1;
  size_t h2=h0^32;
  if (ht[h2]==chk) return h2;
  if (ht[h0+1]<=ht[h1+1] && ht[h0+1]<=ht[h2+1])
	return memset(&ht[h0], 0, 16), ht[h0]=chk, h0;
  else if (ht[h1+1]<ht[h2+1])
	return memset(&ht[h1], 0, 16), ht[h1]=chk, h1;
  else
	return memset(&ht[h2], 0, 16), ht[h2]=chk, h2;
}

/////////////////////// Decoder ///////////////////////

Decoder::Decoder(ZPAQL& z):
	in(0), low(1), high(0xFFFFFFFF), curr(0), pr(z), buf(BUFSIZE) {
}

void Decoder::init() {
  pr.init();
  if (pr.isModeled()) low=1, high=0xFFFFFFFF, curr=0;
  else low=high=curr=0;
}

// Read un-modeled input into buf[low=0..high-1]
// with curr remaining in subblock to read.
void Decoder::loadbuf() {
  assert(!pr.isModeled());
  assert(low==high);
  if (curr==0) {
	for (int i=0; i<4; ++i) {
	  int c=in->get();
	  if (c<0) error("unexpected end of input");
	  curr=curr<<8|c;
	}
  }
  U32 n=buf.size();
  if (n>curr) n=curr;
  high=in->read(&buf[0], n);
  curr-=high;
  low=0;
}

// Return next bit of decoded input, which has 16 bit probability p of being 1
int Decoder::decode(int p) {
  assert(p>=0 && p<65536);
  assert(high>low && low>0);
  if (curr<low || curr>high) error("archive corrupted");
  assert(curr>=low && curr<=high);
  U32 mid=low+U32(((high-low)*U64(U32(p)))>>16);  // split range
  assert(high>mid && mid>=low);
  int y;
  if (curr<=mid) y=1, high=mid;  // pick half
  else y=0, low=mid+1;
  while ((high^low)<0x1000000) { // shift out identical leading bytes
	high=high<<8|255;
	low=low<<8;
	low+=(low==0);
	int c=in->get();
	if (c<0) error("unexpected end of file");
	curr=curr<<8|c;
  }
  return y;
}

// Decompress 1 byte or -1 at end of input
int Decoder::decompress() {
  if (pr.isModeled()) {  // n>0 components?
	if (curr==0) {  // segment initialization
	  for (int i=0; i<4; ++i)
		curr=curr<<8|in->get();
	}
	if (decode(0)) {
	  if (curr!=0) error("decoding end of stream");
	  return -1;
	}
	else {
	  int c=1;
	  while (c<256) {  // get 8 bits
		int p=pr.predict()*2+1;
		c+=c+decode(p);
		pr.update(c&1);
	  }
	  return c-256;
	}
  }
  else {
	if (low==high) loadbuf();
	if (low==high) return -1;
	return buf[low++]&255;
  }
}

// Find end of compressed data and return next byte
int Decoder::skip() {
  int c=-1;
  if (pr.isModeled()) {
	while (curr==0)  // at start?
	  curr=in->get();
	while (curr && (c=in->get())>=0)  // find 4 zeros
	  curr=curr<<8|c;
	while ((c=in->get())==0) ;  // might be more than 4
	return c;
  }
  else {
	if (curr==0)  // at start?
	  for (int i=0; i<4 && (c=in->get())>=0; ++i) curr=curr<<8|c;
	while (curr>0) {
	  U32 n=BUFSIZE;
	  if (n>curr) n=curr;
	  U32 n1=in->read(&buf[0], n);
	  curr-=n1;
	  if (n1<1) return -1;
	  if (curr==0)
		for (int i=0; i<4 && (c=in->get())>=0; ++i) curr=curr<<8|c;
	}
	if (c>=0) c=in->get();
	return c;
  }
}

////////////////////// PostProcessor //////////////////////

// Copy ph, pm from block header
void PostProcessor::init(int h, int m) {
  state=hsize=0;
  ph=h;
  pm=m;
  z.clear();
}

// (PASS=0 | PROG=1 psize[0..1] pcomp[0..psize-1]) data... EOB=-1
// Return state: 1=PASS, 2..4=loading PROG, 5=PROG loaded
int PostProcessor::write(int c) {
  assert(c>=-1 && c<=255);
  switch (state) {
	case 0:  // initial state
	  if (c<0) error("Unexpected EOS");
	  state=c+1;  // 1=PASS, 2=PROG
	  if (state>2) error("unknown post processing type");
	  if (state==1) z.clear();
	  break;
	case 1:  // PASS
	  z.outc(c);
	  break;
	case 2: // PROG
	  if (c<0) error("Unexpected EOS");
	  hsize=c;  // low byte of size
	  state=3;
	  break;
	case 3:  // PROG psize[0]
	  if (c<0) error("Unexpected EOS");
	  hsize+=c*256;  // high byte of psize
	  z.header.resize(hsize+300);
	  z.cend=8;
	  z.hbegin=z.hend=z.cend+128;
	  z.header[4]=ph;
	  z.header[5]=pm;
	  state=4;
	  break;
	case 4:  // PROG psize[0..1] pcomp[0...]
	  if (c<0) error("Unexpected EOS");
	  assert(z.hend<z.header.isize());
	  z.header[z.hend++]=c;  // one byte of pcomp
	  if (z.hend-z.hbegin==hsize) {  // last byte of pcomp?
		hsize=z.cend-2+z.hend-z.hbegin;
		z.header[0]=hsize&255;  // header size with empty COMP
		z.header[1]=hsize>>8;
		z.initp();
		state=5;
	  }
	  break;
	case 5:  // PROG ... data
	  z.run(c);
	  if (c<0) z.flush();
	  break;
  }
  return state;
}

/////////////////////// Decompresser /////////////////////

// Find the start of a block and return true if found. Set memptr
// to memory used.
bool Decompresser::findBlock(double* memptr) {
  assert(state==BLOCK);

  // Find start of block
  U32 h1=0x3D49B113, h2=0x29EB7F93, h3=0x2614BE13, h4=0x3828EB13;
  // Rolling hashes initialized to hash of first 13 bytes
  int c;
  while ((c=dec.in->get())!=-1) {
	h1=h1*12+c;
	h2=h2*20+c;
	h3=h3*28+c;
	h4=h4*44+c;
	if (h1==0xB16B88F1 && h2==0xFF5376F1 && h3==0x72AC5BF1 && h4==0x2F909AF1)
	  break;  // hash of 16 byte string
  }
  if (c==-1) return false;

  // Read header
  if ((c=dec.in->get())!=1 && c!=2) error("unsupported ZPAQ level");
  if (dec.in->get()!=1) error("unsupported ZPAQL type");
  z.read(dec.in);
  if (c==1 && z.header.isize()>6 && z.header[6]==0)
	error("ZPAQ level 1 requires at least 1 component");
  if (memptr) *memptr=z.memory();
  state=FILENAME;
  decode_state=FIRSTSEG;
  return true;
}

// Read the start of a segment (1) or end of block code (255).
// If a segment is found, write the filename and return true, else false.
bool Decompresser::findFilename(Writer* filename) {
  assert(state==FILENAME);
  int c=dec.in->get();
  if (c==1) {  // segment found
	while (true) {
	  c=dec.in->get();
	  if (c==-1) error("unexpected EOF");
	  if (c==0) {
		state=COMMENT;
		return true;
	  }
	  if (filename) filename->put(c);
	}
  }
  else if (c==255) {  // end of block found
	state=BLOCK;
	return false;
  }
  else
	error("missing segment or end of block");
  return false;
}

// Read the comment from the segment header
void Decompresser::readComment(Writer* comment) {
  assert(state==COMMENT);
  state=DATA;
  while (true) {
	int c=dec.in->get();
	if (c==-1) error("unexpected EOF");
	if (c==0) break;
	if (comment) comment->put(c);
  }
  if (dec.in->get()!=0) error("missing reserved byte");
}

// Decompress n bytes, or all if n < 0. Return false if done
bool Decompresser::decompress(int n) {
  assert(state==DATA);
  assert(decode_state!=SKIP);

  // Initialize models to start decompressing block
  if (decode_state==FIRSTSEG) {
	dec.init();
	assert(z.header.size()>5);
	pp.init(z.header[4], z.header[5]);
	decode_state=SEG;
  }

  // Decompress and load PCOMP into postprocessor
  while ((pp.getState()&3)!=1)
	pp.write(dec.decompress());

  // Decompress n bytes, or all if n < 0
  while (n) {
	int c=dec.decompress();
	pp.write(c);
	if (c==-1) {
	  state=SEGEND;
	  return false;
	}
	if (n>0) --n;
  }
  return true;
}

// Read end of block. If a SHA1 checksum is present, write 1 and the
// 20 byte checksum into sha1string, else write 0 in first byte.
// If sha1string is 0 then discard it.
void Decompresser::readSegmentEnd(char* sha1string) {
  assert(state==DATA || state==SEGEND);

  // Skip remaining data if any and get next byte
  int c=0;
  if (state==DATA) {
	c=dec.skip();
	decode_state=SKIP;
  }
  else if (state==SEGEND)
	c=dec.in->get();
  state=FILENAME;

  // Read checksum
  if (c==254) {
	if (sha1string) sha1string[0]=0;  // no checksum
  }
  else if (c==253) {
	if (sha1string) sha1string[0]=1;
	for (int i=1; i<=20; ++i) {
	  c=dec.in->get();
	  if (sha1string) sha1string[i]=c;
	}
  }
  else
	error("missing end of segment marker");
}

/////////////////////////// decompress() //////////////////////

void decompress(Reader* in, Writer* out) {
  Decompresser d;
  d.setInput(in);
  d.setOutput(out);
  while (d.findBlock()) {       // don't calculate memory
	while (d.findFilename()) {  // discard filename
	  d.readComment();          // discard comment
	  d.decompress();           // to end of segment
	  d.readSegmentEnd();       // discard sha1string
	}
  }
}

/////////////////////////// Encoder ///////////////////////////

// Initialize for start of block
void Encoder::init() {
  low=1;
  high=0xFFFFFFFF;
  pr.init();
  if (!pr.isModeled()) low=0, buf.resize(1<<16);
}

// compress bit y having probability p/64K
void Encoder::encode(int y, int p) {
  assert(out);
  assert(p>=0 && p<65536);
  assert(y==0 || y==1);
  assert(high>low && low>0);
  U32 mid=low+U32(((high-low)*U64(U32(p)))>>16);  // split range
  assert(high>mid && mid>=low);
  if (y) high=mid; else low=mid+1; // pick half
  while ((high^low)<0x1000000) { // write identical leading bytes
	out->put(high>>24);  // same as low>>24
	high=high<<8|255;
	low=low<<8;
	low+=(low==0); // so we don't code 4 0 bytes in a row
  }
}

// compress byte c (0..255 or -1=EOS)
void Encoder::compress(int c) {
  assert(out);
  if (pr.isModeled()) {
	if (c==-1)
	  encode(1, 0);
	else {
	  assert(c>=0 && c<=255);
	  encode(0, 0);
	  for (int i=7; i>=0; --i) {
		int p=pr.predict()*2+1;
		assert(p>0 && p<65536);
		int y=c>>i&1;
		encode(y, p);
		pr.update(y);
	  }
	}
  }
  else {
	if (low && (c<0 || low==buf.size())) {
	  out->put((low>>24)&255);
	  out->put((low>>16)&255);
	  out->put((low>>8)&255);
	  out->put(low&255);
	  out->write(&buf[0], low);
	  low=0;
	}
	if (c>=0) buf[low++]=c;
  }
}

//////////////////////////// Compiler /////////////////////////

// Component names
const char* compname[256]=
  {"","const","cm","icm","match","avg","mix2","mix","isse","sse",0};

// Opcodes
const char* opcodelist[272]={
"error","a++",  "a--",  "a!",   "a=0",  "",     "",     "a=r",
"b<>a", "b++",  "b--",  "b!",   "b=0",  "",     "",     "b=r",
"c<>a", "c++",  "c--",  "c!",   "c=0",  "",     "",     "c=r",
"d<>a", "d++",  "d--",  "d!",   "d=0",  "",     "",     "d=r",
"*b<>a","*b++", "*b--", "*b!",  "*b=0", "",     "",     "jt",
"*c<>a","*c++", "*c--", "*c!",  "*c=0", "",     "",     "jf",
"*d<>a","*d++", "*d--", "*d!",  "*d=0", "",     "",     "r=a",
"halt", "out",  "",     "hash", "hashd","",     "",     "jmp",
"a=a",  "a=b",  "a=c",  "a=d",  "a=*b", "a=*c", "a=*d", "a=",
"b=a",  "b=b",  "b=c",  "b=d",  "b=*b", "b=*c", "b=*d", "b=",
"c=a",  "c=b",  "c=c",  "c=d",  "c=*b", "c=*c", "c=*d", "c=",
"d=a",  "d=b",  "d=c",  "d=d",  "d=*b", "d=*c", "d=*d", "d=",
"*b=a", "*b=b", "*b=c", "*b=d", "*b=*b","*b=*c","*b=*d","*b=",
"*c=a", "*c=b", "*c=c", "*c=d", "*c=*b","*c=*c","*c=*d","*c=",
"*d=a", "*d=b", "*d=c", "*d=d", "*d=*b","*d=*c","*d=*d","*d=",
"",     "",     "",     "",     "",     "",     "",     "",
"a+=a", "a+=b", "a+=c", "a+=d", "a+=*b","a+=*c","a+=*d","a+=",
"a-=a", "a-=b", "a-=c", "a-=d", "a-=*b","a-=*c","a-=*d","a-=",
"a*=a", "a*=b", "a*=c", "a*=d", "a*=*b","a*=*c","a*=*d","a*=",
"a/=a", "a/=b", "a/=c", "a/=d", "a/=*b","a/=*c","a/=*d","a/=",
"a%=a", "a%=b", "a%=c", "a%=d", "a%=*b","a%=*c","a%=*d","a%=",
"a&=a", "a&=b", "a&=c", "a&=d", "a&=*b","a&=*c","a&=*d","a&=",
"a&~a", "a&~b", "a&~c", "a&~d", "a&~*b","a&~*c","a&~*d","a&~",
"a|=a", "a|=b", "a|=c", "a|=d", "a|=*b","a|=*c","a|=*d","a|=",
"a^=a", "a^=b", "a^=c", "a^=d", "a^=*b","a^=*c","a^=*d","a^=",
"a<<=a","a<<=b","a<<=c","a<<=d","a<<=*b","a<<=*c","a<<=*d","a<<=",
"a>>=a","a>>=b","a>>=c","a>>=d","a>>=*b","a>>=*c","a>>=*d","a>>=",
"a==a", "a==b", "a==c", "a==d", "a==*b","a==*c","a==*d","a==",
"a<a",  "a<b",  "a<c",  "a<d",  "a<*b", "a<*c", "a<*d", "a<",
"a>a",  "a>b",  "a>c",  "a>d",  "a>*b", "a>*c", "a>*d", "a>",
"",     "",     "",     "",     "",     "",     "",     "",
"",     "",     "",     "",     "",     "",     "",     "lj",
"post", "pcomp","end",  "if",   "ifnot","else", "endif","do",
"while","until","forever","ifl","ifnotl","elsel",";",    0};

// Advance in to start of next token. Tokens are delimited by white
// space. Comments inclosed in ((nested) parenthsis) are skipped.
void Compiler::next() {
  assert(in);
  for (; *in; ++in) {
	if (*in=='\n') ++line;
	if (*in=='(') state+=1+(state<0);
	else if (state>0 && *in==')') --state;
	else if (state<0 && *in<=' ') state=0;
	else if (state==0 && *in>' ') {state=-1; break;}
  }
  if (!*in) error("unexpected end of config");
}

// convert to lower case
int tolower(int c) {return (c>='A' && c<='Z') ? c+'a'-'A' : c;}

// return true if in==word up to white space or '(', case insensitive
bool Compiler::matchToken(const char* word) {
  const char* a=in;
  for (; (*a>' ' && *a!='(' && *word); ++a, ++word)
	if (tolower(*a)!=tolower(*word)) return false;
  return !*word && (*a<=' ' || *a=='(');
}

// Print error message and exit
void Compiler::syntaxError(const char* msg, const char* expected) {
  Array<char> sbuf(128);  // error message to report
  char* s=&sbuf[0];
  strcat(s, "Config line ");
  for (int i=strlen(s), r=1000000; r; r/=10)  // append line number
	if (line/r) s[i++]='0'+line/r%10;
  strcat(s, " at ");
  for (int i=strlen(s); i<40 && *in>' '; ++i)  // append token found
	s[i]=*in++;
  strcat(s, ": ");
  strncat(s, msg, 40);  // append message
  if (expected) {
	strcat(s, ", expected: ");
	strncat(s, expected, 20);  // append expected token if any
  }
  error(s);
}

// Read a token, which must be in the NULL terminated list or else
// exit with an error. If found, return its index.
int Compiler::rtoken(const char* list[]) {
  assert(in);
  assert(list);
  next();
  for (int i=0; list[i]; ++i)
	if (matchToken(list[i]))
	  return i;
  syntaxError("unexpected");
  assert(0);
  return -1; // not reached
}

// Read a token which must be the specified value s
void Compiler::rtoken(const char* s) {
  assert(s);
  next();
  if (!matchToken(s)) syntaxError("expected", s);
}

// Read a number in (low...high) or exit with an error
// For numbers like $N+M, return arg[N-1]+M
int Compiler::rtoken(int low, int high) {
  next();
  int r=0;
  if (in[0]=='$' && in[1]>='1' && in[1]<='9') {
	if (in[2]=='+') r=atoi(in+3);
	if (args) r+=args[in[1]-'1'];
  }
  else if (in[0]=='-' || (in[0]>='0' && in[0]<='9')) r=atoi(in);
  else syntaxError("expected a number");
  if (r<low) syntaxError("number too low");
  if (r>high) syntaxError("number too high");
  return r;
}

// Compile HCOMP or PCOMP code. Exit on error. Return
// code for end token (POST, PCOMP, END)
int Compiler::compile_comp(ZPAQL& z) {
  int op=0;
  const int comp_begin=z.hend;
  while (true) {
	op=rtoken(opcodelist);
	if (op==POST || op==PCOMP || op==END) break;
	int operand=-1; // 0...255 if 2 bytes
	int operand2=-1;  // 0...255 if 3 bytes
	if (op==IF) {
	  op=JF;
	  operand=0; // set later
	  if_stack.push(z.hend+1); // save jump target location
	}
	else if (op==IFNOT) {
	  op=JT;
	  operand=0;
	  if_stack.push(z.hend+1); // save jump target location
	}
	else if (op==IFL || op==IFNOTL) {  // long if
	  if (op==IFL) z.header[z.hend++]=(JT);
	  if (op==IFNOTL) z.header[z.hend++]=(JF);
	  z.header[z.hend++]=(3);
	  op=LJ;
	  operand=operand2=0;
	  if_stack.push(z.hend+1);
	}
	else if (op==ELSE || op==ELSEL) {
	  if (op==ELSE) op=JMP, operand=0;
	  if (op==ELSEL) op=LJ, operand=operand2=0;
	  int a=if_stack.pop();  // conditional jump target location
	  assert(a>comp_begin && a<int(z.hend));
	  if (z.header[a-1]!=LJ) {  // IF, IFNOT
		assert(z.header[a-1]==JT || z.header[a-1]==JF || z.header[a-1]==JMP);
		int j=z.hend-a+1+(op==LJ); // offset at IF
		assert(j>=0);
		if (j>127) syntaxError("IF too big, try IFL, IFNOTL");
		z.header[a]=j;
	  }
	  else {  // IFL, IFNOTL
		int j=z.hend-comp_begin+2+(op==LJ);
		assert(j>=0);
		z.header[a]=j&255;
		z.header[a+1]=(j>>8)&255;
	  }
	  if_stack.push(z.hend+1);  // save JMP target location
	}
	else if (op==ENDIF) {
	  int a=if_stack.pop();  // jump target address
	  assert(a>comp_begin && a<int(z.hend));
	  int j=z.hend-a-1;  // jump offset
	  assert(j>=0);
	  if (z.header[a-1]!=LJ) {
		assert(z.header[a-1]==JT || z.header[a-1]==JF || z.header[a-1]==JMP);
		if (j>127) syntaxError("IF too big, try IFL, IFNOTL, ELSEL\n");
		z.header[a]=j;
	  }
	  else {
		assert(a+1<int(z.hend));
		j=z.hend-comp_begin;
		z.header[a]=j&255;
		z.header[a+1]=(j>>8)&255;
	  }
	}
	else if (op==DO) {
	  do_stack.push(z.hend);
	}
	else if (op==WHILE || op==UNTIL || op==FOREVER) {
	  int a=do_stack.pop();
	  assert(a>=comp_begin && a<int(z.hend));
	  int j=a-z.hend-2;
	  assert(j<=-2);
	  if (j>=-127) {  // backward short jump
		if (op==WHILE) op=JT;
		if (op==UNTIL) op=JF;
		if (op==FOREVER) op=JMP;
		operand=j&255;
	  }
	  else {  // backward long jump
		j=a-comp_begin;
		assert(j>=0 && j<int(z.hend)-comp_begin);
		if (op==WHILE) {
		  z.header[z.hend++]=(JF);
		  z.header[z.hend++]=(3);
		}
		if (op==UNTIL) {
		  z.header[z.hend++]=(JT);
		  z.header[z.hend++]=(3);
		}
		op=LJ;
		operand=j&255;
		operand2=j>>8;
	  }
	}
	else if ((op&7)==7) { // 2 byte operand, read N
	  if (op==LJ) {
		operand=rtoken(0, 65535);
		operand2=operand>>8;
		operand&=255;
	  }
	  else if (op==JT || op==JF || op==JMP) {
		operand=rtoken(-128, 127);
		operand&=255;
	  }
	  else
		operand=rtoken(0, 255);
	}
	if (op>=0 && op<=255)
	  z.header[z.hend++]=(op);
	if (operand>=0)
	  z.header[z.hend++]=(operand);
	if (operand2>=0)
	  z.header[z.hend++]=(operand2);
	if (z.hend>=z.header.isize()-130 || z.hend-z.hbegin+z.cend-2>65535)
	  syntaxError("program too big");
  }
  z.header[z.hend++]=(0); // END
  return op;
}

// Compile a configuration file. Store COMP/HCOMP section in hcomp.
// If there is a PCOMP section, store it in pcomp and store the PCOMP
// command in pcomp_cmd. Replace "$1..$9+n" with args[0..8]+n

Compiler::Compiler(const char* in_, int* args_, ZPAQL& hz_, ZPAQL& pz_,
				   Writer* out2_): in(in_), args(args_), hz(hz_), pz(pz_),
				   out2(out2_), if_stack(1000), do_stack(1000) {
  line=1;
  state=0;
  hz.clear();
  pz.clear();
  hz.header.resize(68000);

  // Compile the COMP section of header
  rtoken("comp");
  hz.header[2]=rtoken(0, 255);  // hh
  hz.header[3]=rtoken(0, 255);  // hm
  hz.header[4]=rtoken(0, 255);  // ph
  hz.header[5]=rtoken(0, 255);  // pm
  const int n=hz.header[6]=rtoken(0, 255);  // n
  hz.cend=7;
  for (int i=0; i<n; ++i) {
	rtoken(i, i);
	CompType type=CompType(rtoken(compname));
	hz.header[hz.cend++]=type;
	int clen=libzpaq::compsize[type&255];
	if (clen<1 || clen>10) syntaxError("invalid component");
	for (int j=1; j<clen; ++j)
	  hz.header[hz.cend++]=rtoken(0, 255);  // component arguments
  }
  hz.header[hz.cend++];  // end
  hz.hbegin=hz.hend=hz.cend+128;

  // Compile HCOMP
  rtoken("hcomp");
  int op=compile_comp(hz);

  // Compute header size
  int hsize=hz.cend-2+hz.hend-hz.hbegin;
  hz.header[0]=hsize&255;
  hz.header[1]=hsize>>8;

  // Compile POST 0 END
  if (op==POST) {
	rtoken(0, 0);
	rtoken("end");
  }

  // Compile PCOMP pcomp_cmd ; program... END
  else if (op==PCOMP) {
	pz.header.resize(68000);
	pz.header[4]=hz.header[4];  // ph
	pz.header[5]=hz.header[5];  // pm
	pz.cend=8;
	pz.hbegin=pz.hend=pz.cend+128;

	// get pcomp_cmd ending with ";" (case sensitive)
	next();
	while (*in && *in!=';') {
	  if (out2)
		out2->put(*in);
	  ++in;
	}
	if (*in) ++in;

	// Compile PCOMP
	op=compile_comp(pz);
	int len=pz.cend-2+pz.hend-pz.hbegin;  // insert header size
	assert(len>=0);
	pz.header[0]=len&255;
	pz.header[1]=len>>8;
	if (op!=END)
	  syntaxError("expected END");
  }
  else if (op!=END)
	syntaxError("expected END or POST 0 END or PCOMP cmd ; ... END");
}

///////////////////// Compressor //////////////////////

// Write 13 byte start tag
// "\x37\x6B\x53\x74\xA0\x31\x83\xD3\x8C\xB2\x28\xB0\xD3"
void Compressor::writeTag() {
  assert(state==INIT);
  enc.out->put(0x37);
  enc.out->put(0x6b);
  enc.out->put(0x53);
  enc.out->put(0x74);
  enc.out->put(0xa0);
  enc.out->put(0x31);
  enc.out->put(0x83);
  enc.out->put(0xd3);
  enc.out->put(0x8c);
  enc.out->put(0xb2);
  enc.out->put(0x28);
  enc.out->put(0xb0);
  enc.out->put(0xd3);
}

void Compressor::startBlock(int level) {

  // Model 1 - min.cfg
  static const char models[]={
  26,0,1,2,0,0,2,3,16,8,19,0,0,96,4,28,
  59,10,59,112,25,10,59,10,59,112,56,0,

  // Model 2 - mid.cfg
  69,0,3,3,0,0,8,3,5,8,13,0,8,17,1,8,
  18,2,8,18,3,8,19,4,4,22,24,7,16,0,7,24,
  -1,0,17,104,74,4,95,1,59,112,10,25,59,112,10,25,
  59,112,10,25,59,112,10,25,59,112,10,25,59,10,59,112,
  25,69,-49,8,112,56,0,

  // Model 3 - max.cfg
  -60,0,5,9,0,0,22,1,-96,3,5,8,13,1,8,16,
  2,8,18,3,8,19,4,8,19,5,8,20,6,4,22,24,
  3,17,8,19,9,3,13,3,13,3,13,3,14,7,16,0,
  15,24,-1,7,8,0,16,10,-1,6,0,15,16,24,0,9,
  8,17,32,-1,6,8,17,18,16,-1,9,16,19,32,-1,6,
  0,19,20,16,0,0,17,104,74,4,95,2,59,112,10,25,
  59,112,10,25,59,112,10,25,59,112,10,25,59,112,10,25,
  59,10,59,112,10,25,59,112,10,25,69,-73,32,-17,64,47,
  14,-25,91,47,10,25,60,26,48,-122,-105,20,112,63,9,70,
  -33,0,39,3,25,112,26,52,25,25,74,10,4,59,112,25,
  10,4,59,112,25,10,4,59,112,25,65,-113,-44,72,4,59,
  112,8,-113,-40,8,68,-81,60,60,25,69,-49,9,112,25,25,
  25,25,25,112,56,0,

  0,0}; // 0,0 = end of list

  if (level<1) error("compression level must be at least 1");
  const char* p=models;
  int i;
  for (i=1; i<level && toU16(p); ++i)
	p+=toU16(p)+2;
  if (toU16(p)<1) error("compression level too high");
  startBlock(p);
}

// Memory reader
class MemoryReader: public Reader {
  const char* p;
public:
  MemoryReader(const char* p_): p(p_) {}
  int get() {return *p++&255;}
};

void Compressor::startBlock(const char* hcomp) {
  assert(state==INIT);
  MemoryReader m(hcomp);
  z.read(&m);
  pz.sha1=&sha1;
  assert(z.header.isize()>6);
  enc.out->put('z');
  enc.out->put('P');
  enc.out->put('Q');
  enc.out->put(1+(z.header[6]==0));  // level 1 or 2
  enc.out->put(1);
  z.write(enc.out, false);
  state=BLOCK1;
}

void Compressor::startBlock(const char* config, int* args, Writer* pcomp_cmd) {
  assert(state==INIT);
  Compiler(config, args, z, pz, pcomp_cmd);
  pz.sha1=&sha1;
  assert(z.header.isize()>6);
  enc.out->put('z');
  enc.out->put('P');
  enc.out->put('Q');
  enc.out->put(1+(z.header[6]==0));  // level 1 or 2
  enc.out->put(1);
  z.write(enc.out, false);
  state=BLOCK1;
}

// Write a segment header
void Compressor::startSegment(const char* filename, const char* comment) {
  assert(state==BLOCK1 || state==BLOCK2);
  enc.out->put(1);
  while (filename && *filename)
	enc.out->put(*filename++);
  enc.out->put(0);
  while (comment && *comment)
	enc.out->put(*comment++);
  enc.out->put(0);
  enc.out->put(0);
  if (state==BLOCK1) state=SEG1;
  if (state==BLOCK2) state=SEG2;
}

// Initialize encoding and write pcomp to first segment
// If len is 0 then length is encoded in pcomp[0..1]
// if pcomp is 0 then get pcomp from pz.header
void Compressor::postProcess(const char* pcomp, int len) {
  if (state==SEG2) return;
  assert(state==SEG1);
  enc.init();
  if (!pcomp) {
	len=pz.hend-pz.hbegin;
	if (len>0) {
	  assert(pz.header.isize()>pz.hend);
	  assert(pz.hbegin>=0);
	  pcomp=(const char*)&pz.header[pz.hbegin];
	}
	assert(len>=0);
  }
  else if (len==0) {
	len=toU16(pcomp);
	pcomp+=2;
  }
  if (len>0) {
	enc.compress(1);
	enc.compress(len&255);
	enc.compress((len>>8)&255);
	for (int i=0; i<len; ++i)
	  enc.compress(pcomp[i]&255);
	if (verify)
	  pz.initp();
  }
  else
	enc.compress(0);
  state=SEG2;
}

// Compress n bytes, or to EOF if n < 0
bool Compressor::compress(int n) {
  if (state==SEG1)
	postProcess();
  assert(state==SEG2);

  const int BUFSIZE=1<<14;
  char buf[BUFSIZE];  // input buffer
  while (n) {
	int nbuf=BUFSIZE;  // bytes read into buf
	if (n>=0 && n<nbuf) nbuf=n;
	int nr=in->read(buf, nbuf);
	if (nr<0 || nr>BUFSIZE || nr>nbuf) error("invalid read size");
	if (nr<=0) return false;
	if (n>=0) n-=nr;
	for (int i=0; i<nr; ++i) {
	  int ch=U8(buf[i]);
	  enc.compress(ch);
	  if (verify) {
		if (pz.hend) pz.run(ch);
		else sha1.put(ch);
	  }
	}
  }
  return true;
}

// End segment, write sha1string if present
void Compressor::endSegment(const char* sha1string) {
  if (state==SEG1)
	postProcess();
  assert(state==SEG2);
  enc.compress(-1);
  if (verify && pz.hend) {
	pz.run(-1);
	pz.flush();
  }
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  if (sha1string) {
	enc.out->put(253);
	for (int i=0; i<20; ++i)
	  enc.out->put(sha1string[i]);
  }
  else
	enc.out->put(254);
  state=BLOCK2;
}

// End segment, write checksum and size is verify is true
char* Compressor::endSegmentChecksum(int64_t* size) {
  if (state==SEG1)
	postProcess();
  assert(state==SEG2);
  enc.compress(-1);
  if (verify && pz.hend) {
	pz.run(-1);
	pz.flush();
  }
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  if (verify) {
	if (size) *size=sha1.usize();
	memcpy(sha1result, sha1.result(), 20);
	enc.out->put(253);
	for (int i=0; i<20; ++i)
	  enc.out->put(sha1result[i]);
  }
  else
	enc.out->put(254);
  state=BLOCK2;
  return verify ? sha1result : 0;
}

// End block
void Compressor::endBlock() {
  assert(state==BLOCK2);
  enc.out->put(255);
  state=INIT;
}

/////////////////////////// compress() ///////////////////////

void compress(Reader* in, Writer* out, int level) {
  assert(level>=1);
  Compressor c;
  c.setInput(in);
  c.setOutput(out);
  c.startBlock(level);
  c.startSegment();
  c.compress();
  c.endSegment();
  c.endBlock();
}

//////////////////////// ZPAQL::assemble() ////////////////////

#ifndef NOJIT
/*
assemble();

Assembles the ZPAQL code in hcomp[0..hlen-1] and stores x86-32 or x86-64
code in rcode[0..rcode_size-1]. Execution begins at rcode[0]. It will not
write beyond the end of rcode, but in any case it returns the number of
bytes that would have been written. It returns 0 in case of error.

The assembled code implements run() and returns 1 if successful or
0 if the ZPAQL code executes an invalid instruction or jumps out of
bounds.

A ZPAQL virtual machine has the following state. All values are
unsigned and initially 0:

  a, b, c, d: 32 bit registers (pointed to by their respective parameters)
  f: 1 bit flag register (pointed to)
  r[0..255]: 32 bit registers
  m[0..msize-1]: 8 bit registers, where msize is a power of 2
  h[0..hsize-1]: 32 bit registers, where hsize is a power of 2
  out: pointer to a Writer
  sha1: pointer to a SHA1

Generally a ZPAQL machine is used to compute contexts which are
placed in h. A second machine might post-process, and write its
output to out and sha1. In either case, a machine is called with
its input in a, representing a single byte (0..255) or
(for a postprocessor) EOF (0xffffffff). Execution returs after a
ZPAQL halt instruction.

ZPAQL instructions are 1 byte unless the last 3 bits are 1.
In this case, a second operand byte follows. Opcode 255 is
the only 3 byte instruction. They are organized:

  00dddxxx = unary opcode xxx on destination ddd (ddd < 111)
  00111xxx = special instruction xxx
  01dddsss = assignment: ddd = sss (ddd < 111)
  1xxxxsss = operation xxxx from sss to a

The meaning of sss and ddd are as follows:

  000 = a   (accumulator)
  001 = b
  010 = c
  011 = d
  100 = *b  (means m[b mod msize])
  101 = *c  (means m[c mod msize])
  110 = *d  (means h[d mod hsize])
  111 = n   (constant 0..255 in second byte of instruction)

For example, 01001110 assigns *d to b. The other instructions xxx
are as follows:

Group 00dddxxx where ddd < 111 and xxx is:
  000 = ddd<>a, swap with a (except 00000000 is an error, and swap
		with *b or *c leaves the high bits of a unchanged)
  001 = ddd++, increment
  010 = ddd--, decrement
  011 = ddd!, not (invert all bits)
  100 = ddd=0, clear (set all bits of ddd to 0)
  101 = not used (error)
  110 = not used
  111 = ddd=r n, assign from r[n] to ddd, n=0..255 in next opcode byte
Except:
  00100111 = jt n, jump if f is true (n = -128..127, relative to next opcode)
  00101111 = jf n, jump if f is false (n = -128..127)
  00110111 = r=a n, assign r[n] = a (n = 0..255)

Group 00111xxx where xxx is:
  000 = halt (return)
  001 = output a
  010 = not used
  011 = hash: a = (a + *b + 512) * 773
  100 = hashd: *d = (*d + a + 512) * 773
  101 = not used
  110 = not used
  111 = unconditional jump (n = -128 to 127, relative to next opcode)

Group 1xxxxsss where xxxx is:
  0000 = a += sss (add, subtract, multiply, divide sss to a)
  0001 = a -= sss
  0010 = a *= sss
  0011 = a /= sss (unsigned, except set a = 0 if sss is 0)
  0100 = a %= sss (remainder, except set a = 0 if sss is 0)
  0101 = a &= sss (bitwise AND)
  0110 = a &= ~sss (bitwise AND with complement of sss)
  0111 = a |= sss (bitwise OR)
  1000 = a ^= sss (bitwise XOR)
  1001 = a <<= (sss % 32) (left shift by low 5 bits of sss)
  1010 = a >>= (sss % 32) (unsigned, zero bits shifted in)
  1011 = a == sss (compare, set f = true if equal or false otherwise)
  1100 = a < sss (unsigned compare, result in f)
  1101 = a > sss (unsigned compare)
  1110 = not used
  1111 = not used except 11111111 is a 3 byte jump to the absolute address
		 in the next 2 bytes in little-endian (LSB first) order.

assemble() translates ZPAQL to 32 bit x86 code to be executed by run().
Registers are mapped as follows:

  eax = source sss from *b, *c, *d or sometimes n
  ecx = pointer to destination *b, *c, *d, or spare
  edx = a
  ebx = f (1 for true, 0 for false)
  esp = stack pointer
  ebp = d
  esi = b
  edi = c

run() saves non-volatile registers (ebp, esi, edi, ebx) on the stack,
loads a, b, c, d, f, and executes the translated instructions.
A halt instruction saves a, b, c, d, f, pops the saved registers
and returns. Invalid instructions or jumps outside of the range
of the ZPAQL code call libzpaq::error().

In 64 bit mode, the following additional registers are used:

  r12 = h
  r14 = r
  r15 = m

*/

// Called by out
static void flush1(ZPAQL* z) {
  z->flush();
}

// return true if op is an undefined ZPAQL instruction
static bool iserr(int op) {
  return op==0 || (op>=120 && op<=127) || (op>=240 && op<=254)
	|| op==58 || (op<64 && (op%8==5 || op%8==6));
}

// Write k bytes of x to rcode[o++] MSB first
static void put(U8* rcode, int n, int& o, U32 x, int k) {
  while (k-->0) {
	if (o<n) rcode[o]=(x>>(k*8))&255;
	++o;
  }
}

// Write 4 bytes of x to rcode[o++] LSB first
static void put4lsb(U8* rcode, int n, int& o, U32 x) {
  for (int k=0; k<4; ++k) {
	if (o<n) rcode[o]=(x>>(k*8))&255;
	++o;
  }
}

// Write a 1-4 byte x86 opcode without or with an 4 byte operand
// to rcode[o...]
#define put1(x) put(rcode, rcode_size, o, (x), 1)
#define put2(x) put(rcode, rcode_size, o, (x), 2)
#define put3(x) put(rcode, rcode_size, o, (x), 3)
#define put4(x) put(rcode, rcode_size, o, (x), 4)
#define put5(x,y) put4(x), put1(y)
#define put6(x,y) put4(x), put2(y)
#define put4r(x) put4lsb(rcode, rcode_size, o, x)
#define puta(x) t=U32(size_t(x)), put4r(t)
#define put1a(x,y) put1(x), puta(y)
#define put2a(x,y) put2(x), puta(y)
#define put3a(x,y) put3(x), puta(y)
#define put4a(x,y) put4(x), puta(y)
#define put5a(x,y,z) put4(x), put1(y), puta(z)
#define put2l(x,y) put2(x), t=U32(size_t(y)), put4r(t), \
  t=U32(size_t(y)>>(S*4)), put4r(t)

// Assemble ZPAQL in in the HCOMP section of header to rcode,
// but do not write beyond rcode_size. Return the number of
// bytes output or that would have been output.
// Execution starts at rcode[0] and returns 1 if successful or 0
// in case of a ZPAQL execution error.
int ZPAQL::assemble() {

  // x86? (not foolproof)
  const int S=sizeof(char*);      // 4 = x86, 8 = x86-64
  U32 t=0x12345678;
  if (*(char*)&t!=0x78 || (S!=4 && S!=8))
	error("JIT supported only for x86-32 and x86-64");

  const U8* hcomp=&header[hbegin];
  const int hlen=hend-hbegin+1;
  const int msize=m.size();
  const int hsize=h.size();
  static const int regcode[8]={2,6,7,5}; // a,b,c,d.. -> edx,esi,edi,ebp,eax..
  Array<int> it(hlen);            // hcomp -> rcode locations
  int done=0;  // number of instructions assembled (0..hlen)
  int o=5;  // rcode output index, reserve space for jmp

  // Code for the halt instruction (restore registers and return)
  const int halt=o;
  if (S==8) {
	put2l(0x48b9, &a);        // mov rcx, a
	put2(0x8911);             // mov [rcx], edx
	put2l(0x48b9, &b);        // mov rcx, b
	put2(0x8931);             // mov [rcx], esi
	put2l(0x48b9, &c);        // mov rcx, c
	put2(0x8939);             // mov [rcx], edi
	put2l(0x48b9, &d);        // mov rcx, d
	put2(0x8929);             // mov [rcx], ebp
	put2l(0x48b9, &f);        // mov rcx, f
	put2(0x8919);             // mov [rcx], ebx
	put4(0x4883c438);         // add rsp, 56
	put2(0x415f);             // pop r15
	put2(0x415e);             // pop r14
	put2(0x415d);             // pop r13
	put2(0x415c);             // pop r12
  }
  else {
	put2a(0x8915, &a);        // mov [a], edx
	put2a(0x8935, &b);        // mov [b], esi
	put2a(0x893d, &c);        // mov [c], edi
	put2a(0x892d, &d);        // mov [d], ebp
	put2a(0x891d, &f);        // mov [f], ebx
	put3(0x83c43c);           // add esp, 60
  }
  put1(0x5d);                 // pop ebp
  put1(0x5b);                 // pop ebx
  put1(0x5f);                 // pop edi
  put1(0x5e);                 // pop esi
  put1(0xc3);                 // ret

  // Code for the out instruction.
  // Store a=edx at outbuf[bufptr++]. If full, call flush1().
  const int outlabel=o;
  if (S==8) {
	put2l(0x48b8, &outbuf[0]);// mov rax, outbuf.p
	put2l(0x49ba, &bufptr);   // mov r10, &bufptr
	put3(0x418b0a);           // mov rcx, [r10]
	put3(0x881408);           // mov [rax+rcx], dl
	put2(0xffc1);             // inc rcx
	put3(0x41890a);           // mov [r10], ecx
	put2a(0x81f9, outbuf.size());  // cmp rcx, outbuf.size()
	put2(0x7401);             // jz L1
	put1(0xc3);               // ret
	put4(0x4883ec48);         // L1: sub rsp, 72  ; call flush1(this)
	put5(0x48897c24,64);      // mov [rsp+64], rdi
	put5(0x48897424,56);      // mov [rsp+56], rsi
	put5(0x48895424,48);      // mov [rsp+48], rdx
	put5(0x48894c24,40);      // mov [rsp+40], rcx
#if defined(unix) && !defined(__CYGWIN__)
	put2l(0x48bf, this);      // mov rdi, this
#else  // Windows
	put2l(0x48b9, this);      // mov rcx, this
#endif
	put2l(0x49bb, &flush1);   // mov r11, &flush1
	put3(0x41ffd3);           // call r11
	put5(0x488b4c24,40);      // mov rcx, [rsp+40]
	put5(0x488b5424,48);      // mov rdx, [rsp+48]
	put5(0x488b7424,56);      // mov rsi, [rsp+56]
	put5(0x488b7c24,64);      // mov rdi, [rsp+64]
	put4(0x4883c448);         // add rsp, 72
	put1(0xc3);               // ret
  }
  else {
	put1a(0xb8, &outbuf[0]);  // mov eax, outbuf.p
	put2a(0x8b0d, &bufptr);   // mov ecx, [bufptr]
	put3(0x881408);           // mov [eax+ecx], dl
	put2(0xffc1);             // inc ecx
	put2a(0x890d, &bufptr);   // mov [bufptr], ecx
	put2a(0x81f9, outbuf.size());  // cmp ecx, outbuf.size()
	put2(0x7401);             // jz L1
	put1(0xc3);               // ret
	put3(0x83ec0c);           // L1: sub esp, 12
	put4(0x89542404);         // mov [esp+4], edx
	put3a(0xc70424, this);    // mov [esp], this
	put1a(0xb8, &flush1);     // mov eax, &flush1
	put2(0xffd0);             // call eax
	put4(0x8b542404);         // mov edx, [esp+4]
	put3(0x83c40c);           // add esp, 12
	put1(0xc3);               // ret
  }

  // Set it[i]=1 for each ZPAQL instruction reachable from the previous
  // instruction + 2 if reachable by a jump (or 3 if both).
  it[0]=2;
  assert(hlen>0 && hcomp[hlen-1]==0);  // ends with error
  do {
	done=0;
	const int NONE=0x80000000;
	for (int i=0; i<hlen; ++i) {
	  int op=hcomp[i];
	  if (it[i]) {
		int next1=i+1+(op%8==7), next2=NONE; // next and jump targets
		if (iserr(op)) next1=NONE;  // error
		if (op==56) next1=NONE, next2=0;  // halt
		if (op==255) next1=NONE, next2=hcomp[i+1]+256*hcomp[i+2]; // lj
		if (op==39||op==47||op==63)next2=i+2+(hcomp[i+1]<<24>>24);// jt,jf,jmp
		if (op==63) next1=NONE;  // jmp
		if ((next2<0 || next2>=hlen) && next2!=NONE) next2=hlen-1; // error
		if (next1!=NONE && !(it[next1]&1)) it[next1]|=1, ++done;
		if (next2!=NONE && !(it[next2]&2)) it[next2]|=2, ++done;
	  }
	}
  } while (done>0);

  // Set it[i] bits 2-3 to 4, 8, or 12 if a comparison
  //  (==, <, > respectively) does not need to save the result in f,
  // or if a conditional jump (jt, jf) does not need to read f.
  // This is true if a comparison is followed directly by a jt/jf,
  // the jt/jf is not a jump target, the byte before is not a jump
  // target (for a 2 byte comparison), and for the comparison instruction
  // if both paths after the jt/jf lead to another comparison or error
  // before another jt/jf. At most hlen steps are traced because after
  // that it must be an infinite loop.
  for (int i=0; i<hlen; ++i) {
	const int op1=hcomp[i]; // 216..239 = comparison
	const int i2=i+1+(op1%8==7);  // address of next instruction
	const int op2=hcomp[i2];  // 39,47 = jt,jf
	if (it[i] && op1>=216 && op1<240 && (op2==39 || op2==47)
		&& it[i2]==1 && (i2==i+1 || it[i+1]==0)) {
	  int code=(op1-208)/8*4; // 4,8,12 is ==,<,>
	  it[i2]+=code;  // OK to test CF, ZF instead of f
	  for (int j=0; j<2 && code; ++j) {  // trace each path from i2
		int k=i2+2; // branch not taken
		if (j==1) k=i2+2+(hcomp[i2+1]<<24>>24);  // branch taken
		for (int l=0; l<hlen && code; ++l) {  // trace at most hlen steps
		  if (k<0 || k>=hlen) break;  // out of bounds, pass
		  const int op=hcomp[k];
		  if (op==39 || op==47) code=0;  // jt,jf, fail
		  else if (op>=216 && op<240) break;  // ==,<,>, pass
		  else if (iserr(op)) break;  // error, pass
		  else if (op==255) k=hcomp[k+1]+256*hcomp[k+2]; // lj
		  else if (op==63) k=k+2+(hcomp[k+1]<<24>>24);  // jmp
		  else if (op==56) k=0;  // halt
		  else k=k+1+(op%8==7);  // ordinary instruction
		}
	  }
	  it[i]+=code;  // if > 0 then OK to not save flags in f (bl)
	}
  }

  // Start of run(): Save x86 and load ZPAQL registers
  const int start=o;
  assert(start>=16);
  put1(0x56);          // push esi/rsi
  put1(0x57);          // push edi/rdi
  put1(0x53);          // push ebx/rbx
  put1(0x55);          // push ebp/rbp
  if (S==8) {
	put2(0x4154);      // push r12
	put2(0x4155);      // push r13
	put2(0x4156);      // push r14
	put2(0x4157);      // push r15
	put4(0x4883ec38);  // sub rsp, 56
	put2l(0x48b8, &a); // mov rax, a
	put2(0x8b10);      // mov edx, [rax]
	put2l(0x48b8, &b); // mov rax, b
	put2(0x8b30);      // mov esi, [rax]
	put2l(0x48b8, &c); // mov rax, c
	put2(0x8b38);      // mov edi, [rax]
	put2l(0x48b8, &d); // mov rax, d
	put2(0x8b28);      // mov ebp, [rax]
	put2l(0x48b8, &f); // mov rax, f
	put2(0x8b18);      // mov ebx, [rax]
	put2l(0x49bc, &h[0]);   // mov r12, h
	put2l(0x49bd, &outbuf[0]); // mov r13, outbuf.p
	put2l(0x49be, &r[0]);   // mov r14, r
	put2l(0x49bf, &m[0]);   // mov r15, m
  }
  else {
	put3(0x83ec3c);    // sub esp, 60
	put2a(0x8b15, &a); // mov edx, [a]
	put2a(0x8b35, &b); // mov esi, [b]
	put2a(0x8b3d, &c); // mov edi, [c]
	put2a(0x8b2d, &d); // mov ebp, [d]
	put2a(0x8b1d, &f); // mov ebx, [f]
  }

  // Assemble in multiple passes until every byte of hcomp has a translation
  for (int istart=0; istart<hlen; ++istart) {
	for (int i=istart; i<hlen&&it[i]; i=i+1+(hcomp[i]%8==7)+(hcomp[i]==255)) {
	  const int code=it[i];

	  // If already assembled, then assemble a jump to it
	  U32 t;
	  assert(it.isize()>i);
	  assert(i>=0 && i<hlen);
	  if (code>=16) {
		if (i>istart) {
		  int a=code-o;
		  if (a>-120 && a<120)
			put2(0xeb00+((a-2)&255)); // jmp short o
		  else
			put1a(0xe9, a-5);  // jmp near o
		}
		break;
	  }

	  // Else assemble the instruction at hcode[i] to rcode[o]
	  else {
		assert(i>=0 && i<it.isize());
		assert(it[i]>0 && it[i]<16);
		assert(o>=16);
		it[i]=o;
		++done;
		const int op=hcomp[i];
		const int arg=hcomp[i+1]+((op==255)?256*hcomp[i+2]:0);
		const int ddd=op/8%8;
		const int sss=op%8;

		// error instruction: return 0
		if (iserr(op)) {
		  put2(0x31c0);           // xor eax, eax
		  put1a(0xe9, halt-o-4);  // jmp near halt
		  continue;
		}

		// Load source *b, *c, *d, or hash (*b) into eax except:
		// {a,b,c,d}=*d, a{+,-,*,&,|,^,=,==,>,>}=*d: load address to eax
		// {a,b,c,d}={*b,*c}: load source into ddd
		if (op==59 || (op>=64 && op<240 && op%8>=4 && op%8<7)) {
		  put2(0x89c0+8*regcode[sss-3+(op==59)]);  // mov eax, {esi,edi,ebp}
		  const int sz=(sss==6?hsize:msize)-1;
		  if (sz>=128) put1a(0x25, sz);            // and eax, dword msize-1
		  else put3(0x83e000+sz);                  // and eax, byte msize-1
		  const int move=(op>=64 && op<112); // = or else ddd is eax
		  if (sss<6) { // ddd={a,b,c,d,*b,*c}
			if (S==8) put5(0x410fb604+8*move*regcode[ddd],0x07);
												   // movzx ddd, byte [r15+rax]
			else put3a(0x0fb680+8*move*regcode[ddd], &m[0]);
												   // movzx ddd, byte [m+eax]
		  }
		  else if ((0x06587000>>(op/8))&1) {// {*b,*c,*d,a/,a%,a&~,a<<,a>>}=*d
			if (S==8) put4(0x418b0484);            // mov eax, [r12+rax*4]
			else put3a(0x8b0485, &h[0]);           // mov eax, [h+eax*4]
		  }
		}

		// Load destination address *b, *c, *d or hashd (*d) into ecx
		if ((op>=32 && op<56 && op%8<5) || (op>=96 && op<120) || op==60) {
		  put2(0x89c1+8*regcode[op/8%8-3-(op==60)]);// mov ecx,{esi,edi,ebp}
		  const int sz=(ddd==6||op==60?hsize:msize)-1;
		  if (sz>=128) put2a(0x81e1, sz);   // and ecx, dword sz
		  else put3(0x83e100+sz);           // and ecx, byte sz
		  if (op/8%8==6 || op==60) { // *d
			if (S==8) put4(0x498d0c8c);     // lea rcx, [r12+rcx*4]
			else put3a(0x8d0c8d, &h[0]);    // lea ecx, [ecx*4+h]
		  }
		  else { // *b, *c
			if (S==8) put4(0x498d0c0f);     // lea rcx, [r15+rcx]
			else put2a(0x8d89, &m[0]);      // lea ecx, [ecx+h]
		  }
		}

		// Code runs of up to 127 identical ++ or -- operations that are not
		// otherwise jump targets as a single instruction. Mark all but
		// the first as not reachable.
		int inc=0;  // run length of identical ++ or -- instructions
		if (op<=50 && (op%8==1 || op%8==2)) {
		  assert(code>=1 && code<=3);
		  inc=1;
		  for (int j=i+1; inc<127 && j<hlen && it[j]==1 && hcomp[j]==op; ++j){
			++inc;
			it[j]=0;
		  }
		}

		// Translate by opcode
		switch((op/8)&31) {
		  case 0:  // ddd = a
		  case 1:  // ddd = b
		  case 2:  // ddd = c
		  case 3:  // ddd = d
			switch(sss) {
			  case 0:  // ddd<>a (swap)
				put2(0x87d0+regcode[ddd]);   // xchg edx, ddd
				break;
			  case 1:  // ddd++
				put3(0x83c000+256*regcode[ddd]+inc); // add ddd, inc
				break;
			  case 2:  // ddd--
				put3(0x83e800+256*regcode[ddd]+inc); // sub ddd, inc
				break;
			  case 3:  // ddd!
				put2(0xf7d0+regcode[ddd]);   // not ddd
				break;
			  case 4:  // ddd=0
				put2(0x31c0+9*regcode[ddd]); // xor ddd,ddd
				break;
			  case 7:  // ddd=r n
				if (S==8)
				  put3a(0x418b86+8*regcode[ddd], arg*4); // mov ddd, [r14+n*4]
				else
				  put2a(0x8b05+8*regcode[ddd], (&r[arg]));//mov ddd, [r+n]
				break;
			}
			break;
		  case 4:  // ddd = *b
		  case 5:  // ddd = *c
			switch(sss) {
			  case 0:  // ddd<>a (swap)
				put2(0x8611);                // xchg dl, [ecx]
				break;
			  case 1:  // ddd++
				put3(0x800100+inc);          // add byte [ecx], inc
				break;
			  case 2:  // ddd--
				put3(0x802900+inc);          // sub byte [ecx], inc
				break;
			  case 3:  // ddd!
				put2(0xf611);                // not byte [ecx]
				break;
			  case 4:  // ddd=0
				put2(0x31c0);                // xor eax, eax
				put2(0x8801);                // mov [ecx], al
				break;
			  case 7:  // jt, jf
			  {
				assert(code>=0 && code<16);
				static const unsigned char jtab[2][4]={{5,4,2,7},{4,5,3,6}};
							   // jnz,je,jb,ja, jz,jne,jae,jbe
				if (code<4) put2(0x84db);    // test bl, bl
				if (arg>=128 && arg-257-i>=0 && o-it[arg-257-i]<120)
				  put2(0x7000+256*jtab[op==47][code/4]); // jx short 0
				else
				  put2a(0x0f80+jtab[op==47][code/4], 0); // jx near 0
				break;
			  }
			}
			break;
		  case 6:  // ddd = *d
			switch(sss) {
			  case 0:  // ddd<>a (swap)
				put2(0x8711);             // xchg edx, [ecx]
				break;
			  case 1:  // ddd++
				put3(0x830100+inc);       // add dword [ecx], inc
				break;
			  case 2:  // ddd--
				put3(0x832900+inc);       // sub dword [ecx], inc
				break;
			  case 3:  // ddd!
				put2(0xf711);             // not dword [ecx]
				break;
			  case 4:  // ddd=0
				put2(0x31c0);             // xor eax, eax
				put2(0x8901);             // mov [ecx], eax
				break;
			  case 7:  // ddd=r n
				if (S==8)
				  put3a(0x418996, arg*4); // mov [r14+n*4], edx
				else
				  put2a(0x8915, &r[arg]); // mov [r+n], edx
				break;
			}
			break;
		  case 7:  // special
			switch(op) {
			  case 56: // halt
				put1a(0xb8, 1);           // mov eax, 1
				put1a(0xe9, halt-o-4);    // jmp near halt
				break;
			  case 57:  // out
				put1a(0xe8, outlabel-o-4);// call outlabel
				break;
			  case 59:  // hash: a = (a + *b + 512) * 773
				put3a(0x8d8410, 512);     // lea edx, [eax+edx+512]
				put2a(0x69d0, 773);       // imul edx, eax, 773
				break;
			  case 60:  // hashd: *d = (*d + a + 512) * 773
				put2(0x8b01);             // mov eax, [ecx]
				put3a(0x8d8410, 512);     // lea eax, [eax+edx+512]
				put2a(0x69c0, 773);       // imul eax, eax, 773
				put2(0x8901);             // mov [ecx], eax
				break;
			  case 63:  // jmp
				put1a(0xe9, 0);           // jmp near 0 (fill in target later)
				break;
			}
			break;
		  case 8:   // a=
		  case 9:   // b=
		  case 10:  // c=
		  case 11:  // d=
			if (sss==7)  // n
			  put1a(0xb8+regcode[ddd], arg);         // mov ddd, n
			else if (sss==6) { // *d
			  if (S==8)
				put4(0x418b0484+(regcode[ddd]<<11)); // mov ddd, [r12+rax*4]
			  else
				put3a(0x8b0485+(regcode[ddd]<<11),&h[0]);// mov ddd, [h+eax*4]
			}
			else if (sss<4) // a, b, c, d
			  put2(0x89c0+regcode[ddd]+8*regcode[sss]);// mov ddd,sss
			break;
		  case 12:  // *b=
		  case 13:  // *c=
			if (sss==7) put3(0xc60100+arg);          // mov byte [ecx], n
			else if (sss==0) put2(0x8811);           // mov byte [ecx], dl
			else {
			  if (sss<4) put2(0x89c0+8*regcode[sss]);// mov eax, sss
			  put2(0x8801);                          // mov byte [ecx], al
			}
			break;
		  case 14:  // *d=
			if (sss<7) put2(0x8901+8*regcode[sss]);  // mov [ecx], sss
			else put2a(0xc701, arg);                 // mov dword [ecx], n
			break;
		  case 15: break; // not used
		  case 16:  // a+=
			if (sss==6) {
			  if (S==8) put4(0x41031484);            // add edx, [r12+rax*4]
			  else put3a(0x031485, &h[0]);           // add edx, [h+eax*4]
			}
			else if (sss<7) put2(0x01c2+8*regcode[sss]);// add edx, sss
			else if (arg>128) put2a(0x81c2, arg);    // add edx, n
			else put3(0x83c200+arg);                 // add edx, byte n
			break;
		  case 17:  // a-=
			if (sss==6) {
			  if (S==8) put4(0x412b1484);            // sub edx, [r12+rax*4]
			  else put3a(0x2b1485, &h[0]);           // sub edx, [h+eax*4]
			}
			else if (sss<7) put2(0x29c2+8*regcode[sss]);// sub edx, sss
			else if (arg>=128) put2a(0x81ea, arg);   // sub edx, n
			else put3(0x83ea00+arg);                 // sub edx, byte n
			break;
		  case 18:  // a*=
			if (sss==6) {
			  if (S==8) put5(0x410faf14,0x84);       // imul edx, [r12+rax*4]
			  else put4a(0x0faf1485, &h[0]);         // imul edx, [h+eax*4]
			}
			else if (sss<7) put3(0x0fafd0+regcode[sss]);// imul edx, sss
			else if (arg>=128) put2a(0x69d2, arg);   // imul edx, n
			else put3(0x6bd200+arg);                 // imul edx, byte n
			break;
		  case 19:  // a/=
		  case 20:  // a%=
			if (sss<7) put2(0x89c1+8*regcode[sss]);  // mov ecx, sss
			else put1a(0xb9, arg);                   // mov ecx, n
			put2(0x85c9);                            // test ecx, ecx
			put3(0x0f44d1);                          // cmovz edx, ecx
			put2(0x7408-2*(op/8==20));               // jz (over rest)
			put2(0x89d0);                            // mov eax, edx
			put2(0x31d2);                            // xor edx, edx
			put2(0xf7f1);                            // div ecx
			if (op/8==19) put2(0x89c2);              // mov edx, eax
			break;
		  case 21:  // a&=
			if (sss==6) {
			  if (S==8) put4(0x41231484);            // and edx, [r12+rax*4]
			  else put3a(0x231485, &h[0]);           // and edx, [h+eax*4]
			}
			else if (sss<7) put2(0x21c2+8*regcode[sss]);// and edx, sss
			else if (arg>=128) put2a(0x81e2, arg);   // and edx, n
			else put3(0x83e200+arg);                 // and edx, byte n
			break;
		  case 22:  // a&~
			if (sss==7) {
			  if (arg<128) put3(0x83e200+(~arg&255));// and edx, byte ~n
			  else put2a(0x81e2, ~arg);              // and edx, ~n
			}
			else {
			  if (sss<4) put2(0x89c0+8*regcode[sss]);// mov eax, sss
			  put2(0xf7d0);                          // not eax
			  put2(0x21c2);                          // and edx, eax
			}
			break;
		  case 23:  // a|=
			if (sss==6) {
			  if (S==8) put4(0x410b1484);            // or edx, [r12+rax*4]
			  else put3a(0x0b1485, &h[0]);           // or edx, [h+eax*4]
			}
			else if (sss<7) put2(0x09c2+8*regcode[sss]);// or edx, sss
			else if (arg>=128) put2a(0x81ca, arg);   // or edx, n
			else put3(0x83ca00+arg);                 // or edx, byte n
			break;
		  case 24:  // a^=
			if (sss==6) {
			  if (S==8) put4(0x41331484);            // xor edx, [r12+rax*4]
			  else put3a(0x331485, &h[0]);           // xor edx, [h+eax*4]
			}
			else if (sss<7) put2(0x31c2+8*regcode[sss]);// xor edx, sss
			else if (arg>=128) put2a(0x81f2, arg);   // xor edx, byte n
			else put3(0x83f200+arg);                 // xor edx, n
			break;
		  case 25:  // a<<=
		  case 26:  // a>>=
			if (sss==7)  // sss = n
			  put3(0xc1e200+8*256*(op/8==26)+arg);   // shl/shr n
			else {
			  put2(0x89c1+8*regcode[sss]);           // mov ecx, sss
			  put2(0xd3e2+8*(op/8==26));             // shl/shr edx, cl
			}
			break;
		  case 27:  // a==
		  case 28:  // a<
		  case 29:  // a>
			if (sss==6) {
			  if (S==8) put4(0x413b1484);            // cmp edx, [r12+rax*4]
			  else put3a(0x3b1485, &h[0]);           // cmp edx, [h+eax*4]
			}
			else if (sss==7)  // sss = n
			  put2a(0x81fa, arg);                    // cmp edx, dword n
			else
			  put2(0x39c2+8*regcode[sss]);           // cmp edx, sss
			if (code<4) {
			  if (op/8==27) put3(0x0f94c3);          // setz bl
			  if (op/8==28) put3(0x0f92c3);          // setc bl
			  if (op/8==29) put3(0x0f97c3);          // seta bl
			}
			break;
		  case 30:  // not used
		  case 31:  // 255 = lj
			if (op==255) put1a(0xe9, 0);             // jmp near
			break;
		}
		if (inc>1) {  // skip run of identical ++ or --
		  i+=inc-1;
		  assert(i<hlen);
		  assert(it[i]==0);
		  assert(hcomp[i]==op);
		}
	  }
	}
  }

  // Finish first pass
  const int rsize=o;
  if (o>rcode_size) return rsize;

  // Fill in jump addresses (second pass)
  for (int i=0; i<hlen; ++i) {
	if (it[i]<16) continue;
	int op=hcomp[i];
	if (op==39 || op==47 || op==63 || op==255) {  // jt, jf, jmp, lj
	  int target=hcomp[i+1];
	  if (op==255) target+=hcomp[i+2]*256;  // lj
	  else {
		if (target>=128) target-=256;
		target+=i+2;
	  }
	  if (target<0 || target>=hlen) target=hlen-1;  // runtime ZPAQL error
	  o=it[i];
	  assert(o>=16 && o<rcode_size);
	  if ((op==39 || op==47) && rcode[o]==0x84) o+=2;  // jt, jf -> skip test
	  assert(o>=16 && o<rcode_size);
	  if (rcode[o]==0x0f) ++o;  // first byte of jz near, jnz near
	  assert(o<rcode_size);
	  op=rcode[o++];  // x86 opcode
	  target=it[target]-o;
	  if ((op>=0x72 && op<0x78) || op==0xeb) {  // jx, jmp short
		--target;
		if (target<-128 || target>127)
		  error("Cannot code x86 short jump");
		assert(o<rcode_size);
		rcode[o]=target&255;
	  }
	  else if ((op>=0x82 && op<0x88) || op==0xe9) // jx, jmp near
	  {
		target-=4;
		puta(target);
	  }
	  else assert(false);  // not a x86 jump
	}
  }

  // Jump to start
  o=0;
  put1a(0xe9, start-5);  // jmp near start
  return rsize;
}

//////////////////////// Predictor::assemble_p() /////////////////////

// Assemble the ZPAQL code in the HCOMP section of z.header to pcomp and
// return the number of bytes of x86 or x86-64 code written, or that would
// be written if pcomp were large enough. The code for predict() begins
// at pr.pcomp[0] and update() at pr.pcomp[5], both as jmp instructions.

// The assembled code is equivalent to int predict(Predictor*)
// and void update(Predictor*, int y); The Preditor address is placed in
// edi/rdi. The update bit y is placed in ebp/rbp.

int Predictor::assemble_p() {
  Predictor& pr=*this;
  U8* rcode=pr.pcode;         // x86 output array
  int rcode_size=pcode_size;  // output size
  int o=0;                    // output index in pcode
  const int S=sizeof(char*);  // 4 or 8
  U8* hcomp=&pr.z.header[0];  // The code to translate
#define off(x)  ((char*)&(pr.x)-(char*)&pr)
#define offc(x) ((char*)&(pr.comp[i].x)-(char*)&pr)

  // test for little-endian (probably x86)
  U32 t=0x12345678;
  if (*(char*)&t!=0x78 || (S!=4 && S!=8))
	error("JIT supported only for x86-32 and x86-64");

  // Initialize for predict(). Put predictor address in edi/rdi
  put1a(0xe9, 5);             // jmp predict
  put1a(0, 0x90909000);       // reserve space for jmp update
  put1(0x53);                 // push ebx/rbx
  put1(0x55);                 // push ebp/rbp
  put1(0x56);                 // push esi/rsi
  put1(0x57);                 // push edi/rdi
  if (S==4)
	put4(0x8b7c2414);         // mov edi,[esp+0x14] ; pr
  else {
#if !defined(unix) || defined(__CYGWIN__)
	put3(0x4889cf);           // mov rdi, rcx (1st arg in Win64)
#endif
  }

  // Code predict() for each component
  const int n=hcomp[6];  // number of components
  U8* cp=hcomp+7;
  for (int i=0; i<n; ++i, cp+=compsize[cp[0]]) {
	if (cp-hcomp>=pr.z.cend) error("comp too big");
	if (cp[0]<1 || cp[0]>9) error("invalid component");
	assert(compsize[cp[0]]>0 && compsize[cp[0]]<8);
	switch (cp[0]) {

	  case CONS:  // c
		break;

	  case CM:  // sizebits limit
		// Component& cr=comp[i];
		// cr.cxt=h[i]^hmap4;
		// p[i]=stretch(cr.cm(cr.cxt)>>17);

		put2a(0x8b87, off(h[i]));              // mov eax, [edi+&h[i]]
		put2a(0x3387, off(hmap4));             // xor eax, [edi+&hmap4]
		put1a(0x25, (1<<cp[1])-1);             // and eax, size-1
		put2a(0x8987, offc(cxt));              // mov [edi+cxt], eax
		if (S==8) put1(0x48);                  // rex.w (esi->rsi)
		put2a(0x8bb7, offc(cm));               // mov esi, [edi+&cm]
		put3(0x8b0486);                        // mov eax, [esi+eax*4]
		put3(0xc1e811);                        // shr eax, 17
		put4a(0x0fbf8447, off(stretcht));      // movsx eax,word[edi+eax*2+..]
		put2a(0x8987, off(p[i]));              // mov [edi+&p[i]], eax
		break;

	  case ISSE:  // sizebits j -- c=hi, cxt=bh
		// assert((hmap4&15)>0);
		// if (c8==1 || (c8&0xf0)==16)
		//   cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
		// cr.cxt=cr.ht[cr.c+(hmap4&15)];  // bit history
		// int *wt=(int*)&cr.cm[cr.cxt*2];
		// p[i]=clamp2k((wt[0]*p[cp[2]]+wt[1]*64)>>16);

	  case ICM: // sizebits
		// assert((hmap4&15)>0);
		// if (c8==1 || (c8&0xf0)==16) cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
		// cr.cxt=cr.ht[cr.c+(hmap4&15)];
		// p[i]=stretch(cr.cm(cr.cxt)>>8);
		//
		// Find cxt row in hash table ht. ht has rows of 16 indexed by the low
		// sizebits of cxt with element 0 having the next higher 8 bits for
		// collision detection. If not found after 3 adjacent tries, replace
		// row with lowest element 1 as priority. Return index of row.
		//
		// size_t Predictor::find(Array<U8>& ht, int sizebits, U32 cxt) {
		//  assert(ht.size()==size_t(16)<<sizebits);
		//  int chk=cxt>>sizebits&255;
		//  size_t h0=(cxt*16)&(ht.size()-16);
		//  if (ht[h0]==chk) return h0;
		//  size_t h1=h0^16;
		//  if (ht[h1]==chk) return h1;
		//  size_t h2=h0^32;
		//  if (ht[h2]==chk) return h2;
		//  if (ht[h0+1]<=ht[h1+1] && ht[h0+1]<=ht[h2+1])
		//    return memset(&ht[h0], 0, 16), ht[h0]=chk, h0;
		//  else if (ht[h1+1]<ht[h2+1])
		//    return memset(&ht[h1], 0, 16), ht[h1]=chk, h1;
		//  else
		//    return memset(&ht[h2], 0, 16), ht[h2]=chk, h2;
		// }

		if (S==8) put1(0x48);                  // rex.w
		put2a(0x8bb7, offc(ht));               // mov esi, [edi+&ht]
		put2(0x8b07);                          // mov eax, edi ; c8
		put2(0x89c1);                          // mov ecx, eax ; c8
		put3(0x83f801);                        // cmp eax, 1
		put2(0x740a);                          // je L1
		put1a(0x25, 240);                      // and eax, 0xf0
		put3(0x83f810);                        // cmp eax, 16
		put2(0x7576);                          // jne L2 ; skip find()
		   // L1: ; find cxt in ht, return index in eax
		put3(0xc1e104);                        // shl ecx, 4
		put2a(0x038f, off(h[i]));              // add [edi+&h[i]]
		put2(0x89c8);                          // mov eax, ecx ; cxt
		put3(0xc1e902+cp[1]);                  // shr ecx, sizebits+2
		put2a(0x81e1, 255);                    // and eax, 255 ; chk
		put3(0xc1e004);                        // shl eax, 4
		put1a(0x25, (64<<cp[1])-16);           // and eax, ht.size()-16 = h0
		put3(0x3a0c06);                        // cmp cl, [esi+eax] ; ht[h0]
		put2(0x744d);                          // je L3 ; match h0
		put3(0x83f010);                        // xor eax, 16 ; h1
		put3(0x3a0c06);                        // cmp cl, [esi+eax]
		put2(0x7445);                          // je L3 ; match h1
		put3(0x83f030);                        // xor eax, 48 ; h2
		put3(0x3a0c06);                        // cmp cl, [esi+eax]
		put2(0x743d);                          // je L3 ; match h2
		  // No checksum match, so replace the lowest priority among h0,h1,h2
		put3(0x83f021);                        // xor eax, 33 ; h0+1
		put3(0x8a1c06);                        // mov bl, [esi+eax] ; ht[h0+1]
		put2(0x89c2);                          // mov edx, eax ; h0+1
		put3(0x83f220);                        // xor edx, 32  ; h2+1
		put3(0x3a1c16);                        // cmp bl, [esi+edx]
		put2(0x7708);                          // ja L4 ; test h1 vs h2
		put3(0x83f230);                        // xor edx, 48  ; h1+1
		put3(0x3a1c16);                        // cmp bl, [esi+edx]
		put2(0x7611);                          // jbe L7 ; replace h0
		  // L4: ; h0 is not lowest, so replace h1 or h2
		put3(0x83f010);                        // xor eax, 16 ; h1+1
		put3(0x8a1c06);                        // mov bl, [esi+eax]
		put3(0x83f030);                        // xor eax, 48 ; h2+1
		put3(0x3a1c06);                        // cmp bl, [esi+eax]
		put2(0x7303);                          // jae L7
		put3(0x83f030);                        // xor eax, 48 ; h1+1
		  // L7: ; replace row pointed to by eax = h0,h1,h2
		put3(0x83f001);                        // xor eax, 1
		put3(0x890c06);                        // mov [esi+eax], ecx ; chk
		put2(0x31c9);                          // xor ecx, ecx
		put4(0x894c0604);                      // mov [esi+eax+4], ecx
		put4(0x894c0608);                      // mov [esi+eax+8], ecx
		put4(0x894c060c);                      // mov [esi+eax+12], ecx
		  // L3: ; save nibble context (in eax) in c
		put2a(0x8987, offc(c));                // mov [edi+c], eax
		put2(0xeb06);                          // jmp L8
		  // L2: ; get nibble context
		put2a(0x8b87, offc(c));                // mov eax, [edi+c]
		  // L8: ; nibble context is in eax
		put2a(0x8b97, off(hmap4));             // mov edx, [edi+&hmap4]
		put3(0x83e20f);                        // and edx, 15  ; hmap4
		put2(0x01d0);                          // add eax, edx ; c+(hmap4&15)
		put4(0x0fb61406);                      // movzx edx, byte [esi+eax]
		put2a(0x8997, offc(cxt));              // mov [edi+&cxt], edx ; cxt=bh
		if (S==8) put1(0x48);                  // rex.w
		put2a(0x8bb7, offc(cm));               // mov esi, [edi+&cm] ; cm

		// esi points to cm[256] (ICM) or cm[512] (ISSE) with 23 bit
		// prediction (ICM) or a pair of 20 bit signed weights (ISSE).
		// cxt = bit history bh (0..255) is in edx.
		if (cp[0]==ICM) {
		  put3(0x8b0496);                      // mov eax, [esi+edx*4];cm[bh]
		  put3(0xc1e808);                      // shr eax, 8
		  put4a(0x0fbf8447, off(stretcht));    // movsx eax,word[edi+eax*2+..]
		}
		else {  // ISSE
		  put2a(0x8b87, off(p[cp[2]]));        // mov eax, [edi+&p[j]]
		  put4(0x0faf04d6);                    // imul eax, [esi+edx*8] ;wt[0]
		  put4(0x8b4cd604);                    // mov ecx, [esi+edx*8+4];wt[1]
		  put3(0xc1e106);                      // shl ecx, 6
		  put2(0x01c8);                        // add eax, ecx
		  put3(0xc1f810);                      // sar eax, 16
		  put1a(0xb9, 2047);                   // mov ecx, 2047
		  put2(0x39c8);                        // cmp eax, ecx
		  put3(0x0f4fc1);                      // cmovg eax, ecx
		  put1a(0xb9, -2048);                  // mov ecx, -2048
		  put2(0x39c8);                        // cmp eax, ecx
		  put3(0x0f4cc1);                      // cmovl eax, ecx

		}
		put2a(0x8987, off(p[i]));              // mov [edi+&p[i]], eax
		break;

	  case MATCH: // sizebits bufbits: a=len, b=offset, c=bit, cxt=bitpos,
				  //                   ht=buf, limit=pos
		// assert(cr.cm.size()==(size_t(1)<<cp[1]));
		// assert(cr.ht.size()==(size_t(1)<<cp[2]));
		// assert(cr.a<=255);
		// assert(cr.c==0 || cr.c==1);
		// assert(cr.cxt<8);
		// assert(cr.limit<cr.ht.size());
		// if (cr.a==0) p[i]=0;
		// else {
		//   cr.c=(cr.ht(cr.limit-cr.b)>>(7-cr.cxt))&1; // predicted bit
		//   p[i]=stretch(dt2k[cr.a]*(cr.c*-2+1)&32767);
		// }

		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(ht));       // mov esi, [edi+&ht]

		// If match length (a) is 0 then p[i]=0
		put2a(0x8b87, offc(a));        // mov eax, [edi+&a]
		put2(0x85c0);                  // test eax, eax
		put2(0x7449);                  // jz L2 ; p[i]=0

		// Else put predicted bit in c
		put1a(0xb9, 7);                // mov ecx, 7
		put2a(0x2b8f, offc(cxt));      // sub ecx, [edi+&cxt]
		put2a(0x8b87, offc(limit));    // mov eax, [edi+&limit]
		put2a(0x2b87, offc(b));        // sub eax, [edi+&b]
		put1a(0x25, (1<<cp[2])-1);     // and eax, ht.size()-1
		put4(0x0fb60406);              // movzx eax, byte [esi+eax]
		put2(0xd3e8);                  // shr eax, cl
		put3(0x83e001);                // and eax, 1  ; predicted bit
		put2a(0x8987, offc(c));        // mov [edi+&c], eax ; c

		// p[i]=stretch(dt2k[cr.a]*(cr.c*-2+1)&32767);
		put2a(0x8b87, offc(a));        // mov eax, [edi+&a]
		put3a(0x8b8487, off(dt2k));    // mov eax, [edi+eax*4+&dt2k] ; weight
		put2(0x7402);                  // jz L1 ; z if c==0
		put2(0xf7d8);                  // neg eax
		put1a(0x25, 0x7fff);           // L1: and eax, 32767
		put4a(0x0fbf8447, off(stretcht)); //movsx eax, word [edi+eax*2+...]
		put2a(0x8987, off(p[i]));      // L2: mov [edi+&p[i]], eax
		break;

	  case AVG: // j k wt
		// p[i]=(p[cp[1]]*cp[3]+p[cp[2]]*(256-cp[3]))>>8;

		put2a(0x8b87, off(p[cp[1]]));  // mov eax, [edi+&p[j]]
		put2a(0x2b87, off(p[cp[2]]));  // sub eax, [edi+&p[k]]
		put2a(0x69c0, cp[3]);          // imul eax, wt
		put3(0xc1f808);                // sar eax, 8
		put2a(0x0387, off(p[cp[2]]));  // add eax, [edi+&p[k]]
		put2a(0x8987, off(p[i]));      // mov [edi+&p[i]], eax
		break;

	  case MIX2:   // sizebits j k rate mask
				   // c=size cm=wt[size] cxt=input
		// cr.cxt=((h[i]+(c8&cp[5]))&(cr.c-1));
		// assert(cr.cxt<cr.a16.size());
		// int w=cr.a16[cr.cxt];
		// assert(w>=0 && w<65536);
		// p[i]=(w*p[cp[2]]+(65536-w)*p[cp[3]])>>16;
		// assert(p[i]>=-2048 && p[i]<2048);

		put2(0x8b07);                  // mov eax, [edi] ; c8
		put1a(0x25, cp[5]);            // and eax, mask
		put2a(0x0387, off(h[i]));      // add eax, [edi+&h[i]]
		put1a(0x25, (1<<cp[1])-1);     // and eax, size-1
		put2a(0x8987, offc(cxt));      // mov [edi+&cxt], eax ; cxt
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(a16));      // mov esi, [edi+&a16]
		put4(0x0fb70446);              // movzx eax, word [edi+eax*2] ; w
		put2a(0x8b8f, off(p[cp[2]]));  // mov ecx, [edi+&p[j]]
		put2a(0x8b97, off(p[cp[3]]));  // mov edx, [edi+&p[k]]
		put2(0x29d1);                  // sub ecx, edx
		put3(0x0fafc8);                // imul ecx, eax
		put3(0xc1e210);                // shl edx, 16
		put2(0x01d1);                  // add ecx, edx
		put3(0xc1f910);                // sar ecx, 16
		put2a(0x898f, off(p[i]));      // mov [edi+&p[i]]
		break;

	  case MIX:    // sizebits j m rate mask
				   // c=size cm=wt[size][m] cxt=index of wt in cm
		// int m=cp[3];
		// assert(m>=1 && m<=i);
		// cr.cxt=h[i]+(c8&cp[5]);
		// cr.cxt=(cr.cxt&(cr.c-1))*m; // pointer to row of weights
		// assert(cr.cxt<=cr.cm.size()-m);
		// int* wt=(int*)&cr.cm[cr.cxt];
		// p[i]=0;
		// for (int j=0; j<m; ++j)
		//   p[i]+=(wt[j]>>8)*p[cp[2]+j];
		// p[i]=clamp2k(p[i]>>8);

		put2(0x8b07);                          // mov eax, [edi] ; c8
		put1a(0x25, cp[5]);                    // and eax, mask
		put2a(0x0387, off(h[i]));              // add eax, [edi+&h[i]]
		put1a(0x25, (1<<cp[1])-1);             // and eax, size-1
		put2a(0x69c0, cp[3]);                  // imul eax, m
		put2a(0x8987, offc(cxt));              // mov [edi+&cxt], eax ; cxt
		if (S==8) put1(0x48);                  // rex.w
		put2a(0x8bb7, offc(cm));               // mov esi, [edi+&cm]
		if (S==8) put1(0x48);                  // rex.w
		put3(0x8d3486);                        // lea esi, [esi+eax*4] ; wt

		// Unroll summation loop: esi=wt[0..m-1]
		for (int k=0; k<cp[3]; k+=8) {
		  const int tail=cp[3]-k;  // number of elements remaining

		  // pack 8 elements of wt in xmm1, 8 elements of p in xmm3
		  put4a(0xf30f6f8e, k*4);              // movdqu xmm1, [esi+k*4]
		  if (tail>3) put4a(0xf30f6f96, k*4+16);//movdqu xmm2, [esi+k*4+16]
		  put5(0x660f72e1,0x08);               // psrad xmm1, 8
		  if (tail>3) put5(0x660f72e2,0x08);   // psrad xmm2, 8
		  put4(0x660f6bca);                    // packssdw xmm1, xmm2
		  put4a(0xf30f6f9f, off(p[cp[2]+k]));  // movdqu xmm3, [edi+&p[j+k]]
		  if (tail>3)
			put4a(0xf30f6fa7,off(p[cp[2]+k+4]));//movdqu xmm4, [edi+&p[j+k+4]]
		  put4(0x660f6bdc);                    // packssdw, xmm3, xmm4
		  if (tail>0 && tail<8) {  // last loop, mask extra weights
			put4(0x660f76ed);                  // pcmpeqd xmm5, xmm5 ; -1
			put5(0x660f73dd, 16-tail*2);       // psrldq xmm5, 16-tail*2
			put4(0x660fdbcd);                  // pand xmm1, xmm5
		  }
		  if (k==0) {  // first loop, initialize sum in xmm0
			put4(0xf30f6fc1);                  // movdqu xmm0, xmm1
			put4(0x660ff5c3);                  // pmaddwd xmm0, xmm3
		  }
		  else {  // accumulate sum in xmm0
			put4(0x660ff5cb);                  // pmaddwd xmm1, xmm3
			put4(0x660ffec1);                  // paddd xmm0, xmm1
		  }
		}

		// Add up the 4 elements of xmm0 = p[i] in the first element
		put4(0xf30f6fc8);                      // movdqu xmm1, xmm0
		put5(0x660f73d9,0x08);                 // psrldq xmm1, 8
		put4(0x660ffec1);                      // paddd xmm0, xmm1
		put4(0xf30f6fc8);                      // movdqu xmm1, xmm0
		put5(0x660f73d9,0x04);                 // psrldq xmm1, 4
		put4(0x660ffec1);                      // paddd xmm0, xmm1
		put4(0x660f7ec0);                      // movd eax, xmm0 ; p[i]
		put3(0xc1f808);                        // sar eax, 8
		put1a(0x3d, 2047);                     // cmp eax, 2047
		put2(0x7e05);                          // jle L1
		put1a(0xb8, 2047);                     // mov eax, 2047
		put1a(0x3d, -2048);                    // L1: cmp eax, -2048
		put2(0x7d05);                          // jge, L2
		put1a(0xb8, -2048);                    // mov eax, -2048
		put2a(0x8987, off(p[i]));              // L2: mov [edi+&p[i]], eax
		break;

	  case SSE:  // sizebits j start limit
		// cr.cxt=(h[i]+c8)*32;
		// int pq=p[cp[2]]+992;
		// if (pq<0) pq=0;
		// if (pq>1983) pq=1983;
		// int wt=pq&63;
		// pq>>=6;
		// assert(pq>=0 && pq<=30);
		// cr.cxt+=pq;
		// p[i]=stretch(((cr.cm(cr.cxt)>>10)*(64-wt)       // p0
		//               +(cr.cm(cr.cxt+1)>>10)*wt)>>13);  // p1
		// // p = p0*(64-wt)+p1*wt = (p1-p0)*wt + p0*64
		// cr.cxt+=wt>>5;

		put2a(0x8b8f, off(h[i]));      // mov ecx, [edi+&h[i]]
		put2(0x030f);                  // add ecx, [edi]  ; c0
		put2a(0x81e1, (1<<cp[1])-1);   // and ecx, size-1
		put3(0xc1e105);                // shl ecx, 5  ; cxt in 0..size*32-32
		put2a(0x8b87, off(p[cp[2]]));  // mov eax, [edi+&p[j]] ; pq
		put1a(0x05, 992);              // add eax, 992
		put2(0x31d2);                  // xor edx, edx ; 0
		put2(0x39d0);                  // cmp eax, edx
		put3(0x0f4cc2);                // cmovl eax, edx
		put1a(0xba, 1983);             // mov edx, 1983
		put2(0x39d0);                  // cmp eax, edx
		put3(0x0f4fc2);                // cmovg eax, edx ; pq in 0..1983
		put2(0x89c2);                  // mov edx, eax
		put3(0x83e23f);                // and edx, 63  ; wt in 0..63
		put3(0xc1e806);                // shr eax, 6   ; pq in 0..30
		put2(0x01c1);                  // add ecx, eax ; cxt in 0..size*32-2
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(cm));       // mov esi, [edi+cm]
		put3(0x8b048e);                // mov eax, [esi+ecx*4] ; cm[cxt]
		put4(0x8b5c8e04);              // mov ebx, [esi+ecx*4+4] ; cm[cxt+1]
		put3(0x83fa20);                // cmp edx, 32  ; wt
		put3(0x83d9ff);                // sbb ecx, -1  ; cxt+=wt>>5
		put2a(0x898f, offc(cxt));      // mov [edi+cxt], ecx  ; cxt saved
		put3(0xc1e80a);                // shr eax, 10 ; p0 = cm[cxt]>>10
		put3(0xc1eb0a);                // shr ebx, 10 ; p1 = cm[cxt+1]>>10
		put2(0x29c3);                  // sub ebx, eax, ; p1-p0
		put3(0x0fafda);                // imul ebx, edx ; (p1-p0)*wt
		put3(0xc1e006);                // shr eax, 6
		put2(0x01d8);                  // add eax, ebx ; p in 0..2^28-1
		put3(0xc1e80d);                // shr eax, 13  ; p in 0..32767
		put4a(0x0fbf8447, off(stretcht));  // movsx eax, word [edi+eax*2+...]
		put2a(0x8987, off(p[i]));      // mov [edi+&p[i]], eax
		break;

	  default:
		error("invalid ZPAQ component");
	}
  }

  // return squash(p[n-1])
  put2a(0x8b87, off(p[n-1]));          // mov eax, [edi+...]
  put1a(0x05, 0x800);                  // add eax, 2048
  put4a(0x0fbf8447, off(squasht[0]));  // movsx eax, word [edi+eax*2+...]
  put1(0x5f);                          // pop edi
  put1(0x5e);                          // pop esi
  put1(0x5d);                          // pop ebp
  put1(0x5b);                          // pop ebx
  put1(0xc3);                          // ret

  // Initialize for update() Put predictor address in edi/rdi
  // and bit y=0..1 in ebp
  int save_o=o;
  o=5;
  put1a(0xe9, save_o-10);      // jmp update
  o=save_o;
  put1(0x53);                  // push ebx/rbx
  put1(0x55);                  // push ebp/rbp
  put1(0x56);                  // push esi/rsi
  put1(0x57);                  // push edi/rdi
  if (S==4) {
	put4(0x8b7c2414);          // mov edi,[esp+0x14] ; (1st arg = pr)
	put4(0x8b6c2418);          // mov ebp,[esp+0x18] ; (2nd arg = y)
  }
  else {
#if defined(unix) && !defined(__CYGWIN__)  // (1st arg already in rdi)
	put3(0x4889f5);            // mov rbp, rsi (2nd arg in Linux-64)
#else
	put3(0x4889cf);            // mov rdi, rcx (1st arg in Win64)
	put3(0x4889d5);            // mov rbp, rdx (2nd arg)
#endif
  }

  // Code update() for each component
  cp=hcomp+7;
  for (int i=0; i<n; ++i, cp+=compsize[cp[0]]) {
	assert(cp-hcomp<pr.z.cend);
	assert (cp[0]>=1 && cp[0]<=9);
	assert(compsize[cp[0]]>0 && compsize[cp[0]]<8);
	switch (cp[0]) {

	  case CONS:  // c
		break;

	  case SSE:  // sizebits j start limit
	  case CM:   // sizebits limit
		// train(cr, y);
		//
		// reduce prediction error in cr.cm
		// void train(Component& cr, int y) {
		//   assert(y==0 || y==1);
		//   U32& pn=cr.cm(cr.cxt);
		//   U32 count=pn&0x3ff;
		//   int error=y*32767-(cr.cm(cr.cxt)>>17);
		//   pn+=(error*dt[count]&-1024)+(count<cr.limit);

		if (S==8) put1(0x48);          // rex.w (esi->rsi)
		put2a(0x8bb7, offc(cm));       // mov esi,[edi+cm]  ; cm
		put2a(0x8b87, offc(cxt));      // mov eax,[edi+cxt] ; cxt
		put1a(0x25, pr.comp[i].cm.size()-1);  // and eax, size-1
		if (S==8) put1(0x48);          // rex.w
		put3(0x8d3486);                // lea esi,[esi+eax*4] ; &cm[cxt]
		put2(0x8b06);                  // mov eax,[esi] ; cm[cxt]
		put2(0x89c2);                  // mov edx, eax  ; cm[cxt]
		put3(0xc1e811);                // shr eax, 17   ; cm[cxt]>>17
		put2(0x89e9);                  // mov ecx, ebp  ; y
		put3(0xc1e10f);                // shl ecx, 15   ; y*32768
		put2(0x29e9);                  // sub ecx, ebp  ; y*32767
		put2(0x29c1);                  // sub ecx, eax  ; error
		put2a(0x81e2, 0x3ff);          // and edx, 1023 ; count
		put3a(0x8b8497, off(dt));      // mov eax,[edi+edx*4+dt] ; dt[count]
		put3(0x0fafc8);                // imul ecx, eax ; error*dt[count]
		put2a(0x81e1, 0xfffffc00);     // and ecx, -1024
		put2a(0x81fa, cp[2+2*(cp[0]==SSE)]*4); // cmp edx, limit*4
		put2(0x110e);                  // adc [esi], ecx ; pn+=...
		break;

	  case ICM:   // sizebits: cxt=bh, ht[c][0..15]=bh row
		// cr.ht[cr.c+(hmap4&15)]=st.next(cr.ht[cr.c+(hmap4&15)], y);
		// U32& pn=cr.cm(cr.cxt);
		// pn+=int(y*32767-(pn>>8))>>2;

	  case ISSE:  // sizebits j  -- c=hi, cxt=bh
		// assert(cr.cxt==cr.ht[cr.c+(hmap4&15)]);
		// int err=y*32767-squash(p[i]);
		// int *wt=(int*)&cr.cm[cr.cxt*2];
		// wt[0]=clamp512k(wt[0]+((err*p[cp[2]]+(1<<12))>>13));
		// wt[1]=clamp512k(wt[1]+((err+16)>>5));
		// cr.ht[cr.c+(hmap4&15)]=st.next(cr.cxt, y);

		// update bit history bh to next(bh,y=ebp) in ht[c+(hmap4&15)]
		put3(0x8b4700+off(hmap4));     // mov eax, [edi+&hmap4]
		put3(0x83e00f);                // and eax, 15
		put2a(0x0387, offc(c));        // add eax [edi+&c] ; cxt
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(ht));       // mov esi, [edi+&ht]
		put4(0x0fb61406);              // movzx edx, byte [esi+eax] ; bh
		put4(0x8d5c9500);              // lea ebx, [ebp+edx*4] ; index to st
		put4a(0x0fb69c1f, off(st));    // movzx ebx,byte[edi+ebx+st]; next bh
		put3(0x881c06);                // mov [esi+eax], bl ; save next bh
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(cm));       // mov esi, [edi+&cm]

		// ICM: update cm[cxt=edx=bit history] to reduce prediction error
		// esi = &cm
		if (cp[0]==ICM) {
		  if (S==8) put1(0x48);        // rex.w
		  put3(0x8d3496);              // lea esi, [esi+edx*4] ; &cm[bh]
		  put2(0x8b06);                // mov eax, [esi] ; pn
		  put3(0xc1e808);              // shr eax, 8 ; pn>>8
		  put2(0x89e9);                // mov ecx, ebp ; y
		  put3(0xc1e10f);              // shl ecx, 15
		  put2(0x29e9);                // sub ecx, ebp ; y*32767
		  put2(0x29c1);                // sub ecx, eax
		  put3(0xc1f902);              // sar ecx, 2
		  put2(0x010e);                // add [esi], ecx
		}

		// ISSE: update weights. edx=cxt=bit history (0..255), esi=cm[512]
		else {
		  put2a(0x8b87, off(p[i]));    // mov eax, [edi+&p[i]]
		  put1a(0x05, 2048);           // add eax, 2048
		  put4a(0x0fb78447, off(squasht)); // movzx eax, word [edi+eax*2+..]
		  put2(0x89e9);                // mov ecx, ebp ; y
		  put3(0xc1e10f);              // shl ecx, 15
		  put2(0x29e9);                // sub ecx, ebp ; y*32767
		  put2(0x29c1);                // sub ecx, eax ; err
		  put2a(0x8b87, off(p[cp[2]]));// mov eax, [edi+&p[j]]
		  put3(0x0fafc1);              // imul eax, ecx
		  put1a(0x05, (1<<12));        // add eax, 4096
		  put3(0xc1f80d);              // sar eax, 13
		  put3(0x0304d6);              // add eax, [esi+edx*8] ; wt[0]
		  put1a(0x3d, (1<<19)-1);      // cmp eax, (1<<19)-1
		  put2(0x7e05);                // jle L1
		  put1a(0xb8, (1<<19)-1);      // mov eax, (1<<19)-1
		  put1a(0x3d, -1<<19);         // cmp eax, -1<<19
		  put2(0x7d05);                // jge L2
		  put1a(0xb8, -1<<19);         // mov eax, -1<<19
		  put3(0x8904d6);              // L2: mov [esi+edx*8], eax
		  put3(0x83c110);              // add ecx, 16 ; err
		  put3(0xc1f905);              // sar ecx, 5
		  put4(0x034cd604);            // add ecx, [esi+edx*8+4] ; wt[1]
		  put2a(0x81f9, (1<<19)-1);    // cmp ecx, (1<<19)-1
		  put2(0x7e05);                // jle L3
		  put1a(0xb9, (1<<19)-1);      // mov ecx, (1<<19)-1
		  put2a(0x81f9, -1<<19);       // cmp ecx, -1<<19
		  put2(0x7d05);                // jge L4
		  put1a(0xb9, -1<<19);         // mov ecx, -1<<19
		  put4(0x894cd604);            // L4: mov [esi+edx*8+4], ecx
		}
		break;

	  case MATCH: // sizebits bufbits:
				  //   a=len, b=offset, c=bit, cm=index, cxt=bitpos
				  //   ht=buf, limit=pos
		// assert(cr.a<=255);
		// assert(cr.c==0 || cr.c==1);
		// assert(cr.cxt<8);
		// assert(cr.cm.size()==(size_t(1)<<cp[1]));
		// assert(cr.ht.size()==(size_t(1)<<cp[2]));
		// if (int(cr.c)!=y) cr.a=0;  // mismatch?
		// cr.ht(cr.limit)+=cr.ht(cr.limit)+y;
		// if (++cr.cxt==8) {
		//   cr.cxt=0;
		//   ++cr.limit;
		//   cr.limit&=(1<<cp[2])-1;
		//   if (cr.a==0) {  // look for a match
		//     cr.b=cr.limit-cr.cm(h[i]);
		//     if (cr.b&(cr.ht.size()-1))
		//       while (cr.a<255
		//              && cr.ht(cr.limit-cr.a-1)==cr.ht(cr.limit-cr.a-cr.b-1))
		//         ++cr.a;
		//   }
		//   else cr.a+=cr.a<255;
		//   cr.cm(h[i])=cr.limit;
		// }

		// Set pointers ebx=&cm, esi=&ht
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(ht));       // mov esi, [edi+&ht]
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8b9f, offc(cm));       // mov ebx, [edi+&cm]

		// if (c!=y) a=0;
		put2a(0x8b87, offc(c));        // mov eax, [edi+&c]
		put2(0x39e8);                  // cmp eax, ebp ; y
		put2(0x7408);                  // jz L1
		put2(0x31c0);                  // xor eax, eax
		put2a(0x8987, offc(a));        // mov [edi+&a], eax

		// ht(limit)+=ht(limit)+y  (1E)
		put2a(0x8b87, offc(limit));    // mov eax, [edi+&limit]
		put4(0x0fb60c06);              // movzx, ecx, byte [esi+eax]
		put2(0x01c9);                  // add ecx, ecx
		put2(0x01e9);                  // add ecx, ebp
		put3(0x880c06);                // mov [esi+eax], cl

		// if (++cxt==8)
		put2a(0x8b87, offc(cxt));      // mov eax, [edi+&cxt]
		put2(0xffc0);                  // inc eax
		put3(0x83e007);                // and eax,byte +0x7
		put2a(0x8987, offc(cxt));      // mov [edi+&cxt],eax
		put2a(0x0f85, 0x9b);           // jnz L8

		// ++limit;
		// limit&=bufsize-1;
		put2a(0x8b87, offc(limit));    // mov eax,[edi+&limit]
		put2(0xffc0);                  // inc eax
		put1a(0x25, (1<<cp[2])-1);     // and eax, bufsize-1
		put2a(0x8987, offc(limit));    // mov [edi+&limit],eax

		// if (a==0)
		put2a(0x8b87, offc(a));        // mov eax, [edi+&a]
		put2(0x85c0);                  // test eax,eax
		put2(0x755c);                  // jnz L6

		//   b=limit-cm(h[i])
		put2a(0x8b8f, off(h[i]));      // mov ecx,[edi+h[i]]
		put2a(0x81e1, (1<<cp[1])-1);   // and ecx, size-1
		put2a(0x8b87, offc(limit));    // mov eax,[edi-&limit]
		put3(0x2b048b);                // sub eax,[ebx+ecx*4]
		put2a(0x8987, offc(b));        // mov [edi+&b],eax

		//   if (b&(bufsize-1))
		put1a(0xa9, (1<<cp[2])-1);     // test eax, bufsize-1
		put2(0x7448);                  // jz L7

		//      while (a<255 && ht(limit-a-1)==ht(limit-a-b-1)) ++a;
		put1(0x53);                    // push ebx
		put2a(0x8b9f, offc(limit));    // mov ebx,[edi+&limit]
		put2(0x89da);                  // mov edx,ebx
		put2(0x29c3);                  // sub ebx,eax  ; limit-b
		put2(0x31c9);                  // xor ecx,ecx  ; a=0
		put2a(0x81f9, 0xff);           // L2: cmp ecx,0xff ; while
		put2(0x741c);                  // jz L3 ; break
		put2(0xffca);                  // dec edx
		put2(0xffcb);                  // dec ebx
		put2a(0x81e2, (1<<cp[2])-1);   // and edx, bufsize-1
		put2a(0x81e3, (1<<cp[2])-1);   // and ebx, bufsize-1
		put3(0x8a0416);                // mov al,[esi+edx]
		put3(0x3a041e);                // cmp al,[esi+ebx]
		put2(0x7504);                  // jnz L3 ; break
		put2(0xffc1);                  // inc ecx
		put2(0xebdc);                  // jmp short L2 ; end while
		put1(0x5b);                    // L3: pop ebx
		put2a(0x898f, offc(a));        // mov [edi+&a],ecx
		put2(0xeb0e);                  // jmp short L7

		// a+=(a<255)
		put1a(0x3d, 0xff);             // L6: cmp eax, 0xff ; a
		put3(0x83d000);                // adc eax, 0
		put2a(0x8987, offc(a));        // mov [edi+&a],eax

		// cm(h[i])=limit
		put2a(0x8b87, off(h[i]));      // L7: mov eax,[edi+&h[i]]
		put1a(0x25, (1<<cp[1])-1);     // and eax, size-1
		put2a(0x8b8f, offc(limit));    // mov ecx,[edi+&limit]
		put3(0x890c83);                // mov [ebx+eax*4],ecx
									   // L8:
		break;

	  case AVG:  // j k wt
		break;

	  case MIX2: // sizebits j k rate mask
				 // cm=wt[size], cxt=input
		// assert(cr.a16.size()==cr.c);
		// assert(cr.cxt<cr.a16.size());
		// int err=(y*32767-squash(p[i]))*cp[4]>>5;
		// int w=cr.a16[cr.cxt];
		// w+=(err*(p[cp[2]]-p[cp[3]])+(1<<12))>>13;
		// if (w<0) w=0;
		// if (w>65535) w=65535;
		// cr.a16[cr.cxt]=w;

		// set ecx=err
		put2a(0x8b87, off(p[i]));      // mov eax, [edi+&p[i]]
		put1a(0x05, 2048);             // add eax, 2048
		put4a(0x0fb78447, off(squasht));//movzx eax, word [edi+eax*2+&squasht]
		put2(0x89e9);                  // mov ecx, ebp ; y
		put3(0xc1e10f);                // shl ecx, 15
		put2(0x29e9);                  // sub ecx, ebp ; y*32767
		put2(0x29c1);                  // sub ecx, eax
		put2a(0x69c9, cp[4]);          // imul ecx, rate
		put3(0xc1f905);                // sar ecx, 5  ; err

		// Update w
		put2a(0x8b87, offc(cxt));      // mov eax, [edi+&cxt]
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(a16));      // mov esi, [edi+&a16]
		if (S==8) put1(0x48);          // rex.w
		put3(0x8d3446);                // lea esi, [esi+eax*2] ; &w
		put2a(0x8b87, off(p[cp[2]]));  // mov eax, [edi+&p[j]]
		put2a(0x2b87, off(p[cp[3]]));  // sub eax, [edi+&p[k]] ; p[j]-p[k]
		put3(0x0fafc1);                // imul eax, ecx  ; * err
		put1a(0x05, 1<<12);            // add eax, 4096
		put3(0xc1f80d);                // sar eax, 13
		put3(0x0fb716);                // movzx edx, word [esi] ; w
		put2(0x01d0);                  // add eax, edx
		put1a(0xba, 0xffff);           // mov edx, 65535
		put2(0x39d0);                  // cmp eax, edx
		put3(0x0f4fc2);                // cmovg eax, edx
		put2(0x31d2);                  // xor edx, edx
		put2(0x39d0);                  // cmp eax, edx
		put3(0x0f4cc2);                // cmovl eax, edx
		put3(0x668906);                // mov word [esi], ax
		break;

	  case MIX: // sizebits j m rate mask
				// cm=wt[size][m], cxt=input
		// int m=cp[3];
		// assert(m>0 && m<=i);
		// assert(cr.cm.size()==m*cr.c);
		// assert(cr.cxt+m<=cr.cm.size());
		// int err=(y*32767-squash(p[i]))*cp[4]>>4;
		// int* wt=(int*)&cr.cm[cr.cxt];
		// for (int j=0; j<m; ++j)
		//   wt[j]=clamp512k(wt[j]+((err*p[cp[2]+j]+(1<<12))>>13));

		// set ecx=err
		put2a(0x8b87, off(p[i]));      // mov eax, [edi+&p[i]]
		put1a(0x05, 2048);             // add eax, 2048
		put4a(0x0fb78447, off(squasht));//movzx eax, word [edi+eax*2+&squasht]
		put2(0x89e9);                  // mov ecx, ebp ; y
		put3(0xc1e10f);                // shl ecx, 15
		put2(0x29e9);                  // sub ecx, ebp ; y*32767
		put2(0x29c1);                  // sub ecx, eax
		put2a(0x69c9, cp[4]);          // imul ecx, rate
		put3(0xc1f904);                // sar ecx, 4  ; err

		// set esi=wt
		put2a(0x8b87, offc(cxt));      // mov eax, [edi+&cxt] ; cxt
		if (S==8) put1(0x48);          // rex.w
		put2a(0x8bb7, offc(cm));       // mov esi, [edi+&cm]
		if (S==8) put1(0x48);          // rex.w
		put3(0x8d3486);                // lea esi, [esi+eax*4] ; wt

		for (int k=0; k<cp[3]; ++k) {
		  put2a(0x8b87,off(p[cp[2]+k]));//mov eax, [edi+&p[cp[2]+k]
		  put3(0x0fafc1);              // imul eax, ecx
		  put1a(0x05, 1<<12);          // add eax, 1<<12
		  put3(0xc1f80d);              // sar eax, 13
		  put2(0x0306);                // add eax, [esi]
		  put1a(0x3d, (1<<19)-1);      // cmp eax, (1<<19)-1
		  put2(0x7e05);                // jge L1
		  put1a(0xb8, (1<<19)-1);      // mov eax, (1<<19)-1
		  put1a(0x3d, -1<<19);         // cmp eax, -1<<19
		  put2(0x7d05);                // jle L2
		  put1a(0xb8, -1<<19);         // mov eax, -1<<19
		  put2(0x8906);                // L2: mov [esi], eax
		  if (k<cp[3]-1) {
			if (S==8) put1(0x48);      // rex.w
			put3(0x83c604);            // add esi, 4
		  }
		}
		break;

	  default:
		error("invalid ZPAQ component");
	}
  }

  // return from update()
  put1(0x5f);                 // pop edi
  put1(0x5e);                 // pop esi
  put1(0x5d);                 // pop ebp
  put1(0x5b);                 // pop ebx
  put1(0xc3);                 // ret

  return o;
}

#endif // ifndef NOJIT

// Return a prediction of the next bit in range 0..32767
// Use JIT code starting at pcode[0] if available, or else create it.
int Predictor::predict() {
#ifdef NOJIT
  return predict0();
#else
  if (!pcode) {
	allocx(pcode, pcode_size, (z.cend*100+4096)&-4096);
	int n=assemble_p();
	if (n>pcode_size) {
	  allocx(pcode, pcode_size, n);
	  n=assemble_p();
	}
	if (!pcode || n<15 || pcode_size<15)
	  error("run JIT failed");
  }
  assert(pcode && pcode[0]);
  return ((int(*)(Predictor*))&pcode[10])(this);
#endif
}

// Update the model with bit y = 0..1
// Use the JIT code starting at pcode[5].
void Predictor::update(int y) {
#ifdef NOJIT
  update0(y);
#else
  assert(pcode && pcode[5]);
  ((void(*)(Predictor*, int))&pcode[5])(this, y);

  // Save bit y in c8, hmap4 (not implemented in JIT)
  c8+=c8+y;
  if (c8>=256) {
	z.run(c8-256);
	hmap4=1;
	c8=1;
	for (int i=0; i<z.header[6]; ++i) h[i]=z.H(i);
  }
  else if (c8>=16 && c8<32)
	hmap4=(hmap4&0xf)<<5|y<<4|1;
  else
	hmap4=(hmap4&0x1f0)|(((hmap4&0xf)*2+y)&0xf);
#endif
}

// Execute the ZPAQL code with input byte or -1 for EOF.
// Use JIT code at rcode if available, or else create it.
void ZPAQL::run(U32 input) {
#ifdef NOJIT
  run0(input);
#else
  if (!rcode) {
	allocx(rcode, rcode_size, (hend*10+4096)&-4096);
	int n=assemble();
	if (n>rcode_size) {
	  allocx(rcode, rcode_size, n);
	  n=assemble();
	}
	if (!rcode || n<10 || rcode_size<10)
	  error("run JIT failed");
  }
  a=input;
  if (!((int(*)())(&rcode[0]))())
	libzpaq::error("Bad ZPAQL opcode");
#endif
}

}  // end namespace libzpaq


#endif

#if 0
// lzfx
#include "deps/lzfx/lzfx.h"
#include "deps/lzfx/lzfx.c"
#endif

#if 0
// lzp1
#include "deps/lzp1/lzp1.cpp"
#endif

#if 0
// fse
#include "deps/fse/fse.h"
#include "deps/fse/fse.c"
#endif

#if 0
// blosc
#include "deps/c-blosc/blosc/blosc.h"
#include "deps/c-blosc/blosc/blosclz.h"
#include "deps/c-blosc/blosc/shuffle.h"
#include "deps/c-blosc/blosc/blosc.c"
#include "deps/c-blosc/blosc/blosclz.c"
#include "deps/c-blosc/blosc/shuffle.c"
struct init_blosc {
	init_blosc() {
		blosc_init();
		blosc_set_nthreads(1);
	}
	~init_blosc() {
		blosc_destroy();
	}
} _;
#endif

#if 0
// bsc
#pragma comment(lib, "Advapi32.lib")
#include "deps/libbsc/libbsc/libbsc.h"
#include "deps/libbsc/libbsc/libbsc/libbsc.cpp"
#include "deps/libbsc/libbsc/lzp/lzp.cpp"
#include "deps/libbsc/libbsc/bwt/bwt.cpp"
#include "deps/libbsc/libbsc/bwt/divsufsort/divsufsort.c"
#include "deps/libbsc/libbsc/adler32/adler32.cpp"
#include "deps/libbsc/libbsc/platform/platform.cpp"
#include "deps/libbsc/libbsc/st/st.cpp"
#include "deps/libbsc/libbsc/coder/qlfc/qlfc.cpp"
#include "deps/libbsc/libbsc/coder/qlfc/qlfc_model.cpp"
#include "deps/libbsc/libbsc/coder/coder.cpp"
//#include "deps/libbsc/libbsc/filters/detectors.cpp"
//#include "deps/libbsc/libbsc/filters/preprocessing.cpp"
struct init_bsc {
	init_bsc() {
		bsc_init(0);
	}
} __;
#endif

#if 0
// bzip2
#define BZ_NO_STDIO
extern "C" void bz_internal_error(int errcode) {
	exit(-1);
}
#undef GET_BIT
#undef True
#undef False
#define Bool Bool2
#define UInt64 UInt642
#include "deps/bzip2/bzlib.h"
#include "deps/bzip2/crctable.c"
#include "deps/bzip2/compress_.c"
#include "deps/bzip2/decompress_.c"
#include "deps/bzip2/blocksort.c"
#undef SET_BINARY_MODE
#include "deps/bzip2/bzlib.c"
#include "deps/bzip2/randtable.c"
#include "deps/bzip2/huffman.c"
#endif

//#include "deps/yappy/yappy.h"
//#include "deps/yappy/yappy.cpp"

// bundle
#ifdef swap
#undef swap
#endif

#line 3 "bundle.cpp"
/*
 * Simple compression interface.
 * Copyright (c) 2013, 2014, Mario 'rlyeh' Rodriguez

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 * - rlyeh ~~ listening to Boris / Missing Pieces
 */

#include <cassert>
#include <cctype>  // std::isprint
#include <cstdio>  // std::sprintf
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "bundle.hpp"

// easylzma interface
namespace {
	/* callbacks for streamed input and output */

	struct wrbuf {
		size_t pos;
		const size_t size;
		uint8_t *const data;

		wrbuf( uint8_t *const data_, size_t size_ ) : pos(0), size(size_), data(data_)
		{}

		size_t writebuf( const void *buf, size_t len ) {
			if( pos + len > size ) {
				len = size - pos;
			}
			memcpy( &data[ pos ], buf, len );
			pos += len;
			return len;
		}
	};

	struct rdbuf {
		size_t pos;
		const size_t size;
		const uint8_t *const data;

		rdbuf( const uint8_t *const data_, size_t size_ ) : pos(0), size(size_), data(data_)
		{}

		size_t readbuf( void *buf, size_t len ) {
			if( pos + len > size ) {
				len = size - pos;
			}
			memcpy( buf, &data[ pos ], len );
			pos += len;
			return len;
		}
	};

	size_t elzma_write_callback( void *ctx, const void *buf, size_t size ) {
		wrbuf * f = (wrbuf *) ctx;
		assert( f );
		return f->writebuf( buf, size );
	}

	int elzma_read_callback( void *ctx, void *buf, size_t *size ) {
		rdbuf * f = (rdbuf *) ctx;
		assert( f );
		return *size = f->readbuf( buf, *size ), 0;
	}

	template<bool is_lzip>
	size_t lzma_decompress( const uint8_t * const data, const size_t size,
						  uint8_t * const new_data, size_t * const out_sizep )
	{
		rdbuf rd( data, size );
		wrbuf wr( new_data, *out_sizep );

		elzma_file_format format = is_lzip ? ELZMA_lzip : ELZMA_lzma;
		elzma_decompress_handle hand = elzma_decompress_alloc();
		bool ok = false, init = NULL != hand;
		if( init && ELZMA_E_OK == elzma_decompress_run(
				hand, elzma_read_callback, (void *) &rd,
				elzma_write_callback, (void *) &wr, format) ) {
			ok = true;
		}
		if( init ) elzma_decompress_free(&hand);
		return ok ? 1 : 0;
	}

	template<bool is_lzip>
	size_t lzma_compress( const uint8_t * const data, const size_t size,
						uint8_t * const new_data, size_t * const out_sizep )
	{
		/* default compression parameters, some of which may be overridded by
		 * command line arguments */
		unsigned char level = 9;
		unsigned char lc = ELZMA_LC_DEFAULT;
		unsigned char lp = ELZMA_LP_DEFAULT;
		unsigned char pb = ELZMA_PB_DEFAULT;
		unsigned int maxDictSize = ELZMA_DICT_SIZE_DEFAULT_MAX;
		unsigned int dictSize = 0;
		elzma_file_format format = is_lzip ? ELZMA_lzip : ELZMA_lzma;

		elzma_compress_handle hand = NULL;

		/* determine a reasonable dictionary size given input size */
		dictSize = elzma_get_dict_size(size);
		if (dictSize > maxDictSize) dictSize = maxDictSize;

		/* allocate a compression handle */
		hand = elzma_compress_alloc();
		if (hand == NULL) {
			return 0;
		}

		if (ELZMA_E_OK != elzma_compress_config(hand, lc, lp, pb, level,
												dictSize, format,
												size)) {
			elzma_compress_free(&hand);
			return 0;
		}

		int rv;
		int pCtx = 0;

		rdbuf rd( data, size );
		wrbuf wr( new_data, *out_sizep );

		rv = elzma_compress_run(hand, elzma_read_callback, (void *) &rd,
								elzma_write_callback, (void *) &wr,
								(NULL), &pCtx);

		if (ELZMA_E_OK != rv) {
			elzma_compress_free(&hand);
			return 0;
		}

		*out_sizep = wr.pos;

		/* clean up */
		elzma_compress_free(&hand);
		return wr.pos;
	}
}

// zpaq interface
namespace
{
	class In: public libzpaq::Reader, public rdbuf {
		public:
		In( const uint8_t *const data_, size_t size_ ) : rdbuf( data_, size_ )
		{}
		int get() {
			if( pos >= size ) {
				return -1;
			}
			return data[pos++];
		}
		int read(char* buf, int n) {
			return this->readbuf( buf, n );
		}
	};

	class Out: public libzpaq::Writer, public wrbuf {
		public:
		Out( uint8_t *const data_, size_t size_ ) : wrbuf( data_, size_ )
		{}
		void put(int c) {
			if( pos < size ) {
				data[pos++] = (unsigned char)(c);
			}
		}
		int write(char* buf, int n) {
			return this->writebuf( buf, n );
		}
	};

	size_t zpaq_compress( const uint8_t * const data, const size_t size,
						uint8_t * const new_data, size_t * const out_sizep )
	{
		In rd( data, size );
		Out wr( new_data, *out_sizep );

		libzpaq::compress(&rd, &wr, 3);  // level [1..3]
		*out_sizep = wr.pos;

		return wr.pos;
	}

	size_t zpaq_decompress( const uint8_t * const data, const size_t size,
						  uint8_t * const new_data, size_t * const out_sizep )
	{
		In rd( data, size );
		Out wr( new_data, *out_sizep );

		libzpaq::decompress( &rd, &wr );

		return 1;
	}
}
namespace libzpaq {
	// Implement error handler
	void error(const char* msg) {
		fprintf( stderr, "<bundle/bunle.cpp> says: ZPAQ fatal error! %s\n", msg );
		exit(1);
	}
}

namespace bundle {

	enum { verbose = false };

	namespace
	{
		std::string hexdump( const std::string &str ) {
			char spr[ 80 ];
			std::string out1, out2;

			for( unsigned i = 0; i < 16 && i < str.size(); ++i ) {
				std::sprintf( spr, "%c ", std::isprint(str[i]) ? str[i] : '.' );
				out1 += spr;
				std::sprintf( spr, "%02X ", (unsigned char)str[i] );
				out2 += spr;
			}

			for( unsigned i = str.size(); i < 16; ++i ) {
				std::sprintf( spr, ". " );
				out1 += spr;
				std::sprintf( spr, "?? " );
				out2 += spr;
			}

			return out1 + "[...] (" + out2 + "[...])";
		}

		void shout( unsigned q, const char *context, size_t from, size_t to ) {
			if( verbose ) {
				std::cout << context << " q:" << q << ",from:" << from << ",to:" << to << std::endl;
			}
		}
	}

	const char *const name_of( unsigned q ) {
		switch( q ) {
			break; default : return "NONE";
			break; case LZ4: return "LZ4";
			break; case MINIZ: return "MINIZ";
			break; case SHOCO: return "SHOCO";
			break; case LZIP: return "LZIP";
			break; case LZMASDK: return "LZMA";
			break; case ZPAQ: return "ZPAQ";
			break; case LZ4HC: return "LZ4HC";
#if 0
			// for archival purposes
			break; case BZIP2: return "BZIP2";
			break; case BLOSC: return "BLOSC";
			break; case BSC: return "BSC";
			break; case FSE: return "FSE";
			break; case LZFX: return "LZFX";
			break; case LZHAM: return "LZHAM";
			break; case LZP1: return "LZP1";
			break; case YAPPY: return "YAPPY";
#endif
		}
	}

	const char *const version_of( unsigned q ) {
		return "0";
	}

	const char *const ext_of( unsigned q ) {
		switch( q ) {
			break; default : return "unc";
			break; case LZ4: return "lz4";
			break; case MINIZ: return "miniz";
			break; case SHOCO: return "shoco";
			break; case LZIP: return "lz";
			break; case LZMASDK: return "lzma";
			break; case ZPAQ: return "zpaq";
			break; case LZ4HC: return "lz4";
#if 0
			// for archival purposes
			break; case BZIP2: return "bz2";
			break; case BLOSC: return "blosc";
			break; case BSC: return "bsc";
			break; case FSE: return "fse";
			break; case LZFX: return "lzfx";
			break; case LZHAM: return "lzham";
			break; case LZP1: return "lzp1";
			break; case YAPPY: return "yappy";
#endif
		}
	}

	unsigned type_of( const void *ptr, size_t size ) {
		unsigned char *mem = (unsigned char *)ptr;
		//std::string s; s.resize( size ); memcpy( &s[0], mem, size );
		//std::cout << hexdump( s) << std::endl;
		if( size >= 4 && mem && mem[0] == 'L' && mem[1] == 'Z' && mem[2] == 'I' && mem[3] == 'P' ) return LZIP;
		if( size >= 1 && mem && mem[0] == 0xEC ) return MINIZ;
		if( size >= 1 && mem && mem[0] >= 0xF0 ) return LZ4;
		return NONE;
	}

	size_t bound( unsigned q, size_t len ) {
		enum { MAX_BUNDLE_HEADERS = 1 + 1 + (16 + 1) + (16 + 1) }; // up to 128-bit length sized streams
		size_t zlen = len;
		switch( q ) {
			break; default : zlen = zlen * 2;
			break; case LZ4: case LZ4HC: zlen = LZ4_compressBound((int)(len));
			break; case MINIZ: zlen = mz_compressBound(len);
#if 0
			// for archival purposes

			break; case LZP1: zlen = lzp_bound_compress((int)(len));
			//break; case FSE: zlen = FSE_compressBound((int)len);
#endif
		}
		return zlen += MAX_BUNDLE_HEADERS, shout( q, "[bound]", len, zlen ), zlen;
	}

	size_t unc_payload( unsigned q ) {
		size_t payload;
		switch( q ) {
			break; default : payload = 0;
#if 0
			break; case LZP1: payload = 256;
#endif
		}
		return payload;
	}

	  // for archival purposes:
	  // const bool pre_init = (Yappy_FillTables(), true);

	  bool pack( unsigned q, const void *in, size_t inlen, void *out, size_t &outlen ) {
		bool ok = false;
		if( in && inlen && out && outlen >= inlen ) {
			ok = true;
			switch( q ) {
				break; default: ok = false;
				break; case LZ4: outlen = LZ4_compress( (const char *)in, (char *)out, inlen );
				break; case LZ4HC: outlen = LZ4_compressHC2( (const char *)in, (char *)out, inlen, 16 );
				break; case MINIZ: outlen = tdefl_compress_mem_to_mem( out, outlen, in, inlen, TDEFL_MAX_PROBES_MASK ); // TDEFL_DEFAULT_MAX_PROBES );
				break; case SHOCO: outlen = shoco_compress( (const char *)in, inlen, (char *)out, outlen );
				break; case LZMASDK: outlen = lzma_compress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
				break; case LZIP: outlen = lzma_compress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
				break; case ZPAQ: outlen = zpaq_compress( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
#if 0
				// for archival purposes:
				break; case YAPPY: outlen = Yappy_Compress( (const unsigned char *)in, (unsigned char *)out, inlen ) - out;
				break; case BZIP2: { unsigned int o(outlen); if( BZ_OK != BZ2_bzBuffToBuffCompress( (char *)out, &o, (char *)in, inlen, 9 /*level*/, 0 /*verbosity*/, 30 /*default*/ ) ) outlen = 0; else outlen = o; }
				break; case BLOSC: { int clevel = 9, doshuffle = 0, typesize = 1;
					int r = blosc_compress( clevel, doshuffle, typesize, inlen, in, out, outlen);
					if( r <= 0 ) outlen = 0; else outlen = r; }
				break; case BSC: outlen = bsc_compress((const unsigned char *)in, (unsigned char *)out, inlen, LIBBSC_DEFAULT_LZPHASHSIZE, LIBBSC_DEFAULT_LZPMINLEN, LIBBSC_DEFAULT_BLOCKSORTER, LIBBSC_CODER_QLFC_ADAPTIVE, LIBBSC_FEATURE_FASTMODE | 0);
				break; case FSE: outlen = FSE_compress( out, (const unsigned char *)in, inlen ); if( outlen < 0 ) outlen = 0;
				break; case LZFX: if( lzfx_compress( in, inlen, out, &outlen ) < 0 ) outlen = 0;
				break; case LZP1: outlen = lzp_compress( (const uint8_t *)in, inlen, (uint8_t *)out, outlen );
				break; case LZHAM: {
						// lzham_z_ulong l; lzham_z_compress2( (unsigned char *)out, &l, (const unsigned char *)in, inlen, LZHAM_Z_BEST_COMPRESSION ); outlen = l;

						lzham_compress_params comp_params = {0};
						comp_params.m_struct_size = sizeof(comp_params);
						comp_params.m_dict_size_log2 = 23;
						comp_params.m_level = static_cast<lzham_compress_level>(0);
						comp_params.m_max_helper_threads = 1;
						comp_params.m_compress_flags |= LZHAM_COMP_FLAG_FORCE_POLAR_CODING;

						lzham_compress_status_t comp_status =
						lzham_lib_compress_memory(&comp_params, (lzham_uint8 *)out, &outlen, (const lzham_uint8*)in, inlen, 0 );

						if (comp_status != LZHAM_COMP_STATUS_SUCCESS) {
							outlen = 0;
						}
				   }
#endif

			}
			// std::cout << name_of( type_of( out, outlen ) ) << std::endl;
		}
		ok = ok && outlen > 0 && outlen < inlen;
		outlen = ok && outlen ? outlen : 0;
		return shout( q, "[pack]", inlen, outlen ), ok;
	  }

	bool unpack( unsigned q, const void *in, size_t inlen, void *out, size_t &outlen ) {
		bool ok = false;
		size_t bytes_read = 0;
		if( in && inlen && out && outlen ) {
			ok = true;
			switch( q ) {
				break; default: ok = false;
				break; case LZ4: case LZ4HC: if( LZ4_decompress_safe( (const char *)in, (char *)out, inlen, outlen ) >= 0 ) bytes_read = inlen; // faster: bytes_read = LZ4_uncompress( (const char *)in, (char *)out, outlen );
				break; case MINIZ: bytes_read = inlen; tinfl_decompress_mem_to_mem( out, outlen, in, inlen, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF );
				break; case SHOCO: bytes_read = inlen; shoco_decompress( (const char *)in, inlen, (char *)out, outlen );
				break; case LZMASDK: if( lzma_decompress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
				break; case LZIP: if( lzma_decompress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
				break; case ZPAQ: if( zpaq_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
#if 0
				// for archival purposes:
				break; case YAPPY: Yappy_UnCompress( (const unsigned char *)in, ((const unsigned char *)in) + inlen, (unsigned char *)out ); bytes_read = inlen;
				break; case BZIP2: { unsigned int o(outlen); if( BZ_OK == BZ2_bzBuffToBuffDecompress( (char *)out, &o, (char *)in, inlen, 0 /*fast*/, 0 /*verbosity*/ ) ) { bytes_read = inlen; outlen = o; }}
				break; case BLOSC: if( blosc_decompress( in, out, outlen ) > 0 ) bytes_read = inlen;
				break; case BSC: bsc_decompress((const unsigned char *)in, inlen, (unsigned char *)out, outlen, /*LIBBSC_FEATURE_FASTMODE | */0); bytes_read = inlen;
				break; case FSE: { int r = FSE_decompress( (unsigned char*)out, outlen, (const void *)in ); if( r >= 0 ) bytes_read = r; }
				break; case LZFX: if( lzfx_decompress( in, inlen, out, &outlen ) >= 0 ) bytes_read = inlen;
				break; case LZP1: lzp_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, outlen ); bytes_read = inlen;
				break; case LZHAM: {
					//bytes_read = inlen; { lzham_z_ulong l = outlen; lzham_z_uncompress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); }

					lzham_decompress_params decomp_params = {0};
					decomp_params.m_struct_size = sizeof(decomp_params);
					decomp_params.m_dict_size_log2 = 23;
					decomp_params.m_decompress_flags |= LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED;

					lzham_decompress_status_t status = lzham_lib_decompress_memory(&decomp_params,
						(lzham_uint8*)out, &outlen, (const lzham_uint8*)in, inlen, 0);
					if( status == LZHAM_DECOMP_STATUS_SUCCESS ) {
						bytes_read = inlen;
					}
				}
#endif
			}
		}
		ok = ok && bytes_read == inlen;
		outlen = ok && outlen ? outlen : 0;
		return shout( q, "[unpack]", inlen, outlen ), ok;
	}

}

namespace bundle
{
	// public API

	unsigned type_of( const std::string &self ) {
		return is_packed( self ) ? self[ 1 ] & 0x0F : NONE;
	}
	std::string name_of( const std::string &self ) {
		return bundle::name_of( type_of(self) );
	}
	std::string version_of( const std::string &self ) {
		return bundle::version_of( type_of(self) );
	}
	std::string ext_of( const std::string &self ) {
		return bundle::ext_of( type_of(self) );
	}

	std::string vlebit( size_t i ) {
		std::string out;
		do {
			out += (unsigned char)( 0x80 | (i & 0x7f));
			i >>= 7;
		} while( i > 0 );
		*out.rbegin() ^= 0x80;
		return out;
	}
	size_t vlebit( const char *&i ) {
		size_t out = 0, j = -7;
		do {
			out |= ((size_t(*i) & 0x7f) << (j += 7) );
		} while( size_t(*i++) & 0x80 );
		return out;
	}
}

namespace bundle
{
	bool pakfile::has( const std::string &property ) const {
		return this->find( property ) != this->end();
	}

	// binary serialization

	bool pak::bin( const std::string &bin_import ) //const
	{
		this->clear();
		std::vector<pakfile> result;

		if( !bin_import.size() )
			return true; // :)

		if( type == paktype::ZIP )
		{
			// Try to open the archive.
			mz_zip_archive zip_archive;
			memset(&zip_archive, 0, sizeof(zip_archive));

			mz_bool status = mz_zip_reader_init_mem( &zip_archive, (void *)bin_import.c_str(), bin_import.size(), 0 );

			if( !status )
				return "mz_zip_reader_init_file() failed!", false;

			// Get and print information about each file in the archive.
			for( unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++ )
			{
				mz_zip_archive_file_stat file_stat;

				if( !mz_zip_reader_file_stat( &zip_archive, i, &file_stat ) )
				{
					mz_zip_reader_end( &zip_archive );
					return "mz_zip_reader_file_stat() failed!", false;
				}

				result.push_back( pakfile() );

				result.back()["filename"] = file_stat.m_filename;
				result.back()["comment"] = file_stat.m_comment;
				result.back()["size"] = (unsigned int)file_stat.m_uncomp_size;
				result.back()["size_z"] = (unsigned int)file_stat.m_comp_size;
				//result.back()["modify_time"] = ze.mtime;
				//result.back()["access_time"] = ze.atime;
				//result.back()["create_time"] = ze.ctime;
				//result.back()["attributes"] = ze.attr;

				if( const bool decode = true )
				{
					// Try to extract file to the heap.
					size_t uncomp_size;

					void *p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);

					if( !p )
					{
						mz_zip_reader_end(&zip_archive);
						return "mz_zip_reader_extract_file_to_heap() failed!", false;
					}

					// Make sure the extraction really succeeded.
					/*
					if ((uncomp_size != strlen(s_pStr)) || (memcmp(p, s_pStr, strlen(s_pStr))))
					{
					free(p);
					mz_zip_reader_end(&zip_archive);
					return "mz_zip_reader_extract_file_to_heap() failed to extract the proper data", false;
					}
					*/

					result.back()["content"].resize( uncomp_size );
					memcpy( (void *)result.back()["content"].data(), p, uncomp_size );

					free(p);
				}
			}

			// We're done.
			mz_zip_reader_end(&zip_archive);
		}
		else
		{}

		this->resize( result.size() );
		std::copy( result.begin(), result.end(), this->begin() );

		return true;
	}

	std::string pak::bin( unsigned q ) const
	{
		std::string result;

		if( type == paktype::ZIP )
		{
			mz_zip_archive zip_archive;
			memset( &zip_archive, 0, sizeof(zip_archive) );

			zip_archive.m_file_offset_alignment = 8;

			mz_bool status = mz_zip_writer_init_heap( &zip_archive, 0, 0 );

			if( !status ) {
				assert( status );
				return "mz_zip_writer_init_heap() failed!", std::string();
			}

			for( const_iterator it = this->begin(), end = this->end(); it != end; ++it ) {
				std::map< std::string, bundle::string >::const_iterator filename = it->find("filename");
				std::map< std::string, bundle::string >::const_iterator content = it->find("content");
				std::map< std::string, bundle::string >::const_iterator comment = it->find("comment");
				if( filename != it->end() && content != it->end() ) {
					const size_t bufsize = content->second.size();

					int quality = q;
					if( it->find("compression") != it->end() ) {
						std::stringstream ss( it->find("compression")->second );
						if( !(ss >> quality) ) quality = q;
					}
					switch( quality ) {
						break; case DEFAULT: default: quality = MZ_DEFAULT_LEVEL;
						break; case UNCOMPRESSED: quality = MZ_NO_COMPRESSION;
						break; case FAST: case ASCII: quality = MZ_BEST_SPEED;
						break; case EXTRA: case UBER: quality = MZ_BEST_COMPRESSION;
					}

					std::string pathfile = filename->second;
					std::replace( pathfile.begin(), pathfile.end(), '\\', '/');

					//std::cout << hexdump(content->second) << std::endl;

					if( comment == it->end() )
					status = mz_zip_writer_add_mem_ex( &zip_archive, pathfile.c_str(), content->second.c_str(), bufsize,
						0, 0, quality, 0, 0 );
					else
					status = mz_zip_writer_add_mem_ex( &zip_archive, pathfile.c_str(), content->second.c_str(), bufsize,
						comment->second.c_str(), comment->second.size(), quality, 0, 0 );

					//status = mz_zip_writer_add_mem( &zip_archive, filename->second.c_str(), content->second.c_str(), bufsize, quality );
					if( !status ) {
						assert( status );
						return "mz_zip_writer_add_mem() failed!", std::string();
					}
				}
			}

			void *pBuf;
			size_t pSize;

			status = mz_zip_writer_finalize_heap_archive( &zip_archive, &pBuf, &pSize);
			if( !status ) {
				assert( status );
				return "mz_zip_writer_finalize_heap_archive() failed!", std::string();
			}

			// Ends archive writing, freeing all allocations, and closing the output file if mz_zip_writer_init_file() was used.
			// Note for the archive to be valid, it must have been finalized before ending.
			status = mz_zip_writer_end( &zip_archive );
			if( !status ) {
				assert( status );
				return "mz_zip_writer_end() failed!", std::string();
			}

			result.resize( pSize );
			memcpy( &result.at(0), pBuf, pSize );

			free( pBuf );
		}
		else
		{}

		return result;
	}
}


