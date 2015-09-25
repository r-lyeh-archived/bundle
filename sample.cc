#include <cassert>
#include "bundle.hpp"

int main() {
    // 55 mb dataset
    std::string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original + std::string( i + 1, 32 + i );

    // pack, unpack & verify a few encoders
    using namespace bundle;
    std::vector<unsigned> libs { RAW, LZ4F, LZ4, SHOCO, MINIZ, LZMA20, LZIP, LZMA25, BROTLI9, BROTLI11, ZSTD, BSC, SHRINKER, CSC20 };
    for( auto &use : libs ) {
        std::string packed = pack(use, original);
        std::string unpacked = unpack(packed);
        std::cout << name_of(use) << ": " << original.size() << " to " << packed.size() << " bytes" << std::endl;
        assert( original == unpacked );
    }

    std::cout << "All ok." << std::endl;
}
