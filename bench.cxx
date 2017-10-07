#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bundle.h"

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
    auto datas = bundle::measures( original, bundle::encodings() );
    for( auto &data : datas ) {
        cout << data.str() << endl;
    }

    auto smallest = bundle::sort_smallest_encoders(datas);
    auto fastestc = bundle::sort_fastest_encoders(datas);
    auto fastestd = bundle::sort_fastest_decoders(datas);
    auto averaged = bundle::sort_average_coders(datas);

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

    rank = "average    coders: ";
    for( auto &R : averaged )
        cout << rank << bundle::name_of( datas[R].q ), rank = ',';
    cout << endl;

    cout << endl;
    cout << "|Rank|Compression ratio      |Fastest compressors    |Fastest decompressors  |Average speed          |Memory efficiency|" << endl;
    cout << "|---:|:----------------------|:----------------------|:----------------------|:----------------------|:----------------|" << endl;
    for( auto end = datas.size(), it = end - end; it < end; ++it ) {
        std::stringstream ss;
        const char *ord[] = { "st", "nd", "rd" };
        char buf[6][256];
        sprintf( buf[0], "%d%s", int(it+1), it < 3 ? ord[it] : "th" );
        sprintf( buf[1], "%05.2f%% %s", 
            it >= smallest.size() ? 0 :                  datas[smallest[it]].ratio,
            it >= smallest.size() ? "": bundle::name_of( datas[smallest[it]].q ));
        sprintf( buf[2], "%05.2fMB/s %s",
            it >= fastestc.size() ? 0 :                  datas[fastestc[it]].encspeed(),
            it >= fastestc.size() ? "": bundle::name_of( datas[fastestc[it]].q ));
        sprintf( buf[3], "%05.2fMB/s %s",
            it >= fastestd.size() ? 0 :                  datas[fastestd[it]].decspeed(),
            it >= fastestd.size() ? "": bundle::name_of( datas[fastestd[it]].q ));
        sprintf( buf[4], "%05.2fMB/s %s",
            it >= averaged.size() ? 0 :                  datas[averaged[it]].avgspeed(),
            it >= averaged.size() ? "": bundle::name_of( datas[averaged[it]].q ));
        sprintf( buf[5], "|%4s|%-23s|%-23s|%-23s|%-23s|tbd", buf[0], buf[1], buf[2], buf[3], buf[4]);
        cout << buf[5] << endl;
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

    // write .bun file
    {
        bundle::archive pak;
        for( auto &result : datas ) {
            if( result.pass ) {
                pak.push_back( bundle::file() );
                pak.back()["name"] = string() + bundle::name_of(result.q);
                pak.back()["data"] = result.packed;
            }
        }
        
        ofstream bun( "test.bun", std::ios::binary );
        bun << pak.bun();
    }
}
