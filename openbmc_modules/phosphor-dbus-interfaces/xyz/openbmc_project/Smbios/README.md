# SMBIOS MDR V2

## Overview
SMBIOS MDR V2 service exposes D-Bus methods for SMBIOS Version 2 operations.

### SMBIOS MDR V2 Interface
SMBIOS MDR V2 interface `xyz.openbmc_project.Smbios.MDR_V2` provides following
methods.
#### methods
* GetDirectoryInformation - Get the directory with directory index.
* GetDataInformation - Get the data information with id index and data set ID.
* SendDirectoryInformation - Send directory information to SMBIOS directory.
* GetDataOffer - Get data set ID.
* SendDataInformation - Send data information with directory index.
* FindIdIndex - Find id index by data info.
* SynchronizeDirectoryCommonData - Synchronize directory common data before
SMBIOS data start to transfer.
* AgentSynchronizeData  - Synchronize SMBIOS data from file after data transfer
complete.

#### properties
* DirEntries - Numbers of directory entries. Default: 0
