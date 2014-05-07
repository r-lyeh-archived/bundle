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
