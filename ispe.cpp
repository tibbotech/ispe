#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#include <arpa/inet.h>  // htohl
#include "isp.h"

#define TMP_PFX "isp."
// FILE_SIZE_IMAGE_XBOOT0 + FILE_SIZE_IMAGE_UBOOT0
#define OFF_HDR 0x100000

uint8_t dbg = 0;

void p_hex( uint8_t *_x, uint32_t _s);
void p_sch( uint8_t *_x, uint32_t _s);
void init_script_hdr_parse( const unsigned char *_hdr, isp_hdr_script_t &_x);

int isp_list( const char *_fname);
int isp_exts( const char *_fname);
int isp_extp( const char *_fname, const char *_pname);
int isp_sets( const char *_fname, const char *_sname);

int main( int argc, char *argv[]) {
 if ( argc < 2 || ( argc > 1 && argv[ 1][ 0] == '-')) {
   printf( "Usage: %s <img> [-v] <cmd> [cmdparams]\n", argv[ 0]);
   printf( "\t[-v] verbose mode\n");
   printf( "\t<cmd> [params] one of the following:\n");
   printf( "\tlist - list partitions in the image\n");
   printf( "\texts - extract header script\n");
   printf( "\textp <name> - extract partition\n");
   printf( "\tsets <file> - update header script from file\n");
   return( 1);  }
 uint8_t aoff = 2, i = 0;
 if ( argc > aoff) while ( argv[ aoff][ ++i] == 'v') dbg++;
 if ( dbg) printf( "dbg%01d: Verbose mode\n", dbg);
 if ( dbg) aoff++;
 char *Iname = argv[ 1];
 if ( dbg) printf( "dbg0: IMG: %s\n", Iname);
 char *cmd = NULL;
 char *arg = NULL;
 if ( argc <= aoff) {
   printf( "ERR: No cmd, run %s for help\n", argv[ 0]);
   return( 1);  }
 if ( strcmp( argv[ aoff], "list") == 0) cmd = argv[ aoff];
 if ( strcmp( argv[ aoff], "exts") == 0) cmd = argv[ aoff];
 if ( strcmp( argv[ aoff], "sets") == 0) {
   if ( aoff + 1 >= argc) {
     printf( "ERR: script filename is reqired. See help\n");
     return( 1);  }
   cmd = argv[ aoff];
   arg = argv[ aoff + 1];
 }
 if ( strcmp( argv[ aoff], "extp") == 0) {
   if ( aoff + 1 >= argc) {
     printf( "ERR: partition name is reqired. See help\n");
     return( 1);  }
   cmd = argv[ aoff];
   arg = argv[ aoff + 1];
 }
 if ( !cmd) {
   printf( "ERR: Unknown cmd, run %s for help\n", argv[ 0]);
   return( 1);  }
 if ( dbg) printf( "dbg0: CMD %s\n", cmd);
 if ( sizeof( isp_part_t) != SIZE_PARTITION_INFO_S) {
   printf( "ERR: sizeof(partition_info) != %d. Check your compiler\n", SIZE_PARTITION_INFO_S);
   return( 1);  }
 int ret = 0;
 if ( strcmp( cmd, "list") == 0) ret = isp_list( Iname);
 if ( strcmp( cmd, "exts") == 0) ret = isp_exts( Iname);
 if ( strcmp( cmd, "extp") == 0) ret = isp_extp( Iname, arg);
 if ( strcmp( cmd, "sets") == 0) ret = isp_sets( Iname, arg);
 return( ret);  }

// _m == "rb", ...
FILE *ispimg_read_hdr( const char *_fname, const char *_m, isp_hdr_t &_HDR) {
 FILE *Ifp;
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 if ( !( Ifp = fopen( _fname, _m))) {
   printf( "ERR: img '%s': %s(%d)\n", _fname, strerror( errno), errno);
   return( NULL);  }
 long off_hdr = OFF_HDR;
 if ( fseek( Ifp, off_hdr, SEEK_SET) != 0) {
   printf( "ERR: '%s' at pos %lX: %s(%d)\n", _fname, off_hdr, strerror( errno), errno);
   fclose( Ifp);  return( NULL);  }
 long off_cur = ftell( Ifp);
 if ( off_hdr != off_cur) {
   printf( "ERR: '%s' can't pos at %lX\n", _fname, off_hdr);
   fclose( Ifp);  return( NULL);  }
 if ( dbg) printf( "dbg0: Reading %ld bytes at %lX\n", sizeof( _HDR), off_hdr);
 if ( fread( &_HDR, sizeof( _HDR), 1, Ifp) != 1) {
   printf( "ERR: '%s' reading HDR at %lX\n", _fname, off_hdr);
   fclose( Ifp);  return( NULL);  }
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( Ifp);  }

// set script for the image
int isp_sets( const char *_fname, const char *_sname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_read_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);

 int ret = 0;
 size_t szr = 0;
 FILE *Sfp;
 if ( !( Sfp = fopen( _sname, "rb"))) {
   printf( "ERR: can't open file %s: %s(%d)\n", _sname, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 char buf[ sizeof( HDR.init_script)];
 memset( buf, 0, sizeof( buf));
 // read script image from file
 if ( ( szr = fread( buf, 1, sizeof( buf), Sfp)) < sizeof( isp_hdr_script_t)) {
   printf( "ERR: read %ld bytes failed: %s(%d) %ld\n", sizeof( buf), strerror( errno), errno, szr);
   fclose( Ifp);  fclose( Sfp);  return( 1);  }
 fclose( Sfp);
 // try to detect the script size
 isp_hdr_script_t xxx0;
 init_script_hdr_parse( ( unsigned char *)buf, xxx0);
 if ( dbg) printf( "dbg0: script img parsed size: %d\n", xxx0.l);
 if ( xxx0.l > ( sizeof( HDR.init_script) - FIT_HDR_OFF - sizeof( xxx0))) {
   printf( "ERR: parser script size: %d > %ld (%ld-%d-%ld)\n", xxx0.l, sizeof( HDR.init_script) - FIT_HDR_OFF - sizeof( xxx0), sizeof( HDR.init_script), FIT_HDR_OFF, sizeof( xxx0));
   fclose( Ifp);  return( 1);  }
 if ( xxx0.l < 1) {
   printf( "WRN: parsed script len=0. Probably the script image is wrong.\n");
 }
 // go to script position
 long off_hdr = OFF_HDR + offsetof( isp_hdr_t, init_script);
 if ( fseek( Ifp, off_hdr, SEEK_SET) != 0) {
   printf( "ERR: '%s' at pos %lX: %s(%d)\n", _fname, off_hdr, strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 long off_cur = ftell( Ifp);
 if ( off_hdr != off_cur) {
   printf( "ERR: '%s' can't pos at %lX\n", _fname, off_hdr);
   fclose( Ifp);  return( 1);  }
 // write it as-is
 memcpy( HDR.init_script, buf, sizeof( buf));
 if ( fwrite( HDR.init_script, sizeof( buf), 1, Ifp) != 1) {
   printf( "ERR: write %ld bytes failed: %s(%d)\n", sizeof( buf), strerror( errno), errno);
   ret = 1;  }
 // writing the script part /
 fclose( Ifp);
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 return( ret);  }

// extract script from the image
int isp_exts( const char *_fname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_read_hdr( _fname, "rb", HDR);
 if ( !Ifp) return( 1);
 fclose( Ifp);
 int ret = 0;
 FILE *Ofp;
 char buf[ sizeof( HDR.init_script)];
 sprintf( buf, TMP_PFX"h.script.raw");
 if ( !( Ofp = fopen( buf, "wb"))) {
   printf( "ERR: can't create file %s: %s(%d)\n", buf, strerror(errno), errno);
   return( 1);  }
 memset( buf, 0, sizeof( buf));
 if ( fwrite( HDR.init_script, sizeof( buf), 1, Ofp) != 1) {
   printf( "ERR: write %ld bytes failed: %s(%d)\n", sizeof( buf), strerror( errno), errno);
   ret = 1;  }
 fclose( Ofp);
 if ( dbg) printf( "dbg0: Trying to extract clear text\n");
 isp_hdr_script_t xxx0;
 init_script_hdr_parse( HDR.init_script, xxx0);
 if ( dbg) printf( "dbg0: Text size should be %d\n", xxx0.l);
 size_t h_off = FIT_HDR_OFF + sizeof( xxx0);
 if ( xxx0.l > sizeof( HDR.init_script) - h_off) {
   printf( "ERR: text size %d > %ld (%ld-%ld)\n", xxx0.l, (sizeof( HDR.init_script) - h_off), sizeof( HDR.init_script), h_off);
   return( 1);  }
 sprintf( buf, TMP_PFX"h.script.txt");
 if ( !( Ofp = fopen( buf, "wb"))) {
   printf( "ERR: can't create file %s: %s(%d)\n", buf, strerror(errno), errno);
   return( 1);  }
 if ( fwrite( HDR.init_script + h_off, 1, xxx0.l, Ofp) != xxx0.l) {
   printf( "ERR: write %ld bytes failed: %s(%d)\n", sizeof( buf), strerror( errno), errno);
   ret = 1;  }
 fclose( Ofp);
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 return( ret);  }

// extract partition from the image
int isp_extp( const char *_fname, const char *_pname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_read_hdr( _fname, "rb", HDR);
 if ( !Ifp) return( 1);
 int ret = 1, found = 0;
 FILE *Ofp;
 for ( int i = 0; i < NUM_OF_PARTITION; i++) {
   if ( i > 0 && HDR.partition_info[ i].partition_size < 1) continue;
   if ( strcmp( _pname, ( const char *)HDR.partition_info[ i].file_name) != 0) continue;
   if ( dbg) printf( "dbg0: Found '%s' partition at %X\n", _pname, HDR.partition_info[ i].file_offset);
   found = 1;
   char buf[ 2048];
   sprintf( buf, TMP_PFX"p.%s", _pname);
   if ( !( Ofp = fopen( buf, "wb"))) {
     printf( "ERR: can't create file %s: %s(%d)\n", _pname, strerror(errno), errno);
     break;  }
   if ( fseek( Ifp, HDR.partition_info[ i].file_offset, SEEK_SET) != 0) {
     printf( "ERR: '%s' at pos %X: %s(%d)\n", _fname, HDR.partition_info[ i].file_offset, strerror( errno), errno);
     fclose( Ofp);  break;  }
   // reading partition...
   size_t p_idx = HDR.partition_info[ i].file_size, sz = sizeof( buf), szr;
   if ( dbg) printf( "dbg0: partition %ld bytes\n", p_idx);
   while ( p_idx > 0) {
     if ( sz > p_idx) sz = p_idx;
     if ( dbg > 1) printf( "dbg1: reading %ld bytes\n", sz);
     if ( ( szr = fread( buf, 1, sz, Ifp)) < 0) {
       printf( "ERR: read %ld bytes failed: %s(%d)\n", sz, strerror( errno), errno);
       fclose( Ofp); break;  }
     if ( dbg > 1) printf( "dbg1: writing %ld bytes\n", szr);
     if ( fwrite( buf, szr, 1, Ofp) != 1) {
       printf( "ERR: write %ld bytes failed: %s(%d)\n", szr, strerror( errno), errno);
       fclose( Ofp); break;  }
     p_idx -= szr;  }
   // reading partition... /
   fclose( Ofp);  ret = 0;
 }
 fclose( Ifp);
 if ( !found) printf( "ERR: part '%s' not found\n", _pname);
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 return( ret);  }

// show image info
int isp_list( const char *_fname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_read_hdr( _fname, "rb", HDR);
 fclose( Ifp);
 printf( "HEADER:\n");
 printf( "\tsign (str,%ld/%ld): %s\n", sizeof( HDR.signature), sizeof( HDR.signature), HDR.signature);
 printf( "\tsign (hex,%ld/%ld): ", sizeof( HDR.signature), sizeof( HDR.signature));  p_hex( HDR.signature, sizeof( HDR.signature));  printf( "\n");
 printf( "\tinit script (str,16/%ld): ", sizeof( HDR.init_script) - FIT_HDR_OFF - sizeof( isp_hdr_script_t));
 p_sch( HDR.init_script + FIT_HDR_OFF + sizeof( isp_hdr_script_t), 16);
 printf( "<skipped...>\n");
 isp_hdr_script_t xxx0;
 init_script_hdr_parse( HDR.init_script, xxx0);
 printf( "\tinit script size: %d\n", xxx0.l);
 if (dbg) {
   printf( "\tinit script (str,80/%ld): ", sizeof( HDR.init_script)); p_sch( HDR.init_script, 86);  printf( "\n");
 }
 printf( "\tflags: 0x%X\n", HDR.flags);
 for ( int i = 0; i < NUM_OF_PARTITION; i++) {
   if ( i > 0 && HDR.partition_info[ i].partition_size < 1) continue;
   printf( "PARTITION%d\n", i);
   printf( "\tfilename: %s\n", HDR.partition_info[ i].file_name);
   printf( "\tmd5sum: %s\n", HDR.partition_info[ i].md5sum);
   printf( "\tfile offset: 0x%X\n", HDR.partition_info[ i].file_offset);
   printf( "\tfile size: %ld\n", HDR.partition_info[ i].file_size);
   printf( "\tpart start addr: 0x%X\n", HDR.partition_info[ i].partition_start_addr);
   printf( "\tpart size: %ld\n", HDR.partition_info[ i].partition_size);
   printf( "\tflags: 0x%X\n", HDR.partition_info[ i].flags);
   printf( "\temmc part start block: %d\n", HDR.partition_info[ i].emmc_partition_start);
 }
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( 0);  }

void init_script_hdr_parse( const unsigned char *_s, isp_hdr_script_t &_x) {
 memcpy( &_x, _s + FIT_HDR_OFF, sizeof( _x));
 _x.l = ntohl( _x.l);
 return;  }

void p_sch( uint8_t *_x, uint32_t _s) {
 for ( uint32_t i = 0; i < _s; i++) printf( "%c", _x[ i]);
 return;  }
void p_hex( uint8_t *_x, uint32_t _s) {
 for ( uint32_t i = 0; i < _s; i++) printf( "%01X", _x[ i]);
 return;  }
