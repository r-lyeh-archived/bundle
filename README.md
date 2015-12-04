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
[0x00  ...]          Optional zero padding (N bits)
[0x70 0x??]          Header (8 bits). De/compression algorithm (8 bits)
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
|Rank|Compression ratio      |Fastest compressors    |Fastest decompressors  |Average speed          |
|---:|:----------------------|:----------------------|:----------------------|:----------------------|
| 1st|91.12% ZPAQ            |874.36MB/s RAW         |2472.60MB/s RAW        |1291.89MB/s RAW        
| 2nd|90.67% MCM             |97.72MB/s LZJB         |290.69MB/s LZ4         |119.72MB/s LZJB        
| 3rd|89.96% TANGELO         |71.45MB/s SHRINKER     |207.38MB/s SHRINKER    |106.29MB/s SHRINKER    
| 4th|88.24% BSC             |65.51MB/s SHOCO        |198.69MB/s LZ4F        |88.93MB/s LZ4F         
| 5th|87.65% LZMA25          |57.29MB/s LZ4F         |179.78MB/s MINIZ       |87.29MB/s SHOCO        
| 6th|87.65% LZIP            |48.64MB/s ZSTDF        |154.52MB/s LZJB        |61.25MB/s ZSTDF        
| 7th|87.54% BROTLI11        |21.47MB/s ZLING        |130.76MB/s SHOCO       |33.90MB/s ZLING        
| 8th|87.42% CSC20           |16.10MB/s CRUSH        |113.23MB/s BROTLI9     |27.60MB/s CRUSH        
| 9th|87.31% BCM             |07.15MB/s ZSTD         |112.88MB/s ZSTD        |13.45MB/s ZSTD         
|10th|86.33% ZMOLLY          |04.04MB/s BSC          |109.14MB/s BROTLI11    |04.80MB/s BSC          
|11th|86.02% LZMA20          |02.24MB/s BROTLI9      |96.73MB/s CRUSH        |04.39MB/s BROTLI9      
|12th|85.94% BROTLI9         |02.14MB/s ZMOLLY       |82.68MB/s ZSTDF        |02.78MB/s MINIZ        
|13th|85.13% ZSTD            |01.81MB/s BCM          |80.44MB/s ZLING        |02.64MB/s CSC20        
|14th|82.72% ZLING           |01.40MB/s MINIZ        |47.98MB/s LZMA25       |02.52MB/s LZIP         
|15th|81.99% MINIZ           |01.36MB/s CSC20        |45.61MB/s CSC20        |02.50MB/s LZMA25       
|16th|78.23% ZSTDF           |01.30MB/s LZIP         |43.04MB/s LZMA20       |02.46MB/s LZMA20       
|17th|77.93% LZ4             |01.28MB/s LZMA25       |37.68MB/s LZIP         |02.37MB/s LZ4          
|18th|77.04% CRUSH           |01.26MB/s LZMA20       |05.89MB/s BSC          |02.06MB/s ZMOLLY       
|19th|67.46% SHRINKER        |01.19MB/s LZ4          |01.99MB/s ZMOLLY       |01.75MB/s BCM          
|20th|63.41% LZ4F            |00.58MB/s MCM          |01.69MB/s BCM          |00.60MB/s MCM          
|21th|59.78% LZJB            |00.33MB/s TANGELO      |00.62MB/s MCM          |00.33MB/s TANGELO      
|22th|06.16% SHOCO           |00.22MB/s ZPAQ         |00.33MB/s TANGELO      |00.22MB/s ZPAQ         
|23th|00.00% RAW             |00.06MB/s BROTLI11     |00.22MB/s ZPAQ         |00.13MB/s BROTLI11     
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
|BUNDLE_NO_CRUSH|(undefined)|Define to remove CRUSH library from build
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
|[crush](http://sourceforge.net/projects/crush/)|Ilya Muravyov|Public Domain|1.00|reentrant fix|
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
- v2.0.4 (2015/12/04): Add padding support; Fix reentrant CRUSH; Optimizations & fixes
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
