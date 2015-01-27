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
