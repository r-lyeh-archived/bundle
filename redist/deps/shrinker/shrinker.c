#include "Shrinker.h"

#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;
#else
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

//the code from LZ4
#if (GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
# define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
# define expect(expr,value)    (expr)
#endif
#define likely(expr)     expect((expr) != 0, 1)
#define unlikely(expr)   expect((expr) != 0, 0)
////////////////////

#define HASH_BITS 15
#define HASH(a) ((a*21788233) >> (32 - HASH_BITS))
#define MINMATCH 4

#include <limits.h>
#if ULONG_MAX == UINT_MAX //32bit
#define MEMCPY_NOOVERLAP(a, b, c) do{do{memcpy(a, b, 4);a+=4;b+=4;}while(b<c);a-=(b-c);b=c;}while(0)
#define MEMCPY_NOOVERLAP_NOSURPASS(a, b, c) do{c-=4;while(b<c){memcpy(a, b, 4);a+=4;b+=4;}c+=4;while(b<c)*a++=*b++;}while(0)
#define MEMCPY(a, b, c) do{if (likely(a>b+4)) MEMCPY_NOOVERLAP(a, b, c); else while(b<c) *a++=*b++;}while(0)
#else
#define MEMCPY_NOOVERLAP(a, b, c) do{do{memcpy(a, b, 8);a+=8;b+=8;}while(b<c);a-=(b-c);b=c;}while(0)
#define MEMCPY_NOOVERLAP_NOSURPASS(a, b, c) do{c-=8;while(b<c){memcpy(a, b, 8);a+=8;b+=8;}c+=8;while(b<c)*a++=*b++;}while(0)
#define MEMCPY(a, b, c) do{if (likely(a>b+8)) MEMCPY_NOOVERLAP(a, b, c); else while(b<c) *a++=*b++;}while(0)
#endif

inline u16 unaligned_load_u16(void* ptr)
{
  u16 tempU16;
  memcpy(&tempU16, ptr, 2);
  return tempU16;
}

inline u32 unaligned_load_u32(void* ptr)
{
  u32 tempU32;
  memcpy(&tempU32, ptr, 4);
  return tempU32;
}

int shrinker_compress(void *in, void *out, int size)
{
    u32* ht = (u32*)malloc((1 << HASH_BITS)*4);
    u8 *src = (u8*)in, *dst = (u8*)out;
    u8 *src_end = src + size - MINMATCH - 8;
    u8 *dst_end = dst + size - MINMATCH - 8;
    u8 *p_last_lit = src;
    u32 cur_hash, len, match_dist;
    u8 flag, *pflag, cache;

    if (size < 32 || size > (1 << 27) - 1)
    {
        free(ht);
        return -1;
    }
    memset(ht, 0, (1 << HASH_BITS)*4);

    while(likely(src < src_end) && likely(dst < dst_end))
    {
        u32 u32val, distance = src - (u8*)in;
        u8 *pfind, *pcur;
        pcur = src;
        u32val = unaligned_load_u32(pcur);
        cur_hash = HASH(u32val);
        cache = ht[cur_hash] >> 27;
        pfind = (u8*)in + (ht[cur_hash] & 0x07ffffff);
        ht[cur_hash] = distance|(*src<<27);

        if (unlikely(cache == (*pcur & 0x1f))
            && pfind + 0xffff >= (u8*)pcur
            && pfind < pcur
            && unaligned_load_u32(pfind) == unaligned_load_u32(pcur))
        {  
            pfind += 4; pcur += 4;
            while(likely(pcur < src_end) && unaligned_load_u32(pfind) == unaligned_load_u32(pcur))
            { pfind += 4; pcur += 4;}
            if (likely(pcur < src_end))
            if (unaligned_load_u16(pfind) == unaligned_load_u16(pcur)) {pfind += 2; pcur += 2;}
            if (*pfind == *pcur) {pfind++; pcur++;}

            pflag = dst++;
            len = src - p_last_lit;
            if (likely(len < 7)) flag = len << 5;
            else {
                len -= 7;flag = (7<<5);
                while (len >= 255) { *dst++ = 255;len-= 255;}
                *dst++ = len;
            }

            len = pcur - src  - MINMATCH;
            if (likely(len < 15))  flag |= len;
            else {
                len -= 15; flag |= 15;
                while (len >= 255) { *dst++ = 255;len -= 255;}
                *dst++ = len;
            }
            match_dist = pcur - pfind - 1;
            *pflag = flag;
            *dst++ = match_dist & 0xff;
            if (match_dist > 0xff) {
                *pflag |= 0x10;
                *dst++ = match_dist >> 8;
            }
            MEMCPY_NOOVERLAP(dst, p_last_lit, src);

            u32val = unaligned_load_u32(src+1); ht[HASH(u32val)] = (src - (u8*)in + 1)|(*(src+1)<<27);
            u32val = unaligned_load_u32(src+3); ht[HASH(u32val)] = (src - (u8*)in + 3)|(*(src+3)<<27);
            p_last_lit = src = pcur;
            continue;
        }
        src++;
    }

    if (dst - (u8*)out + 3 >= src - (u8*)in)
    {
        free(ht);
        return -1;
    }
    src = (u8*)in + size;
    pflag = dst++;
    len = src - p_last_lit;
    if (likely(len < 7)) 
        flag = len << 5;
    else {
        len -= 7; flag = (7<<5);
        while (len >= 255) { *dst++ = 255; len -= 255;}
        *dst++ = len;
    }

    flag |= 0x10; // any number
    *pflag = flag;
    *dst++ = 0xff; *dst++ = 0xff;
    MEMCPY_NOOVERLAP_NOSURPASS(dst, p_last_lit, src);

    free(ht);
    if (dst > dst_end) return -1;
    else return dst - (u8*)out;
}



int shrinker_decompress(void *in, void *out, int size)
{
    u8 *src = (u8*)in, *dst = (u8*)out;
    u8 *end = dst + size;
    u8 *pcpy, *pend;
    u8 flag, long_dist;
    u32 literal_len;
    u32 match_len, match_dist;

    for(;;) {
        flag = *src++;
        literal_len = flag >> 5;
        match_len = flag & 0xf;
        long_dist = flag & 0x10;

        if (unlikely(literal_len == 7)) {
            while((flag = *src++) == 255)
                literal_len += 255;
            literal_len += flag;
        }

        if (unlikely(match_len == 15)) {
            while((flag = *src++) == 255)
                match_len += 255;
            match_len += flag;
        }

        match_dist = *src++;
        if (long_dist) 
        {
            match_dist |= ((*src++) << 8);
            if (unlikely(match_dist == 0xffff)) {
                pend = src + literal_len;
                if (unlikely(dst + literal_len > end)) return -1;
                MEMCPY_NOOVERLAP_NOSURPASS(dst, src, pend);
                break;
            }
        }

        pend = src + literal_len;
        if (unlikely(dst + literal_len > end)) return -1;
        MEMCPY_NOOVERLAP(dst, src, pend);
        pcpy = dst - match_dist - 1;
        pend = pcpy + match_len + MINMATCH;
        if (unlikely(pcpy < (u8*)out || dst + match_len + MINMATCH > end)) return -1;
        MEMCPY(dst, pcpy, pend);
    }
    return dst - (u8*)out;
}