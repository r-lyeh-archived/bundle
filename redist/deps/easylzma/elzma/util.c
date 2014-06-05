#include "util.h"

#ifdef WIN32
#include <stdio.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

int
deleteFile(const char * path)
{
    return unlink(path);
}
