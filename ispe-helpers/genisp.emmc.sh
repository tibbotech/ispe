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

# init EMMC first
echo "echo \"Initializing EMMC...\"" > ${O}
echo "mmc dev 0 && mmc rescan" >> ${O}
echo "" >> ${O}
# fill GPT with 0xFF
echo "mw.b \$isp_ram_addr 0xFF 0x4400 && mmc write \$isp_ram_addr 0x00 0x22" >> ${O}
echo "mmc dev 0 && mmc rescan" >> ${O}
echo "" >> ${O}

# generating UUIDs...
echo "uuid uuid_gpt_disk" >> ${O}
echo "${output}" | grep "filename:" | while read x part; do
  echo "uuid uuid_gpt_${part}" >> ${O}
done
echo "" >> ${O}

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

# generating partitions...
i=-1
echo -ne "setenv partitions \"uuid_disk=\${uuid_gpt_disk}" >> ${O}
echo "${output}" | grep "filename:" | while read x part; do
  # do not mention xboot1 in GPT
  if [ "${part}" == "xboot1" ]; then  continue;  fi;
  i=$(($i+1))
  pinfo=$(dv_part "${output}" "${part}")
  p_start=$(dv_part_info "${pinfo}" "emmc part start block: ")
  p_start=$(printf '0x%x' $((p_start*512)))
  p_size="${pA_size[$i]}"
  if [ "${p_size}" != "-" ]; then
    p_size="${p_size}KiB"
  fi;
  # echo "${part} at ${p_start} : ${p_size}"
  echo -ne ";name=${part},uuid=\${uuid_gpt_${part}}" >> ${O}
  echo -ne ",start=${p_start}" >> ${O}
  echo -ne ",size=${p_size}" >> ${O}
done
echo "\"" >> ${O}
echo "" >> ${O}

echo "" >> ${O}
echo "printenv partitions" >> ${O}
echo "" >> ${O}
echo "echo \"Writing GPT ...\"" >> ${O}
echo "gpt write mmc 0 \$partitions" >> ${O}
echo "" >> ${O}
echo "echo \"Initialize eMMC again due to GPT update ...\"" >> ${O}
echo "mmc dev 0 && mmc rescan" >> ${O}
echo "" >> ${O}
echo "mmc part" >> ${O}
echo "" >> ${O}
echo "setenv partitions" >> ${O}
echo "" >> ${O}

echo "setenv uuid_gpt_disk" >> ${O}
echo "${output}" | grep "filename:" | while read x part; do
  echo "setenv uuid_gpt_${part}" >> ${O}
done
echo "" >> ${O}

# write the writers...

echo "${output}" | grep "filename:" | while read x part; do
  echo "echo writing ${part} contents ..." >> ${O}
  pinfo=$(dv_part "${output}" "${part}")
  p_pos=$(dv_part_info "${pinfo}" "file offset: ")
  p_size0=$(dv_part_info "${pinfo}" "part size: ")
  p_size1=$(dv_part_info "${pinfo}" "file size: ")
  p_size=$p_size0
  if [ $p_size1 -gt $p_size0 ]; then p_size=$p_size1;  fi;
  p_len=${p_size}
  p_emmc0=$(dv_part_info "${pinfo}" "emmc part start block: ")
  if [ -z "${p_emmc0}" ]; then
    echo "Warning: 'emmc part start block' is not defined for ${part}"
    p_emmc0="0x0"
  fi;
  if [ "${part}" == "xboot1" ]; then
    echo "mmc partconf 0 0 7 1" >> ${O}
  fi;
  # FIXME: in cycle p_emmc += off
  blocks=$(dv_blocks "$p_size" 0x200000)
  for i in $blocks; do
    echo "fatload \$isp_if \$isp_dev \$isp_ram_addr /ISPBOOOT.BIN ${i} ${p_pos}" >> ${O}
    block=$(printf '0x%x' $((i/512)))
    echo "mmc write \$isp_ram_addr ${p_emmc0} ${block}" >> ${O}
    p_pos=$(printf '0x%x' $((p_pos+${i})))
    p_emmc0=$(printf '0x%x' $((p_emmc0+(${i}/512))))
  done
  if [ "${part}" == "xboot1" ]; then
    echo "mmc partconf 0 0 0 0" >> ${O}
  fi;
  echo "" >> ${O}
done;

echo "" >> ${O}

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
