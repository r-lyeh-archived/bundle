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
        bundle::archive pak;

        pak.resize(2);

        pak[0]["name"] = "test.txt";
        pak[0]["data"] = "hello world";

        pak[1]["name"] = "test2.txt";
        pak[1]["data"] = "1337";

        std::cout << "zipping files..." << std::endl;

        // save zip archive to memory string (then optionally to disk)
        binary = pak.bin();

        std::cout << "saving test:\n" << pak.toc() << std::endl;
    }

    if( const bool loading_test = true )
    {
        std::cout << "unzipping files..." << std::endl;

        bundle::archive pak;
        pak.bin( binary );

        std::cout << "loading test:\n" << pak.toc() << std::endl;

        assert( pak.size() == 2 );

        assert( pak[0]["name"] == "test.txt" );
        assert( pak[0]["data"] == "hello world" );

        assert( pak[1]["name"] == "test2.txt" );
        assert( pak[1]["data"] == "1337" );
    }

    if( const bool compression_tests = true )
    {
        using namespace bundle;

        // 55 mb dataset
        std::string original( "There's a lady who's sure all that glitters is gold" );
        for (int i = 0; i < 20; ++i) original += original + std::string( i + 1, 32 + i );

        if( argc > 1 ) {
            std::ifstream ifs( argv[1], std::ios::binary );
            std::stringstream ss;
            ss << ifs.rdbuf();
            original = ss.str();
        }

        std::cout << "benchmarking compression of " << original.size() << " bytes..." << std::endl;

        // some benchmarks
        auto data = measures( original ); // , fast_encodings() );
        for( auto &in : data ) {
            std::cout << in.str() << std::endl;
        }

        std::cout << "fastest decompressor: " << name_of( find_fastest_decompressor(data) ) << std::endl;
        std::cout << "fastest compressor: " << name_of( find_fastest_compressor(data) ) << std::endl;
        std::cout << "smallest compressor: " << name_of( find_smallest_compressor(data) ) << std::endl;

        bundle::archive pak;

        for( auto &result : data ) {
            if( result.pass ) {
                pak.push_back( bundle::file() );
                pak.back()["name"] = std::string() + name_of(result.q);
                pak.back()["type"] = std::string() + ext_of(result.q);
                pak.back()["data"] = result.packed;
            }
        }

        std::cout << "toc:\n" << pak.toc() << std::endl;

        std::ofstream ofs( "test.zip", std::ios::binary );
        ofs << pak.bin();
    }

    std::cout << "All ok." << std::endl;
}
