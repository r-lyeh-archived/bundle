/* Simple compression interface.
 * Copyright (c) 2013, 2014, 2015, 2016, 2017 r-lyeh.
 * ZLIB/libPNG licensed.
 
 * - rlyeh ~~ listening to Boris / Missing Pieces
 */

#include <cassert>
#include <stdio.h>
#include <stdint.h>

#include <cctype>  // std::isprint
#include <cstdio>  // std::sprintf
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "bundle.h"

namespace {

    /* callbacks for streamed input and output */

    struct membuf : std::streambuf {
        membuf( const char* base, std::ptrdiff_t n ) {
            this->setg( (char *)base, (char *)base, (char *)base + n );
        }
        membuf( char* base, std::ptrdiff_t n ) {
            this->setp( base, base + n );
        }
        size_t position() const {
            return this->pptr() - this->pbase();
        }
    };

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
}

#ifndef BUNDLE_NO_ZLING
// zling interface
namespace zling {

    struct MemInputter : public baidu::zling::Inputter {
        const unsigned char *begin, *ptr, *end;
        MemInputter( const unsigned char *mem, size_t len ) 
        : begin(mem), ptr(mem), end(mem + len) 
        {}
        size_t GetData(unsigned char* buf, size_t len) {
            len = ptr + len >= end ? end - ptr : len; 
            memcpy( buf, ptr, len );
            ptr += len;
            return len;
        }
        bool IsEnd() {
            return ptr >= end;
        }
        bool IsErr() {
            return 0;
        }
        size_t GetInputSize() const { 
            return ptr - begin;
        }
    };

    struct MemOutputter : public baidu::zling::Outputter {
        unsigned char *begin, *ptr, *end;
        MemOutputter( unsigned char *mem, size_t len ) 
        : begin(mem), ptr(mem), end(mem + len) 
        {}
        size_t PutData(unsigned char* buf, size_t len) {
            len = ptr + len >= end ? end - ptr : len; 
            memcpy( ptr, buf, len );
            ptr += len;
            return len;
        }
        bool IsErr() {
            return 0;
        }
        size_t GetOutputSize() const { 
            return ptr - begin;
        }
    };

    struct ActionHandler: baidu::zling::ActionHandler {
        MemInputter*  inputter;
        MemOutputter* outputter;

        ActionHandler() : inputter(0), outputter(0) 
        {}

        void OnInit() {
            inputter  = dynamic_cast<MemInputter*>(GetInputter());
            outputter = dynamic_cast<MemOutputter*>(GetOutputter());
        }

        void OnDone() {
        }

        void OnProcess(unsigned char* orig_data, size_t orig_size) { /*
            const char* encode_direction;
            uint64_t isize;
            uint64_t osize;

            if (IsEncode()) {
                encode_direction = "=>";
                isize = inputter->GetInputSize();
                osize = outputter->GetOutputSize();
            } else {
                encode_direction = "<=";
                isize = outputter->GetOutputSize();
                osize = inputter->GetInputSize();
            }
        */ }
    };
}
#endif

// easylzma interface
#ifndef BUNDLE_NO_LZIP
namespace {
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
#endif

// zpaq interface
#ifndef BUNDLE_NO_ZPAQ
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

        libzpaq::compress(&rd, &wr, "5");  // "0".."5" = faster..better
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
        fprintf( stderr, "<bundle/bundle.cpp> says: ZPAQ fatal error! %s\n", msg );
        exit(1);
    }
}
#endif

// CSC interface
#ifndef BUNDLE_NO_CSC
namespace {
    struct CSCSeqStream {
        union {
            ISeqInStream is;
            ISeqOutStream os;
        };
        rdbuf *rd;
        wrbuf *wr;
    };

    int csc_read(void *p, void *buf, size_t *size) {
        CSCSeqStream *sss = (CSCSeqStream *)p;
        *size = sss->rd->readbuf(buf, *size);
        return 0;
    }

    size_t csc_write(void *p, const void *buf, size_t size) {
        CSCSeqStream *sss = (CSCSeqStream *)p;
        return sss->wr->writebuf(buf, size);
    }
}
#endif

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

            return out2 + "[...] (" + out1 + "[...])";
        }

        bool init_bsc() {
#       ifndef BUNDLE_NO_BSC
            static const bool init_bsc = (bsc_init(0), true);
#       endif
            return true;
        }

        bool init_mcm() {
#       ifndef BUNDLE_NO_MCM
            static const bool init_mcm = (CompressorFactories::init(), true);
#       endif
            return true;
        }

        bool init() {
#       if 0
            // for archival purposes:
            static const bool init_yappy = (Yappy_FillTables(), true);
            static struct raii {
                raii() {
                    blosc_init();
                    blosc_set_nthreads(1);
                }
                ~raii() {
                    blosc_destroy();
                }
            } init_blosc;
#       endif
            return true;
        }
    }


    enum archives {
        ZIP, BUN
    };

    bool file::has( const std::string &property ) const {
        return this->find( property ) != this->end();
    }

    // binary serialization
    bool archive::bin( int type, const std::string &binary )
    {
        this->clear();
        std::vector<file> &result = *this;

        if( !binary.size() )
            return true; // :)

        if( type == ZIP || type == BUN )
        {
#ifndef BUNDLE_NO_MINIZ
            // Try to open the archive.
            mz_zip_archive zip_archive;
            memset(&zip_archive, 0, sizeof(zip_archive));

            mz_bool status = mz_zip_reader_init_mem( &zip_archive, (void *)binary.c_str(), binary.size(), 0 );

            if( !status )
                return  false; // "mz_zip_reader_init_file() failed!"

            // Get and print information about each file in the archive.
            for( unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++ )
            {
                mz_zip_archive_file_stat file_stat;

                if( !mz_zip_reader_file_stat( &zip_archive, i, &file_stat ) )
                {
                    mz_zip_reader_end( &zip_archive );
                    return  false; // "mz_zip_reader_file_stat() failed!"
                }

                result.push_back( file() );
                file &back = result.back();

                // @todo: add offset in file
                back["/**/"] = file_stat.m_comment;
                back["name"] = file_stat.m_filename;
                back["size"] = bundle::itoa( (unsigned int)file_stat.m_uncomp_size );
                back["zlen"] = bundle::itoa( (unsigned int)file_stat.m_comp_size );
                //back["modt"] = bundle::itoa( ze.mtime );
                //back["acct"] = bundle::itoa( ze.atime );
                //back["cret"] = bundle::itoa( ze.ctime );
                //back["attr"] = bundle::itoa( ze.attr );

                if( const bool decode = true )
                {
                    // Try to extract file to the heap.
                    size_t uncomp_size = file_stat.m_uncomp_size;
                    back["data"].resize( uncomp_size );
                    mz_bool ok = mz_zip_reader_extract_file_to_mem(&zip_archive, file_stat.m_filename, &back["data"][0], uncomp_size, 0);

                    // Make sure the extraction really succeeded.
                    if( !ok ) {
                        mz_zip_reader_end(&zip_archive);
                        //assert( !"mz_zip_reader_extract_file_to_heap() failed!" );
                        return false;
                    }

                    const void *p = &back["data"][0];
                    if( type == BUN && bundle::is_packed(p, uncomp_size) ) {
                        back["type"] = bundle::name_of( bundle::type_of(p, uncomp_size) );
                        back["size"] = bundle::itoa( bundle::len( p, uncomp_size ) );
                    } else {
                        back["type"] = "ZIP";
                    }
                }
            }

            // We're done.
            mz_zip_reader_end(&zip_archive);
#else
            return false;
#endif
        }
        else
        {}

        return true;
    }

    std::string archive::bin( int type, unsigned level ) const
    {
        std::string result;

        if( type == ZIP || type == BUN )
        {
#ifndef BUNDLE_NO_MINIZ
            mz_zip_archive zip_archive;
            memset( &zip_archive, 0, sizeof(zip_archive) );

            zip_archive.m_file_offset_alignment = 8;

            mz_bool status = mz_zip_writer_init_heap( &zip_archive, 0, 0 );

            if( !status ) {
                //assert( !"mz_zip_writer_init_heap() failed!" );
                return std::string();
            }

            for( const_iterator it = this->begin(), end = this->end(); it != end; ++it ) {
                std::map< std::string, std::string >::const_iterator filename = it->find("name");
                std::map< std::string, std::string >::const_iterator content = it->find("data");
                std::map< std::string, std::string >::const_iterator comment = it->find("/**/");
                if( filename != it->end() && content != it->end() ) {
                    const size_t bufsize = content->second.size();

                    int quality, padding(0);
                    if( type == ZIP ) {
                        quality = level;
                        if( it->find("comp") != it->end() ) {
                            std::stringstream ss( it->find("comp")->second );
                            if( !(ss >> quality) ) quality = level;
                        }
                        /**/ if( level >= 80 ) quality = MZ_BEST_COMPRESSION;
                        else if( level >= 50 ) quality = MZ_DEFAULT_LEVEL;
                        else if( level >= 10 ) quality = MZ_BEST_SPEED;
                        else                 quality = MZ_NO_COMPRESSION;
                    }
                    else
                    if( type == BUN ) {
                        quality = MZ_NO_COMPRESSION;
                        padding = bundle::padding( content->second );
                    }

                    std::string pathfile = filename->second;
                    std::replace( pathfile.begin(), pathfile.end(), '\\', '/');

                    //std::cout << hexdump(content->second) << std::endl;

                    if( comment == it->end() ) {
                        status = mz_zip_writer_add_mem_ex( &zip_archive, pathfile.c_str(), content->second.c_str() + padding, bufsize,
                            0, 0, quality, 0, 0 );
                    } else {
                        status = mz_zip_writer_add_mem_ex( &zip_archive, pathfile.c_str(), content->second.c_str() + padding, bufsize,
                            comment->second.c_str(), comment->second.size(), quality, 0, 0 );
                    }

                    if( !status ) {
                        //assert( !"mz_zip_writer_add_mem() failed!" );
                        return std::string();
                    }
                }
            }

            void *pBuf;
            size_t pSize;

            status = mz_zip_writer_finalize_heap_archive( &zip_archive, &pBuf, &pSize);
            if( !status ) {
                //assert( !"mz_zip_writer_finalize_heap_archive() failed!" );
                return std::string();
            }

            // Ends archive writing, freeing all allocations, and closing the output file if mz_zip_writer_init_file() was used.
            // Note for the archive to be valid, it must have been finalized before ending.
            status = mz_zip_writer_end( &zip_archive );
            if( !status ) {
                //assert( !"mz_zip_writer_end() failed!" );
                return std::string(); 
            }

            result.resize( pSize );
            memcpy( &result.at(0), pBuf, pSize );

            free( pBuf );
#endif
        }
        else
        {}

        return result;
    }

    // .bun binary serialization
    bool archive::bun( const std::string &binary ) {
        return bin( BUN, binary );
    }
    std::string archive::bun() const {
        return bin( BUN, 0 );
    }

    // .zip binary serialization
    bool archive::zip( const std::string &binary ) {
        return bin( ZIP, binary );
    }
    std::string archive::zip( unsigned level ) const {
        return bin( ZIP, level );
    }
}

extern "C" {

//BUNDLE_API
size_t bundle_enc_vlebit( char *out, size_t val ) {
    char *begin = out;
        do {
            *out++ = (unsigned char)( 0x80 | (val & 0x7f));
            val >>= 7;
        } while( val > 0 );
        out[-1] ^= 0x80;
    return out - begin;
}

//BUNDLE_API
size_t bundle_dec_vlebit( const char *i, size_t *val ) {
    const char *begin = i;
        size_t out = 0, j = -7;
        do {
            out |= ((size_t(*i) & 0x7f) << (j += 7) );
        } while( size_t(*i++) & 0x80 );
        *val = out;
    return i - begin;
}

//BUNDLE_API
size_t bundle_padding( const void *mem, size_t size ) {
    size_t padding = 0;
    unsigned char *mem8 = (unsigned char *)mem;
    while( mem8 && size && (0 == *mem8) ) mem8++, padding++, size--;
    return padding;
}

//BUNDLE_API
bool bundle_is_packed( const void *mem, size_t size ) {
    unsigned char *mem8 = (unsigned char *)mem;
    while( mem8 && size && (0 == *mem8) ) mem8++, size--;
    return mem8 && (size >= 2) && (0x70 == mem8[0]) && (mem8[1] <= bundle::BZIP2);
}
//BUNDLE_API
bool bundle_is_unpacked( const void *mem, size_t size ) {
    return !bundle_is_packed( mem, size );
}

//BUNDLE_API
const char *const bundle_name_of( unsigned q ) {
    switch( q ) {
        break; default : return "RAW";
        break; case BUNDLE_LZ4F: return "LZ4F";
        break; case BUNDLE_MINIZ: return "MINIZ";
        break; case BUNDLE_SHOCO: return "SHOCO";
        break; case BUNDLE_LZIP: return "LZIP";
        break; case BUNDLE_LZMA20: return "LZMA20";
        break; case BUNDLE_LZMA25: return "LZMA25";
        break; case BUNDLE_ZPAQ: return "ZPAQ";
        break; case BUNDLE_LZ4: return "LZ4";
        break; case BUNDLE_BROTLI9: return "BROTLI9";
        break; case BUNDLE_BROTLI11: return "BROTLI11";
        break; case BUNDLE_ZSTD: return "ZSTD";
        break; case BUNDLE_ZSTDF: return "ZSTDF";
        break; case BUNDLE_BSC: return "BSC";
        break; case BUNDLE_SHRINKER: return "SHRINKER";
        break; case BUNDLE_CSC20: return "CSC20";
        break; case BUNDLE_BCM: return "BCM";
        break; case BUNDLE_ZLING: return "ZLING";
        break; case BUNDLE_MCM: return "MCM";
        break; case BUNDLE_TANGELO: return "TANGELO";
        break; case BUNDLE_ZMOLLY: return "ZMOLLY";
        break; case BUNDLE_CRUSH: return "CRUSH";
        break; case BUNDLE_LZJB: return "LZJB";
        break; case BUNDLE_BZIP2: return "BZIP2";
#if 0
        // for archival purposes
        break; case BUNDLE_BLOSC: return "BLOSC";
        break; case BUNDLE_FSE: return "FSE";
        break; case BUNDLE_LZFX: return "LZFX";
        break; case BUNDLE_LZHAM: return "LZHAM";
        break; case BUNDLE_LZP1: return "LZP1";
        break; case BUNDLE_YAPPY: return "YAPPY";
#endif
    }
}

//BUNDLE_API
const char *const bundle_version_of( unsigned q ) {
    return "0";
}

//BUNDLE_API
const char *const bundle_ext_of( unsigned q ) {
    switch( q ) {
        break; default : return "";
        break; case BUNDLE_LZ4F: return "lz4";
        break; case BUNDLE_MINIZ: return "defl";
        break; case BUNDLE_SHOCO: return "sho";
        break; case BUNDLE_LZIP: return "lz";
        break; case BUNDLE_LZMA20: return "lzma";
        break; case BUNDLE_LZMA25: return "lzma";
        break; case BUNDLE_ZPAQ: return "zpaq";
        break; case BUNDLE_LZ4: return "lz4";
        break; case BUNDLE_BROTLI9: return "bro";
        break; case BUNDLE_BROTLI11: return "bro";
        break; case BUNDLE_ZSTD: return "zstd";
        break; case BUNDLE_ZSTDF: return "zstd";
        break; case BUNDLE_BSC: return "bsc";
        break; case BUNDLE_SHRINKER: return "shk";
        break; case BUNDLE_CSC20: return "csc";
        break; case BUNDLE_BCM: return "bcm";
        break; case BUNDLE_ZLING: return "zli";
        break; case BUNDLE_MCM: return "mcm";
        break; case BUNDLE_TANGELO: return "tan";
        break; case BUNDLE_ZMOLLY: return "zmo";
        break; case BUNDLE_CRUSH: return "crush";
        break; case BUNDLE_LZJB: return "lzjb";
        break; case BUNDLE_BZIP2: return "bz2";
#if 0
        // for archival purposes
        break; case BUNDLE_BLOSC: return "blosc";
        break; case BUNDLE_FSE: return "fse";
        break; case BUNDLE_LZFX: return "lzfx";
        break; case BUNDLE_LZHAM: return "lzham";
        break; case BUNDLE_LZP1: return "lzp1";
        break; case BUNDLE_YAPPY: return "yappy";
#endif
    }
}

//BUNDLE_API
unsigned bundle_guess_type_of( const void *ptr, size_t len ) {
    unsigned char *mem = (unsigned char *)ptr + bundle_padding( ptr, len );
    //std::string s; s.resize( len ); memcpy( &s[0], mem, len );
    //std::cout << hexdump( s) << std::endl;
    if( len >= 4 && mem && mem[0] == 'L' && mem[1] == 'Z' && mem[2] == 'I' && mem[3] == 'P' ) return BUNDLE_LZIP;
    if( len >= 1 && mem && mem[0] == 0xEC ) return BUNDLE_MINIZ;
    if( len >= 1 && mem && mem[0] >= 0xF0 ) return BUNDLE_LZ4F;
    return BUNDLE_RAW;
}

//BUNDLE_API
unsigned bundle_type_of( const void *ptr, size_t len ) {
    const char *mem = (const char *)ptr + bundle_padding( ptr, len ) + 2;
    return (*mem);
}

//BUNDLE_API
size_t bundle_len( const void *ptr, size_t len ) {
    if( !bundle_is_packed(ptr, len) ) return 0;
    const char *mem = (const char *)ptr + bundle_padding( ptr, len ) + 3;
    return bundle::vlebit(mem);
}

//BUNDLE_API
size_t bundle_zlen( const void *ptr, size_t len ) {
    if( !bundle_is_packed(ptr, len) ) return 0;
    const char *mem = (const char *)ptr + bundle_padding( ptr, len ) + 3;
    return bundle::vlebit(mem), bundle::vlebit(mem);
}

//BUNDLE_API
const void *bundle_zptr( const void *ptr, size_t len ) {
    if( !bundle_is_packed(ptr, len) ) return 0;
    const char *mem = (const char *)ptr + bundle_padding( ptr, len ) + 3;
    return bundle::vlebit(mem), bundle::vlebit(mem), (const void *)mem;
}

//BUNDLE_API
size_t bundle_bound( unsigned q, size_t len ) {
    size_t zlen = len;
    switch( q ) {
        break; default : zlen = zlen * 2;
        break; case BUNDLE_RAW: zlen = zlen * 1;
#ifndef BUNDLE_NO_LZ4
        break; case BUNDLE_LZ4F: case BUNDLE_LZ4: zlen = LZ4_compressBound((int)(len));
#endif
#ifndef BUNDLE_NO_MINIZ
        break; case BUNDLE_MINIZ: zlen = mz_compressBound(len);
#endif
#ifndef BUNDLE_NO_ZSTD
        break; case BUNDLE_ZSTD:  zlen = ZSTD_compressBound(len);
        break; case BUNDLE_ZSTDF: zlen = ZSTD_compressBound(len);
#endif
#if 0
        // for archival purposes
        break; case BUNDLE_LZP1: zlen = lzp_bound_compress((int)(len));
        //break; case BUNDLE_FSE: zlen = FSE_compressBound((int)len);
#endif
    }
    return zlen += BUNDLE_MAX_HEADER_SIZE;
}

//BUNDLE_API
size_t bundle_unc_payload( unsigned q ) {
    size_t payload;
    switch( q ) {
        break; default : payload = 0;
#if 0
        break; case BUNDLE_LZP1: payload = 256;
#endif
    }
    return payload;
}

//BUNDLE_API
bool bundle_pack( unsigned q, const void *in, size_t inlen, void *out, size_t *outlen ) {
    bool ok = false;
    if( in && inlen && out && *outlen >= inlen ) {
        ok = true;
        switch( q ) {
            break; default: ok = false;
            break; case BUNDLE_RAW: memcpy( out, in, *outlen = inlen ); 
#ifndef BUNDLE_NO_LZ4
            break; case BUNDLE_LZ4F: *outlen = LZ4_compress( (const char *)in, (char *)out, inlen );
            break; case BUNDLE_LZ4: *outlen = LZ4_compressHC2( (const char *)in, (char *)out, inlen, 16 );
#endif
#ifndef BUNDLE_NO_MINIZ
            break; case BUNDLE_MINIZ: *outlen = tdefl_compress_mem_to_mem( out, *outlen, in, inlen, TDEFL_MAX_PROBES_MASK ); // TDEFL_DEFAULT_MAX_PROBES );
#endif
#ifndef BUNDLE_NO_SHOCO
            break; case BUNDLE_SHOCO: *outlen = shoco_compress( (const char *)in, inlen, (char *)out, *outlen );
#endif
#ifndef BUNDLE_NO_LZMA
            break; case BUNDLE_LZMA20: case BUNDLE_LZMA25: { //outlen = lzma_compress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, outlen );
                    SizeT propsSize = LZMA_PROPS_SIZE;
                    *outlen = *outlen - LZMA_PROPS_SIZE - 8;
#if 0
                    ok = ( SZ_OK == LzmaCompress(
                    &((unsigned char *)out)[LZMA_PROPS_SIZE + 8], outlen,
                    (const unsigned char *)in, inlen,
                    &((unsigned char *)out)[0], &propsSize,
                    level, dictSize, lc, lp, pb, fb, numThreads ) );
#else
                    CLzmaEncProps props;
                    LzmaEncProps_Init(&props);
                    props.level = 9;                                      /* 0 <= level <= 9, default = 5 */
                    props.dictSize = 1 << (q == BUNDLE_LZMA25 ? 25 : 20); /* default = (1 << 24) */
                    props.lc = 3;                                         /* 0 <= lc <= 8, default = 3  */
                    props.lp = 0;                                         /* 0 <= lp <= 4, default = 0  */
                    props.pb = 2;                                         /* 0 <= pb <= 4, default = 2  */
                    props.fb = 32;                                        /* 5 <= fb <= 273, default = 32 */
                    props.numThreads = 1;                                 /* 1 or 2, default = 2 */
                    props.writeEndMark = 1;                               /* 0 or 1, default = 0 */

                    ok = (SZ_OK == LzmaEncode(
                    &((unsigned char *)out)[LZMA_PROPS_SIZE + 8], outlen,
                    (const unsigned char *)in, inlen,
                    &props, &((unsigned char *)out)[0], (SizeT*)&propsSize, props.writeEndMark,
                    NULL, &g_Alloc, &g_Alloc));
                    ok = ok && (propsSize == LZMA_PROPS_SIZE);
#endif
                    if( ok ) {
                        // serialize outsize as well (classic 13-byte LZMA header)
                        uint64_t x = htole64( (uint64_t)outlen );
                        memcpy( &((unsigned char *)out)[LZMA_PROPS_SIZE], (unsigned char *)&x, 8 );
                        *outlen = *outlen + LZMA_PROPS_SIZE + 8;
                    }
            }
#endif
#ifndef BUNDLE_NO_LZIP
            break; case BUNDLE_LZIP: *outlen = lzma_compress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, outlen );
#endif
#ifndef BUNDLE_NO_ZPAQ
            break; case BUNDLE_ZPAQ: *outlen = zpaq_compress( (const uint8_t *)in, inlen, (uint8_t *)out, outlen );
#endif
#ifndef BUNDLE_NO_BROTLI
            break; case BUNDLE_BROTLI9: case BUNDLE_BROTLI11: {
                    brotli::BrotliParams bp;
                    // Default compression mode. The compressor does not know anything in
                    // advance about the properties of the input. MODE_GENERIC
                    // Compression mode for UTF-8 format text input. MODE_TEXT
                    // Compression mode used in WOFF 2.0. MODE_FONT
                    bp.mode = brotli::BrotliParams::MODE_GENERIC;
                    // Controls the compression-speed vs compression-density tradeoffs. The higher
                    // the quality, the slower the compression. Range is 0 to 11.
                    bp.quality = BUNDLE_BROTLI9 == q ? 9 : 11; 
                    // Base 2 logarithm of the sliding window size. Range is 10 to 24.
                    bp.lgwin = 24; 
                    // Base 2 logarithm of the maximum input block size. Range is 16 to 24.
                    // If set to 0, the value will be set based on the quality.
                    bp.lgblock = 0;
                    ok = (1 == brotli::BrotliCompressBuffer( bp, inlen, (const uint8_t *)in, outlen, (uint8_t *)out ));
            }
#endif
#ifndef BUNDLE_NO_ZSTD
            break; case BUNDLE_ZSTD:  *outlen = ZSTD_HC_compress( out, *outlen, in, inlen, 13 /*1..26*/ ); if( ZSTD_isError(*outlen) ) *outlen = 0;
            break; case BUNDLE_ZSTDF: *outlen = ZSTD_compress( out, *outlen, in, inlen ); if( ZSTD_isError(*outlen) ) *outlen = 0;
#endif
#ifndef BUNDLE_NO_BSC
            break; case BUNDLE_BSC: {
                static const bool crt0 = bundle::init_bsc();
                *outlen = bsc_compress((const unsigned char *)in, (unsigned char *)out, inlen, LIBBSC_DEFAULT_LZPHASHSIZE, LIBBSC_DEFAULT_LZPMINLEN, LIBBSC_DEFAULT_BLOCKSORTER, LIBBSC_CODER_QLFC_ADAPTIVE, LIBBSC_FEATURE_FASTMODE | 0);
            }
#endif
#ifndef BUNDLE_NO_SHRINKER
            break; case BUNDLE_SHRINKER: *outlen = shrinker_compress((void *)in, out, inlen); if( -1 == int(*outlen) ) *outlen = 0;
#endif
#ifndef BUNDLE_NO_CSC
            break; case BUNDLE_CSC20: {
                    CSCSeqStream isss, osss;
                    isss.rd = new rdbuf( (const uint8_t *)in, inlen );
                    isss.is.Read = csc_read;
                    osss.wr = new wrbuf( (uint8_t *)out, *outlen );
                    osss.os.Write = csc_write;

                    CSCProps p;
                    uint32_t dict_size = 20 * 1024 * 1024;
                    int level = 5;

                    if (inlen < dict_size)
                        dict_size = inlen;

                    // init the default settings
                    CSCEncProps_Init(&p, dict_size, level);

                    //printf("Estimated memory usage: %llu MB\n", CSCEnc_EstMemUsage(&p) / 1048576ull);
                    unsigned char buf[CSC_PROP_SIZE];
                    CSCEnc_WriteProperties(&p, buf, 0);
                    osss.wr->writebuf( buf, CSC_PROP_SIZE );

                    CSCEncHandle h = CSCEnc_Create(&p, (ISeqOutStream*)&osss, 0);
                    CSCEnc_Encode(h, (ISeqInStream*)&isss, 0);
                    CSCEnc_Encode_Flush(h);
                    CSCEnc_Destroy(h);

                    *outlen = osss.wr->pos;
                    delete isss.rd, isss.rd = 0;
                    delete osss.wr, osss.wr = 0;
            }
#endif
#ifndef BUNDLE_NO_ZLING
        break; case BUNDLE_ZLING: {
            //try {
                zling::MemInputter  inputter((const unsigned char *)in, inlen);
                zling::MemOutputter outputter((unsigned char *)out, *outlen);
                //zling::ActionHandler handler;
                bool ok = 0 == baidu::zling::Encode(&inputter, &outputter, 0/*&handler*/, 4 /*0..4*/);
                *outlen = outputter.GetOutputSize();
            /*} catch (const std::runtime_error& e) {
                fprintf(stderr, "zling: runtime error: %s\n", e.what());
            } catch (const std::bad_alloc& e) {
                fprintf(stderr, "zling: allocation failed.");
            }*/
        }
#endif
#ifndef BUNDLE_NO_ZMOLLY
        break; case BUNDLE_ZMOLLY: {
                enum { zmolly_blocksize = 1048576 * 16 };  /*x1..99*/
                membuf inbuf((const char *)in, inlen);
                membuf outbuf((char *)out, *outlen);
                std::istream is( &inbuf );
                std::ostream os( &outbuf );
                bool ok = 0 == zmolly_encode( is, os, zmolly_blocksize );
                os.flush();
                if (ok) {
                    *outlen = outbuf.position();
                }
        }
#endif
#ifndef BUNDLE_NO_TANGELO
        break; case BUNDLE_TANGELO: {
                membuf inbuf((const char *)in, inlen);
                membuf outbuf((char *)out, *outlen);
                std::istream is( &inbuf );
                std::ostream os( &outbuf );
                tangelo::Encoder<1> en(is, os);
                for( int i = 0; i < inlen; i++ ) {
                    en.compress(is.get());
                }
                en.flush();
                *outlen = outbuf.position();
        }
#endif
#ifndef BUNDLE_NO_BCM
        break; case BUNDLE_BCM: {
                membuf inbuf((const char *)in, inlen);
                membuf outbuf((char *)out, *outlen);
                std::istream is( &inbuf );
                std::ostream os( &outbuf );
                if( bcm::compress(is, os, inlen) ) {
                    *outlen = outbuf.position();
                }
        }
#endif
#ifndef BUNDLE_NO_MCM
        break; case BUNDLE_MCM: {
                static const bool crt0 = bundle::init_mcm();
                ReadMemoryStream is((byte*)in, (byte*)in+inlen);
                WriteMemoryStream os((byte*)out);
                CompressionOptions options;
                /*
                options.comp_level_ = CompressionOptions::kCompLevelMid; //kModeCompress;
                options.mem_usage_ = CompressionOptions::kDefaultMemUsage; //6;
                options.lzp_type_ = CompressionOptions::kLZPTypeAuto;
                options.filter_type_ = CompressionOptions::kFilterTypeAuto;
                */
                {
                    Archive archive(&os, options);
                    archive.compress(&is);
                }
                *outlen = os.tell();
        }
#endif
#ifndef BUNDLE_NO_CRUSH
        break; case BUNDLE_CRUSH: {
            *outlen = crush::compress(0, (uint8_t*)in, int(inlen), (uint8_t*)out);
        }
#endif
#ifndef BUNDLE_NO_LZJB
        break; case BUNDLE_LZJB: {
            *outlen = lzjb_compress2010((uint8_t *)in, (uint8_t *)out, inlen, *outlen, 0);
        }
#endif
#ifndef BUNDLE_NO_BZIP2
        break; case BUNDLE_BZIP2: {
            unsigned int o(*outlen);
            if( BZ_OK == BZ2_bzBuffToBuffCompress( (char *)out, &o, (char *)in, inlen, 9 /*level*/, 0 /*verbosity*/, 30 /*default*/ ) ) {
                *outlen = o;
            } else {
                *outlen = 0;
            }
        }
#endif
#if 0
            // for archival purposes:
            break; case BUNDLE_YAPPY: *outlen = Yappy_Compress( (const unsigned char *)in, (unsigned char *)out, inlen ) - out;
            break; case BUNDLE_BLOSC: { int clevel = 9, doshuffle = 0, typesize = 1;
                int r = blosc_compress( clevel, doshuffle, typesize, inlen, in, out, *outlen);
                if( r <= 0 ) *outlen = 0; else *outlen = r; }
            break; case BUNDLE_FSE: *outlen = FSE_compress( out, (const unsigned char *)in, inlen ); if( *outlen < 0 ) *outlen = 0;
            break; case BUNDLE_LZFX: if( lzfx_compress( in, inlen, out, outlen ) < 0 ) *outlen = 0;
            break; case BUNDLE_LZP1: *outlen = lzp_compress( (const uint8_t *)in, inlen, (uint8_t *)out, *outlen );
            break; case BUNDLE_LZHAM: {
                    // lzham_z_ulong l; lzham_z_compress2( (unsigned char *)out, &l, (const unsigned char *)in, inlen, LZHAM_Z_BEST_COMPRESSION ); *outlen = l;

                    lzham_compress_params comp_params = {0};
                    comp_params.m_struct_size = sizeof(comp_params);
                    comp_params.m_dict_size_log2 = 23;
                    comp_params.m_level = static_cast<lzham_compress_level>(0);
                    comp_params.m_max_helper_threads = 1;
                    comp_params.m_compress_flags |= LZHAM_COMP_FLAG_FORCE_POLAR_CODING;

                    lzham_compress_status_t comp_status =
                    lzham_lib_compress_memory(&comp_params, (lzham_uint8 *)out, outlen, (const lzham_uint8*)in, inlen, 0 );

                    if (comp_status != LZHAM_COMP_STATUS_SUCCESS) {
                        *outlen = 0;
                    }
               }
#endif
        }
        // std::cout << name_of( type_of( out, *outlen ) ) << std::endl;
    }
    ok = ok && *outlen > 0 && *outlen < inlen;
    *outlen = ok && *outlen ? *outlen : 0;
    return ok;
  }

//BUNDLE_API
bool bundle_unpack( unsigned q, const void *in, size_t inlen, void *out, size_t *outlen ) {
    static const bool crt0 = bundle::init();
    bool ok = false;
    size_t bytes_read = 0;
    if( in && inlen && out && *outlen ) {
        ok = true;
        switch( q ) {
            break; default: ok = false;
            break; case BUNDLE_RAW: memcpy( out, in, bytes_read = *outlen = inlen ); 
#ifndef BUNDLE_NO_LZ4
            break; case BUNDLE_LZ4F: case BUNDLE_LZ4: if( LZ4_decompress_safe( (const char *)in, (char *)out, inlen, *outlen ) >= 0 ) bytes_read = inlen; // faster: bytes_read = LZ4_uncompress( (const char *)in, (char *)out, *outlen );
#endif
#ifndef BUNDLE_NO_MINIZ
            break; case BUNDLE_MINIZ: if( TINFL_DECOMPRESS_MEM_TO_MEM_FAILED != tinfl_decompress_mem_to_mem( out, *outlen, in, inlen, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF ) ) bytes_read = inlen;
#endif
#ifndef BUNDLE_NO_SHOCO
            break; case BUNDLE_SHOCO: bytes_read = shoco_decompress( (const char *)in, inlen, (char *)out, *outlen ) == *outlen ? inlen : 0;
#endif
#ifndef BUNDLE_NO_LZMA
            break; case BUNDLE_LZMA20: case BUNDLE_LZMA25: {
                    size_t inlen2 = inlen - LZMA_PROPS_SIZE - 8;
                    if( SZ_OK == LzmaUncompress((unsigned char *)out, outlen, (unsigned char *)in + LZMA_PROPS_SIZE + 8, &inlen2, (unsigned char *)in, LZMA_PROPS_SIZE) ) {
                        bytes_read = inlen;
                    }
            }
#endif
#ifndef BUNDLE_NO_LZIP
            break; case BUNDLE_LZIP: if( lzma_decompress<1>( (const uint8_t *)in, inlen, (uint8_t *)out, outlen ) ) bytes_read = inlen;
#endif
#ifndef BUNDLE_NO_ZPAQ
            break; case BUNDLE_ZPAQ: if( zpaq_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, outlen ) ) bytes_read = inlen;
#endif
#ifndef BUNDLE_NO_BROTLI
            break; case BUNDLE_BROTLI9: case BUNDLE_BROTLI11: if( 1 == BrotliDecompressBuffer(inlen, (const uint8_t *)in, outlen, (uint8_t *)out ) ) bytes_read = inlen;
#endif
#ifndef BUNDLE_NO_ZSTD
            break; case BUNDLE_ZSTD:  bytes_read = ZSTD_decompress( out, *outlen, in, inlen ); if( !ZSTD_isError(bytes_read) ) bytes_read = inlen;
            break; case BUNDLE_ZSTDF: bytes_read = ZSTD_decompress( out, *outlen, in, inlen ); if( !ZSTD_isError(bytes_read) ) bytes_read = inlen;
#endif
#ifndef BUNDLE_NO_BSC
            break; case BUNDLE_BSC: {
                static const bool crt0 = bundle::init_bsc();
                bsc_decompress((const unsigned char *)in, inlen, (unsigned char *)out, *outlen, /*LIBBSC_FEATURE_FASTMODE | */0); bytes_read = inlen;
            }
#endif
#ifndef BUNDLE_NO_SHRINKER
            break; case BUNDLE_SHRINKER: bytes_read = shrinker_decompress((void *)in, out, *outlen); bytes_read = -1 == int(bytes_read) ? 0 : inlen;
#endif
#ifndef BUNDLE_NO_CSC
            break; case BUNDLE_CSC20: {
                CSCSeqStream isss, osss;
                isss.rd = new rdbuf( (const uint8_t *)in, inlen );
                isss.is.Read = csc_read;
                osss.wr = new wrbuf( (uint8_t *)out, *outlen );
                osss.os.Write = csc_write;

                CSCProps p;
                unsigned char buf[CSC_PROP_SIZE];
                isss.rd->readbuf( buf, CSC_PROP_SIZE );
                CSCDec_ReadProperties(&p, buf);
                CSCDecHandle h = CSCDec_Create(&p, (ISeqInStream*)&isss, 0);
                CSCDec_Decode(h, (ISeqOutStream*)&osss, 0);
                CSCDec_Destroy(h);

                bytes_read = isss.rd->pos;
                delete isss.rd, isss.rd = 0;
                delete osss.wr, osss.wr = 0;
            }
#endif
#ifndef BUNDLE_NO_ZLING
        break; case BUNDLE_ZLING: {
            //try {
                zling::MemInputter  inputter((const unsigned char *)in, inlen);
                zling::MemOutputter outputter((unsigned char *)out, *outlen);
                zling::ActionHandler handler;
                bool ok = 0 == baidu::zling::Decode(&inputter, &outputter, &handler);
                if( ok ) bytes_read = inlen;
            /*} catch (const std::runtime_error& e) {
                fprintf(stderr, "zling: runtime error: %s\n", e.what());
            } catch (const std::bad_alloc& e) {
                fprintf(stderr, "zling: allocation failed.");
            }*/
        }
#endif
#ifndef BUNDLE_NO_ZMOLLY
        break; case BUNDLE_ZMOLLY: {
                membuf inbuf((const char *)in, inlen);
                membuf outbuf((char *)out, *outlen);
                std::istream is( &inbuf );
                std::ostream os( &outbuf );
                bool ok = 0 == zmolly_decode(is, os);
                os.flush();
                if( ok ) {
                    bytes_read = inlen;
                }
        }
#endif
#ifndef BUNDLE_NO_TANGELO
        break; case BUNDLE_TANGELO: {
                membuf inbuf((const char *)in, inlen);
                membuf outbuf((char *)out, *outlen);
                std::istream is( &inbuf );
                std::ostream os( &outbuf );
                tangelo::Encoder<0> en( is, os );
                for( int i = 0; i < *outlen; i++ ) {
                    os.put( en.decompress() );
                }
                bytes_read = inlen;
        }
#endif
#ifndef BUNDLE_NO_BCM
        break; case BUNDLE_BCM: {
                membuf inbuf((const char *)in, inlen);
                membuf outbuf((char *)out, *outlen);
                std::istream is( &inbuf );
                std::ostream os( &outbuf );
                if( bcm::decompress(is, os) ) {
                    bytes_read = inlen;
                }
        }
#endif
#ifndef BUNDLE_NO_MCM
        break; case BUNDLE_MCM: {
                static const bool crt0 = bundle::init_mcm();
                ReadMemoryStream is((byte*)in, (byte*)in+inlen);
                WriteMemoryStream os((byte*)out);
                Archive archive(&is);
                const auto& header = archive.getHeader();
                if ( header.isArchive() && header.isSameVersion()) {
                    archive.decompress(&os);
                    bytes_read = inlen;
                }
        }
#endif
#ifndef BUNDLE_NO_CRUSH
        break; case BUNDLE_CRUSH: {
                bytes_read = *outlen == crush::decompress( (uint8_t*)in, (uint8_t*)out, int(*outlen) ) ? inlen : 0;
        }
#endif
#ifndef BUNDLE_NO_LZJB
        break; case BUNDLE_LZJB: {
                bytes_read = *outlen == lzjb_decompress2010( (uint8_t*)in, (uint8_t*)out, inlen, *outlen, 0) ? inlen : 0;
        }
#endif
#ifndef BUNDLE_NO_BZIP2
        break; case BUNDLE_BZIP2: { 
                unsigned int o(*outlen); 
                if( BZ_OK == BZ2_bzBuffToBuffDecompress( (char *)out, &o, (char *)in, inlen, 0 /*fast*/, 0 /*verbosity*/ ) ) { 
                    bytes_read = inlen;
                }
        }
#endif
#if 0
            // for archival purposes:
            break; case BUNDLE_EASYLZMA: if( lzma_decompress<0>( (const uint8_t *)in, inlen, (uint8_t *)out, outlen ) ) bytes_read = inlen;
            break; case BUNDLE_YAPPY: Yappy_UnCompress( (const unsigned char *)in, ((const unsigned char *)in) + inlen, (unsigned char *)out ); bytes_read = inlen;
            break; case BUNDLE_BLOSC: if( blosc_decompress( in, out, *outlen ) > 0 ) bytes_read = inlen;
            break; case BUNDLE_FSE: { int r = FSE_decompress( (unsigned char*)out, *outlen, (const void *)in ); if( r >= 0 ) bytes_read = r; }
            break; case BUNDLE_LZFX: if( lzfx_decompress( in, inlen, out, &outlen ) >= 0 ) bytes_read = inlen;
            break; case BUNDLE_LZP1: lzp_decompress( (const uint8_t *)in, inlen, (uint8_t *)out, *outlen ); bytes_read = inlen;
            break; case BUNDLE_LZHAM: {
                //bytes_read = inlen; { lzham_z_ulong l = *outlen; lzham_z_uncompress( (unsigned char *)out, &l, (const unsigned char *)in, inlen ); }

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
    *outlen = ok && *outlen ? *outlen : 0;
    return ok;
}

}
