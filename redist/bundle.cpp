// simple compression interface
// - rlyeh. mit licensed

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

#include \
"bundle.hpp"


namespace {
  /* Compresses 'size' bytes from 'data'. Returns the address of a
     malloc'd buffer containing the compressed data and its size in
     '*out_sizep'.
     In case of error, returns 0 and does not modify '*out_sizep'.
  */
  uint8_t * lzip_compress( const uint8_t * const data, const int size,
                        uint8_t * const new_data, int * const out_sizep )
    {
    struct LZ_Encoder * encoder;
  //uint8_t * new_data;
    const int match_len_limit = 36;
    const unsigned long long member_size = INT64_MAX;
    int delta_size, new_data_size;
    int new_pos = 0;
    int written = 0;
    bool error = false;
    int dict_size = 8 << 20;              /* 8 MiB */

    if( dict_size > size ) dict_size = size;      /* saves memory */
    if( dict_size < LZ_min_dictionary_size() )
      dict_size = LZ_min_dictionary_size();
    encoder = LZ_compress_open( dict_size, match_len_limit, member_size );
    if( !encoder || LZ_compress_errno( encoder ) != LZ_ok )
      { LZ_compress_close( encoder ); return 0; }

    delta_size = (size < 256) ? 64 : size / 4;        /* size may be zero */
  //  new_data_size = delta_size;               /* initial size */
    new_data_size = int(*out_sizep);

  //  new_data = (uint8_t *)malloc( new_data_size );
    if( !new_data )
      { LZ_compress_close( encoder ); return 0; }

    while( true )
      {
      int rd;
      if( LZ_compress_write_size( encoder ) > 0 )
        {
        if( written < size )
          {
          const int wr = LZ_compress_write( encoder, data + written,
                                            size - written );
          if( wr < 0 ) { error = true; break; }
          written += wr;
          }
        if( written >= size ) LZ_compress_finish( encoder );
        }
      rd = LZ_compress_read( encoder, new_data + new_pos,
                             new_data_size - new_pos );
      if( rd < 0 ) { error = true; break; }
      new_pos += rd;
      if( LZ_compress_finished( encoder ) == 1 ) break;
      if( new_pos >= new_data_size )
        {
  #if 1
          error = true; break;
  #else
        uint8_t * const tmp =
          (uint8_t *)realloc( new_data, new_data_size + delta_size );
        if( !tmp ) { error = true; break; }
        new_data = tmp;
        new_data_size += delta_size;
  #endif
        }
      }

    if( LZ_compress_close( encoder ) < 0 ) error = true;
    if( error ) { /*free( new_data );*/ return 0; }
    *out_sizep = new_pos;
    return new_data;
    }


  /* Decompresses 'size' bytes from 'data'. Returns the address of a
     malloc'd buffer containing the decompressed data and its size in
     '*out_sizep'.
     In case of error, returns 0 and does not modify '*out_sizep'.
  */
  uint8_t * lzip_decompress( const uint8_t * const data, const int size,
                          uint8_t * const new_data, int * const out_sizep )
    {
    struct LZ_Decoder * const decoder = LZ_decompress_open();
  //uint8_t * new_data;
    const int delta_size = size;          /* size must be > zero */
  //  int new_data_size = delta_size;       /* initial size */
    int new_data_size = int(*out_sizep);
    int new_pos = 0;
    int written = 0;
    bool error = false;
    if( !decoder || LZ_decompress_errno( decoder ) != LZ_ok )
      { LZ_decompress_close( decoder ); return 0; }

    //new_data = (uint8_t *)malloc( new_data_size );
    if( !new_data )
      { LZ_decompress_close( decoder ); return 0; }

    while( true )
      {
      int rd;
      if( LZ_decompress_write_size( decoder ) > 0 )
        {
        if( written < size )
          {
          const int wr = LZ_decompress_write( decoder, data + written,
                                              size - written );
          if( wr < 0 ) { error = true; break; }
          written += wr;
          }
        if( written >= size ) LZ_decompress_finish( decoder );
        }
      rd = LZ_decompress_read( decoder, new_data + new_pos,
                               new_data_size - new_pos );
      if( rd < 0 ) { error = true; break; }
      new_pos += rd;
      if( LZ_decompress_finished( decoder ) == 1 ) break;
      if( new_pos >= new_data_size )
        {
  #if 1
          error = true; break;
  #else
        uint8_t * const tmp =
          (uint8_t *)realloc( new_data, new_data_size + delta_size );
        if( !tmp ) { error = true; break; }
        new_data = tmp;
        new_data_size += delta_size;
  #endif
        }
      }

    if( LZ_decompress_close( decoder ) < 0 ) error = true;
    if( error ) { /*free( new_data );*/ return 0; }
    *out_sizep = new_pos;
    return new_data;
    }
}


namespace bundle {

    enum { verbose = false };

    std::string hexdump( const std::string &str );

    namespace
    {
        void shout( unsigned q, const char *context, size_t from, size_t to ) {
            if( verbose ) {
                std::cout << context << " q:" << q << ",from:" << from << ",to:" << to << std::endl;
            }
        }
    }

    const char *const nameof( unsigned q ) {
        switch( q ) {
            break; default : return "NONE";
            break; case LZ4: return "LZ4";
            break; case MINIZ: return "MINIZ";
            break; case SHOCO: return "SHOCO";
            break; case LZLIB: return "LZLIB";
            /* for archival reasons: */
            // break; case LZHAM: return "LZHAM";
        }
    }

    const char *const version( unsigned q ) {
        switch( q ) {
            break; default : return "0";
        }
    }

    const char *const extof( unsigned q ) {
        switch( q ) {
            break; default : return "unc";
            break; case LZ4: return "lz4";
            break; case MINIZ: return "miniz";
            break; case SHOCO: return "shoco";
            break; case LZLIB: return "lz";
            /* for archival reasons: */
            // break; case LZHAM: return "lzham";
        }
    }

    unsigned typeof( const void *ptr, size_t size ) {
        unsigned char *mem = (unsigned char *)ptr;
        //std::string s; s.resize( size ); memcpy( &s[0], mem, size );
        //std::cout << hexdump( s) << std::endl;
        if( size >= 4 && mem && mem[0] == 'L' && mem[1] == 'Z' && mem[2] == 'I' && mem[3] == 'P' ) return LZLIB;
        if( size >= 1 && mem && mem[0] == 0xEC ) return MINIZ;
        if( size >= 1 && mem && mem[0] >= 0xF0 ) return LZ4;
        return NONE;
    }

    size_t bound( unsigned q, size_t len ) {
        size_t zlen = len;
        switch( q ) {
            break; default : zlen *= 2;
            break; case LZ4: zlen = LZ4_compressBound((int)(len));
        }
        return shout( q, "[bound]", len, zlen ), zlen;
    }

      bool pack( unsigned q, const char *in, size_t inlen, char *out, size_t &outlen ) {
        bool ok = false;
        if( in && inlen && out && outlen >= inlen ) {
            ok = true;
            switch( q ) {
                break; default: ok = false;
                break; case LZ4: outlen = LZ4_compress( in, out, inlen );
                break; case MINIZ: outlen = tdefl_compress_mem_to_mem( out, outlen, in, inlen, TDEFL_DEFAULT_MAX_PROBES ); //TDEFL_MAX_PROBES_MASK ); //
                break; case SHOCO: outlen = shoco_compress( in, inlen, out, outlen );
                break; case LZLIB: { int l; lzip_compress( (const uint8_t *)in, inlen, (uint8_t *)out, &l ); outlen = l; }
                /* for archival reasons: */
                // break; case LZHAM: { lzham_z_ulong l; lzham_z_compress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); outlen = l; }
            }
            // std::cout << nameof( typeof( out, outlen ) ) << std::endl;
        }
        ok = ok && outlen > 0 && outlen < inlen;
        outlen = ok && outlen ? outlen : 0;
        return shout( q, "[pack]", inlen, outlen ), ok;
      }

    bool unpack( unsigned q, const char *in, size_t inlen, char *out, size_t &outlen ) {
        bool ok = false;
        size_t bytes_read = 0;
        if( in && inlen && out && outlen ) {
            ok = true;
            switch( q ) {
                break; default: ok = false;
                break; case LZ4: bytes_read = LZ4_uncompress( in, out, outlen );
                break; case MINIZ: bytes_read = inlen; tinfl_decompress_mem_to_mem( out, outlen, in, inlen, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF );
                break; case SHOCO: bytes_read = inlen; shoco_decompress( in, inlen, out, outlen );
                break; case LZLIB: bytes_read = inlen; { int l = outlen; lzip_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, &l ); outlen = l; }
                /* for archival reasons: */
                // break; case LZHAM: bytes_read = inlen; { lzham_z_ulong l = outlen; lzham_z_uncompress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); }
            }
        }
        ok = ok && bytes_read == inlen;
        outlen = ok && outlen ? outlen : 0;
        return shout( q, "[unpack]", inlen, outlen ), ok;
    }

}

namespace bundle
{
    // compression methods

    bool is_z( const std::string &self ) {
        return self.size() > 0 && self.at(0) == '\0';
    }

    bool is_unz( const std::string &self ) {
        return !is_z( self );
    }

    std::string z( const std::string &self, unsigned q ) {
        if( is_z( self ) )
            return self;

        if( !self.size() )
            return self;

        std::vector<char> binary;
        std::string input = self;

        // compress

        if( !pack( binary, input, q ) )
            return self;

        // encapsulated by exploiting std::string design (portable? standard?)
        // should i encapsulate by escaping characters instead? (safer but slower)

        std::ostringstream oss;

        oss << '\x01' <<
            std::setfill('0') << std::setw(8) << std::hex << input.size() <<
            std::setfill('0') << std::setw(8) << std::hex << binary.size();

        std::string output = oss.str();

        output.resize( 1 + 8 + 8 + binary.size() );

        std::memcpy( &output.at( 1 + 8 + 8 ), &binary.at( 0 ), binary.size() );

        output.at( 0 ) = '\0';

        return output;
    }

    std::string unz( const std::string &self, unsigned q ) {
        if( is_z( self ) )
        {
            // decapsulate
            size_t size1, size2;

            std::stringstream ioss1;
            ioss1 << std::setfill('0') << std::setw(8) << std::hex << self.substr( 1, 8 );
            ioss1 >> size1;

            std::stringstream ioss2;
            ioss2 << std::setfill('0') << std::setw(8) << std::hex << self.substr( 9, 8 );
            ioss2 >> size2;

            std::string output;
            output.resize( size1 );

            std::vector<char> content( size2 );
            std::memcpy( &content.at( 0 ), &self.at( 17 ), size2 );

            // decompress
            if( unpack( output, content, q ) )
                return output;
        }

        return self;
    }
}

