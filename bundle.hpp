/** this is an amalgamated file. do not edit.
 */


#line 1 "bundle.hpp"
// simple compression interface
// - rlyeh. mit licensed

#pragma once
#include <cstdio>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <sstream>

namespace bundle
{
	// per lib
	enum { UNDEFINED, SHOCO, LZ4, MINIZ, LZLIB };
	// per family
	enum { NONE = UNDEFINED, ENTROPY = SHOCO, LZ77 = LZ4, DEFLATE = MINIZ, LZMA = LZLIB };
	// per context
	enum { UNCOMPRESSED = NONE, ASCII = ENTROPY, FAST = LZ77, DEFAULT = DEFLATE, EXTRA = LZMA };

	// dont compress if compression ratio is below 5%
	enum { NO_COMPRESSION_TRESHOLD = 5 };

	// high level API

	bool is_z( const std::string &self );
	bool is_unz( const std::string &self );

	std::string z( const std::string &self, unsigned q = DEFAULT );
	std::string unz( const std::string &self, unsigned q = DEFAULT );

	// low level API

	  bool pack( unsigned q, const char *in, size_t len, char *out, size_t &zlen );
	  bool unpack( unsigned q, const char *in, size_t len, char *out, size_t &zlen );
	size_t bound( unsigned q, size_t len );
	const char *const nameof( unsigned q );
	const char *const version( unsigned q );
	const char *const extof( unsigned q );
	unsigned typeof( const void *mem, size_t size );

	//

	template < class T1, class T2 >
	static inline bool pack( T2 &buffer_out, const T1 &buffer_in, unsigned q = DEFAULT ) {
		// sanity checks
		static_assert( sizeof(*std::begin(T1())) == 1, "" );
		static_assert( sizeof(*std::begin(T2())) == 1, "" );

		// resize to worst case
		size_t zlen = bound(q, buffer_in.size());
		buffer_out.resize( zlen );

		// compress
		bool result = pack( q, (const char *)&buffer_in.at(0), buffer_in.size(), (char *)&buffer_out.at(0), zlen );

		// resize properly
		return result ? ( buffer_out.resize( zlen ), true ) : ( buffer_out = T2(), false );
	}

	template < class T1, class T2 >
	static inline bool unpack( T2 &buffer_out, const T1 &buffer_in, unsigned q = DEFAULT ) {
		// sanity checks
		static_assert( sizeof(*std::begin(T1())) == 1, "" );
		static_assert( sizeof(*std::begin(T2())) == 1, "" );

		// note: buffer_out must be resized properly before calling this function!!
		size_t zlen = buffer_out.size();
		return unpack( q, (const char *)&buffer_in.at(0), buffer_in.size(), (char *)&buffer_out.at(0), zlen );
	}

	static inline std::vector<unsigned> encodings() {
		static std::vector<unsigned> all { LZ4, SHOCO, MINIZ, LZLIB, NONE };
		return all;
	}

	// measures for all given encodings
	struct measure {
		unsigned q = NONE;
		double ratio = 0;
		double enctime = 0;
		double dectime = 0;
		double memusage = 0;
		bool pass = 0;
		std::string str() const {
			std::stringstream ss;
			ss << ( pass ? "[ OK ] " : "[FAIL] ") << nameof(q) << ": ratio=" << ratio << "% enctime=" << enctime << "ms dectime=" << dectime << " ms";
			return ss.str();
		}
	};

	template< class T, bool do_enc = true, bool do_dec = true, bool do_verify = true >
	std::vector< measure > measures( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
		std::vector< measure > results;
		std::string zipped, unzipped;

		for( auto scheme : encodings ) {
			results.push_back( measure() );
			auto &r = results.back();
			r.q = scheme;

			if( do_enc ) {
				auto begin = std::chrono::high_resolution_clock::now();
				zipped = z(original, scheme);
				auto end = std::chrono::high_resolution_clock::now();
				r.enctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
				r.ratio = 100 - 100 * ( double( zipped.size() ) / original.size() );
			}

			if( do_dec ) {
				auto begin = std::chrono::high_resolution_clock::now();
				unzipped = unz(zipped, scheme);
				auto end = std::chrono::high_resolution_clock::now();
				r.dectime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
				r.pass = ( original == unzipped );
			}

			r.pass = ( do_verify && do_enc && do_dec ? original == unzipped : true );
		}

		return results;
	}

	// find best choice for given data
	template< class T >
	unsigned find_smallest_compressor( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double ratio = 0;

		auto results = measures< true, false, false >( original, encodings );
		for( auto &r : results ) {
			if( r.pass && r.ratio > ratio && r.ratio >= (100 - NO_COMPRESSION_TRESHOLD / 100.0) ) {
				ratio = r.ratio;
				q = r.q;
			}
		}

		return q;
	}

	template< class T >
	unsigned find_fastest_compressor( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double enctime = 9999999;

		auto results = measures< true, false, false >( original, encodings );
		for( auto &r : results ) {
			if( r.pass && r.enctime < enctime ) {
				enctime = r.enctime;
				q = r.q;
			}
		}

		return q;
	}

	template< class T >
	unsigned find_fastest_decompressor( const T& original, const std::vector<unsigned> &encodings = encodings() ) {
		unsigned q = bundle::NONE;
		double dectime = 9999999;

		auto results = measures< false, true, false >( original, encodings );
		for( auto &r : results ) {
			if( r.pass && r.dectime < dectime ) {
				dectime = r.dectime;
				q = r.q;
			}
		}

		return q;
	}
}


#line 1 "pak.hpp"
// simple compression interface
// - rlyeh. mit licensed

#pragma once
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

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

		template< typename T0, typename T1 = std::string >
		string( const std::string &_fmt, const T0 &t0, const T1 &t1 = T1() ) : std::string() {
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
		std::string bin() const;

		// debug

		std::string toc() const {
			std::string ret;
			for( const auto &file : *this ) {
				ret += "\t{\n";
				for( const auto &property : file ) {
					if( property.first == "content" )
						ret += "\t\t\"size\":\"" + string( property.second.size() ) + "\",\n";
					else
						ret += "\t\t\"" + property.first + "\":\"" + property.second + "\",\n";
				}
				ret += "\t},\n";
			}
			if( ret.size() >= 2 ) { ret[ ret.size() - 2 ] = '\n', ret.pop_back(); }
			return std::string() + "[\n" + ret + "]\n";
		}
	};
}

