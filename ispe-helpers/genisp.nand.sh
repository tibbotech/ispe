#!/bin/bash

# Creates ISP script for the image
# result is saved to $2 or isp.txt (if no $2)

if [ $# -lt 1 ]; then
  echo "Usage: $0 <image-file> [out-file]"
  exit 1;
fi;

. ${ISPEDIR}ispe-helpers/sh.defs

O="${2:-isp.txt}"

output=$(${ISPEDIR}ispe ${1} list)
if [ $? -ne 0 ]; then  echo "${output}";  exit 1;  fi;

# format NAND first
echo "echo \"Initializing NAND...\"" > ${O}
echo "nand info" >> ${O}
echo "nand erase.chip && nand bad" >> ${O}
echo "" >> ${O}
# clean partitions
echo "mtdparts default && mtdparts delall" >> ${O}
echo "" >> ${O}

echo "setenv isp_addr_next \${nand_erasesize}" >> ${O}
echo "setenv isp_mtdpart_size 0x\${isp_addr_next}" >> ${O}
echo "mtdparts add nand0 \${isp_mtdpart_size}@0x00000000 nand_header" >> ${O}
echo "printenv mtdparts" >> ${O}
echo "" >> ${O}
# u-boot CONFIG_ENV_OFFSET preconfigured value
echo "setenv isp_nand_addr_1st_part 0x400000" >> ${O}
echo "" >> ${O}

# starting NAND partitions...

##### set partition sizes array
declare -A pA_size
declare -A pA_start
i=-1
LLL=$(echo -e "${output}" | grep "filename:")
while read x part; do
  # do not mention xboot1 in GPT
  if [ "${part}" == "xboot1" ]; then  continue;  fi;
  i=$(($i+1))
  pinfo=$(dv_part "${output}" "${part}")
  p_start=$(dv_part_info "${pinfo}" "emmc part start block: ")
  pA_size[$i]="-";
  pA_start[$i]="${p_start}"
  # if previous partition size is not defined - set it now
  iP=$(($i-1))
  if [ $i -gt 0 ] && [ "${pA_size[$iP]}" == "-" ]; then
    p0=$(printf '%d' ${pA_start[$iP]})
    p1=$(printf '%d' ${pA_start[$i]})
    pA_size[$iP]=$(((($p1-$p0)/2)))
  fi;
  # check if part size defined in IMG
  sz=$(dv_part_info "${pinfo}" "part size: ")
  if [ "${sz}" != "0" ]; then  pA_size[$i]="${sz}";  continue;  fi;
  # partition size is not defined, calculate it on next iteration
done <<< "$LLL"
##### set partition sizes array /

BHDR_SZ=""
BBLK=0
# write the writers...
while read x part; do
  echo "echo writing ${part} contents ..." >> ${O}
  pinfo=$(dv_part "${output}" "${part}")
  p_flags=$(dv_part_info "${pinfo}" "flags: ")
  p_pos=$(dv_part_info "${pinfo}" "file offset: ")
  p_size0=$(dv_part_info "${pinfo}" "part size: ")
  p_size1=$(dv_part_info "${pinfo}" "file size: ")
  if [ "x${BHDR_SZ}" == "x" ]; then  BHDR_SZ=$(printf "0x%X" ${p_size1});  fi;
  p_nand0=$(dv_part_info "${pinfo}" "nand part start addr: ")
  if [ -z "${p_nand0}" ]; then
    echo "Warning: 'nand part start addr' is not defined for ${part}"
    p_nand0="0x0"
  fi;
  partsz=$(printf "0x%X" $p_size0)
  if [ "${part}" == "rootfs" ]; then  partsz="-";  fi;
  if [ "${p_flags}" != "0x0" ]; then
    echo "setexpr isp_nand_addr 0x\${isp_addr_next}" >> ${O}
  else
    echo "setexpr isp_nand_addr \$isp_nand_addr_1st_part + ${p_nand0}" >> ${O}
    echo "setenv isp_nand_addr 0x\${isp_nand_addr}" >> ${O}
  fi;
  printf "echo partition: %s, start from \$isp_nand_addr, size: 0x%X, programmed size: 0x%X\n" $part $p_size0 $p_size1 >> ${O}
  if [ "${p_flags}" != "0x0" ]; then
    echo "setexpr isp_mtdpart_size \${isp_nand_addr_1st_part} - \${isp_nand_addr}" >> ${O}
    echo "setenv isp_mtdpart_size 0x\${isp_mtdpart_size}" >> ${O}
  else
    echo "setenv isp_mtdpart_size ${partsz}" >> ${O}
  fi;
  echo "mtdparts add nand0 \${isp_mtdpart_size}@\${isp_nand_addr} ${part}" >> ${O}

  if [ "${part}" == "rootfs" ]; then
    echo "ubi part rootfs 2048" >> ${O}
    echo "ubi create rootfs ${partsz}" >> ${O}
  fi;
  # FIXME: in cycle p_emmc += off
  blocks=$(dv_blocks "$p_size1" 0x200000)
  for i in $blocks; do
    echo "fatload \$isp_if \$isp_dev \$isp_ram_addr /ISPBOOOT.BIN ${i} ${p_pos}" >> ${O}
    block=$(printf '0x%x' $((i/512)))
    if [ "${p_flags}" != "0x0" ]; then
      echo "setenv isp_nand_addr_write_bblk_${BBLK} \$isp_nand_addr" >> ${O}
      echo "bblk write bblk \$isp_ram_addr \$isp_nand_addr ${i}" >> ${O}
    elif [ "${part}" == "rootfs" ]; then
      echo "ubi write.part \$isp_ram_addr ${part} ${i}" >> ${O}
    else
      echo "nand write \$isp_ram_addr \${isp_addr_nand_write_next} ${i}" >> ${O}
    fi;
    p_pos=$(printf '0x%x' $((p_pos+${i})))
    p_emmc0=$(printf '0x%x' $((p_emmc0+(${i}/512))))
  done
  if [ "${p_flags}" != "0x0" ]; then  BBLK=$(($BBLK+1));  fi;
  echo "" >> ${O}
done <<< "$LLL"

echo "" >> ${O}

echo "echo programming header (boot parameters) ..." >> ${O}
echo "bblk write bhdr auto 0 ${BHDR_SZ}" >> ${O}
echo "" >> ${O}

# FIXME: really need?
echo "setenv part_sizes uboot2_1Menv_512Kenv_redund_512Knonos_1Mdtb_256Kkernel_32Mrootfs_1024M" >> ${O}
echo "" >> ${O}
echo "setenv isp_addr_next" >> ${O}
echo "setenv isp_addr_nand_read_next" >> ${O}
echo "setenv isp_addr_nand_write_next" >> ${O}
echo "setenv isp_ram_addr" >> ${O}
echo "setenv isp_mtdpart_size" >> ${O}
echo "setenv isp_nand_addr" >> ${O}
echo "setenv isp_nand_addr_1st_part" >> ${O}
echo "setenv isp_nand_addr_write_bblk_0" >> ${O}
echo "setenv isp_nand_addr_write_bblk_1" >> ${O}
echo "setenv isp_nand_addr_write_bblk_2" >> ${O}
echo "setenv isp_size_total" >> ${O}
echo "setenv md5sum_value" >> ${O}
echo "setenv script_addr" >> ${O}
echo "setenv isp_image_header_offset" >> ${O}
echo "setenv isp_main_storage" >> ${O}
echo "echo Following environment variables will be saved:" >> ${O}
echo "printenv" >> ${O}
echo "env save" >> ${O}
echo "env save" >> ${O}
echo "setenv isp_all_or_update_done 0x01" >> ${O}

echo "echo --------------------------" >> ${O}
echo "echo Done" >> ${O}

ret=0
exit $res
