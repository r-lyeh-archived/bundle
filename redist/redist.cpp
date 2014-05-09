/** this is an amalgamated file. do not edit.
 */
#include "bundle.hpp"
#include "deps/lz4/lz4.h"
#include "deps/shoco/shoco.h"
#include "deps/miniz/miniz.c"
#define mz_crc32 mz_crc32_lz
#include "deps/lzlib/lzlib.c"
// lz4 defines 'inline' and 'restrict' which is later required by shoco
#include "deps/lz4/lz4.c"
#include "deps/shoco/shoco.c"
#include "bundle.cpp"
