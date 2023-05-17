if test "$isp_if" = usb ; then
    echo ISP file from USB storage
elif test "$isp_if" = mmc ; then
    echo ISP file from SD Card
else
    echo set to USB device 0 and use memory area start from 0x1000000
    setenv isp_if usb
    setenv isp_dev 0
    setenv isp_ram_addr 0x1000000
    $isp_if start
fi

echo "Interface: $isp_if $isp_dev"
echo "Interface: $isp_if"

fatinfo $isp_if $isp_dev
fatls   $isp_if $isp_dev /

setenv isp_size_total {T_OFF}
if test "$isp_force_to_abort" = yes ; then
    echo isp_force_to_abort
    exit -1
fi

echo Load ISP main script and run it ...
if test "$isp_main_storage" = nand ; then
  fatload $isp_if $isp_dev $isp_ram_addr /ISPBOOOT.BIN {T_SIZE} {T_OFF}
elif test "$isp_main_storage" = emmc ; then
  fatload $isp_if $isp_dev $isp_ram_addr /ISPBOOOT.BIN {T_SIZE} {T_OFF}
else
  setenv isp_main_storage nand
  fatload $isp_if $isp_dev $isp_ram_addr /ISPBOOOT.BIN {T_SIZE} {T_OFF}
fi
source $isp_ram_addr
