#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bundle.hpp"

int main( int argc, char **argv )
{
    using namespace std;

    // 55 mb dataset
    string original( "There's a lady who's sure all that glitters is gold" );
    for (int i = 0; i < 20; ++i) original += original + string( i + 1, 32 + i );

    if( argc > 1 ) {
        ifstream ifs( argv[1], ios::binary );
        stringstream ss;
        ss << ifs.rdbuf();
        original = ss.str();
    }

    cout << "benchmarking compression of " << original.size() << " bytes..." << endl;

    // spinning
    bundle::pack( bundle::RAW, original );
    // some benchmarks
    auto datas = bundle::measures( original ); // , fast_encodings() );
    for( auto &data : datas ) {
        cout << data.str() << endl;
    }

    auto smallest = bundle::sort_smallest_encoders(datas);
    auto fastestc = bundle::sort_fastest_encoders(datas);
    auto fastestd = bundle::sort_fastest_decoders(datas);

    string rank = "smallest encoders: ";
    for( auto &R : smallest )
        cout << rank << bundle::name_of( datas[R].q ), rank = ',';
    cout << endl;

    rank = "fastest  encoders: ";
    for( auto &R : fastestc )
        cout << rank << bundle::name_of( datas[R].q ), rank = ',';
    cout << endl;

    rank = "fastest  decoders: ";
    for( auto &R : fastestd )
        cout << rank << bundle::name_of( datas[R].q ), rank = ',';
    cout << endl;

    cout << endl;
    cout << "|Rank|Smallest results|Fastest compressors|Fastest decompressors|Memory efficiency|" << endl;
    cout << "|---:|:---------------|:------------------|:--------------------|:----------------|" << endl;
    for( auto end = datas.size(), it = end - end; it < end; ++it ) {
        std::stringstream ss;
        const char *ord[] = { "st", "nd", "rd" };
        char buf[256];
        sprintf( buf, "|%d%s|%5.2f%% %s|%5.2fMB/s %s|%5.2fMB/s %s|tbd\n", int(it+1), it < 3 ? ord[it] : "th", 
            it >= smallest.size() ? 0 :                  datas[smallest[it]].ratio,
            it >= smallest.size() ? "": bundle::name_of( datas[smallest[it]].q ),
            it >= fastestc.size() ? 0 :                  datas[fastestc[it]].encspeed(),
            it >= fastestc.size() ? "": bundle::name_of( datas[fastestc[it]].q ),
            it >= fastestd.size() ? 0 :                  datas[fastestd[it]].decspeed(),
            it >= fastestd.size() ? "": bundle::name_of( datas[fastestd[it]].q )
        );
        cout << buf;
    }

    // write .zip file
    {
        bundle::archive pak;
        for( auto &result : datas ) {
            if( result.pass ) {
                pak.push_back( bundle::file() );
                pak.back()["name"] = string() + bundle::name_of(result.q);
                pak.back()["data"] = original;
            }
        }
        
        ofstream zip( "test.zip", std::ios::binary );
        zip << pak.zip(60); // 60% compression
    }

    // write .bnd file
    {
        bundle::archive pak;
        for( auto &result : datas ) {
            if( result.pass ) {
                pak.push_back( bundle::file() );
                pak.back()["name"] = string() + bundle::name_of(result.q);
                pak.back()["data"] = result.packed;
            }
        }
        
        ofstream bnd( "test.bnd", std::ios::binary );
        bnd << pak.bnd();
    }
}
