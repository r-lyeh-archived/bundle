#include <iostream>
#include <stdint.h>
#include <string>

#include "giant.hpp"

int main() {
    // sample

    uint16_t u16 = 0x1234;
    uint32_t u32 = 0x12345678;
    uint64_t u64 = 0x123456789ABCDEF0;

    uint16_t u16sw = giant::swap( u16 );
    uint32_t u32sw = giant::swap( u32 );
    uint64_t u64sw = giant::swap( u64 );

    std::cout << std::hex;
    std::cout << "Target is " << ( giant::is_little ? "little endian" : "big endian") << std::endl;

    // checks
    assert( giant::swap(u16sw) == u16 );
    assert( giant::swap(u32sw) == u32 );
    assert( giant::swap(u64sw) == u64 );

    if( giant::is_little )
    assert( giant::htole('unix') == 'unix' && giant::htobe('unix') == 'xinu' );
    else
    assert( giant::htobe('unix') == 'unix' && giant::htole('unix') == 'xinu' );

    // show conversions
    std::cout << "big to host:    " << giant::betoh(u64)    << std::endl;
    std::cout << "little to host: " << giant::letoh(u64) << std::endl;
    std::cout << "host to big:    " << giant::htobe(u64)    << std::endl;
    std::cout << "host to little: " << giant::htole(u64) << std::endl;

    // non-pod types wont get swapped neither converted
    assert( giant::swap( std::string("hello world") ) == "hello world" );
    assert( giant::letobe( std::string("hello world") ) == "hello world" );

    return 0;
}
