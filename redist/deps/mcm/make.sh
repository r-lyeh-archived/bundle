g++ -DNDEBUG -O3 -fomit-frame-pointer -msse2 -std=c++0x -D_FILE_OFFSET_BITS=64 -o mcm CM.cpp Archive.cpp Huffman.cpp MCM.cpp Memory.cpp Util.cpp Compressor.cpp LZ.cpp -lpthread
