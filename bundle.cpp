/** this is an amalgamated file. do not edit.
 */

#line 1 "bundle.hpp"
#ifndef BUNDLE_HPP
#define BUNDLE_HPP

#if defined(_MSC_VER) && _MSC_VER < 1700
#define BUNDLE_CXX11 0
#else
#define BUNDLE_CXX11 1
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
	enum { UNDEFINED, SHOCO, LZ4, MINIZ, LZLIB };
	// per family
	enum { NONE = UNDEFINED, ASCII = SHOCO, LZ77 = LZ4, DEFLATE = MINIZ, LZMA = LZLIB };
	// per context
	enum { UNCOMPRESSED = NONE, ENTROPY = ASCII, FAST = LZ77, DEFAULT = DEFLATE, EXTRA = LZMA };

	// dont compress if compression ratio is below 5%
	enum { NO_COMPRESSION_TRESHOLD = 5 };

	// high level API

	bool is_packed( const std::string &self );
	bool is_unpacked( const std::string &self );

	std::string pack( unsigned q, const std::string &self );
	std::string unpack( const std::string &self );

	unsigned typeof( const std::string &self );
	std::string nameof( const std::string &self );
	std::string versionof( const std::string &self );
	std::string extof( const std::string &self );
	size_t length( const std::string &self );
	size_t zlength( const std::string &self );
	void *zptr( const std::string &self );

	// low level API

	  bool pack( unsigned q, const char *in, size_t len, char *out, size_t &zlen );
	  bool unpack( unsigned q, const char *in, size_t len, char *out, size_t &zlen );
	size_t bound( unsigned q, size_t len );
	const char *const nameof( unsigned q );
	const char *const versionof( unsigned q );
	const char *const extof( unsigned q );
	unsigned typeof( const void *mem, size_t size );

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
		bool result = pack( q, (const char *)&buffer_in.at(0), buffer_in.size(), (char *)&buffer_out.at(0), zlen );

		// resize properly
		return result ? ( buffer_out.resize( zlen ), true ) : ( buffer_out = T2(), false );
	}

	template < class T1, class T2 >
	static inline bool unpack( unsigned q, T2 &buffer_out, const T1 &buffer_in) {
		// sanity checks
		assert( sizeof(buffer_in.at(0)) == 1 && "size of input elements != 1" );
		assert( sizeof(buffer_out.at(0)) == 1 && "size of output elements != 1" );

		// note: buffer_out must be resized properly before calling this function!!
		size_t zlen = buffer_out.size();
		return unpack( q, (const char *)&buffer_in.at(0), buffer_in.size(), (char *)&buffer_out.at(0), zlen );
	}

	static inline std::vector<unsigned> encodings() {
		static std::vector<unsigned> all;
		if( all.empty() ) {
			all.push_back( LZ4 );
			all.push_back( SHOCO );
			all.push_back( MINIZ );
			all.push_back( LZLIB );
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
			ss << ( pass ? "[ OK ] " : "[FAIL] ") << nameof(q) << ": ratio=" << ratio << "% enctime=" << enctime << "ms dectime=" << dectime << " ms";
			return ss.str();
		}
	};

	template< class T, bool do_enc = true, bool do_dec = true, bool do_verify = true >
	std::vector< measure > measures( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
		std::vector< measure > results;
		std::string zipped, unzipped;

		for( auto scheme : encodings ) {
			results.push_back( measure() );
			auto &r = results.back();
			r.q = scheme;

			if( do_enc ) {
				auto begin = std::chrono::high_resolution_clock::now();
				zipped = pack(scheme, original);
				auto end = std::chrono::high_resolution_clock::now();
				r.enctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
				r.ratio = 100 - 100 * ( double( zipped.size() ) / original.size() );
			}

			if( do_dec ) {
				auto begin = std::chrono::high_resolution_clock::now();
				unzipped = unpack(zipped);
				auto end = std::chrono::high_resolution_clock::now();
				r.dectime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
				r.pass = ( original == unzipped );
			}

			r.pass = ( do_verify && do_enc && do_dec && is_packed(zipped) && is_unpacked(unzipped) ? original == unzipped : true );
		}

		return results;
	}

	// find best choice for given data
	template< class T >
	unsigned find_smallest_compressor( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double ratio = 0;

		auto results = measures< true, false, false >( original, encodings );
		for( auto &r : results ) {
			if( r.pass && r.ratio > ratio && r.ratio >= (100 - NO_COMPRESSION_TRESHOLD / 100.0) ) {
				ratio = r.ratio;
				q = r.q;
			}
		}

		return q;
	}

	template< class T >
	unsigned find_fastest_compressor( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double enctime = 9999999;

		auto results = measures< true, false, false >( original, encodings );
		for( auto &r : results ) {
			if( r.pass && r.enctime < enctime ) {
				enctime = r.enctime;
				q = r.q;
			}
		}

		return q;
	}

	template< class T >
	unsigned find_fastest_decompressor( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
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
			std::string t[] = { string(), string(t0), string(t1) };
			for( const char *fmt = _fmt.c_str(); *fmt; ++fmt ) {
				/**/ if( *fmt == '\1' ) t[0] += t[1];
				else if( *fmt == '\2' ) t[0] += t[2];
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

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
#define MINIZ_HAS_64BIT_REGISTERS 1

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


//#define mz_crc32 mz_crc32_lz

#line 1 "lzlib.c"
#if defined(_MSC_VER) && _MSC_VER < 1700
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int int64_t;
#else
#include <stdbool.h>
#include <stdint.h>
#endif
#include <stdlib.h>
#include <string.h>


#line 1 "lzlib.h"
#ifdef __cplusplus
extern "C" {
#endif

static const char * const LZ_version_string = "1.5";

enum LZ_Errno { LZ_ok = 0,         LZ_bad_argument, LZ_mem_error,
				LZ_sequence_error, LZ_header_error, LZ_unexpected_eof,
				LZ_data_error,     LZ_library_error };

const char * LZ_version( void );
const char * LZ_strerror( const enum LZ_Errno lz_errno );

int LZ_min_dictionary_bits( void );
int LZ_min_dictionary_size( void );
int LZ_max_dictionary_bits( void );
int LZ_max_dictionary_size( void );
int LZ_min_match_len_limit( void );
int LZ_max_match_len_limit( void );

/*---------------------- Compression Functions ----------------------*/

struct LZ_Encoder;

struct LZ_Encoder * LZ_compress_open( const int dictionary_size,
									  const int match_len_limit,
									  const unsigned long long member_size );
int LZ_compress_close( struct LZ_Encoder * const encoder );

int LZ_compress_finish( struct LZ_Encoder * const encoder );
int LZ_compress_restart_member( struct LZ_Encoder * const encoder,
								const unsigned long long member_size );
int LZ_compress_sync_flush( struct LZ_Encoder * const encoder );

int LZ_compress_read( struct LZ_Encoder * const encoder,
					  uint8_t * const buffer, const int size );
int LZ_compress_write( struct LZ_Encoder * const encoder,
					   const uint8_t * const buffer, const int size );
int LZ_compress_write_size( struct LZ_Encoder * const encoder );

enum LZ_Errno LZ_compress_errno( struct LZ_Encoder * const encoder );
int LZ_compress_finished( struct LZ_Encoder * const encoder );
int LZ_compress_member_finished( struct LZ_Encoder * const encoder );

unsigned long long LZ_compress_data_position( struct LZ_Encoder * const encoder );
unsigned long long LZ_compress_member_position( struct LZ_Encoder * const encoder );
unsigned long long LZ_compress_total_in_size( struct LZ_Encoder * const encoder );
unsigned long long LZ_compress_total_out_size( struct LZ_Encoder * const encoder );

/*--------------------- Decompression Functions ---------------------*/

struct LZ_Decoder;

struct LZ_Decoder * LZ_decompress_open( void );
int LZ_decompress_close( struct LZ_Decoder * const decoder );

int LZ_decompress_finish( struct LZ_Decoder * const decoder );
int LZ_decompress_reset( struct LZ_Decoder * const decoder );
int LZ_decompress_sync_to_member( struct LZ_Decoder * const decoder );

int LZ_decompress_read( struct LZ_Decoder * const decoder,
						uint8_t * const buffer, const int size );
int LZ_decompress_write( struct LZ_Decoder * const decoder,
						 const uint8_t * const buffer, const int size );
int LZ_decompress_write_size( struct LZ_Decoder * const decoder );

enum LZ_Errno LZ_decompress_errno( struct LZ_Decoder * const decoder );
int LZ_decompress_finished( struct LZ_Decoder * const decoder );
int LZ_decompress_member_finished( struct LZ_Decoder * const decoder );

int LZ_decompress_member_version( struct LZ_Decoder * const decoder );
int LZ_decompress_dictionary_size( struct LZ_Decoder * const decoder );
unsigned LZ_decompress_data_crc( struct LZ_Decoder * const decoder );

unsigned long long LZ_decompress_data_position( struct LZ_Decoder * const decoder );
unsigned long long LZ_decompress_member_position( struct LZ_Decoder * const decoder );
unsigned long long LZ_decompress_total_in_size( struct LZ_Decoder * const decoder );
unsigned long long LZ_decompress_total_out_size( struct LZ_Decoder * const decoder );

#ifdef __cplusplus
}
#endif


#line 1 "lzip.h"
#ifndef max
  #define max(x,y) ((x) >= (y) ? (x) : (y))
#endif
#ifndef min
  #define min(x,y) ((x) <= (y) ? (x) : (y))
#endif

typedef int State;

enum { states = 12 };

static inline bool St_is_char( const State st ) { return st < 7; }

static inline State St_set_char( const State st )
  {
  static const State next[states] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5 };
  return next[st];
  }

static inline State St_set_match( const State st )
  { return ( ( st < 7 ) ? 7 : 10 ); }

static inline State St_set_rep( const State st )
  { return ( ( st < 7 ) ? 8 : 11 ); }

static inline State St_set_short_rep( const State st )
  { return ( ( st < 7 ) ? 9 : 11 ); }

enum {
  min_dictionary_bits = 12,
  min_dictionary_size = 1 << min_dictionary_bits,
  max_dictionary_bits = 29,
  max_dictionary_size = 1 << max_dictionary_bits,
  literal_context_bits = 3,
  pos_state_bits = 2,
  pos_states = 1 << pos_state_bits,
  pos_state_mask = pos_states - 1,

  dis_slot_bits = 6,
  start_dis_model = 4,
  end_dis_model = 14,
  modeled_distances = 1 << (end_dis_model / 2),		/* 128 */
  dis_align_bits = 4,
  dis_align_size = 1 << dis_align_bits,

  len_low_bits = 3,
  len_mid_bits = 3,
  len_high_bits = 8,
  len_low_symbols = 1 << len_low_bits,
  len_mid_symbols = 1 << len_mid_bits,
  len_high_symbols = 1 << len_high_bits,
  max_len_symbols = len_low_symbols + len_mid_symbols + len_high_symbols,

  min_match_len = 2,					/* must be 2 */
  max_match_len = min_match_len + max_len_symbols - 1,	/* 273 */
  min_match_len_limit = 5,

  dis_states = 4 };

static inline int get_dis_state( const int len )
  { return min( len - min_match_len, dis_states - 1 ); }

static inline int get_lit_state( const uint8_t prev_byte )
  { return ( prev_byte >> ( 8 - literal_context_bits ) ); }

enum { bit_model_move_bits = 5,
	   bit_model_total_bits = 11,
	   bit_model_total = 1 << bit_model_total_bits };

typedef int Bit_model;

static inline void Bm_init( Bit_model * const probability )
  { *probability = bit_model_total / 2; }

static inline void Bm_array_init( Bit_model * const p, const int size )
  { int i = 0; while( i < size ) p[i++] = bit_model_total / 2; }

struct Len_model
  {
  Bit_model choice1;
  Bit_model choice2;
  Bit_model bm_low[pos_states][len_low_symbols];
  Bit_model bm_mid[pos_states][len_mid_symbols];
  Bit_model bm_high[len_high_symbols];
  };

static inline void Lm_init( struct Len_model * const lm )
  {
  Bm_init( &lm->choice1 );
  Bm_init( &lm->choice2 );
  Bm_array_init( lm->bm_low[0], pos_states * len_low_symbols );
  Bm_array_init( lm->bm_mid[0], pos_states * len_mid_symbols );
  Bm_array_init( lm->bm_high, len_high_symbols );
  }

/* Table of CRCs of all 8-bit messages. */
static const uint32_t crc32[256] =
  {
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
  0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
  0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
  0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
  0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
  0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
  0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
  0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
  0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
  0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
  0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
  0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
  0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
  0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
  0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
  0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
  0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
  0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
  0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
  0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
  0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
  0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
  0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

static inline void CRC32_update_byte( uint32_t * const crc, const uint8_t byte )
  { *crc = crc32[(*crc^byte)&0xFF] ^ ( *crc >> 8 ); }

static inline void CRC32_update_buf( uint32_t * const crc,
									 const uint8_t * const buffer, const int size )
  {
  int i;
  for( i = 0; i < size; ++i )
	*crc = crc32[(*crc^buffer[i])&0xFF] ^ ( *crc >> 8 );
  }

static inline int real_bits( unsigned value )
  {
  int bits = 0;
  while( value > 0 ) { value >>= 1; ++bits; }
  return bits;
  }

static const uint8_t magic_string[4] = { 0x4C, 0x5A, 0x49, 0x50 }; /* "LZIP" */

typedef uint8_t File_header[6];		/* 0-3 magic bytes */
					/*   4 version */
					/*   5 coded_dict_size */
enum { Fh_size = 6 };

static inline void Fh_set_magic( File_header data )
  { memcpy( data, magic_string, 4 ); data[4] = 1; }

static inline bool Fh_verify_magic( const File_header data )
  { return ( memcmp( data, magic_string, 4 ) == 0 ); }

static inline uint8_t Fh_version( const File_header data )
  { return data[4]; }

static inline bool Fh_verify_version( const File_header data )
  { return ( data[4] == 1 ); }

static inline unsigned Fh_get_dictionary_size( const File_header data )
  {
  unsigned sz = ( 1 << ( data[5] & 0x1F ) );
  if( sz > min_dictionary_size )
	sz -= ( sz / 16 ) * ( ( data[5] >> 5 ) & 7 );
  return sz;
  }

static inline bool Fh_set_dictionary_size( File_header data, const unsigned sz )
  {
  if( sz >= min_dictionary_size && sz <= max_dictionary_size )
	{
	data[5] = real_bits( sz - 1 );
	if( sz > min_dictionary_size )
	  {
	  const unsigned base_size = 1 << data[5];
	  const unsigned wedge = base_size / 16;
	  int i;
	  for( i = 7; i >= 1; --i )
		if( base_size - ( i * wedge ) >= sz )
		  { data[5] |= ( i << 5 ); break; }
	  }
	return true;
	}
  return false;
  }

static inline bool Fh_verify( const File_header data )
  {
  return ( Fh_verify_magic( data ) && Fh_verify_version( data ) &&
		   Fh_get_dictionary_size( data ) >= min_dictionary_size &&
		   Fh_get_dictionary_size( data ) <= max_dictionary_size );
  }

typedef uint8_t File_trailer[20];
			/*  0-3  CRC32 of the uncompressed data */
			/*  4-11 size of the uncompressed data */
			/* 12-19 member size including header and trailer */

enum { Ft_size = 20 };

static inline unsigned Ft_get_data_crc( const File_trailer data )
  {
  unsigned tmp = 0;
  int i;
  for( i = 3; i >= 0; --i ) { tmp <<= 8; tmp += data[i]; }
  return tmp;
  }

static inline void Ft_set_data_crc( File_trailer data, unsigned crc )
  {
  int i;
  for( i = 0; i <= 3; ++i ) { data[i] = (uint8_t)crc; crc >>= 8; }
  }

static inline unsigned long long Ft_get_data_size( const File_trailer data )
  {
  unsigned long long tmp = 0;
  int i;
  for( i = 11; i >= 4; --i ) { tmp <<= 8; tmp += data[i]; }
  return tmp;
  }

static inline void Ft_set_data_size( File_trailer data, unsigned long long sz )
  {
  int i;
  for( i = 4; i <= 11; ++i ) { data[i] = (uint8_t)sz; sz >>= 8; }
  }

static inline unsigned long long Ft_get_member_size( const File_trailer data )
  {
  unsigned long long tmp = 0;
  int i;
  for( i = 19; i >= 12; --i ) { tmp <<= 8; tmp += data[i]; }
  return tmp;
  }

static inline void Ft_set_member_size( File_trailer data, unsigned long long sz )
  {
  int i;
  for( i = 12; i <= 19; ++i ) { data[i] = (uint8_t)sz; sz >>= 8; }
  }


#line 1 "cbuffer.c"
struct Circular_buffer
  {
  uint8_t * buffer;
  int buffer_size;		/* capacity == buffer_size - 1 */
  int get;			/* buffer is empty when get == put */
  int put;
  };

static inline void Cb_reset( struct Circular_buffer * const cb )
  { cb->get = 0; cb->put = 0; }

static inline bool Cb_init( struct Circular_buffer * const cb,
							const int buf_size )
  {
  cb->buffer = (uint8_t *)malloc( buf_size + 1 );
  cb->buffer_size = buf_size + 1;
  cb->get = 0;
  cb->put = 0;
  return ( cb->buffer != 0 );
  }

static inline void Cb_free( struct Circular_buffer * const cb )
  { free( cb->buffer ); cb->buffer = 0; }

static inline int Cb_used_bytes( const struct Circular_buffer * const cb )
	{ return ( (cb->get <= cb->put) ? 0 : cb->buffer_size ) + cb->put - cb->get; }

static inline int Cb_free_bytes( const struct Circular_buffer * const cb )
	{ return ( (cb->get <= cb->put) ? cb->buffer_size : 0 ) - cb->put + cb->get - 1; }

static inline uint8_t Cb_get_byte( struct Circular_buffer * const cb )
	{
	const uint8_t b = cb->buffer[cb->get];
	if( ++cb->get >= cb->buffer_size ) cb->get = 0;
	return b;
	}

static inline void Cb_put_byte( struct Circular_buffer * const cb,
								const uint8_t b )
	{
	cb->buffer[cb->put] = b;
	if( ++cb->put >= cb->buffer_size ) cb->put = 0;
	}

/* Copies up to 'out_size' bytes to 'out_buffer' and updates 'get'.
   Returns the number of bytes copied.
*/
static int Cb_read_data( struct Circular_buffer * const cb,
						 uint8_t * const out_buffer, const int out_size )
  {
  int size = 0;
  if( out_size < 0 ) return 0;
  if( cb->get > cb->put )
	{
	size = min( cb->buffer_size - cb->get, out_size );
	if( size > 0 )
	  {
	  memcpy( out_buffer, cb->buffer + cb->get, size );
	  cb->get += size;
	  if( cb->get >= cb->buffer_size ) cb->get = 0;
	  }
	}
  if( cb->get < cb->put )
	{
	const int size2 = min( cb->put - cb->get, out_size - size );
	if( size2 > 0 )
	  {
	  memcpy( out_buffer + size, cb->buffer + cb->get, size2 );
	  cb->get += size2;
	  size += size2;
	  }
	}
  return size;
  }

/* Copies up to 'in_size' bytes from 'in_buffer' and updates 'put'.
   Returns the number of bytes copied.
*/
static int Cb_write_data( struct Circular_buffer * const cb,
						  const uint8_t * const in_buffer, const int in_size )
  {
  int size = 0;
  if( in_size < 0 ) return 0;
  if( cb->put >= cb->get )
	{
	size = min( cb->buffer_size - cb->put - (cb->get == 0), in_size );
	if( size > 0 )
	  {
	  memcpy( cb->buffer + cb->put, in_buffer, size );
	  cb->put += size;
	  if( cb->put >= cb->buffer_size ) cb->put = 0;
	  }
	}
  if( cb->put < cb->get )
	{
	const int size2 = min( cb->get - cb->put - 1, in_size - size );
	if( size2 > 0 )
	  {
	  memcpy( cb->buffer + cb->put, in_buffer + size, size2 );
	  cb->put += size2;
	  size += size2;
	  }
	}
  return size;
  }


#line 1 "decoder.h"
enum { rd_min_available_bytes = 8 };

struct Range_decoder
  {
  struct Circular_buffer cb;		/* input buffer */
  unsigned long long member_position;
  uint32_t code;
  uint32_t range;
  bool at_stream_end;
  bool reload_pending;
  };

static inline bool Rd_init( struct Range_decoder * const rdec )
  {
  if( !Cb_init( &rdec->cb, 65536 + rd_min_available_bytes ) ) return false;
  rdec->member_position = 0;
  rdec->code = 0;
  rdec->range = 0xFFFFFFFFU;
  rdec->at_stream_end = false;
  rdec->reload_pending = false;
  return true;
  }

static inline void Rd_free( struct Range_decoder * const rdec )
  { Cb_free( &rdec->cb ); }

static inline bool Rd_finished( const struct Range_decoder * const rdec )
  { return rdec->at_stream_end && !Cb_used_bytes( &rdec->cb ); }

static inline void Rd_finish( struct Range_decoder * const rdec )
  { rdec->at_stream_end = true; }

static inline bool Rd_enough_available_bytes( const struct Range_decoder * const rdec )
  {
  return ( Cb_used_bytes( &rdec->cb ) >= rd_min_available_bytes ||
		   ( rdec->at_stream_end && Cb_used_bytes( &rdec->cb ) > 0 ) );
  }

static inline int Rd_available_bytes( const struct Range_decoder * const rdec )
  { return Cb_used_bytes( &rdec->cb ); }

static inline int Rd_free_bytes( const struct Range_decoder * const rdec )
  { if( rdec->at_stream_end ) return 0; return Cb_free_bytes( &rdec->cb ); }

static inline void Rd_purge( struct Range_decoder * const rdec )
  { rdec->at_stream_end = true; Cb_reset( &rdec->cb ); }

static inline void Rd_reset( struct Range_decoder * const rdec )
  { rdec->at_stream_end = false; Cb_reset( &rdec->cb ); }

/* Seeks a member header and updates 'get'.
   Returns true if it finds a valid header.
*/
static bool Rd_find_header( struct Range_decoder * const rdec )
  {
  while( rdec->cb.get != rdec->cb.put )
	{
	if( rdec->cb.buffer[rdec->cb.get] == magic_string[0] )
	  {
	  int get = rdec->cb.get;
	  int i;
	  File_header header;
	  for( i = 0; i < Fh_size; ++i )
		{
		if( get == rdec->cb.put ) return false;	/* not enough data */
		header[i] = rdec->cb.buffer[get];
		if( ++get >= rdec->cb.buffer_size ) get = 0;
		}
	  if( Fh_verify( header ) ) return true;
	  }
	if( ++rdec->cb.get >= rdec->cb.buffer_size ) rdec->cb.get = 0;
	}
  return false;
  }

/* Returns true, fills 'header', and updates 'get' if 'get' points to a
   valid header.
   Else returns false and leaves 'get' unmodified.
*/
static bool Rd_read_header( struct Range_decoder * const rdec,
							File_header header )
  {
  int get = rdec->cb.get;
  int i;
  for( i = 0; i < Fh_size; ++i )
	{
	if( get == rdec->cb.put ) return false;	/* not enough data */
	header[i] = rdec->cb.buffer[get];
	if( ++get >= rdec->cb.buffer_size ) get = 0;
	}
  if( Fh_verify( header ) )
	{
	rdec->cb.get = get;
	rdec->member_position = Fh_size;
	rdec->reload_pending = true;
	return true;
	}
  return false;
  }

static inline int Rd_write_data( struct Range_decoder * const rdec,
								 const uint8_t * const inbuf, const int size )
  {
  if( rdec->at_stream_end || size <= 0 ) return 0;
  return Cb_write_data( &rdec->cb, inbuf, size );
  }

static inline uint8_t Rd_get_byte( struct Range_decoder * const rdec )
  {
  ++rdec->member_position;
  return Cb_get_byte( &rdec->cb );
  }

static inline int Rd_read_data( struct Range_decoder * const rdec,
								uint8_t * const outbuf, const int size )
  {
  const int sz = Cb_read_data( &rdec->cb, outbuf, size );
  if( sz > 0 ) rdec->member_position += sz;
  return sz;
  }

static bool Rd_try_reload( struct Range_decoder * const rdec, const bool force )
  {
  if( force ) rdec->reload_pending = true;
  if( rdec->reload_pending && Rd_available_bytes( rdec ) >= 5 )
	{
	int i;
	rdec->reload_pending = false;
	rdec->code = 0;
	for( i = 0; i < 5; ++i )
	  rdec->code = (rdec->code << 8) | Rd_get_byte( rdec );
	rdec->range = 0xFFFFFFFFU;
	}
  return !rdec->reload_pending;
  }

static inline void Rd_normalize( struct Range_decoder * const rdec )
  {
  if( rdec->range <= 0x00FFFFFFU )
	{
	rdec->range <<= 8;
	rdec->code = (rdec->code << 8) | Rd_get_byte( rdec );
	}
  }

static inline int Rd_decode( struct Range_decoder * const rdec,
							 const int num_bits )
  {
  int symbol = 0;
  int i;
  for( i = num_bits; i > 0; --i )
	{
	uint32_t mask;
	Rd_normalize( rdec );
	rdec->range >>= 1;
/*    symbol <<= 1; */
/*    if( rdec->code >= rdec->range ) { rdec->code -= rdec->range; symbol |= 1; } */
	mask = 0U - (rdec->code < rdec->range);
	rdec->code -= rdec->range;
	rdec->code += rdec->range & mask;
	symbol = (symbol << 1) + (mask + 1);
	}
  return symbol;
  }

static inline int Rd_decode_bit( struct Range_decoder * const rdec,
								 Bit_model * const probability )
  {
  uint32_t bound;
  Rd_normalize( rdec );
  bound = ( rdec->range >> bit_model_total_bits ) * *probability;
  if( rdec->code < bound )
	{
	rdec->range = bound;
	*probability += (bit_model_total - *probability) >> bit_model_move_bits;
	return 0;
	}
  else
	{
	rdec->range -= bound;
	rdec->code -= bound;
	*probability -= *probability >> bit_model_move_bits;
	return 1;
	}
  }

static inline int Rd_decode_tree( struct Range_decoder * const rdec,
								  Bit_model bm[], const int num_bits )
  {
  int symbol = 1;
  int i;
  for( i = num_bits; i > 0; --i )
	symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  return symbol - (1 << num_bits);
  }

static inline int Rd_decode_tree6( struct Range_decoder * const rdec,
								   Bit_model bm[] )
  {
  int symbol = 1;
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  return symbol - (1 << 6);
  }

static inline int Rd_decode_tree_reversed( struct Range_decoder * const rdec,
										   Bit_model bm[], const int num_bits )
  {
  int model = 1;
  int symbol = 0;
  int i;
  for( i = 0; i < num_bits; ++i )
	{
	const bool bit = Rd_decode_bit( rdec, &bm[model] );
	model <<= 1;
	if( bit ) { ++model; symbol |= (1 << i); }
	}
  return symbol;
  }

static inline int Rd_decode_tree_reversed4( struct Range_decoder * const rdec,
											Bit_model bm[] )
  {
  int model = 1;
  int symbol = 0;
  int bit = Rd_decode_bit( rdec, &bm[model] );
  model = (model << 1) + bit; symbol |= bit;
  bit = Rd_decode_bit( rdec, &bm[model] );
  model = (model << 1) + bit; symbol |= (bit << 1);
  bit = Rd_decode_bit( rdec, &bm[model] );
  model = (model << 1) + bit; symbol |= (bit << 2);
  if( Rd_decode_bit( rdec, &bm[model] ) ) symbol |= 8;
  return symbol;
  }

static inline int Rd_decode_matched( struct Range_decoder * const rdec,
									 Bit_model bm[], int match_byte )
  {
  Bit_model * const bm1 = bm + 0x100;
  int symbol = 1;
  int i;
  for( i = 7; i >= 0; --i )
	{
	int match_bit, bit;
	match_byte <<= 1;
	match_bit = match_byte & 0x100;
	bit = Rd_decode_bit( rdec, &bm1[match_bit+symbol] );
	symbol = ( symbol << 1 ) | bit;
	if( match_bit != bit << 8 )
	  {
	  while( symbol < 0x100 )
		symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
	  break;
	  }
	}
  return symbol - 0x100;
  }

static inline int Rd_decode_len( struct Range_decoder * const rdec,
								 struct Len_model * const lm,
								 const int pos_state )
  {
  if( Rd_decode_bit( rdec, &lm->choice1 ) == 0 )
	return Rd_decode_tree( rdec, lm->bm_low[pos_state], len_low_bits );
  if( Rd_decode_bit( rdec, &lm->choice2 ) == 0 )
	return len_low_symbols +
		   Rd_decode_tree( rdec, lm->bm_mid[pos_state], len_mid_bits );
  return len_low_symbols + len_mid_symbols +
		 Rd_decode_tree( rdec, lm->bm_high, len_high_bits );
  }

enum { lzd_min_free_bytes = max_match_len };

struct LZ_decoder
  {
  struct Circular_buffer cb;
  unsigned long long partial_data_pos;
  int dictionary_size;
  uint32_t crc;
  bool member_finished;
  bool verify_trailer_pending;
  unsigned rep0;		/* rep[0-3] latest four distances */
  unsigned rep1;		/* used for efficient coding of */
  unsigned rep2;		/* repeated distances */
  unsigned rep3;
  State state;

  Bit_model bm_literal[1<<literal_context_bits][0x300];
  Bit_model bm_match[states][pos_states];
  Bit_model bm_rep[states];
  Bit_model bm_rep0[states];
  Bit_model bm_rep1[states];
  Bit_model bm_rep2[states];
  Bit_model bm_len[states][pos_states];
  Bit_model bm_dis_slot[dis_states][1<<dis_slot_bits];
  Bit_model bm_dis[modeled_distances-end_dis_model];
  Bit_model bm_align[dis_align_size];

  struct Range_decoder * rdec;
  struct Len_model match_len_model;
  struct Len_model rep_len_model;
  };

static inline bool LZd_enough_free_bytes( const struct LZ_decoder * const decoder )
  { return Cb_free_bytes( &decoder->cb ) >= lzd_min_free_bytes; }

static inline uint8_t LZd_get_prev_byte( const struct LZ_decoder * const decoder )
  {
  const int i =
	( ( decoder->cb.put > 0 ) ? decoder->cb.put : decoder->cb.buffer_size ) - 1;
  return decoder->cb.buffer[i];
  }

static inline uint8_t LZd_get_byte( const struct LZ_decoder * const decoder,
									const int distance )
  {
  int i = decoder->cb.put - distance - 1;
  if( i < 0 ) i += decoder->cb.buffer_size;
  return decoder->cb.buffer[i];
  }

static inline void LZd_put_byte( struct LZ_decoder * const decoder,
								 const uint8_t b )
  {
  CRC32_update_byte( &decoder->crc, b );
  decoder->cb.buffer[decoder->cb.put] = b;
  if( ++decoder->cb.put >= decoder->cb.buffer_size )
	{ decoder->partial_data_pos += decoder->cb.put; decoder->cb.put = 0; }
  }

static inline void LZd_copy_block( struct LZ_decoder * const decoder,
								   const int distance, int len )
  {
  int i = decoder->cb.put - distance - 1;
  if( i < 0 ) i += decoder->cb.buffer_size;
  if( len < decoder->cb.buffer_size - max( decoder->cb.put, i ) &&
	  len <= abs( decoder->cb.put - i ) )	/* no wrap, no overlap */
	{
	CRC32_update_buf( &decoder->crc, decoder->cb.buffer + i, len );
	memcpy( decoder->cb.buffer + decoder->cb.put, decoder->cb.buffer + i, len );
	decoder->cb.put += len;
	}
  else for( ; len > 0; --len )
	{
	LZd_put_byte( decoder, decoder->cb.buffer[i] );
	if( ++i >= decoder->cb.buffer_size ) i = 0;
	}
  }

static inline bool LZd_init( struct LZ_decoder * const decoder,
							 const File_header header,
							 struct Range_decoder * const rde )
  {
  decoder->dictionary_size = Fh_get_dictionary_size( header );
  if( !Cb_init( &decoder->cb, max( 65536, decoder->dictionary_size ) + lzd_min_free_bytes ) )
	return false;
  decoder->partial_data_pos = 0;
  decoder->crc = 0xFFFFFFFFU;
  decoder->member_finished = false;
  decoder->verify_trailer_pending = false;
  decoder->rep0 = 0;
  decoder->rep1 = 0;
  decoder->rep2 = 0;
  decoder->rep3 = 0;
  decoder->state = 0;

  Bm_array_init( decoder->bm_literal[0], (1 << literal_context_bits) * 0x300 );
  Bm_array_init( decoder->bm_match[0], states * pos_states );
  Bm_array_init( decoder->bm_rep, states );
  Bm_array_init( decoder->bm_rep0, states );
  Bm_array_init( decoder->bm_rep1, states );
  Bm_array_init( decoder->bm_rep2, states );
  Bm_array_init( decoder->bm_len[0], states * pos_states );
  Bm_array_init( decoder->bm_dis_slot[0], dis_states * (1 << dis_slot_bits) );
  Bm_array_init( decoder->bm_dis, modeled_distances - end_dis_model );
  Bm_array_init( decoder->bm_align, dis_align_size );

  decoder->rdec = rde;
  Lm_init( &decoder->match_len_model );
  Lm_init( &decoder->rep_len_model );
  decoder->cb.buffer[decoder->cb.buffer_size-1] = 0; /* prev_byte of first_byte */
  return true;
  }

static inline void LZd_free( struct LZ_decoder * const decoder )
  { Cb_free( &decoder->cb ); }

static inline bool LZd_member_finished( const struct LZ_decoder * const decoder )
  { return ( decoder->member_finished && !Cb_used_bytes( &decoder->cb ) ); }

static inline unsigned LZd_crc( const struct LZ_decoder * const decoder )
  { return decoder->crc ^ 0xFFFFFFFFU; }

static inline unsigned long long
LZd_data_position( const struct LZ_decoder * const decoder )
  { return decoder->partial_data_pos + decoder->cb.put; }


#line 1 "decoder.c"
static bool LZd_verify_trailer( struct LZ_decoder * const decoder )
  {
  File_trailer trailer;
  const unsigned long long member_size =
	decoder->rdec->member_position + Ft_size;

  int size = Rd_read_data( decoder->rdec, trailer, Ft_size );
  if( size < Ft_size )
	return false;

  return ( decoder->rdec->code == 0 &&
		   Ft_get_data_crc( trailer ) == LZd_crc( decoder ) &&
		   Ft_get_data_size( trailer ) == LZd_data_position( decoder ) &&
		   Ft_get_member_size( trailer ) == member_size );
  }

/* Return value: 0 = OK, 1 = decoder error, 2 = unexpected EOF,
				 3 = trailer error, 4 = unknown marker found. */
static int LZd_decode_member( struct LZ_decoder * const decoder )
  {
  struct Range_decoder * const rdec = decoder->rdec;
  State * const state = &decoder->state;

  if( decoder->member_finished ) return 0;
  if( !Rd_try_reload( rdec, false ) ) return 0;
  if( decoder->verify_trailer_pending )
	{
	if( Rd_available_bytes( rdec ) < Ft_size && !rdec->at_stream_end )
	  return 0;
	decoder->verify_trailer_pending = false;
	decoder->member_finished = true;
	if( LZd_verify_trailer( decoder ) ) return 0; else return 3;
	}

  while( !Rd_finished( rdec ) )
	{
	const int pos_state = LZd_data_position( decoder ) & pos_state_mask;
	if( !Rd_enough_available_bytes( rdec ) ||
		!LZd_enough_free_bytes( decoder ) )
	  return 0;
	if( Rd_decode_bit( rdec, &decoder->bm_match[*state][pos_state] ) == 0 )	/* 1st bit */
	  {
	  const uint8_t prev_byte = LZd_get_prev_byte( decoder );
	  if( St_is_char( *state ) )
		{
		*state -= ( *state < 4 ) ? *state : 3;
		LZd_put_byte( decoder, Rd_decode_tree( rdec,
					  decoder->bm_literal[get_lit_state(prev_byte)], 8 ) );
		}
	  else
		{
		*state -= ( *state < 10 ) ? 3 : 6;
		LZd_put_byte( decoder, Rd_decode_matched( rdec,
					  decoder->bm_literal[get_lit_state(prev_byte)],
					  LZd_get_byte( decoder, decoder->rep0 ) ) );
		}
	  }
	else
	  {
	  int len;
	  if( Rd_decode_bit( rdec, &decoder->bm_rep[*state] ) == 1 )	/* 2nd bit */
		{
		if( Rd_decode_bit( rdec, &decoder->bm_rep0[*state] ) == 1 )	/* 3rd bit */
		  {
		  unsigned distance;
		  if( Rd_decode_bit( rdec, &decoder->bm_rep1[*state] ) == 0 )	/* 4th bit */
			distance = decoder->rep1;
		  else
			{
			if( Rd_decode_bit( rdec, &decoder->bm_rep2[*state] ) == 0 )	/* 5th bit */
			  distance = decoder->rep2;
			else
			  { distance = decoder->rep3; decoder->rep3 = decoder->rep2; }
			decoder->rep2 = decoder->rep1;
			}
		  decoder->rep1 = decoder->rep0;
		  decoder->rep0 = distance;
		  }
		else
		  {
		  if( Rd_decode_bit( rdec, &decoder->bm_len[*state][pos_state] ) == 0 )	/* 4th bit */
			{ *state = St_set_short_rep( *state );
			  LZd_put_byte( decoder, LZd_get_byte( decoder, decoder->rep0 ) ); continue; }
		  }
		*state = St_set_rep( *state );
		len = min_match_len + Rd_decode_len( rdec, &decoder->rep_len_model, pos_state );
		}
	  else
		{
		int dis_slot;
		const unsigned rep0_saved = decoder->rep0;
		len = min_match_len + Rd_decode_len( rdec, &decoder->match_len_model, pos_state );
		dis_slot = Rd_decode_tree6( rdec, decoder->bm_dis_slot[get_dis_state(len)] );
		if( dis_slot < start_dis_model ) decoder->rep0 = dis_slot;
		else
		  {
		  const int direct_bits = ( dis_slot >> 1 ) - 1;
		  decoder->rep0 = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
		  if( dis_slot < end_dis_model )
			decoder->rep0 += Rd_decode_tree_reversed( rdec,
							 decoder->bm_dis + decoder->rep0 - dis_slot - 1,
							 direct_bits );
		  else
			{
			decoder->rep0 += Rd_decode( rdec, direct_bits - dis_align_bits ) << dis_align_bits;
			decoder->rep0 += Rd_decode_tree_reversed4( rdec, decoder->bm_align );
			if( decoder->rep0 == 0xFFFFFFFFU )		/* Marker found */
			  {
			  decoder->rep0 = rep0_saved;
			  Rd_normalize( rdec );
			  if( len == min_match_len )	/* End Of Stream marker */
				{
				if( Rd_available_bytes( rdec ) < Ft_size && !rdec->at_stream_end )
				  { decoder->verify_trailer_pending = true; return 0; }
				decoder->member_finished = true;
				if( LZd_verify_trailer( decoder ) ) return 0; else return 3;
				}
			  if( len == min_match_len + 1 )	/* Sync Flush marker */
				{
				if( Rd_try_reload( rdec, true ) ) continue;
				else return 0;
				}
			  return 4;
			  }
			}
		  }
		decoder->rep3 = decoder->rep2;
		decoder->rep2 = decoder->rep1; decoder->rep1 = rep0_saved;
		*state = St_set_match( *state );
		if( decoder->rep0 >= (unsigned)decoder->dictionary_size ||
			( decoder->rep0 >= (unsigned)decoder->cb.put &&
			  !decoder->partial_data_pos ) )
		  return 1;
		}
	  LZd_copy_block( decoder, decoder->rep0, len );
	  }
	}
  return 2;
  }


#line 1 "encoder.h"
enum { max_num_trials = 1 << 12,
	   price_shift_bits = 6,
	   price_step_bits = 2 };

static const uint8_t dis_slots[1<<10] =
  {
   0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,
   8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19 };

static inline uint8_t get_slot( const uint32_t dis )
  {
  if( dis < (1 << 10) ) return dis_slots[dis];
  if( dis < (1 << 19) ) return dis_slots[dis>> 9] + 18;
  if( dis < (1 << 28) ) return dis_slots[dis>>18] + 36;
  return dis_slots[dis>>27] + 54;
  }

static const short prob_prices[bit_model_total >> price_step_bits] =
{
640, 539, 492, 461, 438, 419, 404, 390, 379, 369, 359, 351, 343, 336, 330, 323,
318, 312, 307, 302, 298, 293, 289, 285, 281, 277, 274, 270, 267, 264, 261, 258,
255, 252, 250, 247, 244, 242, 239, 237, 235, 232, 230, 228, 226, 224, 222, 220,
218, 216, 214, 213, 211, 209, 207, 206, 204, 202, 201, 199, 198, 196, 195, 193,
192, 190, 189, 188, 186, 185, 184, 182, 181, 180, 178, 177, 176, 175, 174, 172,
171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 159, 158, 157, 157, 156,
155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 145, 144, 143, 142, 141,
140, 140, 139, 138, 137, 136, 136, 135, 134, 133, 133, 132, 131, 130, 130, 129,
128, 127, 127, 126, 125, 125, 124, 123, 123, 122, 121, 121, 120, 119, 119, 118,
117, 117, 116, 115, 115, 114, 114, 113, 112, 112, 111, 111, 110, 109, 109, 108,
108, 107, 106, 106, 105, 105, 104, 104, 103, 103, 102, 101, 101, 100, 100,  99,
 99,  98,  98,  97,  97,  96,  96,  95,  95,  94,  94,  93,  93,  92,  92,  91,
 91,  90,  90,  89,  89,  88,  88,  88,  87,  87,  86,  86,  85,  85,  84,  84,
 83,  83,  83,  82,  82,  81,  81,  80,  80,  80,  79,  79,  78,  78,  77,  77,
 77,  76,  76,  75,  75,  75,  74,  74,  73,  73,  73,  72,  72,  71,  71,  71,
 70,  70,  70,  69,  69,  68,  68,  68,  67,  67,  67,  66,  66,  65,  65,  65,
 64,  64,  64,  63,  63,  63,  62,  62,  61,  61,  61,  60,  60,  60,  59,  59,
 59,  58,  58,  58,  57,  57,  57,  56,  56,  56,  55,  55,  55,  54,  54,  54,
 53,  53,  53,  53,  52,  52,  52,  51,  51,  51,  50,  50,  50,  49,  49,  49,
 48,  48,  48,  48,  47,  47,  47,  46,  46,  46,  45,  45,  45,  45,  44,  44,
 44,  43,  43,  43,  43,  42,  42,  42,  41,  41,  41,  41,  40,  40,  40,  40,
 39,  39,  39,  38,  38,  38,  38,  37,  37,  37,  37,  36,  36,  36,  35,  35,
 35,  35,  34,  34,  34,  34,  33,  33,  33,  33,  32,  32,  32,  32,  31,  31,
 31,  31,  30,  30,  30,  30,  29,  29,  29,  29,  28,  28,  28,  28,  27,  27,
 27,  27,  26,  26,  26,  26,  26,  25,  25,  25,  25,  24,  24,  24,  24,  23,
 23,  23,  23,  22,  22,  22,  22,  22,  21,  21,  21,  21,  20,  20,  20,  20,
 20,  19,  19,  19,  19,  18,  18,  18,  18,  18,  17,  17,  17,  17,  17,  16,
 16,  16,  16,  15,  15,  15,  15,  15,  14,  14,  14,  14,  14,  13,  13,  13,
 13,  13,  12,  12,  12,  12,  12,  11,  11,  11,  11,  10,  10,  10,  10,  10,
  9,   9,   9,   9,   9,   9,   8,   8,   8,   8,   8,   7,   7,   7,   7,   7,
  6,   6,   6,   6,   6,   5,   5,   5,   5,   5,   4,   4,   4,   4,   4,   4,
  3,   3,   3,   3,   3,   2,   2,   2,   2,   2,   1,   1,   1,   1,   1,   1 };

static inline int get_price( const int probability )
  { return prob_prices[probability >> price_step_bits]; }

static inline int price0( const Bit_model probability )
  { return get_price( probability ); }

static inline int price1( const Bit_model probability )
  { return get_price( bit_model_total - probability ); }

static inline int price_bit( const Bit_model bm, const int bit )
  { if( bit ) return price1( bm ); else return price0( bm ); }

static inline int price_symbol( const Bit_model bm[], int symbol,
								const int num_bits )
  {
  int price = 0;
  symbol |= ( 1 << num_bits );
  while( symbol > 1 )
	{
	const int bit = symbol & 1;
	symbol >>= 1;
	price += price_bit( bm[symbol], bit );
	}
  return price;
  }

static inline int price_symbol_reversed( const Bit_model bm[], int symbol,
										 const int num_bits )
  {
  int price = 0;
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i )
	{
	const int bit = symbol & 1;
	price += price_bit( bm[model], bit );
	model = ( model << 1 ) | bit;
	symbol >>= 1;
	}
  return price;
  }

static inline int price_matched( const Bit_model bm[], unsigned symbol,
								 unsigned match_byte )
  {
  int price = 0;
  unsigned mask = 0x100;
  symbol |= 0x100;

  do {
	unsigned bit, match_bit;
	match_byte <<= 1;
	match_bit = match_byte & mask;
	symbol <<= 1;
	bit = symbol & 0x100;
	price += price_bit( bm[match_bit+(symbol>>9)+mask], bit );
	mask &= ~(match_byte ^ symbol);	/* if( match_bit != bit ) mask = 0; */
	}
  while( symbol < 0x10000 );
  return price;
  }

struct Pair			/* distance-length pair */
  {
  int dis;
  int len;
  };

enum { /* bytes to keep in buffer before dictionary */
	   before_size = max_num_trials + 1,
	   /* bytes to keep in buffer after pos */
	   after_size = max_num_trials + ( 2 * max_match_len ) + 1,
	   num_prev_positions3 = 1 << 16,
	   num_prev_positions2 = 1 << 10 };

struct Matchfinder
  {
  unsigned long long partial_data_pos;
  uint8_t * buffer;		/* input buffer */
  int32_t * prev_positions;	/* last seen position of key */
  int32_t * prev_pos_tree;	/* previous positions of key */
  int match_len_limit;
  int buffer_size;
  int dictionary_size;		/* bytes to keep in buffer before pos */
  int pos;			/* current pos in buffer */
  int cyclic_pos;		/* cycles through [0, dictionary_size] */
  int pos_limit;		/* when reached, a new block must be read */
  int stream_pos;		/* first byte not yet read from file */
  int cycles;
  unsigned key4_mask;
  int num_prev_positions;	/* size of prev_positions */
  bool at_stream_end;		/* stream_pos shows real end of file */
  bool been_flushed;
  bool flushing;
  };

static int Mf_write_data( struct Matchfinder * const mf,
						  const uint8_t * const inbuf, const int size )
  {
  const int sz = min( mf->buffer_size - mf->stream_pos, size );
  if( !mf->at_stream_end && sz > 0 )
	{
	memcpy( mf->buffer + mf->stream_pos, inbuf, sz );
	mf->stream_pos += sz;
	}
  return sz;
  }

static bool Mf_normalize_pos( struct Matchfinder * const mf );

static inline void Mf_free( struct Matchfinder * const mf )
  {
  free( mf->prev_positions );
  free( mf->buffer );
  }

static inline uint8_t Mf_peek( const struct Matchfinder * const mf, const int i )
  { return mf->buffer[mf->pos+i]; }

static inline int Mf_available_bytes( const struct Matchfinder * const mf )
  { return mf->stream_pos - mf->pos; }

static inline unsigned long long
Mf_data_position( const struct Matchfinder * const mf )
  { return mf->partial_data_pos + mf->pos; }

static inline void Mf_finish( struct Matchfinder * const mf )
  { mf->at_stream_end = true; mf->flushing = false; }

static inline bool Mf_finished( const struct Matchfinder * const mf )
  { return mf->at_stream_end && mf->pos >= mf->stream_pos; }

static inline void Mf_set_flushing( struct Matchfinder * const mf,
									const bool flushing )
  { mf->flushing = flushing; }

static inline bool Mf_flushing_or_end( const struct Matchfinder * const mf )
  { return mf->at_stream_end || mf->flushing; }

static inline int Mf_free_bytes( const struct Matchfinder * const mf )
  { if( mf->at_stream_end ) return 0; return mf->buffer_size - mf->stream_pos; }

static inline bool Mf_enough_available_bytes( const struct Matchfinder * const mf )
  {
  return ( mf->pos + after_size <= mf->stream_pos ||
		   ( Mf_flushing_or_end( mf ) && mf->pos < mf->stream_pos ) );
  }

static inline const uint8_t *
Mf_ptr_to_current_pos( const struct Matchfinder * const mf )
  { return mf->buffer + mf->pos; }

static inline bool Mf_dec_pos( struct Matchfinder * const mf,
							   const int ahead )
  {
  if( ahead < 0 || mf->pos < ahead ) return false;
  mf->pos -= ahead;
  mf->cyclic_pos -= ahead;
  if( mf->cyclic_pos < 0 ) mf->cyclic_pos += mf->dictionary_size + 1;
  return true;
  }

static inline int Mf_true_match_len( const struct Matchfinder * const mf,
									 const int index, const int distance,
									 int len_limit )
  {
  const uint8_t * const data = mf->buffer + mf->pos + index;
  int i = 0;

  if( index + len_limit > Mf_available_bytes( mf ) )
	len_limit = Mf_available_bytes( mf ) - index;
  while( i < len_limit && data[i-distance] == data[i] ) ++i;
  return i;
  }

static inline bool Mf_move_pos( struct Matchfinder * const mf )
  {
  if( ++mf->cyclic_pos > mf->dictionary_size ) mf->cyclic_pos = 0;
  if( ++mf->pos >= mf->pos_limit ) return Mf_normalize_pos( mf );
  return true;
  }

static int Mf_get_match_pairs( struct Matchfinder * const mf, struct Pair * pairs );

enum { re_min_free_bytes = 2 * max_num_trials };

struct Range_encoder
  {
  struct Circular_buffer cb;
  uint64_t low;
  unsigned long long partial_member_pos;
  uint32_t range;
  int ff_count;
  uint8_t cache;
  };

static inline void Re_shift_low( struct Range_encoder * const renc )
  {
  const bool carry = ( renc->low > 0xFFFFFFFFU );
  if( carry || renc->low < 0xFF000000U )
	{
	Cb_put_byte( &renc->cb, renc->cache + carry );
	for( ; renc->ff_count > 0; --renc->ff_count )
	  Cb_put_byte( &renc->cb, 0xFF + carry );
	renc->cache = renc->low >> 24;
	}
  else ++renc->ff_count;
  renc->low = ( renc->low & 0x00FFFFFFU ) << 8;
  }

static inline bool Re_init( struct Range_encoder * const renc )
  {
  if( !Cb_init( &renc->cb, 65536 + re_min_free_bytes ) ) return false;
  renc->low = 0;
  renc->partial_member_pos = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  return true;
  }

static inline void Re_free( struct Range_encoder * const renc )
  { Cb_free( &renc->cb ); }

static inline unsigned long long
Re_member_position( const struct Range_encoder * const renc )
  { return renc->partial_member_pos + Cb_used_bytes( &renc->cb ) + renc->ff_count; }

static inline bool Re_enough_free_bytes( const struct Range_encoder * const renc )
  { return Cb_free_bytes( &renc->cb ) >= re_min_free_bytes; }

static inline int Re_read_data( struct Range_encoder * const renc,
								uint8_t * const out_buffer, const int out_size )
  {
  const int size = Cb_read_data( &renc->cb, out_buffer, out_size );
  if( size > 0 ) renc->partial_member_pos += size;
  return size;
  }

static inline void Re_flush( struct Range_encoder * const renc )
  {
  int i; for( i = 0; i < 5; ++i ) Re_shift_low( renc );
  renc->low = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  }

static inline void Re_encode( struct Range_encoder * const renc,
							  const int symbol, const int num_bits )
  {
  int i;
  for( i = num_bits - 1; i >= 0; --i )
	{
	renc->range >>= 1;
	if( (symbol >> i) & 1 ) renc->low += renc->range;
	if( renc->range <= 0x00FFFFFFU )
	  { renc->range <<= 8; Re_shift_low( renc ); }
	}
  }

static inline void Re_encode_bit( struct Range_encoder * const renc,
								  Bit_model * const probability, const int bit )
  {
  const uint32_t bound = ( renc->range >> bit_model_total_bits ) * *probability;
  if( !bit )
	{
	renc->range = bound;
	*probability += (bit_model_total - *probability) >> bit_model_move_bits;
	}
  else
	{
	renc->low += bound;
	renc->range -= bound;
	*probability -= *probability >> bit_model_move_bits;
	}
  if( renc->range <= 0x00FFFFFFU )
	{ renc->range <<= 8; Re_shift_low( renc ); }
  }

static inline void Re_encode_tree( struct Range_encoder * const renc,
								   Bit_model bm[], const int symbol, const int num_bits )
  {
  int mask = ( 1 << ( num_bits - 1 ) );
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i, mask >>= 1 )
	{
	const int bit = ( symbol & mask );
	Re_encode_bit( renc, &bm[model], bit );
	model <<= 1;
	if( bit ) model |= 1;
	}
  }

static inline void Re_encode_tree_reversed( struct Range_encoder * const renc,
											Bit_model bm[], int symbol, const int num_bits )
  {
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i )
	{
	const int bit = symbol & 1;
	Re_encode_bit( renc, &bm[model], bit );
	model = ( model << 1 ) | bit;
	symbol >>= 1;
	}
  }

static inline void Re_encode_matched( struct Range_encoder * const renc,
									  Bit_model bm[], unsigned symbol,
									  unsigned match_byte )
  {
  unsigned mask = 0x100;
  symbol |= 0x100;

  do {
	unsigned bit, match_bit;
	match_byte <<= 1;
	match_bit = match_byte & mask;
	symbol <<= 1;
	bit = symbol & 0x100;
	Re_encode_bit( renc, &bm[match_bit+(symbol>>9)+mask], bit );
	mask &= ~(match_byte ^ symbol);	/* if( match_bit != bit ) mask = 0; */
	}
  while( symbol < 0x10000 );
  }

struct Len_encoder
  {
  struct Len_model lm;
  int prices[pos_states][max_len_symbols];
  int len_symbols;
  int counters[pos_states];
  };

static void Lee_update_prices( struct Len_encoder * const len_encoder,
							   const int pos_state )
  {
  int * const pps = len_encoder->prices[pos_state];
  int tmp = price0( len_encoder->lm.choice1 );
  int len = 0;
  for( ; len < len_low_symbols && len < len_encoder->len_symbols; ++len )
	pps[len] = tmp +
			   price_symbol( len_encoder->lm.bm_low[pos_state], len, len_low_bits );
  tmp = price1( len_encoder->lm.choice1 );
  for( ; len < len_low_symbols + len_mid_symbols && len < len_encoder->len_symbols; ++len )
	pps[len] = tmp + price0( len_encoder->lm.choice2 ) +
			   price_symbol( len_encoder->lm.bm_mid[pos_state], len - len_low_symbols, len_mid_bits );
  for( ; len < len_encoder->len_symbols; ++len )
	/* using 4 slots per value makes "Lee_price" faster */
	len_encoder->prices[3][len] = len_encoder->prices[2][len] =
	len_encoder->prices[1][len] = len_encoder->prices[0][len] =
	  tmp + price1( len_encoder->lm.choice2 ) +
	  price_symbol( len_encoder->lm.bm_high, len - len_low_symbols - len_mid_symbols, len_high_bits );
  len_encoder->counters[pos_state] = len_encoder->len_symbols;
  }

static void Lee_init( struct Len_encoder * const len_encoder,
					  const int match_len_limit )
  {
  int i;
  Lm_init( &len_encoder->lm );
  len_encoder->len_symbols = match_len_limit + 1 - min_match_len;
  for( i = 0; i < pos_states; ++i ) Lee_update_prices( len_encoder, i );
  }

static void Lee_encode( struct Len_encoder * const len_encoder,
						struct Range_encoder * const renc,
						int symbol, const int pos_state );

static inline int Lee_price( const struct Len_encoder * const len_encoder,
							 const int symbol, const int pos_state )
  { return len_encoder->prices[pos_state][symbol - min_match_len]; }

enum { infinite_price = 0x0FFFFFFF,
	   max_marker_size = 16,
	   num_rep_distances = 4,			/* must be 4 */
	   single_step_trial = -2,
	   dual_step_trial = -1 };

struct Trial
  {
  State state;
  int price;		/* dual use var; cumulative price, match length */
  int dis;		/* rep index or match distance */
  int prev_index;	/* index of prev trial in trials[] */
  int dis2;
  int prev_index2;	/*   -2  trial is single step */
			/*   -1  literal + rep0 */
			/* >= 0  rep or match + literal + rep0 */
  int reps[num_rep_distances];
  };

static inline void Tr_update( struct Trial * const trial, const int pr,
							  const int d, const int p_i )
  {
  if( pr < trial->price )
	{
	trial->price = pr;
	trial->dis = d; trial->prev_index = p_i;
	trial->prev_index2 = single_step_trial;
	}
  }

static inline void Tr_update2( struct Trial * const trial, const int pr,
							   const int d, const int p_i )
  {
  if( pr < trial->price )
	{
	trial->price = pr;
	trial->dis = d; trial->prev_index = p_i;
	trial->prev_index2 = dual_step_trial;
	}
  }

static inline void Tr_update3( struct Trial * const trial, const int pr,
							   const int d, const int p_i,
							   const int d2, const int p_i2 )
  {
  if( pr < trial->price )
	{
	trial->price = pr;
	trial->dis = d; trial->prev_index = p_i;
	trial->dis2 = d2; trial->prev_index2 = p_i2;
	}
  }

struct LZ_encoder
  {
  unsigned long long member_size_limit;
  int pending_num_pairs;
  uint32_t crc;

  Bit_model bm_literal[1<<literal_context_bits][0x300];
  Bit_model bm_match[states][pos_states];
  Bit_model bm_rep[states];
  Bit_model bm_rep0[states];
  Bit_model bm_rep1[states];
  Bit_model bm_rep2[states];
  Bit_model bm_len[states][pos_states];
  Bit_model bm_dis_slot[dis_states][1<<dis_slot_bits];
  Bit_model bm_dis[modeled_distances-end_dis_model];
  Bit_model bm_align[dis_align_size];

  struct Matchfinder * matchfinder;
  struct Range_encoder renc;
  struct Len_encoder match_len_encoder;
  struct Len_encoder rep_len_encoder;

  int num_dis_slots;
  int rep_distances[num_rep_distances];
  struct Pair pairs[max_match_len+1];
  struct Trial trials[max_num_trials];

  int dis_slot_prices[dis_states][2*max_dictionary_bits];
  int dis_prices[dis_states][modeled_distances];
  int align_prices[dis_align_size];
  int align_price_count;
  int fill_counter;
  State state;
  bool member_finished;
  };

static inline bool LZe_member_finished( const struct LZ_encoder * const encoder )
  {
  return ( encoder->member_finished && !Cb_used_bytes( &encoder->renc.cb ) );
  }

static inline void LZe_free( struct LZ_encoder * const encoder )
  { Re_free( &encoder->renc ); }

static inline unsigned LZe_crc( const struct LZ_encoder * const encoder )
  { return encoder->crc ^ 0xFFFFFFFFU; }

	   /* move-to-front dis in/into reps */
static inline void LZe_mtf_reps( const int dis, int reps[num_rep_distances] )
  {
  int i;
  if( dis >= num_rep_distances )
	{
	for( i = num_rep_distances - 1; i > 0; --i ) reps[i] = reps[i-1];
	reps[0] = dis - num_rep_distances;
	}
  else if( dis > 0 )
	{
	const int distance = reps[dis];
	for( i = dis; i > 0; --i ) reps[i] = reps[i-1];
	reps[0] = distance;
	}
  }

static inline int LZe_price_rep_len1( const struct LZ_encoder * const encoder,
									  const State state, const int pos_state )
  {
  return price0( encoder->bm_rep0[state] ) +
		 price0( encoder->bm_len[state][pos_state] );
  }

static inline int LZe_price_rep( const struct LZ_encoder * const encoder,
								 const int rep,
								 const State state, const int pos_state )
  {
  int price;
  if( rep == 0 ) return price0( encoder->bm_rep0[state] ) +
						price1( encoder->bm_len[state][pos_state] );
  price = price1( encoder->bm_rep0[state] );
  if( rep == 1 )
	price += price0( encoder->bm_rep1[state] );
  else
	{
	price += price1( encoder->bm_rep1[state] );
	price += price_bit( encoder->bm_rep2[state], rep - 2 );
	}
  return price;
  }

static inline int LZe_price_rep0_len( const struct LZ_encoder * const encoder,
									  const int len,
									  const State state, const int pos_state )
  {
  return LZe_price_rep( encoder, 0, state, pos_state ) +
		 Lee_price( &encoder->rep_len_encoder, len, pos_state );
  }

static inline int LZe_price_dis( const struct LZ_encoder * const encoder,
								 const int dis, const int dis_state )
  {
  if( dis < modeled_distances )
	return encoder->dis_prices[dis_state][dis];
  else
	return encoder->dis_slot_prices[dis_state][get_slot( dis )] +
		   encoder->align_prices[dis & (dis_align_size - 1)];
  }

static inline int LZe_price_pair( const struct LZ_encoder * const encoder,
								  const int dis, const int len,
								  const int pos_state )
  {
  return Lee_price( &encoder->match_len_encoder, len, pos_state ) +
		 LZe_price_dis( encoder, dis, get_dis_state( len ) );
  }

static inline int LZe_price_literal( const struct LZ_encoder * const encoder,
									uint8_t prev_byte, uint8_t symbol )
  { return price_symbol( encoder->bm_literal[get_lit_state(prev_byte)], symbol, 8 ); }

static inline int LZe_price_matched( const struct LZ_encoder * const encoder,
									 uint8_t prev_byte, uint8_t symbol,
									 uint8_t match_byte )
  { return price_matched( encoder->bm_literal[get_lit_state(prev_byte)],
						  symbol, match_byte ); }

static inline void LZe_encode_literal( struct LZ_encoder * const encoder,
									   uint8_t prev_byte, uint8_t symbol )
  { Re_encode_tree( &encoder->renc,
					encoder->bm_literal[get_lit_state(prev_byte)], symbol, 8 ); }

static inline void LZe_encode_matched( struct LZ_encoder * const encoder,
									   uint8_t prev_byte, uint8_t symbol,
									   uint8_t match_byte )
  { Re_encode_matched( &encoder->renc,
					   encoder->bm_literal[get_lit_state(prev_byte)],
					   symbol, match_byte ); }

static inline void LZe_encode_pair( struct LZ_encoder * const encoder,
									const uint32_t dis, const int len,
									const int pos_state )
  {
  const int dis_slot = get_slot( dis );
  Lee_encode( &encoder->match_len_encoder, &encoder->renc, len, pos_state );
  Re_encode_tree( &encoder->renc, encoder->bm_dis_slot[get_dis_state(len)],
				  dis_slot, dis_slot_bits );

  if( dis_slot >= start_dis_model )
	{
	const int direct_bits = ( dis_slot >> 1 ) - 1;
	const uint32_t base = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
	const uint32_t direct_dis = dis - base;

	if( dis_slot < end_dis_model )
	  Re_encode_tree_reversed( &encoder->renc,
							   encoder->bm_dis + base - dis_slot - 1,
							   direct_dis, direct_bits );
	else
	  {
	  Re_encode( &encoder->renc, direct_dis >> dis_align_bits,
				 direct_bits - dis_align_bits );
	  Re_encode_tree_reversed( &encoder->renc, encoder->bm_align,
							   direct_dis, dis_align_bits );
	  --encoder->align_price_count;
	  }
	}
  }

static inline int LZe_read_match_distances( struct LZ_encoder * const encoder )
  {
  const int num_pairs =
	Mf_get_match_pairs( encoder->matchfinder, encoder->pairs );
  if( num_pairs > 0 )
	{
	int len = encoder->pairs[num_pairs-1].len;
	if( len == encoder->matchfinder->match_len_limit && len < max_match_len )
	  {
	  len += Mf_true_match_len( encoder->matchfinder, len,
								encoder->pairs[num_pairs-1].dis + 1,
								max_match_len - len );
	  encoder->pairs[num_pairs-1].len = len;
	  }
	}
  return num_pairs;
  }

static inline bool LZe_move_pos( struct LZ_encoder * const encoder, int n )
  {
  if( --n >= 0 && !Mf_move_pos( encoder->matchfinder ) ) return false;
  while( --n >= 0 )
	{
	Mf_get_match_pairs( encoder->matchfinder, 0 );
	if( !Mf_move_pos( encoder->matchfinder ) ) return false;
	}
  return true;
  }

static inline void LZe_backward( struct LZ_encoder * const encoder, int cur )
  {
  int * const dis = &encoder->trials[cur].dis;
  while( cur > 0 )
	{
	const int prev_index = encoder->trials[cur].prev_index;
	struct Trial * const prev_trial = &encoder->trials[prev_index];

	if( encoder->trials[cur].prev_index2 != single_step_trial )
	  {
	  prev_trial->dis = -1;
	  prev_trial->prev_index = prev_index - 1;
	  prev_trial->prev_index2 = single_step_trial;
	  if( encoder->trials[cur].prev_index2 >= 0 )
		{
		struct Trial * const prev_trial2 = &encoder->trials[prev_index-1];
		prev_trial2->dis = encoder->trials[cur].dis2;
		prev_trial2->prev_index = encoder->trials[cur].prev_index2;
		prev_trial2->prev_index2 = single_step_trial;
		}
	  }
	prev_trial->price = cur - prev_index;			/* len */
	cur = *dis; *dis = prev_trial->dis; prev_trial->dis = cur;
	cur = prev_index;
	}
  }


#line 1 "encoder.c"
static bool Mf_normalize_pos( struct Matchfinder * const mf )
  {
  if( mf->pos > mf->stream_pos )
	{ mf->pos = mf->stream_pos; return false; }
  if( !mf->at_stream_end )
	{
	int i;
	const int offset = mf->pos - mf->dictionary_size - before_size;
	const int size = mf->stream_pos - offset;
	memmove( mf->buffer, mf->buffer + offset, size );
	mf->partial_data_pos += offset;
	mf->pos -= offset;
	mf->stream_pos -= offset;
	for( i = 0; i < mf->num_prev_positions; ++i )
	  if( mf->prev_positions[i] >= 0 ) mf->prev_positions[i] -= offset;
	for( i = 0; i < 2 * ( mf->dictionary_size + 1 ); ++i )
	  if( mf->prev_pos_tree[i] >= 0 ) mf->prev_pos_tree[i] -= offset;
	}
  return true;
  }

static bool Mf_init( struct Matchfinder * const mf,
					 const int dict_size, const int match_len_limit )
  {
  const int buffer_size_limit = ( 2 * dict_size ) + before_size + after_size;
  unsigned size;
  int i;

  mf->partial_data_pos = 0;
  mf->match_len_limit = match_len_limit;
  mf->pos = 0;
  mf->cyclic_pos = 0;
  mf->stream_pos = 0;
  mf->cycles = ( match_len_limit < max_match_len ) ?
			   16 + ( match_len_limit / 2 ) : 256;
  mf->at_stream_end = false;
  mf->been_flushed = false;
  mf->flushing = false;

  mf->buffer_size = max( 65536, buffer_size_limit );
  mf->buffer = (uint8_t *)malloc( mf->buffer_size );
  if( !mf->buffer ) return false;
  mf->dictionary_size = dict_size;
  mf->pos_limit = mf->buffer_size - after_size;
  size = 1 << max( 16, real_bits( mf->dictionary_size - 1 ) - 2 );
  if( mf->dictionary_size > 1 << 26 )		/* 64 MiB */
	size >>= 1;
  mf->key4_mask = size - 1;
  size += num_prev_positions2;
  size += num_prev_positions3;

  mf->num_prev_positions = size;
  size += ( 2 * ( mf->dictionary_size + 1 ) );
  if( size * sizeof (int32_t) <= size ) mf->prev_positions = 0;
  else mf->prev_positions = (int32_t *)malloc( size * sizeof (int32_t) );
  if( !mf->prev_positions ) { free( mf->buffer ); return false; }
  mf->prev_pos_tree = mf->prev_positions + mf->num_prev_positions;
  for( i = 0; i < mf->num_prev_positions; ++i ) mf->prev_positions[i] = -1;
  return true;
  }

static void Mf_adjust_distionary_size( struct Matchfinder * const mf )
  {
  if( mf->stream_pos < mf->dictionary_size )
	{
	int size;
	mf->buffer_size =
	mf->dictionary_size =
	mf->pos_limit = max( min_dictionary_size, mf->stream_pos );
	size = 1 << max( 16, real_bits( mf->dictionary_size - 1 ) - 2 );
	if( mf->dictionary_size > 1 << 26 )
	  size >>= 1;
	mf->key4_mask = size - 1;
	size += num_prev_positions2;
	size += num_prev_positions3;
	mf->num_prev_positions = size;
	mf->prev_pos_tree = mf->prev_positions + mf->num_prev_positions;
	}
  }

static void Mf_reset( struct Matchfinder * const mf )
  {
  int i;
  const int size = mf->stream_pos - mf->pos;
  if( size > 0 ) memmove( mf->buffer, mf->buffer + mf->pos, size );
  mf->partial_data_pos = 0;
  mf->stream_pos -= mf->pos;
  mf->pos = 0;
  mf->cyclic_pos = 0;
  mf->at_stream_end = false;
  mf->been_flushed = false;
  mf->flushing = false;
  for( i = 0; i < mf->num_prev_positions; ++i ) mf->prev_positions[i] = -1;
  }

static int Mf_get_match_pairs( struct Matchfinder * const mf, struct Pair * pairs )
  {
  int32_t * ptr0 = mf->prev_pos_tree + ( mf->cyclic_pos << 1 );
  int32_t * ptr1 = ptr0 + 1;
  int32_t * newptr;
  int len = 0, len0 = 0, len1 = 0;
  int maxlen = min_match_len - 1;
  int num_pairs = 0;
  const int min_pos = (mf->pos > mf->dictionary_size) ?
					   mf->pos - mf->dictionary_size : 0;
  const uint8_t * const data = mf->buffer + mf->pos;
  int count, delta, key2, key3, key4, newpos;
  unsigned tmp;
  int len_limit = mf->match_len_limit;

  if( len_limit > Mf_available_bytes( mf ) )
	{
	mf->been_flushed = true;
	len_limit = Mf_available_bytes( mf );
	if( len_limit < 4 ) { *ptr0 = *ptr1 = -1; return 0; }
	}

  tmp = crc32[data[0]] ^ data[1];
  key2 = tmp & ( num_prev_positions2 - 1 );
  tmp ^= (uint32_t)data[2] << 8;
  key3 = num_prev_positions2 + ( tmp & ( num_prev_positions3 - 1 ) );
  key4 = num_prev_positions2 + num_prev_positions3 +
		 ( ( tmp ^ ( crc32[data[3]] << 5 ) ) & mf->key4_mask );

  if( pairs )
	{
	int np2 = mf->prev_positions[key2];
	int np3 = mf->prev_positions[key3];
	if( np2 >= min_pos && mf->buffer[np2] == data[0] )
	  {
	  pairs[0].dis = mf->pos - np2 - 1;
	  pairs[0].len = maxlen = 2;
	  num_pairs = 1;
	  }
	if( np2 != np3 && np3 >= min_pos && mf->buffer[np3] == data[0] )
	  {
	  maxlen = 3;
	  pairs[num_pairs].dis = mf->pos - np3 - 1;
	  ++num_pairs;
	  np2 = np3;
	  }
	if( num_pairs > 0 )
	  {
	  delta = mf->pos - np2;
	  while( maxlen < len_limit && data[maxlen-delta] == data[maxlen] )
		++maxlen;
	  pairs[num_pairs-1].len = maxlen;
	  if( maxlen >= len_limit ) pairs = 0;
	  }
	if( maxlen < 3 ) maxlen = 3;
	}

  mf->prev_positions[key2] = mf->pos;
  mf->prev_positions[key3] = mf->pos;
  newpos = mf->prev_positions[key4];
  mf->prev_positions[key4] = mf->pos;

  for( count = mf->cycles; ; )
	{
	if( newpos < min_pos || --count < 0 ) { *ptr0 = *ptr1 = -1; break; }

	if( mf->been_flushed ) len = 0;
	delta = mf->pos - newpos;
	newptr = mf->prev_pos_tree +
	  ( ( mf->cyclic_pos - delta +
		  ( (mf->cyclic_pos >= delta) ? 0 : mf->dictionary_size + 1 ) ) << 1 );
	if( data[len-delta] == data[len] )
	  {
	  while( ++len < len_limit && data[len-delta] == data[len] ) {}
	  if( pairs && maxlen < len )
		{
		pairs[num_pairs].dis = delta - 1;
		pairs[num_pairs].len = maxlen = len;
		++num_pairs;
		}
	  if( len >= len_limit )
		{
		*ptr0 = newptr[0];
		*ptr1 = newptr[1];
		break;
		}
	  }
	if( data[len-delta] < data[len] )
	  {
	  *ptr0 = newpos;
	  ptr0 = newptr + 1;
	  newpos = *ptr0;
	  len0 = len; if( len1 < len ) len = len1;
	  }
	else
	  {
	  *ptr1 = newpos;
	  ptr1 = newptr;
	  newpos = *ptr1;
	  len1 = len; if( len0 < len ) len = len0;
	  }
	}
  return num_pairs;
  }

static void Lee_encode( struct Len_encoder * const len_encoder,
						struct Range_encoder * const renc,
						int symbol, const int pos_state )
  {
  symbol -= min_match_len;
  if( symbol < len_low_symbols )
	{
	Re_encode_bit( renc, &len_encoder->lm.choice1, 0 );
	Re_encode_tree( renc, len_encoder->lm.bm_low[pos_state], symbol, len_low_bits );
	}
  else
	{
	Re_encode_bit( renc, &len_encoder->lm.choice1, 1 );
	if( symbol < len_low_symbols + len_mid_symbols )
	  {
	  Re_encode_bit( renc, &len_encoder->lm.choice2, 0 );
	  Re_encode_tree( renc, len_encoder->lm.bm_mid[pos_state],
					  symbol - len_low_symbols, len_mid_bits );
	  }
	else
	  {
	  Re_encode_bit( renc, &len_encoder->lm.choice2, 1 );
	  Re_encode_tree( renc, len_encoder->lm.bm_high,
					  symbol - len_low_symbols - len_mid_symbols, len_high_bits );
	  }
	}
  if( --len_encoder->counters[pos_state] <= 0 )
	Lee_update_prices( len_encoder, pos_state );
  }

	 /* End Of Stream mark => (dis == 0xFFFFFFFFU, len == min_match_len) */
static bool LZe_full_flush( struct LZ_encoder * const encoder, const State state )
  {
  int i;
  const int pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
  File_trailer trailer;
  if( encoder->member_finished ||
	  Cb_free_bytes( &encoder->renc.cb ) < max_marker_size + Ft_size )
	return false;
  Re_encode_bit( &encoder->renc, &encoder->bm_match[state][pos_state], 1 );
  Re_encode_bit( &encoder->renc, &encoder->bm_rep[state], 0 );
  LZe_encode_pair( encoder, 0xFFFFFFFFU, min_match_len, pos_state );
  Re_flush( &encoder->renc );
  Ft_set_data_crc( trailer, LZe_crc( encoder ) );
  Ft_set_data_size( trailer, Mf_data_position( encoder->matchfinder ) );
  Ft_set_member_size( trailer, Re_member_position( &encoder->renc ) + Ft_size );
  for( i = 0; i < Ft_size; ++i )
	Cb_put_byte( &encoder->renc.cb, trailer[i] );
  return true;
  }

	 /* Sync Flush mark => (dis == 0xFFFFFFFFU, len == min_match_len + 1) */
static bool LZe_sync_flush( struct LZ_encoder * const encoder )
  {
  const int pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
  const State state = encoder->state;
  if( encoder->member_finished ||
	  Cb_free_bytes( &encoder->renc.cb ) < max_marker_size )
	return false;
  Re_encode_bit( &encoder->renc, &encoder->bm_match[state][pos_state], 1 );
  Re_encode_bit( &encoder->renc, &encoder->bm_rep[state], 0 );
  LZe_encode_pair( encoder, 0xFFFFFFFFU, min_match_len + 1, pos_state );
  Re_flush( &encoder->renc );
  return true;
  }

static void LZe_fill_align_prices( struct LZ_encoder * const encoder )
  {
  int i;
  for( i = 0; i < dis_align_size; ++i )
	encoder->align_prices[i] =
	  price_symbol_reversed( encoder->bm_align, i, dis_align_bits );
  encoder->align_price_count = dis_align_size;
  }

static void LZe_fill_distance_prices( struct LZ_encoder * const encoder )
  {
  int dis, dis_state;
  for( dis = start_dis_model; dis < modeled_distances; ++dis )
	{
	const int dis_slot = dis_slots[dis];
	const int direct_bits = ( dis_slot >> 1 ) - 1;
	const int base = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
	const int price =
	  price_symbol_reversed( encoder->bm_dis + base - dis_slot - 1,
							 dis - base, direct_bits );
	for( dis_state = 0; dis_state < dis_states; ++dis_state )
	  encoder->dis_prices[dis_state][dis] = price;
	}

  for( dis_state = 0; dis_state < dis_states; ++dis_state )
	{
	int * const dsp = encoder->dis_slot_prices[dis_state];
	int * const dp = encoder->dis_prices[dis_state];
	const Bit_model * const bmds = encoder->bm_dis_slot[dis_state];
	int slot = 0;
	for( ; slot < end_dis_model && slot < encoder->num_dis_slots; ++slot )
	  dsp[slot] = price_symbol( bmds, slot, dis_slot_bits );
	for( ; slot < encoder->num_dis_slots; ++slot )
	  dsp[slot] = price_symbol( bmds, slot, dis_slot_bits ) +
				  (((( slot >> 1 ) - 1 ) - dis_align_bits ) << price_shift_bits );

	for( dis = 0; dis < start_dis_model; ++dis )
	  dp[dis] = dsp[dis];
	for( ; dis < modeled_distances; ++dis )
	  dp[dis] += dsp[dis_slots[dis]];
	}
  }

static bool LZe_init( struct LZ_encoder * const encoder,
					  struct Matchfinder * const mf,
					  const File_header header,
					  const unsigned long long member_size )
  {
  int i;
  encoder->member_size_limit = member_size - Ft_size - max_marker_size;
  encoder->pending_num_pairs = 0;
  encoder->crc = 0xFFFFFFFFU;

  Bm_array_init( encoder->bm_literal[0], (1 << literal_context_bits) * 0x300 );
  Bm_array_init( encoder->bm_match[0], states * pos_states );
  Bm_array_init( encoder->bm_rep, states );
  Bm_array_init( encoder->bm_rep0, states );
  Bm_array_init( encoder->bm_rep1, states );
  Bm_array_init( encoder->bm_rep2, states );
  Bm_array_init( encoder->bm_len[0], states * pos_states );
  Bm_array_init( encoder->bm_dis_slot[0], dis_states * (1 << dis_slot_bits) );
  Bm_array_init( encoder->bm_dis, modeled_distances - end_dis_model );
  Bm_array_init( encoder->bm_align, dis_align_size );

  encoder->matchfinder = mf;
  if( !Re_init( &encoder->renc ) ) return false;
  Lee_init( &encoder->match_len_encoder, encoder->matchfinder->match_len_limit );
  Lee_init( &encoder->rep_len_encoder, encoder->matchfinder->match_len_limit );
  encoder->num_dis_slots =
	2 * real_bits( encoder->matchfinder->dictionary_size - 1 );

  for( i = 0; i < num_rep_distances; ++i ) encoder->rep_distances[i] = 0;
  encoder->align_price_count = 0;
  encoder->fill_counter = 0;
  encoder->state = 0;
  encoder->member_finished = false;

  for( i = 0; i < Fh_size; ++i )
	Cb_put_byte( &encoder->renc.cb, header[i] );
  return true;
  }

/* Return value == number of bytes advanced (ahead).
   trials[0]..trials[ahead-1] contain the steps to encode.
   ( trials[0].dis == -1 && trials[0].price == 1 ) means literal.
*/
static int LZe_sequence_optimizer( struct LZ_encoder * const encoder,
								   const int reps[num_rep_distances],
								   const State state )
  {
  int main_len, num_pairs, i, rep, cur = 0, num_trials, len;
  int replens[num_rep_distances];
  int rep_index = 0;

  if( encoder->pending_num_pairs > 0 )		/* from previous call */
	{
	num_pairs = encoder->pending_num_pairs;
	encoder->pending_num_pairs = 0;
	}
  else
	num_pairs = LZe_read_match_distances( encoder );
  main_len = ( num_pairs > 0 ) ? encoder->pairs[num_pairs-1].len : 0;

  for( i = 0; i < num_rep_distances; ++i )
	{
	replens[i] =
	  Mf_true_match_len( encoder->matchfinder, 0, reps[i] + 1, max_match_len );
	if( replens[i] > replens[rep_index] ) rep_index = i;
	}
  if( replens[rep_index] >= encoder->matchfinder->match_len_limit )
	{
	encoder->trials[0].dis = rep_index;
	encoder->trials[0].price = replens[rep_index];
	if( !LZe_move_pos( encoder, replens[rep_index] ) ) return 0;
	return replens[rep_index];
	}

  if( main_len >= encoder->matchfinder->match_len_limit )
	{
	encoder->trials[0].dis = encoder->pairs[num_pairs-1].dis + num_rep_distances;
	encoder->trials[0].price = main_len;
	if( !LZe_move_pos( encoder, main_len ) ) return 0;
	return main_len;
	}

  {
  const int pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
  const int match_price = price1( encoder->bm_match[state][pos_state] );
  const int rep_match_price = match_price + price1( encoder->bm_rep[state] );
  const uint8_t prev_byte = Mf_peek( encoder->matchfinder, -1 );
  const uint8_t cur_byte = Mf_peek( encoder->matchfinder, 0 );
  const uint8_t match_byte = Mf_peek( encoder->matchfinder, -reps[0]-1 );

  encoder->trials[0].state = state;
  encoder->trials[1].dis = -1;
  encoder->trials[1].price = price0( encoder->bm_match[state][pos_state] );
  if( St_is_char( state ) )
	encoder->trials[1].price +=
	  LZe_price_literal( encoder, prev_byte, cur_byte );
  else
	encoder->trials[1].price +=
	  LZe_price_matched( encoder, prev_byte, cur_byte, match_byte );

  if( match_byte == cur_byte )
	Tr_update( &encoder->trials[1], rep_match_price +
			   LZe_price_rep_len1( encoder, state, pos_state ), 0, 0 );

  num_trials = max( main_len, replens[rep_index] );

  if( num_trials < min_match_len )
	{
	encoder->trials[0].dis = encoder->trials[1].dis;
	encoder->trials[0].price = 1;
	if( !Mf_move_pos( encoder->matchfinder ) ) return 0;
	return 1;
	}

  for( i = 0; i < num_rep_distances; ++i )
	encoder->trials[0].reps[i] = reps[i];
  encoder->trials[1].prev_index = 0;
  encoder->trials[1].prev_index2 = single_step_trial;

  for( len = min_match_len; len <= num_trials; ++len )
	encoder->trials[len].price = infinite_price;

  for( rep = 0; rep < num_rep_distances; ++rep )
	{
	int price;
	if( replens[rep] < min_match_len ) continue;
	price = rep_match_price + LZe_price_rep( encoder, rep, state, pos_state );

	for( len = min_match_len; len <= replens[rep]; ++len )
	  Tr_update( &encoder->trials[len], price +
				 Lee_price( &encoder->rep_len_encoder, len, pos_state ),
				 rep, 0 );
	}

  if( main_len > replens[0] )
	{
	const int normal_match_price = match_price + price0( encoder->bm_rep[state] );
	i = 0, len = max( replens[0] + 1, min_match_len );
	while( len > encoder->pairs[i].len ) ++i;
	while( true )
	  {
	  const int dis = encoder->pairs[i].dis;
	  Tr_update( &encoder->trials[len], normal_match_price +
				 LZe_price_pair( encoder, dis, len, pos_state ),
				 dis + num_rep_distances, 0 );
	  if( ++len > encoder->pairs[i].len && ++i >= num_pairs ) break;
	  }
	}
  }

  if( !Mf_move_pos( encoder->matchfinder ) ) return 0;

  while( true )				/* price optimization loop */
	{
	struct Trial *cur_trial, *next_trial;
	int newlen, pos_state, prev_index, prev_index2, available_bytes, len_limit;
	int start_len = min_match_len;
	int next_price, match_price, rep_match_price;
	State cur_state;
	uint8_t prev_byte, cur_byte, match_byte;

	if( ++cur >= num_trials )		/* no more initialized trials */
	  {
	  LZe_backward( encoder, cur );
	  return cur;
	  }

	num_pairs = LZe_read_match_distances( encoder );
	newlen = ( num_pairs > 0 ) ? encoder->pairs[num_pairs-1].len : 0;
	if( newlen >= encoder->matchfinder->match_len_limit )
	  {
	  encoder->pending_num_pairs = num_pairs;
	  LZe_backward( encoder, cur );
	  return cur;
	  }

	/* give final values to current trial */
	cur_trial = &encoder->trials[cur];
	prev_index = cur_trial->prev_index;
	prev_index2 = cur_trial->prev_index2;

	if( prev_index2 != single_step_trial )
	  {
	  --prev_index;
	  if( prev_index2 >= 0 )
		{
		cur_state = encoder->trials[prev_index2].state;
		if( cur_trial->dis2 < num_rep_distances )
		  cur_state = St_set_rep( cur_state );
		else
		  cur_state = St_set_match( cur_state );
		}
	  else
		cur_state = encoder->trials[prev_index].state;
	  cur_state = St_set_char( cur_state );
	  }
	else
	  cur_state = encoder->trials[prev_index].state;

	if( prev_index == cur - 1 )
	  {
	  if( cur_trial->dis == 0 )
		cur_state = St_set_short_rep( cur_state );
	  else
		cur_state = St_set_char( cur_state );
	  for( i = 0; i < num_rep_distances; ++i )
		cur_trial->reps[i] = encoder->trials[prev_index].reps[i];
	  }
	else
	  {
	  int dis;
	  if( prev_index2 >= 0 )
		{
		dis = cur_trial->dis2;
		prev_index = prev_index2;
		cur_state = St_set_rep( cur_state );
		}
	  else
		{
		dis = cur_trial->dis;
		if( dis < num_rep_distances )
		  cur_state = St_set_rep( cur_state );
		else
		  cur_state = St_set_match( cur_state );
		}
	  for( i = 0; i < num_rep_distances; ++i )
		cur_trial->reps[i] = encoder->trials[prev_index].reps[i];
	  LZe_mtf_reps( dis, cur_trial->reps );
	  }
	cur_trial->state = cur_state;

	pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
	prev_byte = Mf_peek( encoder->matchfinder, -1 );
	cur_byte = Mf_peek( encoder->matchfinder, 0 );
	match_byte = Mf_peek( encoder->matchfinder, -cur_trial->reps[0]-1 );

	next_price = cur_trial->price +
				 price0( encoder->bm_match[cur_state][pos_state] );
	if( St_is_char( cur_state ) )
	  next_price += LZe_price_literal( encoder, prev_byte, cur_byte );
	else
	  next_price += LZe_price_matched( encoder, prev_byte, cur_byte, match_byte );
	if( !Mf_move_pos( encoder->matchfinder ) ) return 0;

	/* try last updates to next trial */
	next_trial = &encoder->trials[cur+1];

	Tr_update( next_trial, next_price, -1, cur );

	match_price = cur_trial->price + price1( encoder->bm_match[cur_state][pos_state] );
	rep_match_price = match_price + price1( encoder->bm_rep[cur_state] );

	if( match_byte == cur_byte && next_trial->dis != 0 )
	  {
	  const int price = rep_match_price +
						LZe_price_rep_len1( encoder, cur_state, pos_state );
	  if( price <= next_trial->price )
		{
		next_trial->price = price;
		next_trial->dis = 0;
		next_trial->prev_index = cur;
		next_trial->prev_index2 = single_step_trial;
		}
	  }

	available_bytes = min( Mf_available_bytes( encoder->matchfinder ) + 1,
						   max_num_trials - 1 - cur );
	if( available_bytes < min_match_len ) continue;

	len_limit = min( encoder->matchfinder->match_len_limit, available_bytes );

	/* try literal + rep0 */
	if( match_byte != cur_byte && next_trial->prev_index != cur )
	  {
	  const uint8_t * const data = Mf_ptr_to_current_pos( encoder->matchfinder ) - 1;
	  const int dis = cur_trial->reps[0] + 1;
	  const int limit = min( encoder->matchfinder->match_len_limit + 1,
							 available_bytes );
	  len = 1;
	  while( len < limit && data[len-dis] == data[len] ) ++len;
	  if( --len >= min_match_len )
		{
		const int pos_state2 = ( pos_state + 1 ) & pos_state_mask;
		const State state2 = St_set_char( cur_state );
		const int price = next_price +
				  price1( encoder->bm_match[state2][pos_state2] ) +
				  price1( encoder->bm_rep[state2] ) +
				  LZe_price_rep0_len( encoder, len, state2, pos_state2 );
		while( num_trials < cur + 1 + len )
		  encoder->trials[++num_trials].price = infinite_price;
		Tr_update2( &encoder->trials[cur+1+len], price, 0, cur + 1 );
		}
	  }

	/* try rep distances */
	for( rep = 0; rep < num_rep_distances; ++rep )
	  {
	  const uint8_t * const data = Mf_ptr_to_current_pos( encoder->matchfinder ) - 1;
	  int price;
	  const int dis = cur_trial->reps[rep] + 1;

	  if( data[-dis] != data[0] || data[1-dis] != data[1] ) continue;
	  for( len = min_match_len; len < len_limit; ++len )
		if( data[len-dis] != data[len] ) break;
	  while( num_trials < cur + len )
		encoder->trials[++num_trials].price = infinite_price;
	  price = rep_match_price +
			  LZe_price_rep( encoder, rep, cur_state, pos_state );
	  for( i = min_match_len; i <= len; ++i )
		Tr_update( &encoder->trials[cur+i], price +
				   Lee_price( &encoder->rep_len_encoder, i, pos_state ),
				   rep, cur );

	  if( rep == 0 ) start_len = len + 1;	/* discard shorter matches */

	  /* try rep + literal + rep0 */
	  {
	  int len2 = len + 1, pos_state2;
	  const int limit = min( encoder->matchfinder->match_len_limit + len2,
							 available_bytes );
	  State state2;
	  while( len2 < limit && data[len2-dis] == data[len2] ) ++len2;
	  len2 -= len + 1;
	  if( len2 < min_match_len ) continue;

	  pos_state2 = ( pos_state + len ) & pos_state_mask;
	  state2 = St_set_rep( cur_state );
	  price += Lee_price( &encoder->rep_len_encoder, len, pos_state ) +
			   price0( encoder->bm_match[state2][pos_state2] ) +
			   LZe_price_matched( encoder, data[len-1], data[len], data[len-dis] );
	  pos_state2 = ( pos_state2 + 1 ) & pos_state_mask;
	  state2 = St_set_char( state2 );
	  price += price1( encoder->bm_match[state2][pos_state2] ) +
			   price1( encoder->bm_rep[state2] ) +
			   LZe_price_rep0_len( encoder, len2, state2, pos_state2 );
	  while( num_trials < cur + len + 1 + len2 )
		encoder->trials[++num_trials].price = infinite_price;
	  Tr_update3( &encoder->trials[cur+len+1+len2], price, 0, cur + len + 1,
				  rep, cur );
	  }
	  }

	/* try matches */
	if( newlen >= start_len && newlen <= len_limit )
	  {
	  int dis;
	  const int normal_match_price = match_price +
									 price0( encoder->bm_rep[cur_state] );

	  while( num_trials < cur + newlen )
		encoder->trials[++num_trials].price = infinite_price;

	  i = 0;
	  while( start_len > encoder->pairs[i].len ) ++i;
	  dis = encoder->pairs[i].dis;
	  for( len = start_len; ; ++len )
		{
		int price = normal_match_price +
					LZe_price_pair( encoder, dis, len, pos_state );

		Tr_update( &encoder->trials[cur+len], price, dis + num_rep_distances, cur );

		/* try match + literal + rep0 */
		if( len == encoder->pairs[i].len )
		  {
		  const uint8_t * const data = Mf_ptr_to_current_pos( encoder->matchfinder ) - 1;
		  const int dis2 = dis + 1;
		  int len2 = len + 1;
		  const int limit = min( encoder->matchfinder->match_len_limit + len2,
								 available_bytes );
		  while( len2 < limit && data[len2-dis2] == data[len2] ) ++len2;
		  len2 -= len + 1;
		  if( len2 >= min_match_len )
			{
			int pos_state2 = ( pos_state + len ) & pos_state_mask;
			State state2 = St_set_match( cur_state );
			price += price0( encoder->bm_match[state2][pos_state2] ) +
					 LZe_price_matched( encoder, data[len-1], data[len], data[len-dis2] );
			pos_state2 = ( pos_state2 + 1 ) & pos_state_mask;
			state2 = St_set_char( state2 );
			price += price1( encoder->bm_match[state2][pos_state2] ) +
					 price1( encoder->bm_rep[state2] ) +
					 LZe_price_rep0_len( encoder, len2, state2, pos_state2 );

			while( num_trials < cur + len + 1 + len2 )
			  encoder->trials[++num_trials].price = infinite_price;
			Tr_update3( &encoder->trials[cur+len+1+len2], price, 0,
						cur + len + 1, dis + num_rep_distances, cur );
			}
		  if( ++i >= num_pairs ) break;
		  dis = encoder->pairs[i].dis;
		  }
		}
	  }
	}
  }

static bool LZe_encode_member( struct LZ_encoder * const encoder )
  {
  const int fill_count =
	( encoder->matchfinder->match_len_limit > 12 ) ? 128 : 512;
  int ahead, i;
  State * const state = &encoder->state;

  if( encoder->member_finished ) return true;
  if( Re_member_position( &encoder->renc ) >= encoder->member_size_limit )
	{
	if( LZe_full_flush( encoder, *state ) ) encoder->member_finished = true;
	return true;
	}

  if( Mf_data_position( encoder->matchfinder ) == 0 &&
	  !Mf_finished( encoder->matchfinder ) )	/* encode first byte */
	{
	const uint8_t prev_byte = 0;
	uint8_t cur_byte;
	if( Mf_available_bytes( encoder->matchfinder ) < max_match_len &&
		!Mf_flushing_or_end( encoder->matchfinder ) )
	  return true;
	cur_byte = Mf_peek( encoder->matchfinder, 0 );
	Re_encode_bit( &encoder->renc, &encoder->bm_match[*state][0], 0 );
	LZe_encode_literal( encoder, prev_byte, cur_byte );
	CRC32_update_byte( &encoder->crc, cur_byte );
	Mf_get_match_pairs( encoder->matchfinder, 0 );
	if( !Mf_move_pos( encoder->matchfinder ) ) return false;
	}

  while( !Mf_finished( encoder->matchfinder ) )
	{
	if( !Mf_enough_available_bytes( encoder->matchfinder ) ||
		!Re_enough_free_bytes( &encoder->renc ) ) return true;
	if( encoder->pending_num_pairs == 0 )
	  {
	  if( encoder->fill_counter <= 0 )
		{ LZe_fill_distance_prices( encoder ); encoder->fill_counter = fill_count; }
	  if( encoder->align_price_count <= 0 )
		LZe_fill_align_prices( encoder );
	  }

	ahead = LZe_sequence_optimizer( encoder, encoder->rep_distances, *state );
	if( ahead <= 0 ) return false;		/* can't happen */

	for( i = 0; ; )
	  {
	  const int pos_state =
		( Mf_data_position( encoder->matchfinder ) - ahead ) & pos_state_mask;
	  const int dis = encoder->trials[i].dis;
	  const int len = encoder->trials[i].price;

	  bool bit = ( dis < 0 && len == 1 );
	  Re_encode_bit( &encoder->renc,
					 &encoder->bm_match[*state][pos_state], !bit );
	  if( bit )					/* literal byte */
		{
		const uint8_t prev_byte = Mf_peek( encoder->matchfinder, -ahead-1 );
		const uint8_t cur_byte = Mf_peek( encoder->matchfinder, -ahead );
		CRC32_update_byte( &encoder->crc, cur_byte );
		if( St_is_char( *state ) )
		  LZe_encode_literal( encoder, prev_byte, cur_byte );
		else
		  {
		  const uint8_t match_byte =
			Mf_peek( encoder->matchfinder, -ahead-encoder->rep_distances[0]-1 );
		  LZe_encode_matched( encoder, prev_byte, cur_byte, match_byte );
		  }
		*state = St_set_char( *state );
		}
	  else					/* match or repeated match */
		{
		CRC32_update_buf( &encoder->crc, Mf_ptr_to_current_pos( encoder->matchfinder ) - ahead, len );
		LZe_mtf_reps( dis, encoder->rep_distances );
		bit = ( dis < num_rep_distances );
		Re_encode_bit( &encoder->renc, &encoder->bm_rep[*state], bit );
		if( bit )
		  {
		  bit = ( dis == 0 );
		  Re_encode_bit( &encoder->renc, &encoder->bm_rep0[*state], !bit );
		  if( bit )
			Re_encode_bit( &encoder->renc, &encoder->bm_len[*state][pos_state], len > 1 );
		  else
			{
			Re_encode_bit( &encoder->renc, &encoder->bm_rep1[*state], dis > 1 );
			if( dis > 1 )
			  Re_encode_bit( &encoder->renc, &encoder->bm_rep2[*state], dis > 2 );
			}
		  if( len == 1 ) *state = St_set_short_rep( *state );
		  else
			{
			Lee_encode( &encoder->rep_len_encoder, &encoder->renc, len, pos_state );
			*state = St_set_rep( *state );
			}
		  }
		else
		  {
		  LZe_encode_pair( encoder, dis - num_rep_distances, len, pos_state );
		  --encoder->fill_counter;
		  *state = St_set_match( *state );
		  }
		}
	  ahead -= len; i += len;
	  if( Re_member_position( &encoder->renc ) >= encoder->member_size_limit )
		{
		if( !Mf_dec_pos( encoder->matchfinder, ahead ) ) return false;
		if( LZe_full_flush( encoder, *state ) ) encoder->member_finished = true;
		return true;
		}
	  if( ahead <= 0 ) break;
	  }
	}
  if( LZe_full_flush( encoder, *state ) ) encoder->member_finished = true;
  return true;
  }

struct LZ_Encoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  struct Matchfinder * matchfinder;
  struct LZ_encoder * lz_encoder;
  enum LZ_Errno lz_errno;
  int flush_pending;
  File_header member_header;
  bool fatal;
  };

static void LZ_Encoder_init( struct LZ_Encoder * const e,
							 const File_header header )
  {
  int i;
  e->partial_in_size = 0;
  e->partial_out_size = 0;
  e->matchfinder = 0;
  e->lz_encoder = 0;
  e->lz_errno = LZ_ok;
  e->flush_pending = 0;
  for( i = 0; i < Fh_size; ++i ) e->member_header[i] = header[i];
  e->fatal = false;
  }

struct LZ_Decoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  struct Range_decoder * rdec;
  struct LZ_decoder * lz_decoder;
  enum LZ_Errno lz_errno;
  File_header member_header;		/* header of current member */
  bool fatal;
  bool seeking;
  };

static void LZ_Decoder_init( struct LZ_Decoder * const d )
  {
  int i;
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  d->rdec = 0;
  d->lz_decoder = 0;
  d->lz_errno = LZ_ok;
  for( i = 0; i < Fh_size; ++i ) d->member_header[i] = 0;
  d->fatal = false;
  d->seeking = false;
  }

static bool verify_encoder( struct LZ_Encoder * const e )
  {
  if( !e ) return false;
  if( !e->matchfinder || !e->lz_encoder )
	{ e->lz_errno = LZ_bad_argument; return false; }
  return true;
  }

static bool verify_decoder( struct LZ_Decoder * const d )
  {
  if( !d ) return false;
  if( !d->rdec )
	{ d->lz_errno = LZ_bad_argument; return false; }
  return true;
  }

/*------------------------- Misc Functions -------------------------*/

const char * LZ_version( void ) { return LZ_version_string; }

const char * LZ_strerror( const enum LZ_Errno lz_errno )
  {
  switch( lz_errno )
	{
	case LZ_ok            : return "ok";
	case LZ_bad_argument  : return "bad argument";
	case LZ_mem_error     : return "not enough memory";
	case LZ_sequence_error: return "sequence error";
	case LZ_header_error  : return "header error";
	case LZ_unexpected_eof: return "unexpected eof";
	case LZ_data_error    : return "data error";
	case LZ_library_error : return "library error";
	}
  return "invalid error code";
  }

int LZ_min_dictionary_bits( void ) { return min_dictionary_bits; }
int LZ_min_dictionary_size( void ) { return min_dictionary_size; }
int LZ_max_dictionary_bits( void ) { return max_dictionary_bits; }
int LZ_max_dictionary_size( void ) { return max_dictionary_size; }
int LZ_min_match_len_limit( void ) { return min_match_len_limit; }
int LZ_max_match_len_limit( void ) { return max_match_len; }

/*---------------------- Compression Functions ----------------------*/

struct LZ_Encoder * LZ_compress_open( const int dictionary_size,
									  const int match_len_limit,
									  const unsigned long long member_size )
  {
  File_header header;
  const bool error = ( !Fh_set_dictionary_size( header, dictionary_size ) ||
					   match_len_limit < min_match_len_limit ||
					   match_len_limit > max_match_len ||
					   member_size < min_dictionary_size );

  struct LZ_Encoder * const e =
	(struct LZ_Encoder *)malloc( sizeof (struct LZ_Encoder) );
  if( !e ) return 0;
  Fh_set_magic( header );
  LZ_Encoder_init( e, header );
  if( error ) e->lz_errno = LZ_bad_argument;
  else
	{
	e->matchfinder = (struct Matchfinder *)malloc( sizeof (struct Matchfinder) );
	if( e->matchfinder )
	  {
	  e->lz_encoder = (struct LZ_encoder *)malloc( sizeof (struct LZ_encoder) );
	  if( e->lz_encoder )
		{
		if( Mf_init( e->matchfinder,
					 Fh_get_dictionary_size( header ), match_len_limit ) )
		  {
		  if( LZe_init( e->lz_encoder, e->matchfinder, header, member_size ) )
			return e;
		  Mf_free( e->matchfinder );
		  }
		free( e->lz_encoder ); e->lz_encoder = 0;
		}
	  free( e->matchfinder ); e->matchfinder = 0;
	  }
	e->lz_errno = LZ_mem_error;
	}
  e->fatal = true;
  return e;
  }

int LZ_compress_close( struct LZ_Encoder * const e )
  {
  if( !e ) return -1;
  if( e->lz_encoder )
	{ LZe_free( e->lz_encoder ); free( e->lz_encoder ); }
  if( e->matchfinder )
   { Mf_free( e->matchfinder ); free( e->matchfinder ); }
  free( e );
  return 0;
  }

int LZ_compress_finish( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  Mf_finish( e->matchfinder );
  e->flush_pending = 0;
  /* if (open --> write --> finish) use same dictionary size as lzip. */
  /* this does not save any memory. */
  if( Mf_data_position( e->matchfinder ) == 0 &&
	  LZ_compress_total_out_size( e ) == Fh_size )
	Mf_adjust_distionary_size( e->matchfinder );
  return 0;
  }

int LZ_compress_restart_member( struct LZ_Encoder * const e,
								const unsigned long long member_size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( !LZe_member_finished( e->lz_encoder ) )
	{ e->lz_errno = LZ_sequence_error; return -1; }
  if( member_size < min_dictionary_size )
	{ e->lz_errno = LZ_bad_argument; return -1; }

  e->partial_in_size += Mf_data_position( e->matchfinder );
  e->partial_out_size += Re_member_position( &e->lz_encoder->renc );
  Mf_reset( e->matchfinder );

  LZe_free( e->lz_encoder );
  if( !LZe_init( e->lz_encoder, e->matchfinder, e->member_header, member_size ) )
	{
	LZe_free( e->lz_encoder ); free( e->lz_encoder ); e->lz_encoder = 0;
	e->lz_errno = LZ_mem_error; e->fatal = true;
	return -1;
	}
  e->lz_errno = LZ_ok;
  return 0;
  }

int LZ_compress_sync_flush( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( e->flush_pending <= 0 && !Mf_flushing_or_end( e->matchfinder ) )
	{
	e->flush_pending = 2;	/* 2 consecutive markers guarantee decoding */
	Mf_set_flushing( e->matchfinder, true );
	if( !LZe_encode_member( e->lz_encoder ) )
	  { e->lz_errno = LZ_library_error; e->fatal = true; return -1; }
	while( e->flush_pending > 0 && LZe_sync_flush( e->lz_encoder ) )
	  { if( --e->flush_pending <= 0 ) Mf_set_flushing( e->matchfinder, false ); }
	}
  return 0;
  }

int LZ_compress_read( struct LZ_Encoder * const e,
					  uint8_t * const buffer, const int size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( !LZe_encode_member( e->lz_encoder ) )
	{ e->lz_errno = LZ_library_error; e->fatal = true; return -1; }
  while( e->flush_pending > 0 && LZe_sync_flush( e->lz_encoder ) )
	{ if( --e->flush_pending <= 0 ) Mf_set_flushing( e->matchfinder, false ); }
  return Re_read_data( &e->lz_encoder->renc, buffer, size );
  }

int LZ_compress_write( struct LZ_Encoder * const e,
					   const uint8_t * const buffer, const int size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( e->flush_pending > 0 || size < 0 ) return 0;
  return Mf_write_data( e->matchfinder, buffer, size );
  }

int LZ_compress_write_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( e->flush_pending > 0 ) return 0;
  return Mf_free_bytes( e->matchfinder );
  }

enum LZ_Errno LZ_compress_errno( struct LZ_Encoder * const e )
  {
  if( !e ) return LZ_bad_argument;
  return e->lz_errno;
  }

int LZ_compress_finished( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return -1;
  return ( e->flush_pending <= 0 && Mf_finished( e->matchfinder ) &&
		   LZe_member_finished( e->lz_encoder ) );
  }

int LZ_compress_member_finished( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return -1;
  return LZe_member_finished( e->lz_encoder );
  }

unsigned long long LZ_compress_data_position( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return Mf_data_position( e->matchfinder );
  }

unsigned long long LZ_compress_member_position( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return Re_member_position( &e->lz_encoder->renc );
  }

unsigned long long LZ_compress_total_in_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return e->partial_in_size + Mf_data_position( e->matchfinder );
  }

unsigned long long LZ_compress_total_out_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return e->partial_out_size + Re_member_position( &e->lz_encoder->renc );
  }

/*--------------------- Decompression Functions ---------------------*/

struct LZ_Decoder * LZ_decompress_open( void )
  {
  struct LZ_Decoder * const d =
	(struct LZ_Decoder *)malloc( sizeof (struct LZ_Decoder) );
  if( !d ) return 0;
  LZ_Decoder_init( d );

  d->rdec = (struct Range_decoder *)malloc( sizeof (struct Range_decoder) );
  if( !d->rdec || !Rd_init( d->rdec ) )
	{
	if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); d->rdec = 0; }
	d->lz_errno = LZ_mem_error; d->fatal = true;
	}
  return d;
  }

int LZ_decompress_close( struct LZ_Decoder * const d )
  {
  if( !d ) return -1;
  if( d->lz_decoder )
	{ LZd_free( d->lz_decoder ); free( d->lz_decoder ); }
  if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); }
  free( d );
  return 0;
  }

int LZ_decompress_finish( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  if( d->seeking ) { d->seeking = false; Rd_purge( d->rdec ); }
  else Rd_finish( d->rdec );
  return 0;
  }

int LZ_decompress_reset( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  if( d->lz_decoder )
	{ LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  Rd_reset( d->rdec );
  d->lz_errno = LZ_ok;
  d->fatal = false;
  d->seeking = false;
  return 0;
  }

int LZ_decompress_sync_to_member( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  if( d->lz_decoder )
	{ LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  if( Rd_find_header( d->rdec ) ) d->seeking = false;
  else
	{
	if( !d->rdec->at_stream_end ) d->seeking = true;
	else { d->seeking = false; Rd_purge( d->rdec ); }
	}
  d->lz_errno = LZ_ok;
  d->fatal = false;
  return 0;
  }

int LZ_decompress_read( struct LZ_Decoder * const d,
						uint8_t * const buffer, const int size )
  {
  int result;
  if( !verify_decoder( d ) || d->fatal ) return -1;
  if( d->seeking ) return 0;

  if( d->lz_decoder && LZd_member_finished( d->lz_decoder ) )
	{
	d->partial_in_size += d->rdec->member_position;
	d->partial_out_size += LZd_data_position( d->lz_decoder );
	LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0;
	}
  if( !d->lz_decoder )
	{
	if( Rd_available_bytes( d->rdec ) < 5 + Fh_size )
	  {
	  if( !d->rdec->at_stream_end || Rd_finished( d->rdec ) ) return 0;
	  /* set position at EOF */
	  d->partial_in_size += Rd_available_bytes( d->rdec );
	  if( Rd_available_bytes( d->rdec ) <= Fh_size ||
		  !Rd_read_header( d->rdec, d->member_header ) )
		d->lz_errno = LZ_header_error;
	  else
		d->lz_errno = LZ_unexpected_eof;
	  d->fatal = true;
	  return -1;
	  }
	if( !Rd_read_header( d->rdec, d->member_header ) )
	  {
	  d->lz_errno = LZ_header_error;
	  d->fatal = true;
	  return -1;
	  }
	d->lz_decoder = (struct LZ_decoder *)malloc( sizeof (struct LZ_decoder) );
	if( !d->lz_decoder || !LZd_init( d->lz_decoder, d->member_header, d->rdec ) )
	  {					/* not enough free memory */
	  if( d->lz_decoder )
		{ LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
	  d->lz_errno = LZ_mem_error;
	  d->fatal = true;
	  return -1;
	  }
	}
  result = LZd_decode_member( d->lz_decoder );
  if( result != 0 )
	{
	if( result == 2 ) d->lz_errno = LZ_unexpected_eof;
	else d->lz_errno = LZ_data_error;
	d->fatal = true;
	return -1;
	}
  return Cb_read_data( &d->lz_decoder->cb, buffer, size );
  }

int LZ_decompress_write( struct LZ_Decoder * const d,
						 const uint8_t * const buffer, const int size )
  {
  int result;
  if( !verify_decoder( d ) || d->fatal ) return -1;

  result = Rd_write_data( d->rdec, buffer, size );
  while( d->seeking )
	{
	int size2;
	if( Rd_find_header( d->rdec ) ) d->seeking = false;
	if( result >= size ) break;
	size2 = Rd_write_data( d->rdec, buffer + result, size - result );
	if( size2 > 0 ) result += size2;
	else break;
	}
  return result;
  }

int LZ_decompress_write_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  return Rd_free_bytes( d->rdec );
  }

enum LZ_Errno LZ_decompress_errno( struct LZ_Decoder * const d )
  {
  if( !d ) return LZ_bad_argument;
  return d->lz_errno;
  }

int LZ_decompress_finished( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return ( Rd_finished( d->rdec ) &&
		   ( !d->lz_decoder || LZd_member_finished( d->lz_decoder ) ) );
  }

int LZ_decompress_member_finished( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return ( d->lz_decoder && LZd_member_finished( d->lz_decoder ) );
  }

int LZ_decompress_member_version( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return Fh_version( d->member_header );
  }

int LZ_decompress_dictionary_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return Fh_get_dictionary_size( d->member_header );
  }

unsigned LZ_decompress_data_crc( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
	return LZd_crc( d->lz_decoder );
  return 0;
  }

unsigned long long LZ_decompress_data_position( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
	return LZd_data_position( d->lz_decoder );
  return 0;
  }

unsigned long long LZ_decompress_member_position( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
	return d->rdec->member_position;
  return 0;
  }

unsigned long long LZ_decompress_total_in_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return 0;
  if( d->lz_decoder )
	return d->partial_in_size + d->rdec->member_position;
  return d->partial_in_size;
  }

unsigned long long LZ_decompress_total_out_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return 0;
  if( d->lz_decoder )
	return d->partial_out_size + LZd_data_position( d->lz_decoder );
  return d->partial_out_size;
  }


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
  { 0x80000000, 1, 2, { 26, 24, 24, 24, 24, 24, 24, 24 }, { 15, 3, 0, 0, 0, 0, 0, 0 }, 0xc0, 0x80 },
  { 0xc0000000, 2, 4, { 25, 22, 19, 16, 16, 16, 16, 16 }, { 15, 7, 7, 7, 0, 0, 0, 0 }, 0xe0, 0xc0 },
  { 0xe0000000, 4, 8, { 23, 19, 15, 11, 8, 5, 2, 0 }, { 31, 15, 15, 15, 7, 7, 7, 3 }, 0xf0, 0xe0 }
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


#line 1 "bundle.cpp"
/*
 * Simple compression interface.
 * Copyright (c) 2013, 2014, Mario 'rlyeh' Rodriguez

 * wire::eval() based on code by Peter Kankowski (see http://goo.gl/Kx6Oi)
 * wire::format() based on code by Adam Rosenfield (see http://goo.gl/XPnoe)
 * wire::format() based on code by Tom Distler (see http://goo.gl/KPT66)

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
  /* Compresses 'size' bytes from 'data'. Returns the address of a
	 malloc'd buffer containing the compressed data and its size in
	 '*out_sizep'.
	 In case of error, returns 0 and does not modify '*out_sizep'.
  */
  uint8_t * lzip_compress( const uint8_t * const data, const int size,
						uint8_t * const new_data, int * const out_sizep )
	{
	struct LZ_Encoder * encoder;
  //uint8_t * new_data;
	const int match_len_limit = 36;
	const unsigned long long member_size = (unsigned long long)~0;
	int delta_size, new_data_size;
	int new_pos = 0;
	int written = 0;
	bool error = false;
	int dict_size = 8 << 20;              /* 8 MiB */

	if( dict_size > size ) dict_size = size;      /* saves memory */
	if( dict_size < LZ_min_dictionary_size() )
	  dict_size = LZ_min_dictionary_size();
	encoder = LZ_compress_open( dict_size, match_len_limit, member_size );
	if( !encoder || LZ_compress_errno( encoder ) != LZ_ok )
	  { LZ_compress_close( encoder ); return 0; }

	delta_size = (size < 256) ? 64 : size / 4;        /* size may be zero */
  //  new_data_size = delta_size;               /* initial size */
	new_data_size = int(*out_sizep);

  //  new_data = (uint8_t *)malloc( new_data_size );
	if( !new_data )
	  { LZ_compress_close( encoder ); return 0; }

	while( true )
	  {
	  int rd;
	  if( LZ_compress_write_size( encoder ) > 0 )
		{
		if( written < size )
		  {
		  const int wr = LZ_compress_write( encoder, data + written,
											size - written );
		  if( wr < 0 ) { error = true; break; }
		  written += wr;
		  }
		if( written >= size ) LZ_compress_finish( encoder );
		}
	  rd = LZ_compress_read( encoder, new_data + new_pos,
							 new_data_size - new_pos );
	  if( rd < 0 ) { error = true; break; }
	  new_pos += rd;
	  if( LZ_compress_finished( encoder ) == 1 ) break;
	  if( new_pos >= new_data_size )
		{
  #if 1
		  error = true; break;
  #else
		uint8_t * const tmp =
		  (uint8_t *)realloc( new_data, new_data_size + delta_size );
		if( !tmp ) { error = true; break; }
		new_data = tmp;
		new_data_size += delta_size;
  #endif
		}
	  }

	if( LZ_compress_close( encoder ) < 0 ) error = true;
	if( error ) { /*free( new_data );*/ return 0; }
	*out_sizep = new_pos;
	return new_data;
	}

  /* Decompresses 'size' bytes from 'data'. Returns the address of a
	 malloc'd buffer containing the decompressed data and its size in
	 '*out_sizep'.
	 In case of error, returns 0 and does not modify '*out_sizep'.
  */
  uint8_t * lzip_decompress( const uint8_t * const data, const int size,
						  uint8_t * const new_data, int * const out_sizep )
	{
	struct LZ_Decoder * const decoder = LZ_decompress_open();
  //uint8_t * new_data;
	const int delta_size = size;          /* size must be > zero */
  //  int new_data_size = delta_size;       /* initial size */
	int new_data_size = int(*out_sizep);
	int new_pos = 0;
	int written = 0;
	bool error = false;
	if( !decoder || LZ_decompress_errno( decoder ) != LZ_ok )
	  { LZ_decompress_close( decoder ); return 0; }

	//new_data = (uint8_t *)malloc( new_data_size );
	if( !new_data )
	  { LZ_decompress_close( decoder ); return 0; }

	while( true )
	  {
	  int rd;
	  if( LZ_decompress_write_size( decoder ) > 0 )
		{
		if( written < size )
		  {
		  const int wr = LZ_decompress_write( decoder, data + written,
											  size - written );
		  if( wr < 0 ) { error = true; break; }
		  written += wr;
		  }
		if( written >= size ) LZ_decompress_finish( decoder );
		}
	  rd = LZ_decompress_read( decoder, new_data + new_pos,
							   new_data_size - new_pos );
	  if( rd < 0 ) { error = true; break; }
	  new_pos += rd;
	  if( LZ_decompress_finished( decoder ) == 1 ) break;
	  if( new_pos >= new_data_size )
		{
  #if 1
		  error = true; break;
  #else
		uint8_t * const tmp =
		  (uint8_t *)realloc( new_data, new_data_size + delta_size );
		if( !tmp ) { error = true; break; }
		new_data = tmp;
		new_data_size += delta_size;
  #endif
		}
	  }

	if( LZ_decompress_close( decoder ) < 0 ) error = true;
	if( error ) { /*free( new_data );*/ return 0; }
	*out_sizep = new_pos;
	return new_data;
	}
}

namespace bundle {

	enum { verbose = false };

	std::string hexdump( const std::string &str );

	namespace
	{
		void shout( unsigned q, const char *context, size_t from, size_t to ) {
			if( verbose ) {
				std::cout << context << " q:" << q << ",from:" << from << ",to:" << to << std::endl;
			}
		}
	}

	const char *const nameof( unsigned q ) {
		switch( q ) {
			break; default : return "NONE";
			break; case LZ4: return "LZ4";
			break; case MINIZ: return "MINIZ";
			break; case SHOCO: return "SHOCO";
			break; case LZLIB: return "LZLIB";
			/* for archival reasons: */
			// break; case LZHAM: return "LZHAM";
		}
	}

	const char *const versionof( unsigned q ) {
		return "0";
	}

	const char *const extof( unsigned q ) {
		switch( q ) {
			break; default : return "unc";
			break; case LZ4: return "lz4";
			break; case MINIZ: return "miniz";
			break; case SHOCO: return "shoco";
			break; case LZLIB: return "lz";
			/* for archival reasons: */
			// break; case LZHAM: return "lzham";
		}
	}

	unsigned typeof( const void *ptr, size_t size ) {
		unsigned char *mem = (unsigned char *)ptr;
		//std::string s; s.resize( size ); memcpy( &s[0], mem, size );
		//std::cout << hexdump( s) << std::endl;
		if( size >= 4 && mem && mem[0] == 'L' && mem[1] == 'Z' && mem[2] == 'I' && mem[3] == 'P' ) return LZLIB;
		if( size >= 1 && mem && mem[0] == 0xEC ) return MINIZ;
		if( size >= 1 && mem && mem[0] >= 0xF0 ) return LZ4;
		return NONE;
	}

	size_t bound( unsigned q, size_t len ) {
		size_t zlen = len;
		switch( q ) {
			break; default : zlen *= 2;
			break; case LZ4: zlen = LZ4_compressBound((int)(len));
		}
		return shout( q, "[bound]", len, zlen ), zlen;
	}

	  bool pack( unsigned q, const char *in, size_t inlen, char *out, size_t &outlen ) {
		bool ok = false;
		if( in && inlen && out && outlen >= inlen ) {
			ok = true;
			switch( q ) {
				break; default: ok = false;
				break; case LZ4: outlen = LZ4_compress( in, out, inlen );
				break; case MINIZ: outlen = tdefl_compress_mem_to_mem( out, outlen, in, inlen, TDEFL_DEFAULT_MAX_PROBES ); //TDEFL_MAX_PROBES_MASK ); //
				break; case SHOCO: outlen = shoco_compress( in, inlen, out, outlen );
				break; case LZLIB: { int l; lzip_compress( (const uint8_t *)in, inlen, (uint8_t *)out, &l ); outlen = l; }
				/* for archival reasons: */
				// break; case LZHAM: { lzham_z_ulong l; lzham_z_compress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); outlen = l; }
			}
			// std::cout << nameof( typeof( out, outlen ) ) << std::endl;
		}
		ok = ok && outlen > 0 && outlen < inlen;
		outlen = ok && outlen ? outlen : 0;
		return shout( q, "[pack]", inlen, outlen ), ok;
	  }

	bool unpack( unsigned q, const char *in, size_t inlen, char *out, size_t &outlen ) {
		bool ok = false;
		size_t bytes_read = 0;
		if( in && inlen && out && outlen ) {
			ok = true;
			switch( q ) {
				break; default: ok = false;
				break; case LZ4: bytes_read = LZ4_uncompress( in, out, outlen );
				break; case MINIZ: bytes_read = inlen; tinfl_decompress_mem_to_mem( out, outlen, in, inlen, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF );
				break; case SHOCO: bytes_read = inlen; shoco_decompress( in, inlen, out, outlen );
				break; case LZLIB: bytes_read = inlen; { int l = outlen; lzip_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, &l ); outlen = l; }
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
	// compression methods

	namespace {
		enum {
			OFFSET_MARK = 0,
			OFFSET_LEN = 1,
			OFFSET_ZLEN = 5,
			OFFSET_TYPE = 9,
			OFFSET_ZDATA = 10
		};
	}

	bool is_packed( const std::string &self ) {
		return self.size() > 0 && self.at(0) == '\0';
	}
	bool is_unpacked( const std::string &self ) {
		return !is_packed( self );
	}

	unsigned typeof( const std::string &self ) {
		return is_packed( self ) ? self[ OFFSET_TYPE ] : NONE;
	}
	std::string nameof( const std::string &self ) {
		return bundle::nameof( typeof(self) );
	}
	std::string versionof( const std::string &self ) {
		return bundle::versionof( typeof(self) );
	}
	std::string extof( const std::string &self ) {
		return bundle::extof( typeof(self) );
	}

	size_t length( const std::string &self ) {
		uint32_t size = self.size();
		if( is_packed(self) ) {
			size  = ((unsigned char)self[OFFSET_LEN + 0]) << 24;
			size |= ((unsigned char)self[OFFSET_LEN + 1]) << 16;
			size |= ((unsigned char)self[OFFSET_LEN + 2]) <<  8;
			size |= ((unsigned char)self[OFFSET_LEN + 3]) <<  0;
		}
		return size;
	}
	size_t zlength( const std::string &self ) {
		uint32_t size = 0;
		if( is_packed(self) ) {
			size  = ((unsigned char)self[OFFSET_ZLEN + 0]) << 24;
			size |= ((unsigned char)self[OFFSET_ZLEN + 1]) << 16;
			size |= ((unsigned char)self[OFFSET_ZLEN + 2]) <<  8;
			size |= ((unsigned char)self[OFFSET_ZLEN + 3]) <<  0;
		}
		return size;
	}

	void *zptr( const std::string &self ) {
		return is_packed( self ) ? (void *)&self[OFFSET_ZDATA] : (void *)0;
	}

	std::string pack( unsigned q, const std::string &self ) {
		if( is_packed( self ) )
			return self;

		if( !self.size() )
			return self;

		std::vector<char> binary;
		std::string input = self;

		// compress

		if( !pack( q, binary, input ) )
			return self;

		// encapsulated by exploiting std::string design (portable? standard?)
		// should i encapsulate by escaping characters instead? (safer but slower)

		uint32_t size1 = (uint32_t)input.size(), size2 = (uint32_t)binary.size();
		std::string output( 10 + size2, '\0' );

		output[1] = (unsigned char)(( size1 >> 24 ) & 0xff );
		output[2] = (unsigned char)(( size1 >> 16 ) & 0xff );
		output[3] = (unsigned char)(( size1 >>  8 ) & 0xff );
		output[4] = (unsigned char)(( size1 >>  0 ) & 0xff );

		output[5] = (unsigned char)(( size2 >> 24 ) & 0xff );
		output[6] = (unsigned char)(( size2 >> 16 ) & 0xff );
		output[7] = (unsigned char)(( size2 >>  8 ) & 0xff );
		output[8] = (unsigned char)(( size2 >>  0 ) & 0xff );

		output[9] = (unsigned char)( q & 0xff );

		std::memcpy( &output.at( 10 ), &binary.at( 0 ), size2 );

		return output;
	}

	std::string unpack( const std::string &self ) {
		if( is_packed( self ) )
		{
			// decapsulate
			uint32_t size1 = uint32_t( length( self ) ), size2 = uint32_t( zlength( self ) );
			unsigned Q = typeof( self );

			std::string output( size1, '\0' );

			std::vector<char> content( size2 );
			std::memcpy( &content.at( 0 ), zptr( self ), size2 );

			// decompress
			if( unpack( Q, output, content ) )
				return output;
		}

		return self;
	}
}

namespace bundle
{
	static std::string hexdump( const std::string &str ) {
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

