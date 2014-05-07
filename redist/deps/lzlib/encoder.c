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

static bool Mf_normalize_pos( struct Matchfinder * const mf )
  {
  if( mf->pos > mf->stream_pos )
    { mf->pos = mf->stream_pos; return false; }
  if( !mf->at_stream_end )
    {
    int i;
    const int offset = mf->pos - mf->dictionary_size - before_size;
    const int size = mf->stream_pos - offset;
    memmove( mf->buffer, mf->buffer + offset, size );
    mf->partial_data_pos += offset;
    mf->pos -= offset;
    mf->stream_pos -= offset;
    for( i = 0; i < mf->num_prev_positions; ++i )
      if( mf->prev_positions[i] >= 0 ) mf->prev_positions[i] -= offset;
    for( i = 0; i < 2 * ( mf->dictionary_size + 1 ); ++i )
      if( mf->prev_pos_tree[i] >= 0 ) mf->prev_pos_tree[i] -= offset;
    }
  return true;
  }


static bool Mf_init( struct Matchfinder * const mf,
                     const int dict_size, const int match_len_limit )
  {
  const int buffer_size_limit = ( 2 * dict_size ) + before_size + after_size;
  unsigned size;
  int i;

  mf->partial_data_pos = 0;
  mf->match_len_limit = match_len_limit;
  mf->pos = 0;
  mf->cyclic_pos = 0;
  mf->stream_pos = 0;
  mf->cycles = ( match_len_limit < max_match_len ) ?
               16 + ( match_len_limit / 2 ) : 256;
  mf->at_stream_end = false;
  mf->been_flushed = false;
  mf->flushing = false;

  mf->buffer_size = max( 65536, buffer_size_limit );
  mf->buffer = (uint8_t *)malloc( mf->buffer_size );
  if( !mf->buffer ) return false;
  mf->dictionary_size = dict_size;
  mf->pos_limit = mf->buffer_size - after_size;
  size = 1 << max( 16, real_bits( mf->dictionary_size - 1 ) - 2 );
  if( mf->dictionary_size > 1 << 26 )		/* 64 MiB */
    size >>= 1;
  mf->key4_mask = size - 1;
  size += num_prev_positions2;
  size += num_prev_positions3;

  mf->num_prev_positions = size;
  size += ( 2 * ( mf->dictionary_size + 1 ) );
  if( size * sizeof (int32_t) <= size ) mf->prev_positions = 0;
  else mf->prev_positions = (int32_t *)malloc( size * sizeof (int32_t) );
  if( !mf->prev_positions ) { free( mf->buffer ); return false; }
  mf->prev_pos_tree = mf->prev_positions + mf->num_prev_positions;
  for( i = 0; i < mf->num_prev_positions; ++i ) mf->prev_positions[i] = -1;
  return true;
  }


static void Mf_adjust_distionary_size( struct Matchfinder * const mf )
  {
  if( mf->stream_pos < mf->dictionary_size )
    {
    int size;
    mf->buffer_size =
    mf->dictionary_size =
    mf->pos_limit = max( min_dictionary_size, mf->stream_pos );
    size = 1 << max( 16, real_bits( mf->dictionary_size - 1 ) - 2 );
    if( mf->dictionary_size > 1 << 26 )
      size >>= 1;
    mf->key4_mask = size - 1;
    size += num_prev_positions2;
    size += num_prev_positions3;
    mf->num_prev_positions = size;
    mf->prev_pos_tree = mf->prev_positions + mf->num_prev_positions;
    }
  }


static void Mf_reset( struct Matchfinder * const mf )
  {
  int i;
  const int size = mf->stream_pos - mf->pos;
  if( size > 0 ) memmove( mf->buffer, mf->buffer + mf->pos, size );
  mf->partial_data_pos = 0;
  mf->stream_pos -= mf->pos;
  mf->pos = 0;
  mf->cyclic_pos = 0;
  mf->at_stream_end = false;
  mf->been_flushed = false;
  mf->flushing = false;
  for( i = 0; i < mf->num_prev_positions; ++i ) mf->prev_positions[i] = -1;
  }


static int Mf_get_match_pairs( struct Matchfinder * const mf, struct Pair * pairs )
  {
  int32_t * ptr0 = mf->prev_pos_tree + ( mf->cyclic_pos << 1 );
  int32_t * ptr1 = ptr0 + 1;
  int32_t * newptr;
  int len = 0, len0 = 0, len1 = 0;
  int maxlen = min_match_len - 1;
  int num_pairs = 0;
  const int min_pos = (mf->pos > mf->dictionary_size) ?
                       mf->pos - mf->dictionary_size : 0;
  const uint8_t * const data = mf->buffer + mf->pos;
  int count, delta, key2, key3, key4, newpos;
  unsigned tmp;
  int len_limit = mf->match_len_limit;

  if( len_limit > Mf_available_bytes( mf ) )
    {
    mf->been_flushed = true;
    len_limit = Mf_available_bytes( mf );
    if( len_limit < 4 ) { *ptr0 = *ptr1 = -1; return 0; }
    }

  tmp = crc32[data[0]] ^ data[1];
  key2 = tmp & ( num_prev_positions2 - 1 );
  tmp ^= (uint32_t)data[2] << 8;
  key3 = num_prev_positions2 + ( tmp & ( num_prev_positions3 - 1 ) );
  key4 = num_prev_positions2 + num_prev_positions3 +
         ( ( tmp ^ ( crc32[data[3]] << 5 ) ) & mf->key4_mask );

  if( pairs )
    {
    int np2 = mf->prev_positions[key2];
    int np3 = mf->prev_positions[key3];
    if( np2 >= min_pos && mf->buffer[np2] == data[0] )
      {
      pairs[0].dis = mf->pos - np2 - 1;
      pairs[0].len = maxlen = 2;
      num_pairs = 1;
      }
    if( np2 != np3 && np3 >= min_pos && mf->buffer[np3] == data[0] )
      {
      maxlen = 3;
      pairs[num_pairs].dis = mf->pos - np3 - 1;
      ++num_pairs;
      np2 = np3;
      }
    if( num_pairs > 0 )
      {
      delta = mf->pos - np2;
      while( maxlen < len_limit && data[maxlen-delta] == data[maxlen] )
        ++maxlen;
      pairs[num_pairs-1].len = maxlen;
      if( maxlen >= len_limit ) pairs = 0;
      }
    if( maxlen < 3 ) maxlen = 3;
    }

  mf->prev_positions[key2] = mf->pos;
  mf->prev_positions[key3] = mf->pos;
  newpos = mf->prev_positions[key4];
  mf->prev_positions[key4] = mf->pos;

  for( count = mf->cycles; ; )
    {
    if( newpos < min_pos || --count < 0 ) { *ptr0 = *ptr1 = -1; break; }

    if( mf->been_flushed ) len = 0;
    delta = mf->pos - newpos;
    newptr = mf->prev_pos_tree +
      ( ( mf->cyclic_pos - delta +
          ( (mf->cyclic_pos >= delta) ? 0 : mf->dictionary_size + 1 ) ) << 1 );
    if( data[len-delta] == data[len] )
      {
      while( ++len < len_limit && data[len-delta] == data[len] ) {}
      if( pairs && maxlen < len )
        {
        pairs[num_pairs].dis = delta - 1;
        pairs[num_pairs].len = maxlen = len;
        ++num_pairs;
        }
      if( len >= len_limit )
        {
        *ptr0 = newptr[0];
        *ptr1 = newptr[1];
        break;
        }
      }
    if( data[len-delta] < data[len] )
      {
      *ptr0 = newpos;
      ptr0 = newptr + 1;
      newpos = *ptr0;
      len0 = len; if( len1 < len ) len = len1;
      }
    else
      {
      *ptr1 = newpos;
      ptr1 = newptr;
      newpos = *ptr1;
      len1 = len; if( len0 < len ) len = len0;
      }
    }
  return num_pairs;
  }


static void Lee_encode( struct Len_encoder * const len_encoder,
                        struct Range_encoder * const renc,
                        int symbol, const int pos_state )
  {
  symbol -= min_match_len;
  if( symbol < len_low_symbols )
    {
    Re_encode_bit( renc, &len_encoder->lm.choice1, 0 );
    Re_encode_tree( renc, len_encoder->lm.bm_low[pos_state], symbol, len_low_bits );
    }
  else
    {
    Re_encode_bit( renc, &len_encoder->lm.choice1, 1 );
    if( symbol < len_low_symbols + len_mid_symbols )
      {
      Re_encode_bit( renc, &len_encoder->lm.choice2, 0 );
      Re_encode_tree( renc, len_encoder->lm.bm_mid[pos_state],
                      symbol - len_low_symbols, len_mid_bits );
      }
    else
      {
      Re_encode_bit( renc, &len_encoder->lm.choice2, 1 );
      Re_encode_tree( renc, len_encoder->lm.bm_high,
                      symbol - len_low_symbols - len_mid_symbols, len_high_bits );
      }
    }
  if( --len_encoder->counters[pos_state] <= 0 )
    Lee_update_prices( len_encoder, pos_state );
  }


     /* End Of Stream mark => (dis == 0xFFFFFFFFU, len == min_match_len) */
static bool LZe_full_flush( struct LZ_encoder * const encoder, const State state )
  {
  int i;
  const int pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
  File_trailer trailer;
  if( encoder->member_finished ||
      Cb_free_bytes( &encoder->renc.cb ) < max_marker_size + Ft_size )
    return false;
  Re_encode_bit( &encoder->renc, &encoder->bm_match[state][pos_state], 1 );
  Re_encode_bit( &encoder->renc, &encoder->bm_rep[state], 0 );
  LZe_encode_pair( encoder, 0xFFFFFFFFU, min_match_len, pos_state );
  Re_flush( &encoder->renc );
  Ft_set_data_crc( trailer, LZe_crc( encoder ) );
  Ft_set_data_size( trailer, Mf_data_position( encoder->matchfinder ) );
  Ft_set_member_size( trailer, Re_member_position( &encoder->renc ) + Ft_size );
  for( i = 0; i < Ft_size; ++i )
    Cb_put_byte( &encoder->renc.cb, trailer[i] );
  return true;
  }


     /* Sync Flush mark => (dis == 0xFFFFFFFFU, len == min_match_len + 1) */
static bool LZe_sync_flush( struct LZ_encoder * const encoder )
  {
  const int pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
  const State state = encoder->state;
  if( encoder->member_finished ||
      Cb_free_bytes( &encoder->renc.cb ) < max_marker_size )
    return false;
  Re_encode_bit( &encoder->renc, &encoder->bm_match[state][pos_state], 1 );
  Re_encode_bit( &encoder->renc, &encoder->bm_rep[state], 0 );
  LZe_encode_pair( encoder, 0xFFFFFFFFU, min_match_len + 1, pos_state );
  Re_flush( &encoder->renc );
  return true;
  }


static void LZe_fill_align_prices( struct LZ_encoder * const encoder )
  {
  int i;
  for( i = 0; i < dis_align_size; ++i )
    encoder->align_prices[i] =
      price_symbol_reversed( encoder->bm_align, i, dis_align_bits );
  encoder->align_price_count = dis_align_size;
  }


static void LZe_fill_distance_prices( struct LZ_encoder * const encoder )
  {
  int dis, dis_state;
  for( dis = start_dis_model; dis < modeled_distances; ++dis )
    {
    const int dis_slot = dis_slots[dis];
    const int direct_bits = ( dis_slot >> 1 ) - 1;
    const int base = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
    const int price =
      price_symbol_reversed( encoder->bm_dis + base - dis_slot - 1,
                             dis - base, direct_bits );
    for( dis_state = 0; dis_state < dis_states; ++dis_state )
      encoder->dis_prices[dis_state][dis] = price;
    }

  for( dis_state = 0; dis_state < dis_states; ++dis_state )
    {
    int * const dsp = encoder->dis_slot_prices[dis_state];
    int * const dp = encoder->dis_prices[dis_state];
    const Bit_model * const bmds = encoder->bm_dis_slot[dis_state];
    int slot = 0;
    for( ; slot < end_dis_model && slot < encoder->num_dis_slots; ++slot )
      dsp[slot] = price_symbol( bmds, slot, dis_slot_bits );
    for( ; slot < encoder->num_dis_slots; ++slot )
      dsp[slot] = price_symbol( bmds, slot, dis_slot_bits ) +
                  (((( slot >> 1 ) - 1 ) - dis_align_bits ) << price_shift_bits );

    for( dis = 0; dis < start_dis_model; ++dis )
      dp[dis] = dsp[dis];
    for( ; dis < modeled_distances; ++dis )
      dp[dis] += dsp[dis_slots[dis]];
    }
  }


static bool LZe_init( struct LZ_encoder * const encoder,
                      struct Matchfinder * const mf,
                      const File_header header,
                      const unsigned long long member_size )
  {
  int i;
  encoder->member_size_limit = member_size - Ft_size - max_marker_size;
  encoder->pending_num_pairs = 0;
  encoder->crc = 0xFFFFFFFFU;

  Bm_array_init( encoder->bm_literal[0], (1 << literal_context_bits) * 0x300 );
  Bm_array_init( encoder->bm_match[0], states * pos_states );
  Bm_array_init( encoder->bm_rep, states );
  Bm_array_init( encoder->bm_rep0, states );
  Bm_array_init( encoder->bm_rep1, states );
  Bm_array_init( encoder->bm_rep2, states );
  Bm_array_init( encoder->bm_len[0], states * pos_states );
  Bm_array_init( encoder->bm_dis_slot[0], dis_states * (1 << dis_slot_bits) );
  Bm_array_init( encoder->bm_dis, modeled_distances - end_dis_model );
  Bm_array_init( encoder->bm_align, dis_align_size );

  encoder->matchfinder = mf;
  if( !Re_init( &encoder->renc ) ) return false;
  Lee_init( &encoder->match_len_encoder, encoder->matchfinder->match_len_limit );
  Lee_init( &encoder->rep_len_encoder, encoder->matchfinder->match_len_limit );
  encoder->num_dis_slots =
    2 * real_bits( encoder->matchfinder->dictionary_size - 1 );

  for( i = 0; i < num_rep_distances; ++i ) encoder->rep_distances[i] = 0;
  encoder->align_price_count = 0;
  encoder->fill_counter = 0;
  encoder->state = 0;
  encoder->member_finished = false;

  for( i = 0; i < Fh_size; ++i )
    Cb_put_byte( &encoder->renc.cb, header[i] );
  return true;
  }


/* Return value == number of bytes advanced (ahead).
   trials[0]..trials[ahead-1] contain the steps to encode.
   ( trials[0].dis == -1 && trials[0].price == 1 ) means literal.
*/
static int LZe_sequence_optimizer( struct LZ_encoder * const encoder,
                                   const int reps[num_rep_distances],
                                   const State state )
  {
  int main_len, num_pairs, i, rep, cur = 0, num_trials, len;
  int replens[num_rep_distances];
  int rep_index = 0;

  if( encoder->pending_num_pairs > 0 )		/* from previous call */
    {
    num_pairs = encoder->pending_num_pairs;
    encoder->pending_num_pairs = 0;
    }
  else
    num_pairs = LZe_read_match_distances( encoder );
  main_len = ( num_pairs > 0 ) ? encoder->pairs[num_pairs-1].len : 0;

  for( i = 0; i < num_rep_distances; ++i )
    {
    replens[i] =
      Mf_true_match_len( encoder->matchfinder, 0, reps[i] + 1, max_match_len );
    if( replens[i] > replens[rep_index] ) rep_index = i;
    }
  if( replens[rep_index] >= encoder->matchfinder->match_len_limit )
    {
    encoder->trials[0].dis = rep_index;
    encoder->trials[0].price = replens[rep_index];
    if( !LZe_move_pos( encoder, replens[rep_index] ) ) return 0;
    return replens[rep_index];
    }

  if( main_len >= encoder->matchfinder->match_len_limit )
    {
    encoder->trials[0].dis = encoder->pairs[num_pairs-1].dis + num_rep_distances;
    encoder->trials[0].price = main_len;
    if( !LZe_move_pos( encoder, main_len ) ) return 0;
    return main_len;
    }

  {
  const int pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
  const int match_price = price1( encoder->bm_match[state][pos_state] );
  const int rep_match_price = match_price + price1( encoder->bm_rep[state] );
  const uint8_t prev_byte = Mf_peek( encoder->matchfinder, -1 );
  const uint8_t cur_byte = Mf_peek( encoder->matchfinder, 0 );
  const uint8_t match_byte = Mf_peek( encoder->matchfinder, -reps[0]-1 );

  encoder->trials[0].state = state;
  encoder->trials[1].dis = -1;
  encoder->trials[1].price = price0( encoder->bm_match[state][pos_state] );
  if( St_is_char( state ) )
    encoder->trials[1].price +=
      LZe_price_literal( encoder, prev_byte, cur_byte );
  else
    encoder->trials[1].price +=
      LZe_price_matched( encoder, prev_byte, cur_byte, match_byte );

  if( match_byte == cur_byte )
    Tr_update( &encoder->trials[1], rep_match_price +
               LZe_price_rep_len1( encoder, state, pos_state ), 0, 0 );

  num_trials = max( main_len, replens[rep_index] );

  if( num_trials < min_match_len )
    {
    encoder->trials[0].dis = encoder->trials[1].dis;
    encoder->trials[0].price = 1;
    if( !Mf_move_pos( encoder->matchfinder ) ) return 0;
    return 1;
    }

  for( i = 0; i < num_rep_distances; ++i )
    encoder->trials[0].reps[i] = reps[i];
  encoder->trials[1].prev_index = 0;
  encoder->trials[1].prev_index2 = single_step_trial;

  for( len = min_match_len; len <= num_trials; ++len )
    encoder->trials[len].price = infinite_price;

  for( rep = 0; rep < num_rep_distances; ++rep )
    {
    int price;
    if( replens[rep] < min_match_len ) continue;
    price = rep_match_price + LZe_price_rep( encoder, rep, state, pos_state );

    for( len = min_match_len; len <= replens[rep]; ++len )
      Tr_update( &encoder->trials[len], price +
                 Lee_price( &encoder->rep_len_encoder, len, pos_state ),
                 rep, 0 );
    }

  if( main_len > replens[0] )
    {
    const int normal_match_price = match_price + price0( encoder->bm_rep[state] );
    i = 0, len = max( replens[0] + 1, min_match_len );
    while( len > encoder->pairs[i].len ) ++i;
    while( true )
      {
      const int dis = encoder->pairs[i].dis;
      Tr_update( &encoder->trials[len], normal_match_price +
                 LZe_price_pair( encoder, dis, len, pos_state ),
                 dis + num_rep_distances, 0 );
      if( ++len > encoder->pairs[i].len && ++i >= num_pairs ) break;
      }
    }
  }

  if( !Mf_move_pos( encoder->matchfinder ) ) return 0;

  while( true )				/* price optimization loop */
    {
    struct Trial *cur_trial, *next_trial;
    int newlen, pos_state, prev_index, prev_index2, available_bytes, len_limit;
    int start_len = min_match_len;
    int next_price, match_price, rep_match_price;
    State cur_state;
    uint8_t prev_byte, cur_byte, match_byte;

    if( ++cur >= num_trials )		/* no more initialized trials */
      {
      LZe_backward( encoder, cur );
      return cur;
      }

    num_pairs = LZe_read_match_distances( encoder );
    newlen = ( num_pairs > 0 ) ? encoder->pairs[num_pairs-1].len : 0;
    if( newlen >= encoder->matchfinder->match_len_limit )
      {
      encoder->pending_num_pairs = num_pairs;
      LZe_backward( encoder, cur );
      return cur;
      }

    /* give final values to current trial */
    cur_trial = &encoder->trials[cur];
    prev_index = cur_trial->prev_index;
    prev_index2 = cur_trial->prev_index2;

    if( prev_index2 != single_step_trial )
      {
      --prev_index;
      if( prev_index2 >= 0 )
        {
        cur_state = encoder->trials[prev_index2].state;
        if( cur_trial->dis2 < num_rep_distances )
          cur_state = St_set_rep( cur_state );
        else
          cur_state = St_set_match( cur_state );
        }
      else
        cur_state = encoder->trials[prev_index].state;
      cur_state = St_set_char( cur_state );
      }
    else
      cur_state = encoder->trials[prev_index].state;

    if( prev_index == cur - 1 )
      {
      if( cur_trial->dis == 0 )
        cur_state = St_set_short_rep( cur_state );
      else
        cur_state = St_set_char( cur_state );
      for( i = 0; i < num_rep_distances; ++i )
        cur_trial->reps[i] = encoder->trials[prev_index].reps[i];
      }
    else
      {
      int dis;
      if( prev_index2 >= 0 )
        {
        dis = cur_trial->dis2;
        prev_index = prev_index2;
        cur_state = St_set_rep( cur_state );
        }
      else
        {
        dis = cur_trial->dis;
        if( dis < num_rep_distances )
          cur_state = St_set_rep( cur_state );
        else
          cur_state = St_set_match( cur_state );
        }
      for( i = 0; i < num_rep_distances; ++i )
        cur_trial->reps[i] = encoder->trials[prev_index].reps[i];
      LZe_mtf_reps( dis, cur_trial->reps );
      }
    cur_trial->state = cur_state;

    pos_state = Mf_data_position( encoder->matchfinder ) & pos_state_mask;
    prev_byte = Mf_peek( encoder->matchfinder, -1 );
    cur_byte = Mf_peek( encoder->matchfinder, 0 );
    match_byte = Mf_peek( encoder->matchfinder, -cur_trial->reps[0]-1 );

    next_price = cur_trial->price +
                 price0( encoder->bm_match[cur_state][pos_state] );
    if( St_is_char( cur_state ) )
      next_price += LZe_price_literal( encoder, prev_byte, cur_byte );
    else
      next_price += LZe_price_matched( encoder, prev_byte, cur_byte, match_byte );
    if( !Mf_move_pos( encoder->matchfinder ) ) return 0;

    /* try last updates to next trial */
    next_trial = &encoder->trials[cur+1];

    Tr_update( next_trial, next_price, -1, cur );

    match_price = cur_trial->price + price1( encoder->bm_match[cur_state][pos_state] );
    rep_match_price = match_price + price1( encoder->bm_rep[cur_state] );

    if( match_byte == cur_byte && next_trial->dis != 0 )
      {
      const int price = rep_match_price +
                        LZe_price_rep_len1( encoder, cur_state, pos_state );
      if( price <= next_trial->price )
        {
        next_trial->price = price;
        next_trial->dis = 0;
        next_trial->prev_index = cur;
        next_trial->prev_index2 = single_step_trial;
        }
      }

    available_bytes = min( Mf_available_bytes( encoder->matchfinder ) + 1,
                           max_num_trials - 1 - cur );
    if( available_bytes < min_match_len ) continue;

    len_limit = min( encoder->matchfinder->match_len_limit, available_bytes );

    /* try literal + rep0 */
    if( match_byte != cur_byte && next_trial->prev_index != cur )
      {
      const uint8_t * const data = Mf_ptr_to_current_pos( encoder->matchfinder ) - 1;
      const int dis = cur_trial->reps[0] + 1;
      const int limit = min( encoder->matchfinder->match_len_limit + 1,
                             available_bytes );
      len = 1;
      while( len < limit && data[len-dis] == data[len] ) ++len;
      if( --len >= min_match_len )
        {
        const int pos_state2 = ( pos_state + 1 ) & pos_state_mask;
        const State state2 = St_set_char( cur_state );
        const int price = next_price +
                  price1( encoder->bm_match[state2][pos_state2] ) +
                  price1( encoder->bm_rep[state2] ) +
                  LZe_price_rep0_len( encoder, len, state2, pos_state2 );
        while( num_trials < cur + 1 + len )
          encoder->trials[++num_trials].price = infinite_price;
        Tr_update2( &encoder->trials[cur+1+len], price, 0, cur + 1 );
        }
      }

    /* try rep distances */
    for( rep = 0; rep < num_rep_distances; ++rep )
      {
      const uint8_t * const data = Mf_ptr_to_current_pos( encoder->matchfinder ) - 1;
      int price;
      const int dis = cur_trial->reps[rep] + 1;

      if( data[-dis] != data[0] || data[1-dis] != data[1] ) continue;
      for( len = min_match_len; len < len_limit; ++len )
        if( data[len-dis] != data[len] ) break;
      while( num_trials < cur + len )
        encoder->trials[++num_trials].price = infinite_price;
      price = rep_match_price +
              LZe_price_rep( encoder, rep, cur_state, pos_state );
      for( i = min_match_len; i <= len; ++i )
        Tr_update( &encoder->trials[cur+i], price +
                   Lee_price( &encoder->rep_len_encoder, i, pos_state ),
                   rep, cur );

      if( rep == 0 ) start_len = len + 1;	/* discard shorter matches */

      /* try rep + literal + rep0 */
      {
      int len2 = len + 1, pos_state2;
      const int limit = min( encoder->matchfinder->match_len_limit + len2,
                             available_bytes );
      State state2;
      while( len2 < limit && data[len2-dis] == data[len2] ) ++len2;
      len2 -= len + 1;
      if( len2 < min_match_len ) continue;

      pos_state2 = ( pos_state + len ) & pos_state_mask;
      state2 = St_set_rep( cur_state );
      price += Lee_price( &encoder->rep_len_encoder, len, pos_state ) +
               price0( encoder->bm_match[state2][pos_state2] ) +
               LZe_price_matched( encoder, data[len-1], data[len], data[len-dis] );
      pos_state2 = ( pos_state2 + 1 ) & pos_state_mask;
      state2 = St_set_char( state2 );
      price += price1( encoder->bm_match[state2][pos_state2] ) +
               price1( encoder->bm_rep[state2] ) +
               LZe_price_rep0_len( encoder, len2, state2, pos_state2 );
      while( num_trials < cur + len + 1 + len2 )
        encoder->trials[++num_trials].price = infinite_price;
      Tr_update3( &encoder->trials[cur+len+1+len2], price, 0, cur + len + 1,
                  rep, cur );
      }
      }

    /* try matches */
    if( newlen >= start_len && newlen <= len_limit )
      {
      int dis;
      const int normal_match_price = match_price +
                                     price0( encoder->bm_rep[cur_state] );

      while( num_trials < cur + newlen )
        encoder->trials[++num_trials].price = infinite_price;

      i = 0;
      while( start_len > encoder->pairs[i].len ) ++i;
      dis = encoder->pairs[i].dis;
      for( len = start_len; ; ++len )
        {
        int price = normal_match_price +
                    LZe_price_pair( encoder, dis, len, pos_state );

        Tr_update( &encoder->trials[cur+len], price, dis + num_rep_distances, cur );

        /* try match + literal + rep0 */
        if( len == encoder->pairs[i].len )
          {
          const uint8_t * const data = Mf_ptr_to_current_pos( encoder->matchfinder ) - 1;
          const int dis2 = dis + 1;
          int len2 = len + 1;
          const int limit = min( encoder->matchfinder->match_len_limit + len2,
                                 available_bytes );
          while( len2 < limit && data[len2-dis2] == data[len2] ) ++len2;
          len2 -= len + 1;
          if( len2 >= min_match_len )
            {
            int pos_state2 = ( pos_state + len ) & pos_state_mask;
            State state2 = St_set_match( cur_state );
            price += price0( encoder->bm_match[state2][pos_state2] ) +
                     LZe_price_matched( encoder, data[len-1], data[len], data[len-dis2] );
            pos_state2 = ( pos_state2 + 1 ) & pos_state_mask;
            state2 = St_set_char( state2 );
            price += price1( encoder->bm_match[state2][pos_state2] ) +
                     price1( encoder->bm_rep[state2] ) +
                     LZe_price_rep0_len( encoder, len2, state2, pos_state2 );

            while( num_trials < cur + len + 1 + len2 )
              encoder->trials[++num_trials].price = infinite_price;
            Tr_update3( &encoder->trials[cur+len+1+len2], price, 0,
                        cur + len + 1, dis + num_rep_distances, cur );
            }
          if( ++i >= num_pairs ) break;
          dis = encoder->pairs[i].dis;
          }
        }
      }
    }
  }


static bool LZe_encode_member( struct LZ_encoder * const encoder )
  {
  const int fill_count =
    ( encoder->matchfinder->match_len_limit > 12 ) ? 128 : 512;
  int ahead, i;
  State * const state = &encoder->state;

  if( encoder->member_finished ) return true;
  if( Re_member_position( &encoder->renc ) >= encoder->member_size_limit )
    {
    if( LZe_full_flush( encoder, *state ) ) encoder->member_finished = true;
    return true;
    }

  if( Mf_data_position( encoder->matchfinder ) == 0 &&
      !Mf_finished( encoder->matchfinder ) )	/* encode first byte */
    {
    const uint8_t prev_byte = 0;
    uint8_t cur_byte;
    if( Mf_available_bytes( encoder->matchfinder ) < max_match_len &&
        !Mf_flushing_or_end( encoder->matchfinder ) )
      return true;
    cur_byte = Mf_peek( encoder->matchfinder, 0 );
    Re_encode_bit( &encoder->renc, &encoder->bm_match[*state][0], 0 );
    LZe_encode_literal( encoder, prev_byte, cur_byte );
    CRC32_update_byte( &encoder->crc, cur_byte );
    Mf_get_match_pairs( encoder->matchfinder, 0 );
    if( !Mf_move_pos( encoder->matchfinder ) ) return false;
    }

  while( !Mf_finished( encoder->matchfinder ) )
    {
    if( !Mf_enough_available_bytes( encoder->matchfinder ) ||
        !Re_enough_free_bytes( &encoder->renc ) ) return true;
    if( encoder->pending_num_pairs == 0 )
      {
      if( encoder->fill_counter <= 0 )
        { LZe_fill_distance_prices( encoder ); encoder->fill_counter = fill_count; }
      if( encoder->align_price_count <= 0 )
        LZe_fill_align_prices( encoder );
      }

    ahead = LZe_sequence_optimizer( encoder, encoder->rep_distances, *state );
    if( ahead <= 0 ) return false;		/* can't happen */

    for( i = 0; ; )
      {
      const int pos_state =
        ( Mf_data_position( encoder->matchfinder ) - ahead ) & pos_state_mask;
      const int dis = encoder->trials[i].dis;
      const int len = encoder->trials[i].price;

      bool bit = ( dis < 0 && len == 1 );
      Re_encode_bit( &encoder->renc,
                     &encoder->bm_match[*state][pos_state], !bit );
      if( bit )					/* literal byte */
        {
        const uint8_t prev_byte = Mf_peek( encoder->matchfinder, -ahead-1 );
        const uint8_t cur_byte = Mf_peek( encoder->matchfinder, -ahead );
        CRC32_update_byte( &encoder->crc, cur_byte );
        if( St_is_char( *state ) )
          LZe_encode_literal( encoder, prev_byte, cur_byte );
        else
          {
          const uint8_t match_byte =
            Mf_peek( encoder->matchfinder, -ahead-encoder->rep_distances[0]-1 );
          LZe_encode_matched( encoder, prev_byte, cur_byte, match_byte );
          }
        *state = St_set_char( *state );
        }
      else					/* match or repeated match */
        {
        CRC32_update_buf( &encoder->crc, Mf_ptr_to_current_pos( encoder->matchfinder ) - ahead, len );
        LZe_mtf_reps( dis, encoder->rep_distances );
        bit = ( dis < num_rep_distances );
        Re_encode_bit( &encoder->renc, &encoder->bm_rep[*state], bit );
        if( bit )
          {
          bit = ( dis == 0 );
          Re_encode_bit( &encoder->renc, &encoder->bm_rep0[*state], !bit );
          if( bit )
            Re_encode_bit( &encoder->renc, &encoder->bm_len[*state][pos_state], len > 1 );
          else
            {
            Re_encode_bit( &encoder->renc, &encoder->bm_rep1[*state], dis > 1 );
            if( dis > 1 )
              Re_encode_bit( &encoder->renc, &encoder->bm_rep2[*state], dis > 2 );
            }
          if( len == 1 ) *state = St_set_short_rep( *state );
          else
            {
            Lee_encode( &encoder->rep_len_encoder, &encoder->renc, len, pos_state );
            *state = St_set_rep( *state );
            }
          }
        else
          {
          LZe_encode_pair( encoder, dis - num_rep_distances, len, pos_state );
          --encoder->fill_counter;
          *state = St_set_match( *state );
          }
        }
      ahead -= len; i += len;
      if( Re_member_position( &encoder->renc ) >= encoder->member_size_limit )
        {
        if( !Mf_dec_pos( encoder->matchfinder, ahead ) ) return false;
        if( LZe_full_flush( encoder, *state ) ) encoder->member_finished = true;
        return true;
        }
      if( ahead <= 0 ) break;
      }
    }
  if( LZe_full_flush( encoder, *state ) ) encoder->member_finished = true;
  return true;
  }
