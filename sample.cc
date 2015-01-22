#include <cassert>
#include "bundle.hpp"

int main() {
    // 50 mb dataset
    std::string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original;

    using namespace bundle;
    for( auto &encoding : std::vector<unsigned> { NONE, LZ4, LZ4HC, SHOCO, MINIZ, LZIP, LZMA, ZPAQ, BROTLI } ) {
        std::string packed = pack(encoding, original);
        std::string unpacked = unpack(packed);
        std::cout << name_of(encoding) << ": " << original.size() << " to " << packed.size() << " bytes" << std::endl;
        assert( original == unpacked );
    }

    std::cout << "All ok." << std::endl;
}
