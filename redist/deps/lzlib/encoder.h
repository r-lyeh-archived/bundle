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

enum { max_num_trials = 1 << 12,
       price_shift_bits = 6,
       price_step_bits = 2 };

static const uint8_t dis_slots[1<<10] =
  {
   0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,
   8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19 };

static inline uint8_t get_slot( const uint32_t dis )
  {
  if( dis < (1 << 10) ) return dis_slots[dis];
  if( dis < (1 << 19) ) return dis_slots[dis>> 9] + 18;
  if( dis < (1 << 28) ) return dis_slots[dis>>18] + 36;
  return dis_slots[dis>>27] + 54;
  }


static const short prob_prices[bit_model_total >> price_step_bits] =
{
640, 539, 492, 461, 438, 419, 404, 390, 379, 369, 359, 351, 343, 336, 330, 323,
318, 312, 307, 302, 298, 293, 289, 285, 281, 277, 274, 270, 267, 264, 261, 258,
255, 252, 250, 247, 244, 242, 239, 237, 235, 232, 230, 228, 226, 224, 222, 220,
218, 216, 214, 213, 211, 209, 207, 206, 204, 202, 201, 199, 198, 196, 195, 193,
192, 190, 189, 188, 186, 185, 184, 182, 181, 180, 178, 177, 176, 175, 174, 172,
171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 159, 158, 157, 157, 156,
155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 145, 144, 143, 142, 141,
140, 140, 139, 138, 137, 136, 136, 135, 134, 133, 133, 132, 131, 130, 130, 129,
128, 127, 127, 126, 125, 125, 124, 123, 123, 122, 121, 121, 120, 119, 119, 118,
117, 117, 116, 115, 115, 114, 114, 113, 112, 112, 111, 111, 110, 109, 109, 108,
108, 107, 106, 106, 105, 105, 104, 104, 103, 103, 102, 101, 101, 100, 100,  99,
 99,  98,  98,  97,  97,  96,  96,  95,  95,  94,  94,  93,  93,  92,  92,  91,
 91,  90,  90,  89,  89,  88,  88,  88,  87,  87,  86,  86,  85,  85,  84,  84,
 83,  83,  83,  82,  82,  81,  81,  80,  80,  80,  79,  79,  78,  78,  77,  77,
 77,  76,  76,  75,  75,  75,  74,  74,  73,  73,  73,  72,  72,  71,  71,  71,
 70,  70,  70,  69,  69,  68,  68,  68,  67,  67,  67,  66,  66,  65,  65,  65,
 64,  64,  64,  63,  63,  63,  62,  62,  61,  61,  61,  60,  60,  60,  59,  59,
 59,  58,  58,  58,  57,  57,  57,  56,  56,  56,  55,  55,  55,  54,  54,  54,
 53,  53,  53,  53,  52,  52,  52,  51,  51,  51,  50,  50,  50,  49,  49,  49,
 48,  48,  48,  48,  47,  47,  47,  46,  46,  46,  45,  45,  45,  45,  44,  44,
 44,  43,  43,  43,  43,  42,  42,  42,  41,  41,  41,  41,  40,  40,  40,  40,
 39,  39,  39,  38,  38,  38,  38,  37,  37,  37,  37,  36,  36,  36,  35,  35,
 35,  35,  34,  34,  34,  34,  33,  33,  33,  33,  32,  32,  32,  32,  31,  31,
 31,  31,  30,  30,  30,  30,  29,  29,  29,  29,  28,  28,  28,  28,  27,  27,
 27,  27,  26,  26,  26,  26,  26,  25,  25,  25,  25,  24,  24,  24,  24,  23,
 23,  23,  23,  22,  22,  22,  22,  22,  21,  21,  21,  21,  20,  20,  20,  20,
 20,  19,  19,  19,  19,  18,  18,  18,  18,  18,  17,  17,  17,  17,  17,  16,
 16,  16,  16,  15,  15,  15,  15,  15,  14,  14,  14,  14,  14,  13,  13,  13,
 13,  13,  12,  12,  12,  12,  12,  11,  11,  11,  11,  10,  10,  10,  10,  10,
  9,   9,   9,   9,   9,   9,   8,   8,   8,   8,   8,   7,   7,   7,   7,   7,
  6,   6,   6,   6,   6,   5,   5,   5,   5,   5,   4,   4,   4,   4,   4,   4,
  3,   3,   3,   3,   3,   2,   2,   2,   2,   2,   1,   1,   1,   1,   1,   1 };

static inline int get_price( const int probability )
  { return prob_prices[probability >> price_step_bits]; }


static inline int price0( const Bit_model probability )
  { return get_price( probability ); }

static inline int price1( const Bit_model probability )
  { return get_price( bit_model_total - probability ); }

static inline int price_bit( const Bit_model bm, const int bit )
  { if( bit ) return price1( bm ); else return price0( bm ); }


static inline int price_symbol( const Bit_model bm[], int symbol,
                                const int num_bits )
  {
  int price = 0;
  symbol |= ( 1 << num_bits );
  while( symbol > 1 )
    {
    const int bit = symbol & 1;
    symbol >>= 1;
    price += price_bit( bm[symbol], bit );
    }
  return price;
  }


static inline int price_symbol_reversed( const Bit_model bm[], int symbol,
                                         const int num_bits )
  {
  int price = 0;
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i )
    {
    const int bit = symbol & 1;
    price += price_bit( bm[model], bit );
    model = ( model << 1 ) | bit;
    symbol >>= 1;
    }
  return price;
  }


static inline int price_matched( const Bit_model bm[], unsigned symbol,
                                 unsigned match_byte )
  {
  int price = 0;
  unsigned mask = 0x100;
  symbol |= 0x100;

  do {
    unsigned bit, match_bit;
    match_byte <<= 1;
    match_bit = match_byte & mask;
    symbol <<= 1;
    bit = symbol & 0x100;
    price += price_bit( bm[match_bit+(symbol>>9)+mask], bit );
    mask &= ~(match_byte ^ symbol);	/* if( match_bit != bit ) mask = 0; */
    }
  while( symbol < 0x10000 );
  return price;
  }


struct Pair			/* distance-length pair */
  {
  int dis;
  int len;
  };


enum { /* bytes to keep in buffer before dictionary */
       before_size = max_num_trials + 1,
       /* bytes to keep in buffer after pos */
       after_size = max_num_trials + ( 2 * max_match_len ) + 1,
       num_prev_positions3 = 1 << 16,
       num_prev_positions2 = 1 << 10 };

struct Matchfinder
  {
  unsigned long long partial_data_pos;
  uint8_t * buffer;		/* input buffer */
  int32_t * prev_positions;	/* last seen position of key */
  int32_t * prev_pos_tree;	/* previous positions of key */
  int match_len_limit;
  int buffer_size;
  int dictionary_size;		/* bytes to keep in buffer before pos */
  int pos;			/* current pos in buffer */
  int cyclic_pos;		/* cycles through [0, dictionary_size] */
  int pos_limit;		/* when reached, a new block must be read */
  int stream_pos;		/* first byte not yet read from file */
  int cycles;
  unsigned key4_mask;
  int num_prev_positions;	/* size of prev_positions */
  bool at_stream_end;		/* stream_pos shows real end of file */
  bool been_flushed;
  bool flushing;
  };

static int Mf_write_data( struct Matchfinder * const mf,
                          const uint8_t * const inbuf, const int size )
  {
  const int sz = min( mf->buffer_size - mf->stream_pos, size );
  if( !mf->at_stream_end && sz > 0 )
    {
    memcpy( mf->buffer + mf->stream_pos, inbuf, sz );
    mf->stream_pos += sz;
    }
  return sz;
  }

static bool Mf_normalize_pos( struct Matchfinder * const mf );

static inline void Mf_free( struct Matchfinder * const mf )
  {
  free( mf->prev_positions );
  free( mf->buffer );
  }

static inline uint8_t Mf_peek( const struct Matchfinder * const mf, const int i )
  { return mf->buffer[mf->pos+i]; }

static inline int Mf_available_bytes( const struct Matchfinder * const mf )
  { return mf->stream_pos - mf->pos; }

static inline unsigned long long
Mf_data_position( const struct Matchfinder * const mf )
  { return mf->partial_data_pos + mf->pos; }

static inline void Mf_finish( struct Matchfinder * const mf )
  { mf->at_stream_end = true; mf->flushing = false; }

static inline bool Mf_finished( const struct Matchfinder * const mf )
  { return mf->at_stream_end && mf->pos >= mf->stream_pos; }

static inline void Mf_set_flushing( struct Matchfinder * const mf,
                                    const bool flushing )
  { mf->flushing = flushing; }

static inline bool Mf_flushing_or_end( const struct Matchfinder * const mf )
  { return mf->at_stream_end || mf->flushing; }

static inline int Mf_free_bytes( const struct Matchfinder * const mf )
  { if( mf->at_stream_end ) return 0; return mf->buffer_size - mf->stream_pos; }

static inline bool Mf_enough_available_bytes( const struct Matchfinder * const mf )
  {
  return ( mf->pos + after_size <= mf->stream_pos ||
           ( Mf_flushing_or_end( mf ) && mf->pos < mf->stream_pos ) );
  }

static inline const uint8_t *
Mf_ptr_to_current_pos( const struct Matchfinder * const mf )
  { return mf->buffer + mf->pos; }

static inline bool Mf_dec_pos( struct Matchfinder * const mf,
                               const int ahead )
  {
  if( ahead < 0 || mf->pos < ahead ) return false;
  mf->pos -= ahead;
  mf->cyclic_pos -= ahead;
  if( mf->cyclic_pos < 0 ) mf->cyclic_pos += mf->dictionary_size + 1;
  return true;
  }

static inline int Mf_true_match_len( const struct Matchfinder * const mf,
                                     const int index, const int distance,
                                     int len_limit )
  {
  const uint8_t * const data = mf->buffer + mf->pos + index;
  int i = 0;

  if( index + len_limit > Mf_available_bytes( mf ) )
    len_limit = Mf_available_bytes( mf ) - index;
  while( i < len_limit && data[i-distance] == data[i] ) ++i;
  return i;
  }

static inline bool Mf_move_pos( struct Matchfinder * const mf )
  {
  if( ++mf->cyclic_pos > mf->dictionary_size ) mf->cyclic_pos = 0;
  if( ++mf->pos >= mf->pos_limit ) return Mf_normalize_pos( mf );
  return true;
  }

static int Mf_get_match_pairs( struct Matchfinder * const mf, struct Pair * pairs );


enum { re_min_free_bytes = 2 * max_num_trials };

struct Range_encoder
  {
  struct Circular_buffer cb;
  uint64_t low;
  unsigned long long partial_member_pos;
  uint32_t range;
  int ff_count;
  uint8_t cache;
  };

static inline void Re_shift_low( struct Range_encoder * const renc )
  {
  const bool carry = ( renc->low > 0xFFFFFFFFU );
  if( carry || renc->low < 0xFF000000U )
    {
    Cb_put_byte( &renc->cb, renc->cache + carry );
    for( ; renc->ff_count > 0; --renc->ff_count )
      Cb_put_byte( &renc->cb, 0xFF + carry );
    renc->cache = renc->low >> 24;
    }
  else ++renc->ff_count;
  renc->low = ( renc->low & 0x00FFFFFFU ) << 8;
  }

static inline bool Re_init( struct Range_encoder * const renc )
  {
  if( !Cb_init( &renc->cb, 65536 + re_min_free_bytes ) ) return false;
  renc->low = 0;
  renc->partial_member_pos = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  return true;
  }

static inline void Re_free( struct Range_encoder * const renc )
  { Cb_free( &renc->cb ); }

static inline unsigned long long
Re_member_position( const struct Range_encoder * const renc )
  { return renc->partial_member_pos + Cb_used_bytes( &renc->cb ) + renc->ff_count; }

static inline bool Re_enough_free_bytes( const struct Range_encoder * const renc )
  { return Cb_free_bytes( &renc->cb ) >= re_min_free_bytes; }

static inline int Re_read_data( struct Range_encoder * const renc,
                                uint8_t * const out_buffer, const int out_size )
  {
  const int size = Cb_read_data( &renc->cb, out_buffer, out_size );
  if( size > 0 ) renc->partial_member_pos += size;
  return size;
  }

static inline void Re_flush( struct Range_encoder * const renc )
  {
  int i; for( i = 0; i < 5; ++i ) Re_shift_low( renc );
  renc->low = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  }

static inline void Re_encode( struct Range_encoder * const renc,
                              const int symbol, const int num_bits )
  {
  int i;
  for( i = num_bits - 1; i >= 0; --i )
    {
    renc->range >>= 1;
    if( (symbol >> i) & 1 ) renc->low += renc->range;
    if( renc->range <= 0x00FFFFFFU )
      { renc->range <<= 8; Re_shift_low( renc ); }
    }
  }

static inline void Re_encode_bit( struct Range_encoder * const renc,
                                  Bit_model * const probability, const int bit )
  {
  const uint32_t bound = ( renc->range >> bit_model_total_bits ) * *probability;
  if( !bit )
    {
    renc->range = bound;
    *probability += (bit_model_total - *probability) >> bit_model_move_bits;
    }
  else
    {
    renc->low += bound;
    renc->range -= bound;
    *probability -= *probability >> bit_model_move_bits;
    }
  if( renc->range <= 0x00FFFFFFU )
    { renc->range <<= 8; Re_shift_low( renc ); }
  }

static inline void Re_encode_tree( struct Range_encoder * const renc,
                                   Bit_model bm[], const int symbol, const int num_bits )
  {
  int mask = ( 1 << ( num_bits - 1 ) );
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i, mask >>= 1 )
    {
    const int bit = ( symbol & mask );
    Re_encode_bit( renc, &bm[model], bit );
    model <<= 1;
    if( bit ) model |= 1;
    }
  }

static inline void Re_encode_tree_reversed( struct Range_encoder * const renc,
                                            Bit_model bm[], int symbol, const int num_bits )
  {
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i )
    {
    const int bit = symbol & 1;
    Re_encode_bit( renc, &bm[model], bit );
    model = ( model << 1 ) | bit;
    symbol >>= 1;
    }
  }

static inline void Re_encode_matched( struct Range_encoder * const renc,
                                      Bit_model bm[], unsigned symbol,
                                      unsigned match_byte )
  {
  unsigned mask = 0x100;
  symbol |= 0x100;

  do {
    unsigned bit, match_bit;
    match_byte <<= 1;
    match_bit = match_byte & mask;
    symbol <<= 1;
    bit = symbol & 0x100;
    Re_encode_bit( renc, &bm[match_bit+(symbol>>9)+mask], bit );
    mask &= ~(match_byte ^ symbol);	/* if( match_bit != bit ) mask = 0; */
    }
  while( symbol < 0x10000 );
  }


struct Len_encoder
  {
  struct Len_model lm;
  int prices[pos_states][max_len_symbols];
  int len_symbols;
  int counters[pos_states];
  };

static void Lee_update_prices( struct Len_encoder * const len_encoder,
                               const int pos_state )
  {
  int * const pps = len_encoder->prices[pos_state];
  int tmp = price0( len_encoder->lm.choice1 );
  int len = 0;
  for( ; len < len_low_symbols && len < len_encoder->len_symbols; ++len )
    pps[len] = tmp +
               price_symbol( len_encoder->lm.bm_low[pos_state], len, len_low_bits );
  tmp = price1( len_encoder->lm.choice1 );
  for( ; len < len_low_symbols + len_mid_symbols && len < len_encoder->len_symbols; ++len )
    pps[len] = tmp + price0( len_encoder->lm.choice2 ) +
               price_symbol( len_encoder->lm.bm_mid[pos_state], len - len_low_symbols, len_mid_bits );
  for( ; len < len_encoder->len_symbols; ++len )
    /* using 4 slots per value makes "Lee_price" faster */
    len_encoder->prices[3][len] = len_encoder->prices[2][len] =
    len_encoder->prices[1][len] = len_encoder->prices[0][len] =
      tmp + price1( len_encoder->lm.choice2 ) +
      price_symbol( len_encoder->lm.bm_high, len - len_low_symbols - len_mid_symbols, len_high_bits );
  len_encoder->counters[pos_state] = len_encoder->len_symbols;
  }

static void Lee_init( struct Len_encoder * const len_encoder,
                      const int match_len_limit )
  {
  int i;
  Lm_init( &len_encoder->lm );
  len_encoder->len_symbols = match_len_limit + 1 - min_match_len;
  for( i = 0; i < pos_states; ++i ) Lee_update_prices( len_encoder, i );
  }

static void Lee_encode( struct Len_encoder * const len_encoder,
                        struct Range_encoder * const renc,
                        int symbol, const int pos_state );

static inline int Lee_price( const struct Len_encoder * const len_encoder,
                             const int symbol, const int pos_state )
  { return len_encoder->prices[pos_state][symbol - min_match_len]; }


enum { infinite_price = 0x0FFFFFFF,
       max_marker_size = 16,
       num_rep_distances = 4,			/* must be 4 */
       single_step_trial = -2,
       dual_step_trial = -1 };

struct Trial
  {
  State state;
  int price;		/* dual use var; cumulative price, match length */
  int dis;		/* rep index or match distance */
  int prev_index;	/* index of prev trial in trials[] */
  int dis2;
  int prev_index2;	/*   -2  trial is single step */
			/*   -1  literal + rep0 */
			/* >= 0  rep or match + literal + rep0 */
  int reps[num_rep_distances];
  };

static inline void Tr_update( struct Trial * const trial, const int pr,
                              const int d, const int p_i )
  {
  if( pr < trial->price )
    {
    trial->price = pr;
    trial->dis = d; trial->prev_index = p_i;
    trial->prev_index2 = single_step_trial;
    }
  }

static inline void Tr_update2( struct Trial * const trial, const int pr,
                               const int d, const int p_i )
  {
  if( pr < trial->price )
    {
    trial->price = pr;
    trial->dis = d; trial->prev_index = p_i;
    trial->prev_index2 = dual_step_trial;
    }
  }

static inline void Tr_update3( struct Trial * const trial, const int pr,
                               const int d, const int p_i,
                               const int d2, const int p_i2 )
  {
  if( pr < trial->price )
    {
    trial->price = pr;
    trial->dis = d; trial->prev_index = p_i;
    trial->dis2 = d2; trial->prev_index2 = p_i2;
    }
  }


struct LZ_encoder
  {
  unsigned long long member_size_limit;
  int pending_num_pairs;
  uint32_t crc;

  Bit_model bm_literal[1<<literal_context_bits][0x300];
  Bit_model bm_match[states][pos_states];
  Bit_model bm_rep[states];
  Bit_model bm_rep0[states];
  Bit_model bm_rep1[states];
  Bit_model bm_rep2[states];
  Bit_model bm_len[states][pos_states];
  Bit_model bm_dis_slot[dis_states][1<<dis_slot_bits];
  Bit_model bm_dis[modeled_distances-end_dis_model];
  Bit_model bm_align[dis_align_size];

  struct Matchfinder * matchfinder;
  struct Range_encoder renc;
  struct Len_encoder match_len_encoder;
  struct Len_encoder rep_len_encoder;

  int num_dis_slots;
  int rep_distances[num_rep_distances];
  struct Pair pairs[max_match_len+1];
  struct Trial trials[max_num_trials];

  int dis_slot_prices[dis_states][2*max_dictionary_bits];
  int dis_prices[dis_states][modeled_distances];
  int align_prices[dis_align_size];
  int align_price_count;
  int fill_counter;
  State state;
  bool member_finished;
  };


static inline bool LZe_member_finished( const struct LZ_encoder * const encoder )
  {
  return ( encoder->member_finished && !Cb_used_bytes( &encoder->renc.cb ) );
  }


static inline void LZe_free( struct LZ_encoder * const encoder )
  { Re_free( &encoder->renc ); }

static inline unsigned LZe_crc( const struct LZ_encoder * const encoder )
  { return encoder->crc ^ 0xFFFFFFFFU; }

       /* move-to-front dis in/into reps */
static inline void LZe_mtf_reps( const int dis, int reps[num_rep_distances] )
  {
  int i;
  if( dis >= num_rep_distances )
    {
    for( i = num_rep_distances - 1; i > 0; --i ) reps[i] = reps[i-1];
    reps[0] = dis - num_rep_distances;
    }
  else if( dis > 0 )
    {
    const int distance = reps[dis];
    for( i = dis; i > 0; --i ) reps[i] = reps[i-1];
    reps[0] = distance;
    }
  }

static inline int LZe_price_rep_len1( const struct LZ_encoder * const encoder,
                                      const State state, const int pos_state )
  {
  return price0( encoder->bm_rep0[state] ) +
         price0( encoder->bm_len[state][pos_state] );
  }

static inline int LZe_price_rep( const struct LZ_encoder * const encoder,
                                 const int rep,
                                 const State state, const int pos_state )
  {
  int price;
  if( rep == 0 ) return price0( encoder->bm_rep0[state] ) +
                        price1( encoder->bm_len[state][pos_state] );
  price = price1( encoder->bm_rep0[state] );
  if( rep == 1 )
    price += price0( encoder->bm_rep1[state] );
  else
    {
    price += price1( encoder->bm_rep1[state] );
    price += price_bit( encoder->bm_rep2[state], rep - 2 );
    }
  return price;
  }

static inline int LZe_price_rep0_len( const struct LZ_encoder * const encoder,
                                      const int len,
                                      const State state, const int pos_state )
  {
  return LZe_price_rep( encoder, 0, state, pos_state ) +
         Lee_price( &encoder->rep_len_encoder, len, pos_state );
  }

static inline int LZe_price_dis( const struct LZ_encoder * const encoder,
                                 const int dis, const int dis_state )
  {
  if( dis < modeled_distances )
    return encoder->dis_prices[dis_state][dis];
  else
    return encoder->dis_slot_prices[dis_state][get_slot( dis )] +
           encoder->align_prices[dis & (dis_align_size - 1)];
  }

static inline int LZe_price_pair( const struct LZ_encoder * const encoder,
                                  const int dis, const int len,
                                  const int pos_state )
  {
  return Lee_price( &encoder->match_len_encoder, len, pos_state ) +
         LZe_price_dis( encoder, dis, get_dis_state( len ) );
  }

static inline int LZe_price_literal( const struct LZ_encoder * const encoder,
                                    uint8_t prev_byte, uint8_t symbol )
  { return price_symbol( encoder->bm_literal[get_lit_state(prev_byte)], symbol, 8 ); }

static inline int LZe_price_matched( const struct LZ_encoder * const encoder,
                                     uint8_t prev_byte, uint8_t symbol,
                                     uint8_t match_byte )
  { return price_matched( encoder->bm_literal[get_lit_state(prev_byte)],
                          symbol, match_byte ); }

static inline void LZe_encode_literal( struct LZ_encoder * const encoder,
                                       uint8_t prev_byte, uint8_t symbol )
  { Re_encode_tree( &encoder->renc,
                    encoder->bm_literal[get_lit_state(prev_byte)], symbol, 8 ); }

static inline void LZe_encode_matched( struct LZ_encoder * const encoder,
                                       uint8_t prev_byte, uint8_t symbol,
                                       uint8_t match_byte )
  { Re_encode_matched( &encoder->renc,
                       encoder->bm_literal[get_lit_state(prev_byte)],
                       symbol, match_byte ); }

static inline void LZe_encode_pair( struct LZ_encoder * const encoder,
                                    const uint32_t dis, const int len,
                                    const int pos_state )
  {
  const int dis_slot = get_slot( dis );
  Lee_encode( &encoder->match_len_encoder, &encoder->renc, len, pos_state );
  Re_encode_tree( &encoder->renc, encoder->bm_dis_slot[get_dis_state(len)],
                  dis_slot, dis_slot_bits );

  if( dis_slot >= start_dis_model )
    {
    const int direct_bits = ( dis_slot >> 1 ) - 1;
    const uint32_t base = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
    const uint32_t direct_dis = dis - base;

    if( dis_slot < end_dis_model )
      Re_encode_tree_reversed( &encoder->renc,
                               encoder->bm_dis + base - dis_slot - 1,
                               direct_dis, direct_bits );
    else
      {
      Re_encode( &encoder->renc, direct_dis >> dis_align_bits,
                 direct_bits - dis_align_bits );
      Re_encode_tree_reversed( &encoder->renc, encoder->bm_align,
                               direct_dis, dis_align_bits );
      --encoder->align_price_count;
      }
    }
  }

static inline int LZe_read_match_distances( struct LZ_encoder * const encoder )
  {
  const int num_pairs =
    Mf_get_match_pairs( encoder->matchfinder, encoder->pairs );
  if( num_pairs > 0 )
    {
    int len = encoder->pairs[num_pairs-1].len;
    if( len == encoder->matchfinder->match_len_limit && len < max_match_len )
      {
      len += Mf_true_match_len( encoder->matchfinder, len,
                                encoder->pairs[num_pairs-1].dis + 1,
                                max_match_len - len );
      encoder->pairs[num_pairs-1].len = len;
      }
    }
  return num_pairs;
  }

static inline bool LZe_move_pos( struct LZ_encoder * const encoder, int n )
  {
  if( --n >= 0 && !Mf_move_pos( encoder->matchfinder ) ) return false;
  while( --n >= 0 )
    {
    Mf_get_match_pairs( encoder->matchfinder, 0 );
    if( !Mf_move_pos( encoder->matchfinder ) ) return false;
    }
  return true;
  }

static inline void LZe_backward( struct LZ_encoder * const encoder, int cur )
  {
  int * const dis = &encoder->trials[cur].dis;
  while( cur > 0 )
    {
    const int prev_index = encoder->trials[cur].prev_index;
    struct Trial * const prev_trial = &encoder->trials[prev_index];

    if( encoder->trials[cur].prev_index2 != single_step_trial )
      {
      prev_trial->dis = -1;
      prev_trial->prev_index = prev_index - 1;
      prev_trial->prev_index2 = single_step_trial;
      if( encoder->trials[cur].prev_index2 >= 0 )
        {
        struct Trial * const prev_trial2 = &encoder->trials[prev_index-1];
        prev_trial2->dis = encoder->trials[cur].dis2;
        prev_trial2->prev_index = encoder->trials[cur].prev_index2;
        prev_trial2->prev_index2 = single_step_trial;
        }
      }
    prev_trial->price = cur - prev_index;			/* len */
    cur = *dis; *dis = prev_trial->dis; prev_trial->dis = cur;
    cur = prev_index;
    }
  }
