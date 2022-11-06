#!/bin/sh

if [ $# -le 1 ]; then
  echo "Usage: ${0} fromtxtfile tobinfile";
  exit 1;
fi;

mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "Init ISP script" -d ${1} ${2}
