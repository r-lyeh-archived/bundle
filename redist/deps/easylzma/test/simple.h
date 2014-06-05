/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 * 
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 *
 * simple.h - a wrapper around easylzma to compress/decompress to memory 
 */

#ifndef __SIMPLE_H__
#define __SIMPLE_H__

#include "easylzma/compress.h"
#include "easylzma/decompress.h"

/* compress a chunk of memory and return a dynamically allocated buffer
 * if successful.  return value is an easylzma error code */
int simpleCompress(elzma_file_format format,
                   const unsigned char * inData,
                   size_t inLen,
                   unsigned char ** outData,
                   size_t * outLen);

/* decompress a chunk of memory and return a dynamically allocated buffer
 * if successful.  return value is an easylzma error code */
int simpleDecompress(elzma_file_format format,
                     const unsigned char * inData,
                     size_t inLen,
                     unsigned char ** outData,
                     size_t * outLen);

#endif
