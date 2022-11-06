#ifndef ISP_H
#define ISP_H

#define SIZE_FILE_NAME 32
#define SIZE_FULL_FILE_NAME 256

// is it the size of the structure?
#define SIZE_PARTITION_INFO_S 128

#define SIZE_INIT_SCRIPT 2048
// why this value?
#define NUM_OF_PARTITION 111

struct partition_info_s {
    uint8_t  file_name[SIZE_FILE_NAME];  // file name of source (basename only)
    uint8_t  md5sum[36];
    uint32_t file_offset;                // offset in output file
    uint64_t file_size;                  // file size of the partition
    uint32_t partition_start_addr;       // partition's start address in NAND, there will be an offset added in U-Boot script ($isp_nand_addr_1st_part), less than 4GB is expected.
    // for files before argv[ARGC_PACK_IMAGE_BINARY_PARTITION1_FILE], it's zero
    // this value is none zero for files, which start from argv[ARGC_PACK_IMAGE_BINARY_PARTITION2_FILE]
    // Reason: Start address of followings are calculated at run-time:
    //     Header, XBoot-1, UBoot-1, ..., and Partition-0.
    uint64_t partition_size;             // reserved size for this partition, less than 4GB is expected.
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

// used internally
struct isp_info_s {
    struct file_header_s file_header;

    uint8_t full_file_name[NUM_OF_PARTITION][SIZE_FULL_FILE_NAME];
    uint8_t full_file_name_xboot0[SIZE_FULL_FILE_NAME];
    uint8_t full_file_name_uboot0[SIZE_FULL_FILE_NAME];
    uint8_t file_name_pack_image[SIZE_FULL_FILE_NAME];
    // u08 base_file_name_pack_image[SIZE_FILE_NAME];
    int nand_block_size;
    int idx_partition;
    uint32_t flags;
    FILE *fd;
    char file_disk_image[32];
    int idx_gpt_header_primary;
    uint8_t *key_ptr;
};

// https://formats.kaitai.io/uimage/
#define FIT_HDR_OFF 64
struct isp_hdr_script {
 uint32_t l;
 uint32_t x0;
};
typedef struct isp_hdr_script isp_hdr_script_t;

#endif
