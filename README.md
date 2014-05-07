bundle
======

- Bundle is a lightweight C++11 compression library that provides interface for LZMA, LZ77 and DEFLATE algorithms.
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

    for( auto &encoding : bundle::encodings() ) {
        std::string encname = bundle::name(encoding);
        std::string zipped = bundle::z(original, encoding);
        std::string unzipped = bundle::unz(compressed, encoding);
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

### evaluated libraries
- [LZO](http://www.oberhumer.com/opensource/lzo/), [LibLZF](http://freshmeat.net/projects/liblzf), [FastLZ](http://fastlz.org/), replaced by [LZ4](https://code.google.com/p/lz4/)
- [ZLIB](http://www.zlib.net/), replaced by [miniz](https://code.google.com/p/miniz/)
- [LZHAM](https://code.google.com/p/lzham/), replaced by [lzlib](http://www.nongnu.org/lzip/lzlib.html)
- [SMAZ](https://github.com/antirez/smaz), replaced by [SHOCO](https://github.com/Ed-von-Schleck/shoco)

## licenses
- [bundle](https://github.com/r-lyeh/bundle), MIT license.
- [lzlib](http://www.nongnu.org/lzip/lzlib.html) by Antonio Diaz Diaz, GNU General Public License.
- [LZ4](https://code.google.com/p/lz4/) by Yann Collet, BSD license.
- [miniz](https://code.google.com/p/miniz/) by Rich Geldreich, public domain.
- [SHOCO](https://github.com/Ed-von-Schleck/shoco) by Christian Schramm, MIT license.
