# Device ID Configuration

There is a default dev_id.json file provided by
meta-phosphor/common/recipes-phosphor/ipmi/phosphor-ipmi-host.bb

Any target can override the default json file by providing a
phosphor-ipmi-host.bbappend with an ODM or platform customizable configuration.

For a specific example, see:
[Witherspoon](https://github.com/openbmc/openbmc/blob/master/
meta-openbmc-machines/meta-openpower/meta-ibm/meta-witherspoon/
recipes-phosphor/ipmi/phosphor-ipmi-host.bbappend)

The JSON format for get_device_id:

    {"id": 0, "revision": 0, "addn_dev_support": 0,
        "manuf_id": 0, "prod_id": 0, "aux": 0}


Each value in this JSON object should be an integer. The file is placed in
/usr/share/ipmi-providers/ by Yocto, and will be parsed upon the first call to
get_device_id. The data is then cached for future use. If you change the data
at runtime, simply restart the service to see the new data fetched by a call to
get_device_id.
