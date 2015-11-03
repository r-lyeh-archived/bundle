#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bundle.hpp"

void zip_tests() {
    std::string binary;

    if( const bool zip_saving_test = true )
    {
        bundle::archive pak;

        pak.resize(2);

        pak[0]["name"] = "test.txt";
        pak[0]["data"] = "hello world";

        pak[1]["name"] = "test2.txt";
        pak[1]["data"] = "1337";

        std::cout << "zipping files..." << std::endl;

        // save .zip archive to memory string (then optionally to disk)
        binary = pak.zip(60); // compression level = 60 (of 100)

        std::cout << "saving test:\n" << pak.toc() << std::endl;
    }

    if( const bool zip_loading_test = true )
    {
        std::cout << "unzipping files..." << std::endl;

        bundle::archive pak;
        pak.zip( binary );

        std::cout << "loading test:\n" << pak.toc() << std::endl;

        assert( pak.size() == 2 );

        assert( pak[0]["name"] == "test.txt" );
        assert( pak[0]["data"] == "hello world" );

        assert( pak[1]["name"] == "test2.txt" );
        assert( pak[1]["data"] == "1337" );
    }
}

void bnd_tests() {
    std::string binary;

    if( const bool bnd_saving_test = true )
    {
        bundle::archive pak;
        pak.resize(2);

        pak[0]["name"] = "test_lz4.txt";
        pak[0]["data"] = "hellohellohellohellohellohello";
        pak[0]["data"] = bundle::pack( bundle::LZ4, pak[0]["data"] );

        pak[1]["name"] = "test_shoco.txt";
        pak[1]["data"] = "hellohellohellohellohellohello";
        pak[1]["data"] = bundle::pack( bundle::SHOCO, pak[1]["data"] );

        std::cout << "packing files..." << std::endl;

        // save .bnd archive to memory string (then optionally to disk)
        binary = pak.bnd();

        std::cout << "saving test:\n" << pak.toc() << std::endl;
    }

    if( const bool bnd_loading_test = true )
    {
        std::cout << "unpacking files..." << std::endl;

        bundle::archive pak;
        pak.bnd( binary );

        std::cout << "loading test:\n" << pak.toc() << std::endl;

        assert( pak.size() == 2 );

        pak[0]["data"] = bundle::unpack( pak[0]["data"] );
        pak[1]["data"] = bundle::unpack( pak[1]["data"] );

        std::cout << pak[0]["data"] << std::endl;

        assert( pak[0]["name"] == "test_lz4.txt" );
        assert( pak[1]["data"] == "hellohellohellohellohellohello" );

        assert( pak[1]["name"] == "test_shoco.txt" );
        assert( pak[1]["data"] == "hellohellohellohellohellohello" );

        std::cout << pak.toc() << std::endl;
    }
}

int main() {
    zip_tests();
    bnd_tests();
    std::cout << "All ok." << std::endl;
}
