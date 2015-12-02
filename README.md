# bundle :package: <a href="https://travis-ci.org/r-lyeh/bundle"><img src="https://api.travis-ci.org/r-lyeh/bundle.svg?branch=master" align="right" /></a>

Bundle is an embeddable compression library that supports 22 compression algorithms and 2 archive formats. 

Distributed in two files.

### Features
- [x] Archive support: .bnd, .zip
- [x] Stream support: DEFLATE, LZMA, LZIP, ZPAQ, LZ4, ZSTD, BROTLI, BSC, CSC, BCM, MCM, ZMOLLY, ZLING, TANGELO, SHRINKER, CRUSH, LZJB and SHOCO 
- [x] Cross-platform, compiles on clang++/g++/visual studio (C++03).
- [x] Optimized for highest compression ratios on each compressor, where possible.
- [x] Optimized for fastest decompression times on each decompressor, where possible.
- [x] Configurable, redistributable, self-contained, amalgamated and cross-platform.
- [x] Optional benchmark infrastructure (C++11).
- [x] ZLIB/LibPNG licensed.

### Bundle stream format
```c++
[0x00 0x70 0x??]     Header (16 bits). De/compression algorithm (8 bits)
                     enum { RAW, SHOCO, LZ4F, MINIZ, LZIP, LZMA20, ZPAQ, LZ4,      //  0..7
                            BROTLI9, ZSTD, LZMA25, BSC, BROTLI11, SHRINKER, CSC20, //  7..14
                            ZSTDF, BCM, ZLING, MCM, TANGELO, ZMOLLY, CRUSH, LZJB   // 15..22
                     };
[vle_unpacked_size]  Unpacked size of the stream (N bytes). Data is stored in a variable
                     length encoding value, where bytes are just shifted and added into a
                     big accumulator until MSB is found.
[vle_packed_size]    Packed size of the stream (N bytes). Data is stored in a variable
                     length encoding value, where bytes are just shifted and added into a
                     big accumulator until MSB is found.
[bitstream]          Compressed bitstream (N bytes). As returned by compressor.
                     If possible, header-less bitstreams are preferred.
```

### Bundle .bnd archive format
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
    using namespace bundle;
    using namespace std;

    // 23 mb dataset
    string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 18; ++i) original += original + string( i + 1, 32 + i );

    // pack, unpack & verify all encoders
    vector<unsigned> libs { 
        RAW, SHOCO, LZ4F, MINIZ, LZIP, LZMA20,
        ZPAQ, LZ4, BROTLI9, ZSTD, LZMA25,
        BSC, BROTLI11, SHRINKER, CSC20, BCM,
        ZLING, MCM, TANGELO, ZMOLLY, CRUSH, LZJB
    };
    for( auto &lib : libs ) {
        string packed = pack(lib, original);
        string unpacked = unpack(packed);
        cout << original.size() << " -> " << packed.size() << " bytes (" << name_of(lib) << ")" << endl;
        assert( original == unpacked );
    }

    cout << "All ok." << endl;
}
```

### On choosing compressors (on a regular basis)
|Rank|Smallest results|Fastest compressors|Fastest decompressors|Memory efficiency|
|---:|:---------------|:------------------|:--------------------|:----------------|
|1st|91.12% ZPAQ|918.71MB/s RAW|2404.10MB/s RAW|tbd
|2nd|90.67% MCM|348.47MB/s LZ4F|944.42MB/s LZ4|tbd
|3rd|89.96% TANGELO|231.33MB/s SHRINKER|874.44MB/s LZ4F|tbd
|4th|88.24% BSC|200.89MB/s LZJB|520.30MB/s SHRINKER|tbd
|5th|87.65% LZMA25|177.70MB/s ZSTDF|388.09MB/s MINIZ|tbd
|6th|87.65% LZIP|157.27MB/s SHOCO|351.40MB/s ZSTD|tbd
|7th|87.53% BROTLI11|38.45MB/s ZLING|318.99MB/s LZJB|tbd
|8th|87.42% CSC20|30.93MB/s CRUSH|295.25MB/s SHOCO|tbd
|9th|87.31% BCM|13.25MB/s ZSTD|282.99MB/s ZSTDF|tbd
|10th|86.33% ZMOLLY| 8.73MB/s BSC|259.36MB/s BROTLI11|tbd
|11th|86.02% LZMA20| 6.43MB/s ZMOLLY|253.16MB/s BROTLI9|tbd
|12th|85.94% BROTLI9| 5.60MB/s BROTLI9|195.61MB/s CRUSH|tbd
|13th|85.13% ZSTD| 5.13MB/s BCM|168.58MB/s ZLING|tbd
|14th|82.71% ZLING| 3.85MB/s LZ4|118.45MB/s LZMA25|tbd
|15th|81.98% MINIZ| 3.43MB/s MINIZ|104.09MB/s LZMA20|tbd
|16th|78.23% ZSTDF| 2.81MB/s CSC20|85.65MB/s LZIP|tbd
|17th|77.93% LZ4| 2.57MB/s LZMA25|77.92MB/s CSC20|tbd
|18th|77.04% CRUSH| 2.55MB/s LZMA20|12.93MB/s BSC|tbd
|19th|67.46% SHRINKER| 2.54MB/s LZIP| 6.76MB/s ZMOLLY|tbd
|20th|63.41% LZ4F| 2.23MB/s MCM| 4.04MB/s BCM|tbd
|21th|59.78% LZJB| 1.14MB/s TANGELO| 2.41MB/s MCM|tbd
|22th| 6.16% SHOCO| 0.23MB/s ZPAQ| 1.12MB/s TANGELO|tbd
|23th| 0.00% RAW| 0.23MB/s BROTLI11| 0.22MB/s ZPAQ|tbd

- Note: SHOCO is a _text_ compressor intended to be used for plain ascii IDs only.

### Charts

[@mavam](https://github.com/mavam) has an awesome R script that plots some fancy graphics in [his compbench repository](https://github.com/mavam/compbench). The following CC images are a few of his own showcasing an invocation for a 10,000 packet PCAP trace:

![Tradeoff](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/pcap-tradeoff.png)
![Compression Ratio](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/pcap-compression-ratio.png)
![Throughput Scatterplot](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/pcap-throughput-scatter.png)
![Throughput Barplot](https://raw.githubusercontent.com/mavam/compbench/master/screenshots/pcap-throughput-bars.png)

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
namespace bundle
{
  struct file : map<string,string> { // ~map of properties
    bool    has(property);           // property check
    string &get(property);           // property access
    string  toc() const;             // inspection (json)
  };
  struct archive : vector<file>    { // ~sequence of files
    void   bnd(string);              // .bnd serialization
    string bnd() const;              // .bnd serialization
    void   zip(string);              // .zip serialization
    string zip() const;              // .zip serialization
    string toc() const;              // inspection (json)
  };
}
```

### Build Directives (Licensing)
|#define directive|Default value|Meaning|
|:-----------|:---------------|:---------|
|BUNDLE_NO_APACHE2|(undefined)|Define to remove any Apache 2.0 library from build
|BUNDLE_NO_BSD2|(undefined)|Define to remove any BSD-2 library from build
|BUNDLE_NO_BSD3|(undefined)|Define to remove any BSD-3 library from build
|BUNDLE_NO_CDDL|(undefined)|Define to remove any CDDL library from build
|BUNDLE_NO_GPL|(undefined)|Define to remove any GPL library from build
|BUNDLE_NO_MIT|(undefined)|Define to remove any MIT library from build
|BUNDLE_NO_UNLICENSE|(undefined)|Define to remove any Public Domain library from build (*)

(*): will disable `.bnd` and `.zip` archive support as well.

### Build Directives (Libraries)
|#define directive|Default value|Meaning|
|:-----------|:---------------|:---------|
|BUNDLE_NO_BCM|(undefined)|Define to remove BCM library from build
|BUNDLE_NO_BROTLI|(undefined)|Define to remove Brotli library from build
|BUNDLE_NO_BSC|(undefined)|Define to remove LibBsc library from build
|BUNDLE_NO_CRUNCH|(undefined)|Define to remove CRUNCH library from build
|BUNDLE_NO_CSC|(undefined)|Define to remove CSC library from build
|BUNDLE_NO_LZ4|(undefined)|Define to remove LZ4/LZ4 libraries 
|BUNDLE_NO_LZIP|(undefined)|Define to remove EasyLZMA library from build
|BUNDLE_NO_LZJB|(undefined)|Define to remove LZJB library from build
|BUNDLE_NO_LZMA|(undefined)|Define to remove LZMA library from build
|BUNDLE_NO_MCM|(undefined)|Define to remove MCM library from build
|BUNDLE_NO_MINIZ|(undefined)|Define to remove MiniZ library from build (*)
|BUNDLE_NO_SHOCO|(undefined)|Define to remove Shoco library from build
|BUNDLE_NO_SHRINKER|(undefined)|Define to remove Shrinker library from build
|BUNDLE_NO_TANGELO|(undefined)|Define to remove TANGELO library from build
|BUNDLE_NO_ZLING|(undefined)|Define to remove ZLING library from build
|BUNDLE_NO_ZMOLLY|(undefined)|Define to remove ZMOLLY library from build
|BUNDLE_NO_ZPAQ|(undefined)|Define to remove ZPAQ library from build
|BUNDLE_NO_ZSTD|(undefined)|Define to remove ZSTD library from build

(*): will disable `.bnd` and `.zip` archive support as well.

## Build Directives (Other)
|#define directive|Default value|Meaning|
|:-----------|:---------------|:---------|
|BUNDLE_USE_OMP_TIMER|(undefined)|Define as 1 to use OpenMP timers
|BUNDLE_USE_CXX11|(autodetected)|Define as 0/1 to disable/enable C++11 features

### Licensing table
| Software | Author(s) | License | Version | Major changes? |
|:---------|:----------|:--------|--------:|:---------------------|
|[bundle](https://github.com/r-lyeh/bundle)|r-lyeh|ZLIB/LibPNG|latest||
|[bcm](http://sourceforge.net/projects/bcm/)|Ilya Muravyov|Public Domain|1.00|istream based now|
|[brotli](https://github.com/google/brotli)|Jyrki Alakuijala, Zoltan Szabadka|Apache 2.0|2015/11/03||
|[crush](http://sourceforge.net/projects/crush/)|Ilya Muravyov|Public Domain|1.00||
|[csc](https://github.com/fusiyuan2010/CSC)|Siyuan Fu|Public Domain|2015/06/16||
|[easylzma](https://github.com/lloyd/easylzma)|Igor Pavlov, Lloyd Hilaiel|Public Domain|0.0.7||
|[endian](https://gist.github.com/panzi/6856583)|Mathias Panzenböck|Public Domain||msvc fix|
|[libbsc](https://github.com/IlyaGrebnov/libbsc)|Ilya Grebnov|Apache 2.0|3.1.0||
|[libzling](https://github.com/richox/libzling)|Zhang Li|BSD-3|2015/09/16||
|[libzpaq](https://github.com/zpaq/zpaq)|Matt Mahoney|Public Domain|7.05||
|[lz4](https://github.com/Cyan4973/lz4)|Yann Collet|BSD-2|1.7.1||
|[lzjb](http://en.wikipedia.org/wiki/LZJB)|Jeff Bonwick|CDDL license|2010||
|[mcm](https://github.com/mathieuchartier/mcm)|Mathieu Chartier|GPL|0.84||
|[miniz](https://code.google.com/p/miniz/)|Rich Geldreich|Public Domain|v1.15 r.4.1|alignment fix|
|[shoco](https://github.com/Ed-von-Schleck/shoco)|Christian Schramm|MIT|2015/03/16||
|[shrinker](https://code.google.com/p/data-shrinker/)|Siyuan Fu|BSD-3|rev 3||
|[tangelo](http://encode.ru/threads/1738-TANGELO-new-compressor-(derived-from-PAQ8-FP8))|Matt Mahoney, Jan Ondrus|GPL|2.41|reentrant fixes, istream based now|
|[zmolly](https://github.com/richox/zmolly)|Zhang Li|BSD-3|0.0.1|reentrant and memstream fixes|
|[zstd](https://github.com/Cyan4973/zstd)|Yann Collet|BSD-2|0.3.2||

### Evaluated alternatives
[FastLZ](http://fastlz.org/), [FLZP](http://cs.fit.edu/~mmahoney/compression/#flzp), [LibLZF](http://freshmeat.net/projects/liblzf), [LZFX](https://code.google.com/p/lzfx/), [LZHAM](https://code.google.com/p/lzham/), [LZLIB](http://www.nongnu.org/lzip/lzlib.html), [LZO](http://www.oberhumer.com/opensource/lzo/), [LZP](http://www.cbloom.com/src/index_lz.html), [SMAZ](https://github.com/antirez/smaz), [Snappy](https://code.google.com/p/snappy/), [ZLIB](http://www.zlib.net/), [bzip2](http://www.bzip2.org/), [Yappy](http://blog.gamedeff.com/?p=371), [CMix](http://www.byronknoll.com/cmix.html), [M1](https://sites.google.com/site/toffer86/m1-project)

### Changelog
- v2.0.3 (2015/12/02): Add LZJB and CRUSH; Add BUNDLE_NO_CDDL directive
- v2.0.2 (2015/11/07): Fix ZMolly segmentation fault (OSX)
- v2.0.1 (2015/11/04): Fix clang warnings and compilation errors
- v2.0.0 (2015/11/03): Add BCM, ZLING, MCM, Tangelo, ZMolly, ZSTDf support
- v2.0.0 (2015/11/03): Change archive format (break change)
- v2.0.0 (2015/11/03): Disambiguate .bnd/.zip archive handling
- v2.0.0 (2015/11/03): Fix compilation errors (C++03)
- v2.0.0 (2015/11/03): Fix missing entries in benchmarks
- v2.0.0 (2015/11/03): Improve runtime C++ de/initialization stability
- v2.0.0 (2015/11/03): Optimize archive decompression
- v2.0.0 (2015/11/03): Remove obsolete enums
- v2.0.0 (2015/11/03): Upgrade Brotli, ZPAQ, LZ4, ZSTD and Shoco
- v1.0.2 (2015/10/29): Skip extra copy during archive decompression
- v1.0.2 (2015/10/29): Add extra archive meta-info
- v1.0.1 (2015/10/10): Shrink to fit during measures() function
- v1.0.0 (2015/10/09): Change benchmark API to sort multiples values as well (API break change)
- v0.9.8 (2015/10/07): Remove confusing bundle::string variant class from API
- v0.9.7 (2015/10/07): Add license configuration directives
- v0.9.6 (2015/10/03): Add library configuration directives
- v0.9.5 (2015/09/28): Add missing prototypes
- v0.9.5 (2015/09/28): Bugfix helper function
- v0.9.4 (2015/09/26): Add CSC20 + Shrinker support
- v0.9.4 (2015/09/26): Rename enums LZ4->LZ4F/LZ4HC->LZ4
- v0.9.3 (2015/09/25): Add a few missing API calls
- v0.9.2 (2015/09/22): Pump up Brotli
- v0.9.2 (2015/09/22): Split BROTLI enum into BROTLI9/11 pair
- v0.9.1 (2015/05/10): Switch to ZLIB/LibPNG license
- v0.9.0 (2015/04/08): BSC support
- v0.8.1 (2015/04/07): Pump up Brotli+ZSTD
- v0.8.1 (2015/04/07): LZMA20/25 dict
- v0.8.1 (2015/04/07): Unify FOURCCs
- v0.8.0 (2015/01/27): ZSTD support
- v0.8.0 (2015/01/27): Reorder enums
- v0.8.0 (2015/01/27): Simplify API
- v0.7.1 (2015/01/26): Fix LZMA
- v0.7.1 (2015/01/26): Verify DEFLATEs
- v0.7.1 (2015/01/26): New AUTO enum
- v0.7.0 (2014/10/22): Brotli support
- v0.7.0 (2014/10/22): Pump up LZ4
- v0.6.3 (2014/09/27): Switch to BOOST license
- v0.6.2 (2014/09/02): Fix 0-byte streams
- v0.6.2 (2014/09/02): Deflate alignment
- v0.6.1 (2014/06/30): Safer LZ4 decompression
- v0.6.1 (2014/06/30): Pump up LZ4 + ZPAQ
- v0.6.0 (2014/06/26): LZ4HC support
- v0.6.0 (2014/06/26): Optimize in-place decompression
- v0.5.0 (2014/06/09): ZPAQ support
- v0.5.0 (2014/06/09): UBER encoding
- v0.5.0 (2014/06/09): Fixes
- v0.4.1 (2014/06/05): Switch to lzmasdk
- v0.4.0 (2014/05/30): Maximize compression (lzma)
- v0.3.0 (2014/05/28): Fix alignment (deflate)
- v0.3.0 (2014/05/28): Change stream header
- v0.2.1 (2014/05/23): Fix overflow bug
- v0.2.0 (2014/05/14): Add VLE header
- v0.2.0 (2014/05/14): Fix vs201x compilation errors
- v0.1.0 (2014/05/13): Add high-level API
- v0.1.0 (2014/05/13): Add iOS support
- v0.0.0 (2014/05/09): Initial commit
