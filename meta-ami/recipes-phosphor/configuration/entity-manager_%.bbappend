FILESEXTRAPATHS_append := ":${THISDIR}/${PN}"
SRC_URI_append = " file://WC-Baseboard.json \
                   file://WP-Baseboard.json \
                   file://TNP-baseboard.json \
                   file://FCXXPDBASSMBL_PDB.json \
                   file://OPB2RH-Chassis.json \
                   file://CYP-baseboard.json \
                   file://FBTP-Zone.json \
                   file://MIDPLANE-2U2X12SWITCH.json"

SRC_URI_append += "file://SensorBoard.json"
SRC_URI_append += "file://SensorSBFan.json"

#RDEPENDS_${PN} += "default-fru"

SRC_URI_append += "file://0001-updated-configuration-sensors-units.patch"
SRC_URI_append += "file://0002-Unuse-hw-disable-json.patch"
SRC_URI_append += "file://0003-Frontpanel-riser-tempsenorjson.patch"
SRC_URI_append += "file://0004-Psu-Threshold.patch"
SRC_URI_append += "file://0005-digital-sensor.patch"
SRC_URI_append += "file://0006-Inventory-disable.patch"
SRC_URI_append += "file://0007-Updated-Flextronic-configuration.patch"
SRC_URI_append += "file://0008-Added-SdrInfo.patch"
SRC_URI_append += "file://0009-SEL-sensor.patch"
SRC_URI_append += "file://0010-Intrusion-Sensor-json.patch"
SRC_URI_append += "file://0011-SELandACPI-sensor.patch"
SRC_URI_append += "file://0012-memoryand-hdd-sensor.patch"
SRC_URI_append += "file://0015-Updated-Tiogapass-Configuration.patch"
SRC_URI_append += "file://0016-FBTP-cpusensor-sdrinfo.patch"
#Intel patch
SRC_URI_append += "file://0013-Add-retries-to-mapper-calls.patch"
SRC_URI_append += "file://0014-Improve-initialization-of-I2C-sensors.patch"


do_install_append(){
     install -d ${D}/usr/share/entity-manager/configurations
     install -m 0444 ${WORKDIR}/*.json ${D}/usr/share/entity-manager/configurations
}
