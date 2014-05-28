/** this is an amalgamated file. do not edit.
 */
#include "bundle.hpp"
#include "deps/lz4/lz4.h"
#include "deps/shoco/shoco.h"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
//#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
//#define MINIZ_HAS_64BIT_REGISTERS 1
#include "deps/miniz/miniz.c"
//#define mz_crc32 mz_crc32_lz
#include "deps/lzlib/lzlib.c"
// lz4 defines 'inline' and 'restrict' which is later required by shoco
#include "deps/lz4/lz4.c"
#include "deps/shoco/shoco.c"
#ifdef swap
#undef swap
#endif
#include "bundle.cpp"
