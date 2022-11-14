#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#include <arpa/inet.h>  // htohl
#include <unistd.h>     // ftruncate
#include <sys/stat.h>   // stat()
#include <stdlib.h>     // strtol()
#include <openssl/md5.h>    // openssl/md5 funcs
#include "isp.h"

int RW( FILE *_R, const off_t _Roff, FILE *_W, const off_t _Woff, const off_t _len) {
 if ( _len == 0) return( 0);
 // if files are different - able to pos once
 if ( _R != _W) {
   if ( _Roff) DBG(1, "R seek to 0x%lx", _Roff);
   if ( _Roff && _pos( _R, _Roff) != 0) {
     ERR( "seek0 R at 0x%lX: %s(%d)", _Roff, strerror( errno), errno);
     return( 1);  }
   if ( _Woff) DBG(1, "W seek to 0x%lx", _Woff);
   if ( _Woff && _pos( _W, _Woff) != 0) {
     ERR( "seek0 W at 0x%lX: %s(%d)", _Woff, strerror( errno), errno);
     return( 1);  }
 }
 off_t bdone = 0, R0 = _Roff, W0 = _Woff;
 char buf[ 2048];
 size_t sz = sizeof( buf), szr = 0; 
 if ( _len < 0) {  R0 -= sz;  W0 -= sz;  }
 while ( 1) {
   if ( _len > 0 && bdone >= _len) break;
   if ( _len < 0 && bdone <= _len) break;
   if ( _len > 0 && ( off_t)sz > ( _len - bdone)) sz = ( _len - bdone);
   if ( _len < 0 && ( off_t)sz > ( bdone - _len)) sz = ( bdone - _len);
   DBG(4, "R %ld bytes at 0x%lX", sz, R0 + bdone);
   if ( _R == _W && _pos( _R, R0 + bdone) != 0) { 
     ERR( "seek1 R at pos 0x%lX", R0 + bdone);  return( 1);  }
   if ( ( szr = fread( buf, 1, sz, _R)) < 0) {
     ERR( "R %ld bytes: %s(%d)", sz, strerror( errno), errno);
     return( 1);  }
   if ( szr < 1) {  DBG(4, "R eof");  break;  }
   DBG(4, "W %ld bytes at 0x%lX", szr, W0 + bdone);
   if ( _R == _W && _pos( _W, W0 + bdone) != 0) { 
     ERR( "seek1 W at 0x%lX", W0 + bdone);  return( 1);  }
   if ( fwrite( buf, szr, 1, _W) != 1) {
     ERR( "W %ld bytes: %s(%d)", szr, strerror( errno), errno);
     return( 1);  }
   bdone += ( _len > 0 ? szr : -szr);  }
 return( 0); }

isp_part_t *find_part( isp_hdr_t &_hdr, const char *_pname, uint8_t &_idx) {
 isp_part_t *P = NULL;
 DBG(2,"%s('%s')", __FUNCTION__, _pname);
 for ( int i = 0; i < NUM_OF_PARTITION; i++) {
   if ( i > 0 && _hdr.partition_info[ i].file_offset < 1) continue;
   if ( strcmp( _pname, ( const char *)_hdr.partition_info[ i].file_name) != 0) continue;
   P = &( _hdr.partition_info[ i]);  _idx = i;  break;  }
 DBG(2,"%s() r:%s", __FUNCTION__, ( P ? "found" : "not found"));
 return( P);  }
 
// _m == "rb", ...
FILE *ispimg_R_hdr( const char *_fname, const char *_m, isp_hdr_t &_HDR) {
 FILE *Ifp;
 DBG(3, "%s()", __FUNCTION__);
 if ( !( Ifp = fopen( _fname, _m))) {
   ERR( "Rhdr '%s': %s(%d)", _fname, strerror( errno), errno);
   return( NULL);  }
 long off_hdr = OFF_HDR;
 if ( _pos( Ifp, off_hdr) != 0) {
   ERR( "Rhdr '%s' can't pos to 0x%lX", _fname, off_hdr);
   fclose( Ifp);  return( NULL);  }
 DBG(1, "Rhdr R %ld bytes at 0x%lX", sizeof( _HDR), off_hdr);
 if ( fread( &_HDR, sizeof( _HDR), 1, Ifp) != 1) {
   ERR( "Rhdr R of '%s' at 0x%lX: %s(%d)", _fname, off_hdr, strerror(errno), errno);
   fclose( Ifp);  return( NULL);  }
 DBG(3, "%s() /", __FUNCTION__);
 return( Ifp);  }

// write header
int ispimg_W_hdr( FILE *_fp, isp_hdr_t &_HDR) {
 DBG(3, "%s()", __FUNCTION__);
 long off_hdr = OFF_HDR;
 if ( _pos( _fp, off_hdr) != 0) {
   ERR( "Whdr can't pos at 0x%lX", off_hdr);
   return( -1);  }
 DBG(1, "Whdr W %ld bytes at 0x%lX", sizeof( _HDR), off_hdr);
 if ( fwrite( &_HDR, sizeof( _HDR), 1, _fp) != 1) {
   ERR( "Whdr W at 0x%lX: %s(%d)", off_hdr, strerror( errno), errno);
   return( -1);  }
 DBG(3, "%s() /", __FUNCTION__);
 return( 0);  }

int _pos( FILE *_F, off_t _p) {
 if ( fseek( _F, _p, SEEK_SET) != 0) return( 1);
 long off_cur = ftell( _F);
 if ( _p != off_cur) return( 1);
 return( 0);  }

off_t _pos_eof( FILE *_F) {
 if ( fseek( _F, 0, SEEK_END) != 0) return( 1);
 off_t off_cur = ftell( _F);
// if ( _p != off_cur) return( 1);
 return( off_cur);  }

off_t _siz( FILE *_F) {
 struct stat fst;
 if ( fstat( fileno( _F), &fst) != 0) return( 0);
 return( fst.st_size);  }

void init_script_hdr_parse( const unsigned char *_s, isp_hdr_script_t &_x) {
 memcpy( &_x, _s + FIT_HDR_OFF, sizeof( _x));
 _x.l = ntohl( _x.l);
 return;  }

void p_sch( uint8_t *_x, uint32_t _s) {
 for ( uint32_t i = 0; i < _s; i++) printf( "%c", ( _x[ i] == 0 ? ' ' : _x[ i]));
 return;  }
void p_hex( uint8_t *_x, uint32_t _s) {
 for ( uint32_t i = 0; i < _s; i++) printf( "%01X", _x[ i]);
 return;  }

int md5sum( FILE *_F, char *_s) {
 MD5_CTX c;
 isp_part_t P;
 DBG(3, "%s()", __FUNCTION__);
 if ( sizeof( P.md5sum) < MD5_DIGEST_LENGTH) {
   ERR( "%s() sizeof( P.md5sum)=%ld < MD5_DIGEST_LENGTH=%d", __FUNCTION__, sizeof( P.md5sum), MD5_DIGEST_LENGTH);
   return( 1);  }

 DBG(4, "md5_init...");
 MD5_Init( &c);
 _pos( _F, 0);
 
 DBG(4, "md5 reading data...");
 unsigned char out[MD5_DIGEST_LENGTH];
 char buf[ 2048];
 size_t sz = sizeof( buf), szr = 0;
 while ( sz > 0) {
   if ( ( szr = fread( buf, 1, sz, _F)) < 0) {
     ERR( "%s R %s(%d)", __FUNCTION__, strerror( errno), errno);
     return( 1);  }
   if ( szr < 1) break;
   MD5_Update( &c, buf, szr);
 }
 DBG(4, "md5_fin...");
 MD5_Final( out, &c);
 memset( _s, 0, sizeof( P.md5sum));
 for ( int i = 0; i < MD5_DIGEST_LENGTH; i++) sprintf( _s + i*2, "%02x", out[ i]);
 DBG(3, "%s() /", __FUNCTION__);
 return( 0);  }
