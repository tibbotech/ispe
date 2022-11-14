#!/bin/bash

. sh.defs

# please, get this file at
#

f0pfx="./"
f1pfx="./"

f0="ISPBOOOT.BIN.orig"
f1="ISPBOOOT.BIN"

if [ ! -f "${f0pfx}${f0}" ]; then
  echo "No ${f0pfx}${f0} found, downloading it"
  curl https://archives.tibbo.com/downloads/LTPS/FW/LTPPg2/${f0} > ${f0pfx}${f0}
fi;

./ispe ${f0pfx}${f0} list

./ispe ${f1pfx}${f1} crea

echo "Reassembling ${f1pfx}${f1} from ${f0pfx}${f0} ..."
output=$(./ispe ${f0pfx}${f0} list)
echo "${output}" | grep "filename" | while read x0 pname; do
  echo "Extracting ${pname} ..."
  ./ispe ${f0pfx}${f0} part "${pname}" extp;
  ./ispe ${f1pfx}${f1} part "${pname}" addp;
  ./ispe ${f1pfx}${f1} part "${pname}" file ./isp.p.${pname}
  rm -rf ./isp.p.${pname}
done;

echo "Extracting pre-header area (xboot0,64K) from ${f0pfx}${f0} ..."
./ispe ${f0pfx}${f0} extb 0x00 $((64*1024))
./ispe ${f1pfx}${f1} setb 0x00 ./isp.b.0.$((64*1024))


echo "Extracting pre-header area (uboot0,960K) from ${f0pfx}${f0} ..."
./ispe ${f0pfx}${f0} extb 0x10000 $((960*1024))
./ispe ${f1pfx}${f1} setb 0x10000 ./isp.b.10000.$((960*1024))

echo "Extracting script from ${f0pfx}${f0} ..."
./ispe ${f0pfx}${f0} head exts
./ispe ${f1pfx}${f1} head sets ./isp.h.script.raw

echo "Adding new partition to ${f1pfx}${f1} ..."
./ispe ${f1pfx}${f1} part "rootfsB" addp
./ispe ${f1pfx}${f1} part "rootfsB" file ./img-tst-tini-tppg2.ext4

#echo "Set HDR flag = 1"
#./ispe ${f1pfx}${f1} head flag 0x01

echo "${output}" | grep "filename" | while read x0 pname; do
  pinfo=$(dv_part "${output}" "${pname}")
  p_emmc0=$(dv_part_info "${pinfo}" "emmc part start block: ")
  echo "Setting ${p_emmc0} EMMC offset for ${pname}"
  ./ispe ${f1pfx}${f1} part "${pname}" emmc ${p_emmc0}
done;

echo "Fixing xboot to 0x0 EMMC off..."
./ispe ${f1pfx}${f1} part "xboot1" emmc 0x0

echo "Fixing rootfsB to 0x32222 EMMC off..."
#./ispe ${f1pfx}${f1} part "rootfsB" emmc 0x32222
./ispe ${f1pfx}${f1} part "rootfsB" emmc $(printf '0x%x' $(((4*1024*1024*1024-500*1024*1024)/512)))

echo "Generating the ISP script..."
./gen_isp_emmc.sh ${f1pfx}${f1} ./emmc.txt
./script_enc.sh "DV ISP Script" ./emmc.txt ./emmc.raw

echo "Installing the ISP script..."
./ispe ${f1pfx}${f1} -vv setb eof ./emmc.raw

output=$(./ispe ${f1pfx}${f1} list)
LP=$(echo "${output}" | grep "Last part " | sed -e 's/Last part EOF: 0x//')
TL=$(echo "${output}" | grep "Tail data " | sed -e 's/Tail data len: //')
echo "LP:${LP}"
echo "TL:${TL}"

echo "Installing the HDR script..."
cat ./head.script.txt.T | sed -e "s/{T_OFF}/0x${LP}/" -e "s/{T_SIZE}/0x${TLh}/" > ./head.script.txt
../script_enc.sh "Init ISP Script dv" ./head.script.txt ./head.script.raw
./ispe ${f1pfx}${f1} head sets ./head.script.raw

ret=0
exit $res
