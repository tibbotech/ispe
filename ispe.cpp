#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#include <arpa/inet.h>  // htohl
#include <unistd.h>     // ftruncate
#include <sys/stat.h>   // stat()
#include <stdlib.h>     // strtol()
#include "isp.h"

#define TMP_PFX "isp."
// FILE_SIZE_IMAGE_XBOOT0 + FILE_SIZE_IMAGE_UBOOT0
#define OFF_HDR 0x100000

uint8_t dbg = 0;

void p_hex( uint8_t *_x, uint32_t _s);
void p_sch( uint8_t *_x, uint32_t _s);
void init_script_hdr_parse( const unsigned char *_hdr, isp_hdr_script_t &_x);
FILE *ispimg_R_hdr( const char *_fname, const char *_m, isp_hdr_t &_HDR);
int ispimg_W_hdr( FILE *_fp, isp_hdr_t &_HDR);

int isp_list( const char *_fname);
int isp_exts( const char *_fname);
int isp_extp( const char *_fname, const char *_pname);
int isp_extb( const char *_fname, const off_t _off, const size_t _len);
int isp_delp( const char *_fname, const char *_pname);
int isp_wipp( const char *_fname, const char *_pname);
int isp_sets( const char *_fname, const char *_sname);

int main( int argc, char *argv[]) {
 if ( argc < 2 || ( argc > 1 && argv[ 1][ 0] == '-')) {
   printf( "Usage: %s <img> [-v] <cmd> [cmdparams]\n", argv[ 0]);
   printf( "\t[-v] verbose mode\n");
   printf( "\t<cmd> [params] one of the following:\n");
   printf( "\tlist - list partitions in the image\n");
   printf( "\texts - extract header script\n");
   printf( "\textp <name> - extract partition\n");
   printf( "\textb <hoff> <dlen> - extract <dlen> (dec) bytes at <hoff> (hex) offset\n");
   printf( "\tsets <file> - update header script from script image file\n");
   printf( "\tdelp <name> - delete partition from the image\n");
   printf( "\twipp <name> - wipe the partition keeping general info\n");
   return( 1);  }
 uint8_t aoff = 2, i = 0;
 if ( argc > aoff) while ( argv[ aoff][ ++i] == 'v') dbg++;
 if ( dbg) printf( "dbg%01d: Verbose mode\n", dbg);
 if ( dbg) aoff++;
 char *Iname = argv[ 1];
 if ( dbg) printf( "dbg0: IMG: %s\n", Iname);
 if ( argc <= aoff) {
   printf( "ERR: No cmd, run %s for help\n", argv[ 0]);
   return( 1);  }
 char *cmd = argv[ aoff];
 char *arg0 = NULL, *arg1 = NULL;
 if ( strcmp( cmd, "sets") == 0) {
   if ( aoff + 1 >= argc) {
     printf( "ERR: script filename is reqired. See help\n");
     return( 1);  }
   arg0 = argv[ aoff + 1];
 }
 if ( strcmp( cmd, "extp") == 0 || strcmp( cmd, "delp") == 0 ||
      strcmp( cmd, "wipp") == 0
    ) {
   if ( aoff + 1 >= argc) {
     printf( "ERR: partition name is reqired. See help\n");
     return( 1);  }
   arg0 = argv[ aoff + 1];
 }
 if ( strcmp( cmd, "extb") == 0) {
   if ( aoff + 2 >= argc) {
     printf( "ERR: <offset> and <len> are reqired. See help\n");
     return( 1);  }
   arg0 = argv[ aoff + 1];
   arg1 = argv[ aoff + 2];
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
 if ( strcmp( cmd, "extp") == 0) ret = isp_extp( Iname, arg0);
 if ( strcmp( cmd, "extb") == 0) ret = isp_extb( Iname, strtol( arg0, NULL, 16), atoi( arg1));
 if ( strcmp( cmd, "sets") == 0) ret = isp_sets( Iname, arg0);
 if ( strcmp( cmd, "delp") == 0) ret = isp_delp( Iname, arg0);
 if ( strcmp( cmd, "wipp") == 0) ret = isp_wipp( Iname, arg0);
 return( ret);  }

// set script for the image
int isp_sets( const char *_fname, const char *_sname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
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
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( ret);  }

// extract script from the image
int isp_exts( const char *_fname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "rb", HDR);
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
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( ret);  }

isp_part_t *find_part( isp_hdr_t &_hdr, const char *_pname, uint8_t &_idx) {
 isp_part_t *P = NULL;
 for ( int i = 0; i < NUM_OF_PARTITION; i++) {
   if ( i > 0 && _hdr.partition_info[ i].file_offset < 1) continue;
   if ( strcmp( _pname, ( const char *)_hdr.partition_info[ i].file_name) != 0) continue;
   P = &( _hdr.partition_info[ i]);  _idx = i;  break;  }
 return( P);  }
 
// wipe partition from the image
int isp_wipp( const char *_fname, const char *_pname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 // search for the partition
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {
   printf( "ERR: partition '%s' not found\n", _pname);
   fclose( Ifp);  return( 1);  }
 if ( dbg) printf( "dbg0: Found '%s'[%d] partition at %X\n", _pname, pIdx, P->file_offset);
 // wipe info
 memset( P->md5sum, 0, sizeof( P->md5sum));
 P->file_size = 0;
 if ( ispimg_W_hdr( Ifp, HDR) != 0) {  fclose( Ifp);  return( 1);  }
 fclose( Ifp);
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( 0);  }

// extract _len bytes of binary data starting at _off
int isp_extb( const char *_fname, const off_t _off, const size_t _len) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "rb", HDR);
 if ( !Ifp) return( 1);
 FILE *Ofp;
 char buf[ 2048];
 sprintf( buf, TMP_PFX"b.%lX.%ld", _off, _len);
 if ( !( Ofp = fopen( buf, "wb"))) {
   printf( "ERR: can't create file %s: %s(%d)\n", buf, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 if ( fseek( Ifp, _off, SEEK_SET) != 0) {
   printf( "ERR: at pos %lX: %s(%d)\n", _off, strerror( errno), errno);
   fclose( Ofp);  fclose( Ifp);  return( 1);  }
 int ret = 0;
 size_t sz = sizeof( buf), szr, len = _len;
 if ( dbg) printf( "dbg0: dumping %ld bytes from 0x%lX pos\n", _len, _off);
 while ( len > 0) {
   if ( sz > len) sz = len;
   if ( dbg > 1) printf( "dbg1: reading %ld bytes\n", sz);
   if ( ( szr = fread( buf, 1, sz, Ifp)) < 0) {
     printf( "ERR: read %ld bytes failed: %s(%d)\n", sz, strerror( errno), errno);
     ret = 1;  break;  }
   if ( dbg > 1) printf( "dbg1: writing %ld bytes\n", szr);
   if ( fwrite( buf, szr, 1, Ofp) != 1) {
     printf( "ERR: write %ld bytes failed: %s(%d)\n", szr, strerror( errno), errno);
     ret = 1;  break;  }
   len -= szr;  }
 fclose( Ofp);
 fclose( Ifp);
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( ret);  }

// extract partition from the image
int isp_extp( const char *_fname, const char *_pname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 // search for the partition
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {
   printf( "ERR: partition '%s' not found\n", _pname);
   fclose( Ifp);  return( 1);  }
 if ( dbg) printf( "dbg0: Found '%s'[%d] partition at %X\n", _pname, pIdx, P->file_offset);
 FILE *Ofp;
 char buf[ 2048];
 sprintf( buf, TMP_PFX"p.%s", _pname);
 if ( !( Ofp = fopen( buf, "wb"))) {
   printf( "ERR: can't create file %s: %s(%d)\n", buf, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 if ( fseek( Ifp, P->file_offset, SEEK_SET) != 0) {
   printf( "ERR: '%s' at pos %X: %s(%d)\n", _fname, P->file_offset, strerror( errno), errno);
   fclose( Ofp);  fclose( Ifp);  return( 1);  }
 // reading partition...
 size_t p_idx = P->file_size, sz = sizeof( buf), szr;
 int ret = 0;
 if ( dbg) printf( "dbg0: partition %ld bytes\n", p_idx);
 while ( p_idx > 0) {
   if ( sz > p_idx) sz = p_idx;
   if ( dbg > 1) printf( "dbg1: reading %ld bytes\n", sz);
   if ( ( szr = fread( buf, 1, sz, Ifp)) < 0) {
     printf( "ERR: read %ld bytes failed: %s(%d)\n", sz, strerror( errno), errno);
     ret = 1;  break;  }
   if ( dbg > 1) printf( "dbg1: writing %ld bytes\n", szr);
   if ( fwrite( buf, szr, 1, Ofp) != 1) {
     printf( "ERR: write %ld bytes failed: %s(%d)\n", szr, strerror( errno), errno);
     ret = 1;  break;  }
   p_idx -= szr;  }
 // reading partition... /
 fclose( Ofp);
 fclose( Ifp);
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( ret);  }

// delete partition from the image
int isp_delp( const char *_fname, const char *_pname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 // search for the partition
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {
   printf( "ERR: partition '%s' not found\n", _pname);
   fclose( Ifp);  return( 1);  }
 if ( dbg) printf( "dbg0: Found '%s'[%d] partition at %X\n", _pname, pIdx, P->file_offset);
 isp_part_t *Pn = &( HDR.partition_info[ pIdx + 1]);    // next partition
 if ( Pn->file_offset < 1) Pn = NULL;
 if ( dbg &&  Pn) printf( "dbg0: Next part '%s' is at %X\n", Pn->file_name, Pn->file_offset);
 if ( dbg && !Pn) printf( "dbg0: This partition is last one\n");
 struct stat fst;
 if ( fstat( fileno( Ifp), &fst) != 0) {
   printf( "ERR: stat failed: %s(%d)\n", strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 off_t Isize = fst.st_size;
 if ( dbg) printf( "dbg0: (I/P/F) size: %ld/%ld/%ld\n", Isize, P->partition_size, P->file_size);
 off_t Psize = P->partition_size;
 // if RESERVED partition size ends after the image file end... 
 if ( P->partition_size + P->file_offset > ( uint64_t)Isize) {
   printf( "WRN: Part ends at %lX, after the EOF:%lX\n", Psize + P->file_offset, Isize);
   Psize = P->file_size;
   printf( "WRN: assuming Psize = %ld (file_size)\n", Psize);  }
 if ( Pn && ( P->file_offset + Psize) > Pn->file_offset) {
   printf( "WRN: Part ends at %lX, after next part start offset:%X\n", P->file_offset + Psize, Pn->file_offset);
   Psize = Pn->file_offset - P->file_offset;
   printf( "WRN: assuming Psize = %ld (offsets diff)\n", Psize);  }
 off_t truncate_size = Isize - Psize;
 off_t R_off = P->file_offset + Psize;      // read starting from ...
 off_t W_off = P->file_offset;              // write starting from ...
 off_t move_size = Isize - ( P->file_offset + Psize);
 if ( move_size < 1) printf( "WRN: 0 bytes to move ?!\n");
 if ( dbg) printf( "dbg0: %ld bytes to move, %ld bytes for delete\n", move_size, Psize);
 char buf[ 2048];
 off_t i = 0;
 off_t szr = 0, btr = 0;
 while ( i < move_size) {
   // move to R_off + i and read block
   if ( fseek( Ifp, R_off + i, SEEK_SET) != 0) {
     printf( "ERR: R seek at pos %lX: %s(%d)\n", R_off + i, strerror( errno), errno);
     fclose( Ifp);  return( 1);  }
   btr = sizeof( buf);
   if ( btr > ( move_size - i)) btr = ( move_size - i);
   if ( btr < 1) break;
   if ( ( szr = fread( buf, 1, btr, Ifp)) < 1) {
     printf( "ERR: read %ld bytes failed at %lX: %s(%d)\n", btr, R_off + i, strerror( errno), errno);
     fclose( Ifp);  return( 1);  }
   // move to W_off + i and write block
   if ( fseek( Ifp, W_off + i, SEEK_SET) != 0) {
     printf( "ERR: R seek at pos %lX: %s(%d)\n", R_off + i, strerror( errno), errno);
     fclose( Ifp);  return( 1);  }
   if ( ( fwrite( buf, szr, 1, Ifp)) != 1) {
     printf( "ERR: write %ld bytes at %lX failed: %s(%d)\n", szr, W_off + i, strerror( errno), errno);
     fclose( Ifp);  return( 1);  }
   i += szr;  }
 if ( dbg) printf( "dbg0: %ld bytes moved from %lX to %lX\n", move_size, R_off, W_off);
 // wipe part info from header and save it
 if ( dbg) printf( "dbg0: Updating HDR...\n");
 // move parts pointers if there are something after
 for ( int i = pIdx; i < NUM_OF_PARTITION; i++) {
   P = &( HDR.partition_info[ i]);
   if ( i > 0 && P->file_offset < 1) memset( P, 0, sizeof( *P));
   memcpy( P, &( HDR.partition_info[ i + 1]), sizeof( *P));
   if ( i > 0 && P->file_offset < 1) break;
   if ( dbg) printf( "dbg0: shift '%s'[%d] %X -%lX\n", P->file_name, i, P->file_offset, Psize);
   P->file_offset -= Psize;
 }
 if ( ispimg_W_hdr( Ifp, HDR) != 0) {  fclose( Ifp);  return( 1);  }
 // truncate the result
 if ( dbg) printf( "dbg0: truncating I from %ld to %ld bytes (-%ld)...\n", Isize, truncate_size, ( Isize - truncate_size));
 if ( ftruncate( fileno( Ifp), truncate_size)) {
   printf( "ERR: truncating: %s(%d)\n", strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 fclose( Ifp);
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( 0);  }

// show image info
int isp_list( const char *_fname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "rb", HDR);
 struct stat fst;
 memset( &fst, 0, sizeof( fst));
 fstat( fileno( Ifp), &fst);
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
 isp_part_t *P = NULL;
 off_t leop;
 for ( int i = 0; i < NUM_OF_PARTITION; i++) {
   if ( i > 0 && HDR.partition_info[ i].file_offset < 1) continue;
   P = &( HDR.partition_info[ i]);
   leop = P->file_offset + P->partition_size;
   printf( "PARTITION%d\n", i);
   printf( "\tfilename: %s\n", P->file_name);
   printf( "\tmd5sum: %s\n", P->md5sum);
   printf( "\tfile offset: 0x%X\n", P->file_offset);
   printf( "\tfile size: %ld\n", P->file_size);
   printf( "\tpart start addr: 0x%X\n", P->partition_start_addr);
   printf( "\tpart size: %ld\n", P->partition_size);
   printf( "\tflags: 0x%X\n", P->flags);
   printf( "\temmc part start block: %d\n", P->emmc_partition_start);
   if ( fst.st_size < leop) {
     printf( "WRN: virtually ends at %ld (%ld bytes after)\n", leop, ( leop - fst.st_size));
     printf( "WRN: image eof is at %ld\n", fst.st_size);
   }
 }
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( 0);  }

// _m == "rb", ...
FILE *ispimg_R_hdr( const char *_fname, const char *_m, isp_hdr_t &_HDR) {
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

// write header
int ispimg_W_hdr( FILE *_fp, isp_hdr_t &_HDR) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 long off_hdr = OFF_HDR;
 if ( fseek( _fp, off_hdr, SEEK_SET) != 0) {
   printf( "ERR: at pos %lX: %s(%d)\n", off_hdr, strerror( errno), errno);
   return( -1);  }
 long off_cur = ftell( _fp);
 if ( off_hdr != off_cur) {
   printf( "ERR: can't pos at %lX\n", off_hdr);
   return( -1);  }
 if ( dbg) printf( "dbg0: Writing %ld bytes at %lX\n", sizeof( _HDR), off_hdr);
 if ( fwrite( &_HDR, sizeof( _HDR), 1, _fp) != 1) {
   printf( "ERR: writing HDR at %lX\n", off_hdr);
   return( -1);  }
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
