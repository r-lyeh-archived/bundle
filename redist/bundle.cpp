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

#include <cassert>
#include <cctype>  // std::isprint
#include <cstdio>  // std::sprintf
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "bundle.hpp"

// easylzma interface
namespace {
    /* callbacks for streamed input and output */

    struct wrbuf {
        size_t pos;
        const size_t size;
        uint8_t *const data;

        wrbuf( uint8_t *const data_, size_t size_ ) : pos(0), size(size_), data(data_)
        {}

        size_t writebuf( const void *buf, size_t len ) {
            if( pos + len > size ) {
                len = size - pos;
            }
            memcpy( &data[ pos ], buf, len );
            pos += len;
            return len;
        }
    };

    struct rdbuf {
        size_t pos;
        const size_t size;
        const uint8_t *const data;

        rdbuf( const uint8_t *const data_, size_t size_ ) : pos(0), size(size_), data(data_)
        {}

        size_t readbuf( void *buf, size_t len ) {
            if( pos + len > size ) {
                len = size - pos;
            }
            memcpy( buf, &data[ pos ], len );
            pos += len;
            return len;
        }
    };

    size_t elzma_write_callback( void *ctx, const void *buf, size_t size ) {
        wrbuf * f = (wrbuf *) ctx;
        assert( f );
        return f->writebuf( buf, size );
    }

    int elzma_read_callback( void *ctx, void *buf, size_t *size ) {
        rdbuf * f = (rdbuf *) ctx;
        assert( f );
        return *size = f->readbuf( buf, *size ), 0;
    }

    template<bool is_lzip>
    size_t lzma_decompress( const uint8_t * const data, const size_t size,
                          uint8_t * const new_data, size_t * const out_sizep )
    {
        rdbuf rd( data, size );
        wrbuf wr( new_data, *out_sizep );

        elzma_file_format format = is_lzip ? ELZMA_lzip : ELZMA_lzma;
        elzma_decompress_handle hand = elzma_decompress_alloc();
        bool ok = false, init = NULL != hand;
        if( init && ELZMA_E_OK == elzma_decompress_run(
                hand, elzma_read_callback, (void *) &rd,
                elzma_write_callback, (void *) &wr, format) ) {
            ok = true;
        }
        if( init ) elzma_decompress_free(&hand);
        return ok ? 1 : 0;
    }

    template<bool is_lzip>
    size_t lzma_compress( const uint8_t * const data, const size_t size,
                        uint8_t * const new_data, size_t * const out_sizep )
    {
        /* default compression parameters, some of which may be overridded by
         * command line arguments */
        unsigned char level = 9;
        unsigned char lc = ELZMA_LC_DEFAULT;
        unsigned char lp = ELZMA_LP_DEFAULT;
        unsigned char pb = ELZMA_PB_DEFAULT;
        unsigned int maxDictSize = ELZMA_DICT_SIZE_DEFAULT_MAX;
        unsigned int dictSize = 0;
        elzma_file_format format = is_lzip ? ELZMA_lzip : ELZMA_lzma;

        elzma_compress_handle hand = NULL;

        /* determine a reasonable dictionary size given input size */
        dictSize = elzma_get_dict_size(size);
        if (dictSize > maxDictSize) dictSize = maxDictSize;

        /* allocate a compression handle */
        hand = elzma_compress_alloc();
        if (hand == NULL) {
            return 0;
        }

        if (ELZMA_E_OK != elzma_compress_config(hand, lc, lp, pb, level,
                                                dictSize, format,
                                                size)) {
            elzma_compress_free(&hand);
            return 0;
        }

        int rv;
        int pCtx = 0;

        rdbuf rd( data, size );
        wrbuf wr( new_data, *out_sizep );

        rv = elzma_compress_run(hand, elzma_read_callback, (void *) &rd,
                                elzma_write_callback, (void *) &wr,
                                (NULL), &pCtx);

        if (ELZMA_E_OK != rv) {
            elzma_compress_free(&hand);
            return 0;
        }

        *out_sizep = wr.pos;

        /* clean up */
        elzma_compress_free(&hand);
        return wr.pos;
    }
}

// zpaq interface
namespace
{
    class In: public libzpaq::Reader, public rdbuf {
        public:
        In( const uint8_t *const data_, size_t size_ ) : rdbuf( data_, size_ )
        {}
        int get() {
            if( pos >= size ) {
                return -1;
            }
            return data[pos++];
        }
        int read(char* buf, int n) {
            return this->readbuf( buf, n );
        }
    };

    class Out: public libzpaq::Writer, public wrbuf {
        public:
        Out( uint8_t *const data_, size_t size_ ) : wrbuf( data_, size_ )
        {}
        void put(int c) {
            if( pos < size ) {
                data[pos++] = (unsigned char)(c);
            }
        }
        int write(char* buf, int n) {
            return this->writebuf( buf, n );
        }
    };

    size_t zpaq_compress( const uint8_t * const data, const size_t size,
                        uint8_t * const new_data, size_t * const out_sizep )
    {
        In rd( data, size );
        Out wr( new_data, *out_sizep );

        libzpaq::compress(&rd, &wr, 3);  // level [1..3]
        *out_sizep = wr.pos;

        return wr.pos;
    }

    size_t zpaq_decompress( const uint8_t * const data, const size_t size,
                          uint8_t * const new_data, size_t * const out_sizep )
    {
        In rd( data, size );
        Out wr( new_data, *out_sizep );

        libzpaq::decompress( &rd, &wr );

        return 1;
    }
}
namespace libzpaq {
    // Implement error handler
    void error(const char* msg) {
        fprintf( stderr, "<bundle/bunle.cpp> says: ZPAQ fatal error! %s\n", msg );
        exit(1);
    }
}


namespace bundle {

    enum { verbose = false };

    namespace
    {
        std::string hexdump( const std::string &str ) {
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

        void shout( unsigned q, const char *context, size_t from, size_t to ) {
            if( verbose ) {
                std::cout << context << " q:" << q << ",from:" << from << ",to:" << to << std::endl;
            }
        }
    }

    const char *const name_of( unsigned q ) {
        switch( q ) {
            break; default : return "NONE";
            break; case LZ4: return "LZ4";
            break; case MINIZ: return "MINIZ";
            break; case SHOCO: return "SHOCO";
            break; case LZIP: return "LZIP";
            break; case LZMASDK: return "LZMA";
            break; case ZPAQ: return "ZPAQ";
            break; case LZ4HC: return "LZ4HC";
#if 0
            // for archival purposes
            break; case BZIP2: return "BZIP2";
            break; case BLOSC: return "BLOSC";
            break; case BSC: return "BSC";
            break; case FSE: return "FSE";
            break; case LZFX: return "LZFX";
            break; case LZHAM: return "LZHAM";
            break; case LZP1: return "LZP1";
#endif
        }
    }

    const char *const version_of( unsigned q ) {
        return "0";
    }

    const char *const ext_of( unsigned q ) {
        switch( q ) {
            break; default : return "unc";
            break; case LZ4: return "lz4";
            break; case MINIZ: return "miniz";
            break; case SHOCO: return "shoco";
            break; case LZIP: return "lz";
            break; case LZMASDK: return "lzma";
            break; case ZPAQ: return "zpaq";
            break; case LZ4HC: return "lz4";
#if 0
            // for archival purposes
            break; case BZIP2: return "bz2";
            break; case BLOSC: return "blosc";
            break; case BSC: return "bsc";
            break; case FSE: return "fse";
            break; case LZFX: return "lzfx";
            break; case LZHAM: return "lzham";
            break; case LZP1: return "lzp1";
#endif
        }
    }

    unsigned type_of( const void *ptr, size_t size ) {
        unsigned char *mem = (unsigned char *)ptr;
        //std::string s; s.resize( size ); memcpy( &s[0], mem, size );
        //std::cout << hexdump( s) << std::endl;
        if( size >= 4 && mem && mem[0] == 'L' && mem[1] == 'Z' && mem[2] == 'I' && mem[3] == 'P' ) return LZIP;
        if( size >= 1 && mem && mem[0] == 0xEC ) return MINIZ;
        if( size >= 1 && mem && mem[0] >= 0xF0 ) return LZ4;
        return NONE;
    }

    size_t bound( unsigned q, size_t len ) {
        enum { MAX_BUNDLE_HEADERS = 1 + 1 + (16 + 1) + (16 + 1) }; // up to 128-bit length sized streams
        size_t zlen = len;
        switch( q ) {
            break; default : zlen = zlen * 2;
            break; case LZ4: case LZ4HC: zlen = LZ4_compressBound((int)(len));
            break; case MINIZ: zlen = mz_compressBound(len);
#if 0
            // for archival purposes

            break; case LZP1: zlen = lzp_bound_compress((int)(len));
            //break; case FSE: zlen = FSE_compressBound((int)len);
#endif
        }
        return zlen += MAX_BUNDLE_HEADERS, shout( q, "[bound]", len, zlen ), zlen;
    }

    size_t unc_payload( unsigned q ) {
        size_t payload;
        switch( q ) {
            break; default : payload = 0;
#if 0
            break; case LZP1: payload = 256;
#endif
        }
        return payload;
    }

      bool pack( unsigned q, const void *in, size_t inlen, void *out, size_t &outlen ) {
        bool ok = false;
        if( in && inlen && out && outlen >= inlen ) {
            ok = true;
            switch( q ) {
                break; default: ok = false;
                break; case LZ4: outlen = LZ4_compress( (const char *)in, (char *)out, inlen );
                break; case LZ4HC: outlen = LZ4_compressHC2( (const char *)in, (char *)out, inlen, 16 );
                break; case MINIZ: outlen = tdefl_compress_mem_to_mem( out, outlen, in, inlen, TDEFL_MAX_PROBES_MASK ); // TDEFL_DEFAULT_MAX_PROBES );
                break; case SHOCO: outlen = shoco_compress( (const char *)in, inlen, (char *)out, outlen );
                break; case LZMASDK: outlen = lzma_compress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
                break; case LZIP: outlen = lzma_compress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
                break; case ZPAQ: outlen = zpaq_compress( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen );
#if 0
                // for archival purposes
                break; case BZIP2: { unsigned int o(outlen); if( BZ_OK != BZ2_bzBuffToBuffCompress( (char *)out, &o, (char *)in, inlen, 9 /*level*/, 0 /*verbosity*/, 30 /*default*/ ) ) outlen = 0; else outlen = o; }
                break; case BLOSC: { int clevel = 9, doshuffle = 0, typesize = 1;
                    int r = blosc_compress( clevel, doshuffle, typesize, inlen, in, out, outlen);
                    if( r <= 0 ) outlen = 0; else outlen = r; }
                break; case BSC: outlen = bsc_compress((const unsigned char *)in, (unsigned char *)out, inlen, LIBBSC_DEFAULT_LZPHASHSIZE, LIBBSC_DEFAULT_LZPMINLEN, LIBBSC_DEFAULT_BLOCKSORTER, LIBBSC_CODER_QLFC_ADAPTIVE, LIBBSC_FEATURE_FASTMODE | 0);
                break; case FSE: outlen = FSE_compress( out, (const unsigned char *)in, inlen ); if( outlen < 0 ) outlen = 0;
                break; case LZFX: if( lzfx_compress( in, inlen, out, &outlen ) < 0 ) outlen = 0;
                break; case LZP1: outlen = lzp_compress( (const uint8_t *)in, inlen, (uint8_t *)out, outlen );
                break; case LZHAM: {
                        // lzham_z_ulong l; lzham_z_compress2( (unsigned char *)out, &l, (const unsigned char *)in, inlen, LZHAM_Z_BEST_COMPRESSION ); outlen = l;

                        lzham_compress_params comp_params = {0};
                        comp_params.m_struct_size = sizeof(comp_params);
                        comp_params.m_dict_size_log2 = 23;
                        comp_params.m_level = static_cast<lzham_compress_level>(0);
                        comp_params.m_max_helper_threads = 1;
                        comp_params.m_compress_flags |= LZHAM_COMP_FLAG_FORCE_POLAR_CODING;

                        lzham_compress_status_t comp_status =
                        lzham_lib_compress_memory(&comp_params, (lzham_uint8 *)out, &outlen, (const lzham_uint8*)in, inlen, 0 );

                        if (comp_status != LZHAM_COMP_STATUS_SUCCESS) {
                            outlen = 0;
                        }
                   }
#endif

            }
            // std::cout << name_of( type_of( out, outlen ) ) << std::endl;
        }
        ok = ok && outlen > 0 && outlen < inlen;
        outlen = ok && outlen ? outlen : 0;
        return shout( q, "[pack]", inlen, outlen ), ok;
      }

    bool unpack( unsigned q, const void *in, size_t inlen, void *out, size_t &outlen ) {
        bool ok = false;
        size_t bytes_read = 0;
        if( in && inlen && out && outlen ) {
            ok = true;
            switch( q ) {
                break; default: ok = false;
                break; case LZ4: case LZ4HC: if( LZ4_decompress_safe( (const char *)in, (char *)out, inlen, outlen ) >= 0 ) bytes_read = inlen; // faster: bytes_read = LZ4_uncompress( (const char *)in, (char *)out, outlen );
                break; case MINIZ: bytes_read = inlen; tinfl_decompress_mem_to_mem( out, outlen, in, inlen, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF );
                break; case SHOCO: bytes_read = inlen; shoco_decompress( (const char *)in, inlen, (char *)out, outlen );
                break; case LZMASDK: if( lzma_decompress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
                break; case LZIP: if( lzma_decompress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
                break; case ZPAQ: if( zpaq_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen ) ) bytes_read = inlen;
#if 0
                // for archival purposes
                break; case BZIP2: { unsigned int o(outlen); if( BZ_OK == BZ2_bzBuffToBuffDecompress( (char *)out, &o, (char *)in, inlen, 0 /*fast*/, 0 /*verbosity*/ ) ) { bytes_read = inlen; outlen = o; }}
                break; case BLOSC: if( blosc_decompress( in, out, outlen ) > 0 ) bytes_read = inlen;
                break; case BSC: bsc_decompress((const unsigned char *)in, inlen, (unsigned char *)out, outlen, /*LIBBSC_FEATURE_FASTMODE | */0); bytes_read = inlen;
                break; case FSE: { int r = FSE_decompress( (unsigned char*)out, outlen, (const void *)in ); if( r >= 0 ) bytes_read = r; }
                break; case LZFX: if( lzfx_decompress( in, inlen, out, &outlen ) >= 0 ) bytes_read = inlen;
                break; case LZP1: lzp_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, outlen ); bytes_read = inlen;
                break; case LZHAM: {
                    //bytes_read = inlen; { lzham_z_ulong l = outlen; lzham_z_uncompress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); }

                    lzham_decompress_params decomp_params = {0};
                    decomp_params.m_struct_size = sizeof(decomp_params);
                    decomp_params.m_dict_size_log2 = 23;
                    decomp_params.m_decompress_flags |= LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED;

                    lzham_decompress_status_t status = lzham_lib_decompress_memory(&decomp_params,
                        (lzham_uint8*)out, &outlen, (const lzham_uint8*)in, inlen, 0);
                    if( status == LZHAM_DECOMP_STATUS_SUCCESS ) {
                        bytes_read = inlen;
                    }
                }
#endif
            }
        }
        ok = ok && bytes_read == inlen;
        outlen = ok && outlen ? outlen : 0;
        return shout( q, "[unpack]", inlen, outlen ), ok;
    }

}

namespace bundle
{
    // public API

    unsigned type_of( const std::string &self ) {
        return is_packed( self ) ? self[ 1 ] & 0x0F : NONE;
    }
    std::string name_of( const std::string &self ) {
        return bundle::name_of( type_of(self) );
    }
    std::string version_of( const std::string &self ) {
        return bundle::version_of( type_of(self) );
    }
    std::string ext_of( const std::string &self ) {
        return bundle::ext_of( type_of(self) );
    }

    std::string vlebit( size_t i ) {
        std::string out;
        do {
            out += (unsigned char)( 0x80 | (i & 0x7f));
            i >>= 7;
        } while( i > 0 );
        *out.rbegin() ^= 0x80;
        return out;
    }
    size_t vlebit( const char *&i ) {
        size_t out = 0, j = -7;
        do {
            out |= ((size_t(*i) & 0x7f) << (j += 7) );
        } while( size_t(*i++) & 0x80 );
        return out;
    }
}

namespace bundle
{
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

            zip_archive.m_file_offset_alignment = 8;

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
                        break; case EXTRA: case UBER: quality = MZ_BEST_COMPRESSION;
                    }

					std::string pathfile = filename->second;
					std::replace( pathfile.begin(), pathfile.end(), '\\', '/');

                    //std::cout << hexdump(content->second) << std::endl;

					if( comment == it->end() )
                    status = mz_zip_writer_add_mem_ex( &zip_archive, pathfile.c_str(), content->second.c_str(), bufsize,
                        0, 0, quality, 0, 0 );
                    else
                    status = mz_zip_writer_add_mem_ex( &zip_archive, pathfile.c_str(), content->second.c_str(), bufsize,
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

