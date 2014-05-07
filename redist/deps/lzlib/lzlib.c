/*  Lzlib - Compression library for lzip files
    Copyright (C) 2009, 2010, 2011, 2012, 2013 Antonio Diaz Diaz.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.

    As a special exception, you may use this file as part of a free
    software library without restriction.  Specifically, if other files
    instantiate templates or use macros or inline functions from this
    file, or you compile this file and link it with other files to
    produce an executable, this file does not by itself cause the
    resulting executable to be covered by the GNU General Public
    License.  This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General
    Public License.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lzlib.h"
#include "lzip.h"
#include "cbuffer.c"
#include "decoder.h"
#include "decoder.c"
#include "encoder.h"
#include "encoder.c"


struct LZ_Encoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  struct Matchfinder * matchfinder;
  struct LZ_encoder * lz_encoder;
  enum LZ_Errno lz_errno;
  int flush_pending;
  File_header member_header;
  bool fatal;
  };

static void LZ_Encoder_init( struct LZ_Encoder * const e,
                             const File_header header )
  {
  int i;
  e->partial_in_size = 0;
  e->partial_out_size = 0;
  e->matchfinder = 0;
  e->lz_encoder = 0;
  e->lz_errno = LZ_ok;
  e->flush_pending = 0;
  for( i = 0; i < Fh_size; ++i ) e->member_header[i] = header[i];
  e->fatal = false;
  }


struct LZ_Decoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  struct Range_decoder * rdec;
  struct LZ_decoder * lz_decoder;
  enum LZ_Errno lz_errno;
  File_header member_header;		/* header of current member */
  bool fatal;
  bool seeking;
  };

static void LZ_Decoder_init( struct LZ_Decoder * const d )
  {
  int i;
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  d->rdec = 0;
  d->lz_decoder = 0;
  d->lz_errno = LZ_ok;
  for( i = 0; i < Fh_size; ++i ) d->member_header[i] = 0;
  d->fatal = false;
  d->seeking = false;
  }


static bool verify_encoder( struct LZ_Encoder * const e )
  {
  if( !e ) return false;
  if( !e->matchfinder || !e->lz_encoder )
    { e->lz_errno = LZ_bad_argument; return false; }
  return true;
  }


static bool verify_decoder( struct LZ_Decoder * const d )
  {
  if( !d ) return false;
  if( !d->rdec )
    { d->lz_errno = LZ_bad_argument; return false; }
  return true;
  }


/*------------------------- Misc Functions -------------------------*/

const char * LZ_version( void ) { return LZ_version_string; }


const char * LZ_strerror( const enum LZ_Errno lz_errno )
  {
  switch( lz_errno )
    {
    case LZ_ok            : return "ok";
    case LZ_bad_argument  : return "bad argument";
    case LZ_mem_error     : return "not enough memory";
    case LZ_sequence_error: return "sequence error";
    case LZ_header_error  : return "header error";
    case LZ_unexpected_eof: return "unexpected eof";
    case LZ_data_error    : return "data error";
    case LZ_library_error : return "library error";
    }
  return "invalid error code";
  }


int LZ_min_dictionary_bits( void ) { return min_dictionary_bits; }
int LZ_min_dictionary_size( void ) { return min_dictionary_size; }
int LZ_max_dictionary_bits( void ) { return max_dictionary_bits; }
int LZ_max_dictionary_size( void ) { return max_dictionary_size; }
int LZ_min_match_len_limit( void ) { return min_match_len_limit; }
int LZ_max_match_len_limit( void ) { return max_match_len; }


/*---------------------- Compression Functions ----------------------*/

struct LZ_Encoder * LZ_compress_open( const int dictionary_size,
                                      const int match_len_limit,
                                      const unsigned long long member_size )
  {
  File_header header;
  const bool error = ( !Fh_set_dictionary_size( header, dictionary_size ) ||
                       match_len_limit < min_match_len_limit ||
                       match_len_limit > max_match_len ||
                       member_size < min_dictionary_size );

  struct LZ_Encoder * const e =
    (struct LZ_Encoder *)malloc( sizeof (struct LZ_Encoder) );
  if( !e ) return 0;
  Fh_set_magic( header );
  LZ_Encoder_init( e, header );
  if( error ) e->lz_errno = LZ_bad_argument;
  else
    {
    e->matchfinder = (struct Matchfinder *)malloc( sizeof (struct Matchfinder) );
    if( e->matchfinder )
      {
      e->lz_encoder = (struct LZ_encoder *)malloc( sizeof (struct LZ_encoder) );
      if( e->lz_encoder )
        {
        if( Mf_init( e->matchfinder,
                     Fh_get_dictionary_size( header ), match_len_limit ) )
          {
          if( LZe_init( e->lz_encoder, e->matchfinder, header, member_size ) )
            return e;
          Mf_free( e->matchfinder );
          }
        free( e->lz_encoder ); e->lz_encoder = 0;
        }
      free( e->matchfinder ); e->matchfinder = 0;
      }
    e->lz_errno = LZ_mem_error;
    }
  e->fatal = true;
  return e;
  }


int LZ_compress_close( struct LZ_Encoder * const e )
  {
  if( !e ) return -1;
  if( e->lz_encoder )
    { LZe_free( e->lz_encoder ); free( e->lz_encoder ); }
  if( e->matchfinder )
   { Mf_free( e->matchfinder ); free( e->matchfinder ); }
  free( e );
  return 0;
  }


int LZ_compress_finish( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  Mf_finish( e->matchfinder );
  e->flush_pending = 0;
  /* if (open --> write --> finish) use same dictionary size as lzip. */
  /* this does not save any memory. */
  if( Mf_data_position( e->matchfinder ) == 0 &&
      LZ_compress_total_out_size( e ) == Fh_size )
    Mf_adjust_distionary_size( e->matchfinder );
  return 0;
  }


int LZ_compress_restart_member( struct LZ_Encoder * const e,
                                const unsigned long long member_size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( !LZe_member_finished( e->lz_encoder ) )
    { e->lz_errno = LZ_sequence_error; return -1; }
  if( member_size < min_dictionary_size )
    { e->lz_errno = LZ_bad_argument; return -1; }

  e->partial_in_size += Mf_data_position( e->matchfinder );
  e->partial_out_size += Re_member_position( &e->lz_encoder->renc );
  Mf_reset( e->matchfinder );

  LZe_free( e->lz_encoder );
  if( !LZe_init( e->lz_encoder, e->matchfinder, e->member_header, member_size ) )
    {
    LZe_free( e->lz_encoder ); free( e->lz_encoder ); e->lz_encoder = 0;
    e->lz_errno = LZ_mem_error; e->fatal = true;
    return -1;
    }
  e->lz_errno = LZ_ok;
  return 0;
  }


int LZ_compress_sync_flush( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( e->flush_pending <= 0 && !Mf_flushing_or_end( e->matchfinder ) )
    {
    e->flush_pending = 2;	/* 2 consecutive markers guarantee decoding */
    Mf_set_flushing( e->matchfinder, true );
    if( !LZe_encode_member( e->lz_encoder ) )
      { e->lz_errno = LZ_library_error; e->fatal = true; return -1; }
    while( e->flush_pending > 0 && LZe_sync_flush( e->lz_encoder ) )
      { if( --e->flush_pending <= 0 ) Mf_set_flushing( e->matchfinder, false ); }
    }
  return 0;
  }


int LZ_compress_read( struct LZ_Encoder * const e,
                      uint8_t * const buffer, const int size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( !LZe_encode_member( e->lz_encoder ) )
    { e->lz_errno = LZ_library_error; e->fatal = true; return -1; }
  while( e->flush_pending > 0 && LZe_sync_flush( e->lz_encoder ) )
    { if( --e->flush_pending <= 0 ) Mf_set_flushing( e->matchfinder, false ); }
  return Re_read_data( &e->lz_encoder->renc, buffer, size );
  }


int LZ_compress_write( struct LZ_Encoder * const e,
                       const uint8_t * const buffer, const int size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( e->flush_pending > 0 || size < 0 ) return 0;
  return Mf_write_data( e->matchfinder, buffer, size );
  }


int LZ_compress_write_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( e->flush_pending > 0 ) return 0;
  return Mf_free_bytes( e->matchfinder );
  }


enum LZ_Errno LZ_compress_errno( struct LZ_Encoder * const e )
  {
  if( !e ) return LZ_bad_argument;
  return e->lz_errno;
  }


int LZ_compress_finished( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return -1;
  return ( e->flush_pending <= 0 && Mf_finished( e->matchfinder ) &&
           LZe_member_finished( e->lz_encoder ) );
  }


int LZ_compress_member_finished( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return -1;
  return LZe_member_finished( e->lz_encoder );
  }


unsigned long long LZ_compress_data_position( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return Mf_data_position( e->matchfinder );
  }


unsigned long long LZ_compress_member_position( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return Re_member_position( &e->lz_encoder->renc );
  }


unsigned long long LZ_compress_total_in_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return e->partial_in_size + Mf_data_position( e->matchfinder );
  }


unsigned long long LZ_compress_total_out_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return e->partial_out_size + Re_member_position( &e->lz_encoder->renc );
  }


/*--------------------- Decompression Functions ---------------------*/

struct LZ_Decoder * LZ_decompress_open( void )
  {
  struct LZ_Decoder * const d =
    (struct LZ_Decoder *)malloc( sizeof (struct LZ_Decoder) );
  if( !d ) return 0;
  LZ_Decoder_init( d );

  d->rdec = (struct Range_decoder *)malloc( sizeof (struct Range_decoder) );
  if( !d->rdec || !Rd_init( d->rdec ) )
    {
    if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); d->rdec = 0; }
    d->lz_errno = LZ_mem_error; d->fatal = true;
    }
  return d;
  }


int LZ_decompress_close( struct LZ_Decoder * const d )
  {
  if( !d ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); }
  if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); }
  free( d );
  return 0;
  }


int LZ_decompress_finish( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  if( d->seeking ) { d->seeking = false; Rd_purge( d->rdec ); }
  else Rd_finish( d->rdec );
  return 0;
  }


int LZ_decompress_reset( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  Rd_reset( d->rdec );
  d->lz_errno = LZ_ok;
  d->fatal = false;
  d->seeking = false;
  return 0;
  }


int LZ_decompress_sync_to_member( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  if( Rd_find_header( d->rdec ) ) d->seeking = false;
  else
    {
    if( !d->rdec->at_stream_end ) d->seeking = true;
    else { d->seeking = false; Rd_purge( d->rdec ); }
    }
  d->lz_errno = LZ_ok;
  d->fatal = false;
  return 0;
  }


int LZ_decompress_read( struct LZ_Decoder * const d,
                        uint8_t * const buffer, const int size )
  {
  int result;
  if( !verify_decoder( d ) || d->fatal ) return -1;
  if( d->seeking ) return 0;

  if( d->lz_decoder && LZd_member_finished( d->lz_decoder ) )
    {
    d->partial_in_size += d->rdec->member_position;
    d->partial_out_size += LZd_data_position( d->lz_decoder );
    LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0;
    }
  if( !d->lz_decoder )
    {
    if( Rd_available_bytes( d->rdec ) < 5 + Fh_size )
      {
      if( !d->rdec->at_stream_end || Rd_finished( d->rdec ) ) return 0;
      /* set position at EOF */
      d->partial_in_size += Rd_available_bytes( d->rdec );
      if( Rd_available_bytes( d->rdec ) <= Fh_size ||
          !Rd_read_header( d->rdec, d->member_header ) )
        d->lz_errno = LZ_header_error;
      else
        d->lz_errno = LZ_unexpected_eof;
      d->fatal = true;
      return -1;
      }
    if( !Rd_read_header( d->rdec, d->member_header ) )
      {
      d->lz_errno = LZ_header_error;
      d->fatal = true;
      return -1;
      }
    d->lz_decoder = (struct LZ_decoder *)malloc( sizeof (struct LZ_decoder) );
    if( !d->lz_decoder || !LZd_init( d->lz_decoder, d->member_header, d->rdec ) )
      {					/* not enough free memory */
      if( d->lz_decoder )
        { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
      d->lz_errno = LZ_mem_error;
      d->fatal = true;
      return -1;
      }
    }
  result = LZd_decode_member( d->lz_decoder );
  if( result != 0 )
    {
    if( result == 2 ) d->lz_errno = LZ_unexpected_eof;
    else d->lz_errno = LZ_data_error;
    d->fatal = true;
    return -1;
    }
  return Cb_read_data( &d->lz_decoder->cb, buffer, size );
  }


int LZ_decompress_write( struct LZ_Decoder * const d,
                         const uint8_t * const buffer, const int size )
  {
  int result;
  if( !verify_decoder( d ) || d->fatal ) return -1;

  result = Rd_write_data( d->rdec, buffer, size );
  while( d->seeking )
    {
    int size2;
    if( Rd_find_header( d->rdec ) ) d->seeking = false;
    if( result >= size ) break;
    size2 = Rd_write_data( d->rdec, buffer + result, size - result );
    if( size2 > 0 ) result += size2;
    else break;
    }
  return result;
  }


int LZ_decompress_write_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  return Rd_free_bytes( d->rdec );
  }


enum LZ_Errno LZ_decompress_errno( struct LZ_Decoder * const d )
  {
  if( !d ) return LZ_bad_argument;
  return d->lz_errno;
  }


int LZ_decompress_finished( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return ( Rd_finished( d->rdec ) &&
           ( !d->lz_decoder || LZd_member_finished( d->lz_decoder ) ) );
  }


int LZ_decompress_member_finished( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return ( d->lz_decoder && LZd_member_finished( d->lz_decoder ) );
  }


int LZ_decompress_member_version( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return Fh_version( d->member_header );
  }


int LZ_decompress_dictionary_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return Fh_get_dictionary_size( d->member_header );
  }


unsigned LZ_decompress_data_crc( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
    return LZd_crc( d->lz_decoder );
  return 0;
  }


unsigned long long LZ_decompress_data_position( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
    return LZd_data_position( d->lz_decoder );
  return 0;
  }


unsigned long long LZ_decompress_member_position( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
    return d->rdec->member_position;
  return 0;
  }


unsigned long long LZ_decompress_total_in_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return 0;
  if( d->lz_decoder )
    return d->partial_in_size + d->rdec->member_position;
  return d->partial_in_size;
  }


unsigned long long LZ_decompress_total_out_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return 0;
  if( d->lz_decoder )
    return d->partial_out_size + LZd_data_position( d->lz_decoder );
  return d->partial_out_size;
  }
