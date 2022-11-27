#!/bin/bash

pfx="./xxx"

T="${pfx}/ISPBOOOT.BIN"

install -d ${pfx}/

${ISPEDIR}ispe ${T} crea

# install xboot (0:64K) and uboot(64K:1M) before header
${ISPEDIR}ispe ${T} setb 0x00 ../tppg2-arm5/xboot-emmc.img
${ISPEDIR}ispe ${T} setb 0x10000 ./u-boot.bin-a7021_ppg2.img

${ISPEDIR}ispe ${T} part "xboot1" addp
${ISPEDIR}ispe ${T} part "xboot1" file ../tppg2-arm5/xboot-emmc.img
${ISPEDIR}ispe ${T} part "xboot1" emmc 0x0

${ISPEDIR}ispe ${T} part "uboot1" addp
${ISPEDIR}ispe ${T} part "uboot1" file ./u-boot.bin-a7021_ppg2.img
${ISPEDIR}ispe ${T} part "uboot1" emmc 0x22

${ISPEDIR}ispe ${T} part "uboot2" addp
${ISPEDIR}ispe ${T} part "uboot2" file ./u-boot.bin-a7021_ppg2.img
${ISPEDIR}ispe ${T} part "uboot2" emmc 0x822

${ISPEDIR}ispe ${T} part "env" addp
#${ISPEDIR}ispe ${T} part "env" file ./emptyfile
${ISPEDIR}ispe ${T} part "env" emmc 0x1022

${ISPEDIR}ispe ${T} part "env_redund" addp
#${ISPEDIR}ispe ${T} part "env_redund" file ./emptyfile
${ISPEDIR}ispe ${T} part "env_redund" emmc 0x1422

${ISPEDIR}ispe ${T} part "nonos" addp
${ISPEDIR}ispe ${T} part "nonos" file ../tppg2-arm5/a926-empty.img
${ISPEDIR}ispe ${T} part "nonos" emmc 0x1822

${ISPEDIR}ispe ${T} part "dtb" addp
${ISPEDIR}ispe ${T} part "dtb" file ./sp7021-ltpp3g2revD.dtb
${ISPEDIR}ispe ${T} part "dtb" emmc 0x2022

${ISPEDIR}ispe ${T} part "kernel" addp
${ISPEDIR}ispe ${T} part "kernel" file ./uImage-initramfs-tppg2.bin
${ISPEDIR}ispe ${T} part "kernel" emmc 0x2222

${ISPEDIR}ispe ${T} part "rootfs" addp
${ISPEDIR}ispe ${T} part "rootfs" file ./img-tps-free-tppg2.ext4
${ISPEDIR}ispe ${T} part "rootfs" emmc 0x12222

echo "Generating the ISP script..."
ISPEDIR=${ISPEDIR} ${ISPEDIR}ispe-helpers/genisp.emmc.sh ${T} ${pfx}/emmc.txt
${ISPEDIR}ispe-helpers/script_enc.sh "ISP Script" ${pfx}/emmc.txt ${pfx}/emmc.raw

echo "Installing the ISP script..."
${ISPEDIR}ispe ${T} -vv setb eof ${pfx}/emmc.raw

output=$(${ISPEDIR}ispe ${T} list)
LP=$(echo "${output}" | grep "Last part " | sed -e 's/Last part EOF: 0x//')
TL=$(echo "${output}" | grep "Tail data " | sed -e 's/Tail data len: //')
echo "LP:${LP}"
echo "TL:${TL}"
TLh=$(printf '%x' ${TL})

echo "Installing the HDR script..."
cat ${ISPEDIR}ispe-templates/sp7021.hdr.T | sed -e "s/{T_OFF}/0x${LP}/" -e "s/{T_SIZE}/0x${TLh}/" > ${pfx}/head.script.txt
${ISPEDIR}ispe-helpers/script_enc.sh "Init ISP Script" ${pfx}/head.script.txt ${pfx}/head.script.raw
${ISPEDIR}ispe ${T} head sets ${pfx}/head.script.raw

#${ISPEDIR}ispe ${T} list
