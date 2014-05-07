/*  Buff to buff example - Test program for the lzlib library
    Copyright (C) 2010, 2011, 2012, 2013 Antonio Diaz Diaz.

    This program is free software: you have unlimited permission
    to copy, distribute and modify it.

    Usage is:
      bbexample filename

    This program is an example of how buffer-to-buffer
    compression/decompression can be implemented using lzlib.
*/

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lzlib.h"


/* Compresses 'size' bytes from 'data'. Returns the address of a
   malloc'd buffer containing the compressed data and its size in
   '*out_sizep'.
   In case of error, returns 0 and does not modify '*out_sizep'.
*/
uint8_t * bbcompress( const uint8_t * const data, const int size,
                      int * const out_sizep )
  {
  struct LZ_Encoder * encoder;
  uint8_t * new_data;
  const int match_len_limit = 36;
  const unsigned long long member_size = INT64_MAX;
  int delta_size, new_data_size;
  int new_pos = 0;
  int written = 0;
  bool error = false;
  int dict_size = 8 << 20;				/* 8 MiB */

  if( dict_size > size ) dict_size = size;		/* saves memory */
  if( dict_size < LZ_min_dictionary_size() )
    dict_size = LZ_min_dictionary_size();
  encoder = LZ_compress_open( dict_size, match_len_limit, member_size );
  if( !encoder || LZ_compress_errno( encoder ) != LZ_ok )
    { LZ_compress_close( encoder ); return 0; }

  delta_size = (size < 256) ? 64 : size / 4;		/* size may be zero */
  new_data_size = delta_size;				/* initial size */
  new_data = (uint8_t *)malloc( new_data_size );
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
      uint8_t * const tmp =
        (uint8_t *)realloc( new_data, new_data_size + delta_size );
      if( !tmp ) { error = true; break; }
      new_data = tmp;
      new_data_size += delta_size;
      }
    }

  if( LZ_compress_close( encoder ) < 0 ) error = true;
  if( error ) { free( new_data ); return 0; }
  *out_sizep = new_pos;
  return new_data;
  }


/* Decompresses 'size' bytes from 'data'. Returns the address of a
   malloc'd buffer containing the decompressed data and its size in
   '*out_sizep'.
   In case of error, returns 0 and does not modify '*out_sizep'.
*/
uint8_t * bbdecompress( const uint8_t * const data, const int size,
                        int * const out_sizep )
  {
  struct LZ_Decoder * const decoder = LZ_decompress_open();
  uint8_t * new_data;
  const int delta_size = size;			/* size must be > zero */
  int new_data_size = delta_size;		/* initial size */
  int new_pos = 0;
  int written = 0;
  bool error = false;
  if( !decoder || LZ_decompress_errno( decoder ) != LZ_ok )
    { LZ_decompress_close( decoder ); return 0; }

  new_data = (uint8_t *)malloc( new_data_size );
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
      uint8_t * const tmp =
        (uint8_t *)realloc( new_data, new_data_size + delta_size );
      if( !tmp ) { error = true; break; }
      new_data = tmp;
      new_data_size += delta_size;
      }
    }

  if( LZ_decompress_close( decoder ) < 0 ) error = true;
  if( error ) { free( new_data ); return 0; }
  *out_sizep = new_pos;
  return new_data;
  }


int main( const int argc, const char * const argv[] )
  {
  FILE * file;
  uint8_t * in_buffer, * mid_buffer, * out_buffer;
  const int in_buffer_size = 1 << 20;
  int in_size, mid_size = 0, out_size = 0;

  if( argc < 2 )
    {
    fprintf( stderr, "Usage: bbexample filename\n" );
    return 1;
    }

  file = fopen( argv[1], "rb" );
  if( !file )
    {
    fprintf( stderr, "bbexample: Can't open file '%s' for reading\n", argv[1] );
    return 1;
    }

  in_buffer = (uint8_t *)malloc( in_buffer_size );
  if( !in_buffer )
    {
    fprintf( stderr, "bbexample: Not enough memory.\n" );
    return 1;
    }

  in_size = fread( in_buffer, 1, in_buffer_size, file );
  if( in_size >= in_buffer_size )
    {
    fprintf( stderr, "bbexample: Input file '%s' is too big.\n", argv[1] );
    return 1;
    }
  fclose( file );

  mid_buffer = bbcompress( in_buffer, in_size, &mid_size );
  if( !mid_buffer )
    {
    fprintf( stderr, "bbexample: Not enough memory or compress error.\n" );
    return 1;
    }

  out_buffer = bbdecompress( mid_buffer, mid_size, &out_size );
  if( !out_buffer )
    {
    fprintf( stderr, "bbexample: Not enough memory or decompress error.\n" );
    return 1;
    }

  if( in_size != out_size ||
      ( in_size > 0 && memcmp( in_buffer, out_buffer, in_size ) != 0 ) )
    {
    fprintf( stderr, "bbexample: Decompressed data differs from original.\n" );
    return 1;
    }

  free( out_buffer );
  free( mid_buffer );
  free( in_buffer );
  return 0;
  }
