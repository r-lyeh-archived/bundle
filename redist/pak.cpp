// simple compression interface
// - rlyeh. mit licensed

#include <cassert>
#include <cstdio>  // std::sprintf
#include <cctype>  // std::isprint

#include <iostream>
#include <vector>
#include <string>

#ifdef BUNDLE_REDIST
#include \
"pak.hpp"
#else
#include \
"bundle.hpp"
#endif

//#include "deps/miniz/miniz.h"

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

    std::string pak::bin() const
    {
        std::string result;

        if( type == paktype::ZIP )
        {
            mz_zip_archive zip_archive;
            memset( &zip_archive, 0, sizeof(zip_archive) );

            mz_bool status = mz_zip_writer_init_heap( &zip_archive, 0, 128 * 1024 );

            if( !status ) {
                assert( status );
                return "mz_zip_writer_init_heap() failed!", std::string();
            }

            for( const_iterator it = this->begin(), end = this->end(); it != end; ++it ) {
                auto filename = it->find("filename");
                auto content = it->find("content");
                if( filename != it->end() && content != it->end() ) {
                    const size_t bufsize = content->second.size();

                    status = mz_zip_writer_add_mem( &zip_archive, filename->second.c_str(), content->second.c_str(), bufsize, MZ_DEFAULT_COMPRESSION );
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

