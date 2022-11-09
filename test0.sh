#!/bin/bash

# please, get this file at
#

f0pfx="./"

f0="ISPBOOOT.BIN"

if [ ! -f "${f0pfx}${f0}" ]; then
  echo "No ${f0pfx}${f0} found, downloading it"
  curl https://archives.tibbo.com/downloads/LTPS/FW/LTPPg2/${f0} > ${f0pfx}${f0}
fi;

./ispe ${f0pfx}${f0} list

echo "Extracting partitions from ${f0pfx}${f0} ..."
./ispe ${f0pfx}${f0} list | grep "filename" | \
while read x0 pname; do \
  echo "Extracting ${pname} ..."
  ./ispe ${f0pfx}${f0} part ${pname} extp; \
done;

echo "Extracting pre-header area (xboot0,64K) from ${f0pfx}${f0} ..."
./ispe ${f0pfx}${f0} extb 0x00 $((64*1024))

echo "Extracting pre-header area (uboot0,960K) from ${f0pfx}${f0} ..."
./ispe ${f0pfx}${f0} extb 0x10000 $((960*1024))

echo "Extracting script from ${f0pfx}${f0} ..."
./ispe ${f0pfx}${f0} head exts

f1pfx="./"
f1="./ISPBOOOT.bin"

echo "Creating new ${f1pfx}${f1} image ..."
./ispe ${f1pfx}${f1} crea

echo "${f1pfx}${f1} : Adding 'part0' partition ..."
./ispe ${f1pfx}${f1} part "part0" addp
echo "${f1pfx}${f1} : Adding 'part1' partition ..."
./ispe ${f1pfx}${f1} part "part1" addp

ff="./isp.p.kernel"
echo "${f1pfx}${f1} : Uploading ${ff} to 'part1' ..."
./ispe ${f1pfx}${f1} part "part1" file ${ff}
ff="./isp.p.dtb"
echo "${f1pfx}${f1} : Uploading ${ff} to 'part0' ..."
./ispe ${f1pfx}${f1} part "part0" file ${ff}

echo "${f1pfx}${f1} : Extracting 'part0' ..."
./ispe ${f1pfx}${f1} part "part0" extp
echo "${f1pfx}${f1} : Extracting 'part1' ..."
./ispe ${f1pfx}${f1} part "part1" extp

#./ispe ./ISPBOOOT.bin -v list

res=0

echo "Comparing part0 dump to the source ..."
diff ./isp.p.part0 ./isp.p.dtb
if [ $? -ne 0 ]; then  res=$(($res+1));  echo "part0 ERROR";  fi;
echo "Comparing part1 dump to the source ..."
diff ./isp.p.part1 ./isp.p.kernel
if [ $? -ne 0 ]; then  res=$(($res+1));  echo "part1 ERROR";  fi;

if [ $res -eq 0 ]; then
  echo "Congratulations! No errors found"
fi;

exit $res
