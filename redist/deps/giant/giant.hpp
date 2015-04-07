/*
 * A tiny library to handle little/big endianness.

 * Copyright (c) 2011-2014 Mario 'rlyeh' Rodriguez
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See license copy at http://www.boost.org/LICENSE_1_0.txt)

 * To do:
 * - Get autodetect out of template, so it does not get instanced once per type.

 * - rlyeh // listening to White Orange ~~ Where
 */

#pragma once

#include <cassert>
#include <algorithm>
#include <type_traits>

#if defined (__GLIBC__)
#   include <endian.h>
#endif

namespace giant
{
    
#if defined(_LITTLE_ENDIAN) \
    || ( defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && BYTE_ORDER == LITTLE_ENDIAN ) \
    || ( defined(_BYTE_ORDER) && defined(_LITTLE_ENDIAN) && _BYTE_ORDER == _LITTLE_ENDIAN ) \
    || ( defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && __BYTE_ORDER == __LITTLE_ENDIAN ) \
    || defined(__i386__) || defined(__alpha__) \
    || defined(__ia64) || defined(__ia64__) \
    || defined(_M_IX86) || defined(_M_IA64) \
    || defined(_M_ALPHA) || defined(__amd64) \
    || defined(__amd64__) || defined(_M_AMD64) \
    || defined(__x86_64) || defined(__x86_64__) \
    || defined(_M_X64)
    enum { xinu_type = 0, unix_type = 1, nuxi_type = 2, type = xinu_type, is_little = 1, is_big = 0 };
#elif defined(_BIG_ENDIAN) \
    || ( defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN ) \
    || ( defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && _BYTE_ORDER == _BIG_ENDIAN ) \
    || ( defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && __BYTE_ORDER == __BIG_ENDIAN ) \
    || defined(__sparc) || defined(__sparc__) \
    || defined(_POWER) || defined(__powerpc__) \
    || defined(__ppc__) || defined(__hpux) \
    || defined(_MIPSEB) || defined(_POWER) \
    || defined(__s390__)
    enum { xinu_type = 0, unix_type = 1, nuxi_type = 2, type = unix_type, is_little = 0, is_big = 1 };
#else
#   error <giant/giant.hpp> says: Middle endian/NUXI order is not supported
    enum { xinu_type = 0, unix_type = 1, nuxi_type = 2, type = nuxi_type, is_little = 0, is_big = 0 };
#endif

    template<typename T>
    T swap( T out )
    {
        static union autodetect {
            int word;
            char byte[ sizeof(int) ];
            autodetect() : word(1) {
                assert(( "<giant/giant.hpp> says: wrong endianness detected!", (!byte[0] && is_big) || (byte[0] && is_little) ));
            }
        } _;

        if( !std::is_pod<T>::value ) {
            return out;
        }

        char *ptr;

        switch( sizeof( T ) ) {
            case 0:
            case 1:
                break;
            case 2:
                ptr = reinterpret_cast<char *>(&out);
                std::swap( ptr[0], ptr[1] );
                break;
            case 4:
                ptr = reinterpret_cast<char *>(&out);
                std::swap( ptr[0], ptr[3] );
                std::swap( ptr[1], ptr[2] );
                break;
            case 8:
                ptr = reinterpret_cast<char *>(&out);
                std::swap( ptr[0], ptr[7] );
                std::swap( ptr[1], ptr[6] );
                std::swap( ptr[2], ptr[5] );
                std::swap( ptr[3], ptr[4] );
                break;
            case 16:
                ptr = reinterpret_cast<char *>(&out);
                std::swap( ptr[0], ptr[15] );
                std::swap( ptr[1], ptr[14] );
                std::swap( ptr[2], ptr[13] );
                std::swap( ptr[3], ptr[12] );
                std::swap( ptr[4], ptr[11] );
                std::swap( ptr[5], ptr[10] );
                std::swap( ptr[6], ptr[9] );
                std::swap( ptr[7], ptr[8] );
                break;
            default:
                assert( !"<giant/giant.hpp> says: POD type bigger than 256 bits (?)" );
                break;
        }

        return out;
    }

    template<typename T>
    T letobe( const T &in ) {
        return swap( in );
    }
    template<typename T>
    T betole( const T &in ) {
        return swap( in );
    }

    template<typename T>
    T letoh( const T &in ) {
        return type == xinu_type ? in : swap( in );
    }
    template<typename T>
    T htole( const T &in ) {
        return type == xinu_type ? in : swap( in );
    }

    template<typename T>
    T betoh( const T &in ) {
        return type == unix_type ? in : swap( in );
    }
    template<typename T>
    T htobe( const T &in ) {
        return type == unix_type ? in : swap( in );
    }
}
