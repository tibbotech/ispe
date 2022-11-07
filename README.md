
**mkimage** tool (u-boot-tools) is required for some operations

### Building
```
make
```

### Show ISPBOOOT.BIN data
```
./ispe ./ISPBOOOT.BIN list
```

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

### Extract <len> (decimal) data from the offset <hoff> (hex)
```
./ispe ./ISPBOOOT.bin -vvvv extb 0x100 16
```
Extracts 16 bytes starting at 0x100 offset into 'isp.b.100.16' file

### Remove the partition
```
./ispe ./ISPBOOOT.bin -vvvv delp rootfs
```

### Extract all partitions:
```
./ispe ./ISPBOOOT.BIN list | grep "filename" | \
while read pname x1; do \
  ./ispe ./ISPBOOOT.BIN extp ${pname}; \
done;
```
