
#line 3 "bundle.hpp"
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
#include <limits>

#if BUNDLE_CXX11
#include <chrono>
#endif

#ifdef BUNDLE_USE_OMP_TIMER
#include <omp.h>
#endif

namespace bundle
{
	// per lib
	enum { UNDEFINED, SHOCO, LZ4, MINIZ, LZIP, LZMASDK, ZPAQ, LZ4HC}; /* archival: BZIP2, LZFX, LZHAM, LZP1, FSE, BLOSC */
	// per family
	enum { NONE = UNDEFINED, ASCII = SHOCO, LZ77 = LZ4, DEFLATE = MINIZ, LZMA = LZMASDK, CM = ZPAQ }; /* archival: BWT = BZIP2 */
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
	size_t unc_payload( unsigned q );
	bool pack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );
	bool unpack( unsigned q, const void *in, size_t len, void *out, size_t &zlen );

	std::string vlebit( size_t i );
	size_t vlebit( const char *&i );

	// high level API

	unsigned type_of( const std::string &self );
	std::string name_of( const std::string &self );
	std::string version_of( const std::string &self );
	std::string ext_of( const std::string &self );
	size_t length( const std::string &self );
	size_t zlength( const std::string &self );
	void *zptr( const std::string &self );

	// high level API, templates

	template<typename container>
	static inline bool is_packed( const container &input ) {
		return input.size() >= 2 && 0 == input[0] && input[1] >= 0x70 && input[1] <= 0x7F;
	}

	template<typename container>
	static inline bool is_unpacked( const container &input ) {
		return !is_packed(input);
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

	template < class T1 >
	static inline T1 unpack( const T1 &input ) {
		T1 output;
		unpack( output, input );
		return output;
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

	template<typename container>
	static inline container pack( unsigned q, const container &input ) {
		container output;
		pack( q, output, input );
		return output;
	}

	static inline std::vector<unsigned> encodings() {
		static std::vector<unsigned> all;
		if( all.empty() ) {
			all.push_back( NONE );
			all.push_back( LZ4 );
			all.push_back( SHOCO );
			all.push_back( MINIZ );
			all.push_back( LZIP );
			all.push_back( LZMASDK );
			all.push_back( ZPAQ );
			all.push_back( LZ4HC );
#if 0
			// for archival purposes
			all.push_back( BZIP2 );
			all.push_back( LZFX );
			all.push_back( BSC );
			all.push_back( LZHAM );
			all.push_back( LZP1 );
			all.push_back( FSE );
			all.push_back( BLOSC );
#endif
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
		T packed, unpacked;
		std::string str() const {
			std::stringstream ss;
			ss << ( pass ? "[ OK ] " : "[FAIL] ") << name_of(q) << ": ratio=" << ratio << "% enctime=" << int(enctime) << "us dectime=" << int(dectime) << "us";
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
				r.unpacked = unpack( r.packed );
				auto end = omp_get_wtime();
				r.dectime = ( end - start ) * 1000000;
#else
				auto start = std::chrono::steady_clock::now();
				r.unpacked = unpack( r.packed );
				auto end = std::chrono::steady_clock::now();
				r.dectime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif
				if( encoding != NONE )
				r.pass = r.pass && (do_verify ? original == r.unpacked : r.pass);
			}

			if( !r.pass ) {
				r.ratio = r.enctime = r.dectime = 0;
			}
		}

		return results;
	}

	// find best choice for given data
	template< class T >
	unsigned find_smallest_compressor( const std::vector< measure<T> > &measures ) {
		unsigned q = NONE;
		double ratio = -1;
		for( auto &r : measures ) {
			if( r.pass && r.ratio > ratio && r.ratio >= (100 - NO_COMPRESSION_TRESHOLD / 100.0) ) {
				ratio = r.ratio;
				q = type_of(r.packed);
			}
		}
		return q;
	}

	template< class T >
	unsigned find_fastest_compressor( const std::vector< measure<T> > &measures ) {
		unsigned q = NONE;
		double enctime = std::numeric_limits<double>::max();
		for( auto &r : measures ) {
			if( r.pass && r.enctime < enctime && r.q != NONE ) {
				enctime = r.enctime;
				q = type_of(r.packed);
			}
		}
		return q;
	}

	template< class T >
	unsigned find_fastest_decompressor( const std::vector< measure<T> > &measures ) {
		unsigned q = NONE;
		double dectime = std::numeric_limits<double>::max();
		for( auto &r : measures ) {
			if( r.pass && r.dectime < dectime && r.q != NONE ) {
				dectime = r.dectime;
				q = type_of(r.packed);
			}
		}
		return q;
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

