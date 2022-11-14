// Defines accepted externally:
// SOC_SPHE8368 : build for SPHE8368 SoC (header types are a bit different)
// example:
// make CFLAGS+="-DSOC_SPHE8368"

#ifndef ISP_H
#define ISP_H

#include <inttypes.h>

// SP7021: FILE_SIZE_IMAGE_XBOOT0( 64K) + FILE_SIZE_IMAGE_UBOOT0(940K)
// Q645:   FILE_SIZE_IMAGE_XBOOT0(160K) + FILE_SIZE_IMAGE_UBOOT0(864K)

#define OFF_HDR 0x100000

#define SIZE_FILE_NAME 32
#define SIZE_FULL_FILE_NAME 256

// is it the size of the structure?
#define SIZE_PARTITION_INFO_S 128

#define SIZE_INIT_SCRIPT 2048
// why this value?
#define NUM_OF_PARTITION 111

//#define DBG(L,fmt, ...) { if ( dbg > (L)) printf( "dbg"#L fmt"\n", __VA_ARGS__); }
#define DBG(L,fmt,args...) { if ( dbg > (L)) printf( "dbg"#L ": " fmt"\n", ##args); }
#define ERR(fmt,args...) { printf( "ERR: " fmt"\n", ##args); }

struct partition_info_s {
    uint8_t  file_name[SIZE_FILE_NAME];  // file name of source (basename only)
    uint8_t  md5sum[36];
    uint32_t file_offset;                // offset in output file
#ifdef SOC_SPHE8368
    uint32_t file_size;                  // file size of the partition
#else
    uint64_t file_size;                  // file size of the partition
#endif
    uint32_t partition_start_addr;       // partition's start address in NAND, there will be an offset added in U-Boot script ($isp_nand_addr_1st_part), less than 4GB is expected.
    // for files before argv[ARGC_PACK_IMAGE_BINARY_PARTITION1_FILE], it's zero
    // this value is none zero for files, which start from argv[ARGC_PACK_IMAGE_BINARY_PARTITION2_FILE]
    // Reason: Start address of followings are calculated at run-time:
    //     Header, XBoot-1, UBoot-1, ..., and Partition-0.
    // reserved size for this partition, less than 4GB is #else
#ifdef SOC_SPHE8368
    uint32_t partition_size;
#else
    uint64_t partition_size;
#endif
    uint32_t flags;
    uint32_t emmc_partition_start;       // Unit: block
    uint32_t reserved[7];                // let size of this structure == SIZE_PARTITION_INFO_S
} __attribute__((packed));
typedef struct partition_info_s isp_part_t;

struct file_header_s {
    uint8_t  signature[32];
    uint8_t  init_script[SIZE_INIT_SCRIPT];
    uint32_t flags;
    uint8_t reserved[ sizeof( isp_part_t) - 36];

    isp_part_t partition_info[NUM_OF_PARTITION];
} __attribute__((packed));
typedef file_header_s isp_hdr_t;

// https://formats.kaitai.io/uimage/
#define FIT_HDR_OFF 64
struct isp_hdr_script {
 uint32_t l;
 uint32_t x0;
};
typedef struct isp_hdr_script isp_hdr_script_t;

extern uint8_t dbg;

void p_hex( uint8_t *_x, uint32_t _s);
void p_sch( uint8_t *_x, uint32_t _s);
int _pos( FILE *_F, off_t _p);
off_t _siz( FILE *_F);
off_t _pos_eof( FILE *_F);
void init_script_hdr_parse( const unsigned char *_hdr, isp_hdr_script_t &_x);
FILE *ispimg_R_hdr( const char *_fname, const char *_m, isp_hdr_t &_HDR);
int ispimg_W_hdr( FILE *_fp, isp_hdr_t &_HDR);
isp_part_t *find_part( isp_hdr_t &_hdr, const char *_pname, uint8_t &_idx);
int RW( FILE *_R, const off_t _Roff, FILE *_W, const off_t _Woff, const off_t _len);

int md5sum( FILE *_F, char *_s);

#endif
