# GPIO JSON format

GPIO definitions are stored in '/etc/default/obmc/gpio/gpio_defs.json' on the
BMC.  That file has 2 sections - 'gpio_configs' and 'gpio_definitions'.

## gpio_configs

This section contains the GPIOs used in power control.

It looks like:
```
"gpio_configs": {
    "power_config": {

        #See code in op-pwrctl for details

        #Required
        "power_good_in": "...",

        #Required
        "power_up_outs": [
            {"name": "...", "polarity": true/false},
            {"name": "...", "polarity": true/false}
        ],

        #Optional
        "reset_outs": [
            {"name": "...", "polarity": true/false}
        ],

        #Optional
        "latch_out": "...",

        #Optional
        "pci_reset_outs": [
            {"name": "...", "polarity": true/false, "hold": true/false}
        ]
    }
}
```

## gpio_definitions

This section contains The GPIO pins and directions.

It looks like:
```
    "gpio_definitions": [
        {

            #The name to look up this entry.
            "name": "SOFTWARE_PGOOD",

            #The GPIO pin.
            "pin": "R1",

            #Alternatively to the pin, can use 'num' which is the
            #raw number the GPIO would be accessed with.
            "num": 7,

            #The GPIO direction - in, out, rising, falling, or both
            "direction": "out"
        },
        {
            ...
        }
    ]
```

