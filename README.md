
**mkimage** tool (u-boot-tools) is required for some operations

### Building
```
make
```

### Show ISPBOOOT.BIN data
```
./ispe ./ISPBOOOT.BIN list
```
### Create new empty image
```
./ispe ./ISPBOOOT.BIN -vvv crea
```
### Set image header flag
```
./ispe ./ISPBOOOT.BIN -vvv flag 0x01
```
0x01 means MTD_ONLY

### Extract script
```
./ispe ./ISPBOOOT.BIN -vvv exts
```
sript will be dumped into 
isp.h.script.raw - raw script buffer dump (need to cut last \0 to operate)
isp.h.script.txt - attempt to extract clear text

### Encode plain text in 'isp.h.script.txt' file to image script
```
./script_enc.sh ./isp.h.script.txt ./isp.h.script.raw
```

### Attempt to get script RAW image information from 'isp.h.script.raw':
```
sed 's/\x00*$//' ./isp.h.script.raw > ./isp.h.script.raw.tmp
mkimage -l ./isp.h.script.raw.tmp
```

### Update the header script from script text file:
```
./script_enc.sh ./myscript.txt ./myscript.raw
./ispe ./ISPBOOOT.BIN -vvv sets ./mysript.raw
```

### Extract binary raw data
```
./ispe ./ISPBOOOT.bin -vvvv extb 0x100 16
```
Extracts 16 bytes starting at 0x100 offset into 'isp.b.100.16' file

### Set binary raw data
```
./ispe ./ISPBOOOT.bin -vvvv setb 0x100 ./isp.b.100.16
```
Saves the raw file contents from the file 'isp.b.100.16' to the image starting at 0x100 offset.
IMG Header is protected.

### Remove the partition
```
./ispe ./ISPBOOOT.bin -vvvv delp rootfs
```

### Extract all partitions:
```
./ispe ./ISPBOOOT.BIN list | grep "filename" | \
while read x0 pname; do \
  ./ispe ./ISPBOOOT.BIN extp ${pname}; \
done;
```
