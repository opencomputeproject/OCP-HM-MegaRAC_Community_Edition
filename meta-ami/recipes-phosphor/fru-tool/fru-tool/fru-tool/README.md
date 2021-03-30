fru-tool
======================
*fru-tool* is a command-line utility for generating IPMI FRU binary data files.

## Description
Every modern component of a computer or electronic equipment, commonly referred to as a Field Replaceable Unit or FRU, contains a memory block that stores the inventory information of that component. This includes the manufacturer's name, product name, manufacture date, serial numbers and other details that help identify the component.

The Intel FRU Information Storage for IPMI [specification](https://www.intel.com/content/dam/www/public/us/en/documents/product-briefs/platform-management-fru-document-rev-1-2-feb-2013.pdf) defines the standard format that devices are expected to conform to within their FRU areas. Dell EMC PowerEdge servers leverage this format across the board, from PCIe controllers to power supplies to the chassis itself. Each component vendor populates the FRU area during their manufacturing process and all FRU areas are easily accessible via IPMI.

The OEM FRU storage feature of Dell EMC PowerEdge servers is an additional FRU area that allows OEM customers, who use Dell EMC servers as a component of their solution, to include their own tracking information in the FRU storage area. This can be loaded into the server during factory deployment and can be accessed when the information is required during troubleshooting or support. This allows the OEM customers to store their own part numbers and inventory information within the server, enabling them to track their solutions in their internal management systems. This is similar to the way Dell EMC servers use the standard FRU areas to store tracking information such as service tags and manufacture date and use that information when having to identify and support those systems once in the field.

Considering that the FRU area is a binary payload, it is not trivial to build the content structure by hand. In order to simplify the effort for OEM customers, this Python tool is provided to speed up the process of creating the payload.

While this *fru-tool* was specifically authored to support this OEM use case, it conforms to Intel's specification and can be used to build the FRU structure for any device.

## Prerequisites
*fru-tool* has been tested with Python version 2.7 and 3.5 which can be downloaded and installed from www.python.org.

In order to write, read or edit the OEM FRU storage area on the target server, the open source [IPMItool](https://sourceforge.net/projects/ipmitool/) utility or equivalent is required. This utility can be installed on Linux distributions by using the built-in package manager such as yum or apt-get. Dell EMC provides a Windows version which can be found in the *Driver and Downloads* section for any PowerEdge server on [Dell EMC Support](http://support.dell.com) under the *Systems Management* section. It is contained in the package named *Dell OpenManage BMC Utility* which can also be found on Google by searching for the package by name. For documentation on IPMItool, search for 'man ipmitool' on Google.

## Installation
Installation is as simple as cloning or downloading this project from GitHub. The main files of interest are the [fru.py](fru.py) Python script and [fru.ini](fru.ini) which defines the contents of the FRU binary.

## Usage Instructions
Once the fru.ini file has been filled out as required for the solution, *fru-tool* can be used to convert it into a BIN format which can then be deployed onto a server for test purposes. The syntax is:-

```
python fru.py INIFILE BINFILE [--force] [--cmd]

INIFILE = input INI file as described above 
BINFILE = output BIN file that can be programmed into server 
--force = overwrite BIN file if it already exists 
  --cmd = print sample IPMItool commands as a reference
```

For example, let’s create an INI file with ```common.product = 1``` and ```product.manufacturer = Widgets Inc.``` and generate the BIN file with the ```--cmd``` flag to show IPMItool command examples. It is important to enable the product area in the ```[common]``` section before the tool includes it in the BIN file. If this step is omitted, the tool assumes that the section is to be omitted.

```
> python fru.py inifile.ini binfile.bin --cmd

  [Product]
  ; ipmitool -I lanplus -H %IP% -U root -P password fru edit 17 field p 0 123456789012
  0: manufacturer = b'Widgets Inc.' (12)
```

The tool generates the BIN file and displays what fields were added to it, along with the field length. It also displays the IPMItool syntax (due to ```--cmd```) that can be used to edit each field once the BIN file is programmed on a server. This is useful since the typical use case is to create a template BIN which can be created once using the FRU tool and then programmed on each server, followed by editing all server unique fields as a second step.

Detailed usage information and use cases for the OEM FRU feature can be found in the [OEM FRU Whitepaper](http://en.community.dell.com/techcenter/extras/m/white_papers/20444358).

## Contribution
In order to contribute, feel free to fork the project and submit a pull request with all your changes and a description on what was added or removed and why. If approved, the project owners will merge it.

## Licensing
*fru-tool* is freely distributed under the [MIT License](LICENSE.txt).

## Support
Please file bugs and issues on the Github issues page for this project. For general discussions and further support you can join the [{code} Community slack channel](http://community.codedellemc.com/). The code and documentation are released with no warranties or SLAs and are intended to be supported through a community driven process.