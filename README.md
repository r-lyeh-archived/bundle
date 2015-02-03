bundle <img src="https://api.travis-ci.org/r-lyeh/bundle.svg" align="right" />
======

- Bundle is an embeddable compression library that supports ZIP, LZMA, LZIP, ZPAQ, LZ4, ZSTD, BROTLI and SHOCO (C++03)(C++11).
- Bundle is optimized for highest compression ratios on each compressor, wherever possible.
- Bundle is optimized for fastest decompression times on each decompressor, wherever possible.
- Bundle is easy to integrate, comes in an amalgamated distribution.
- Bundle is tiny. Header and source files. Self-contained, dependencies included. 
- Bundle is cross-platform.
- Bundle is BOOST licensed.

### bundle stream format
```c++
[b000000000111xxxx]  Header (12 bits). De/compression algorithm (4 bits)
                     { NONE, SHOCO, LZ4, DEFLATE, LZIP, LZMASDK, ZPAQ, LZ4HC, BROTLI, ZSTD }.
[vle_unpacked_size]  Unpacked size of the stream (N bytes). Data is stored in a variable
                     length encoding value, where bytes are just shifted and added into a
                     big accumulator until MSB is found.
[vle_packed_size]    Packed size of the stream (N bytes). Data is stored in a variable
                     length encoding value, where bytes are just shifted and added into a
                     big accumulator until MSB is found.
[bitstream]          Compressed bitstream (N bytes). As returned by compressor.
                     If possible, header-less bitstreams are preferred.
```

### bundle archive format
```c++
- Files/datas are packed into streams by using any compression method (see above)
- Streams are archived into a standard ZIP file:
  - ZIP entry compression is (0) for packed streams and (1-9) for unpacked streams.
  - ZIP entry comment is a serialized JSON of (file) meta-datas (@todo).
- Note: you can mix streams of different algorithms into the very same ZIP archive.
```

### sample 
```c++
#include <cassert>
#include "bundle.hpp"

int main() {
    // 50 mb dataset
    std::string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original;

    // pack, unpack & verify
    using namespace bundle;
    std::vector<unsigned> libs { RAW, LZ4, LZ4HC, SHOCO, MINIZ, LZIP, LZMA, ZPAQ, BROTLI, ZSTD };
    for( auto &use : libs ) {
        std::string packed = pack(use, original);
        std::string unpacked = unpack(packed);
        std::cout << name_of(use) << ": " << original.size() << " to " << packed.size() << " bytes" << std::endl;
        assert( original == unpacked );
    }

    std::cout << "All ok." << std::endl;
}
```

### possible output
```
[ OK ] NONE: ratio=0% enctime=27270us dectime=15431us (zlen=53477377 bytes)
[ OK ] LZ4: ratio=99.6077% enctime=18020us dectime=17506us (zlen=209785 bytes)
[ OK ] SHOCO: ratio=27.451% enctime=362605us dectime=259619us (zlen=38797323 bytes)
[ OK ] MINIZ: ratio=99.7089% enctime=177436us dectime=31469us (zlen=155652 bytes)
[ OK ] LZIP: ratio=99.9856% enctime=1925159us dectime=176294us (zlen=7697 bytes)
[ OK ] LZMA: ratio=99.9856% enctime=1771894us dectime=47160us (zlen=7692 bytes)
[ OK ] ZPAQ: ratio=99.9972% enctime=98318191us dectime=100046003us (zlen=1479 bytes)
[ OK ] LZ4HC: ratio=99.6077% enctime=19146us dectime=18155us (zlen=209785 bytes)
[ OK ] BROTLI: ratio=99.9993% enctime=4801348us dectime=117371us (zlen=387 bytes)
[ OK ] ZSTD: ratio=99.9854% enctime=22606us dectime=20628us (zlen=7820 bytes)
All ok.
```

### on picking up compressors (on regular basis)
- sorted by compression ratio `zpaq < lzma / lzip < brotli < zstd < miniz < lz4hc < lz4`
- sorted by compression time `lz4 < lz4hc < zstd < miniz < lzma < lzip << zpaq <<< brotli`
- sorted by decompression time `lz4hc < lz4 < zstd < miniz < brotli < lzma < lzip << zpaq`
- sorted by memory overhead `lz4 < lz4hc < zstd < miniz < brotli < lzma / lzip < zpaq`
- and maybe use SHOCO for plain text ascii IDs (SHOCO is an entropy text-compressor)

### functional api
```c++
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
```

### archival api
```c++
struct file : map<string,string> { // ~map of properties
  bool has(property);              // property check
  string &get(property);           // property access
};
struct archive : vector<file>    { // ~sequence of files
  void bin(string);                // serialization
  string bin() const;              // serialization
  string toc() const;              // debug
};
```

### licenses
- [bundle](https://github.com/r-lyeh/bundle), BOOST license.
- [giant](https://githhub.com/r-lyeh/giant), BOOST license.
- [easylzma](https://github.com/lloyd/easylzma) by Igor Pavlov and Lloyd Hilaiel, public domain.
- [libzpaq](https://github.com/zpaq/zpaq) by Matt Mahoney, public domain.
- [lz4](https://github.com/Cyan4973/lz4) by Yann Collet, BSD license.
- [zstd](https://github.com/Cyan4973/zstd) by Yann Collet, BSD license.
- [miniz](https://code.google.com/p/miniz/) by Rich Geldreich, public domain.
- [shoco](https://github.com/Ed-von-Schleck/shoco) by Christian Schramm, MIT license.
- [brotli](https://github.com/google/brotli) by Jyrki Alakuijala and Zoltan Szabadka, Apache 2.0 license.

### evaluated alternatives
[BSC](https://github.com/IlyaGrebnov/libbsc), [FastLZ](http://fastlz.org/), [FLZP](http://cs.fit.edu/~mmahoney/compression/#flzp), [LibLZF](http://freshmeat.net/projects/liblzf), [LZFX](https://code.google.com/p/lzfx/), [LZHAM](https://code.google.com/p/lzham/), [LZJB](http://en.wikipedia.org/wiki/LZJB), [LZLIB](http://www.nongnu.org/lzip/lzlib.html), [LZO](http://www.oberhumer.com/opensource/lzo/), [LZP](http://www.cbloom.com/src/index_lz.html), [SMAZ](https://github.com/antirez/smaz), [Snappy](https://code.google.com/p/snappy/), [ZLIB](http://www.zlib.net/), [bzip2](http://www.bzip2.org/), Yappy
