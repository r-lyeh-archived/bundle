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

static bool LZd_verify_trailer( struct LZ_decoder * const decoder )
  {
  File_trailer trailer;
  const unsigned long long member_size =
    decoder->rdec->member_position + Ft_size;

  int size = Rd_read_data( decoder->rdec, trailer, Ft_size );
  if( size < Ft_size )
    return false;

  return ( decoder->rdec->code == 0 &&
           Ft_get_data_crc( trailer ) == LZd_crc( decoder ) &&
           Ft_get_data_size( trailer ) == LZd_data_position( decoder ) &&
           Ft_get_member_size( trailer ) == member_size );
  }


/* Return value: 0 = OK, 1 = decoder error, 2 = unexpected EOF,
                 3 = trailer error, 4 = unknown marker found. */
static int LZd_decode_member( struct LZ_decoder * const decoder )
  {
  struct Range_decoder * const rdec = decoder->rdec;
  State * const state = &decoder->state;

  if( decoder->member_finished ) return 0;
  if( !Rd_try_reload( rdec, false ) ) return 0;
  if( decoder->verify_trailer_pending )
    {
    if( Rd_available_bytes( rdec ) < Ft_size && !rdec->at_stream_end )
      return 0;
    decoder->verify_trailer_pending = false;
    decoder->member_finished = true;
    if( LZd_verify_trailer( decoder ) ) return 0; else return 3;
    }

  while( !Rd_finished( rdec ) )
    {
    const int pos_state = LZd_data_position( decoder ) & pos_state_mask;
    if( !Rd_enough_available_bytes( rdec ) ||
        !LZd_enough_free_bytes( decoder ) )
      return 0;
    if( Rd_decode_bit( rdec, &decoder->bm_match[*state][pos_state] ) == 0 )	/* 1st bit */
      {
      const uint8_t prev_byte = LZd_get_prev_byte( decoder );
      if( St_is_char( *state ) )
        {
        *state -= ( *state < 4 ) ? *state : 3;
        LZd_put_byte( decoder, Rd_decode_tree( rdec,
                      decoder->bm_literal[get_lit_state(prev_byte)], 8 ) );
        }
      else
        {
        *state -= ( *state < 10 ) ? 3 : 6;
        LZd_put_byte( decoder, Rd_decode_matched( rdec,
                      decoder->bm_literal[get_lit_state(prev_byte)],
                      LZd_get_byte( decoder, decoder->rep0 ) ) );
        }
      }
    else
      {
      int len;
      if( Rd_decode_bit( rdec, &decoder->bm_rep[*state] ) == 1 )	/* 2nd bit */
        {
        if( Rd_decode_bit( rdec, &decoder->bm_rep0[*state] ) == 1 )	/* 3rd bit */
          {
          unsigned distance;
          if( Rd_decode_bit( rdec, &decoder->bm_rep1[*state] ) == 0 )	/* 4th bit */
            distance = decoder->rep1;
          else
            {
            if( Rd_decode_bit( rdec, &decoder->bm_rep2[*state] ) == 0 )	/* 5th bit */
              distance = decoder->rep2;
            else
              { distance = decoder->rep3; decoder->rep3 = decoder->rep2; }
            decoder->rep2 = decoder->rep1;
            }
          decoder->rep1 = decoder->rep0;
          decoder->rep0 = distance;
          }
        else
          {
          if( Rd_decode_bit( rdec, &decoder->bm_len[*state][pos_state] ) == 0 )	/* 4th bit */
            { *state = St_set_short_rep( *state );
              LZd_put_byte( decoder, LZd_get_byte( decoder, decoder->rep0 ) ); continue; }
          }
        *state = St_set_rep( *state );
        len = min_match_len + Rd_decode_len( rdec, &decoder->rep_len_model, pos_state );
        }
      else
        {
        int dis_slot;
        const unsigned rep0_saved = decoder->rep0;
        len = min_match_len + Rd_decode_len( rdec, &decoder->match_len_model, pos_state );
        dis_slot = Rd_decode_tree6( rdec, decoder->bm_dis_slot[get_dis_state(len)] );
        if( dis_slot < start_dis_model ) decoder->rep0 = dis_slot;
        else
          {
          const int direct_bits = ( dis_slot >> 1 ) - 1;
          decoder->rep0 = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
          if( dis_slot < end_dis_model )
            decoder->rep0 += Rd_decode_tree_reversed( rdec,
                             decoder->bm_dis + decoder->rep0 - dis_slot - 1,
                             direct_bits );
          else
            {
            decoder->rep0 += Rd_decode( rdec, direct_bits - dis_align_bits ) << dis_align_bits;
            decoder->rep0 += Rd_decode_tree_reversed4( rdec, decoder->bm_align );
            if( decoder->rep0 == 0xFFFFFFFFU )		/* Marker found */
              {
              decoder->rep0 = rep0_saved;
              Rd_normalize( rdec );
              if( len == min_match_len )	/* End Of Stream marker */
                {
                if( Rd_available_bytes( rdec ) < Ft_size && !rdec->at_stream_end )
                  { decoder->verify_trailer_pending = true; return 0; }
                decoder->member_finished = true;
                if( LZd_verify_trailer( decoder ) ) return 0; else return 3;
                }
              if( len == min_match_len + 1 )	/* Sync Flush marker */
                {
                if( Rd_try_reload( rdec, true ) ) continue;
                else return 0;
                }
              return 4;
              }
            }
          }
        decoder->rep3 = decoder->rep2;
        decoder->rep2 = decoder->rep1; decoder->rep1 = rep0_saved;
        *state = St_set_match( *state );
        if( decoder->rep0 >= (unsigned)decoder->dictionary_size ||
            ( decoder->rep0 >= (unsigned)decoder->cb.put &&
              !decoder->partial_data_pos ) )
          return 1;
        }
      LZd_copy_block( decoder, decoder->rep0, len );
      }
    }
  return 2;
  }
