# ISPEditor automation examples

## sp7021.emmc.sh

Assembles ISPBOOOT.BIN installable to EMMC from binaries.

Usage:
```
ISPEDIR=../ ./sp7021.emmc.sh
```
Binaries required:

../tppg2-arm5/xboot-emmc.img
./u-boot.bin-a7021_ppg2.img
./u-boot.bin-a7021_ppg2.img
../tppg2-arm5/a926-empty.img
./sp7021-ltpp3g2revD.dtb
./uImage-initramfs-tppg2.bin
./img-tps-free-tppg2.ext4

## mender.repack.sh

Downloads traditional ISPBOOOT.BIN, disassembles it to parts, creates the new 
one from the parts, adds one more partition#9 'rootfsB' with contents from
./img-tst-tini-tppg2.ext4
file and complete image with new installation scripts.

```
ISPEDIR=../ ./mender.repack.sh
```

Binaries required:

./img-tst-tini-tppg2.ext4
