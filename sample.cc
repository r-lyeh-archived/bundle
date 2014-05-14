#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#include "bundle.hpp"

int main( int argc, char **argv )
{
    std::string binary;

    if( const bool saving_test = true )
    {
        bundle::pak pak;

        pak.resize(2);

        pak[0]["filename"] = "test.txt";
        pak[0]["content"] = "hello world";

        pak[1]["filename"] = "test2.txt";
        pak[1]["content"] = 1337;

        std::cout << "zipping files..." << std::endl;

        // save zip archive to memory string (then optionally to disk)
        binary = pak.bin();

        std::cout << "saving test:\n" << pak.toc() << std::endl;
    }

    if( const bool loading_test = true )
    {
        std::cout << "unzipping files..." << std::endl;

        bundle::pak pak;
        pak.bin( binary );

        std::cout << "loading test:\n" << pak.toc() << std::endl;

        assert( pak.size() == 2 );

        assert( pak[0]["filename"] == "test.txt" );
        assert( pak[0]["content"] == "hello world" );

        assert( pak[1]["filename"] == "test2.txt" );
        assert( pak[1]["content"] == "1337" );
    }

    if( const bool compression_tests = true )
    {
        using namespace bundle;

        // 50 mb dataset
        std::string original( "There's a lady who's sure all that glitters is gold" );
        for (int i = 0; i < 20; ++i) original += original;

        if( argc > 1 ) {
            std::ifstream ifs( argv[1], std::ios::binary );
            std::stringstream ss;
            ss << ifs.rdbuf();
            original = ss.str();
        }

        std::cout << "benchmarking compression of " << original.size() << " bytes..." << std::endl;

        // some benchmarks
        auto data = measures( original );
        for( auto &in : data ) {
            std::cout << in.str() << std::endl;
        }

        std::cout << "compressing " << original.size() << " bytes..." << std::endl;

        bundle::pak pak;

        for( auto &encoding : encodings() ) {
            pak.push_back( pakfile() );
            pak.back()["filename"] = std::string() + name_of(encoding);
            pak.back()["ext"] = std::string() + ext_of(encoding);
            pak.back()["content"] = pack( encoding, original );
        }

        std::cout << "toc:\n" << pak.toc() << std::endl;

        std::ofstream ofs( "test.zip", std::ios::binary );
        ofs << pak.bin();
    }

    std::cout << "All ok." << std::endl;
}
