
# ISPEditor

ISPBOOOT.BIN file editor good for scripting and automation.

```
Usage: ./ispe <img> [-v] <cmd> [cmdparams]
        [-v] verbose mode (-vvv.. increase verbosity)
        <cmd> [params] one of the following:
        list - list partitions in the image
        crea - create an empty image
        extb <0xXX> <dlen> - extract <dlen> (dec) bytes at XX offset
        setb <0xXX> <name> - save raw binary file <name> at <0xXX> offset
        head exts - extract header script
        head flag <0xXX> - set image header flag
        head sets <file> - update header script from script image file
        part <name> dele - delete the partition from the image
        part <name> addp - create new partition
        part <name> extp - extract partition to ...
        part <name> wipe - wipe the partition keeping general info
        part <name> file <file> - load data for the partition from the raw file
        part <name> flag <0xXX> - set flag = 0xXX to partition <name>
        part <name> size <0xXX> - set size = 0xXX to partition <name>
        part <name> nand <0xXX> - set NAND offset = 0xXX to partition <name>
        part <name> emmc <0xXX> - set EMMC start block off = 0xXX

```
EXIT_CODE == 0 on success:
```
./ispe ./myimage ...
if [ $? -ne 0 ]; then  echo "Failed";
else  echo "OP is done";  fi;
```

Please, see test0.sh for more examples.

## Building
```
make
```
### Build Requirements:

- openssl-dev package (libssl, libcrypto + headers)

### Runtime Requirements:

- mkimage (u-boot-tools package)

## Examples

#### Generate EMMC ISP script for the existing image
```
./gen_isp_emmc.sh ./ISPBOOOT.BIN ./emmc.txt
```

#### Show ISPBOOOT.BIN data
```
./ispe ./ISPBOOOT.BIN list
```
#### Create new empty image
```
./ispe ./ISPBOOOT.BIN -vvv crea
```
#### Set image header flag
```
./ispe ./ISPBOOOT.BIN head flag 0x01
```
0x01 means MTD_ONLY

#### Add new empty partition
```
./ispe ./ISPBOOOT.BIN part "part0" addp
```

#### Load data to the partition
```
./ispe ./ISPBOOOT.BIN part "dtb" file ./isp.p.dtb
```

#### Extract script
```
./ispe ./ISPBOOOT.BIN head exts
```
sript will be dumped into 
isp.h.script.raw - raw script buffer dump (need to cut last \0 to operate)
isp.h.script.txt - attempt to extract clear text

#### Encode plain text in 'isp.h.script.txt' file to image script
```
./script_enc.sh ./isp.h.script.txt ./isp.h.script.raw
```

#### Attempt to get script RAW image information from 'isp.h.script.raw':
```
sed 's/\x00*$//' ./isp.h.script.raw > ./isp.h.script.raw.tmp
mkimage -l ./isp.h.script.raw.tmp
```

#### Update the header script from script text file:
```
./script_enc.sh "Init ISP Script" ./myscript.txt ./myscript.raw
./ispe ./ISPBOOOT.BIN head sets ./mysript.raw
```

#### Extract binary raw data
```
./ispe ./ISPBOOOT.bin extb 0x100 16
```
Extracts 16 bytes starting at 0x100 offset into 'isp.b.100.16' file

#### Set binary raw data
```
./ispe ./ISPBOOOT.bin setb 0x100 ./isp.b.100.16
```
Saves the raw file contents from the file 'isp.b.100.16' to the image starting at 0x100 offset.
IMG Header is protected.
