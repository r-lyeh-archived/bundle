/*
 * Simple compression interface.
 * Copyright (c) 2013, 2014, 2015, Mario 'rlyeh' Rodriguez
 *
 * Distributed under the zlib/libpng license

 * - rlyeh ~~ listening to Boris / Missing Pieces
 */

#ifndef BUNDLE_HPP
#define BUNDLE_HPP

#if ( defined(_MSC_VER) && _MSC_VER >= 1800 ) || __cplusplus >= 201103L
#define BUNDLE_CXX11 1
#else
#define BUNDLE_CXX11 0
#endif

#define BUNDLE_VERSION "0.9.7" /* (2015/10/07) Add license configuration directives { BUNDLE_NO_BSD2, BUNDLE_NO_BSD3, ... }
#define BUNDLE_VERSION "0.9.6" // (2015/10/03) Add library configuration directives { BUNDLE_NO_ZSTD, BUNDLE_NO_CSC, ... }
#define BUNDLE_VERSION "0.9.5" // (2015/09/28) Add missing prototypes; bugfix helper function
#define BUNDLE_VERSION "0.9.4" // (2015/09/26) Add CSC20 + Shrinker support; rename enums LZ4->LZ4F/LZ4HC->LZ4
#define BUNDLE_VERSION "0.9.3" // (2015/09/25) Add a few missing API calls
#define BUNDLE_VERSION "0.9.2" // (2015/09/22) Pump up Brotli; split BROTLI enum into BROTLI9/11 pair
#define BUNDLE_VERSION "0.9.1" // (2015/05/10) Switch to ZLIB/LibPNG license
#define BUNDLE_VERSION "0.9.0" // (2015/04/08) BSC support
#define BUNDLE_VERSION "0.8.1" // (2015/04/07) Pump up Brotli+ZSTD, LZMA20/25 dict, unify FOURCCs
#define BUNDLE_VERSION "0.8.0" // (2015/01/27) ZSTD support, reorder enums, simplify API
#define BUNDLE_VERSION "0.7.1" // (2015/01/26) Fix LZMA, verify DEFLATEs, new AUTO enum
#define BUNDLE_VERSION "0.7.0" // (2014/10/22) Brotli support, pump up LZ4
#define BUNDLE_VERSION "0.6.3" // (2014/09/27) Switch to BOOST license
#define BUNDLE_VERSION "0.6.2" // (2014/09/02) Fix 0-byte streams, deflate alignment
#define BUNDLE_VERSION "0.6.1" // (2014/06/30) Safer lz4 decompression, pump up lz4+zpaq
#define BUNDLE_VERSION "0.6.0" // (2014/06/26) LZ4HC support, optimize in-place decompression
#define BUNDLE_VERSION "0.5.0" // (2014/06/09) ZPAQ support, UBER encoding, fixes
#define BUNDLE_VERSION "0.4.1" // (2014/06/05) Switch to lzmasdk
#define BUNDLE_VERSION "0.4.0" // (2014/05/30) Maximize compression (lzma)
#define BUNDLE_VERSION "0.3.0" // (2014/05/28) Fix alignment (deflate), change stream header
#define BUNDLE_VERSION "0.2.1" // (2014/05/23) Fix overflow bug
#define BUNDLE_VERSION "0.2.0" // (2014/05/14) Add VLE header, fix vs201x compilation errors
#define BUNDLE_VERSION "0.1.0" // (2014/05/13) Add high-level API, iOS support
#define BUNDLE_VERSION "0.0.0" // (2014/05/09) Initial commit */

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if BUNDLE_CXX11
#include <chrono>
#endif

#ifdef BUNDLE_USE_OMP_TIMER
#include <omp.h>
#endif

namespace bundle
{
    // libraries and/or encoders
    enum { RAW, SHOCO, LZ4F, MINIZ, LZIP, LZMA20, ZPAQ, LZ4, BROTLI9, ZSTD, LZMA25, BSC, BROTLI11, SHRINKER, CSC20 }; /* archival: BZIP2, LZFX, LZHAM, LZP1, FSE, BLOSC, YAPPY */
    // some algorithm aliases
    enum { UNDEFINED = RAW, ASCII = SHOCO, BINARY = MINIZ, LZ77 = LZ4, DEFLATE = MINIZ, LZMA = LZMA20, CM = ZPAQ }; /* archival: BWT = BZIP2 */
    // speed/ratio aliases
    enum { NONE = RAW, UNCOMPRESSED = RAW, VERY_FAST = LZ4F, FAST = LZ4, DEFAULT = MINIZ, OVER = LZMA20, EXTRA = LZMA25, UBER = ZPAQ, AUTO = ~0u };

    // algorithm properties
    const char *const name_of( unsigned q );
    const char *const version_of( unsigned q );
    const char *const ext_of( unsigned q );
    size_t unc_payload( unsigned q );
    size_t bound( unsigned q, size_t len );

    // dont compress if compression ratio is below 5%
    enum { NO_COMPRESSION_TRESHOLD = 5 };

    // low level API (raw pointers)

    bool is_packed( const void *mem, size_t size );
    bool is_unpacked( const void *mem, size_t size );
    unsigned type_of( const void *mem, size_t size );
    unsigned guess_type_of( const void *mem, size_t size );
    size_t len( const void *mem, size_t size );
    size_t zlen( const void *mem, size_t size );
    const void *zptr( const void *mem, size_t size );
    bool pack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );
    bool unpack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );

    std::string vlebit( size_t i );
    size_t vlebit( const char *&i );

    // medium level API, templates (in-place)

    template<typename container>
    static inline bool is_packed( const container &input ) {
        return is_packed( &input[0], input.size() );
    }

    template<typename container>
    static inline bool is_unpacked( const container &input ) {
        return !is_packed( &input[0], input.size() );
    }

    template<typename T>
    static inline unsigned guess_type_of( const T &input ) {
        return guess_type_of( &input[0], input.size() );
    }
    template<typename T>
    static inline unsigned type_of( const T &input ) {
        return type_of( &input[0], input.size() );
    }
    template<typename T>
    static inline const char *const name_of( const T &input ) {
        return name_of( type_of( input ) );
    }
    template<typename T>
    static inline const char *const version_of( const T &input ) {
        return version_of( type_of( input ) );
    }
    template<typename T>
    static inline const char *const ext_of( const T &input ) {
        return ext_of( type_of( input ) );
    }
    template<typename T>
    static inline size_t len( const T &input ) {
        return len( &input[0], input.size() );
    }
    template<typename T>
    static inline size_t zlen( const T &input ) {
        return zlen( &input[0], input.size() );
    }
    template<typename T>
    static inline const void *zptr( const T &input ) {
        return zptr( &input[0], input.size() );
    }

    template < class T1, class T2 >
    static inline bool unpack( T2 &output, const T1 &input ) {
        // sanity checks
        assert( sizeof(input.at(0)) == 1 && "size of input elements != 1" );
        assert( sizeof(output.at(0)) == 1 && "size of output elements != 1" );

        if( is_packed( input ) ) {
            // decapsulate
            unsigned Q = input[1] & 0x0F;
            const char *ptr = (const char *)&input[2];
            size_t size1 = vlebit(ptr);
            size_t size2 = vlebit(ptr);

            // decompress
            size1 += unc_payload(Q);
            output.resize( size1 );

            // note: output must be resized properly before calling this function!!
            if( unpack( Q, ptr, size2, &output[0], size1 ) ) {
                output.resize( size1 );
                return true;
            }
        }

        output = input;
        return false;
    }

    template < class T1, class T2 >
    static inline bool pack( unsigned q, T2 &output, const T1 &input ) {
        // sanity checks
        assert( sizeof(input.at(0)) == 1 && "size of input elements != 1" );
        assert( sizeof(output.at(0)) == 1 && "size of output elements != 1" );

        if( input.empty() ) {
            output = input;
            return true;
        }

        if( 1 /* is_unpacked( input ) */ ) {
            // resize to worst case
            size_t zlen = bound(q, input.size());
            output.resize( zlen );

            // compress
            if( pack( q, &input.at(0), input.size(), &output.at(0), zlen ) ) {
                // resize properly
                output.resize( zlen );

                // encapsulate
                std::string header = std::string() + char(0) + char(0x70 | (q & 0x0F)) + vlebit(input.size()) + vlebit(output.size());
                unsigned header_len = header.size();
                output.resize( zlen + header_len );
                memmove( &output[header_len], &output[0], zlen );
                memcpy( &output[0], &header[0], header_len );
                return true;
            }
        }

        output = input;
        return false;
    }

    // high level API, templates (copy)

    template < class T1 >
    static inline T1 unpack( const T1 &input ) {
        T1 output;
        unpack( output, input );
        return output;
    }


    template<typename container>
    static inline container pack( unsigned q, const container &input ) {
        container output;
        pack( q, output, input );
        return output;
    }

    // helpers

    static inline std::vector<unsigned> fast_encodings() {
        static std::vector<unsigned> all;
        if( all.empty() ) {
            all.push_back( NONE );
            all.push_back( LZ4F );
            all.push_back( SHOCO );
            all.push_back( MINIZ );
            all.push_back( LZIP );
            all.push_back( LZMA20 );
            all.push_back( LZMA25 );
            all.push_back( LZ4 );
            all.push_back( ZSTD );
            all.push_back( BSC );
            all.push_back( BROTLI9 );
            all.push_back( SHRINKER );
            all.push_back( CSC20 );
#if 0
            // for archival purposes
            all.push_back( BZIP2 );
            all.push_back( LZFX );
            all.push_back( LZHAM );
            all.push_back( LZP1 );
            all.push_back( FSE );
            all.push_back( BLOSC );
            all.push_back( YAPPY );
#endif
        }
        return all;
    }

    static inline std::vector<unsigned> encodings() {
        static std::vector<unsigned> all;
        if( all.empty() ) {
            all = fast_encodings();
            all.push_back( BROTLI11 );
            all.push_back( ZPAQ );
        }
        return all;
    }

#if BUNDLE_CXX11

    // measures for all given encodings
    template<typename T>
    struct measure {
        unsigned q = NONE;
        double ratio = 0;
        double enctime = 0;
        double dectime = 0;
        double memusage = 0;
        bool pass = 0;
        T packed /*, unpacked */;
        std::string str() const {
            std::stringstream ss;
            ss << ( pass ? "[ OK ] " : "[FAIL] ") << name_of(q) << ": ratio=" << ratio << "% enctime=" << int(enctime) << "us dectime=" << int(dectime) << "us (zlen=" << packed.size() << " bytes)";
            return ss.str();
        }
    };

    template< class T, bool do_enc = true, bool do_dec = true, bool do_verify = true >
    std::vector< measure<T> > measures( const T& original, const std::vector<unsigned> &use_encodings = encodings() ) {
        std::vector< measure<T> > results;

        for( auto encoding : use_encodings ) {
            //std::cout << name_of(encoding) << std::endl;
            results.push_back( measure<T>() );
            auto &r = results.back();
            r.q = encoding;
            r.pass = true;

            if( r.pass && do_enc ) {
#ifdef BUNDLE_USE_OMP_TIMER
                auto start = omp_get_wtime();
                r.packed = pack( encoding, original );
                auto end = omp_get_wtime();
                r.enctime = ( end - start ) * 1000000;
#else
                auto start = std::chrono::steady_clock::now();
                r.packed = pack( encoding, original );
                auto end = std::chrono::steady_clock::now();
                r.enctime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif
                r.ratio = 100 - 100 * ( double( r.packed.size() ) / original.size() );
                if( encoding != NONE )
                r.pass = r.pass && is_packed(r.packed);
            }

            if( r.pass && do_dec ) {
#ifdef BUNDLE_USE_OMP_TIMER
                auto start = omp_get_wtime();
                std::string unpacked = unpack( r.packed );
                auto end = omp_get_wtime();
                r.dectime = ( end - start ) * 1000000;
#else
                auto start = std::chrono::steady_clock::now();
                std::string unpacked = unpack( r.packed );
                auto end = std::chrono::steady_clock::now();
                r.dectime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif
                if( encoding != NONE )
                r.pass = r.pass && (do_verify ? original == unpacked : r.pass);
            }

            if( !r.pass ) {
                r.ratio = r.enctime = r.dectime = 0;
            }
        }

        return results;
    }

    // find best choice for given data
    template< class T >
    unsigned find_slot_for_smallest_compressor( const std::vector< measure<T> > &measures, double no_compression_treshold = NO_COMPRESSION_TRESHOLD / 1.0 ) {
        unsigned q = ~0;
        double ratio = -1;
        for( auto end = measures.size(), it = end - end; it < end; ++it ) {
            auto &r = measures[ it ];
            if( r.pass && r.ratio > ratio && r.ratio >= no_compression_treshold ) {
                ratio = r.ratio;
                q = it;
            }
        }
        return q;
    }

    template< class T >
    unsigned find_slot_for_fastest_compressor( const std::vector< measure<T> > &measures ) {
        unsigned q = ~0;
        double enctime = std::numeric_limits<double>::max();
        for( auto end = measures.size(), it = end - end; it < end; ++it ) {
            auto &r = measures[ it ];
            if( r.pass && r.enctime < enctime && r.q != NONE ) {
                enctime = r.enctime;
                q = it;
            }
        }
        return q;
    }

    template< class T >
    unsigned find_slot_for_fastest_decompressor( const std::vector< measure<T> > &measures ) {
        unsigned q = ~0;
        double dectime = std::numeric_limits<double>::max();
        for( auto end = measures.size(), it = end - end; it < end; ++it ) {
            auto &r = measures[ it ];
            if( r.pass && r.dectime < dectime && r.q != NONE ) {
                dectime = r.dectime;
                q = it;
            }
        }
        return q;
    }

    template< class T >
    unsigned find_smallest_compressor( const std::vector< measure<T> > &measures, double no_compression_treshold = NO_COMPRESSION_TRESHOLD / 1.0 ) {
        auto slot = find_slot_for_smallest_compressor( measures, no_compression_treshold );
        return slot == ~0 ? NONE : type_of( measures[slot].packed );
    }

    template< class T >
    unsigned find_fastest_compressor( const std::vector< measure<T> > &measures ) {
        auto slot = find_slot_for_fastest_compressor( measures );
        return slot == ~0 ? NONE : type_of( measures[slot].packed );
    }

    template< class T >
    unsigned find_fastest_decompressor( const std::vector< measure<T> > &measures ) {
        auto slot = find_slot_for_fastest_decompressor( measures );
        return slot == ~0 ? NONE : type_of( measures[slot].packed );
    }

#endif
}

namespace bundle
{
    class string : public std::string
    {
        public:

        string() : std::string()
        {}

        template< typename T >
        explicit string( const T &t ) : std::string() {
            operator=(t);
        }

        template< typename T >
        string &operator=( const T &t ) {
            std::stringstream ss;
            ss.precision(20);
            return ss << t ? (this->assign( ss.str() ), *this) : *this;
        }

        template< typename T >
        T as() const {
            T t;
            std::stringstream ss( *this );
            return ss >> t ? t : T();
        }

        template< typename T0 >
        string( const std::string &_fmt, const T0 &t0 ) : std::string() {
            std::string t[] = { string(), string(t0) };
            for( const char *fmt = _fmt.c_str(); *fmt; ++fmt ) {
                /**/ if( *fmt == '\1' ) t[0] += t[1];
                else                    t[0] += *fmt;
            }
            this->assign( t[0] );
        }

        template< typename T0, typename T1 >
        string( const std::string &_fmt, const T0 &t0, const T1 &t1 ) : std::string() {
            std::string t[] = { string(), string(t0), string(t1) };
            for( const char *fmt = _fmt.c_str(); *fmt; ++fmt ) {
                /**/ if( *fmt == '\1' ) t[0] += t[1];
                else if( *fmt == '\2' ) t[0] += t[2];
                else                    t[0] += *fmt;
            }
            this->assign( t[0] );
        }
    };

    struct file : public std::map< std::string, bundle::string >
    {
        bool has( const std::string &property ) const;

        template <typename T>
        T get( const std::string &property ) const {
            return (*this->find( property )).second.as<T>();
        }
    };

    class archive : public std::vector< bundle::file >
    {
        public:

        enum container { ZIP } type;

        explicit
        archive( const container &type = ZIP ) : type(type)
        {}

        // binary serialization
        bool bin( const std::string &binary );
        std::string bin( unsigned q = EXTRA ) const;

        // debug

        std::string toc() const {
            // @todo: add offset in file
            std::string ret;
            for( const_iterator it = this->begin(), end = this->end(); it != end; ++it ) {
                const bundle::file &file = *it;
                ret += "\t{\n";
                for( bundle::file::const_iterator it = file.begin(), end = file.end(); it != end; ++it ) {
                    const std::pair< std::string, bundle::string > &property = *it;
                    if( property.first == "data" )
                        ret += "\t\t\"data\":\"... (" + string( property.second.size() ) + " bytes)\",\n";
                    else
                        ret += "\t\t\"" + property.first + "\":\"" + property.second + "\",\n";
                }
                ret += "\t},\n";
            }
            if( ret.size() >= 2 ) { ret[ ret.size() - 2 ] = '\n', ret = ret.substr( 0, ret.size() - 1 ); }
            return std::string() + "[\n" + ret + "]\n";
        }
    };
}

#endif

