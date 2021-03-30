# phosphor-ecc

### Overview

This function is to provide the BMC with the ability to record ECC error SELs.
The EDAC driver of BMC can detects and corrects memory errors, which helps
identify problems before they become catastrophic faulty memory module.

### Requirements

* The EDAC driver must be supported and enabled
* The `phosphor-sel-logger` package must be installed

### Monitor Daemon

Run the application and look up the ECC error count every second after service
is started. On first start, it resets all correctable ECC counts and
uncorrectable ECC counts in the EDAC driver.

it also provide the following path on D-Bus:

* bus name    : `xyz.openbmc_project.Memory.ECC`
* object path : `/xyz/openbmc_project/metrics/memory/BmcECC`
* interface   : `xyz.openbmc_project.Memory.MemoryECC`

The interface with the following properties:

| Property | Type | Description |
| -------- | ---- | ----------- |
| isLoggingLimitReached | bool | ECC logging reach limits|
| ceCount| int64 | correctable ECC events |
| ueCount| int64 | uncorrectable ECC events |
| state| string | bmc ECC event state |

It also devised a mechanism to limit the "maximum number" of logs to avoid
creating a large number of correctable ECC logs. When the `maximum quantity` is
reached, the ECC service will stop to record the ECC log. The `maximum quantity`
(default:100) is saved in the configuration file, and the user can modify the
value if necessary.

### Create the ECC SEL

Use the `phosphor-sel-logger` package to record the following logs in BMC SEL
format.

* correctable ECC log : when fetching the `ce_count` from EDAC driver parameter
  and the count exceeds previous count.
* uncorrectable ECC log : when fetching the `ue_count` from EDAC driver
  parameter and the count exceeds previous count.
* logging limit reached log : When the correctable ECC log reaches the
  `maximum quantity`.