giant
=====

- Giant is a tiny C++11 library to handle little/big endianness.
- Giant is safe. Converts POD types only. Non-POD types are returned as-is.
- Giant is tiny. Header-only library.
- Giant is cross-platform. Compiles under MSVC/GCC. Works on Windows/Linux.
- Giant has no dependencies.
- Giant is BOOST licensed.

### API
- `const bool giant::is_little`
- `const bool giant::is_big`
- `T giant::swap(T)`
- `T giant::letobe(T)`
- `T giant::betole(T)`
- `T giant::htobe(T)`
- `T giant::htole(T)`
- `T giant::betoh(T)`
- `T giant::letoh(T)`

### Sample
```c++
~giant>
~giant> cat sample.cc

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

~giant>
~giant> g++ sample.cc && ./a.out
Target is little endian
big to host:    f0debc9a78563412
little to host: 123456789abcdef0
host to big:    f0debc9a78563412
host to little: 123456789abcdef0
~giant>
```
