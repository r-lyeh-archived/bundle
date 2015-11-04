# bundle :package: <a href="https://travis-ci.org/r-lyeh/bundle"><img src="https://api.travis-ci.org/r-lyeh/bundle.svg?branch=master" align="right" /></a>

Bundle is an embeddable compression library that supports 20 compression algorithms and 2 archive formats. Distributed in two files.

### Features
- [x] Archive support: .bnd, .zip
- [x] Stream support: DEFLATE, LZMA, LZIP, ZPAQ, LZ4, ZSTD, BROTLI, BSC, CSC, BCM, MCM, ZMOLLY, ZLING, TANGELO, SHRINKER and SHOCO 
- [x] Cross-platform, compiles on clang++/g++/visual studio (C++03).
- [x] Optimized for highest compression ratios on each compressor, where possible.
- [x] Optimized for fastest decompression times on each decompressor, where possible.
- [x] Configurable, redistributable, self-contained, amalgamated and cross-platform.
- [x] Optional benchmark infrastructure (C++11).
- [x] ZLIB/LibPNG licensed.

### Bundle stream format
```c++
[0x00 0x70 0x??]     Header (16 bits). De/compression algorithm (8 bits)
                     enum { RAW, SHOCO, LZ4F, MINIZ, LZIP, LZMA20, ZPAQ,         // 0.. 6
                            LZ4, BROTLI9, ZSTD, LZMA25, BSC, BROTLI11, SHRINKER, // 7..13
                            CSC20, ZSTDF, BCM, ZLING, MCM, TANGELO, ZMOLLY       //14..20
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
        BSC, BROTLI11, SHRINKER, CSC20,
        BCM, ZLING, MCM, TANGELO, ZMOLLY
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

### Tips on choosing compressors (on a regular basis)
|Rank|Smallest results|Fastest compressors|Fastest decompressors|Memory efficiency|
|---:|:---------------|:------------------|:--------------------|:----------------|
|1st|BSC|ZSTDF|LZ4|tbd|
|2nd|BROTLI11|LZ4F|LZ4F|tbd|
|3rd|BROTLI9|SHRINKER|ZSTDF|tbd|
|4th|MCM|BSC|ZSTD|tbd|
|5th|ZPAQ|ZLING|MINIZ|tbd|
|6th|CSC20|LZ4|SHRINKER|tbd|
|7th|LZMA25|ZSTD|BROTLI11|tbd|
|8th|ZSTD|MINIZ|BROTLI9|tbd|
|9th|LZIP|SHOCO|ZLING|tbd|
|10th|TANGELO|BROTLI9|LZMA20|tbd|
|11th|LZMA20|CSC20|LZMA25|tbd|
|12th|BCM|ZMOLLY|BSC|tbd|
|13th|ZMOLLY|LZMA20|LZIP|tbd|
|14th|ZSTDF|LZIP|CSC20|tbd|
|15th|ZLING|LZMA25|SHOCO|tbd|
|16th|LZ4|MCM|ZMOLLY|tbd|
|17th|SHRINKER|BROTLI11|MCM|tbd|
|18th|MINIZ|BCM|BCM|tbd|
|19th|LZ4F|TANGELO|TANGELO|tbd|
|20th|SHOCO|ZPAQ|ZPAQ|tbd|

- Note: SHOCO is a _text_ compressor intended to be used for plain ascii IDs only.

### Possible benchmark output (some real numbers)
|Name|Ratio|Size|Enc time|Dec time|Enc speed|Dec speed|Avg speed
|:--:|----:|---:|-------:|-------:|--------:|--------:|--------:
|NONE|0%|55574506|120585us|35647us|439.522 MB/s|1486.77 MB/s|963.146 MB/s
|BCM|99.9207%|44049|19181252us|17552351us|2.76311 MB/s|3.01954 MB/s|2.89133 MB/s
|BROTLI11|99.9984%|865|12137308us|75504us|4.3667 MB/s|701.942 MB/s|353.154 MB/s
|BROTLI9|99.998%|1087|740223us|77377us|71.6 MB/s|684.951 MB/s|378.276 MB/s
|BSC|99.9991%|525|118282us|88436us|448.078 MB/s|599.298 MB/s|523.688 MB/s
|CSC20|99.9696%|16898|780183us|252945us|67.9327 MB/s|209.531 MB/s|138.732 MB/s
|LZ4F|96.2244%|2098286|53155us|41055us|997.082 MB/s|1290.95 MB/s|1144.01 MB/s
|LZ4|99.5944%|225410|217984us|40475us|243.136 MB/s|1309.43 MB/s|776.284 MB/s
|LZIP|99.9574%|23652|4915601us|220899us|10.782 MB/s|239.928 MB/s|125.355 MB/s
|LZMA20|99.9346%|36356|4766429us|85290us|11.1194 MB/s|621.403 MB/s|316.261 MB/s
|LZMA25|99.9667%|18514|4997295us|86774us|10.6057 MB/s|610.781 MB/s|310.693 MB/s
|MCM|99.997%|1691|5066738us|3422941us|10.4604 MB/s|15.4838 MB/s|12.9721 MB/s
|MINIZ|99.4327%|315272|273670us|42861us|193.663 MB/s|1236.55 MB/s|715.105 MB/s
|SHOCO|26.4155%|40894197|565445us|258275us|93.7313 MB/s|205.207 MB/s|149.469 MB/s
|SHRINKER|99.4873%|284913|86289us|45677us|614.211 MB/s|1160.3 MB/s|887.254 MB/s
|TANGELO|99.9482%|28795|37028795us|38099556us|1.43132 MB/s|1.39109 MB/s|1.4112 MB/s
|ZLING|99.6164%|213176|149697us|79623us|354.048 MB/s|665.63 MB/s|509.839 MB/s
|ZMOLLY|99.8866%|63019|856057us|850040us|61.9117 MB/s|62.35 MB/s|62.1308 MB/s
|ZPAQ|99.9932%|3777|251637719us|231336455us|0.21062 MB/s|0.229103 MB/s|0.219862 MB/s
|ZSTDF|99.8678%|73462|49326us|41852us|1074.47 MB/s|1266.34 MB/s|1170.4 MB/s
|ZSTD|99.9652%|19344|223065us|42336us|237.598 MB/s|1251.88 MB/s|744.737 MB/s

```
enwik8
[ OK ] NONE: ratio=0% enctime=145008us dectime=62003us (zlen=100000000 bytes)(enc:657.67 MB/s,dec:1538.11 MB/s,avg:1097.89 MB/s)
[ OK ] LZ4F: ratio=42.7377% enctime=474027us dectime=119006us (zlen=57262292 bytes)(enc:201.186 MB/s,dec:801.367 MB/s,avg:501.276 MB/s)
[ OK ] SHOCO: ratio=22.5426% enctime=1168066us dectime=525030us (zlen=77457447 bytes)(enc:81.6456 MB/s,dec:181.642 MB/s,avg:131.644 MB/s)
[ OK ] MINIZ: ratio=63.5402% enctime=8125464us dectime=407023us (zlen=36459843 bytes)(enc:11.7369 MB/s,dec:234.305 MB/s,avg:123.021 MB/s)
[ OK ] LZIP: ratio=73.4826% enctime=79136526us dectime=1811103us (zlen=26517402 bytes)(enc:1.2051 MB/s,dec:52.6571 MB/s,avg:26.9311 MB/s)
[ OK ] LZMA20: ratio=71.3133% enctime=51036919us dectime=1614092us (zlen=28686663 bytes)(enc:1.8686 MB/s,dec:59.0843 MB/s,avg:30.4764 MB/s)
[ OK ] LZMA25: ratio=74.6044% enctime=98499633us dectime=1540088us (zlen=25395642 bytes)(enc:0.968201 MB/s,dec:61.9234 MB/s,avg:31.4458 MB/s)
[ OK ] LZ4: ratio=57.8031% enctime=5016286us dectime=116006us (zlen=42196884 bytes)(enc:19.0116 MB/s,dec:822.091 MB/s,avg:420.551 MB/s)
[ OK ] ZSTD: ratio=68.6681% enctime=35749044us dectime=539030us (zlen=31331913 bytes)(enc:2.66769 MB/s,dec:176.924 MB/s,avg:89.7959 MB/s)
[ OK ] BSC: ratio=79.2138% enctime=20642180us dectime=13777788us (zlen=20786211 bytes)(enc:4.62003 MB/s,dec:6.92182 MB/s,avg:5.77093 MB/s)
[ OK ] BROTLI9: ratio=70.3047% enctime=89754133us dectime=654037us (zlen=29695270 bytes)(enc:1.06254 MB/s,dec:145.814 MB/s,avg:73.438 MB/s)
[ OK ] SHRINKER: ratio=48.507% enctime=616035us dectime=216012us (zlen=51493031 bytes)(enc:154.808 MB/s,dec:441.491 MB/s,avg:298.15 MB/s)
[ OK ] CSC20: ratio=74.8646% enctime=45165583us dectime=2440139us (zlen=25135369 bytes)(enc:2.11151 MB/s,dec:39.0828 MB/s,avg:20.5971 MB/s)
[ OK ] BCM: ratio=77.4681% enctime=23395338us dectime=30824763us (zlen=22531862 bytes)(enc:4.07634 MB/s,dec:3.09386 MB/s,avg:3.5851 MB/s)
[ OK ] ZLING: ratio=69.2923% enctime=5462312us dectime=938053us (zlen=30707669 bytes)(enc:17.4592 MB/s,dec:101.665 MB/s,avg:59.5622 MB/s)
[ OK ] MCM: ratio=80.7978% enctime=48620780us dectime=45504602us (zlen=19202227 bytes)(enc:1.96145 MB/s,dec:2.09578 MB/s,avg:2.02861 MB/s)
[ OK ] ZMOLLY: ratio=76.436% enctime=21215213us dectime=22531288us (zlen=23563967 bytes)(enc:4.49524 MB/s,dec:4.23267 MB/s,avg:4.36395 MB/s)
[ OK ] ZSTDF: ratio=59.1564% enctime=707040us dectime=373021us (zlen=40843581 bytes)(enc:134.883 MB/s,dec:255.662 MB/s,avg:195.273 MB/s)
[ OK ] BROTLI11: ratio=72.8748% enctime=742856489us dectime=662037us (zlen=27125160 bytes)(enc:0.128379 MB/s,dec:144.052 MB/s,avg:72.0899 MB/s)
[ OK ] ZPAQ: ratio=79.403% enctime=417270866us dectime=426077370us (zlen=20597003 bytes)(enc:0.22855 MB/s,dec:0.223827 MB/s,avg:0.226188 MB/s)
[ OK ] TANGELO: ratio=78.9696% enctime=107811166us dectime=112691445us (zlen=21030383 bytes)(enc:0.884578 MB/s,dec:0.84627 MB/s,avg:0.865424 MB/s)
smallest encoders: MCM,ZPAQ,BSC,TANGELO,BCM,ZMOLLY,CSC20,LZMA25,LZIP,BROTLI11,LZMA20,BROTLI9,ZLING,ZSTD,MINIZ,ZSTDF,LZ4,SHRINKER,LZ4F,SHOCO
fastest  encoders: LZ4F,SHRINKER,ZSTDF,SHOCO,LZ4,ZLING,MINIZ,BSC,ZMOLLY,BCM,ZSTD,CSC20,MCM,LZMA20,LZIP,BROTLI9,LZMA25,TANGELO,ZPAQ,BROTLI11
fastest  decoders: LZ4,LZ4F,SHRINKER,ZSTDF,MINIZ,SHOCO,ZSTD,BROTLI9,BROTLI11,ZLING,LZMA25,LZMA20,LZIP,CSC20,BSC,ZMOLLY,BCM,MCM,TANGELO,ZPAQ
```

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
|BUNDLE_NO_CSC|(undefined)|Define to remove CSC library from build
|BUNDLE_NO_LZ4|(undefined)|Define to remove LZ4/LZ4 libraries 
|BUNDLE_NO_LZIP|(undefined)|Define to remove EasyLZMA library from build
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
|:--------:|:---------:|:-------:|:-------:|:---------------------|
|[bundle](https://github.com/r-lyeh/bundle)|r-lyeh|ZLIB/LibPNG|latest||
|[bcm](http://sourceforge.net/projects/bcm/)|Ilya Muravyov|Public Domain|1.00|istream based now|
|[brotli](https://github.com/google/brotli)|Jyrki Alakuijala, Zoltan Szabadka|Apache 2.0|2015/11/03||
|[csc](https://github.com/fusiyuan2010/CSC)|Siyuan Fu|Public Domain|2015/06/16||
|[easylzma](https://github.com/lloyd/easylzma)|Igor Pavlov, Lloyd Hilaiel|Public Domain|0.0.7||
|[endian](https://gist.github.com/panzi/6856583)|Mathias Panzenböck|Public Domain||msvc fix|
|[libbsc](https://github.com/IlyaGrebnov/libbsc)|Ilya Grebnov|Apache 2.0|3.1.0||
|[libzling](https://github.com/richox/libzling)|Zhang Li|BSD-3|2015/09/16||
|[libzpaq](https://github.com/zpaq/zpaq)|Matt Mahoney|Public Domain|7.05||
|[lz4](https://github.com/Cyan4973/lz4)|Yann Collet|BSD-2|1.7.1||
|[mcm](https://github.com/mathieuchartier/mcm)|Mathieu Chartier|GPL|0.84||
|[miniz](https://code.google.com/p/miniz/)|Rich Geldreich|Public Domain|v1.15 r.4.1|alignment fix|
|[shoco](https://github.com/Ed-von-Schleck/shoco)|Christian Schramm|MIT|2015/03/16||
|[shrinker](https://code.google.com/p/data-shrinker/)|Siyuan Fu|BSD-3|rev 3||
|[tangelo](http://encode.ru/threads/1738-TANGELO-new-compressor-(derived-from-PAQ8-FP8))|Matt Mahoney, Jan Ondrus|GPL|2.41|reentrant fixes, istream based now|
|[zmolly](https://github.com/richox/zmolly)|Zhang Li|BSD-3|0.0.1|reentrant and memstream fixes|
|[zstd](https://github.com/Cyan4973/zstd)|Yann Collet|BSD-2|0.3.2||

### Evaluated alternatives
[FastLZ](http://fastlz.org/), [FLZP](http://cs.fit.edu/~mmahoney/compression/#flzp), [LibLZF](http://freshmeat.net/projects/liblzf), [LZFX](https://code.google.com/p/lzfx/), [LZHAM](https://code.google.com/p/lzham/), [LZJB](http://en.wikipedia.org/wiki/LZJB), [LZLIB](http://www.nongnu.org/lzip/lzlib.html), [LZO](http://www.oberhumer.com/opensource/lzo/), [LZP](http://www.cbloom.com/src/index_lz.html), [SMAZ](https://github.com/antirez/smaz), [Snappy](https://code.google.com/p/snappy/), [ZLIB](http://www.zlib.net/), [bzip2](http://www.bzip2.org/), [Yappy](http://blog.gamedeff.com/?p=371), [CMix](http://www.byronknoll.com/cmix.html), [Crush](http://sourceforge.net/projects/crush/), [M1](https://sites.google.com/site/toffer86/m1-project)

### Changelog
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
