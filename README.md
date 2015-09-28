# bundle :package: <a href="https://travis-ci.org/r-lyeh/bundle"><img src="https://api.travis-ci.org/r-lyeh/bundle.svg?branch=master" align="right" /></a>

- Bundle is an embeddable compression library that supports ZIP, LZMA, LZIP, ZPAQ, LZ4, ZSTD, BROTLI, BSC, CSC, SHRINKER and SHOCO (C++03)(C++11).
- Bundle is optimized for highest compression ratios on each compressor, where possible.
- Bundle is optimized for fastest decompression times on each decompressor, where possible.
- Bundle is easy to integrate, comes in an amalgamated distribution.
- Bundle is tiny. Header and source files. Self-contained, dependencies included.
- Bundle is cross-platform.
- Bundle is zlib/libpng licensed.

### bundle stream format
```c++
[b000000000111xxxx]  Header (12 bits). De/compression algorithm (4 bits)
                     { NONE,SHOCO,LZ4,DEFLATE,LZIP,LZMA20,ZPAQ,LZ4HC,BROTLI9,ZSTD,LZMA25,BSC,BROTLI11,SHRINKER,CSC20 }.
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
    std::vector<unsigned> libs { RAW, LZ4, LZ4HC, SHOCO, MINIZ, LZMA20, LZIP, LZMA25, BROTLI9, BROTLI11, ZSTD, BSC, SHRINKER, CSC20 };
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
[ OK ] NONE: ratio=0% enctime=78452us dectime=48559us (zlen=55574506 bytes)
[ OK ] LZ4F: ratio=96.2244% enctime=59468us dectime=54583us (zlen=2098285 bytes)
[ OK ] SHOCO: ratio=26.4155% enctime=641602us dectime=292279us (zlen=40894196 bytes)
[ OK ] MINIZ: ratio=99.4327% enctime=323871us dectime=55654us (zlen=315271 bytes)
[ OK ] LZIP: ratio=99.9574% enctime=5345118us dectime=214263us (zlen=23651 bytes)
[ OK ] LZMA20: ratio=99.9346% enctime=5181916us dectime=79593us (zlen=36355 bytes)
[ OK ] LZMA25: ratio=99.9667% enctime=5527585us dectime=83422us (zlen=18513 bytes)
[ OK ] LZ4: ratio=99.5944% enctime=476480us dectime=40808us (zlen=225409 bytes)
[ OK ] ZSTD: ratio=99.8687% enctime=69955us dectime=79821us (zlen=72969 bytes)
[ OK ] BSC: ratio=99.9991% enctime=110453us dectime=143885us (zlen=524 bytes)
[ OK ] BROTLI9: ratio=99.998% enctime=915402us dectime=93187us (zlen=1086 bytes)
[ OK ] SHRINKER: ratio=99.4873% enctime=83200us dectime=55524us (zlen=284912 bytes)
[ OK ] CSC20: ratio=99.9696% enctime=1105565us dectime=272270us (zlen=16897 bytes)
[ OK ] BROTLI11: ratio=99.9984% enctime=15051465us dectime=76943us (zlen=864 bytes)
[ OK ] ZPAQ: ratio=99.9969% enctime=150045662us dectime=147353064us (zlen=1743 bytes)
All ok.
```

### On picking up compressors (as regular basis)
- sorted by compression ratio:
- `shoco < lz4f < miniz < shrinker < lz4 < zstd < brotli9 / lzma20 / lzip < brotli11 / lzma25 / bsc < zpaq`
- sorted by compression time:
- `lz4f < zstd < shrinker < lz4 / miniz < shoco < brotli9 < csc20 < lzma20 / lzip / lzma25 < bsc << brotli11 <<< zpaq`
- sorted by decompression time:
- `lz4 < lz4f < shrinker / miniz < brotli11 / zstd < lzma20 / lzma25 / brotli9 < bsc < lzip < csc20 < shoco <<< zpaq`
- sorted by memory overhead:
- `lz4f < lz4 < zstd < miniz < brotli9 < lzma20 < lzip < lzma25 / bsc << zpaq <<< brotli11`
- Note: SHOCO is a text compressor intended to be used for plain ascii IDs only.

### Charts

[@mavam](https://github.com/mavam) has an awesome R script that plots some fancy graphics in [his compbench repository](https://github.com/mavam/compbench). The following CC images are a few of his own showcasing an invocation for a 10,000 packet PCAP trace:

![Tradeoff](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/tradeoff.png)
![Compression Ratio](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/compression-ratio.png)
![Throughput Scatterplot](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/throughput-scatter.png)
![Throughput Barplot](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/throughput-bars.png)

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

### Licenses
- [bundle](https://github.com/r-lyeh/bundle), ZLIB/LibPNG license.
- [brotli](https://github.com/google/brotli) by Jyrki Alakuijala and Zoltan Szabadka, Apache 2.0 license.
- [easylzma](https://github.com/lloyd/easylzma) by Igor Pavlov and Lloyd Hilaiel, public domain.
- [giant](https://githhub.com/r-lyeh/giant), ZLIB/LibPNG license.
- [libzpaq](https://github.com/zpaq/zpaq) by Matt Mahoney, public domain.
- [libbsc](https://github.com/IlyaGrebnov/libbsc) by Ilya Grebnov, Apache 2.0 license.
- [lz4](https://github.com/Cyan4973/lz4) by Yann Collet, BSD-2 license.
- [miniz](https://code.google.com/p/miniz/) by Rich Geldreich, public domain.
- [shoco](https://github.com/Ed-von-Schleck/shoco) by Christian Schramm, MIT license.
- [zstd](https://github.com/Cyan4973/zstd) by Yann Collet, BSD-2 license.
- [shrinker](https://code.google.com/p/data-shrinker/) by Siyuan Fu, BSD-3 license.
- [csc](https://github.com/fusiyuan2010/CSC) by Siyuan Fu, public domain.

### Evaluated alternatives
[FastLZ](http://fastlz.org/), [FLZP](http://cs.fit.edu/~mmahoney/compression/#flzp), [LibLZF](http://freshmeat.net/projects/liblzf), [LZFX](https://code.google.com/p/lzfx/), [LZHAM](https://code.google.com/p/lzham/), [LZJB](http://en.wikipedia.org/wiki/LZJB), [LZLIB](http://www.nongnu.org/lzip/lzlib.html), [LZO](http://www.oberhumer.com/opensource/lzo/), [LZP](http://www.cbloom.com/src/index_lz.html), [SMAZ](https://github.com/antirez/smaz), [Snappy](https://code.google.com/p/snappy/), [ZLIB](http://www.zlib.net/), [bzip2](http://www.bzip2.org/), Yappy

### Changelog
- v0.9.5 (2015/09/28)
  - Add missing prototypes
  - Bugfix helper function
- v0.9.4 (2015/09/26)
  - Add CSC20 + Shrinker support
  - Rename enums LZ4->LZ4F/LZ4HC->LZ4
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
  - Safer LZ4 decompression
  - Pump up LZ4 + ZPAQ
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
  - Add iOS support
- v0.0.0 (2014/05/09)
  - Initial commit
