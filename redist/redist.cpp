/** This is an auto-generated file. Do not edit.
 */

#include "bundle.h"

// here comes the fun. take a coffee {

// ensure some libs evaluate the wrong directive too {
#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif
// }

// disable MSVC warnings {
#if defined(_WIN32)
#pragma warning(disable:4334)
#pragma warning(disable:4309)
#pragma warning(disable:4267)
#pragma warning(disable:4244)
#pragma warning(disable:4101)
#pragma warning(disable:4800)
#pragma warning(disable:4065)
#pragma warning(disable:4018)
#endif
// }

// ensure no other lib is polluting min/max macros at any point { 
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#endif
#include <algorithm>
// }

// same for OMP {
#ifdef _OPENMP
#   include <omp.h>
#endif
// }

// exclude c++11 libs
#if BUNDLE_USE_CXX11 == 0
#   ifndef BUNDLE_NO_ZMOLLY
#       define BUNDLE_NO_ZMOLLY
#   endif
#   ifndef BUNDLE_NO_MCM
#       define BUNDLE_NO_MCM
#   endif
#endif

// exclude licenses
#ifdef BUNDLE_NO_UNLICENSE
#   ifndef BUNDLE_NO_BCM
#       define BUNDLE_NO_BCM
#   endif
#   ifndef BUNDLE_NO_CSC
#       define BUNDLE_NO_CSC
#   endif
#   ifndef BUNDLE_NO_MINIZ
#       define BUNDLE_NO_MINIZ
#   endif
#   ifndef BUNDLE_NO_ZPAQ
#       define BUNDLE_NO_ZPAQ
#   endif
#   ifndef BUNDLE_NO_LZIP
#       define BUNDLE_NO_LZIP
#   endif
#   ifndef BUNDLE_NO_LZMA
#       define BUNDLE_NO_LZMA
#   endif
#   ifndef BUNDLE_NO_CRUSH
#       define BUNDLE_NO_CRUSH
#   endif
#endif

#ifdef BUNDLE_NO_CDDL
#   ifndef BUNDLE_NO_LZJB
#       define BUNDLE_NO_LZJB
#   endif
#endif

#ifdef BUNDLE_NO_BSD2
#   ifndef BUNDLE_NO_ZSTD
#       define BUNDLE_NO_ZSTD
#   endif
#   ifndef BUNDLE_NO_LZ4
#       define BUNDLE_NO_LZ4
#   endif
#endif

#ifdef BUNDLE_NO_BSD3
#   ifndef BUNDLE_NO_ZLING
#       define BUNDLE_NO_ZLING
#   endif
#   ifndef BUNDLE_NO_ZMOLLY
#       define BUNDLE_NO_ZMOLLY
#   endif
#   ifndef BUNDLE_NO_SHRINKER
#       define BUNDLE_NO_SHRINKER
#   endif
#   ifndef BUNDLE_NO_BZIP2
#       define BUNDLE_NO_BZIP2
#   endif
#endif

#ifdef BUNDLE_NO_MIT
#   ifndef BUNDLE_NO_SHOCO
#       define BUNDLE_NO_SHOCO
#   endif
#endif

#ifdef BUNDLE_NO_APACHE2
#   ifndef BUNDLE_NO_BSC
#       define BUNDLE_NO_BSC
#   endif
#   ifndef BUNDLE_NO_BROTLI
#       define BUNDLE_NO_BROTLI
#   endif
#endif

#ifdef BUNDLE_NO_GPL
#   ifndef BUNDLE_NO_TANGELO
#       define BUNDLE_NO_TANGELO
#   endif
#   ifndef BUNDLE_NO_MCM
#       define BUNDLE_NO_MCM
#   endif
#endif

// mutual exclusion
#if defined(BUNDLE_NO_LZMA) && !defined(BUNDLE_NO_LZIP)
#   define BUNDLE_NO_LZIP
#endif

// brotli
#ifndef BUNDLE_NO_BROTLI
#include "deps/brotli/enc/backward_references.cc"
#include "deps/brotli/enc/block_splitter.cc"
#include "deps/brotli/enc/brotli_bit_stream.cc"
#include "deps/brotli/enc/dictionary.cc"
#include "deps/brotli/enc/encode.cc"
//#include "deps/brotli/enc/encode_parallel.cc"
#include "deps/brotli/enc/entropy_encode.cc"
#include "deps/brotli/enc/histogram.cc"
#include "deps/brotli/enc/literal_cost.cc"
#include "deps/brotli/enc/metablock.cc"
#include "deps/brotli/enc/static_dict.cc"
#include "deps/brotli/enc/streams.cc"
#include "deps/brotli/enc/utf8_util.cc"
#define kBrotliDictionary                 kBrotliDictionary_Dec
#define kBrotliDictionaryOffsetsByLength  kBrotliDictionaryOffsetsByLength_Dec
#define kBrotliDictionarySizeBitsByLength kBrotliDictionarySizeBitsByLength_Dec
#define kBrotliMinDictionaryWordLength    kBrotliMinDictionaryWordLength_Dec
#define kBrotliMaxDictionaryWordLength    kBrotliMaxDictionaryWordLength_Dec
#include "deps/brotli/dec/bit_reader.c"
#include "deps/brotli/dec/decode.c"
#include "deps/brotli/dec/dictionary.c"
#include "deps/brotli/dec/huffman.c"
#include "deps/brotli/dec/state.c"
#include "deps/brotli/dec/streams.c"
#endif

// lzham
#if 0
#ifndef BUNDLE_NO_LZHAM
#define LZHAM_NO_ZLIB_COMPATIBLE_NAMES
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
#endif

// lzp1
#if 0
#include "deps/lzp1/lzp1.hpp"
#endif

#ifndef BUNDLE_NO_SHOCO
#include "deps/shoco/shoco.h"
#endif

#ifndef BUNDLE_NO_LZIP
#ifndef BUNDLE_NO_LZMA
#include "deps/easylzma/src/easylzma/compress.h"
#include "deps/easylzma/src/easylzma/decompress.h"
#endif
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif

#ifndef BUNDLE_NO_ZPAQ
#include "deps/zpaq/libzpaq.h"
#endif

// miniz
#ifndef BUNDLE_NO_MINIZ
#define MINIZ_NO_STDIO 1
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
//#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
//#define MINIZ_HAS_64BIT_REGISTERS 1
#include "deps/miniz/miniz.c"
#endif

// lz4, which defines 'inline' and 'restrict' which is later required by shoco
#ifndef BUNDLE_NO_LZ4
#include "deps/lz4/lib/lz4.c"
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
#define LZ4_NbCommonBytes LZ4_NbCommonBytes_hc
#define limitedOutput limitedOutput_hc
#define limitedOutput_directive limitedOutput_directive_hc
#define LZ4_stream_t LZ4_stream_t_hc
#define LZ4_streamDecode_t LZ4_streamDecode_t_hc
#define LZ4_resetStream LZ4_resetStream_hc
#define LZ4_createStream LZ4_createStream_hc
#define LZ4_freeStream LZ4_freeStream_hc
#define LZ4_loadDict LZ4_loadDict_hc
#define LZ4_compress_continue LZ4_compress_continue_hc
#define LZ4_compress_limitedOutput_continue LZ4_compress_limitedOutput_continue_hc
#define LZ4_saveDict LZ4_saveDict_hc
#define LZ4_setStreamDecode LZ4_setStreamDecode_hc
#define LZ4_streamDecode_t2 LZ4_streamDecode_t2_hc
#define LZ4_createStreamDecode LZ4_createStreamDecode_hc
#define LZ4_freeStreamDecode LZ4_freeStreamDecode_hc
#define LZ4_decompress_safe_continue LZ4_decompress_safe_continue_hc
#define LZ4_decompress_fast_continue LZ4_decompress_fast_continue_hc
#define LZ4_64bits LZ4_64bits_hc
#define LZ4_isLittleEndian LZ4_isLittleEndian_hc
#define LZ4_read16 LZ4_read16_hc
#define LZ4_readLE16 LZ4_readLE16_hc
#define LZ4_writeLE16 LZ4_writeLE16_hc
#define LZ4_read32 LZ4_read32_hc
#define LZ4_read64 LZ4_read64_hc
#define LZ4_read_ARCH LZ4_read_ARCH_hc
#define LZ4_copy4 LZ4_copy4_hc
#define LZ4_copy8 LZ4_copy8_hc
#define LZ4_wildCopy LZ4_wildCopy_hc
#define LZ4_minLength LZ4_minLength_hc
#define LZ4_count LZ4_count_hc
#define prime5bytes prime5bytes_hc
#include "deps/lz4/lib/lz4hc.c"
#undef KB
#undef MB
#undef MAX_DISTANCE
#undef FORCE_INLINE
#endif

// shoco
#ifndef BUNDLE_NO_SHOCO
#include "deps/shoco/shoco.c"
#endif

// easylzma
#ifndef BUNDLE_NO_LZIP
#ifndef BUNDLE_NO_LZMA
#include "deps/easylzma/src/common_internal.c"
#include "deps/easylzma/src/compress.c"
#include "deps/easylzma/src/decompress.c"
#include "deps/easylzma/src/lzip_header.c"
#include "deps/easylzma/src/lzma_header.c"
#endif
#endif

// lzma sdk
#ifndef BUNDLE_NO_LZMA
#define _7ZIP_ST
#include "deps/easylzma/src/pavlov/7zCrc.c"
#include "deps/easylzma/src/pavlov/Alloc.c"
#include "deps/easylzma/src/pavlov/Bra.c"
#include "deps/easylzma/src/pavlov/Bra86.c"
#include "deps/easylzma/src/pavlov/BraIA64.c"
#include "deps/easylzma/src/pavlov/LzFind.c"
#include "deps/easylzma/src/pavlov/LzmaDec.c"
#include "deps/easylzma/src/pavlov/LzmaEnc.c"
#include "deps/easylzma/src/pavlov/LzmaLib.c"
#undef NORMALIZE
#undef IF_BIT_0
#undef UPDATE_0
#undef UPDATE_1
#undef kNumFullDistances
#undef kTopValue
#include "deps/easylzma/src/pavlov/Bcj2.c"
#endif

// zpaq
#ifndef BUNDLE_NO_ZPAQ
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
#include "deps/zpaq/libzpaq.cpp"
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
#endif

// bsc
#ifndef BUNDLE_NO_BSC
#pragma comment(lib, "Advapi32.lib")
//#define LIBBSC_SORT_TRANSFORM_SUPPORT 1
#ifdef INLINE
#undef INLINE
#endif
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
#endif

#ifndef BUNDLE_NO_BCM
#include "deps/bcm/bcm.cpp"
#ifdef _fseeki64
#undef _fseeki64
#endif
#ifdef _ftelli64
#undef _ftelli64
#endif
#endif

#ifndef BUNDLE_NO_BZIP2
// bzip2
#define BZ_NO_STDIO
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef GET_BIT
#undef GET_BIT
#endif
#define Bool Bool2
#define UInt64 UInt642
extern "C" void bz_internal_error(int errcode) {
    exit(-1);
}
#include "deps/bzip2/bzlib.h"
#include "deps/bzip2/crctable.c"
#include "deps/bzip2/compress.c"
#include "deps/bzip2/decompress.c"
#include "deps/bzip2/blocksort.c"
#ifdef SET_BINARY_MODE
#undef SET_BINARY_MODE
#endif
#include "deps/bzip2/bzlib.c"
#include "deps/bzip2/randtable.c"
#include "deps/bzip2/huffman.c"
#endif

//#include "deps/yappy/yappy.h"
//#include "deps/yappy/yappy.cpp"

//
#ifdef swap
#undef swap
#endif

#ifndef BUNDLE_NO_ZSTD
#ifdef MIN
#undef MIN
#endif
#ifdef MAX
#undef MAX
#endif
#ifdef ERROR
#undef ERROR
#endif
#undef HASH_MASK
#undef HASH_LOG
#define BYTE BYTE_zstd
#define U16  U16_zstd
#define S16  S16_zstd
#define U32  U32_zstd
#define S32  S32_zstd
#define U64  U64_zstd
#define S64  S64_zstd
#define ZSTD_LEGACY_SUPPORT 0
#include "deps/zstd/lib/fse.h"
#include "deps/zstd/lib/fse.c"
#include "deps/zstd/lib/huff0.h"
#include "deps/zstd/lib/huff0.c"
#include "deps/zstd/lib/zstd.h"
#include "deps/zstd/lib/zstd.c"
#ifdef KB
#undef KB
#endif
#ifdef MB
#undef MB
#endif
#ifdef GB
#undef GB
#endif
#ifdef MAXD_LOG
#undef MAXD_LOG
#endif
#include "deps/zstd/lib/zstdhc.h"
#include "deps/zstd/lib/zstdhc.c"
#endif

#ifndef BUNDLE_NO_SHRINKER
#include "deps/shrinker/shrinker.h"
#include "deps/shrinker/shrinker.c"
#endif

#ifndef BUNDLE_NO_CSC
#undef MIN
#undef MAX
#undef KB
#undef MB
#define KB KB2
#define MB MB2
#define _7Z_TYPES
#include "deps/libcsc/csc_types.h"
#include "deps/libcsc/csc_analyzer.h"
#include "deps/libcsc/csc_coder.h"
#include "deps/libcsc/csc_common.h"
#include "deps/libcsc/csc_dec.h"
#include "deps/libcsc/csc_enc.h"
#include "deps/libcsc/csc_encoder_main.h"
#include "deps/libcsc/csc_filters.h"
#include "deps/libcsc/csc_lz.h"
#include "deps/libcsc/csc_memio.h"
#include "deps/libcsc/csc_mf.h"
#include "deps/libcsc/csc_model.h"
#include "deps/libcsc/csc_profiler.h"
#include "deps/libcsc/csc_typedef.h"
#include "deps/libcsc/csc_analyzer.cpp"
#include "deps/libcsc/csc_coder.cpp"
#include "deps/libcsc/csc_dec.cpp"
#include "deps/libcsc/csc_filters.cpp"
#include "deps/libcsc/csc_lz.cpp"
#include "deps/libcsc/csc_memio.cpp"
#include "deps/libcsc/csc_mf.cpp"
#include "deps/libcsc/csc_model.cpp"
#include "deps/libcsc/csc_profiler.cpp"
#define CSCInstance CSCInstance_Enc
#include "deps/libcsc/csc_enc.cpp"
#include "deps/libcsc/csc_encoder_main.cpp"
#include "deps/libcsc/csc_default_alloc.cpp"
#endif

#ifndef BUNDLE_NO_MCM
#ifdef KB
#undef KB
#endif
#ifdef MB
#undef MB
#endif
#ifdef GB
#undef GB
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef CM
#undef CM
#endif
#define KB2 KB2_MCM
#define MB2 MB2_MCM
#define GB2 GB2_MCM
#define Analyzer Analyzer_MCM
#define CM CM_MCM
#ifdef LZ
#undef LZ
#endif
#define LZ LZ_MCM
#define MatchFinder MatchFinder_MCM
#ifdef kBlockSize
#undef kBlockSize
#endif
#define kBlockSize_MCM
#ifdef kNumStates
#undef kNumStates
#endif
#define kNumStates kNumStates_MCM
#include "deps/mcm/Archive.cpp"
#include "deps/mcm/CM.cpp"
#include "deps/mcm/Compressor.cpp"
//#include "deps/mcm/FilterTest.cpp"
#include "deps/mcm/Huffman.cpp"
#include "deps/mcm/LZ.cpp"
#include "deps/mcm/MCM.cpp"
#include "deps/mcm/Memory.cpp"
#include "deps/mcm/Util.cpp"
#endif

#ifndef BUNDLE_NO_ZLING
#ifdef kMatchMinLen
#undef kMatchMinLen
#endif
#define kMatchMinLen kMatchMinLen_zling
#ifdef kMatchMaxLen
#undef kMatchMaxLen
#endif
#define kMatchMaxLen kMatchMaxLen_zling
#ifdef kBlockSize
#undef kBlockSize
#endif
#define kBlockSize kBlockSize_zling
#include "deps/libzling/libzling/libzling.cpp"
#include "deps/libzling/libzling/libzling_huffman.cpp"
#include "deps/libzling/libzling/libzling_lz.cpp"
#include "deps/libzling/libzling/libzling_utils.cpp"
#endif

#ifndef BUNDLE_NO_TANGELO
#include "deps/tangelo/tangelo.cpp"
#endif

#ifndef BUNDLE_NO_ZMOLLY
#include "deps/zmolly/0.0.1/zmolly.cpp"
#endif

#ifndef BUNDLE_NO_CRUSH
#include "deps/crush/crush.cpp"
#endif

#ifndef BUNDLE_NO_LZJB
#include "deps/lzjb/lzjb2010.c"
#endif

#if 0
#ifndef BUNDLE_NO_BZIP2
extern "C" {
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef GET_BIT
#undef GET_BIT
#endif
#define Bool Bool2
#include "deps/bzip2/bzlib.c"
#include "deps/bzip2/compress.c"
#include "deps/bzip2/decompress.c"
#include "deps/bzip2/crctable.c"
#include "deps/bzip2/blocksort.c"
#include "deps/bzip2/huffman.c"
#include "deps/bzip2/randtable.c"
}
#endif
#endif

#include "deps/endian/endian.h"

// } end of coffee

#include "bundle.cpp"
