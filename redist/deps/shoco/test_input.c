#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "shoco.h"

#define BUFFER_SIZE 1024

static double ratio(int original, int compressed) {
  return ((double)(original - compressed) / (double)original);
}

static int percent(double ratio) {
  return ratio * 100;
}

static void print(char *in, double ratio) {
  printf("'%s' (%d%%)\n", in, percent(ratio));
}

int main(int argc, char ** argv) {
  char buffer[BUFFER_SIZE] = { 0 };
  char comp[BUFFER_SIZE] = { 0 };
  char out[BUFFER_SIZE] = { 0 };
  int count = 0;
  double ratios = 0;
  double rat = 0;
  int inlen, complen, outlen;

  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
    char *end = strchr(buffer, '\n');
    if (end != NULL)
      *end = '\0';
    else
      break;

    inlen = strlen(buffer);
    complen = shoco_compress(buffer, 0, comp, BUFFER_SIZE);
    outlen = shoco_decompress(comp, complen, out, BUFFER_SIZE);
    rat = ratio(inlen, complen);
    if (complen != 0) {
      ++count;
      ratios += rat;
    }
    if ((argc > 1) && ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--verbose") == 0))) {
      print(buffer, rat);
    }
    assert(inlen == outlen);
    assert(strcmp(buffer, out) == 0);
  }
  printf("Number of compressed strings: %d, average compression ratio: %d%%\n", count, percent(ratios / count));
  return 0;
}
