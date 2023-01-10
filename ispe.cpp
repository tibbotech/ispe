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
#define IMAGE_NAME "ISP_image"

uint8_t dbg = 0;

//#define DBG(L,fmt,args...) if ( dbg > (L)) printf( "dbg"#L fmt"\n", ##args);

int isp_list( const char *_fname);
int isp_crea( const char *_fname);
int isp_extb( const char *_fname, const off_t _off, const size_t _len);
int isp_setb( const char *_fname, const char *_offs, const char *_pname);
int isp_head_exts( const char *_fname);
int isp_head_sets( const char *_fname, const char *_sname);
int isp_head_flag( const char *_fname, const off_t _val);
int isp_part_extp( const char *_fname, const char *_pname);
int isp_part_addp( const char *_fname, const char *_pname);
int isp_part_dele( const char *_fname, const char *_pname);
int isp_part_file( const char *_fname, const char *_pname, const char *_file);
int isp_part_flag( const char *_fname, const char *_pname, const off_t _val);
int isp_part_size( const char *_fname, const char *_pname, const off_t _val);
int isp_part_nand( const char *_fname, const char *_pname, const off_t _val);
int isp_part_emmc( const char *_fname, const char *_pname, const off_t _val);

int main( int argc, char *argv[]) {
 if ( argc < 2 || ( argc > 1 && argv[ 1][ 0] == '-')) {
   printf( "Usage: %s <img> [-v] <cmd> [cmdparams]\n", argv[ 0]);
   printf( "\t[-v] verbose mode (-vvv.. increase verbosity)\n");
   printf( "\t<cmd> [params] one of the following:\n");
   printf( "\tlist - list partitions in the image\n");
   printf( "\tcrea - create an empty image\n");
   printf( "\textb <0xXX> <dlen> - extract <dlen> (dec) bytes at XX offset\n");
   printf( "\tsetb <0xXX> <name> - save raw binary file <name> at <0xXX> offset\n");
   printf( "\thead exts - extract header script\n");
   printf( "\thead flag <0xXX> - set image header flag\n");
   printf( "\thead sets <file> - update header script from script image file\n");
   printf( "\tpart <name> dele - delete the partition from the image\n");
   printf( "\tpart <name> addp - create new partition\n");
   printf( "\tpart <name> extp - extract partition to ...\n");
   printf( "\tpart <name> file <file> - load data for the partition from the raw file\n");
   printf( "\tpart <name> flag <0xXX> - set flag = 0xXX to partition <name>\n");
   printf( "\tpart <name> size <0xXX> - set size = 0xXX to partition <name>\n");
   printf( "\tpart <name> nand <0xXX> - set NAND offset = 0xXX to partition <name>\n");
   printf( "\tpart <name> emmc <0xXX> - set EMMC start block off = 0xXX\n");
   return( 1);  }
 uint8_t aoff = 2, i = 0;
 if ( argc > aoff) while ( argv[ aoff][ ++i] == 'v') dbg++;
 DBG(1, "Verbose mode%d", dbg);
 if ( dbg) aoff++;
 char *Iname = argv[ 1];
 DBG(1, "IMG: %s", Iname);
 if ( argc <= aoff) {
   ERR( "No cmd, run %s for help", argv[ 0]);
   return( 1);  }
 char *cmd = argv[ aoff];
 char *arg0 = NULL, *arg1 = NULL, *arg2 = NULL;
 if ( strcmp( cmd, "head") == 0) {
   aoff++;
   if ( argc - aoff < 1) {
     ERR( "At least 1 parameters is reqired. See help");
     return( 1);  }
 }
 if ( strcmp( cmd, "extb") == 0 || strcmp( cmd, "setb") == 0 ||
      strcmp( cmd, "part") == 0
   ) {
   if ( argc - aoff < 3) {
     ERR( "At least 2 parameters are reqired. See help");
     return( 1);  }
   aoff++;
 }
 arg0 = argv[ aoff + 0];
 arg1 = argv[ aoff + 1];
 arg2 = argv[ aoff + 2];
 if ( !cmd) {
   ERR( "Unknown cmd, run %s for help", argv[ 0]);
   return( 1);  }
 DBG(1, "CMD %s", cmd);
 if ( sizeof( isp_part_t) != SIZE_PARTITION_INFO_S) {
   ERR( "sizeof(partition_info) != %d. Check your compiler", SIZE_PARTITION_INFO_S);
   return( 1);  }
 int ret = -100;
 if ( strcmp( cmd, "list") == 0) ret = isp_list( Iname);
 if ( strcmp( cmd, "crea") == 0) ret = isp_crea( Iname);
 if ( strcmp( cmd, "extb") == 0) ret = isp_extb( Iname, strtol( arg0, NULL, 16), atol( arg1));
 if ( strcmp( cmd, "setb") == 0) ret = isp_setb( Iname, arg0, arg1);
 while ( strcmp( cmd, "head") == 0) {
   if ( strcmp( arg0, "exts") == 0) {  ret = isp_head_exts( Iname);  break;  }
   if ( !arg1) {  ERR( "1 more arg required");  break;  }
   if ( strcmp( arg0, "flag") == 0) {  ret = isp_head_flag( Iname, strtol( arg1, NULL, 16));  break;  }
   if ( strcmp( arg0, "sets") == 0) {  ret = isp_head_sets( Iname, arg1);  break;  }
   break;  }
 while ( strcmp( cmd, "part") == 0) {
   if ( strcmp( arg1, "extp") == 0) {  ret = isp_part_extp( Iname, arg0);  break;  }
   if ( strcmp( arg1, "addp") == 0) {  ret = isp_part_addp( Iname, arg0);  break;  }
   if ( strcmp( arg1, "dele") == 0) {  ret = isp_part_dele( Iname, arg0);  break;  }
   if ( !arg2) {  ERR( "1 more arg required");  break;  }
   if ( strcmp( arg1, "file") == 0) {  ret = isp_part_file( Iname, arg0, arg2);  break;  }
   if ( strcmp( arg1, "flag") == 0) {  ret = isp_part_flag( Iname, arg0, strtol( arg2, NULL, 16));  break;  }
   if ( strcmp( arg1, "size") == 0) {  ret = isp_part_size( Iname, arg0, strtol( arg2, NULL, 16));  break;  }
   if ( strcmp( arg1, "nand") == 0) {  ret = isp_part_nand( Iname, arg0, strtol( arg2, NULL, 16));  break;  }
   if ( strcmp( arg1, "emmc") == 0) {  ret = isp_part_emmc( Iname, arg0, strtol( arg2, NULL, 16));  break;  }
   break;  }
 if ( ret == -100) {
   ERR( "Unknown cmd or wrong parameters, run %s for help", argv[ 0]);
   return( 1);  }
 return( ret);  }

// create an empty image
int isp_crea( const char *_fname) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = fopen( _fname, "w+b");
 if ( !Ifp) {
   ERR( "create file %s: %s(%d)", _fname, strerror(errno), errno);
   return( 1);  }
 memset( &HDR, 0, sizeof( HDR));
 strcpy( ( char *)( HDR.signature), IMAGE_NAME);
 int ret = ispimg_W_hdr( Ifp, HDR);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// set header flag
int isp_head_flag( const char *_fname, const off_t _val) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 HDR.flags = _val;
 int ret = ispimg_W_hdr( Ifp, HDR);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// add new partition
int isp_part_addp( const char *_fname, const char *_pname) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 // find first empty partition
 off_t lastp_off = OFF_HDR + sizeof( HDR);
 isp_part_t *P = NULL, *Pp = NULL;
 for ( int i = 0; i < NUM_OF_PARTITION; i++) {
   Pp = &( HDR.partition_info[ i]);
   if ( Pp->file_offset > 0) {
     if ( Pp->file_size > 0) lastp_off = ( Pp->file_offset + Pp->file_size);
     continue;  }
   P = Pp;  break;  }
 if ( !P) {  ERR( "no free partitions?!");  fclose( Ifp);  return( 1);  }
 DBG(2, "Last part off:%lX", lastp_off);
 strcpy( ( char *)P->file_name, _pname);
 P->file_offset = lastp_off;
 int ret = ispimg_W_hdr( Ifp, HDR);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

int isp_part_file( const char *_fname, const char *_pname, const char *_file) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {  ERR( "part '%s' not found", _pname);  fclose( Ifp);  return( 1);  }
 FILE *Sfp;
 if ( !( Sfp = fopen( _file, "rb"))) {
   ERR( "open '%s': %s(%d)", _file, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 off_t Ssize = _siz( Sfp);
 off_t pad = ( EMMC_BLKSIZE - ( Ssize % EMMC_BLKSIZE));
 if ( pad == EMMC_BLKSIZE) pad = 0;
 if ( pad) DBG(1, "should pad %ld bytes", pad)
 off_t old_plen = P->file_size;
 off_t add_len = 0;
 if ( ( Ssize + pad) > old_plen) add_len = Ssize + pad - old_plen;

 isp_part_t *Pi = NULL;
 off_t move_at = 0;
 DBG(1, "shift to %ld bytes", add_len);
 for ( int i = pIdx + 1; add_len > 0 && i < NUM_OF_PARTITION; i++) {
   Pi = &( HDR.partition_info[ i]);
   if ( Pi->file_offset < 1) break;
   if ( move_at < 1) move_at = Pi->file_offset;
   DBG(1, "part '%s' off 0x%X --> 0x%lX", Pi->file_name, Pi->file_offset, Pi->file_offset + add_len);
   Pi->file_offset += add_len;  }
 // if next partition is found - move starting from its position
 off_t Isize = _siz( Ifp);
 int ret = 0;
 if ( move_at) {
   DBG(1, "--> %ld bytes at %lX for %ld", Isize - move_at, move_at, add_len);
   ret = RW( Ifp, Isize, Ifp, Isize + add_len, -( Isize - move_at), 0);
 }
 if ( ret != 0) {  fclose( Ifp);  fclose( Sfp);  return( ret);  }
 // save the partition table
 P->file_size = Ssize + pad;
 md5sum( Sfp, ( char *)P->md5sum, pad);
 if ( ispimg_W_hdr( Ifp, HDR) != 0) {  fclose( Ifp);  fclose( Sfp);  return( 1);  }
 DBG(1, "writing the body...");
 _pos( Sfp, 0);
 ret = RW( Sfp, 0, Ifp, P->file_offset, Ssize, pad);
 fclose( Sfp);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// set flag for the partition
int isp_part_flag( const char *_fname, const char *_pname, const off_t _val) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {  ERR( "part '%s' not found", _pname);  fclose( Ifp);  return( 1);  }
 P->flags = _val;
 int ret = ispimg_W_hdr( Ifp, HDR);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// set size for the partition
int isp_part_size( const char *_fname, const char *_pname, const off_t _val) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {  ERR( "part '%s' not found", _pname);  fclose( Ifp);  return( 1);  }
 P->partition_size = _val;
 int ret = ispimg_W_hdr( Ifp, HDR);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// set nand OFF for the partition
int isp_part_nand( const char *_fname, const char *_pname, const off_t _val) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {  ERR( "part '%s' not found", _pname);  fclose( Ifp);  return( 1);  }
 P->partition_start_addr = _val;
 int ret = ispimg_W_hdr( Ifp, HDR);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// set emmc OFF for the partition
int isp_part_emmc( const char *_fname, const char *_pname, const off_t _val) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {  ERR( "part '%s' not found", _pname);  fclose( Ifp);  return( 1);  }
 P->emmc_partition_start = _val;
 int ret = ispimg_W_hdr( Ifp, HDR);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// set script for the image
int isp_head_sets( const char *_fname, const char *_sname) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);

 FILE *Sfp;
 if ( !( Sfp = fopen( _sname, "rb"))) {
   ERR( "open '%s': %s(%d)", _sname, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 char buf[ sizeof( HDR.init_script)];
 memset( buf, 0, sizeof( buf));
 size_t szr = 0;
 // read script image from file
 if ( ( szr = fread( buf, 1, sizeof( buf), Sfp)) < sizeof( isp_hdr_script_t)) {
   ERR( "read %ld bytes failed: %s(%d) %ld", sizeof( buf), strerror( errno), errno, szr);
   fclose( Ifp);  fclose( Sfp);  return( 1);  }
 fclose( Sfp);
 // try to detect the script size
 isp_hdr_script_t xxx0;
 init_script_hdr_parse( ( unsigned char *)buf, xxx0);
 DBG(1, "script img parsed size: %d", xxx0.l);
 if ( xxx0.l > ( sizeof( HDR.init_script) - FIT_HDR_OFF - sizeof( xxx0))) {
   ERR( "parsed script size: %d > %ld (%ld-%d-%ld)", xxx0.l, sizeof( HDR.init_script) - FIT_HDR_OFF - sizeof( xxx0), sizeof( HDR.init_script), FIT_HDR_OFF, sizeof( xxx0));
   fclose( Ifp);  return( 1);  }
 if ( xxx0.l < 1) {
   printf( "WRN: parsed script len=0. Probably the script image is wrong.\n");
 }
 // go to script position
 long off_hdr = OFF_HDR + offsetof( isp_hdr_t, init_script);
 if ( _pos( Ifp, off_hdr) != 0) {
   ERR( "'%s' at pos 0x%lX: %s(%d)", _fname, off_hdr, strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 // write it as-is
 memcpy( HDR.init_script, buf, sizeof( buf));
 if ( fwrite( HDR.init_script, sizeof( buf), 1, Ifp) != 1) {
   ERR( "write %ld bytes failed: %s(%d)", sizeof( buf), strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 // writing the script part /
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( 0);  }

// save 
int isp_setb( const char *_fname, const char *_offs, const char *_sname) {
 DBG(3, "%s()", __FUNCTION__);
 FILE *Sfp;
 off_t off = -1;
 if ( !_offs) {  ERR( "offset is null");  return( 1);  }
 if ( strcasecmp( _offs, "eof") != 0) off = strtol( _offs, NULL, 16);
 if ( off <  0) DBG(1, "Add to the EOF");
 if ( off >= 0) DBG(1, "Insert at 0x%lX", off);
 if ( !( Sfp = fopen( _sname, "rb"))) {
   ERR( "open '%s': %s(%d)", _sname, strerror(errno), errno);
   return( 1);  }
 off_t Ssize = _siz( Sfp);
 if ( Ssize == 0) {
   ERR( "stat failed: %s(%d)", strerror( errno), errno);
   fclose( Sfp);  return( 1);  }
// off_t pad = ( EMMC_BLKSIZE - ( Ssize % EMMC_BLKSIZE));
// if ( pad == EMMC_BLKSIZE) pad = 0;
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 if ( off == -1) {
   off = _pos_eof( Ifp);
   DBG(1,"EOF is at 0x%lx", off);
 }
 // find the last part off
 off_t lastp_off = OFF_HDR + sizeof( HDR);
 isp_part_t *Pi = NULL;
 for ( int i = 0; i < NUM_OF_PARTITION; i++) {
   Pi = &( HDR.partition_info[ i]);
   if ( Pi->file_offset > 0) lastp_off = Pi->file_offset;
   if ( Pi->file_size < 1) continue;
   lastp_off += Pi->file_size;  }
 DBG(1, "last partition data offset eof:0x%lX", lastp_off);
 if ( off + Ssize/* + pad*/ > OFF_HDR && off < lastp_off) {
   ERR( "0x%X (HDR) <= 0x%lX (off) < 0x%lX (last partition data offset)", OFF_HDR, off, lastp_off);
   fclose( Ifp);  fclose( Sfp);  return( 1);  }
// // FIXME: test for size
// if ( off + Ssize >= OFF_HDR) {
//   ERR( "0x%lX (off) + %ld (file size) > 0x%X (HDR off)", off, Ssize, OFF_HDR);
//   fclose( Ifp);  fclose( Sfp);  return( 1);  }
 // read and write
 DBG(1, "Writing %ld bytes at 0x%lX", Ssize, off);
 _pos( Ifp, off);
 int ret = RW( Sfp, 0, Ifp, off, Ssize, 0);
 // read and write /
 fclose( Ifp);
 fclose( Sfp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// extract script from the image
int isp_head_exts( const char *_fname) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "rb", HDR);
 if ( !Ifp) return( 1);
 fclose( Ifp);
 FILE *Ofp;
 char buf[ sizeof( HDR.init_script)];
 sprintf( buf, TMP_PFX"h.script.raw");
 if ( !( Ofp = fopen( buf, "wb"))) {
   ERR( "create '%s': %s(%d)", buf, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 memset( buf, 0, sizeof( buf));
 if ( fwrite( HDR.init_script, sizeof( buf), 1, Ofp) != 1) {
   ERR( "write %ld bytes failed: %s(%d)", sizeof( buf), strerror( errno), errno);
   fclose( Ifp);  fclose( Ofp);  return( 1);  }
 fclose( Ofp);
 DBG(1, "Trying to extract clear text");
 isp_hdr_script_t xxx0;
 init_script_hdr_parse( HDR.init_script, xxx0);
 DBG(1, "Text size should be %d", xxx0.l);
 size_t h_off = FIT_HDR_OFF + sizeof( xxx0);
 if ( xxx0.l > sizeof( HDR.init_script) - h_off) {
   ERR( "text size %d > %ld (%ld-%ld)", xxx0.l, (sizeof( HDR.init_script) - h_off), sizeof( HDR.init_script), h_off);
   fclose( Ifp);  return( 1);  }
 sprintf( buf, TMP_PFX"h.script.txt");
 if ( !( Ofp = fopen( buf, "wb"))) {
   ERR( "can't create file %s: %s(%d)", buf, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 if ( fwrite( HDR.init_script + h_off, 1, xxx0.l, Ofp) != xxx0.l) {
   ERR( "write %ld bytes failed: %s(%d)", sizeof( buf), strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 fclose( Ofp);
 DBG(3, "%s() /", __FUNCTION__);
 return( 0);  }

// extract _len bytes of binary data starting at _off
int isp_extb( const char *_fname, const off_t _off, const size_t _len) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "rb", HDR);
 if ( !Ifp) return( 1);
 FILE *Ofp;
 char buf[ 2048];
 sprintf( buf, TMP_PFX"b.%lX.%ld", _off, _len);
 if ( !( Ofp = fopen( buf, "wb"))) {
   ERR( "can't create file %s: %s(%d)", buf, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 DBG(1, "dumping %ld bytes from 0x%lX pos", _len, _off);
 _pos( Ifp, 0);
 int ret = RW( Ifp, _off, Ofp, 0, _len, 0);
 fclose( Ofp);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// extract partition from the image
int isp_part_extp( const char *_fname, const char *_pname) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 // search for the partition
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {  ERR( "part '%s' not found", _pname);  fclose( Ifp);  return( 1);  }
 DBG(1, "found '%s'[%d] part at 0x%X", _pname, pIdx, P->file_offset);
 FILE *Ofp;
 char buf[ 2048];
 sprintf( buf, TMP_PFX"p.%s", _pname);
 if ( !( Ofp = fopen( buf, "wb"))) {
   ERR( "can't create file %s: %s(%d)", buf, strerror(errno), errno);
   fclose( Ifp);  return( 1);  }
 // reading partition...
 DBG(1, "partition %ld bytes", P->file_size);
 int ret = RW( Ifp, P->file_offset, Ofp, 0, P->file_size, 0);
 // reading partition... /
 fclose( Ofp);
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( ret);  }

// delete partition from the image
int isp_part_dele( const char *_fname, const char *_pname) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "r+b", HDR);
 if ( !Ifp) return( 1);
 off_t Isize = _siz( Ifp);
 if ( Isize == 0) {
   ERR( "stat failed: %s(%d)", strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 // search for the partition
 uint8_t pIdx;
 isp_part_t *P = find_part( HDR, _pname, pIdx);
 if ( !P) {  ERR( "part '%s' not found", _pname);  fclose( Ifp);  return( 1);  }
 DBG(1, "Found '%s'[%d] partition at 0x%X", _pname, pIdx, P->file_offset);
 isp_part_t *Pn = &( HDR.partition_info[ pIdx + 1]);    // next partition
 if ( Pn->file_offset < 1) Pn = NULL;
 if (  Pn) DBG(1, "Next part '%s' is at 0x%X", Pn->file_name, Pn->file_offset);
 if ( !Pn) DBG(1, "This partition is last one");
 DBG(1, "(I/P/F) size: %ld/%ld/%ld", Isize, P->partition_size, P->file_size);
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
 DBG(1, "%ld bytes to move, %ld bytes to delete", move_size, Psize);
 if ( RW( Ifp, R_off, Ifp, W_off, move_size, 0) != 0) {
   fclose( Ifp);  return( 1);  }
 DBG(1, "%ld bytes moved from 0x%lX to 0x%lX", move_size, R_off, W_off);
 // wipe part info from header and save it
 DBG(1, "Updating HDR...");
 // move parts pointers if there are something after
 for ( int i = pIdx; i < NUM_OF_PARTITION; i++) {
   P = &( HDR.partition_info[ i]);
   if ( i > 0 && P->file_offset < 1) memset( P, 0, sizeof( *P));
   memcpy( P, &( HDR.partition_info[ i + 1]), sizeof( *P));
   if ( i > 0 && P->file_offset < 1) break;
   DBG(1, "shift '%s'[%d] 0x%X -0x%lX", P->file_name, i, P->file_offset, Psize);
   P->file_offset -= Psize;
 }
 if ( ispimg_W_hdr( Ifp, HDR) != 0) {  fclose( Ifp);  return( 1);  }
 // truncate the result
 DBG(1, "truncating I from %ld to %ld bytes (-%ld)...", Isize, truncate_size, ( Isize - truncate_size));
 if ( ftruncate( fileno( Ifp), truncate_size)) {
   ERR( "truncating: %s(%d)", strerror( errno), errno);
   fclose( Ifp);  return( 1);  }
 fclose( Ifp);
 DBG(3, "%s() /", __FUNCTION__);
 return( 0);  }

// show image info
int isp_list( const char *_fname) {
 DBG(3, "%s()", __FUNCTION__);
 isp_hdr_t HDR;
 FILE *Ifp = ispimg_R_hdr( _fname, "rb", HDR);
 if ( !Ifp) return( 1);
 off_t Isize = _siz( Ifp);
 fclose( Ifp);
 printf( "HEADER 0x%X :\n", OFF_HDR);
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
   printf( "\tfile size: %" PRIu64 "\n", P->file_size);
   printf( "\tpart start addr: 0x%X\n", P->partition_start_addr);
   printf( "\tpart size: %" PRIu64 "\n", P->partition_size);
   printf( "\tflags: 0x%X\n", P->flags);
   printf( "\temmc part start block: 0x%X\n", P->emmc_partition_start);
   if ( Isize < leop) {
     printf( "WRN: virtually ends at %ld (%ld bytes after)\n", leop, ( leop - Isize));
     printf( "WRN: image eof is at %ld\n", Isize);
   }
 }
 if ( P) {
   printf( "Last part EOF: 0x%lX\n", P->file_offset + P->file_size);
   printf( "Image EOF: 0x%lX\n", Isize);
   printf( "Tail data len: %ld\n", Isize - ( P->file_offset + P->file_size));
 }
 DBG(3, "%s() /", __FUNCTION__);
 return( 0);  }
