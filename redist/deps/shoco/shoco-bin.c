#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shoco.h"

static const char USAGE[] = "compresses or decompresses your (presumably short) data.\n"
"usage: shoco {c(ompress),d(ecompress)} <file-to-(de)compress> <output-filename>\n";

typedef enum {
  JOB_COMPRESS,
  JOB_DECOMPRESS,
} Job;

#define MAX_STACK_ALLOCATION_SIZE 65536

int main(int argc, char **argv) {
  Job job; 
  unsigned long in_size;
  char *in_buffer;
  char *out_buffer;
  FILE *fin;
  FILE *fout;
  int len;

  if (argc < 4) {
    puts(USAGE);
    return 1;
  }
  if (argv[1][0] == 'c')
    job = JOB_COMPRESS;
  else if (argv[1][0] == 'd')
    job = JOB_DECOMPRESS;
  else {
    puts(USAGE);
    return 1;
  }
  char *infile = argv[2];
  char *outfile = argv[3];

  fin = fopen (infile, "rb" );
  if (fin == NULL) {
    fputs("Something went wrong opening the file. Does it even exist?", stderr);
    exit(1);
  }

  // obtain file size:
  fseek(fin, 0, SEEK_END);
  in_size = ftell(fin);
  rewind(fin);

  if (in_size > MAX_STACK_ALLOCATION_SIZE) {
    in_buffer = (char *)malloc(sizeof(char) * in_size);
    out_buffer = (char *)malloc(sizeof(char) * in_size * 4);
    if ((in_buffer == NULL) || (out_buffer == NULL)) {
      fputs("Memory error. This really shouldn't happen.", stderr);
      exit(2);
    }
  } else {
    in_buffer = (char *)alloca(sizeof(char) * in_size);
    out_buffer = (char *)alloca(sizeof(char) * in_size * 4);
  }

  if (fread(in_buffer, sizeof(char), in_size, fin) != in_size) {
    fputs("Error reading the input file.", stderr);
    exit(3);
  }
  fclose(fin);

  if (job == JOB_COMPRESS)
    len = shoco_compress(in_buffer, in_size, out_buffer, in_size * 4);
  else
    len = shoco_decompress(in_buffer, in_size, out_buffer, in_size * 4);

  fout = fopen(outfile, "wb");
  fwrite(out_buffer , sizeof(char), len, fout);
  fclose(fout);

  if (in_size > MAX_STACK_ALLOCATION_SIZE) {
    free(in_buffer);
    free(out_buffer);
  }

  return 0;
}
