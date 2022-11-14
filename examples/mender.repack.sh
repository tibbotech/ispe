#!/bin/bash

. ${ISPEDIR}ispe-helpers/sh.defs

f0pfx="./xxx/"
f1pfx="./xxx/"

f0="ISPBOOOT.BIN.orig"
f1="ISPBOOOT.BIN"

I0="${f0pfx}${f0}"
I1="${f1pfx}${f1}"

install -d ${f0pfx}
install -d ${f1pfx}

if [ ! -f "${I0}" ]; then
  echo "No ${I0} found, downloading it"
  curl https://archives.tibbo.com/downloads/LTPS/FW/LTPPg2/${f1} > ${I0}
fi;
if [ ! -f "./img-tst-tini-tppg2.ext4" ]; then
  echo "No ./img-tst-tini-tppg2.ext4";
  exit 1;
fi;

${ISPEDIR}ispe ${I0} list
${ISPEDIR}ispe ${I1} crea

echo "Reassembling ${I1} from ${I0} ..."
output=$(${ISPEDIR}ispe ${I0} list)
echo "${output}" | grep "filename" | while read x0 pname; do
  echo "Extracting ${pname} ..."
  ${ISPEDIR}ispe ${I0} part "${pname}" extp;
  ${ISPEDIR}ispe ${I1} part "${pname}" addp;
  ${ISPEDIR}ispe ${I1} part "${pname}" file ./isp.p.${pname}
  rm -rf ./isp.p.${pname}
done;

echo "Extracting pre-header area (xboot0,64K) from ${I0} ..."
${ISPEDIR}ispe ${I0} extb 0x00 $((64*1024))
${ISPEDIR}ispe ${I1} setb 0x00 ./isp.b.0.$((64*1024))

echo "Extracting pre-header area (uboot0,960K) from ${I0} ..."
${ISPEDIR}ispe ${I0} extb 0x10000 $((960*1024))
${ISPEDIR}ispe ${I1} setb 0x10000 ./isp.b.10000.$((960*1024))

echo "Extracting script from ${I0} ..."
${ISPEDIR}ispe ${I0} head exts
${ISPEDIR}ispe ${I1} head sets ./isp.h.script.raw

echo "Adding new partition to ${I1} ..."
${ISPEDIR}ispe ${I1} part "rootfsB" addp
${ISPEDIR}ispe ${I1} part "rootfsB" file ./img-tst-tini-tppg2.ext4

#echo "Set HDR flag = 1"
#${ISPEDIR}ispe ${I1} head flag 0x01

echo "${output}" | grep "filename" | while read x0 pname; do
  pinfo=$(dv_part "${output}" "${pname}")
  p_emmc0=$(dv_part_info "${pinfo}" "emmc part start block: ")
  echo "Setting ${p_emmc0} EMMC offset for ${pname}"
  ${ISPEDIR}ispe ${I1} part "${pname}" emmc ${p_emmc0}
done;

echo "Fixing xboot to 0x0 EMMC off..."
${ISPEDIR}ispe ${I1} part "xboot1" emmc 0x0

echo "Fixing rootfsB to EMMC (4GB - 500MB) off..."
${ISPEDIR}ispe ${I1} part "rootfsB" emmc $(printf '0x%x' $(((4*1024*1024*1024-500*1024*1024)/512)))

echo "Generating the ISP script..."
${ISPEDIR}ispe-helpers/genisp.emmc.sh ${I1} ${f1pfx}emmc.txt
${ISPEDIR}ispe-helpers/script_enc.sh "ISP Script" ${f1pfx}emmc.txt ${f1pfx}emmc.raw

echo "Installing the ISP script..."
${ISPEDIR}ispe ${I1} -vv setb eof ${f1pfx}emmc.raw

output=$(${ISPEDIR}ispe ${I1} list)
LP=$(echo "${output}" | grep "Last part " | sed -e 's/Last part EOF: 0x//')
TL=$(echo "${output}" | grep "Tail data " | sed -e 's/Tail data len: //')
echo "LP:${LP}"
echo "TL:${TL}"

echo "Installing the HDR script..."
cat ${ISPEDIR}ispe-templates/sp7021.hdr.T | sed -e "s/{T_OFF}/0x${LP}/" -e "s/{T_SIZE}/0x${TLh}/" > ${f1pfx}/head.script.txt
${ISPEDIR}ispe-helpers/script_enc.sh "Init ISP Script" ${f1pfx}/head.script.txt ${f1pfx}/head.script.raw
${ISPEDIR}ispe ${I1} head sets ${f1pfx}/head.script.raw

ret=0
exit $res
