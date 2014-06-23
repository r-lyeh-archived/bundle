bundle
======

- Bundle is a lightweight C++ compression library that provides interface for ZIP, LZMA, LZIP, ZPAQ, LZ4 and SHOCO algorithms.
- Bundle is optimized for highest compression ratios on each compressor, wherever possible.
- Bundle is optimized for fastest decompression times on each decompressor, wherever possible.
- Bundle is easy to integrate, comes in an amalgamated distribution.
- Bundle is tiny. All dependencies included. Self-contained.
- Bundle is cross-platform.
- Bundle is MIT licensed.

### licensing details
- [bundle](https://github.com/r-lyeh/bundle), MIT license.
- [easylzma](https://github.com/lloyd/easylzma) by Igor Pavlov and Lloyd Hilaiel, public domain.
- [libzpaq](https://github.com/zpaq/zpaq) by Matt Mahoney, public domain.
- [LZ4](https://code.google.com/p/lz4/) by Yann Collet, BSD license.
- [miniz](https://code.google.com/p/miniz/) by Rich Geldreich, public domain.
- [SHOCO](https://github.com/Ed-von-Schleck/shoco) by Christian Schramm, MIT license.

### sample usage
```c++
#include <cassert>
#include "bundle.hpp"
using namespace bundle;

int main() {
    // 50 mb dataset
    std::string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original;

    for( auto &encoding : { LZ4, LZ4HC, SHOCO, MINIZ, LZIP, LZMA, ZPAQ, NONE } ) {
        std::string packed = pack(original, encoding);
        std::string unpacked = unpack(compressed);
        std::cout << name(encoding) << ": " << original.size() << " to " << packed.size() << " bytes" << std::endl;
        assert( original == unpacked );
    }

    std::cout << "All ok." << std::endl;
}
```

### possible output
```
LZ4: 53477376 to 209785 bytes
LZ4HC: 53477376 to 209785 bytes
SHOCO: 53477376 to 38797322 bytes
MINIZ: 53477376 to 155650 bytes
LZIP: 53477376 to 7695 bytes
LZMA: 53477376 to 7690 bytes
ZPAQ: 53477376 to 1347 bytes
NONE: 53477376 to 53477376 bytes
All ok.
```

### on picking up compressors
- sorted by compression ratio `zpaq < lzma < lzip < miniz < lz4hc < lz4`
- sorted by compression time `lz4 < lz4hc < miniz < lzma < lzip << zpaq`
- sorted by decompression time `lz4hc < lz4 < miniz < lzma < lzip << zpaq`
- sorted by memory overhead `lz4 < lz4hc < miniz < lzma < zpaq`
- and maybe use SHOCO for plain text ascii IDs (SHOCO is an entropy text-compressor)

### public api (@todocument)
- bool is_packed( T )
- bool is_unpacked( T )
- T pack( unsigned q, T )
- bool pack( unsigned q, T out, U in )
- bool pack( unsigned q, const char *in, size_t len, char *out, size_t &zlen )
- T unpack( T )
- bool unpack( unsigned q, T out, U in )
- bool unpack( unsigned q, const char *in, size_t len, char *out, size_t &zlen )
- unsigned type_of( string )
- string name_of( string )
- string version_of( string )
- string ext_of( string )
- size_t length( string )
- size_t zlength( string )
- void *zptr( string )
- size_t bound( unsigned q, size_t len )
- const char *const name_of( unsigned q )
- const char *const version( unsigned q )
- const char *const ext_of( unsigned q )
- unsigned type_of( const void *mem, size_t size )
- also, pakfile
- also, pak

### some evaluated libraries
- [BSC](https://github.com/IlyaGrebnov/libbsc)
- [FastLZ](http://fastlz.org/)
- [FLZP](http://cs.fit.edu/~mmahoney/compression/#flzp)
- [LibLZF](http://freshmeat.net/projects/liblzf)
- [LZFX](https://code.google.com/p/lzfx/)
- [LZHAM](https://code.google.com/p/lzham/)
- [LZJB](http://en.wikipedia.org/wiki/LZJB)
- [LZLIB](http://www.nongnu.org/lzip/lzlib.html)
- [LZO](http://www.oberhumer.com/opensource/lzo/)
- [LZP](http://www.cbloom.com/src/index_lz.html)
- [SMAZ](https://github.com/antirez/smaz)
- [Snappy](https://code.google.com/p/snappy/)
- [ZLIB](http://www.zlib.net/)
- [bzip2](http://www.bzip2.org/)
