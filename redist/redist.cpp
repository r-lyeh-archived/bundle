/** this is an amalgamated file. do not edit.
 */

// headers
#include "bundle.hpp"
#include "deps/lz4/lz4.h"
#include "deps/shoco/shoco.h"
#include "deps/easylzma/src/easylzma/compress.h"
#include "deps/easylzma/src/easylzma/decompress.h"
#include "deps/zpaq/libzpaq.h"

// miniz
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
//#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
//#define MINIZ_HAS_64BIT_REGISTERS 1
#include "deps/miniz/miniz.c"
// lz4 defines 'inline' and 'restrict' which is later required by shoco
#include "deps/lz4/lz4.c"
// shoco
#include "deps/shoco/shoco.c"

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

// bundle
#ifdef swap
#undef swap
#endif
#include "bundle.cpp"
