#! /bin/sh

DIRECTORY="/sys/kernel/config/usb_gadget/eth"

if [ -d "$DIRECTORY" ]; then
  exit 0
fi

mkdir /sys/kernel/config/usb_gadget/eth
echo 0x046b >/sys/kernel/config/usb_gadget/eth/idVendor
echo 0xFFb0 >/sys/kernel/config/usb_gadget/eth/idProduct
echo 0x02 > /sys/kernel/config/usb_gadget/eth/bDeviceClass
echo 0x00 > /sys/kernel/config/usb_gadget/eth/bDeviceSubClass
echo 0x00 > /sys/kernel/config/usb_gadget/eth/bDeviceProtocol
echo 0x0100 > /sys/kernel/config/usb_gadget/eth/bcdDevice

mkdir /sys/kernel/config/usb_gadget/eth/strings/0x409
echo 1234567890 > /sys/kernel/config/usb_gadget/eth/strings/0x409/serialnumber
echo Intel Corporation > /sys/kernel/config/usb_gadget/eth/strings/0x409/manufacturer
echo Virtual Ethernet. > /sys/kernel/config/usb_gadget/eth/strings/0x409/product

#NEXT 4 LINES
mkdir /sys/kernel/config/usb_gadget/eth/configs/c.1
echo  0 > /sys/kernel/config/usb_gadget/eth/configs/c.1/MaxPower
echo  0xC0 > /sys/kernel/config/usb_gadget/eth/configs/c.1/bmAttributes
mkdir /sys/kernel/config/usb_gadget/eth/configs/c.1/strings/0x409

mkdir /sys/kernel/config/usb_gadget/eth/functions/ecm.usb0
mkdir /sys/kernel/config/usb_gadget/eth/configs/c.2
echo  0 > /sys/kernel/config/usb_gadget/eth/configs/c.2/MaxPower
echo  0xC0 > /sys/kernel/config/usb_gadget/eth/configs/c.2/bmAttributes
mkdir /sys/kernel/config/usb_gadget/eth/configs/c.2/strings/0x409

#NEXT 3 LINES
mkdir /sys/kernel/config/usb_gadget/eth/functions/rndis.usb0
echo RNDIS > /sys/kernel/config/usb_gadget/eth/functions/rndis.usb0/os_desc/interface.rndis/compatible_id
echo 5162001 > /sys/kernel/config/usb_gadget/eth/functions/rndis.usb0/os_desc/interface.rndis/sub_compatible_id
cd /sys/kernel/config/usb_gadget/eth && ln -s functions/ecm.usb0 configs/c.2/

#NEXT 2 LINES
cd /sys/kernel/config/usb_gadget/eth && ln -s functions/rndis.usb0 configs/c.1/
cd /sys/kernel/config/usb_gadget/eth && ln -s configs/c.1 os_desc/c.1
echo 1e6a0000.usb-vhub:p3 > /sys/kernel/config/usb_gadget/eth/UDC

## Assigning MAC address
USB0_MAC=`dmesg | grep "usb0: MAC" | cut -d ' ' -f 7`
ifconfig usb0 down
ifconfig usb0 hw ether $USB0_MAC
ifconfig usb0 169.254.0.17 netmask 255.255.0.0
ifconfig usb0 up
