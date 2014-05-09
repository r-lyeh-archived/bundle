bundle
======

- Bundle is a lightweight C++ compression library that provides interface for LZMA, LZ77 and DEFLATE algorithms.
- Bundle features LZLIB, MINIZ and LZ4 de/compression libraries.
- Bundle features support for ZIP archivers.
- A plain English string compressor is also provided (SHOCO).
- Easy to integrate. Amalgamated distribution.
- Tiny. All dependencies included. Self-contained.
- Cross-platform.
- MIT licensed.

### sample
```c++
int main() {
    // 50 mb dataset
    std::string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original;

    for( auto &encoding : { LZ4, SHOCO, MINIZ, LZLIB, NONE } ) {
        std::string encname = bundle::name(encoding);
        std::string zipped = bundle::pack(original, encoding);
        std::string unzipped = bundle::unpack(compressed, encoding);
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
LZLIB: 53477376 to 7712 bytes
NONE: 53477376 to 53477376 bytes
All ok.
```

### on picking up compressors
- sorted by compression time: lz4 < miniz < lzlib
- sorted by decompression time: lz4 < miniz < lzlib
- sorted by memory overhead: lz4 < miniz < lzlib
- sorted by disk space: lzlib < miniz < lz4
- and maybe use SHOCO for plain text ascii IDs

### public api (@todoc)
- bool is_packed( const string &self )
- bool is_unpacked( const string &self )
- std::string pack( unsigned q, const string &self )
- std::string unpack( unsigned q, const string &self )
- bool pack( unsigned q, const char *in, size_t len, char *out, size_t &zlen )
- bool unpack( unsigned q, const char *in, size_t len, char *out, size_t &zlen )
- size_t bound( unsigned q, size_t len )
- const char *const nameof( unsigned q )
- const char *const version( unsigned q )
- const char *const extof( unsigned q )
- unsigned typeof( const void *mem, size_t size )
- bool pack( unsigned q, <T2> &buffer_out, const <T1> &buffer_in )
- bool unpack( unsigned q, <T2> &buffer_out, const <T1> &buffer_in )

### evaluated libraries
- [LZO](http://www.oberhumer.com/opensource/lzo/), [LibLZF](http://freshmeat.net/projects/liblzf), [FastLZ](http://fastlz.org/), replaced by [LZ4](https://code.google.com/p/lz4/)
- [ZLIB](http://www.zlib.net/), replaced by [miniz](https://code.google.com/p/miniz/)
- [LZHAM](https://code.google.com/p/lzham/), replaced by [lzlib](http://www.nongnu.org/lzip/lzlib.html)
- [SMAZ](https://github.com/antirez/smaz), replaced by [SHOCO](https://github.com/Ed-von-Schleck/shoco)

### licenses
- [bundle](https://github.com/r-lyeh/bundle), MIT license.
- [lzlib](http://www.nongnu.org/lzip/lzlib.html) by Antonio Diaz Diaz, GNU General Public License.
- [LZ4](https://code.google.com/p/lz4/) by Yann Collet, BSD license.
- [miniz](https://code.google.com/p/miniz/) by Rich Geldreich, public domain.
- [SHOCO](https://github.com/Ed-von-Schleck/shoco) by Christian Schramm, MIT license.
