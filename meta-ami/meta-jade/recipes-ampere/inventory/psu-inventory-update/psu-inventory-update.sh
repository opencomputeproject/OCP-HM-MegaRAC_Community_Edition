#!/bin/bash

# Description: This script will update the Dbus object base on
#              The PSU FRU content
#
# This script support the Ampere platform with FRU PSU support on
# I2C6 (PSU0 - 0x50; PSU1 - 0x51)
#
# Syntax: psu-inventory-update.sh [psu-num]
# where: psu-num is the psu number (0 or 1)
#
# Author: Chanh Nguyen <chnguyen@amperecomputing.com>

i2c_chan=6
INVENTORY_SERVICE='xyz.openbmc_project.Inventory.Manager'
INVENTORY_OBJECT='/xyz/openbmc_project/inventory'
INVENTORY_PATH='xyz.openbmc_project.Inventory.Manager'
OBJECT_PATH="/system/chassis/motherboard/powersupply$1"
DRIVER_PATH="/sys/bus/i2c/drivers/pmbus/"
manufacturer_name=''
product_name=''
product_partnumber=''
product_vernumber=''
product_serialnumber=''

pmbus_read() {

  # Syntax: pmbus_read $1 $2 $3 $4
  # Where:  $1 is I2C Channel
  #         $2 is I2C Address
  #         $3 is Data Address
  #         $4 is Data Length

  len=$4

  while [ $len -gt 0 ]
  do

    # The I2C block data reading just support the max length is 32 bytes
    if [ $len -lt 32 ]; then
      data_tmp=$(i2cget -f -y $1 $2 $3 i $len)
      data+=$(echo ${data_tmp} | sed -e "s/$len\: //")
    else
      data_tmp=$(i2cget -f -y $1 $2 $3 i 32)
      data+=$(echo ${data_tmp} | sed -e "s/32\: //")
    fi

    if [[ -z "$data" ]]; then
      echo "i2c$1 device $2 command $3 error" >&2
      exit 1
    fi

    len=$(( $len - 32 ))
    data+=' '
  done

  arry=$(echo ${data} | sed -e "s/$4\: //" | sed -e "s/\0x00//g" | sed -e "s/\0xff//g" | sed -e "s/\0x7f//g" | sed -e "s/\0x0f//g" | sed -e "s/\0x14//g")

  string=''
  for d in ${arry}
  do
    hex=$(echo $d | sed -e "s/0\x//")
    string+=$(echo -e "\x${hex}");
  done

  # Return a result string
  echo $string
}

fru_read() {

  # Syntax: fru_read $1 $2
  # Where:  $1 is I2C Channel
  #         $2 is I2C Address

  # The data format is compliant with the Intel IPMI v1.0 specification.

  declare -i offset=0
  declare -i len
  # Clear bit #7 #6 for unspecified
  declare -i mask_len=0x3f

  # PRODUCT INFO AREA OFFSET at 0x4 offset
  offset+=4
  product_area=$(i2cget -f -y $1 $2 $offset)

  # Go to the PRODUCT AREA (In multiples of 8 bytes)
  offset=$((8*$product_area))

  # Skip Header of The PRODUCT AREA
  offset+=3

  # Read MANUFACTURER'S NAME
  len=$(i2cget -f -y $1 $2 $offset)
  len=$(($len&$mask_len))
  offset+=1
  manufacturer_name=$( pmbus_read $1 $2 $offset $len)

  offset+=$(($len))

  # Read PRODUCT NAME
  len=$(i2cget -f -y $1 $2 $offset)
  len=$(($len&$mask_len))
  offset+=1
  product_name=$( pmbus_read $1 $2 $offset $len)

  offset+=$(($len))

  # Read PRODUCT PARTNUMBER
  len=$(i2cget -f -y $1 $2 $offset)
  len=$(($len&$mask_len))
  offset+=1
  product_partnumber=$( pmbus_read $1 $2 $offset $len)

  offset+=$(($len))

  # Read PRODUCT VERSION
  len=$(i2cget -f -y $1 $2 $offset)
  len=$(($len&$mask_len))
  offset+=1
  product_vernumber=$( pmbus_read $1 $2 $offset $len)

  offset+=$(($len))

  # Read PRODUCT SERIAL NUMBER
  len=$(i2cget -f -y $1 $2 $offset)
  len=$(($len&$mask_len))
  offset+=1
  product_serialnumber=$( pmbus_read $1 $2 $offset $len)

}

update_inventory() {
  if [ -z "$6" ]; then
      busctl call \
      ${INVENTORY_SERVICE} \
      ${INVENTORY_OBJECT} \
      ${INVENTORY_PATH} \
      Notify a{oa{sa{sv}}} 1 \
      ${OBJECT_PATH} 1 $2 $3 \
      $4 $5 ""
  else
      busctl call \
      ${INVENTORY_SERVICE} \
      ${INVENTORY_OBJECT} \
      ${INVENTORY_PATH} \
      Notify a{oa{sa{sv}}} 1 \
      ${OBJECT_PATH} 1 $2 $3 \
      $4 $5 $6
  fi
}


if [ $# -eq 0 ]; then
  echo 'No PSU device is given' >&2
  exit 1
fi

if [ $1 -eq 0 ]; then
  i2c_addr=0x50
  driver_name="${i2c_chan}-0058"
elif [ $1 -eq 1 ]; then
  i2c_addr=0x51
  driver_name="${i2c_chan}-0059"
else
  echo "ERROR: The PSU $1 device is not correctly"
  exit 1
fi

# Check if the powersupply object dbus exist
error=$( busctl introspect ${INVENTORY_SERVICE} ${INVENTORY_OBJECT}${OBJECT_PATH} )
if [[ $? -ne 0 ]]; then
  echo "ERROR: Not Found ${INVENTORY_OBJECT}${OBJECT_PATH}"
  exit 1
fi

# Check if the powersupply present
if [ -e ${DRIVER_PATH}${driver_name} ]; then
  fru_read $i2c_chan $i2c_addr
else
  echo "WARNING: The powersupply$1 is not present"
fi

  update_inventory $1 "xyz.openbmc_project.Inventory.Decorator.Asset" 1 "Manufacturer" "s" $manufacturer_name
  update_inventory $1 "xyz.openbmc_project.Inventory.Decorator.Asset" 1 "Model" "s" $product_name
  update_inventory $1 "xyz.openbmc_project.Inventory.Decorator.Asset" 1 "PartNumber" "s" $product_partnumber
  update_inventory $1 "xyz.openbmc_project.Inventory.Decorator.Asset" 1 "SerialNumber" "s" $product_serialnumber

