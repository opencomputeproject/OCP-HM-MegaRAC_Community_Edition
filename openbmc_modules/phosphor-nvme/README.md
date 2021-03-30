### phosphor-nvme

#### Introduction

phosphor-nvme is the nvme manager service maintains for NVMe drive information
update and related notification processing service. The service update
information to `xyz/openbmc_project/Nvme/Status.interface.yaml`,
`xyz/openbmc_project/Sensor/Value.interface.yaml` and
other interfaces in `xyz.openbmc_project.Inventory.Manager`.

#### General usage

The service `xyz.openbmc_project.nvme.manager` provides object on D-Bus:

* /xyz/openbmc_project/sensors/temperature/nvme(index)

where object implements interface `xyz.openbmc_project.Sensor.Value`.

NVMe drive export as sensor and sensor value is temperature of drive.
It can get the sensor value of the drive through ipmitool command `sdr elist`
if the corresponding settings in the sensor map are configured correctly.
For example:

* To get sensor value:

   ```
   ### With ipmi command on BMC
   ipmitool sdr elist
   ```

The service also updates other NVMe drive information to D-bus
`xyz.openbmc_project.Inventory.Manager`. The service
`xyz.openbmc_project.Inventory.Manager` provides object on D-Bus:

* /xyz/openbmc_project/inventory/system/chassis/motherboard/nvme(index)

where object implements interfaces:

* xyz.openbmc_project.Inventory.Item
* xyz.openbmc_project.Inventory.Decorator.Asset
* xyz.openbmc_project.Nvme.Status

Interface `xyz.openbmc_project.Nvme.Status` with the following properties:

| Property | Type | Description |
| -------- | ---- | ----------- |
| SmartWarnings| string | Indicates smart warnings for the state |
| StatusFlags | string | Indicates the status of the drives |
| DriveLifeUsed | string | A vendor specific estimate of the percentage |
| TemperatureFault| bool | If warning type about temperature happened |
| BackupdrivesFault | bool | If warning type about backup drives happened |
| CapacityFault| bool | If warning type about capacity happened |
| DegradesFault| bool | If warning type about degrades happened |
| MediaFault| bool | If warning type about media happened |

Interface `xyz.openbmc_project.Inventory.Item` with the following properties:

| Property | Type | Description |
| -------- | ---- | ----------- |
| Present | bool | Whether or not the item is present |

Interface `xyz.openbmc_project.Inventory.Decorator.Asset` with the following
properties:

| Property | Type | Description |
| -------- | ---- | ----------- |
| SerialNumber | string | The item serial number |
| Manufacturer | string | The item manufacturer |

Each property in the inventory manager can be obtained via the busctl
get-property command. For example:

* To get property Present:

   ```
   ### With busctl on BMC
   busctl get-property xyz.openbmc_project.Inventory.Manager /xyz/openbmc_project/inventory/system/chassis/motherboard/nvme0 xyz.openbmc_project.Inventory.Item Present
   ```

#### Configuration file

There is a JSON configuration file `nvme_config.json` for drive index, bus ID,
and the LED object path and bus name for each drive.
For example,

```json
{
    "config": [
        {
            "NVMeDriveIndex": 0,
            "NVMeDriveBusID": 16,
            "NVMeDriveFaultLEDGroupPath": "/xyz/openbmc_project/led/groups/led_u2_0_fault",
            "NVMeDriveLocateLEDGroupPath": "/xyz/openbmc_project/led/groups/led_u2_0_locate",
            "NVMeDriveLocateLEDControllerBusName":"xyz.openbmc_project.LED.Controller.led_u2_0_locate",
            "NVMeDriveLocateLEDControllerPath":"/xyz/openbmc_project/led/physical/led_u2_0_locate",
            "NVMeDrivePresentPin": 148,
            "NVMeDrivePwrGoodPin": 161
        },
        {
            "NVMeDriveIndex": 1,
            "NVMeDriveBusID": 17,
            "NVMeDriveFaultLEDGroupPath": "/xyz/openbmc_project/led/groups/led_u2_1_fault",
            "NVMeDriveLocateLEDGroupPath": "/xyz/openbmc_project/led/groups/led_u2_1_locate",
            "NVMeDriveLocateLEDControllerBusName":"xyz.openbmc_project.LED.Controller.led_u2_1_locate",
            "NVMeDriveLocateLEDControllerPath":"/xyz/openbmc_project/led/physical/led_u2_1_locate",
            "NVMeDrivePresentPin": 149,
            "NVMeDrivePwrGoodPin": 162
        }
    ],
    "threshold":[
        {
            "criticalHigh":70,
            "criticalLow":0,
            "maxValue":70,
            "minValue":0
        }
    ]
}
```

* config
  * NvmeDriveIndex: The index of the NVMe drive, which will be displayed in the
                    object path.
  * NVMeDriveBusID: The bus id of the NVMe drive, since it communicates with SMBus.
  * NVMeDriveFaultLEDGroupPath: Object path of fault LED  in LED Group Manager.
  * NVMeDriveLocateLEDGroupPath: Object path of locate LED  in LED Group Manager.
  * NVMeDriveLocateLEDControllerBusName: D-Bus name of locate LED in LED Controller.
  * NVMeDriveLocateLEDControllerPath: Object path of locate LED in LED Controller.
  * NVMeDrivePresentPin: Gpio present pin of NVMe drive.
  * NVMeDrivePwrGoodPin: Gpio Power good pin of NVMe drive.
* threshold
  * criticalHigh: Upper critical threshold.
  * criticalLow: Lower critical threshold.
  * maxValue: Sensor maximum value.
  * minValue: Sensor value.

#### Process

1. It will register a D-bus called `xyz.openbmc_project.nvme.manager`
   description above.
2. Obtain the drive index, bus ID, GPIO present pin, power good pin and fault
   LED object path from the json file mentioned above.
3. Each cycle will do following steps:
   1. Check if the present pin of target drive is true, if true, means drive
      exists and go to next step. If not, means drive does not exists and
      remove object path from D-bus by drive index.
   2. Check if the power good pin of target drive is true, if true means drive
      is ready then create object path by drive index and go to next step. If
      not, means drive power abnormal, turn on fault LED and log in journal.
   3. Send a NVMe-MI command via SMBus Block Read protocol by bus ID of target
      drive to get data. Data get from NVMe drives are "Status Flags",
      "SMART Warnings", "Temperature", "Percentage Drive Life Used",
      "Vendor ID", and "Serial Number".
   4. The data will be set to the properties in D-bus.

This service will run automatically and look up NVMe drives every second.
