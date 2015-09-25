#include "Shrinker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "lzf.h"
//#include "lz4.h"

#define MB 1048576
#define KB 1024
#define BLOCK_SIZE (1*MB)

int main(int argc,char *argv[])
{
    int methods = 0;
    if (argc<4)
    {
        printf("Shinker Fast&Light Compression Demo by fusiyuan2010@gmail.com\n");
        printf("usage:\n");
        printf("\tcompress:    %s c original_file compressed_file\n", argv[0]);
        printf("\tdecompress:  %s d compressed_file original_file\n", argv[0]);
        return -1;
    }

    char *src=(char*)malloc(BLOCK_SIZE);
    char *dst=(char*)malloc(BLOCK_SIZE + (BLOCK_SIZE >> 6));

    FILE *fin=fopen(argv[2],"rb");
    FILE *fout=fopen(argv[3],"wb");
    if (fin==NULL || fout==NULL)
        return -1;

    long long in_total = 0, out_total = 0;


    if (argv[1][1] != 0) methods = argv[1][1] - '0';
    if (argv[1][0]=='c' || argv[1][0]=='C')
    {
        long long size;
        while((size=fread(src,1,BLOCK_SIZE,fin))>0)
        {
            long long outsize;
            switch(methods)
            {
            case 0:
                outsize = shrinker_compress(src,dst,size);
                break;
            case 1:
                //outsize = lzf_compress(src, size, dst, size - 1);
                break;
            case 2:
                //outsize = LZ4_compress(src, dst, size);
                break;
            default:
                outsize = shrinker_compress(src,dst,size);
                outsize = size;
                break;
            }
            if (outsize < 0 || outsize >= size)
            {
                outsize = size;
                fwrite(&size,8,1,fout);
                fwrite(&size,8,1,fout);
                fwrite(src,1,size,fout);
            }
            else
            {
                fwrite(&outsize,8,1,fout);
                fwrite(&size,8,1,fout);
                fwrite(dst,1,outsize,fout);
            }
            in_total += size;
            out_total += outsize;
            printf("\r%lld  =>  %lld", in_total, out_total);
        }

    }
    else if (argv[1][0]=='d' || argv[1][0]=='D')
    {
        long long size;
        long long readsize;
        long long srcsize;
        while((size=fread(&readsize,8,1,fin))>0)
        {
            fread(&srcsize,8, 1,fin);
            fread(src,1,readsize,fin);
            int outsize;
            if (readsize == srcsize) {
                fwrite(src,1,readsize,fout);
            }
            else {
            switch(methods)
            {
            case 0:
                outsize = shrinker_decompress(src,dst,srcsize);
                outsize = srcsize;
                break;
            case 1:
                //outsize = lzf_decompress(src, MB, dst, srcsize);
                outsize = srcsize;
                break;
            case 2:
                //outsize = LZ4_uncompress(src, dst, srcsize);
                outsize = srcsize;
                break;
            default:
                outsize = shrinker_decompress(src,dst,srcsize);
                outsize = srcsize;
                break;
            }
            fwrite(dst,1,outsize,fout);
            }
            in_total += readsize;
            out_total += srcsize;
            printf("\r%lld  =>  %lld", in_total, out_total);
        }
    }
    else 
        return -1;
    printf("\n");

    fclose(fin);
    fclose(fout);
    free(src);
    free(dst);
    return 0;
}
