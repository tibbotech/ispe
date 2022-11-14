#!/bin/sh

if [ $# -le 2 ]; then
  echo "Usage: ${0} 'label' fromtxtfile tobinfile";
  exit 1;
fi;

mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "${1}" -d ${2} ${3}
