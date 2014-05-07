/*  Minilzip - Test program for the lzlib library
    Copyright (C) 2009, 2010, 2011, 2012, 2013 Antonio Diaz Diaz.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
    Exit status: 0 for a normal exit, 1 for environmental problems
    (file not found, invalid flags, I/O errors, etc), 2 to indicate a
    corrupt or invalid input file, 3 for an internal consistency error
    (eg, bug) which caused minilzip to panic.
*/

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#if defined(__MSVCRT__)
#include <io.h>
#define fchmod(x,y) 0
#define fchown(x,y,z) 0
#define strtoull strtoul
#define SIGHUP SIGTERM
#define S_ISSOCK(x) 0
#define S_IRGRP 0
#define S_IWGRP 0
#define S_IROTH 0
#define S_IWOTH 0
#endif
#if defined(__OS2__)
#include <io.h>
#endif

#include "carg_parser.h"
#include "lzlib.h"

#if CHAR_BIT != 8
#error "Environments where CHAR_BIT != 8 are not supported."
#endif

#ifndef max
  #define max(x,y) ((x) >= (y) ? (x) : (y))
#endif
#ifndef min
  #define min(x,y) ((x) <= (y) ? (x) : (y))
#endif

void cleanup_and_fail( const int retval );
void show_error( const char * const msg, const int errcode, const bool help );
void internal_error( const char * const msg );


const char * const Program_name = "Minilzip";
const char * const program_name = "minilzip";
const char * const program_year = "2013";
const char * invocation_name = 0;

#ifdef O_BINARY
const int o_binary = O_BINARY;
#else
const int o_binary = 0;
#endif

struct { const char * from; const char * to; } const known_extensions[] = {
  { ".lz",  ""     },
  { ".tlz", ".tar" },
  { 0,      0      } };

struct Lzma_options
  {
  int dictionary_size;		/* 4 KiB .. 512 MiB */
  int match_len_limit;		/* 5 .. 273 */
  };

enum Mode { m_compress, m_decompress, m_test };

char * output_filename = 0;
int outfd = -1;
int verbosity = 0;
const mode_t usr_rw = S_IRUSR | S_IWUSR;
const mode_t all_rw = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
mode_t outfd_mode = S_IRUSR | S_IWUSR;
bool delete_output_on_interrupt = false;


struct Pretty_print
  {
  const char * name;
  const char * stdin_name;
  int longest_name;
  bool first_post;
  };

static inline void Pp_set_name( struct Pretty_print * const pp,
                                const char * const filename )
  {
  if( filename && filename[0] && strcmp( filename, "-" ) != 0 )
    pp->name = filename;
  else pp->name = pp->stdin_name;
  pp->first_post = true;
  }


static inline void Pp_reset( struct Pretty_print * const pp )
  { if( pp->name && pp->name[0] ) pp->first_post = true; }


static void Pp_show_msg( struct Pretty_print * const pp, const char * const msg )
  {
  if( verbosity >= 0 )
    {
    if( pp->first_post )
      {
      int i, len;
      pp->first_post = false;
      fprintf( stderr, "  %s: ", pp->name );
      len = pp->longest_name - strlen( pp->name );
      for( i = 0; i < len; ++i ) fprintf( stderr, " " );
      if( !msg ) fflush( stderr );
      }
    if( msg ) fprintf( stderr, "%s.\n", msg );
    }
  }


static void show_help( void )
  {
  printf( "%s - Test program for the lzlib library.\n", Program_name );
  printf( "\nUsage: %s [options] [files]\n", invocation_name );
  printf( "\nOptions:\n"
          "  -h, --help                     display this help and exit\n"
          "  -V, --version                  output version information and exit\n"
          "  -b, --member-size=<bytes>      set member size limit in bytes\n"
          "  -c, --stdout                   send output to standard output\n"
          "  -d, --decompress               decompress\n"
          "  -f, --force                    overwrite existing output files\n"
          "  -F, --recompress               force recompression of compressed files\n"
          "  -k, --keep                     keep (don't delete) input files\n"
          "  -m, --match-length=<bytes>     set match length limit in bytes [36]\n"
          "  -o, --output=<file>            if reading stdin, place the output into <file>\n"
          "  -q, --quiet                    suppress all messages\n"
          "  -s, --dictionary-size=<bytes>  set dictionary size limit in bytes [8 MiB]\n"
          "  -S, --volume-size=<bytes>      set volume size limit in bytes\n"
          "  -t, --test                     test compressed file integrity\n"
          "  -v, --verbose                  be verbose (a 2nd -v gives more)\n"
          "  -1 .. -9                       set compression level [default 6]\n"
          "      --fast                     alias for -1\n"
          "      --best                     alias for -9\n"
          "If no file names are given, minilzip compresses or decompresses\n"
          "from standard input to standard output.\n"
          "Numbers may be followed by a multiplier: k = kB = 10^3 = 1000,\n"
          "Ki = KiB = 2^10 = 1024, M = 10^6, Mi = 2^20, G = 10^9, Gi = 2^30, etc...\n"
          "The bidimensional parameter space of LZMA can't be mapped to a linear\n"
          "scale optimal for all files. If your files are large, very repetitive,\n"
          "etc, you may need to use the --match-length and --dictionary-size\n"
          "options directly to achieve optimal performance. For example, -9m64\n"
          "usually compresses executables more (and faster) than -9.\n"
          "\nExit status: 0 for a normal exit, 1 for environmental problems (file\n"
          "not found, invalid flags, I/O errors, etc), 2 to indicate a corrupt or\n"
          "invalid input file, 3 for an internal consistency error (eg, bug) which\n"
          "caused minilzip to panic.\n"
          "\nReport bugs to lzip-bug@nongnu.org\n"
          "Lzlib home page: http://www.nongnu.org/lzip/lzlib.html\n" );
  }


static void show_version( void )
  {
  printf( "%s %s\n", Program_name, PROGVERSION );
  printf( "Copyright (C) %s Antonio Diaz Diaz.\n", program_year );
  printf( "Using Lzlib %s\n", LZ_version() );
  printf( "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
          "This is free software: you are free to change and redistribute it.\n"
          "There is NO WARRANTY, to the extent permitted by law.\n" );
  }


static void show_header( struct LZ_Decoder * const decoder )
  {
  const char * const prefix[8] =
    { "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi" };
  enum { factor = 1024 };
  const char * p = "";
  const char * np = "  ";
  unsigned num = LZ_decompress_dictionary_size( decoder ), i;
  bool exact = ( num % factor == 0 );

  for( i = 0; i < 8 && ( num > 9999 || ( exact && num >= factor ) ); ++i )
    { num /= factor; if( num % factor != 0 ) exact = false;
      p = prefix[i]; np = ""; }
  fprintf( stderr, "dictionary size %s%4u %sB.  ", np, num, p );
  }


static unsigned long long getnum( const char * const ptr,
                                  const unsigned long long llimit,
                                  const unsigned long long ulimit )
  {
  unsigned long long result;
  char * tail;
  errno = 0;
  result = strtoull( ptr, &tail, 0 );
  if( tail == ptr )
    {
    show_error( "Bad or missing numerical argument.", 0, true );
    exit( 1 );
    }

  if( !errno && tail[0] )
    {
    int factor = ( tail[1] == 'i' ) ? 1024 : 1000;
    int exponent = 0, i;
    bool bad_multiplier = false;
    switch( tail[0] )
      {
      case ' ': break;
      case 'Y': exponent = 8; break;
      case 'Z': exponent = 7; break;
      case 'E': exponent = 6; break;
      case 'P': exponent = 5; break;
      case 'T': exponent = 4; break;
      case 'G': exponent = 3; break;
      case 'M': exponent = 2; break;
      case 'K': if( factor == 1024 ) exponent = 1; else bad_multiplier = true;
                break;
      case 'k': if( factor == 1000 ) exponent = 1; else bad_multiplier = true;
                break;
      default : bad_multiplier = true;
      }
    if( bad_multiplier )
      {
      show_error( "Bad multiplier in numerical argument.", 0, true );
      exit( 1 );
      }
    for( i = 0; i < exponent; ++i )
      {
      if( ulimit / factor >= result ) result *= factor;
      else { errno = ERANGE; break; }
      }
    }
  if( !errno && ( result < llimit || result > ulimit ) ) errno = ERANGE;
  if( errno )
    {
    show_error( "Numerical argument out of limits.", 0, false );
    exit( 1 );
    }
  return result;
  }


static int get_dict_size( const char * const arg )
  {
  char * tail;
  int bits = strtol( arg, &tail, 0 );
  if( bits >= LZ_min_dictionary_bits() &&
      bits <= LZ_max_dictionary_bits() && *tail == 0 )
    return ( 1 << bits );
  return getnum( arg, LZ_min_dictionary_size(), LZ_max_dictionary_size() );
  }


static int extension_index( const char * const name )
  {
  int i;
  for( i = 0; known_extensions[i].from; ++i )
    {
    const char * const ext = known_extensions[i].from;
    if( strlen( name ) > strlen( ext ) &&
        strncmp( name + strlen( name ) - strlen( ext ), ext, strlen( ext ) ) == 0 )
      return i;
    }
  return -1;
  }


static int open_instream( const char * const name, struct stat * const in_statsp,
                          const enum Mode program_mode, const int eindex,
                          const bool recompress, const bool to_stdout )
  {
  int infd = -1;
  if( program_mode == m_compress && !recompress && eindex >= 0 )
    {
    if( verbosity >= 0 )
      fprintf( stderr, "%s: Input file '%s' already has '%s' suffix.\n",
               program_name, name, known_extensions[eindex].from );
    }
  else
    {
    infd = open( name, O_RDONLY | o_binary );
    if( infd < 0 )
      {
      if( verbosity >= 0 )
        fprintf( stderr, "%s: Can't open input file '%s': %s.\n",
                 program_name, name, strerror( errno ) );
      }
    else
      {
      const int i = fstat( infd, in_statsp );
      const mode_t mode = in_statsp->st_mode;
      const bool can_read = ( i == 0 &&
                              ( S_ISBLK( mode ) || S_ISCHR( mode ) ||
                                S_ISFIFO( mode ) || S_ISSOCK( mode ) ) );
      const bool no_ofile = to_stdout || program_mode == m_test;
      if( i != 0 || ( !S_ISREG( mode ) && ( !can_read || !no_ofile ) ) )
        {
        if( verbosity >= 0 )
          fprintf( stderr, "%s: Input file '%s' is not a regular file%s.\n",
                   program_name, name,
                   ( can_read && !no_ofile ) ?
                   " and '--stdout' was not specified" : "" );
        close( infd );
        infd = -1;
        }
      }
    }
  return infd;
  }


/* assure at least a minimum size for buffer 'buf' */
static void * resize_buffer( void * buf, const int min_size )
  {
  if( buf ) buf = realloc( buf, min_size );
  else buf = malloc( min_size );
  if( !buf )
    {
    show_error( "Not enough memory.", 0, false );
    cleanup_and_fail( 1 );
    }
  return buf;
  }


static void set_c_outname( const char * const name, const bool multifile )
  {
  output_filename = resize_buffer( output_filename, strlen( name ) + 5 +
                                   strlen( known_extensions[0].from ) + 1 );
  strcpy( output_filename, name );
  if( multifile ) strcat( output_filename, "00001" );
  strcat( output_filename, known_extensions[0].from );
  }


static void set_d_outname( const char * const name, const int i )
  {
  if( i >= 0 )
    {
    const char * const from = known_extensions[i].from;
    if( strlen( name ) > strlen( from ) )
      {
      output_filename = resize_buffer( output_filename, strlen( name ) +
                                       strlen( known_extensions[0].to ) + 1 );
      strcpy( output_filename, name );
      strcpy( output_filename + strlen( name ) - strlen( from ),
              known_extensions[i].to );
      return;
      }
    }
  output_filename = resize_buffer( output_filename, strlen( name ) + 4 + 1 );
  strcpy( output_filename, name );
  strcat( output_filename, ".out" );
  if( verbosity >= 1 )
    fprintf( stderr, "%s: Can't guess original name for '%s' -- using '%s'.\n",
             program_name, name, output_filename );
  }


static bool open_outstream( const bool force )
  {
  int flags = O_CREAT | O_WRONLY | o_binary;
  if( force ) flags |= O_TRUNC; else flags |= O_EXCL;

  outfd = open( output_filename, flags, outfd_mode );
  if( outfd < 0 && verbosity >= 0 )
    {
    if( errno == EEXIST )
      fprintf( stderr, "%s: Output file '%s' already exists, skipping.\n",
               program_name, output_filename );
    else
      fprintf( stderr, "%s: Can't create output file '%s': %s.\n",
               program_name, output_filename, strerror( errno ) );
    }
  return ( outfd >= 0 );
  }


static bool check_tty( const int infd, const enum Mode program_mode )
  {
  if( program_mode == m_compress && outfd >= 0 && isatty( outfd ) )
    {
    show_error( "I won't write compressed data to a terminal.", 0, true );
    return false;
    }
  if( ( program_mode == m_decompress || program_mode == m_test ) &&
      isatty( infd ) )
    {
    show_error( "I won't read compressed data from a terminal.", 0, true );
    return false;
    }
  return true;
  }


void cleanup_and_fail( const int retval )
  {
  if( delete_output_on_interrupt )
    {
    delete_output_on_interrupt = false;
    if( verbosity >= 0 )
      fprintf( stderr, "%s: Deleting output file '%s', if it exists.\n",
               program_name, output_filename );
    if( outfd >= 0 ) { close( outfd ); outfd = -1; }
    if( remove( output_filename ) != 0 && errno != ENOENT )
      show_error( "WARNING: deletion of output file (apparently) failed.", 0, false );
    }
  exit( retval );
  }


     /* Set permissions, owner and times. */
static void close_and_set_permissions( const struct stat * const in_statsp )
  {
  bool warning = false;
  if( in_statsp )
    {
    /* fchown will in many cases return with EPERM, which can be safely ignored. */
    if( ( fchown( outfd, in_statsp->st_uid, in_statsp->st_gid ) != 0 &&
          errno != EPERM ) ||
        fchmod( outfd, in_statsp->st_mode ) != 0 ) warning = true;
    }
  if( close( outfd ) != 0 ) cleanup_and_fail( 1 );
  outfd = -1;
  delete_output_on_interrupt = false;
  if( in_statsp )
    {
    struct utimbuf t;
    t.actime = in_statsp->st_atime;
    t.modtime = in_statsp->st_mtime;
    if( utime( output_filename, &t ) != 0 ) warning = true;
    }
  if( warning && verbosity >= 1 )
    show_error( "Can't change output file attributes.", 0, false );
  }


/* Returns the number of bytes really read.
   If (returned value < size) and (errno == 0), means EOF was reached.
*/
static int readblock( const int fd, uint8_t * const buf, const int size )
  {
  int rest = size;
  errno = 0;
  while( rest > 0 )
    {
    const int n = read( fd, buf + size - rest, rest );
    if( n > 0 ) rest -= n;
    else if( n == 0 ) break;				/* EOF */
    else if( errno != EINTR && errno != EAGAIN ) break;
    errno = 0;
    }
  return size - rest;
  }


/* Returns the number of bytes really written.
   If (returned value < size), it is always an error.
*/
static int writeblock( const int fd, const uint8_t * const buf, const int size )
  {
  int rest = size;
  errno = 0;
  while( rest > 0 )
    {
    const int n = write( fd, buf + size - rest, rest );
    if( n > 0 ) rest -= n;
    else if( n < 0 && errno != EINTR && errno != EAGAIN ) break;
    errno = 0;
    }
  return size - rest;
  }


static bool next_filename( void )
  {
  const unsigned len = strlen( known_extensions[0].from );
  int i, j;

  if( strlen( output_filename ) >= len + 5 )		/* "*00001.lz" */
    for( i = strlen( output_filename ) - len - 1, j = 0; j < 5; --i, ++j )
      {
      if( output_filename[i] < '9' ) { ++output_filename[i]; return true; }
      else output_filename[i] = '0';
      }
  return false;
  }


static int do_compress( struct LZ_Encoder * const encoder,
                        const unsigned long long member_size,
                        const unsigned long long volume_size, const int infd,
                        struct Pretty_print * const pp,
                        const struct stat * const in_statsp )
  {
  unsigned long long partial_volume_size = 0;
  enum { buffer_size = 65536 };
  uint8_t buffer[buffer_size];

  if( verbosity >= 1 ) Pp_show_msg( pp, 0 );
  while( true )
    {
    int in_size = 0, out_size;
    while( LZ_compress_write_size( encoder ) > 0 )
      {
      const int size = min( LZ_compress_write_size( encoder ), buffer_size );
      const int rd = readblock( infd, buffer, size );
      if( rd != size && errno )
        {
        Pp_show_msg( pp, 0 ); show_error( "Read error", errno, false );
        return 1;
        }
      if( rd > 0 && rd != LZ_compress_write( encoder, buffer, rd ) )
        internal_error( "library error (LZ_compress_write)" );
      if( rd < size ) LZ_compress_finish( encoder );
/*      else LZ_compress_sync_flush( encoder ); */
      in_size += rd;
      }
    out_size = LZ_compress_read( encoder, buffer, buffer_size );
    if( out_size < 0 )
      {
      Pp_show_msg( pp, 0 );
      if( verbosity >= 0 )
        fprintf( stderr, "%s: LZ_compress_read error: %s.\n",
                 program_name, LZ_strerror( LZ_compress_errno( encoder ) ) );
        return 1;
      }
    else if( out_size > 0 )
      {
      const int wr = writeblock( outfd, buffer, out_size );
      if( wr != out_size )
        {
        Pp_show_msg( pp, 0 ); show_error( "Write error", errno, false );
        return 1;
        }
      }
    else if( in_size == 0 ) internal_error( "library error (LZ_compress_read)" );
    if( LZ_compress_member_finished( encoder ) )
      {
      unsigned long long size;
      if( LZ_compress_finished( encoder ) == 1 ) break;
      if( volume_size > 0 )
        {
        partial_volume_size += LZ_compress_member_position( encoder );
        if( partial_volume_size >= volume_size - LZ_min_dictionary_size() )
          {
          partial_volume_size = 0;
          if( delete_output_on_interrupt )
            {
            close_and_set_permissions( in_statsp );
            if( !next_filename() )
              { Pp_show_msg( pp, "Too many volume files" ); return 1; }
            if( !open_outstream( true ) ) return 1;
            delete_output_on_interrupt = true;
            }
          }
        size = min( member_size, volume_size - partial_volume_size );
        }
      else
        size = member_size;
      if( LZ_compress_restart_member( encoder, size ) < 0 )
        {
        Pp_show_msg( pp, 0 );
        if( verbosity >= 0 )
          fprintf( stderr, "%s: LZ_compress_restart_member error: %s.\n",
                   program_name, LZ_strerror( LZ_compress_errno( encoder ) ) );
        return 1;
        }
      }
    }

  if( verbosity >= 1 )
    {
    const unsigned long long in_size = LZ_compress_total_in_size( encoder );
    const unsigned long long out_size = LZ_compress_total_out_size( encoder );
    if( in_size == 0 || out_size == 0 )
      fprintf( stderr, " no data compressed.\n" );
    else
      fprintf( stderr, "%6.3f:1, %6.3f bits/byte, "
                       "%5.2f%% saved, %llu in, %llu out.\n",
               (double)in_size / out_size,
               ( 8.0 * out_size ) / in_size,
               100.0 * ( 1.0 - ( (double)out_size / in_size ) ),
               in_size, out_size );
    }
  return 0;
  }


int compress( const unsigned long long member_size,
              const unsigned long long volume_size,
              const struct Lzma_options * const encoder_options,
              const int infd, struct Pretty_print * const pp,
              const struct stat * const in_statsp )
  {
  struct LZ_Encoder * const encoder =
    LZ_compress_open( encoder_options->dictionary_size,
                      encoder_options->match_len_limit, ( volume_size > 0 ) ?
                      min( member_size, volume_size ) : member_size );
  int retval;

  if( !encoder || LZ_compress_errno( encoder ) != LZ_ok )
    {
    if( !encoder || LZ_compress_errno( encoder ) == LZ_mem_error )
      Pp_show_msg( pp, "Not enough memory. Try a smaller dictionary size" );
    else
      internal_error( "invalid argument to encoder" );
    retval = 1;
    }
  else retval = do_compress( encoder, member_size, volume_size,
                             infd, pp, in_statsp );
  LZ_compress_close( encoder );
  return retval;
  }


int do_decompress( struct LZ_Decoder * const decoder, const int infd,
                   struct Pretty_print * const pp, const bool testing )
  {
  enum { buffer_size = 65536 };
  uint8_t buffer[buffer_size];
  bool first_member;

  for( first_member = true; ; )
    {
    int in_size = min( LZ_decompress_write_size( decoder ), buffer_size );
    int out_size = 0;
    if( in_size > 0 )
      {
      const int max_in_size = in_size;
      in_size = readblock( infd, buffer, max_in_size );
      if( in_size != max_in_size && errno )
        {
        Pp_show_msg( pp, 0 ); show_error( "Read error", errno, false );
        return 1;
        }
      if( in_size > 0 && in_size != LZ_decompress_write( decoder, buffer, in_size ) )
        internal_error( "library error (LZ_decompress_write)" );
      if( in_size < max_in_size ) LZ_decompress_finish( decoder );
      }
    while( true )
      {
      const int rd = LZ_decompress_read( decoder, buffer, buffer_size );
      if( rd > 0 )
        {
        out_size += rd;
        if( outfd >= 0 )
          {
          const int wr = writeblock( outfd, buffer, rd );
          if( wr != rd )
            {
            Pp_show_msg( pp, 0 ); show_error( "Write error", errno, false );
            return 1;
            }
          }
        }
      else if( rd < 0 ) { out_size = rd; break; }
      if( LZ_decompress_member_finished( decoder ) == 1 )
        {
        if( verbosity >= 1 )
          {
          const unsigned long long data_position = LZ_decompress_data_position( decoder );
          const unsigned long long member_size = LZ_decompress_member_position( decoder );
          Pp_show_msg( pp, 0 );
          if( verbosity >= 3 ) show_header( decoder );
          if( verbosity >= 2 && data_position > 0 && member_size > 0 )
            fprintf( stderr, "%6.3f:1, %6.3f bits/byte, %5.2f%% saved.  ",
                     (double)data_position / member_size,
                     ( 8.0 * member_size ) / data_position,
                     100.0 * ( 1.0 - ( (double)member_size / data_position ) ) );
          if( verbosity >= 4 )
            fprintf( stderr, "data CRC %08X, data size %9llu, member size %8llu.  ",
                     LZ_decompress_data_crc( decoder ),
                     data_position, member_size );
          fprintf( stderr, testing ? "ok\n" : "done\n" );
          }
        first_member = false; Pp_reset( pp );
        }
      if( rd <= 0 ) break;
      }
    if( out_size < 0 || ( first_member && out_size == 0 ) )
      {
      const enum LZ_Errno lz_errno = LZ_decompress_errno( decoder );
      if( lz_errno == LZ_header_error || ( first_member && out_size == 0 ) )
        {
        if( !first_member ) break;		/* trailing garbage */
        Pp_show_msg( pp, "Bad magic number (file not in lzip format)" );
        return 2;
        }
      if( lz_errno == LZ_mem_error )
        {
        Pp_show_msg( pp, "Not enough memory. Find a machine with more memory" );
        return 1;
        }
      if( verbosity >= 0 )
        {
        Pp_show_msg( pp, 0 );
        if( lz_errno == LZ_unexpected_eof )
          fprintf( stderr, "File ends unexpectedly at pos %llu\n",
                   LZ_decompress_total_in_size( decoder ) );
        else
          fprintf( stderr, "Decoder error at pos %llu: %s.\n",
                   LZ_decompress_total_in_size( decoder ),
                   LZ_strerror( LZ_decompress_errno( decoder ) ) );
        }
      return 2;
      }
    if( LZ_decompress_finished( decoder ) == 1 ) break;
    if( in_size == 0 && out_size == 0 )
      internal_error( "library error (LZ_decompress_read)" );
    }
  return 0;
  }


int decompress( const int infd, struct Pretty_print * const pp,
                const bool testing )
  {
  struct LZ_Decoder * const decoder = LZ_decompress_open();
  int retval;

  if( !decoder || LZ_decompress_errno( decoder ) != LZ_ok )
    {
    Pp_show_msg( pp, "Not enough memory. Find a machine with more memory" );
    retval = 1;
    }
  else retval = do_decompress( decoder, infd, pp, testing );

  LZ_decompress_close( decoder );
  return retval;
  }


void signal_handler( int sig )
  {
  if( sig ) {}				/* keep compiler happy */
  show_error( "Control-C or similar caught, quitting.", 0, false );
  cleanup_and_fail( 1 );
  }


static void set_signals( void )
  {
  signal( SIGHUP, signal_handler );
  signal( SIGINT, signal_handler );
  signal( SIGTERM, signal_handler );
  }


static void Pp_init( struct Pretty_print * const pp,
                     const char * const filenames[], const int num_filenames )
  {
  unsigned stdin_name_len;
  int i;
  pp->name = 0;
  pp->stdin_name = "(stdin)";
  pp->longest_name = 0;
  pp->first_post = false;
  stdin_name_len = strlen( pp->stdin_name );

  for( i = 0; i < num_filenames; ++i )
    {
    const char * const s = filenames[i];
    const int len = ( (strcmp( s, "-" ) == 0) ? stdin_name_len : strlen( s ) );
    if( len > pp->longest_name ) pp->longest_name = len;
    }
  if( pp->longest_name == 0 ) pp->longest_name = stdin_name_len;
  }


void show_error( const char * const msg, const int errcode, const bool help )
  {
  if( verbosity >= 0 )
    {
    if( msg && msg[0] )
      {
      fprintf( stderr, "%s: %s", program_name, msg );
      if( errcode > 0 ) fprintf( stderr, ": %s", strerror( errcode ) );
      fprintf( stderr, "\n" );
      }
    if( help )
      fprintf( stderr, "Try '%s --help' for more information.\n",
               invocation_name );
    }
  }


void internal_error( const char * const msg )
  {
  if( verbosity >= 0 )
    fprintf( stderr, "%s: internal error: %s.\n", program_name, msg );
  exit( 3 );
  }


int main( const int argc, const char * const argv[] )
  {
  /* Mapping from gzip/bzip2 style 1..9 compression modes
     to the corresponding LZMA compression modes. */
  const struct Lzma_options option_mapping[] =
    {
    { 1 << 20,   5 },		/* -0 */
    { 1 << 20,   5 },		/* -1 */
    { 3 << 19,   6 },		/* -2 */
    { 1 << 21,   8 },		/* -3 */
    { 3 << 20,  12 },		/* -4 */
    { 1 << 22,  20 },		/* -5 */
    { 1 << 23,  36 },		/* -6 */
    { 1 << 24,  68 },		/* -7 */
    { 3 << 23, 132 },		/* -8 */
    { 1 << 25, 273 } };		/* -9 */
  struct Lzma_options encoder_options = option_mapping[6];  /* default = "-6" */
  const unsigned long long max_member_size = 0x0100000000000000ULL;
  const unsigned long long max_volume_size = 0x4000000000000000ULL;
  unsigned long long member_size = max_member_size;
  unsigned long long volume_size = 0;
  const char * input_filename = "";
  const char * default_output_filename = "";
  const char ** filenames = 0;
  int num_filenames = 0;
  int infd = -1;
  enum Mode program_mode = m_compress;
  int argind = 0;
  int retval = 0;
  int i;
  bool filenames_given = false;
  bool force = false;
  bool keep_input_files = false;
  bool recompress = false;
  bool to_stdout = false;
  struct Pretty_print pp;

  const struct ap_Option options[] =
    {
    { '0', "fast",            ap_no  },
    { '1',  0,                ap_no  },
    { '2',  0,                ap_no  },
    { '3',  0,                ap_no  },
    { '4',  0,                ap_no  },
    { '5',  0,                ap_no  },
    { '6',  0,                ap_no  },
    { '7',  0,                ap_no  },
    { '8',  0,                ap_no  },
    { '9', "best",            ap_no  },
    { 'b', "member-size",     ap_yes },
    { 'c', "stdout",          ap_no  },
    { 'd', "decompress",      ap_no  },
    { 'f', "force",           ap_no  },
    { 'F', "recompress",      ap_no  },
    { 'h', "help",            ap_no  },
    { 'k', "keep",            ap_no  },
    { 'm', "match-length",    ap_yes },
    { 'n', "threads",         ap_yes },
    { 'o', "output",          ap_yes },
    { 'q', "quiet",           ap_no  },
    { 's', "dictionary-size", ap_yes },
    { 'S', "volume-size",     ap_yes },
    { 't', "test",            ap_no  },
    { 'v', "verbose",         ap_no  },
    { 'V', "version",         ap_no  },
    {  0 ,  0,                ap_no  } };

  struct Arg_parser parser;

  invocation_name = argv[0];

  if( LZ_version()[0] != LZ_version_string[0] )
    internal_error( "bad library version" );
  if( strcmp( PROGVERSION, LZ_version_string ) != 0 )
    internal_error( "bad library version_string" );

  if( !ap_init( &parser, argc, argv, options, 0 ) )
    { show_error( "Memory exhausted.", 0, false ); return 1; }
  if( ap_error( &parser ) )				/* bad option */
    { show_error( ap_error( &parser ), 0, true ); return 1; }

  for( ; argind < ap_arguments( &parser ); ++argind )
    {
    const int code = ap_code( &parser, argind );
    const char * const arg = ap_argument( &parser, argind );
    if( !code ) break;					/* no more options */
    switch( code )
      {
      case '0':
      case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
                encoder_options = option_mapping[code-'0']; break;
      case 'b': member_size = getnum( arg, 100000, max_member_size ); break;
      case 'c': to_stdout = true; break;
      case 'd': program_mode = m_decompress; break;
      case 'f': force = true; break;
      case 'F': recompress = true; break;
      case 'h': show_help(); return 0;
      case 'k': keep_input_files = true; break;
      case 'm': encoder_options.match_len_limit =
                  getnum( arg, LZ_min_match_len_limit(),
                               LZ_max_match_len_limit() ); break;
      case 'n': break;
      case 'o': default_output_filename = arg; break;
      case 'q': verbosity = -1; break;
      case 's': encoder_options.dictionary_size = get_dict_size( arg );
                break;
      case 'S': volume_size = getnum( arg, 100000, max_volume_size ); break;
      case 't': program_mode = m_test; break;
      case 'v': if( verbosity < 4 ) ++verbosity; break;
      case 'V': show_version(); return 0;
      default : internal_error( "uncaught option" );
      }
    } /* end process options */

#if defined(__MSVCRT__) || defined(__OS2__)
  setmode( STDIN_FILENO, O_BINARY );
  setmode( STDOUT_FILENO, O_BINARY );
#endif

  if( program_mode == m_test )
    outfd = -1;

  num_filenames = max( 1, ap_arguments( &parser ) - argind );
  filenames = resize_buffer( filenames, num_filenames * sizeof filenames[0] );
  filenames[0] = "-";

  for( i = 0; argind + i < ap_arguments( &parser ); ++i )
    {
    filenames[i] = ap_argument( &parser, argind + i );
    if( strcmp( filenames[i], "-" ) != 0 ) filenames_given = true;
    }

  if( !to_stdout && program_mode != m_test &&
      ( filenames_given || default_output_filename[0] ) )
    set_signals();

  Pp_init( &pp, filenames, num_filenames );

  output_filename = resize_buffer( output_filename, 1 );
  for( i = 0; i < num_filenames; ++i )
    {
    int tmp;
    struct stat in_stats;
    const struct stat * in_statsp;
    output_filename[0] = 0;

    if( !filenames[i][0] || strcmp( filenames[i], "-" ) == 0 )
      {
      input_filename = "";
      infd = STDIN_FILENO;
      if( program_mode != m_test )
        {
        if( to_stdout || !default_output_filename[0] )
          outfd = STDOUT_FILENO;
        else
          {
          if( program_mode == m_compress )
            set_c_outname( default_output_filename, volume_size > 0 );
          else
            {
            output_filename = resize_buffer( output_filename,
                                             strlen( default_output_filename ) + 1 );
            strcpy( output_filename, default_output_filename );
            }
          outfd_mode = all_rw;
          if( !open_outstream( force ) )
            {
            if( retval < 1 ) retval = 1;
            close( infd ); infd = -1;
            continue;
            }
          }
        }
      }
    else
      {
      const int eindex = extension_index( filenames[i] );
      input_filename = filenames[i];
      infd = open_instream( input_filename, &in_stats, program_mode,
                            eindex, recompress, to_stdout );
      if( infd < 0 ) { if( retval < 1 ) retval = 1; continue; }
      if( program_mode != m_test )
        {
        if( to_stdout ) outfd = STDOUT_FILENO;
        else
          {
          if( program_mode == m_compress )
            set_c_outname( input_filename, volume_size > 0 );
          else set_d_outname( input_filename, eindex );
          outfd_mode = usr_rw;
          if( !open_outstream( force ) )
            {
            if( retval < 1 ) retval = 1;
            close( infd ); infd = -1;
            continue;
            }
          }
        }
      }

    if( !check_tty( infd, program_mode ) ) return 1;

    if( output_filename[0] && !to_stdout && program_mode != m_test )
      delete_output_on_interrupt = true;
    in_statsp = input_filename[0] ? &in_stats : 0;
    Pp_set_name( &pp, input_filename );
    if( program_mode == m_compress )
      tmp = compress( member_size, volume_size, &encoder_options, infd,
                      &pp, in_statsp );
    else
      tmp = decompress( infd, &pp, program_mode == m_test );
    if( tmp > retval ) retval = tmp;
    if( tmp && program_mode != m_test ) cleanup_and_fail( retval );

    if( delete_output_on_interrupt )
      close_and_set_permissions( in_statsp );
    if( input_filename[0] )
      {
      close( infd ); infd = -1;
      if( !keep_input_files && !to_stdout && program_mode != m_test )
        remove( input_filename );
      }
    }
  if( outfd >= 0 && close( outfd ) != 0 )
    {
    show_error( "Can't close stdout", errno, false );
    if( retval < 1 ) retval = 1;
    }
  free( output_filename );
  free( filenames );
  ap_free( &parser );
  return retval;
  }
