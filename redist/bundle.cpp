// simple compression interface
// - rlyeh. mit licensed

#include <cassert>
#include <cctype>  // std::isprint
#include <cstdio>  // std::sprintf
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bundle.hpp"

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
    const unsigned long long member_size = (unsigned long long)~0;
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

    const char *const versionof( unsigned q ) {
        return "0";
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

    namespace {
        enum {
            OFFSET_MARK = 0,
            OFFSET_LEN = 1,
            OFFSET_ZLEN = 5,
            OFFSET_TYPE = 9,
            OFFSET_ZDATA = 10
        };
    }

    bool is_packed( const std::string &self ) {
        return self.size() > 0 && self.at(0) == '\0';
    }
    bool is_unpacked( const std::string &self ) {
        return !is_packed( self );
    }

    unsigned typeof( const std::string &self ) {
        return is_packed( self ) ? self[ OFFSET_TYPE ] : NONE;
    }
    std::string nameof( const std::string &self ) {
        return bundle::nameof( typeof(self) );
    }
    std::string versionof( const std::string &self ) {
        return bundle::versionof( typeof(self) );
    }
    std::string extof( const std::string &self ) {
        return bundle::extof( typeof(self) );
    }

    size_t length( const std::string &self ) {
        uint32_t size = self.size();
        if( is_packed(self) ) {
            size  = ((unsigned char)self[OFFSET_LEN + 0]) << 24;
            size |= ((unsigned char)self[OFFSET_LEN + 1]) << 16;
            size |= ((unsigned char)self[OFFSET_LEN + 2]) <<  8;
            size |= ((unsigned char)self[OFFSET_LEN + 3]) <<  0;
        }
        return size;
    }
    size_t zlength( const std::string &self ) {
        uint32_t size = 0;
        if( is_packed(self) ) {
            size  = ((unsigned char)self[OFFSET_ZLEN + 0]) << 24;
            size |= ((unsigned char)self[OFFSET_ZLEN + 1]) << 16;
            size |= ((unsigned char)self[OFFSET_ZLEN + 2]) <<  8;
            size |= ((unsigned char)self[OFFSET_ZLEN + 3]) <<  0;
        }
        return size;
    }

    void *zptr( const std::string &self ) {
        return is_packed( self ) ? (void *)&self[OFFSET_ZDATA] : (void *)0;
    }


    std::string pack( unsigned q, const std::string &self ) {
        if( is_packed( self ) )
            return self;

        if( !self.size() )
            return self;

        std::vector<char> binary;
        std::string input = self;

        // compress

        if( !pack( q, binary, input ) )
            return self;

        // encapsulated by exploiting std::string design (portable? standard?)
        // should i encapsulate by escaping characters instead? (safer but slower)

        uint32_t size1 = (uint32_t)input.size(), size2 = (uint32_t)binary.size();
        std::string output( 10 + size2, '\0' );

        output[1] = (unsigned char)(( size1 >> 24 ) & 0xff );
        output[2] = (unsigned char)(( size1 >> 16 ) & 0xff );
        output[3] = (unsigned char)(( size1 >>  8 ) & 0xff );
        output[4] = (unsigned char)(( size1 >>  0 ) & 0xff );

        output[5] = (unsigned char)(( size2 >> 24 ) & 0xff );
        output[6] = (unsigned char)(( size2 >> 16 ) & 0xff );
        output[7] = (unsigned char)(( size2 >>  8 ) & 0xff );
        output[8] = (unsigned char)(( size2 >>  0 ) & 0xff );

        output[9] = (unsigned char)( q & 0xff );

        std::memcpy( &output.at( 10 ), &binary.at( 0 ), size2 );

        return output;
    }

    std::string unpack( const std::string &self ) {
        if( is_packed( self ) )
        {
            // decapsulate
            uint32_t size1 = uint32_t( length( self ) ), size2 = uint32_t( zlength( self ) );
            unsigned Q = typeof( self );

            std::string output( size1, '\0' );

            std::vector<char> content( size2 );
            std::memcpy( &content.at( 0 ), zptr( self ), size2 );

            // decompress
            if( unpack( Q, output, content ) )
                return output;
        }

        return self;
    }
}

namespace bundle
{
    static std::string hexdump( const std::string &str ) {
        char spr[ 80 ];
        std::string out1, out2;

        for( unsigned i = 0; i < 16 && i < str.size(); ++i ) {
            std::sprintf( spr, "%c ", std::isprint(str[i]) ? str[i] : '.' );
            out1 += spr;
            std::sprintf( spr, "%02X ", (unsigned char)str[i] );
            out2 += spr;
        }

        for( unsigned i = str.size(); i < 16; ++i ) {
            std::sprintf( spr, ". " );
            out1 += spr;
            std::sprintf( spr, "?? " );
            out2 += spr;
        }

        return out1 + "[...] (" + out2 + "[...])";
    }

    bool pakfile::has( const std::string &property ) const {
        return this->find( property ) != this->end();
    }

    // binary serialization

    bool pak::bin( const std::string &bin_import ) //const
    {
        this->clear();
        std::vector<pakfile> result;

        if( !bin_import.size() )
            return true; // :)

        if( type == paktype::ZIP )
        {
            // Try to open the archive.
            mz_zip_archive zip_archive;
            memset(&zip_archive, 0, sizeof(zip_archive));

            mz_bool status = mz_zip_reader_init_mem( &zip_archive, (void *)bin_import.c_str(), bin_import.size(), 0 );

            if( !status )
                return "mz_zip_reader_init_file() failed!", false;

            // Get and print information about each file in the archive.
            for( unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++ )
            {
                mz_zip_archive_file_stat file_stat;

                if( !mz_zip_reader_file_stat( &zip_archive, i, &file_stat ) )
                {
                    mz_zip_reader_end( &zip_archive );
                    return "mz_zip_reader_file_stat() failed!", false;
                }

                result.push_back( pakfile() );

                result.back()["filename"] = file_stat.m_filename;
                result.back()["comment"] = file_stat.m_comment;
                result.back()["size"] = (unsigned int)file_stat.m_uncomp_size;
                result.back()["size_z"] = (unsigned int)file_stat.m_comp_size;
                //result.back()["modify_time"] = ze.mtime;
                //result.back()["access_time"] = ze.atime;
                //result.back()["create_time"] = ze.ctime;
                //result.back()["attributes"] = ze.attr;

                if( const bool decode = true )
                {
                    // Try to extract file to the heap.
                    size_t uncomp_size;

                    void *p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);

                    if( !p )
                    {
                        mz_zip_reader_end(&zip_archive);
                        return "mz_zip_reader_extract_file_to_heap() failed!", false;
                    }

                    // Make sure the extraction really succeeded.
                    /*
                    if ((uncomp_size != strlen(s_pStr)) || (memcmp(p, s_pStr, strlen(s_pStr))))
                    {
                    free(p);
                    mz_zip_reader_end(&zip_archive);
                    return "mz_zip_reader_extract_file_to_heap() failed to extract the proper data", false;
                    }
                    */

                    result.back()["content"].resize( uncomp_size );
                    memcpy( (void *)result.back()["content"].data(), p, uncomp_size );

                    free(p);
                }
            }

            // We're done.
            mz_zip_reader_end(&zip_archive);
        }
        else
        {}

        this->resize( result.size() );
        std::copy( result.begin(), result.end(), this->begin() );

        return true;
    }

    std::string pak::bin( unsigned q ) const
    {
        std::string result;

        if( type == paktype::ZIP )
        {
            mz_zip_archive zip_archive;
            memset( &zip_archive, 0, sizeof(zip_archive) );

            mz_bool status = mz_zip_writer_init_heap( &zip_archive, 0, 0 );

            if( !status ) {
                assert( status );
                return "mz_zip_writer_init_heap() failed!", std::string();
            }

            for( const_iterator it = this->begin(), end = this->end(); it != end; ++it ) {
                std::map< std::string, bundle::string >::const_iterator filename = it->find("filename");
                std::map< std::string, bundle::string >::const_iterator content = it->find("content");
                std::map< std::string, bundle::string >::const_iterator comment = it->find("comment");
                if( filename != it->end() && content != it->end() ) {
                    const size_t bufsize = content->second.size();

                    int quality = q;
                    if( it->find("compression") != it->end() ) {
                        std::stringstream ss( it->find("compression")->second );
                        if( !(ss >> quality) ) quality = q;
                    }
                    switch( quality ) {
                        break; case DEFAULT: default: quality = MZ_DEFAULT_LEVEL;
                        break; case UNCOMPRESSED: quality = MZ_NO_COMPRESSION;
                        break; case FAST: case ASCII: quality = MZ_BEST_SPEED;
                        break; case EXTRA: quality = MZ_BEST_COMPRESSION;
                    }

                    if( comment == it->end() )
                    status = mz_zip_writer_add_mem_ex( &zip_archive, filename->second.c_str(), content->second.c_str(), bufsize,
                        0, 0, quality, 0, 0 );
                    else
                    status = mz_zip_writer_add_mem_ex( &zip_archive, filename->second.c_str(), content->second.c_str(), bufsize,
                        comment->second.c_str(), comment->second.size(), quality, 0, 0 );

                    //status = mz_zip_writer_add_mem( &zip_archive, filename->second.c_str(), content->second.c_str(), bufsize, quality );
                    if( !status ) {
                        assert( status );
                        return "mz_zip_writer_add_mem() failed!", std::string();
                    }
                }
            }

            void *pBuf;
            size_t pSize;

            status = mz_zip_writer_finalize_heap_archive( &zip_archive, &pBuf, &pSize);
            if( !status ) {
                assert( status );
                return "mz_zip_writer_finalize_heap_archive() failed!", std::string();
            }

            // Ends archive writing, freeing all allocations, and closing the output file if mz_zip_writer_init_file() was used.
            // Note for the archive to be valid, it must have been finalized before ending.
            status = mz_zip_writer_end( &zip_archive );
            if( !status ) {
                assert( status );
                return "mz_zip_writer_end() failed!", std::string();
            }

            result.resize( pSize );
            memcpy( &result.at(0), pBuf, pSize );

            free( pBuf );
        }
        else
        {}

        return result;
    }
}

