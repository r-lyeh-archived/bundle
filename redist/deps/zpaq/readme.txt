zpaq652.zip, June 6, 2014.

zpaq is a journaling archiver optimized for user-level incremental
backup of directory trees in Windows and *nix. It supports AES-256
encryption, 6 multi-threaded compression levels, and content-aware
file fragment level deduplication. For backups it adds only files
whose date has changed, and keeps both old and new versions. You can roll
back the archive date to restore from old versions of the archive.
The default compression level is faster than zip usually with better
compression. zpaq uses a self-describing compressed format to allow
for future improvements without breaking compatibility with older
versions of the program. Contents:

File          Ver.  License          Description
-----------   ----  -------          -----------
zpaq.exe      6.52  GPL v3           Archiver, 32 bit Windows.
zpaq64.exe    6.52  GPL v3           Archiver, 64 bit Windows.
zpaq.cpp      6.52  GPL v3           zpaq user's guide and source code.
libzpaq.h     6.51  Public Domain    libzpaq API documentation and header.
libzpaq.cpp   6.52  Public Domain    libzpaq API source code.
divsufsort.h  2.00  MIT              libdivsufsoft-lite header.
divsufsort.c  2.00  MIT              libdivsufsort-lite source code.
Makefile            Public Domain    To compile in Linux: make

All versions of this software can be found at
http://mattmahoney.net/dc/zpaq.html
Please report bugs to Matt Mahoney at mattmahoneyfl@gmail.com

zpaq is (C) 2011-2014, Dell Inc., written by Matt Mahoney.
Licensed under GPL v3. http://www.gnu.org/copyleft/gpl.html
zpaq is a journaling archiver for compression and incremental backups.

libzpaq is public domain, written by Matt Mahoney.
It is an API in C++ providing streaming compression and decompression
services in the ZPAQ format. It has some encryption functions (SHA1,
SHA256, AES encryption, and Scrypt). See libzpaq.h for documentation.

divsufsort is (C) 2003-2008 Yuta Mori, MIT license (see source code).
It is mirrored from libdivsufsort-lite 2.0 from
http://code.google.com/p/libdivsufsort/

zpaq.exe can run under either 32 or 64 bit Windows. zpaq64.exe runs only
under 64 bit Windows. The 32 bit version defaults to using at most
4 cores (you can select more) and can only use 3 GB memory. In most
cases this makes no difference.

zpaq is a command line program. For a brief description of the commands,
type "zpaq" with no arguments. Full documenation is in zpaq.cpp.
(You do not have to read the code. I just put it there for convenience).


TO COMPILE

Normally you can use "make" to compile for Unix, Linux, or Mac OS/X.

zpaq for Windows was compiled with MinGW g++ 4.8.1 (MinGW-W64 project,
rev5, sjlj, Windows threads) and compressed with upx 3.08w as follows:

  g++ -O3 -s -m64 -static -DNDEBUG zpaq.cpp libzpaq.cpp divsufsort.c -o zpaq64
  g++ -O3 -s -m32 -static -DNDEBUG zpaq.cpp libzpaq.cpp divsufsort.c -o zpaq
  upx zpaq.exe

To compile using Visual Studio:
(tested with ver. 10.0 (2010), cl version 16.00.30319.01 for 80x86)

  cl /O2 /EHsc /DNDEBUG zpaq.cpp libzpaq.cpp divsufsort.c advapi32.lib

For Linux or Mac OS/X:

  make
or
  g++ -O3 -Dunix -DNDEBUG zpaq.cpp libzpaq.cpp divsufsort.c -pthread -o zpaq

Options have the following meanings:

-Dunix   = select Unix or Linux target in zpaq and libzpaq. The default is
           Windows. Some Linux compilers automatically define unix.
-DNDEBUG = turn off run time checks in divsufsort.
-DDEBUG  = turn on run time checks in zpaq and libzpaq.
-DNOJIT  = turn off run time optimization of ZPAQL to 32 or 64 bit x86
           in libzpaq. Use this for a non-x86 processor, or old
           processors not supporting SSE2 (mostly before 2001).
-pthread = link to pthread library (required in unix/Linux).

General options:

-O3 or /O2   = optimize for speed.
/EHsc        = enable C++ exception handling (VC++).
-s           = strip debugging symbols. (Some compilers ignore this).
-m32 or -m64 = select a 32 or 64 bit executable.
-static      = use this if you plan to run the program on a different
               machine than you compiled it on. Makes the executable bigger.

upx compresses 32 bit Windows executables. It is not required. It will
not work on 64 bit executables.
