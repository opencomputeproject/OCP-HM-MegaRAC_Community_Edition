## System states
##   state can change to next state in 2 ways:
##   - a process emits a GotoSystemState signal with state name to goto
##   - objects specified in EXIT_STATE_DEPEND have started
SYSTEM_STATES = [
	'BASE_APPS',
	'BMC_STARTING',
	'BMC_READY',
	'HOST_POWERING_ON',
	'HOST_POWERED_ON',
	'HOST_BOOTING',
	'HOST_BOOTED',
	'HOST_POWERED_OFF',
]

EXIT_STATE_DEPEND = {
	'BASE_APPS' : {
		'/org/openbmc/sensors': 0,
	},
	'BMC_STARTING' : {
		'/org/openbmc/control/chassis0': 0,
		'/org/openbmc/control/power0' : 0,
		'/org/openbmc/control/led/identify' : 0,
		'/org/openbmc/control/host0' : 0,
		'/org/openbmc/control/flash/bios' : 0,
	}
}

ID_LOOKUP = {
	'FRU' : {
		0x0d : '<inventory_root>/system/chassis',
		0x34 : '<inventory_root>/system/chassis/motherboard',
		0x01 : '<inventory_root>/system/chassis/motherboard/cpu',
		0x02 : '<inventory_root>/system/chassis/motherboard/membuf',
		0x03 : '<inventory_root>/system/chassis/motherboard/dimm0',
		0x04 : '<inventory_root>/system/chassis/motherboard/dimm1',
		0x05 : '<inventory_root>/system/chassis/motherboard/dimm2',
		0x06 : '<inventory_root>/system/chassis/motherboard/dimm3',
		0x35 : '<inventory_root>/system',
	},
	'FRU_STR' : {
		'PRODUCT_15' : '<inventory_root>/system',
		'CHASSIS_2' : '<inventory_root>/system/chassis',
		'BOARD_1'   : '<inventory_root>/system/chassis/motherboard/cpu',
		'BOARD_2'   : '<inventory_root>/system/chassis/motherboard/membuf',
		'BOARD_14'   : '<inventory_root>/system/chassis/motherboard',
		'PRODUCT_3'   : '<inventory_root>/system/chassis/motherboard/dimm0',
		'PRODUCT_4'   : '<inventory_root>/system/chassis/motherboard/dimm1',
		'PRODUCT_5'   : '<inventory_root>/system/chassis/motherboard/dimm2',
		'PRODUCT_6'   : '<inventory_root>/system/chassis/motherboard/dimm3',
	},
	'SENSOR' : {
		0x34 : '<inventory_root>/system/chassis/motherboard',
		0x37 : '<inventory_root>/system/chassis/motherboard/refclock',
		0x38 : '<inventory_root>/system/chassis/motherboard/pcieclock',
		0x39 : '<inventory_root>/system/chassis/motherboard/todclock',
		0x3A : '<inventory_root>/system/chassis/apss',
		0x2f : '<inventory_root>/system/chassis/motherboard/cpu',
		0x22 : '<inventory_root>/system/chassis/motherboard/cpu/core1',
		0x23 : '<inventory_root>/system/chassis/motherboard/cpu/core2',
		0x24 : '<inventory_root>/system/chassis/motherboard/cpu/core3',
		0x25 : '<inventory_root>/system/chassis/motherboard/cpu/core4',
		0x26 : '<inventory_root>/system/chassis/motherboard/cpu/core5',
		0x27 : '<inventory_root>/system/chassis/motherboard/cpu/core6',
		0x28 : '<inventory_root>/system/chassis/motherboard/cpu/core9',
		0x29 : '<inventory_root>/system/chassis/motherboard/cpu/core10',
		0x2a : '<inventory_root>/system/chassis/motherboard/cpu/core11',
		0x2b : '<inventory_root>/system/chassis/motherboard/cpu/core12',
		0x2c : '<inventory_root>/system/chassis/motherboard/cpu/core13',
		0x2d : '<inventory_root>/system/chassis/motherboard/cpu/core14',
		0x2e : '<inventory_root>/system/chassis/motherboard/membuf',
		0x1e : '<inventory_root>/system/chassis/motherboard/dimm0',
		0x1f : '<inventory_root>/system/chassis/motherboard/dimm1',
		0x20 : '<inventory_root>/system/chassis/motherboard/dimm2',
		0x21 : '<inventory_root>/system/chassis/motherboard/dimm3',
		0x09 : '/org/openbmc/sensors/host/BootCount',
		0x05 : '/org/openbmc/sensors/host/BootProgress',
		0x32 : '/org/openbmc/sensors/host/OperatingSystemStatus',
	},
	'GPIO_PRESENT' : {
		'SLOT0_PRESENT' : '<inventory_root>/system/chassis/motherboard/pciecard_x16',
		'SLOT1_PRESENT' : '<inventory_root>/system/chassis/motherboard/pciecard_x8',
	}
}

# Miscellaneous non-poll sensor with system specific properties.
# The sensor id is the same as those defined in ID_LOOKUP['SENSOR'].
MISC_SENSORS = {
}

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
