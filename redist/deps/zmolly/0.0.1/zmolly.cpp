/*
 * Copyright (C) 2015-2016 by Zhang Li <richselian at gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <fstream>
#include <algorithm>
#include <bitset>
#include <vector>
#include <thread>
#include <sstream>
#include <map>

/*******************************************************************************
 * Portable CLZ
 ******************************************************************************/
#if defined(__GNUC__)
#   define clz(x) __builtin_clz(x)
#elif defined(_MSC_VER)
#   include <intrin.h>
    uint32_t __inline clz( uint32_t value ) {
        DWORD leading_zero = 0;
        _BitScanReverse( &leading_zero, value );
        return 31 - leading_zero;
    }
#else
    static uint32_t inline popcnt( uint32_t x ) {
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return x & 0x0000003f;
    }
    static uint32_t inline clz( uint32_t x ) {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return 32 - popcnt(x);
    }
#endif

/*******************************************************************************
 * Allocator
 ******************************************************************************/

#if 0

static const int ALLOCATOR_POOLSIZE = 8192;

struct allocator_element_t {
    allocator_element_t* m_next;
};

struct allocator_block_t {
    allocator_block_t* m_next;
    unsigned char m_data[0];

    inline allocator_element_t* get_element(int nsize, int i) {
        return (allocator_element_t*)(m_data + nsize * i);
    }
};

struct allocator_t {
    allocator_block_t* m_blocklist;
    allocator_element_t* m_freelist;

    allocator_t() {
        m_blocklist = nullptr;
        m_freelist = nullptr;
    }
    ~allocator_t() {
        allocator_block_t* block;
        while (m_blocklist) {
            block = m_blocklist;
            m_blocklist = m_blocklist->m_next;
            free(block);
        }
    }

    inline void* alloc(int nsize) {
        if (!m_freelist) {  // freelist is empty, allocate more nodes
            allocator_block_t* block = (allocator_block_t*)malloc(sizeof(*block) + nsize * ALLOCATOR_POOLSIZE);
            block->m_next = m_blocklist;
            block->get_element(nsize, 0)->m_next = nullptr;

            for(auto i = 1; i < ALLOCATOR_POOLSIZE; i++) {
                block->get_element(nsize, i)->m_next = block->get_element(nsize, i - 1);
            }
            m_blocklist = block;
            m_freelist = block->get_element(nsize, ALLOCATOR_POOLSIZE - 1);
        }
        allocator_element_t* node = m_freelist;
        m_freelist = m_freelist->m_next;
        return (void*)node;
    }

    inline void free(void* node) {
        allocator_element_t* element = (allocator_element_t*)node;
        element->m_next = m_freelist;
        m_freelist = element;
    }
};

#else

struct allocator_t {
    std::map< void *, size_t > map;

    allocator_t() {
    }
    ~allocator_t() {
        for( auto &pair : map ) {
            if( pair.second ) {
                free( pair.first );
            }
        }
    }

    inline void* alloc(int nsize) {
        void *ptr = malloc(nsize);
        map[ ptr ] = 1;
        return ptr;
    }

    inline void free(void* ptr) {
        map[ ptr ] = 0;
    }
};

#endif

/*******************************************************************************
 * Arithmetic coder
 ******************************************************************************/
static const uint32_t RC_TOP = 1 << 24;
static const uint32_t RC_BOT = 1 << 16;

struct rc_encoder_t {
    std::ostream& m_ostream;
    uint32_t m_low;
    uint32_t m_range;

    rc_encoder_t(std::ostream& ostream): m_ostream(ostream) {
        m_low = 0;
        m_range = -1;
    }

    inline void encode(uint16_t cum, uint16_t frq, uint16_t sum) {
        m_range /= sum;
        m_low += cum * m_range;
        m_range *= frq;
        while((m_low ^ (m_low + m_range)) < RC_TOP || (m_range < RC_BOT && ((m_range = -m_low & (RC_BOT - 1)), 1))) {
            m_ostream.put(m_low >> 24);
            m_low <<= 8;
            m_range <<= 8;
        }
    }
    inline void flush() {
        m_ostream.put(m_low >> 24); m_low <<= 8;
        m_ostream.put(m_low >> 24); m_low <<= 8;
        m_ostream.put(m_low >> 24); m_low <<= 8;
        m_ostream.put(m_low >> 24); m_low <<= 8;
    }
};

struct rc_decoder_t {
    std::istream& m_istream;
    uint32_t m_low;
    uint32_t m_range;
    uint32_t m_code;

    rc_decoder_t(std::istream& istream): m_istream(istream) {
        m_low = 0;
        m_range = -1;
        m_code = 0;
        m_code |= (unsigned char)m_istream.get() << 24;
        m_code |= (unsigned char)m_istream.get() << 16;
        m_code |= (unsigned char)m_istream.get() << 8;
        m_code |= (unsigned char)m_istream.get() << 0;
    }

    inline void decode(uint16_t cum, uint16_t frq) {
        m_low += cum * m_range;
        m_range *= frq;
        while ((m_low ^ (m_low + m_range)) < RC_TOP || (m_range < RC_BOT && ((m_range = -m_low & (RC_BOT - 1)), 1))) {
            m_code = m_code << 8 | (unsigned char)m_istream.get();
            m_range <<= 8;
            m_low <<= 8;
        }
    }
    inline uint16_t decode_cum(uint16_t sum) {
        m_range /= sum;
        return (m_code - m_low) / m_range;
    }
};

/*******************************************************************************
 * PPM Model
 ******************************************************************************/
static const int PPM_O4_BUCKET_SIZE = 262144;
static const int PPM_SPARSE_MODEL_SYMBOLS = 56;
static const int PPM_SEE_SIZE = 131072;

struct symbol_counter_t {
    uint8_t m_sym;
    uint8_t m_frq;

    inline symbol_counter_t() {
        m_sym = 0;
        m_frq = 0;
    }
};

struct see_model_t {  // secondary escape elimination type
    uint16_t m_c[2];

    inline int encode(rc_encoder_t* coder, int c) {
        if (!c) {
            coder->encode(0, m_c[0], m_c[0] + m_c[1]);
        } else {
            coder->encode(m_c[0], m_c[1], m_c[0] + m_c[1]);
        }
        return c;
    }
    inline int decode(rc_decoder_t* coder) {
        if (m_c[0] > coder->decode_cum(m_c[0] + m_c[1])) {
            coder->decode(0, m_c[0]);
            return 0;
        } else {
            coder->decode(m_c[0], m_c[1]);
            return 1;
        }
    }
    inline void update(int c, int update) {
        if ((m_c[c] += update) > 16000) {
            m_c[0] = (m_c[0] + 1) / 2;
            m_c[1] = (m_c[1] + 1) / 2;
        }
        return;
    }
};

struct dense_model_t {  // dense model types, use for short context
    uint16_t m_sum;
    uint16_t m_cnt;
    uint16_t m_esc;
    symbol_counter_t m_symbols[256];

    inline dense_model_t() {
        m_sum = 0;
        m_cnt = 0;
        m_esc = 0;
    }
    inline int encode(rc_encoder_t* coder, std::bitset<256>& exclude, int c) {
        int found = 0;
        int found_index = 0;
        uint16_t cum = 0;
        uint16_t frq = 0;
        uint16_t sum = 0;
        uint16_t esc = 0;
        uint16_t recent_frq = m_symbols[0].m_frq & -!exclude[m_symbols[0].m_sym];
        if (!exclude.any()) {
            for(auto i = 0; i < m_cnt; i++) {  // no exclusion
                if (m_symbols[i].m_sym == c) {
                    found_index = i;
                    found = 1;
                    break;
                }
                cum += m_symbols[i].m_frq;
            }
            sum = m_sum;
        } else {
            for(auto i = 0; i < m_cnt; i++) {
                if (m_symbols[i].m_sym == c) {
                    found_index = i;
                    found = 1;
                }
                cum += m_symbols[i].m_frq & -(!exclude[m_symbols[i].m_sym] && !found);
                sum += m_symbols[i].m_frq & -(!exclude[m_symbols[i].m_sym]);
            }
        }

        esc = m_esc + !m_esc;
        sum += recent_frq + esc;
        frq = m_symbols[found_index].m_frq;
        if (found_index == 0) {
            frq += recent_frq;
        } else {
            std::swap(m_symbols[found_index], m_symbols[0]);
            cum += recent_frq;
        }

        if (!found) {
            for(auto i = 0; i < m_cnt; i++) {  // do exclude
                exclude[m_symbols[i].m_sym] = 1;
            }
            m_symbols[m_cnt].m_frq = m_symbols[0].m_frq;
            m_symbols[m_cnt].m_sym = m_symbols[0].m_sym;
            m_symbols[0].m_sym = c;
            m_symbols[0].m_frq = 0;
            m_cnt += 1;
            cum = sum - esc;
            frq = esc;
        }
        coder->encode(cum, frq, sum);
        return found;
    }

    inline int decode(rc_decoder_t* coder, std::bitset<256>& exclude) {
        uint16_t cum = 0;
        uint16_t frq = 0;
        uint16_t sum = 0;
        uint16_t esc = 0;
        uint16_t recent_frq = m_symbols[0].m_frq & -!exclude[m_symbols[0].m_sym];
        int sym = -1;

        for(auto i = 0; i < m_cnt; i++) {
            sum += m_symbols[i].m_frq & -!exclude[m_symbols[i].m_sym];
        }
        esc = m_esc + !m_esc;
        sum += recent_frq + esc;
        uint16_t decode_cum = coder->decode_cum(sum);
        if (sum - esc <= decode_cum) {
            for(auto i = 0; i < m_cnt; i++) {  // do exclude
                exclude[m_symbols[i].m_sym] = 1;
            }
            m_symbols[m_cnt].m_frq = m_symbols[0].m_frq;
            m_symbols[m_cnt].m_sym = m_symbols[0].m_sym;
            m_symbols[0].m_frq = 0;
            m_cnt += 1;
            cum = sum - esc;
            frq = esc;
        } else {
            int i = 0;
            if (!exclude.any()) {  // no exclusion
                while (cum + recent_frq + m_symbols[i].m_frq <= decode_cum) {
                    cum += m_symbols[i].m_frq;
                    i++;
                }
            } else {
                while (cum + recent_frq + (m_symbols[i].m_frq & -!exclude[m_symbols[i].m_sym]) <= decode_cum) {
                    cum += m_symbols[i].m_frq & -!exclude[m_symbols[i].m_sym];
                    i++;
                }
            }
            frq = m_symbols[i].m_frq;
            sym = m_symbols[i].m_sym;
            if (i == 0) {
                frq += recent_frq;
            } else {
                std::swap(m_symbols[i], m_symbols[0]);
                cum += recent_frq;
            }
        }
        coder->decode(cum, frq);
        return sym;
    }

    inline void update(int c) {
        m_symbols[0].m_frq += 1;
        m_symbols[0].m_sym = c;
        m_sum += 1;
        m_esc += (m_symbols[0].m_frq == 1) - (m_symbols[0].m_frq == 2);

        if (m_symbols[0].m_frq > 250) {  // rescale
            int n = 0;
            m_cnt = 0;
            m_sum = 0;
            m_esc = 0;
            for(auto i = 0; i + n < 256; i++) {
                if ((m_symbols[i].m_frq = m_symbols[i + n].m_frq / 2) > 0) {
                    m_symbols[i].m_sym = m_symbols[i + n].m_sym;
                    m_cnt += 1;
                    m_sum += m_symbols[i].m_frq;
                    m_esc += m_symbols[i].m_frq == 1;
                } else {
                    n++;
                    i--;
                }
            }
            memset(m_symbols + m_cnt, 0, sizeof(m_symbols[0]) * (256 - m_cnt));
        }
    }

};

struct sparse_model_t {  // sparse model types, use for long context
    sparse_model_t* m_next;
    uint32_t m_context;
    uint16_t m_sum;
    uint8_t m_cnt;
    uint8_t m_visited;
    symbol_counter_t m_symbols[PPM_SPARSE_MODEL_SYMBOLS];  // PPM_SPARSE_MODEL_SYMBOLS=56: make 128byte struct

    inline sparse_model_t() {
        m_next = nullptr;
        m_context = 0;
        m_sum = 0;
        m_cnt = 0;
        m_visited = 0;
    }

    inline int encode(see_model_t* see, rc_encoder_t* coder, int c, std::bitset<256>& exclude) {
        symbol_counter_t tmp_symbol;
        uint16_t cum = 0;
        uint16_t frq = 0;
        int i;
        for(i = 0; i < m_cnt && m_symbols[i].m_sym != c; i++) {  /* search for symbol */
            cum += m_symbols[i].m_frq;
        }
        if (i < m_cnt) {  // found -- bring to front of linked-list
            see->encode(coder, 0);
            see->update(0, 35);
            if (m_cnt != 1) {  // no need to encode binary context
                uint16_t recent_frq = (m_symbols[0].m_frq + 6) / 2;  // recency scaling
                if (i == 0) {
                    frq = m_symbols[i].m_frq + recent_frq;
                } else {
                    frq = m_symbols[i].m_frq;
                    cum += recent_frq;
                    tmp_symbol = m_symbols[i];
                    memmove(m_symbols + 1, m_symbols, sizeof(m_symbols[0]) * i);
                    m_symbols[0] = tmp_symbol;
                }
                coder->encode(cum, frq, m_sum + recent_frq);
            }
            return 1;

        } else {  // not found -- create new node for sym
            see->encode(coder, 1);
            see->update(1, 35);
            for(i = 0; i < m_cnt; i++) {
                exclude[m_symbols[i].m_sym] = 1;  // exclude o4
            }
            if (m_cnt == PPM_SPARSE_MODEL_SYMBOLS) {
                m_sum -= m_symbols[m_cnt - 1].m_frq;
            } else {
                m_cnt += 1;
            }
            memmove(m_symbols + 1, m_symbols, sizeof(m_symbols[0]) * (m_cnt - 1));
            m_symbols[0].m_sym = c;
            m_symbols[0].m_frq = 0;
        }
        return 0;
    }

    inline int decode(see_model_t* see, rc_decoder_t* coder, std::bitset<256>& exclude) {
        uint16_t cum = 0;
        uint16_t frq = 0;
        if (see->decode(coder) == 0) {
            see->update(0, 35);
            if (m_cnt != 1) {  // no need to decode binary context
                uint16_t recent_frq = (m_symbols[0].m_frq + 6) / 2;  // recency scaling
                uint16_t decode_cum = coder->decode_cum(m_sum + recent_frq);
                int i = 0;
                while (cum + recent_frq + m_symbols[i].m_frq <= decode_cum) {
                    cum += m_symbols[i].m_frq;
                    i++;
                }
                if (i == 0) {
                    frq = m_symbols[i].m_frq + recent_frq;
                } else {
                    frq = m_symbols[i].m_frq;
                    cum += recent_frq;
                    symbol_counter_t tmp_symbol = m_symbols[i];
                    memmove(m_symbols + 1, m_symbols, sizeof(m_symbols[0]) * i);
                    m_symbols[0] = tmp_symbol;
                }
                coder->decode(cum, frq);
            }
            return m_symbols[0].m_sym;

        } else {  // not found
            see->update(1, 35);
            for(auto i = 0; i < m_cnt; i++) {
                exclude[m_symbols[i].m_sym] = 1;  // exclude o4
            }
            if (m_cnt == PPM_SPARSE_MODEL_SYMBOLS) {
                m_sum -= m_symbols[m_cnt - 1].m_frq;
            } else {
                m_cnt += 1;
            }
            memmove(m_symbols + 1, m_symbols, sizeof(m_symbols[0]) * (m_cnt - 1));
            m_symbols[0].m_frq = 0;
        }
        return -1;
    }

    inline void update(dense_model_t* lower_o2, int c) {
        int n = 0;
        int o2frq = 0;

        if (m_symbols[0].m_frq == 0) {  // calculate init frequency
            for(auto i = 0; i < lower_o2->m_cnt; i++) {
                if (lower_o2->m_symbols[i].m_sym == c) {
                    o2frq = lower_o2->m_symbols[i].m_frq;
                    break;
                }
            }
            if (m_sum == 0) {
                m_symbols[0].m_frq = 2 + o2frq * 2 / (lower_o2->m_sum + lower_o2->m_cnt + 1 - o2frq);
            } else {
                m_symbols[0].m_frq = 2 + o2frq * (m_sum + m_cnt + 1) / (lower_o2->m_sum + lower_o2->m_cnt + m_sum - o2frq);
            }
            m_symbols[0].m_frq = (m_symbols[0].m_frq > 0) ? m_symbols[0].m_frq : 1;
            m_symbols[0].m_frq = (m_symbols[0].m_frq < 8) ? m_symbols[0].m_frq : 7;
            m_symbols[0].m_sym = c;
            m_sum += m_symbols[0].m_frq;
        } else {
            // update freq
            m_symbols[0].m_frq += 3;
            m_symbols[0].m_sym = c;
            m_sum += 3;
        }

        if (m_symbols[0].m_frq > 250) {  // rescale
            m_cnt = 0;
            m_sum = 0;
            for(auto i = 0; i + n < 56; i++) {
                if ((m_symbols[i].m_frq = m_symbols[i + n].m_frq / 2) > 0) {
                    m_symbols[i].m_sym = m_symbols[i + n].m_sym;
                    m_cnt += 1;
                    m_sum += m_symbols[i].m_frq;
                } else {
                    n++;
                    i--;
                }
            }
            memset(m_symbols + m_cnt, 0, sizeof(m_symbols[0]) * (56 - m_cnt));
        }
        return;
    }
} 
#ifndef _MSC_VER
__attribute__((__aligned__(128)))
#endif
;

// main ppm-model type
struct ppm_model_t {
    std::vector<sparse_model_t*> m_o4_buckets;
    std::vector<dense_model_t> m_o2;
    std::vector<dense_model_t> m_o1;
    std::vector<dense_model_t> m_o0;
    allocator_t m_o4_allocator;
    uint32_t m_o4_counts;
    uint32_t m_context;
    uint8_t m_sse_ch_context;
    uint8_t m_sse_last_esc;
    std::vector<see_model_t> m_see;
    see_model_t see_01;

    ppm_model_t() {
        m_o4_buckets.resize(PPM_O4_BUCKET_SIZE);
        m_o2.resize(65536);
        m_o1.resize(256);
        m_o0.resize(1);
        m_o4_counts = 0;
        m_context = 0;

        m_see.resize(PPM_SEE_SIZE);
        m_sse_ch_context = 0;
        m_sse_last_esc = 0;
        for (auto i = 0; i < PPM_SEE_SIZE; i++) {
            m_see[i].m_c[0] = 20;
            m_see[i].m_c[1] = 20;
        }
        see_01.m_c[0] = 0;
        see_01.m_c[1] = 1;
    }

    inline see_model_t* current_see(sparse_model_t* o4) {
        static const auto log2i = [](uint32_t x) -> int {
            return (31 - clz((x << 1) | 0x01));
        };

        if (o4->m_cnt == 0) {
            return &see_01;  // no symbols under current context -- always escape
        }
        int curcnt = o4->m_cnt;
        int lowsum = current_o2()->m_sum;
        int lowcnt = current_o2()->m_cnt;
        uint32_t context = 0;

        context |= ((m_context >>  6) & 0x03);
        context |= ((m_context >> 14) & 0x03) << 2;
        context |= ((m_context >> 22) & 0x03) << 4;
        context |= m_sse_last_esc << 6;
        if (curcnt == 1) {
            // QUANTIZE(binary) = (sum[3] | lowcnt[2] | lowsum[1] | bin_symbol[3] | last_esc[1] | previous symbols[6])
            context |= (o4->m_symbols[0].m_sym >> 5) << 7;
            context |= (lowsum >= 4) << 10;
            context |= std::min(log2i(lowcnt / 2), 3) << 11;
            context |= std::min(log2i(o4->m_sum / 3), 7) << 13;
            context |= 1 << 16;
            return &m_see[context];
        } else {
            // QUANTIZE = (sum[3] | curcnt[2] | lowsum[1] | (lowcnt - curcnt)[3] | last_esc[1] | previous symbols[6])
            context |= std::min(log2i(std::max(lowcnt - curcnt, 0) / 2), 3) << 7;
            context |= (lowsum >= 4) << 10;
            context |= std::min(log2i(curcnt / 2), 3) << 11;
            context |= std::min(log2i(o4->m_sum / 8), 7) << 13;
            context |= 0 << 16;
            return &m_see[context];
        }
        return nullptr;
    }

    inline sparse_model_t* current_o4() {
        static const auto o4_hash = [](uint32_t context) {
            return (context ^ context >> 14) % PPM_O4_BUCKET_SIZE;
        };
        sparse_model_t* o4 = nullptr;
        sparse_model_t* o4_prev = nullptr;

        if (m_o4_counts >= 1048576) {  // too many o4-context/symbol nodes
            m_o4_counts = 0;
            for(auto i = 0; i < PPM_O4_BUCKET_SIZE; i++) {
                while (m_o4_buckets[i] && m_o4_buckets[i]->m_visited / 2 == 0) {
                    o4 = m_o4_buckets[i];
                    m_o4_buckets[i] = m_o4_buckets[i]->m_next;
                    m_o4_allocator.free(o4);
                }
                for(o4 = m_o4_buckets[i]; o4 != nullptr; o4 = o4->m_next) {
                    if ((o4->m_visited /= 2) == 0) {
                        o4_prev->m_next = o4->m_next;
                        m_o4_allocator.free(o4);
                        o4 = o4_prev;
                    } else {
                        m_o4_counts += 1;
                        o4_prev = o4;
                    }
                }
            }
        }

        o4 = m_o4_buckets[o4_hash(m_context)];
        o4_prev = nullptr;
        while (o4 && o4->m_context != m_context) {  // search for o4 context
            o4_prev = o4;
            o4 = o4->m_next;
        }
        if (o4 != nullptr) {  // found -- bring to front of linked-list
            if (o4_prev != nullptr) {
                o4_prev->m_next = o4->m_next;
                o4->m_next = m_o4_buckets[o4_hash(m_context)];
                m_o4_buckets[o4_hash(m_context)] = o4;
            }
        } else {  // not found -- create new node for context
            o4 = (sparse_model_t*)m_o4_allocator.alloc(sizeof(sparse_model_t));
            memset(o4->m_symbols, 0, sizeof(o4->m_symbols));
            o4->m_context = m_context;
            o4->m_sum = 0;
            o4->m_cnt = 0;
            o4->m_visited = 0;
            o4->m_next = m_o4_buckets[o4_hash(m_context)];
            m_o4_counts += 1;
            m_o4_buckets[o4_hash(m_context)] = o4;
        }
        o4->m_visited += (o4->m_visited < 255);
        return o4;
    }
    inline dense_model_t* current_o2() { return &m_o2[m_context & 0xffff]; }
    inline dense_model_t* current_o1() { return &m_o1[m_context & 0x00ff]; }
    inline dense_model_t* current_o0() { return &m_o0[0]; }

    inline void encode(rc_encoder_t* coder, int c) {
        sparse_model_t* o4 = current_o4();
        dense_model_t* o2 = current_o2();
        dense_model_t* o1 = current_o1();
        dense_model_t* o0 = current_o0();
        int order;
        std::bitset<256> exclude;
        while (-1) {
            order = 4; if (o4->encode(current_see(o4), coder, c, exclude)) break;
            order = 2; if (o2->encode(coder, exclude, c)) break;
            order = 1; if (o1->encode(coder, exclude, c)) break;
            order = 0; if (o0->encode(coder, exclude, c)) break;

            // decode with o(-1)
            uint16_t cum = 0;
            for(auto i = 0; i < c; i++) {
                cum += !exclude[i];
            }
            coder->encode(cum, 1, 256 - exclude.count());
            break;
        }
        switch (order) {  // fall-through switch
            case 0: o0->update(c);
            case 1: o1->update(c);
            case 2: o2->update(c);
            case 4: o4->update(o2, c);
        }
        m_sse_last_esc = (order == 4);
    }

    // main ppm-decode method
    inline int decode(rc_decoder_t* coder) {
        sparse_model_t* o4 = current_o4();
        dense_model_t* o2 = current_o2();
        dense_model_t* o1 = current_o1();
        dense_model_t* o0 = current_o0();
        int order;
        int c;
        std::bitset<256> exclude;
        while (-1) {
            order = 4; if ((c = o4->decode(current_see(o4), coder, exclude)) != -1) break;
            order = 2; if ((c = o2->decode(coder, exclude)) != -1) break;
            order = 1; if ((c = o1->decode(coder, exclude)) != -1) break;
            order = 0; if ((c = o0->decode(coder, exclude)) != -1) break;

            // decode with o(-1)
            uint16_t decode_cum = coder->decode_cum(256 - exclude.count());
            uint16_t cum = 0;
            for(c = 0; cum + !exclude[c] <= decode_cum; c++) {
                cum += !exclude[c];
            }
            coder->decode(cum, 1);
            break;
        }
        switch (order) {  // fall-through switch
            case 0: o0->update(c);
            case 1: o1->update(c);
            case 2: o2->update(c);
            case 4: o4->update(o2, c);
        }
        m_sse_last_esc = (order == 4);
        return c;
    }

    inline void update_context(int c) {
        m_context <<= 8;
        m_context |= c;
    }
};

/*******************************************************************************
 * Matcher
 ******************************************************************************/
struct matcher_t {
    static const int match_min = 7;
    static const int match_max = 255;
    std::vector<uint64_t> m_lzp;

    matcher_t() {
        m_lzp.resize(1048576, 0);
    }
    inline static uint32_t hash2(unsigned char* data) {
        return *(uint16_t*)data;
    }
    inline static uint32_t hash5(unsigned char* data) {
        return (*(uint32_t*)data ^ (*(uint32_t*)data >> 10) ^ (*(uint32_t*)data >> 20)) & 0xfffff;
    }
    inline static uint32_t hash8(unsigned char* data) {
        return (*(uint64_t*)data ^ (*(uint64_t*)data >> 22) ^ (*(uint64_t*)data >> 44)) & 0xfffff;
    }

    inline uint32_t getpos(unsigned char* data, uint32_t pos) {
        uint32_t lzpos[3] = {
            (uint32_t)(m_lzp[hash8(data + pos - 8)] & 0xffffffff),
            (uint32_t)(m_lzp[hash5(data + pos - 5)] & 0xffffffff),
        };
        if (m_lzp[hash8(data + pos - 8)] >> 32 == *(uint32_t*)(data + pos - 4) && lzpos[0] != 0) return lzpos[0];
        if (m_lzp[hash5(data + pos - 5)] >> 32 == *(uint32_t*)(data + pos - 4) && lzpos[1] != 0) return lzpos[1];
        return m_lzp[hash2(data + pos - 2)];
    }

    inline uint32_t lookup(unsigned char* data, uint32_t data_size, uint32_t pos) {
        uint32_t match_pos = getpos(data, pos);
        uint32_t match_len = 0;

        // match content
        if (match_pos > 0 && match_pos + match_len < data_size) {
            while (match_len < match_max && data[match_pos + match_len] == data[pos + match_len]) {
                match_len++;
            }
        }
        return (match_len >= match_min) ? match_len : 1;
    }

    inline void update(unsigned char* data, uint32_t pos) {
        if (pos >= 8) {  // avoid overflow
            m_lzp[hash8(data + pos - 8)] = pos | (uint64_t)*(uint32_t*)(data + pos - 4) << 32;
            m_lzp[hash5(data + pos - 5)] = pos | (uint64_t)*(uint32_t*)(data + pos - 4) << 32;
            m_lzp[hash2(data + pos - 2)] = pos;
        }
        return;
    }
};

/*******************************************************************************
 * Codec
 ******************************************************************************/
#define MLEN_SIZE 64000

int zmolly_encode(std::istream& fdata, std::ostream& fcomp0, int block_size) {
    ppm_model_t ppm;
    std::vector<unsigned char> ib;
    std::stringstream fcomp;

    while (fdata) {
        uint8_t escape = 0;

        // read next block
        ib.resize(block_size);
        fdata.read((char*)ib.data(), block_size);

        if (fdata.bad() || fdata.gcount() == 0) {
            break;
        }
        ib.resize(fdata.gcount());
        int ibpos = 0;

        std::thread thread;
        int mselect = 0;
        int mlen[2][MLEN_SIZE];
        int midx = 0;
        int mpos = 0;

        // count for escape char
        int counts[256] = {0};
        for(auto byte: ib) {
            if (++counts[byte] < counts[escape]) {
                escape = byte;
            }
        }

        fcomp.str("");
        fcomp.put(escape);

        // start encoding
        matcher_t matcher;
        rc_encoder_t coder(fcomp);

        auto func_matching_thread = [&](int* mlen) {
            int match_len;
            int midx = 0;

            while (mpos < int(ib.size()) && midx < MLEN_SIZE) {
                match_len = 1;
                // find a match -- avoid underflow
                if (mpos >= 8)
                // find a match -- avoid overflow
                if (mpos + matcher_t::match_max < int(ib.size())) {
                    match_len = matcher.lookup(ib.data(), ib.size(), mpos);
                    for(auto i = 0; i < match_len; i++) {
                        matcher.update(ib.data(), mpos + i);
                    }
                }
                mpos += match_len;
                mlen[midx++] = match_len;
            }
        };

        // start thread (matching first block)
        thread = std::thread(func_matching_thread, mlen[0]); thread.join();
        thread = std::thread(func_matching_thread, mlen[1]);

        while (ibpos < int(ib.size())) {
            // find match
            if (midx >= MLEN_SIZE) {  // start the next matching thread
                thread.join();
                thread = std::thread(func_matching_thread, mlen[mselect]);
                midx = 0;
                mselect ^= 1;
            }
            int match_len = mlen[mselect][midx++];

            if (match_len > 1) {  // encode a match
                ppm.encode(&coder, escape);
                ppm.update_context(escape);
                ppm.encode(&coder, match_len);
                ppm.m_sse_last_esc = 0;

            } else {  // encode a literal
                ppm.encode(&coder, ib[ibpos]);
                if (ib[ibpos] == escape) {
                    ppm.update_context(escape);
                    ppm.encode(&coder, 0);
                }
            }
            for(auto i = 0; i < match_len; i++) {  // update context
                ppm.update_context(ib[ibpos++]);
            }
        }
        thread.join();
        coder.flush();

        // write back headers
        //  stop-mark: 1
        //  datasize:  4
        //  compsize:  4
        auto fcomp_size = fcomp.tellp();
        fcomp0.put(1);
        fcomp0.put(ib.size() / 16777216 % 256);
        fcomp0.put(ib.size() / 65536 % 256);
        fcomp0.put(ib.size() / 256 % 256);
        fcomp0.put(ib.size() / 1 % 256);
        fcomp0.put(fcomp_size / 16777216 % 256);
        fcomp0.put(fcomp_size / 65536 % 256);
        fcomp0.put(fcomp_size / 256 % 256);
        fcomp0.put(fcomp_size / 1 % 256);

        // write block
        fcomp0 << fcomp.rdbuf();
        fcomp0.flush();

        //fprintf(stderr, "encode-block: %d => %d\n", int(ib.size()), int(fcomp_size));
    }
    fcomp0.put(0);

    if (fcomp0.bad() || fdata.bad()) {
        //fprintf(stderr, "error: I/O Error.");
        return -1;
    }
    return 0;
}

int zmolly_decode(std::istream& fcomp, std::ostream& fdata) {
    ppm_model_t ppm;
    std::vector<unsigned char> ob;

    while (fcomp.get() == 1) {
        uint8_t escape;
        uint8_t c;

        // read header
        int datasize = 0;
        int compsize = 0;
        datasize += (unsigned char)fcomp.get() * 16777216;
        datasize += (unsigned char)fcomp.get() * 65536;
        datasize += (unsigned char)fcomp.get() * 256;
        datasize += (unsigned char)fcomp.get() * 1;
        compsize += (unsigned char)fcomp.get() * 16777216;
        compsize += (unsigned char)fcomp.get() * 65536;
        compsize += (unsigned char)fcomp.get() * 256;
        compsize += (unsigned char)fcomp.get() * 1;
        escape = fcomp.get();

        ob.resize(datasize);
        int obpos = 0;

        // start decoding
        matcher_t matcher;
        rc_decoder_t coder(fcomp);

        while (obpos < int(ob.size())) {
            int match_pos = 0;
            int match_len = 1;
            if ((c = ppm.decode(&coder)) != escape) {
                // literal
                ob[obpos] = c;
            } else {
                ppm.update_context(c);
                match_len = ppm.decode(&coder);
                if (match_len == 0) {  // escape
                    match_len = 1;
                    ob[obpos] = escape;
                } else {  // match
                    match_pos = matcher.getpos(ob.data(), obpos);
                    for(auto i = 0; i < match_len; i++) {
                        ob[obpos + i] = ob[match_pos + i];
                    }
                    ppm.m_sse_last_esc = 0;
                }
            }
            for (auto i = 0; i < match_len; i++) {  // update context
                ppm.update_context(ob[obpos]);
                matcher.update(ob.data(), obpos);
                obpos++;
            }
        }
        fdata.write((char*)ob.data(), ob.size());
        //fprintf(stderr, "decode-block: %u <= %u\n", datasize, compsize);
    }
    return 0;
}

#if 0
/*******************************************************************************
 * Main
 ******************************************************************************/
void display_help_message_and_die() {
    fprintf(stderr, "zmolly:\n");
    fprintf(stderr, "  simple LZP/PPM data compressor.\n");
    fprintf(stderr, "  author: Zhang Li <richselian@gmail.com>\n");
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  encode: zmolly [-bx] -e inputFile outputFile\n");
    fprintf(stderr, "  decode: zmolly       -d inputFile outputFile\n");
    fprintf(stderr, "  x: [1..99] optional block size (MB)\n");
    exit(-1);
}

int main(int argc, char** argv) {
    int block_size = 16777216;  // default
    int mode = 0;

    // optional argument: -b
    if (argc > 1 && strncmp(argv[1], "-b", 2) == 0) {
        block_size = 1048576 * atoi(argv[1] + 2);
        if (block_size <= 0 || block_size > 1048576 * 99) {  // valid blocksize = 1..99 MB
            fprintf(stderr, "error: invalid block size: %s.\n", argv[1]);
            display_help_message_and_die();
        }
        argc -= 1;
        memmove(&argv[1], &argv[2], (argc - 1) * sizeof(char*));
    }

    // get mode
    if (argc != 4) {
        fprintf(stderr, "error: invalid number of arguments.\n");
        display_help_message_and_die();
    }
    if (strcmp(argv[1], "-e") == 0) mode = 'e';
    if (strcmp(argv[1], "-d") == 0) mode = 'd';
    if (mode == 0) {
        fprintf(stderr, "error: invalid mode: %s.\n", argv[1]);
        display_help_message_and_die();
    }

    // open input file
    std::ifstream fin(argv[2], std::ios::in | std::ios::binary);
    if (!fin.is_open()) {
        fprintf(stderr, "error: cannot open input file: %s.\n", argv[2]);
        return -1;
    }

    // open output file
    std::ofstream fout(argv[3], std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        fprintf(stderr, "error: cannot open output file: %s.\n", argv[3]);
        return -1;
    }

    // encode/decode
    if (mode == 'e') return zmolly_encode(fin, fout, block_size);
    if (mode == 'd') return zmolly_decode(fin, fout);
}
#endif
