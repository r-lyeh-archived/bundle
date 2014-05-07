/** this is an amalgamated file. do not edit.
 */

#include "deps/lz4/lz4.h"

#include "deps/shoco/shoco.h"

//#define MINIZ_HEADER_FILE_ONLY
#include "deps/miniz/miniz.c"
//#undef MINIZ_HEADER_FILE_ONLY

#define mz_crc32 mz_crc32_lz
#include "deps/lzlib/lzlib.c"

#include "bundle.cpp"
#include "pak.cpp"

// lz4 defines 'inline' and 'restrict' which is later required by shoco
#include "deps/lz4/lz4.c"
#include "deps/shoco/shoco.c"
