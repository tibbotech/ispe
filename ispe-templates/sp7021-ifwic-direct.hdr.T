if test "$isp_if" = usb ; then
    echo ISP file from USB storage
elif test "$isp_if" = mmc ; then
    echo ISP file from SD Card
else
    echo set to USB device 0 and use memory area start from 0x1000000
    setenv isp_if usb
    setenv isp_dev 0
    $isp_if start
fi

echo "Interface: $isp_if $isp_dev"
echo "Interface: $isp_if"

fatinfo $isp_if $isp_dev
fatls   $isp_if $isp_dev /

echo Starting Linux ...
load ${isp_if} ${isp_dev}:1 ${addr_dst_dtb} /dtb
load ${isp_if} ${isp_dev}:1 ${addr_dst_kernel} /uImage

if test "$isp_if" = usb; then
  setenv curr_root 'root=/dev/sda2 rootwait rw'
elif test "$isp_if" = mmc ; then
  setenv curr_root 'root=/dev/mmcblk1p2 rw rootwait'
fi;

setenv bootargs ${b_c} ${curr_root} ${args_kern}

bootm ${addr_dst_kernel} - ${addr_dst_dtb}
