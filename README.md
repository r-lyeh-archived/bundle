# bundle :package: <a href="https://travis-ci.org/r-lyeh/bundle"><img src="https://api.travis-ci.org/r-lyeh/bundle.svg?branch=master" align="right" /></a>

- Bundle is an embeddable compression library that supports ZIP, LZMA, LZIP, ZPAQ, LZ4, ZSTD, BROTLI, BSC and SHOCO (C++03)(C++11).
- Bundle is optimized for highest compression ratios on each compressor, where possible.
- Bundle is optimized for fastest decompression times on each decompressor, where possible.
- Bundle is easy to integrate, comes in an amalgamated distribution.
- Bundle is tiny. Header and source files. Self-contained, dependencies included.
- Bundle is cross-platform.
- Bundle is zlib/libpng licensed.

### bundle stream format
```c++
[b000000000111xxxx]  Header (12 bits). De/compression algorithm (4 bits)
                     { NONE,SHOCO,LZ4,DEFLATE,LZIP,LZMA20,ZPAQ,LZ4HC,BROTLI9,ZSTD,LZMA25,BSC,BROTLI11 }.
[vle_unpacked_size]  Unpacked size of the stream (N bytes). Data is stored in a variable
                     length encoding value, where bytes are just shifted and added into a
                     big accumulator until MSB is found.
[vle_packed_size]    Packed size of the stream (N bytes). Data is stored in a variable
                     length encoding value, where bytes are just shifted and added into a
                     big accumulator until MSB is found.
[bitstream]          Compressed bitstream (N bytes). As returned by compressor.
                     If possible, header-less bitstreams are preferred.
```

### Bundle archive format
```c++
- Files/datas are packed into streams by using any compression method (see above)
- Streams are archived into a standard ZIP file:
  - ZIP entry compression is (0) for packed streams and (1-9) for unpacked streams.
  - ZIP entry comment is a serialized JSON of (file) meta-datas (@todo).
- Note: you can mix streams of different algorithms into the very same ZIP archive.
```

### Showcase
```c++
#include <cassert>
#include "bundle.hpp"

int main() {
    // 55 mb dataset
    std::string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original + std::string( i + 1, 32 + i );

    // pack, unpack & verify a few encoders
    using namespace bundle;
    std::vector<unsigned> libs { RAW, LZ4, LZ4HC, SHOCO, MINIZ, LZMA20, LZIP, LZMA25, BROTLI9, BROTLI11, ZSTD, BSC };
    for( auto &use : libs ) {
        std::string packed = pack(use, original);
        std::string unpacked = unpack(packed);
        std::cout << name_of(use) << ": " << original.size() << " to " << packed.size() << " bytes" << std::endl;
        assert( original == unpacked );
    }

    std::cout << "All ok." << std::endl;
}
```

### Possible output
```
[ OK ] NONE: ratio=0% enctime=80619us dectime=38367us (zlen=55574506 bytes)
[ OK ] LZ4: ratio=96.2244% enctime=55946us dectime=44934us (zlen=2098285 bytes)
[ OK ] SHOCO: ratio=26.4155% enctime=640547us dectime=296934us (zlen=40894196 bytes)
[ OK ] MINIZ: ratio=99.4327% enctime=329166us dectime=42940us (zlen=315271 bytes)
[ OK ] LZIP: ratio=99.9574% enctime=5209819us dectime=203620us (zlen=23651 bytes)
[ OK ] LZMA20: ratio=99.9346% enctime=4957873us dectime=69628us (zlen=36355 bytes)
[ OK ] LZMA25: ratio=99.9667% enctime=5287068us dectime=82368us (zlen=18513 bytes)
[ OK ] LZ4HC: ratio=99.5944% enctime=479542us dectime=43869us (zlen=225409 bytes)
[ OK ] ZSTD: ratio=99.8687% enctime=53503us dectime=41654us (zlen=72969 bytes)
[ OK ] BSC: ratio=99.9991% enctime=98170us dectime=91463us (zlen=524 bytes)
[ OK ] BROTLI9: ratio=99.998% enctime=857999us dectime=77870us (zlen=1086 bytes)
[ OK ] BROTLI11: ratio=99.9984% enctime=15111576us dectime=82196us (zlen=864 bytes)
[ OK ] ZPAQ: ratio=99.9969% enctime=144051595us dectime=140961629us (zlen=1743 bytes)
All ok.
```

### On picking up compressors (as regular basis)
- sorted by compression ratio:
- `zpaq < lzma25 / bsc < brotli11 < lzip < lzma20 < brotli9 < zstd < miniz < lz4hc < lz4`
- sorted by compression time:
- `lz4 < lz4hc < zstd < miniz < lzma20 < lzip < lzma25 / bsc < brotli9 << brotli11 <<< zpaq`
- sorted by decompression time:
- `lz4hc < lz4 < zstd < miniz < brotli9 < brotli11 < lzma20 / lzma25 < lzip < bsc << zpaq`
- sorted by memory overhead:
- `lz4 < lz4hc < zstd < miniz < brotli9 < lzma20 < lzip < lzma25 / bsc < brotli11 << zpaq`
- and maybe use SHOCO for plain text ascii IDs (SHOCO is an entropy text-compressor)

### API - data
```c++
namespace bundle
{
  // low level API (raw pointers)
  bool is_packed( *ptr, len );
  bool is_unpacked( *ptr, len );
  unsigned type_of( *ptr, len );
  size_t len( *ptr, len );
  size_t zlen( *ptr, len );
  const void *zptr( *ptr, len );
  bool pack( unsigned Q, *in, len, *out, &zlen );
  bool unpack( unsigned Q, *in, len, *out, &zlen );

  // medium level API, templates (in-place)
  bool is_packed( T );
  bool is_unpacked( T );
  unsigned type_of( T );
  size_t len( T );
  size_t zlen( T );
  const void *zptr( T );
  bool unpack( T &, T );
  bool pack( unsigned Q, T &, T );

  // high level API, templates (copy)
  T pack( unsigned Q, T );
  T unpack( T );
}
```
For a complete review check [bundle.hpp header](bundle.hpp)

### API - archives
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

### Changelog
- v0.9.3 (2015/09/25)
  - Add a few missing API calls
- v0.9.2 (2015/09/22)
  - Pump up Brotli
  - Split BROTLI enum into BROTLI9/11 pair
- v0.9.1 (2015/05/10)
  - Switch to ZLIB/LibPNG license
- v0.9.0 (2015/04/08)
  - BSC support
- v0.8.1 (2015/04/07)
  - Pump up Brotli+ZSTD
  - LZMA20/25 dict
  - Unify FOURCCs
- v0.8.0 (2015/01/27)
  - ZSTD support
  - Reorder enums
  - Simplify API
- v0.7.1 (2015/01/26)
  - Fix LZMA
  - Verify DEFLATEs
  - New AUTO enum
- v0.7.0 (2014/10/22)
  - Brotli support
  - Pump up LZ4
- v0.6.3 (2014/09/27)
  - Switch to BOOST license
- v0.6.2 (2014/09/02)
  - Fix 0-byte streams
  - Deflate alignment
- v0.6.1 (2014/06/30)
  - Safer lz4 decompression
  - Pump up lz4+zpaq
- v0.6.0 (2014/06/26)
  - LZ4HC support
  - Optimize in-place decompression
- v0.5.0 (2014/06/09)
  - ZPAQ support
  - UBER encoding
  - Fixes
- v0.4.1 (2014/06/05)
  - Switch to lzmasdk
- v0.4.0 (2014/05/30)
  - Maximize compression (lzma)
- v0.3.0 (2014/05/28)
  - Fix alignment (deflate)
  - Change stream header
- v0.2.1 (2014/05/23)
  - Fix overflow bug
- v0.2.0 (2014/05/14)
  - Add VLE header
  - Fix vs201x compilation errors
- v0.1.0 (2014/05/13)
  - Add high-level API
  - iOS support
- v0.0.0 (2014/05/09)
  - Initial commit

### Licenses
- [bundle](https://github.com/r-lyeh/bundle), zlib/libpng license.
- [brotli](https://github.com/google/brotli) by Jyrki Alakuijala and Zoltan Szabadka, Apache 2.0 license.
- [easylzma](https://github.com/lloyd/easylzma) by Igor Pavlov and Lloyd Hilaiel, public domain.
- [giant](https://githhub.com/r-lyeh/giant), zlib/libpng license.
- [libzpaq](https://github.com/zpaq/zpaq) by Matt Mahoney, public domain.
- [libbsc](https://github.com/IlyaGrebnov/libbsc) by Ilya Grebnov, Apache 2.0 license.
- [lz4](https://github.com/Cyan4973/lz4) by Yann Collet, BSD license.
- [miniz](https://code.google.com/p/miniz/) by Rich Geldreich, public domain.
- [shoco](https://github.com/Ed-von-Schleck/shoco) by Christian Schramm, MIT license.
- [zstd](https://github.com/Cyan4973/zstd) by Yann Collet, BSD license.

### Evaluated alternatives
[FastLZ](http://fastlz.org/), [FLZP](http://cs.fit.edu/~mmahoney/compression/#flzp), [LibLZF](http://freshmeat.net/projects/liblzf), [LZFX](https://code.google.com/p/lzfx/), [LZHAM](https://code.google.com/p/lzham/), [LZJB](http://en.wikipedia.org/wiki/LZJB), [LZLIB](http://www.nongnu.org/lzip/lzlib.html), [LZO](http://www.oberhumer.com/opensource/lzo/), [LZP](http://www.cbloom.com/src/index_lz.html), [SMAZ](https://github.com/antirez/smaz), [Snappy](https://code.google.com/p/snappy/), [ZLIB](http://www.zlib.net/), [bzip2](http://www.bzip2.org/), Yappy
