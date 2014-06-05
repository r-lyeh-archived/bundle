bundle
======

- Bundle is a lightweight C++ compression library that provides interface for ZIP, LZMA, LZIP, LZ4 and SHOCO algorithms.
- Bundle is easy to integrate, comes in an amalgamated distribution.
- Bundle is tiny. All dependencies included. Self-contained.
- Bundle is cross-platform.
- Bundle is MIT licensed.

### sample
```c++
int main() {
    // 50 mb dataset
    std::string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original;

    for( auto &encoding : { LZ4, SHOCO, MINIZ, LZIP, LZMA, NONE } ) {
        std::string encname = bundle::name(encoding);
        std::string zipped = bundle::pack(original, encoding);
        std::string unzipped = bundle::unpack(compressed);
        std::cout << encname << ": " << original.size() << " to " << zipped.size() << " bytes" << std::endl;
        assert( original == unzipped );
    }

    std::cout << "All ok." << std::endl;
}
```

### possible output
```
LZ4: 53477376 to 209793 bytes
SHOCO: 53477376 to 38797329 bytes
MINIZ: 53477376 to 155658 bytes
LZIP: 53477376 to 7695 bytes
LZMA: 53477376 to 7690 bytes
NONE: 53477376 to 53477376 bytes
All ok.
```

### on picking up compressors
- sorted by compression time: lz4 < miniz < lzma2
- sorted by decompression time: lz4 < miniz < lzma2
- sorted by memory overhead: lz4 < miniz < lzma2
- sorted by disk space: lzma2 < miniz < lz4
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

### evaluated libraries
- [LZO](http://www.oberhumer.com/opensource/lzo/), [LibLZF](http://freshmeat.net/projects/liblzf), [FastLZ](http://fastlz.org/), replaced by [LZ4](https://code.google.com/p/lz4/)
- [ZLIB](http://www.zlib.net/), replaced by [miniz](https://code.google.com/p/miniz/)
- [LZHAM](https://code.google.com/p/lzham/), [lzlib](http://www.nongnu.org/lzip/lzlib.html), replaced by [easylzma](https://github.com/lloyd/easylzma)
- [SMAZ](https://github.com/antirez/smaz), replaced by [SHOCO](https://github.com/Ed-von-Schleck/shoco)

### licenses
- [bundle](https://github.com/r-lyeh/bundle), MIT license.
- [easylzma](https://github.com/lloyd/easylzma) by Igor Pavlov and Lloyd Hilaiel, public domain.
- [LZ4](https://code.google.com/p/lz4/) by Yann Collet, BSD license.
- [miniz](https://code.google.com/p/miniz/) by Rich Geldreich, public domain.
- [SHOCO](https://github.com/Ed-von-Schleck/shoco) by Christian Schramm, MIT license.
