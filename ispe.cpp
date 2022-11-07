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
int _pos( FILE *_F, off_t _p);
off_t _siz( FILE *_F);
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
int isp_setb( const char *_fname, const off_t _off, const char *_pname);

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
   printf( "\tsetb <hoff> <name> - save raw binary file <name> at <hoff> (hex) offset\n");
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
 if ( strcmp( cmd, "extb") == 0 || strcmp( cmd, "setb") == 0) {
   if ( aoff + 2 >= argc) {
     printf( "ERR: parameters are reqired. See help\n");
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
 if ( strcmp( cmd, "extb") == 0) ret = isp_extb( Iname, strtol( arg0, NULL, 16), atol( arg1));
 if ( strcmp( cmd, "sets") == 0) ret = isp_sets( Iname, arg0);
 if ( strcmp( cmd, "setb") == 0) ret = isp_setb( Iname, strtol( arg0, NULL, 16), arg1);
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
 if ( _pos( Ifp, off_hdr) != 0) {
   printf( "ERR: '%s' at pos 0x%lX: %s(%d)\n", _fname, off_hdr, strerror( errno), errno);
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

int RW( FILE *_R, const off_t _Roff, FILE *_W, const off_t _Woff, const size_t _len) {
 // if files are different - able to pos once
 if ( _R != _W) {
   if ( dbg && _Roff) printf( "dbg0: R seek to 0x%lx\n", _Roff);
   if ( _Roff && _pos( _R, _Roff) != 0) {
     printf( "ERR: seek R at pos 0x%lX: %s(%d)\n", _Roff, strerror( errno), errno);
     return( 1);  }
   if ( dbg && _Woff) printf( "dbg0: W seek to 0x%lx\n", _Woff);
   if ( _Woff && _pos( _W, _Woff) != 0) {
     printf( "ERR: seek W at pos 0x%lX: %s(%d)\n", _Woff, strerror( errno), errno);
     return( 1);  }
 }
 size_t bdone = 0;
 char buf[ 2048];
 size_t sz = sizeof( buf), szr = 0; 
 while ( bdone < _len) {
   if ( sz > ( _len - bdone)) sz = ( _len - bdone);
   if ( dbg > 1) printf( "dbg1: reading %ld bytes\n", sz);
   if ( _R == _W && _pos( _R, _Roff + bdone) != 0) { 
     printf( "ERR: seek R at pos 0x%lX\n", _Roff + bdone);  return( 1);  }
   if ( ( szr = fread( buf, 1, sz, _R)) < 0) {
     printf( "ERR: read %ld bytes failed: %s(%d)\n", sz, strerror( errno), errno);
     return( 1);  }
   if ( dbg > 1) printf( "dbg1: writing %ld bytes\n", szr);
   if ( _R == _W && _pos( _W, _Woff + bdone) != 0) { 
     printf( "ERR: seek W at pos 0x%lX\n", _Woff + bdone);  return( 1);  }
   if ( fwrite( buf, szr, 1, _W) != 1) {
     printf( "ERR: write %ld bytes failed: %s(%d)\n", szr, strerror( errno), errno);
     return( 1);  }
   bdone += szr;  }
 return( 0); }

// save 
int isp_setb( const char *_fname, const off_t _off, const char *_sname) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 FILE *Sfp;
 if ( !( Sfp = fopen( _sname, "rb"))) {
   printf( "ERR: can't open file %s: %s(%d)\n", _sname, strerror(errno), errno);
   return( 1);  }
 off_t Ssize = _siz( Sfp);
 if ( Ssize == 0) {
   printf( "ERR: stat failed: %s(%d)\n", strerror( errno), errno);
   fclose( Sfp);  return( 1);  }
 // test for size
 if ( _off + Ssize > OFF_HDR) {
   printf( "ERR: 0x%lX (off) + %ld (file size) > 0x%X (HDR off)\n", _off, Ssize, OFF_HDR);
   fclose( Sfp);  return( 1);  }
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 // read and write
 if ( dbg) printf( "dbg0: Writing %ld bytes at 0x%lX\n", Ssize, _off);
 _pos( Ifp, _off);
 int ret = RW( Sfp, 0, Ifp, _off, Ssize);
 // read and write /
 fclose( Ifp);
 fclose( Sfp);
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
 if ( dbg) printf( "dbg0: Found '%s'[%d] partition at 0x%X\n", _pname, pIdx, P->file_offset);
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
 if ( dbg) printf( "dbg0: dumping %ld bytes from 0x%lX pos\n", _len, _off);
 _pos( Ifp, 0);
 int ret = RW( Ifp, _off, Ofp, 0, _len);
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
 if ( dbg) printf( "dbg0: Found '%s'[%d] partition at 0x%X\n", _pname, pIdx, P->file_offset);
 FILE *Ofp;
 char buf[ 2048];
 sprintf( buf, TMP_PFX"p.%s", _pname);
 if ( !( Ofp = fopen( buf, "wb"))) {
   printf( "ERR: can't create file %s: %s(%d)\n", buf, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 // reading partition...
 if ( dbg) printf( "dbg0: partition %ld bytes\n", P->file_size);
 int ret = RW( Ifp, P->file_offset, Ofp, 0, P->file_size);
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
 off_t Isize = _siz( Ifp);
 if ( Isize == 0) {
   printf( "ERR: stat failed: %s(%d)\n", strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 // search for the partition
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {
   printf( "ERR: partition '%s' not found\n", _pname);
   fclose( Ifp);  return( 1);  }
 if ( dbg) printf( "dbg0: Found '%s'[%d] partition at 0x%X\n", _pname, pIdx, P->file_offset);
 isp_part_t *Pn = &( HDR.partition_info[ pIdx + 1]);    // next partition
 if ( Pn->file_offset < 1) Pn = NULL;
 if ( dbg &&  Pn) printf( "dbg0: Next part '%s' is at 0x%X\n", Pn->file_name, Pn->file_offset);
 if ( dbg && !Pn) printf( "dbg0: This partition is last one\n");
 if ( dbg) printf( "dbg0: (I/P/F) size: %ld/%ld/%ld\n", Isize, P->partition_size, P->file_size);
 off_t Psize = P->partition_size;
 // if RESERVED partition size ends after the image file end... 
 if ( P->partition_size + P->file_offset > ( uint64_t)Isize) {
   printf( "WRN: Part ends at 0x%lX, after the EOF:0x%lX\n", Psize + P->file_offset, Isize);
   Psize = P->file_size;
   printf( "WRN: assuming Psize = %ld (file_size)\n", Psize);  }
 if ( Pn && ( P->file_offset + Psize) > Pn->file_offset) {
   printf( "WRN: Part ends at 0x%lX, after next part start offset:0x%X\n", P->file_offset + Psize, Pn->file_offset);
   Psize = Pn->file_offset - P->file_offset;
   printf( "WRN: assuming Psize = %ld (offsets diff)\n", Psize);  }
 off_t truncate_size = Isize - Psize;
 off_t R_off = P->file_offset + Psize;      // read starting from ...
 off_t W_off = P->file_offset;              // write starting from ...
 off_t move_size = Isize - ( P->file_offset + Psize);
 if ( move_size < 1) printf( "WRN: 0 bytes to move ?!\n");
 if ( dbg) printf( "dbg0: %ld bytes to move, %ld bytes to delete\n", move_size, Psize);
 if ( RW( Ifp, R_off, Ifp, W_off, move_size) != 0) {
   fclose( Ifp);  return( 1);  }
 if ( dbg) printf( "dbg0: %ld bytes moved from 0x%lX to 0x%lX\n", move_size, R_off, W_off);
 // wipe part info from header and save it
 if ( dbg) printf( "dbg0: Updating HDR...\n");
 // move parts pointers if there are something after
 for ( int i = pIdx; i < NUM_OF_PARTITION; i++) {
   P = &( HDR.partition_info[ i]);
   if ( i > 0 && P->file_offset < 1) memset( P, 0, sizeof( *P));
   memcpy( P, &( HDR.partition_info[ i + 1]), sizeof( *P));
   if ( i > 0 && P->file_offset < 1) break;
   if ( dbg) printf( "dbg0: shift '%s'[%d] 0x%X -0x%lX\n", P->file_name, i, P->file_offset, Psize);
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
 off_t Isize = _siz( Ifp);
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
   printf( "PARTITION[%d]\n", i);
   printf( "\tfilename: %s\n", P->file_name);
   printf( "\tmd5sum: %s\n", P->md5sum);
   printf( "\tfile offset: 0x%X\n", P->file_offset);
   printf( "\tfile size: %ld\n", P->file_size);
   printf( "\tpart start addr: 0x%X\n", P->partition_start_addr);
   printf( "\tpart size: %ld\n", P->partition_size);
   printf( "\tflags: 0x%X\n", P->flags);
   printf( "\temmc part start block: %d\n", P->emmc_partition_start);
   if ( Isize < leop) {
     printf( "WRN: virtually ends at %ld (%ld bytes after)\n", leop, ( leop - Isize));
     printf( "WRN: image eof is at %ld\n", Isize);
   }
 }
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( 0);  }

// _m == "rb", ...
FILE *ispimg_R_hdr( const char *_fname, const char *_m, isp_hdr_t &_HDR) {
 FILE *Ifp;
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 if ( !( Ifp = fopen( _fname, _m))) {
   printf( "ERR: %s img '%s': %s(%d)\n", __FUNCTION__, _fname, strerror( errno), errno);
   return( NULL);  }
 long off_hdr = OFF_HDR;
 if ( _pos( Ifp, off_hdr) != 0) {
   printf( "ERR: HDR of '%s' can't pos at 0x%lX\n", _fname, off_hdr);
   fclose( Ifp);  return( NULL);  }
 if ( dbg) printf( "dbg0: HDR reading %ld bytes at 0x%lX\n", sizeof( _HDR), off_hdr);
 if ( fread( &_HDR, sizeof( _HDR), 1, Ifp) != 1) {
   printf( "ERR: HDR of '%s' reading 0x%lX\n", _fname, off_hdr);
   fclose( Ifp);  return( NULL);  }
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( Ifp);  }

// write header
int ispimg_W_hdr( FILE *_fp, isp_hdr_t &_HDR) {
 if ( dbg > 1) printf( "dbg1: %s()\n", __FUNCTION__);
 long off_hdr = OFF_HDR;
 if ( _pos( _fp, off_hdr) != 0) {
   printf( "ERR: HDR can't pos at 0x%lX\n", off_hdr);
   return( -1);  }
 if ( dbg) printf( "dbg0: HDR writing %ld bytes at 0x%lX\n", sizeof( _HDR), off_hdr);
 if ( fwrite( &_HDR, sizeof( _HDR), 1, _fp) != 1) {
   printf( "ERR: HDR writing at 0x%lX\n", off_hdr);
   return( -1);  }
 if ( dbg > 1) printf( "dbg1: %s() /\n", __FUNCTION__);
 return( 0);  }

int _pos( FILE *_F, off_t _p) {
 if ( fseek( _F, _p, SEEK_SET) != 0) return( 1);
 long off_cur = ftell( _F);
 if ( _p != off_cur) return( 1);
 return( 0);  }

off_t _siz( FILE *_F) {
 struct stat fst;
 if ( fstat( fileno( _F), &fst) != 0) return( 0);
 return( fst.st_size);  }

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
