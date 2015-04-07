/** this is an amalgamated file. do not edit.
 */

// headers
#include "bundle.hpp"

// brotli
#include "deps/brotli/enc/backward_references.cc"
#include "deps/brotli/enc/block_splitter.cc"
#include "deps/brotli/enc/brotli_bit_stream.cc"
#include "deps/brotli/enc/encode.cc"
//#include "deps/brotli/enc/encode_parallel.cc"
#include "deps/brotli/enc/entropy_encode.cc"
#include "deps/brotli/enc/histogram.cc"
#include "deps/brotli/enc/literal_cost.cc"
#include "deps/brotli/enc/metablock.cc"
#define kBrotliDictionary                 kBrotliDictionary2
#define kBrotliDictionaryOffsetsByLength  kBrotliDictionaryOffsetsByLength2
#define kBrotliDictionarySizeBitsByLength kBrotliDictionarySizeBitsByLength2
#define kMaxDictionaryWordLength          kMaxDictionaryWordLength2
#define kMinDictionaryWordLength          kMinDictionaryWordLength2
#include "deps/brotli/dec/bit_reader.c"
#include "deps/brotli/dec/decode.c"
#include "deps/brotli/dec/huffman.c"
#include "deps/brotli/dec/safe_malloc.c"
#include "deps/brotli/dec/state.c"
#include "deps/brotli/dec/streams.c"


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

#if 0
#include "deps/lzp1/lzp1.hpp"
#endif
//#include "deps/lz4/lz4.h"
//#include "deps/lz4/lz4hc.h"
#include "deps/shoco/shoco.h"
#include "deps/easylzma/src/easylzma/compress.h"
#include "deps/easylzma/src/easylzma/decompress.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "deps/zpaq/libzpaq.h"

#if 1
// miniz
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
//#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
//#define MINIZ_HAS_64BIT_REGISTERS 1
#include "deps/miniz/miniz.c"
#endif

#if 1
// lz4, which defines 'inline' and 'restrict' which is later required by shoco
#include "deps/lz4/lz4.c"
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
#define LZ4_stream_t LZ4_stream_t2
#define LZ4_streamDecode_t LZ4_streamDecode_t2
#define LZ4_resetStream LZ4_resetStream2
#define LZ4_createStream LZ4_createStream2
#define LZ4_freeStream LZ4_freeStream2
#define LZ4_loadDict LZ4_loadDict2
#define LZ4_compress_continue LZ4_compress_continue2
#define LZ4_compress_limitedOutput_continue LZ4_compress_limitedOutput_continue2
#define LZ4_saveDict LZ4_saveDict2
#define LZ4_setStreamDecode LZ4_setStreamDecode2
#define LZ4_streamDecode_t2 LZ4_streamDecode_t22
#define LZ4_createStreamDecode LZ4_createStreamDecode2
#define LZ4_freeStreamDecode LZ4_freeStreamDecode2
#define LZ4_decompress_safe_continue LZ4_decompress_safe_continue2
#define LZ4_decompress_fast_continue LZ4_decompress_fast_continue2
#include "deps/lz4/lz4hc.c"
#undef KB
#undef MB
#undef MAX_DISTANCE
#endif

#if 1
// shoco
#include "deps/shoco/shoco.c"
#endif

#if 1
// easylzma
#include "deps/easylzma/src/common_internal.c"
#include "deps/easylzma/src/compress.c"
#include "deps/easylzma/src/decompress.c"
#include "deps/easylzma/src/lzip_header.c"
#include "deps/easylzma/src/lzma_header.c"
// lzma sdk
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

//
#ifdef swap
#undef swap
#endif

#undef HASH_MASK
#undef HASH_LOG
#include "deps/zstd/lib/fse.h"
#include "deps/zstd/lib/fse.c"
#include "deps/zstd/lib/zstd.h"
#include "deps/zstd/lib/zstd.c"

// bundle
#include "bundle.cpp"

