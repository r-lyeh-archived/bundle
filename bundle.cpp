/** this is an amalgamated file. do not edit.
 */

#line 1 "bundle.hpp"
#ifndef BUNDLE_HPP
#define BUNDLE_HPP

#if ( defined(_MSC_VER) && _MSC_VER >= 1800 ) || __cplusplus >= 201103L
#define BUNDLE_CXX11 1
#else
#define BUNDLE_CXX11 0
#endif

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if BUNDLE_CXX11
#include <chrono>
#endif

namespace bundle
{
	// per lib
	enum { UNDEFINED, SHOCO, LZ4, MINIZ, LZIP, LZMASDK };
	// per family
	enum { NONE = UNDEFINED, ASCII = SHOCO, LZ77 = LZ4, DEFLATE = MINIZ, LZMA = LZMASDK };
	// per context
	enum { UNCOMPRESSED = NONE, ENTROPY = ASCII, FAST = LZ77, DEFAULT = DEFLATE, EXTRA = LZMA };

	// dont compress if compression ratio is below 5%
	enum { NO_COMPRESSION_TRESHOLD = 5 };

	// high level API

	//std::string pack( unsigned q, const std::string &self );
	//std::string unpack( const std::string &self );

	unsigned type_of( const std::string &self );
	std::string name_of( const std::string &self );
	std::string version_of( const std::string &self );
	std::string ext_of( const std::string &self );
	size_t length( const std::string &self );
	size_t zlength( const std::string &self );
	void *zptr( const std::string &self );

	// low level API

	  bool pack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );
	  bool unpack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );
	size_t bound( unsigned q, size_t len );
	const char *const name_of( unsigned q );
	const char *const version_of( unsigned q );
	const char *const ext_of( unsigned q );
	unsigned type_of( const void *mem, size_t size );

	// template API

	template < class T1, class T2 >
	static inline bool pack( unsigned q, T2 &buffer_out, const T1 &buffer_in ) {
		// sanity checks
		assert( sizeof(buffer_in.at(0)) == 1 && "size of input elements != 1" );
		assert( sizeof(buffer_out.at(0)) == 1 && "size of output elements != 1" );

		// resize to worst case
		size_t zlen = bound(q, buffer_in.size());
		buffer_out.resize( zlen );

		// compress
		bool result = pack( q, &buffer_in.at(0), buffer_in.size(), &buffer_out.at(0), zlen );

		// resize properly
		return result ? ( buffer_out.resize( zlen ), true ) : ( buffer_out = T2(), false );
	}

	template < class T1, class T2 >
	static inline bool unpack( unsigned q, T2 &buffer_out, const T1 &buffer_in ) {
		// sanity checks
		assert( sizeof(buffer_in.at(0)) == 1 && "size of input elements != 1" );
		assert( sizeof(buffer_out.at(0)) == 1 && "size of output elements != 1" );

		// note: buffer_out must be resized properly before calling this function!!
		size_t zlen = buffer_out.size();
		return unpack( q, &buffer_in.at(0), buffer_in.size(), &buffer_out.at(0), zlen );
	}

	std::string vlebit( size_t i );
	size_t vlebit( const char *&i );

	template<typename container>
	static inline bool is_packed( const container &input ) {
		return input.size() >= 2 && 0 == input[0] && input[1] >= 0x70 && input[1] <= 0x7F;
	}

	template<typename container>
	static inline bool is_unpacked( const container &input ) {
		return !is_packed(input);
	}

	template<typename container>
	static inline container pack( unsigned q, const container &input ) {
		if( is_packed( input ) )
			return input;

		// sanity checks
		assert( sizeof(input.at(0)) == 1 && "size of input elements != 1" );

		container output( bound( q, input.size() ), '\0' );

		// compress
		size_t len = output.size();
		if( !pack( q, &input[0], input.size(), &output[0], len ) )
			return input;
		output.resize( len );

		// encapsulate
		output = std::string() + char(0) + char(0x70 | (q & 0x0F)) + vlebit(input.size()) + vlebit(output.size()) + output;
		return output;
	}

	template<typename container>
	static inline container unpack( const container &input ) {
		if( is_unpacked( input ) )
			return input;

		// sanity checks
		assert( sizeof(input.at(0)) == 1 && "size of input elements != 1" );

		// decapsulate
		unsigned Q = input[1] & 0x0F;
		const char *ptr = (const char *)&input[2];
		size_t size1 = vlebit(ptr);
		size_t size2 = vlebit(ptr);

		container output( size1, '\0' );

		// decompress
		size_t len = output.size();
		if( !unpack( Q, ptr, size2, &output[0], len ) )
			return input;

		return output;
	}

	static inline std::vector<unsigned> encodings() {
		static std::vector<unsigned> all;
		if( all.empty() ) {
			all.push_back( LZ4 );
			all.push_back( SHOCO );
			all.push_back( MINIZ );
			all.push_back( LZIP );
			all.push_back( LZMASDK );
			all.push_back( NONE );
		}
		return all;
	}

#if BUNDLE_CXX11

	// measures for all given encodings
	struct measure {
		unsigned q = NONE;
		double ratio = 0;
		double enctime = 0;
		double dectime = 0;
		double memusage = 0;
		bool pass = 0;
		std::string str() const {
			std::stringstream ss;
			ss << ( pass ? "[ OK ] " : "[FAIL] ") << name_of(q) << ": ratio=" << ratio << "% enctime=" << enctime << "ms dectime=" << dectime << " ms";
			return ss.str();
		}
	};

	template< class T, bool do_enc = true, bool do_dec = true, bool do_verify = true >
	std::vector< measure > measures( const T& original, const std::vector<unsigned> &use_encodings = encodings() ) {
		std::vector< measure > results;
		T zipped, unzipped;

		for( auto encoding : use_encodings ) {
			results.push_back( measure() );
			auto &r = results.back();
			r.q = encoding;
			r.pass = false;

			if( do_enc ) {
				auto begin = std::chrono::high_resolution_clock::now();
				zipped = pack( encoding, original );
				auto end = std::chrono::high_resolution_clock::now();
				r.enctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
				r.ratio = 100 - 100 * ( double( zipped.size() ) / original.size() );
			}

			if( do_dec ) {
				auto begin = std::chrono::high_resolution_clock::now();
				unzipped = unpack( zipped );
				auto end = std::chrono::high_resolution_clock::now();
				r.dectime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
				r.pass = ( original == unzipped );
			}

			if( encoding == NONE )
				r.pass = r.pass && do_verify && do_enc && do_dec && is_unpacked(zipped) && is_unpacked(unzipped) && original == unzipped;
			else
				r.pass = r.pass && do_verify && do_enc && do_dec && is_packed(zipped) && is_unpacked(unzipped) && original == unzipped;
		}

		return results;
	}

	// find best choice for given data
	template< class T >
	unsigned find_smallest_compressor( const T& original, const std::vector<unsigned> &use_encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double ratio = 0;

		auto results = measures< true, false, false >( original, use_encodings );
		for( auto &r : results ) {
			if( r.pass && r.ratio > ratio && r.ratio >= (100 - NO_COMPRESSION_TRESHOLD / 100.0) ) {
				ratio = r.ratio;
				q = r.q;
			}
		}

		return q;
	}

	template< class T >
	unsigned find_fastest_compressor( const T& original, const std::vector<unsigned> &use_encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double enctime = 9999999;

		auto results = measures< true, false, false >( original, use_encodings );
		for( auto &r : results ) {
			if( r.pass && r.enctime < enctime ) {
				enctime = r.enctime;
				q = r.q;
			}
		}

		return q;
	}

	template< class T >
	unsigned find_fastest_decompressor( const T& original, const std::vector<unsigned> &use_encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double dectime = 9999999;

		auto results = measures< false, true, false >( original, encodings );
		for( auto &r : results ) {
			if( r.pass && r.dectime < dectime ) {
				dectime = r.dectime;
				q = r.q;
			}
		}

		return q;
	}

#endif
}

namespace bundle
{
	struct paktype
	{
		enum enumeration { ZIP };
	};

	class string : public std::string
	{
		public:

		string() : std::string()
		{}

		template< typename T >
		explicit string( const T &t ) : std::string() {
			operator=(t);
		}

		template< typename T >
		string &operator=( const T &t ) {
			std::stringstream ss;
			ss.precision(20);
			return ss << t ? (this->assign( ss.str() ), *this) : *this;
		}

		template< typename T >
		T as() const {
			T t;
			std::stringstream ss( *this );
			return ss >> t ? t : T();
		}

		template< typename T0 >
		string( const std::string &_fmt, const T0 &t0 ) : std::string() {
			std::string t[] = { string(), string(t0) };
			for( const char *fmt = _fmt.c_str(); *fmt; ++fmt ) {
				/**/ if( *fmt == '\1' ) t[0] += t[1];
				else                    t[0] += *fmt;
			}
			this->assign( t[0] );
		}

		template< typename T0, typename T1 >
		string( const std::string &_fmt, const T0 &t0, const T1 &t1 ) : std::string() {
			std::string t[] = { string(), string(t0), string(t1) };
			for( const char *fmt = _fmt.c_str(); *fmt; ++fmt ) {
				/**/ if( *fmt == '\1' ) t[0] += t[1];
				else if( *fmt == '\2' ) t[0] += t[2];
				else                    t[0] += *fmt;
			}
			this->assign( t[0] );
		}
	};

	struct pakfile : public std::map< std::string, bundle::string >
	{
		bool has( const std::string &property ) const;

		template <typename T>
		T get( const std::string &property ) const {
			return (*this->find( property )).second.as<T>();
		}
	};

	class pak : public std::vector< pakfile >
	{
		public:

		const paktype::enumeration type;

		explicit
		pak( const paktype::enumeration &etype = paktype::ZIP ) : type(etype)
		{}

		// binary serialization

		bool bin( const std::string &bin_import ); //const
		std::string bin( unsigned q = EXTRA ) const;

		// debug

		std::string toc() const {
			// @todo: add offset in file
			std::string ret;
			for( const_iterator it = this->begin(), end = this->end(); it != end; ++it ) {
				const pakfile &file = *it;
				ret += "\t{\n";
				for( pakfile::const_iterator it = file.begin(), end = file.end(); it != end; ++it ) {
					const std::pair< std::string, bundle::string > &property = *it;
					if( property.first == "content" )
						ret += "\t\t\"size\":\"" + string( property.second.size() ) + "\",\n";
					else
						ret += "\t\t\"" + property.first + "\":\"" + property.second + "\",\n";
				}
				ret += "\t},\n";
			}
			if( ret.size() >= 2 ) { ret[ ret.size() - 2 ] = '\n', ret = ret.substr( 0, ret.size() - 1 ); }
			return std::string() + "[\n" + ret + "]\n";
		}
	};
}

#endif



#line 1 "lz4.h"
#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

/**************************************
   Version
**************************************/
#define LZ4_VERSION_MAJOR    1    /* for major interface/format changes  */
#define LZ4_VERSION_MINOR    1    /* for minor interface/format changes  */
#define LZ4_VERSION_RELEASE  3    /* for tweaks, bug-fixes, or development */

/**************************************
   Compiler Options
**************************************/
#if (defined(__GNUC__) && defined(__STRICT_ANSI__)) || (defined(_MSC_VER) && !defined(__cplusplus))   /* Visual Studio */
#  define inline __inline           /* Visual C is not C99, but supports some kind of inline */
#endif

/**************************************
   Simple Functions
**************************************/

int LZ4_compress        (const char* source, char* dest, int inputSize);
int LZ4_decompress_safe (const char* source, char* dest, int inputSize, int maxOutputSize);

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
	maxOutputSize : is the size of the destination buffer (which must be already allocated)
	return : the number of bytes decoded in the destination buffer (necessarily <= maxOutputSize)
			 If the source stream is detected malformed, the function will stop decoding and return a negative result.
			 This function is protected against buffer overflow exploits (never writes outside of output buffer, and never reads outside of input buffer). Therefore, it is protected against malicious data packets
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
	inline function is recommended for the general case,
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
	note : This function is a bit faster than LZ4_decompress_safe()
		   This function never writes outside of output buffers, but may read beyond input buffer in case of malicious data packet.
		   Use this function preferably into a trusted environment (data to decode comes from a trusted source).
		   Destination buffer must be already allocated. Its size must be a minimum of 'outputSize' bytes.
*/
int LZ4_decompress_fast (const char* source, char* dest, int originalSize);

/*
LZ4_decompress_safe_partial() :
	This function decompress a compressed block of size 'inputSize' at position 'source'
	into output buffer 'dest' of size 'maxOutputSize'.
	The function tries to stop decompressing operation as soon as 'targetOutputSize' has been reached,
	reducing decompression time.
	return : the number of bytes decoded in the destination buffer (necessarily <= maxOutputSize)
	   Note : this number can be < 'targetOutputSize' should the compressed block to decode be smaller.
			 Always control how many bytes were decoded.
			 If the source stream is detected malformed, the function will stop decoding and return a negative result.
			 This function never writes outside of output buffer, and never reads outside of input buffer. It is therefore protected against malicious data packets
*/
int LZ4_decompress_safe_partial (const char* source, char* dest, int inputSize, int targetOutputSize, int maxOutputSize);

/*
These functions are provided should you prefer to allocate memory for compression tables with your own allocation methods.
To know how much memory must be allocated for the compression tables, use :
int LZ4_sizeofState();

Note that tables must be aligned on 4-bytes boundaries, otherwise compression will fail (return code 0).

The allocated memory can be provided to the compressions functions using 'void* state' parameter.
LZ4_compress_withState() and LZ4_compress_limitedOutput_withState() are equivalent to previously described functions.
They just use the externally allocated memory area instead of allocating their own (on stack, or on heap).
*/
int LZ4_sizeofState(void);
int LZ4_compress_withState               (void* state, const char* source, char* dest, int inputSize);
int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);

/**************************************
   Streaming Functions
**************************************/
void* LZ4_create (const char* inputBuffer);
int   LZ4_compress_continue (void* LZ4_Data, const char* source, char* dest, int inputSize);
int   LZ4_compress_limitedOutput_continue (void* LZ4_Data, const char* source, char* dest, int inputSize, int maxOutputSize);
char* LZ4_slideInputBuffer (void* LZ4_Data);
int   LZ4_free (void* LZ4_Data);

/*
These functions allow the compression of dependent blocks, where each block benefits from prior 64 KB within preceding blocks.
In order to achieve this, it is necessary to start creating the LZ4 Data Structure, thanks to the function :

void* LZ4_create (const char* inputBuffer);
The result of the function is the (void*) pointer on the LZ4 Data Structure.
This pointer will be needed in all other functions.
If the pointer returned is NULL, then the allocation has failed, and compression must be aborted.
The only parameter 'const char* inputBuffer' must, obviously, point at the beginning of input buffer.
The input buffer must be already allocated, and size at least 192KB.
'inputBuffer' will also be the 'const char* source' of the first block.

All blocks are expected to lay next to each other within the input buffer, starting from 'inputBuffer'.
To compress each block, use either LZ4_compress_continue() or LZ4_compress_limitedOutput_continue().
Their behavior are identical to LZ4_compress() or LZ4_compress_limitedOutput(),
but require the LZ4 Data Structure as their first argument, and check that each block starts right after the previous one.
If next block does not begin immediately after the previous one, the compression will fail (return 0).

When it's no longer possible to lay the next block after the previous one (not enough space left into input buffer), a call to :
char* LZ4_slideInputBuffer(void* LZ4_Data);
must be performed. It will typically copy the latest 64KB of input at the beginning of input buffer.
Note that, for this function to work properly, minimum size of an input buffer must be 192KB.
==> The memory position where the next input data block must start is provided as the result of the function.

Compression can then resume, using LZ4_compress_continue() or LZ4_compress_limitedOutput_continue(), as usual.

When compression is completed, a call to LZ4_free() will release the memory used by the LZ4 Data Structure.
*/

int LZ4_sizeofStreamState(void);
int LZ4_resetStreamState(void* state, const char* inputBuffer);

/*
These functions achieve the same result as :
void* LZ4_create (const char* inputBuffer);

They are provided here to allow the user program to allocate memory using its own routines.

To know how much space must be allocated, use LZ4_sizeofStreamState();
Note also that space must be 4-bytes aligned.

Once space is allocated, you must initialize it using : LZ4_resetStreamState(void* state, const char* inputBuffer);
void* state is a pointer to the space allocated.
It must be aligned on 4-bytes boundaries, and be large enough.
The parameter 'const char* inputBuffer' must, obviously, point at the beginning of input buffer.
The input buffer must be already allocated, and size at least 192KB.
'inputBuffer' will also be the 'const char* source' of the first block.

The same space can be re-used multiple times, just by initializing it each time with LZ4_resetStreamState().
return value of LZ4_resetStreamState() must be 0 is OK.
Any other value means there was an error (typically, pointer is not aligned on 4-bytes boundaries).
*/

int LZ4_decompress_safe_withPrefix64k (const char* source, char* dest, int inputSize, int maxOutputSize);
int LZ4_decompress_fast_withPrefix64k (const char* source, char* dest, int outputSize);

/*
*_withPrefix64k() :
	These decoding functions work the same as their "normal name" versions,
	but can use up to 64KB of data in front of 'char* dest'.
	These functions are necessary to decode inter-dependant blocks.
*/

/**************************************
   Obsolete Functions
**************************************/
/*
These functions are deprecated and should no longer be used.
They are provided here for compatibility with existing user programs.
*/
int LZ4_uncompress (const char* source, char* dest, int outputSize);
int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize);

#if defined (__cplusplus)
}
#endif


#line 1 "shoco.h"
#pragma once

#include <stddef.h>

size_t shoco_compress(const char * const in, size_t len, char * const out, size_t bufsize);
size_t shoco_decompress(const char * const in, size_t len, char * const out, size_t bufsize);


#line 1 "compress.h"
#ifndef __EASYLZMACOMPRESS_H__
#define __EASYLZMACOMPRESS_H__


#line 1 "common.h"
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


#line 1 "decompress.h"
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

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
//#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
//#define MINIZ_HAS_64BIT_REGISTERS 1

#line 1 "miniz.c"
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

// Various ZIP archive enums. To completely avoid cross platform compiler alignment and platform endian issues, miniz.c doesn't use structs for any of this stuff.
enum
{
  // ZIP archive identifiers and record sizes
  MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG = 0x06054b50, MZ_ZIP_CENTRAL_DIR_HEADER_SIG = 0x02014b50, MZ_ZIP_LOCAL_DIR_HEADER_SIG = 0x04034b50,
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

  if (!mz_zip_writer_write_zeros(pZip, cur_archive_file_ofs, num_alignment_padding_bytes + sizeof(local_dir_header)))
  {
	pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
	return MZ_FALSE;
  }
  local_dir_header_ofs += num_alignment_padding_bytes;
  if (pZip->m_file_offset_alignment) { MZ_ASSERT((local_dir_header_ofs & (pZip->m_file_offset_alignment - 1)) == 0); }
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

	n = sizeof(mz_uint32) * ((MZ_READ_LE32(pBuf) == 0x08074b50) ? 4 : 3);
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


// lz4 defines 'inline' and 'restrict' which is later required by shoco

#line 1 "lz4.c"
/**************************************
   Tuning parameters
**************************************/
/*
 * MEMORY_USAGE :
 * Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
 * Increasing memory usage improves compression ratio
 * Reduced memory usage can improve speed, due to cache effect
 * Default value is 14, for 16KB, which nicely fits into Intel x86 L1 cache
 */
#define MEMORY_USAGE 14

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
  || defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) \
  || defined(__64BIT__) || defined(_LP64) || defined(__LP64__) \
  || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) )   /* Detects 64 bits mode */
#  define LZ4_ARCH64 1
#else
#  define LZ4_ARCH64 0
#endif

/*
 * Little Endian or Big Endian ?
 * Overwrite the #define below if you know your architecture endianess
 */
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

#line 1 "lz4.h"
#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

/**************************************
   Version
**************************************/
#define LZ4_VERSION_MAJOR    1    /* for major interface/format changes  */
#define LZ4_VERSION_MINOR    1    /* for minor interface/format changes  */
#define LZ4_VERSION_RELEASE  3    /* for tweaks, bug-fixes, or development */

/**************************************
   Compiler Options
**************************************/
#if (defined(__GNUC__) && defined(__STRICT_ANSI__)) || (defined(_MSC_VER) && !defined(__cplusplus))   /* Visual Studio */
#  define inline __inline           /* Visual C is not C99, but supports some kind of inline */
#endif

/**************************************
   Simple Functions
**************************************/

int LZ4_compress        (const char* source, char* dest, int inputSize);
int LZ4_decompress_safe (const char* source, char* dest, int inputSize, int maxOutputSize);

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
	maxOutputSize : is the size of the destination buffer (which must be already allocated)
	return : the number of bytes decoded in the destination buffer (necessarily <= maxOutputSize)
			 If the source stream is detected malformed, the function will stop decoding and return a negative result.
			 This function is protected against buffer overflow exploits (never writes outside of output buffer, and never reads outside of input buffer). Therefore, it is protected against malicious data packets
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
	inline function is recommended for the general case,
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
	note : This function is a bit faster than LZ4_decompress_safe()
		   This function never writes outside of output buffers, but may read beyond input buffer in case of malicious data packet.
		   Use this function preferably into a trusted environment (data to decode comes from a trusted source).
		   Destination buffer must be already allocated. Its size must be a minimum of 'outputSize' bytes.
*/
int LZ4_decompress_fast (const char* source, char* dest, int originalSize);

/*
LZ4_decompress_safe_partial() :
	This function decompress a compressed block of size 'inputSize' at position 'source'
	into output buffer 'dest' of size 'maxOutputSize'.
	The function tries to stop decompressing operation as soon as 'targetOutputSize' has been reached,
	reducing decompression time.
	return : the number of bytes decoded in the destination buffer (necessarily <= maxOutputSize)
	   Note : this number can be < 'targetOutputSize' should the compressed block to decode be smaller.
			 Always control how many bytes were decoded.
			 If the source stream is detected malformed, the function will stop decoding and return a negative result.
			 This function never writes outside of output buffer, and never reads outside of input buffer. It is therefore protected against malicious data packets
*/
int LZ4_decompress_safe_partial (const char* source, char* dest, int inputSize, int targetOutputSize, int maxOutputSize);

/*
These functions are provided should you prefer to allocate memory for compression tables with your own allocation methods.
To know how much memory must be allocated for the compression tables, use :
int LZ4_sizeofState();

Note that tables must be aligned on 4-bytes boundaries, otherwise compression will fail (return code 0).

The allocated memory can be provided to the compressions functions using 'void* state' parameter.
LZ4_compress_withState() and LZ4_compress_limitedOutput_withState() are equivalent to previously described functions.
They just use the externally allocated memory area instead of allocating their own (on stack, or on heap).
*/
int LZ4_sizeofState(void);
int LZ4_compress_withState               (void* state, const char* source, char* dest, int inputSize);
int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);

/**************************************
   Streaming Functions
**************************************/
void* LZ4_create (const char* inputBuffer);
int   LZ4_compress_continue (void* LZ4_Data, const char* source, char* dest, int inputSize);
int   LZ4_compress_limitedOutput_continue (void* LZ4_Data, const char* source, char* dest, int inputSize, int maxOutputSize);
char* LZ4_slideInputBuffer (void* LZ4_Data);
int   LZ4_free (void* LZ4_Data);

/*
These functions allow the compression of dependent blocks, where each block benefits from prior 64 KB within preceding blocks.
In order to achieve this, it is necessary to start creating the LZ4 Data Structure, thanks to the function :

void* LZ4_create (const char* inputBuffer);
The result of the function is the (void*) pointer on the LZ4 Data Structure.
This pointer will be needed in all other functions.
If the pointer returned is NULL, then the allocation has failed, and compression must be aborted.
The only parameter 'const char* inputBuffer' must, obviously, point at the beginning of input buffer.
The input buffer must be already allocated, and size at least 192KB.
'inputBuffer' will also be the 'const char* source' of the first block.

All blocks are expected to lay next to each other within the input buffer, starting from 'inputBuffer'.
To compress each block, use either LZ4_compress_continue() or LZ4_compress_limitedOutput_continue().
Their behavior are identical to LZ4_compress() or LZ4_compress_limitedOutput(),
but require the LZ4 Data Structure as their first argument, and check that each block starts right after the previous one.
If next block does not begin immediately after the previous one, the compression will fail (return 0).

When it's no longer possible to lay the next block after the previous one (not enough space left into input buffer), a call to :
char* LZ4_slideInputBuffer(void* LZ4_Data);
must be performed. It will typically copy the latest 64KB of input at the beginning of input buffer.
Note that, for this function to work properly, minimum size of an input buffer must be 192KB.
==> The memory position where the next input data block must start is provided as the result of the function.

Compression can then resume, using LZ4_compress_continue() or LZ4_compress_limitedOutput_continue(), as usual.

When compression is completed, a call to LZ4_free() will release the memory used by the LZ4 Data Structure.
*/

int LZ4_sizeofStreamState(void);
int LZ4_resetStreamState(void* state, const char* inputBuffer);

/*
These functions achieve the same result as :
void* LZ4_create (const char* inputBuffer);

They are provided here to allow the user program to allocate memory using its own routines.

To know how much space must be allocated, use LZ4_sizeofStreamState();
Note also that space must be 4-bytes aligned.

Once space is allocated, you must initialize it using : LZ4_resetStreamState(void* state, const char* inputBuffer);
void* state is a pointer to the space allocated.
It must be aligned on 4-bytes boundaries, and be large enough.
The parameter 'const char* inputBuffer' must, obviously, point at the beginning of input buffer.
The input buffer must be already allocated, and size at least 192KB.
'inputBuffer' will also be the 'const char* source' of the first block.

The same space can be re-used multiple times, just by initializing it each time with LZ4_resetStreamState().
return value of LZ4_resetStreamState() must be 0 is OK.
Any other value means there was an error (typically, pointer is not aligned on 4-bytes boundaries).
*/

int LZ4_decompress_safe_withPrefix64k (const char* source, char* dest, int inputSize, int maxOutputSize);
int LZ4_decompress_fast_withPrefix64k (const char* source, char* dest, int outputSize);

/*
*_withPrefix64k() :
	These decoding functions work the same as their "normal name" versions,
	but can use up to 64KB of data in front of 'char* dest'.
	These functions are necessary to decode inter-dependant blocks.
*/

/**************************************
   Obsolete Functions
**************************************/
/*
These functions are deprecated and should no longer be used.
They are provided here for compatibility with existing user programs.
*/
int LZ4_uncompress (const char* source, char* dest, int outputSize);
int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize);

#if defined (__cplusplus)
}
#endif


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
#define LZ4_HASHLOG   (MEMORY_USAGE-2)
#define HASHTABLESIZE (1 << MEMORY_USAGE)
#define HASHNBCELLS4  (1 << LZ4_HASHLOG)

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
	U32 hashTable[HASHNBCELLS4];
	const BYTE* bufferStart;
	const BYTE* base;
	const BYTE* nextBlock;
} LZ4_Data_Structure;

typedef enum { notLimited = 0, limited = 1 } limitedOutput_directive;
typedef enum { byPtr, byU32, byU16 } tableType_t;

typedef enum { noPrefix = 0, withPrefix = 1 } prefix64k_directive;

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
#if LZ4_ARCH64 || !defined(__GNUC__)
#  define LZ4_WILDCOPY(d,s,e)     { do { LZ4_COPY8(d,s) } while (d<e); }           /* at the end, d>=e; */
#else
#  define LZ4_WILDCOPY(d,s,e)     { if (likely(e-d <= 8)) LZ4_COPY8(d,s) else do { LZ4_COPY8(d,s) } while (d<e); }
#endif
#define LZ4_SECURECOPY(d,s,e)     { if (d<e) LZ4_WILDCOPY(d,s,e); }

/****************************
   Private local functions
****************************/
#if LZ4_ARCH64

FORCE_INLINE int LZ4_NbCommonBytes (register U64 val)
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

FORCE_INLINE int LZ4_NbCommonBytes (register U32 val)
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

/****************************
   Compression functions
****************************/
int LZ4_compressBound(int isize)  { return LZ4_COMPRESSBOUND(isize); }

FORCE_INLINE int LZ4_hashSequence(U32 sequence, tableType_t tableType)
{
	if (tableType == byU16)
		return (((sequence) * 2654435761U) >> ((MINMATCH*8)-(LZ4_HASHLOG+1)));
	else
		return (((sequence) * 2654435761U) >> ((MINMATCH*8)-LZ4_HASHLOG));
}

FORCE_INLINE int LZ4_hashPosition(const BYTE* p, tableType_t tableType) { return LZ4_hashSequence(A32(p), tableType); }

FORCE_INLINE void LZ4_putPositionOnHash(const BYTE* p, U32 h, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	switch (tableType)
	{
	case byPtr: { const BYTE** hashTable = (const BYTE**) tableBase; hashTable[h] = p; break; }
	case byU32: { U32* hashTable = (U32*) tableBase; hashTable[h] = (U32)(p-srcBase); break; }
	case byU16: { U16* hashTable = (U16*) tableBase; hashTable[h] = (U16)(p-srcBase); break; }
	}
}

FORCE_INLINE void LZ4_putPosition(const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	U32 h = LZ4_hashPosition(p, tableType);
	LZ4_putPositionOnHash(p, h, tableBase, tableType, srcBase);
}

FORCE_INLINE const BYTE* LZ4_getPositionOnHash(U32 h, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	if (tableType == byPtr) { const BYTE** hashTable = (const BYTE**) tableBase; return hashTable[h]; }
	if (tableType == byU32) { U32* hashTable = (U32*) tableBase; return hashTable[h] + srcBase; }
	{ U16* hashTable = (U16*) tableBase; return hashTable[h] + srcBase; }   /* default, to ensure a return */
}

FORCE_INLINE const BYTE* LZ4_getPosition(const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
	U32 h = LZ4_hashPosition(p, tableType);
	return LZ4_getPositionOnHash(h, tableBase, tableType, srcBase);
}

FORCE_INLINE int LZ4_compress_generic(
				 void* ctx,
				 const char* source,
				 char* dest,
				 int inputSize,
				 int maxOutputSize,

				 limitedOutput_directive limitedOutput,
				 tableType_t tableType,
				 prefix64k_directive prefix)
{
	const BYTE* ip = (const BYTE*) source;
	const BYTE* const base = (prefix==withPrefix) ? ((LZ4_Data_Structure*)ctx)->base : (const BYTE*) source;
	const BYTE* const lowLimit = ((prefix==withPrefix) ? ((LZ4_Data_Structure*)ctx)->bufferStart : (const BYTE*)source);
	const BYTE* anchor = (const BYTE*) source;
	const BYTE* const iend = ip + inputSize;
	const BYTE* const mflimit = iend - MFLIMIT;
	const BYTE* const matchlimit = iend - LASTLITERALS;

	BYTE* op = (BYTE*) dest;
	BYTE* const oend = op + maxOutputSize;

	int length;
	const int skipStrength = SKIPSTRENGTH;
	U32 forwardH;

	/* Init conditions */
	if ((U32)inputSize > (U32)LZ4_MAX_INPUT_SIZE) return 0;                                /* Unsupported input size, too large (or negative) */
	if ((prefix==withPrefix) && (ip != ((LZ4_Data_Structure*)ctx)->nextBlock)) return 0;   /* must continue from end of previous block */
	if (prefix==withPrefix) ((LZ4_Data_Structure*)ctx)->nextBlock=iend;                    /* do it now, due to potential early exit */
	if ((tableType == byU16) && (inputSize>=(int)LZ4_64KLIMIT)) return 0;                  /* Size too large (not within 64K limit) */
	if (inputSize<LZ4_minLength) goto _last_literals;                                      /* Input too small, no compression (all literals) */

	/* First Byte */
	LZ4_putPosition(ip, ctx, tableType, base);
	ip++; forwardH = LZ4_hashPosition(ip, tableType);

	/* Main Loop */
	for ( ; ; )
	{
		int findMatchAttempts = (1U << skipStrength) + 3;
		const BYTE* forwardIp = ip;
		const BYTE* ref;
		BYTE* token;

		/* Find a match */
		do {
			U32 h = forwardH;
			int step = findMatchAttempts++ >> skipStrength;
			ip = forwardIp;
			forwardIp = ip + step;

			if (unlikely(forwardIp > mflimit)) { goto _last_literals; }

			forwardH = LZ4_hashPosition(forwardIp, tableType);
			ref = LZ4_getPositionOnHash(h, ctx, tableType, base);
			LZ4_putPositionOnHash(ip, h, ctx, tableType, base);

		} while ((ref + MAX_DISTANCE < ip) || (A32(ref) != A32(ip)));

		/* Catch up */
		while ((ip>anchor) && (ref > lowLimit) && (unlikely(ip[-1]==ref[-1]))) { ip--; ref--; }

		/* Encode Literal length */
		length = (int)(ip - anchor);
		token = op++;
		if ((limitedOutput) && (unlikely(op + length + (2 + 1 + LASTLITERALS) + (length/255) > oend))) return 0;   /* Check output limit */
		if (length>=(int)RUN_MASK)
		{
			int len = length-RUN_MASK;
			*token=(RUN_MASK<<ML_BITS);
			for(; len >= 255 ; len-=255) *op++ = 255;
			*op++ = (BYTE)len;
		}
		else *token = (BYTE)(length<<ML_BITS);

		/* Copy Literals */
		{ BYTE* end=(op)+(length); LZ4_WILDCOPY(op,anchor,end); op=end; }

_next_match:
		/* Encode Offset */
		LZ4_WRITE_LITTLEENDIAN_16(op,(U16)(ip-ref));

		/* Start Counting */
		ip+=MINMATCH; ref+=MINMATCH;    /* MinMatch already verified */
		anchor = ip;
		while (likely(ip<matchlimit-(STEPSIZE-1)))
		{
			size_t diff = AARCH(ref) ^ AARCH(ip);
			if (!diff) { ip+=STEPSIZE; ref+=STEPSIZE; continue; }
			ip += LZ4_NbCommonBytes(diff);
			goto _endCount;
		}
		if (LZ4_ARCH64) if ((ip<(matchlimit-3)) && (A32(ref) == A32(ip))) { ip+=4; ref+=4; }
		if ((ip<(matchlimit-1)) && (A16(ref) == A16(ip))) { ip+=2; ref+=2; }
		if ((ip<matchlimit) && (*ref == *ip)) ip++;
_endCount:

		/* Encode MatchLength */
		length = (int)(ip - anchor);
		if ((limitedOutput) && (unlikely(op + (1 + LASTLITERALS) + (length>>8) > oend))) return 0;    /* Check output limit */
		if (length>=(int)ML_MASK)
		{
			*token += ML_MASK;
			length -= ML_MASK;
			for (; length > 509 ; length-=510) { *op++ = 255; *op++ = 255; }
			if (length >= 255) { length-=255; *op++ = 255; }
			*op++ = (BYTE)length;
		}
		else *token += (BYTE)(length);

		/* Test end of chunk */
		if (ip > mflimit) { anchor = ip;  break; }

		/* Fill table */
		LZ4_putPosition(ip-2, ctx, tableType, base);

		/* Test next position */
		ref = LZ4_getPosition(ip, ctx, tableType, base);
		LZ4_putPosition(ip, ctx, tableType, base);
		if ((ref + MAX_DISTANCE >= ip) && (A32(ref) == A32(ip))) { token = op++; *token=0; goto _next_match; }

		/* Prepare next loop */
		anchor = ip++;
		forwardH = LZ4_hashPosition(ip, tableType);
	}

_last_literals:
	/* Encode Last Literals */
	{
		int lastRun = (int)(iend - anchor);
		if ((limitedOutput) && (((char*)op - dest) + lastRun + 1 + ((lastRun+255-RUN_MASK)/255) > (U32)maxOutputSize)) return 0;   /* Check output limit */
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
	void* ctx = ALLOCATOR(HASHNBCELLS4, 4);   /* Aligned on 4-bytes boundaries */
#else
	U32 ctx[1U<<(MEMORY_USAGE-2)] = {0};      /* Ensure data is aligned on 4-bytes boundaries */
#endif
	int result;

	if (inputSize < (int)LZ4_64KLIMIT)
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, 0, notLimited, byU16, noPrefix);
	else
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, 0, notLimited, (sizeof(void*)==8) ? byU32 : byPtr, noPrefix);

#if (HEAPMODE)
	FREEMEM(ctx);
#endif
	return result;
}

int LZ4_compress_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize)
{
#if (HEAPMODE)
	void* ctx = ALLOCATOR(HASHNBCELLS4, 4);   /* Aligned on 4-bytes boundaries */
#else
	U32 ctx[1U<<(MEMORY_USAGE-2)] = {0};      /* Ensure data is aligned on 4-bytes boundaries */
#endif
	int result;

	if (inputSize < (int)LZ4_64KLIMIT)
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, maxOutputSize, limited, byU16, noPrefix);
	else
		result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, maxOutputSize, limited, (sizeof(void*)==8) ? byU32 : byPtr, noPrefix);

#if (HEAPMODE)
	FREEMEM(ctx);
#endif
	return result;
}

/*****************************
   Using external allocation
*****************************/

int LZ4_sizeofState() { return 1 << MEMORY_USAGE; }

int LZ4_compress_withState (void* state, const char* source, char* dest, int inputSize)
{
	if (((size_t)(state)&3) != 0) return 0;   /* Error : state is not aligned on 4-bytes boundary */
	MEM_INIT(state, 0, LZ4_sizeofState());

	if (inputSize < (int)LZ4_64KLIMIT)
		return LZ4_compress_generic(state, source, dest, inputSize, 0, notLimited, byU16, noPrefix);
	else
		return LZ4_compress_generic(state, source, dest, inputSize, 0, notLimited, (sizeof(void*)==8) ? byU32 : byPtr, noPrefix);
}

int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize)
{
	if (((size_t)(state)&3) != 0) return 0;   /* Error : state is not aligned on 4-bytes boundary */
	MEM_INIT(state, 0, LZ4_sizeofState());

	if (inputSize < (int)LZ4_64KLIMIT)
		return LZ4_compress_generic(state, source, dest, inputSize, maxOutputSize, limited, byU16, noPrefix);
	else
		return LZ4_compress_generic(state, source, dest, inputSize, maxOutputSize, limited, (sizeof(void*)==8) ? byU32 : byPtr, noPrefix);
}

/****************************
   Stream functions
****************************/

int LZ4_sizeofStreamState()
{
	return sizeof(LZ4_Data_Structure);
}

FORCE_INLINE void LZ4_init(LZ4_Data_Structure* lz4ds, const BYTE* base)
{
	MEM_INIT(lz4ds->hashTable, 0, sizeof(lz4ds->hashTable));
	lz4ds->bufferStart = base;
	lz4ds->base = base;
	lz4ds->nextBlock = base;
}

int LZ4_resetStreamState(void* state, const char* inputBuffer)
{
	if ((((size_t)state) & 3) != 0) return 1;   /* Error : pointer is not aligned on 4-bytes boundary */
	LZ4_init((LZ4_Data_Structure*)state, (const BYTE*)inputBuffer);
	return 0;
}

void* LZ4_create (const char* inputBuffer)
{
	void* lz4ds = ALLOCATOR(1, sizeof(LZ4_Data_Structure));
	LZ4_init ((LZ4_Data_Structure*)lz4ds, (const BYTE*)inputBuffer);
	return lz4ds;
}

int LZ4_free (void* LZ4_Data)
{
	FREEMEM(LZ4_Data);
	return (0);
}

char* LZ4_slideInputBuffer (void* LZ4_Data)
{
	LZ4_Data_Structure* lz4ds = (LZ4_Data_Structure*)LZ4_Data;
	size_t delta = lz4ds->nextBlock - (lz4ds->bufferStart + 64 KB);

	if ( (lz4ds->base - delta > lz4ds->base)                          /* underflow control */
	   || ((size_t)(lz4ds->nextBlock - lz4ds->base) > 0xE0000000) )   /* close to 32-bits limit */
	{
		size_t deltaLimit = (lz4ds->nextBlock - 64 KB) - lz4ds->base;
		int nH;

		for (nH=0; nH < HASHNBCELLS4; nH++)
		{
			if ((size_t)(lz4ds->hashTable[nH]) < deltaLimit) lz4ds->hashTable[nH] = 0;
			else lz4ds->hashTable[nH] -= (U32)deltaLimit;
		}
		memcpy((void*)(lz4ds->bufferStart), (const void*)(lz4ds->nextBlock - 64 KB), 64 KB);
		lz4ds->base = lz4ds->bufferStart;
		lz4ds->nextBlock = lz4ds->base + 64 KB;
	}
	else
	{
		memcpy((void*)(lz4ds->bufferStart), (const void*)(lz4ds->nextBlock - 64 KB), 64 KB);
		lz4ds->nextBlock -= delta;
		lz4ds->base -= delta;
	}

	return (char*)(lz4ds->nextBlock);
}

int LZ4_compress_continue (void* LZ4_Data, const char* source, char* dest, int inputSize)
{
	return LZ4_compress_generic(LZ4_Data, source, dest, inputSize, 0, notLimited, byU32, withPrefix);
}

int LZ4_compress_limitedOutput_continue (void* LZ4_Data, const char* source, char* dest, int inputSize, int maxOutputSize)
{
	return LZ4_compress_generic(LZ4_Data, source, dest, inputSize, maxOutputSize, limited, byU32, withPrefix);
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
				 int prefix64k,          /* noPrefix, withPrefix */
				 int partialDecoding,    /* full, partial */
				 int targetOutputSize    /* only used if partialDecoding==partial */
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

	/*const size_t dec32table[] = {0, 3, 2, 3, 0, 0, 0, 0};   / static reduces speed for LZ4_decompress_safe() on GCC64 */
	const size_t dec32table[] = {4-0, 4-3, 4-2, 4-3, 4-0, 4-0, 4-0, 4-0};   /* static reduces speed for LZ4_decompress_safe() on GCC64 */
	static const size_t dec64table[] = {0, 0, 0, (size_t)-1, 0, 1, 2, 3};

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
			unsigned s=255;
			while (((endOnInput)?ip<iend:1) && (s==255))
			{
				s = *ip++;
				length += s;
			}
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
		if ((prefix64k==noPrefix) && (unlikely(ref < (BYTE* const)dest))) goto _output_error;   /* Error : offset outside destination buffer */

		/* get matchlength */
		if ((length=(token&ML_MASK)) == ML_MASK)
		{
			while ((!endOnInput) || (ip<iend-(LASTLITERALS+1)))   /* Ensure enough bytes remain for LASTLITERALS + token */
			{
				unsigned s = *ip++;
				length += s;
				if (s==255) continue;
				break;
			}
		}

		/* copy repeated sequence */
		if (unlikely((op-ref)<(int)STEPSIZE))
		{
			const size_t dec64 = dec64table[(sizeof(void*)==4) ? 0 : op-ref];
			op[0] = ref[0];
			op[1] = ref[1];
			op[2] = ref[2];
			op[3] = ref[3];
			/*op += 4, ref += 4; ref -= dec32table[op-ref];
			A32(op) = A32(ref);
			op += STEPSIZE-4; ref -= dec64;*/
			ref += dec32table[op-ref];
			A32(op+4) = A32(ref);
			op += STEPSIZE; ref -= dec64;
		} else { LZ4_COPYSTEP(op,ref); }
		cpy = op + length - (STEPSIZE-4);

		if (unlikely(cpy>oend-COPYLENGTH-(STEPSIZE-4)))
		{
			if (cpy > oend-LASTLITERALS) goto _output_error;    /* Error : last 5 bytes must be literals */
			LZ4_SECURECOPY(op, ref, (oend-COPYLENGTH));
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

int LZ4_decompress_safe(const char* source, char* dest, int inputSize, int maxOutputSize)
{
	return LZ4_decompress_generic(source, dest, inputSize, maxOutputSize, endOnInputSize, noPrefix, full, 0);
}

int LZ4_decompress_safe_withPrefix64k(const char* source, char* dest, int inputSize, int maxOutputSize)
{
	return LZ4_decompress_generic(source, dest, inputSize, maxOutputSize, endOnInputSize, withPrefix, full, 0);
}

int LZ4_decompress_safe_partial(const char* source, char* dest, int inputSize, int targetOutputSize, int maxOutputSize)
{
	return LZ4_decompress_generic(source, dest, inputSize, maxOutputSize, endOnInputSize, noPrefix, partial, targetOutputSize);
}

int LZ4_decompress_fast_withPrefix64k(const char* source, char* dest, int outputSize)
{
	return LZ4_decompress_generic(source, dest, 0, outputSize, endOnOutputSize, withPrefix, full, 0);
}

int LZ4_decompress_fast(const char* source, char* dest, int outputSize)
{
#ifdef _MSC_VER   /* This version is faster with Visual */
	return LZ4_decompress_generic(source, dest, 0, outputSize, endOnOutputSize, noPrefix, full, 0);
#else
	return LZ4_decompress_generic(source, dest, 0, outputSize, endOnOutputSize, withPrefix, full, 0);
#endif
}

int LZ4_uncompress (const char* source, char* dest, int outputSize) { return LZ4_decompress_fast(source, dest, outputSize); }
int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize) { return LZ4_decompress_safe(source, dest, isize, maxOutputSize); }



#line 1 "shoco.c"
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


#line 1 "shoco.h"
#pragma once

#include <stddef.h>

size_t shoco_compress(const char * const in, size_t len, char * const out, size_t bufsize);
size_t shoco_decompress(const char * const in, size_t len, char * const out, size_t bufsize);

#define _SHOCO_INTERNAL

#line 1 "shoco_model.h"
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

// easylzma

#line 1 "common_internal.c"

#line 1 "common_internal.h"
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



#line 1 "compress.c"

#line 1 "lzma_header.h"
#ifndef __EASYLZMA_LZMA_HEADER__
#define __EASYLZMA_LZMA_HEADER__

/* LZMA-Alone header format gleaned from reading Igor's code */

void initializeLZMAFormatHandler(struct elzma_format_handler * hand);

#endif


#line 1 "lzip_header.h"
#ifndef __EASYLZMA_LZIP_HEADER__
#define __EASYLZMA_LZIP_HEADER__

/* lzip file format documented here:
 * http://download.savannah.gnu.org/releases-noredirect/lzip/manual/ */

void initializeLZIPFormatHandler(struct elzma_format_handler * hand);

#endif


#line 1 "Types.h"
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


#line 1 "LzmaEnc.h"
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


#line 1 "7zCrc.h"
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

	/* "reasonable" defaults for props */
	LzmaEncProps_Init(&(hand->props));
	hand->props.lc = 3;
	hand->props.lp = 0;
	hand->props.pb = 2;
	hand->props.level = 9; // default: 5, max: 9
	hand->props.algo = 1;
	hand->props.fb = 32; // default: 32, max: 273
	hand->props.dictSize = 1 << 22; // default: 1<<24 16MiB
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


#line 1 "decompress.c"

#line 1 "LzmaDec.h"
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


#line 1 "lzip_header.c"
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


#line 1 "lzma_header.c"
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

#line 1 "7zCrc.c"
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



#line 1 "Alloc.c"
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>


#line 1 "Alloc.h"
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


#line 1 "Bra.c"

#line 1 "Bra.h"
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


#line 1 "Bra86.c"
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


#line 1 "BraIA64.c"
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


#line 1 "LzFind.c"
#include <string.h>


#line 1 "LzFind.h"
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


#line 1 "LzHash.h"
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


#line 1 "LzmaDec.c"
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


#line 1 "LzmaEnc.c"
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


#line 1 "LzmaLib.c"

#line 1 "LzmaLib.h"
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

#line 1 "Bcj2.c"

#line 1 "Bcj2.h"
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


#ifdef swap
#undef swap
#endif

#line 1 "bundle.cpp"
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

namespace {
	/* callbacks for streamed input and output */

	struct wrbuf {
		size_t pos;
		const size_t size;
		uint8_t *const data;
	};

	struct rdbuf {
		size_t pos;
		const size_t size;
		const uint8_t *const data;
	};

	size_t elzma_write_callback( void *ctx, const void *buf, size_t size ) {
		wrbuf * f = (wrbuf *) ctx;
		assert( f );

		if( f->pos + size > f->size ) {
			size = f->size - f->pos;
		}

		memcpy( &f->data[ f->pos ], buf, size );
		f->pos += size;

		return size;
	}

	int elzma_read_callback( void *ctx, void *buf, size_t *size ) {
		rdbuf * f = (rdbuf *) ctx;
		assert( f );

		if( f->pos + *size > f->size ) {
			*size = f->size - f->pos;
		}

		memcpy( buf, &f->data[ f->pos ], *size );
		f->pos += *size;

		return 0;
	}

	template<bool is_lzip>
	size_t lzma_decompress( const uint8_t * const data, const size_t size,
						  uint8_t * const new_data, size_t * const out_sizep )
	{
		rdbuf rd = { 0, size, data };
		wrbuf wr = { 0, *out_sizep, new_data };

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

		rdbuf rd = { 0, size, data };
		wrbuf wr = { 0, *out_sizep, new_data };

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
			/* for archival reasons: */
			// break; case LZHAM: return "LZHAM";
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
			/* for archival reasons: */
			// break; case LZHAM: return "lzham";
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
			break; case LZ4: zlen = LZ4_compressBound((int)(len));
		}
		return zlen += MAX_BUNDLE_HEADERS, shout( q, "[bound]", len, zlen ), zlen;
	}

	  bool pack( unsigned q, const void *in, size_t inlen, void *out, size_t &outlen ) {
		bool ok = false;
		if( in && inlen && out && outlen >= inlen ) {
			ok = true;
			switch( q ) {
				break; default: ok = false;
				break; case LZ4: outlen = LZ4_compress( (const char *)in, (char *)out, inlen );
				break; case MINIZ: outlen = tdefl_compress_mem_to_mem( out, outlen, in, inlen, TDEFL_DEFAULT_MAX_PROBES ); //TDEFL_MAX_PROBES_MASK ); //
				break; case SHOCO: outlen = shoco_compress( (const char *)in, inlen, (char *)out, outlen );
				break; case LZMASDK: outlen = lzma_compress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
				break; case LZIP: outlen = lzma_compress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
				/* for archival reasons: */
				// break; case LZHAM: { lzham_z_ulong l; lzham_z_compress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); outlen = l; }
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
				break; case LZ4: bytes_read = LZ4_uncompress( (const char *)in, (char *)out, outlen );
				break; case MINIZ: bytes_read = inlen; tinfl_decompress_mem_to_mem( out, outlen, in, inlen, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF );
				break; case SHOCO: bytes_read = inlen; shoco_decompress( (const char *)in, inlen, (char *)out, outlen );
				break; case LZMASDK: bytes_read = 0; if( lzma_decompress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
				break; case LZIP: bytes_read = 0; if( lzma_decompress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
				/* for archival reasons: */
				// break; case LZHAM: bytes_read = inlen; { lzham_z_ulong l = outlen; lzham_z_uncompress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); }
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
	/*
	bool is_packed( const std::string &self ) {
		return self.size() > 0 && '\0' == self[ 0 ];
	}
	bool is_unpacked( const std::string &self ) {
		return !is_packed( self );
	}
	*/

	unsigned type_of( const std::string &self ) {
		return is_packed( self ) ? self[ 1 ] : NONE;
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

	/*
	std::string pack( unsigned q, const std::string &input ) {
		if( is_packed( input ) )
			return input;

		if( !input.size() )
			return input;

		std::string output( bound( q, input.size() ), '\0' );

		// compress
		size_t len = output.size();
		if( !pack( q, &input[0], input.size(), &output[0], len ) )
			return input;
		output.resize( len );

		// encapsulate
		return std::string() + char(0) + char(q & 0xff) + vlebit(input.size()) + vlebit(output.size()) + output;
	}

	std::string unpack( const std::string &self ) {
		if( is_packed( self ) ) {
			// decapsulate
			unsigned Q = self[1];
			const char *ptr = &self[2];
			size_t size1 = vlebit(ptr);
			size_t size2 = vlebit(ptr);

			std::string output( size1, '\0' );

			// decompress
			size_t len = output.size();
			if( unpack( Q, ptr, size2, &output[0], len ) )
				return output;
		}

		return self;
	} */
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
						break; case EXTRA: quality = MZ_BEST_COMPRESSION;
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


