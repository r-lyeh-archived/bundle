/*
 * Simple compression interface.
 * Copyright (c) 2013, 2014, Mario 'rlyeh' Rodriguez

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 * - rlyeh ~~ listening to Boris / Missing Pieces
 */

#ifndef BUNDLE_HPP
#define BUNDLE_HPP

#if ( defined(_MSC_VER) && _MSC_VER >= 1800 ) || __cplusplus >= 201103L
#define BUNDLE_CXX11 1
#else
#define BUNDLE_CXX11 0
#endif

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if BUNDLE_CXX11
#include <chrono>
#endif

namespace bundle
{
    // per lib
    enum { UNDEFINED, SHOCO, LZ4, MINIZ, LZIP, LZMASDK, ZPAQ };
    // per family
    enum { NONE = UNDEFINED, ASCII = SHOCO, LZ77 = LZ4, DEFLATE = MINIZ, LZMA = LZMASDK, CM = ZPAQ };
    // per context
    enum { UNCOMPRESSED = NONE, ENTROPY = ASCII, FAST = LZ77, DEFAULT = DEFLATE, EXTRA = LZMA, UBER = CM };

    // dont compress if compression ratio is below 5%
    enum { NO_COMPRESSION_TRESHOLD = 5 };

    // low level API

    unsigned type_of( const void *mem, size_t size );
    const char *const name_of( unsigned q );
    const char *const version_of( unsigned q );
    const char *const ext_of( unsigned q );
    size_t bound( unsigned q, size_t len );
    bool pack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );
    bool unpack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );

    // high level API

    unsigned type_of( const std::string &self );
    std::string name_of( const std::string &self );
    std::string version_of( const std::string &self );
    std::string ext_of( const std::string &self );
    size_t length( const std::string &self );
    size_t zlength( const std::string &self );
    void *zptr( const std::string &self );

    // high level API, templates

    template < class T1, class T2 >
    static inline bool pack( unsigned q, T2 &buffer_out, const T1 &buffer_in ) {
        // sanity checks
        assert( sizeof(buffer_in.at(0)) == 1 && "size of input elements != 1" );
        assert( sizeof(buffer_out.at(0)) == 1 && "size of output elements != 1" );

        // resize to worst case
        size_t zlen = bound(q, buffer_in.size());
        buffer_out.resize( zlen );

        // compress
        bool result = pack( q, &buffer_in.at(0), buffer_in.size(), &buffer_out.at(0), zlen );

        // resize properly
        return result ? ( buffer_out.resize( zlen ), true ) : ( buffer_out = T2(), false );
    }

    template < class T1, class T2 >
    static inline bool unpack( unsigned q, T2 &buffer_out, const T1 &buffer_in ) {
        // sanity checks
        assert( sizeof(buffer_in.at(0)) == 1 && "size of input elements != 1" );
        assert( sizeof(buffer_out.at(0)) == 1 && "size of output elements != 1" );

        // note: buffer_out must be resized properly before calling this function!!
        size_t zlen = buffer_out.size();
        return unpack( q, &buffer_in.at(0), buffer_in.size(), &buffer_out.at(0), zlen );
    }

    std::string vlebit( size_t i );
    size_t vlebit( const char *&i );

    template<typename container>
    static inline bool is_packed( const container &input ) {
        return input.size() >= 2 && 0 == input[0] && input[1] >= 0x70 && input[1] <= 0x7F;
    }

    template<typename container>
    static inline bool is_unpacked( const container &input ) {
        return !is_packed(input);
    }

    template<typename container>
    static inline container pack( unsigned q, const container &input ) {
        if( is_packed( input ) )
            return input;

        // sanity checks
        assert( sizeof(input.at(0)) == 1 && "size of input elements != 1" );

        container output( bound( q, input.size() ), '\0' );

        // compress
        size_t len = output.size();
        if( !pack( q, &input[0], input.size(), &output[0], len ) )
            return input;
        output.resize( len );

        // encapsulate
        output = std::string() + char(0) + char(0x70 | (q & 0x0F)) + vlebit(input.size()) + vlebit(output.size()) + output;
        return output;
    }

    template<typename container>
    static inline container unpack( const container &input ) {
        if( is_unpacked( input ) )
            return input;

        // sanity checks
        assert( sizeof(input.at(0)) == 1 && "size of input elements != 1" );

        // decapsulate
        unsigned Q = input[1] & 0x0F;
        const char *ptr = (const char *)&input[2];
        size_t size1 = vlebit(ptr);
        size_t size2 = vlebit(ptr);

        container output( size1, '\0' );

        // decompress
        size_t len = output.size();
        if( !unpack( Q, ptr, size2, &output[0], len ) )
            return input;

        return output;
    }

    static inline std::vector<unsigned> encodings() {
        static std::vector<unsigned> all;
        if( all.empty() ) {
            all.push_back( LZ4 );
            all.push_back( SHOCO );
            all.push_back( MINIZ );
            all.push_back( LZIP );
            all.push_back( LZMASDK );
            all.push_back( ZPAQ );
            all.push_back( NONE );
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
        T zipped, unzipped;
        std::string str() const {
            std::stringstream ss;
            ss << ( pass ? "[ OK ] " : "[FAIL] ") << name_of(q) << ": ratio=" << ratio << "% enctime=" << enctime << "ms dectime=" << dectime << " ms";
            return ss.str();
        }
    };

    template< class T, bool do_enc = true, bool do_dec = true, bool do_verify = true >
    std::vector< measure<T> > measures( const T& original, const std::vector<unsigned> &use_encodings = encodings() ) {
        std::vector< measure<T> > results;

        for( auto encoding : use_encodings ) {
            results.push_back( measure<T>() );
            auto &r = results.back();
            r.q = encoding;
            r.pass = true;

            if( do_enc ) {
                auto begin = std::chrono::high_resolution_clock::now();
                r.zipped = pack( encoding, original );
                auto end = std::chrono::high_resolution_clock::now();
                r.enctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                r.ratio = 100 - 100 * ( double( r.zipped.size() ) / original.size() );
                r.pass = r.pass && (encoding != NONE ? is_packed(r.zipped) : 1);
            }

            if( do_dec ) {
                auto begin = std::chrono::high_resolution_clock::now();
                r.unzipped = unpack( r.zipped );
                auto end = std::chrono::high_resolution_clock::now();
                r.dectime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                r.pass = r.pass && (is_unpacked(r.unzipped));
            }

            if( do_verify ) {
                r.pass = r.pass && (original == r.unzipped);
            }
        }

        return results;
    }

    // find best choice for given data
    template< class T >
    measure<T> find_smallest_compressor( const std::vector< measure<T> > &measures ) {
        const measure<T> *q = 0;
        double ratio = 0;

        for( auto &r : measures ) {
            if( r.pass && r.ratio > ratio && r.ratio >= (100 - NO_COMPRESSION_TRESHOLD / 100.0) ) {
                ratio = r.ratio;
                q = &r;
            }
        }

        return q ? *q : measure<T>();
    }

    template< class T >
    measure<T> find_fastest_compressor( const std::vector< measure<T> > &measures ) {
        const measure<T> *q = 0;
        double enctime = 9999999;

        for( auto &r : measures ) {
            if( r.pass && r.enctime < enctime && r.q != NONE ) {
                enctime = r.enctime;
                q = &r;
            }
        }

        return q ? *q : measure<T>();
    }

    template< class T >
    measure<T> find_fastest_decompressor( const std::vector< measure<T> > &measures ) {
        const measure<T> *q = 0;
        double dectime = 9999999;

        for( auto &r : measures ) {
            if( r.pass && r.dectime < dectime && r.q != NONE ) {
                dectime = r.dectime;
                q = &r;
            }
        }

        return q ? *q : measure<T>();
    }

#endif
}

namespace bundle
{
    struct paktype
    {
        enum enumeration { ZIP };
    };

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

    struct pakfile : public std::map< std::string, bundle::string >
    {
        bool has( const std::string &property ) const;

        template <typename T>
        T get( const std::string &property ) const {
            return (*this->find( property )).second.as<T>();
        }
    };

    class pak : public std::vector< pakfile >
    {
        public:

        const paktype::enumeration type;

        explicit
        pak( const paktype::enumeration &etype = paktype::ZIP ) : type(etype)
        {}

        // binary serialization

        bool bin( const std::string &bin_import ); //const
        std::string bin( unsigned q = EXTRA ) const;

        // debug

        std::string toc() const {
            // @todo: add offset in file
            std::string ret;
            for( const_iterator it = this->begin(), end = this->end(); it != end; ++it ) {
                const pakfile &file = *it;
                ret += "\t{\n";
                for( pakfile::const_iterator it = file.begin(), end = file.end(); it != end; ++it ) {
                    const std::pair< std::string, bundle::string > &property = *it;
                    if( property.first == "content" )
                        ret += "\t\t\"size\":\"" + string( property.second.size() ) + "\",\n";
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
