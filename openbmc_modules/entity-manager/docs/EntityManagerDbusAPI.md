# Entity Manager DBus API

The Entity Manager is in charge of inventory management by scanning JSON files
and representing the data on DBus in a meaningful way. For more information on
the internal structure of the Entity Manager, please refer to the README of the
Entity Manager. This file documents a consumers usage of the entity manager, and
is not a description of the internal workings.

## DBus Object

### Object Paths:

####Entities:  /xyz/openbmc_project/Inventory/Item/{Entity Type}/{Entity Name}

Entities are top level json objects that describe a piece of hardware. They are
groups of configurations with few properties of their own, they are a container
type for most pratical purposes.

####Devices : /xyz/openbmc_project/Inventory/Item/{Entity Type}/{Entity Name}/{Configuration}

Configurations are components that are exposed when an entity is added to the
"global" system configuration. An example would be a TMP75 sensor that is
exposed when the front panel is detected.

__Example__:

```
/xyz/openbmc_project/Inventory/Item/Board/Intel_Front_Panel/Front_Panel_Temp
```

###Interfaces :

####xyz.openbmc_project.{InventoryType}

[see upstream types](https://github.com/openbmc/phosphor-dbus-interfaces/
tree/master/xyz/openbmc_project/Inventory/Item)

* BMC
* Board
* Chassis
* CPU
* ...

These types closely align with Redfish types.

Entity objects describe pieces of physical hardware.

#####Properties:

unsigned int: num_children: Number of configurations under this entity.

std::string name: name of the inventory item


####xyz.openbmc_project.Configuration

Configuration objects describe components owned by the Entity.

#####Properties:

Properties will contain all non-objects (simple types) exposed by the JSON.

 __Example__:

```
path: /xyz/openbmc_project/Inventory/Item/Board/Intel_Front_Panel/Front_Panel_Temp
Interface: xyz.openbmc_project.Configuration
    string name = "front panel temp"
    string address = "0x4D"
    string "bus" = "1"
```

####xyz.openbmc_project.Device.{Object}.{index}

{Object}s are members of the parent device that were originally described as
dictionaries. This allows for creating more complex types, while still being
able to get a picture of the entire system when doing a get managed objects
method call. Array objects will be indexed zero based.

#####Properties:

All members of the dictonary.

__Example__:

```
path: /xyz/openbmc_project/Inventory/Item/Board/Intel_Front_Panel/Front_Panel_Temp
Interface: xyz.openbmc_project.Device.threshold.0
    string direction = "greater than"
    int value = 55
```

##JSON Requirements

###JSON syntax requirements:

Based on the above DBus object, there is an implicit requirement that device
objects may not have more than one level deep of dictionary or list of
dictionary. It is possible to extend in the future to allow nearly infinite
levels deep of dictonary with extending the
__xyz.openbmc_project.Device.{Object}__ to allow
__xyz.openbmc_project.Device.{Object}.{SubObject}.{SubObject}__ but that
complexity will be avoided until proven needed.

__Example__:

Legal:

```
exposes :
[
    {
        "name" : "front panel temp",
        "property": {"key: "value"}
    }
]
```

Not Legal:

```
exposes :
[
    {
        "name" : "front panel temp",
        "property": {
            "key: {
                "key2": "value"
            }
    }
]

```
