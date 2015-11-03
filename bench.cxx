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

    // some benchmarks
    auto data = bundle::measures( original ); // , fast_encodings() );
    for( auto &in : data ) {
        cout << in.str() << endl;
    }

    auto smallest = bundle::find_smallest_encoders(data);
    auto fastestc = bundle::find_fastest_encoders(data);
    auto fastestd = bundle::find_fastest_decoders(data);

    string rank = "smallest encoders: ";
    for( auto &R : smallest )
        cout << rank << bundle::name_of( R ), rank = ',';
    cout << endl;

    rank = "fastest  encoders: ";
    for( auto &R : fastestc )
        cout << rank << bundle::name_of( R ), rank = ',';
    cout << endl;

    rank = "fastest  decoders: ";
    for( auto &R : fastestd )
        cout << rank << bundle::name_of( R ), rank = ',';
    cout << endl;

    cout << endl;
    cout << "|Rank|Smallest results|Fastest compressors|Fastest decompressors|Memory efficiency|" << endl;
    cout << "|---:|:---------------|:------------------|:--------------------|:----------------|" << endl;
    for( auto end = data.size(), it = end - end; it < end; ++it ) {
        std::string ord[] = { "st", "nd", "rd" };
        cout << "|" << (it+1) << ( it < 3 ? ord[it] : "th" )
              + "|" + (it < smallest.size() ? bundle::name_of( smallest[it] ) : string())
              + "|" + (it < fastestc.size() ? bundle::name_of( fastestc[it] ) : string())
              + "|" + (it < fastestd.size() ? bundle::name_of( fastestd[it] ) : string())
              + "|tbd|" << endl;
    }

    // write .zip file
    {
        bundle::archive pak;
        for( auto &result : data ) {
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
        for( auto &result : data ) {
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
