## System states
##   state can change to next state in 2 ways:
##   - a process emits a GotoSystemState signal with state name to goto
##   - objects specified in EXIT_STATE_DEPEND have started
SYSTEM_STATES = [
	'BASE_APPS',
	'BMC_STARTING',
	'BMC_STARTING2',
	'BMC_READY',
	'HOST_POWERING_ON',
	'HOST_POWERED_ON',
	'INVENTORY_UPLOADED',
	'HOST_BOOTING',
	'HOST_BOOTED',
	'HOST_POWERED_OFF',
]

EXIT_STATE_DEPEND = {
	'BASE_APPS' : {
		'/org/openbmc/sensors': 0,
	},
	'BMC_STARTING' : {
		'/org/openbmc/control/power0' : 0,
		'/org/openbmc/control/host0' : 0,
		'/org/openbmc/control/flash/bios' : 0,
	},
	'BMC_STARTING2' : {
		'/org/openbmc/control/fans' : 0,
		'/org/openbmc/control/chassis0': 0,
	},
}

FRU_INSTANCES = {
	'<inventory_root>/system' : { 'fru_type' : 'SYSTEM','is_fru' : True, 'present' : "True" },
	'<inventory_root>/system/bios' : { 'fru_type' : 'SYSTEM','is_fru' : True, 'present' : "True" },
	'<inventory_root>/system/misc' : { 'fru_type' : 'SYSTEM','is_fru' : False, },

	'<inventory_root>/system/chassis' : { 'fru_type' : 'SYSTEM','is_fru' : True, 'present' : "True" },

	'<inventory_root>/system/chassis/motherboard' : { 'fru_type' : 'MAIN_PLANAR','is_fru' : True, },
	'<inventory_root>/system/chassis/io_board' : { 'fru_type' : 'DAUGHTER_CARD','is_fru' : True, },
	'<inventory_root>/system/chassis/sas_expander' : { 'fru_type' : 'DAUGHTER_CARD','is_fru' : True, },
	'<inventory_root>/system/chassis/hdd_backplane' : { 'fru_type' : 'DAUGHTER_CARD','is_fru' : True, },

	'<inventory_root>/system/systemevent'                  : { 'fru_type' : 'SYSTEM_EVENT', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/refclock' : { 'fru_type' : 'MAIN_PLANAR', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/pcieclock': { 'fru_type' : 'MAIN_PLANAR', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/todclock' : { 'fru_type' : 'MAIN_PLANAR', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/apss'     : { 'fru_type' : 'MAIN_PLANAR', 'is_fru' : False, },

	'<inventory_root>/system/chassis/fan0' : { 'fru_type' : 'FAN','is_fru' : True, },
	'<inventory_root>/system/chassis/fan1' : { 'fru_type' : 'FAN','is_fru' : True, },
	'<inventory_root>/system/chassis/fan2' : { 'fru_type' : 'FAN','is_fru' : True, },
	'<inventory_root>/system/chassis/fan3' : { 'fru_type' : 'FAN','is_fru' : True, },
	'<inventory_root>/system/chassis/fan4' : { 'fru_type' : 'FAN','is_fru' : True, },
	'<inventory_root>/system/chassis/fan5' : { 'fru_type' : 'FAN','is_fru' : True, },

	'<inventory_root>/system/chassis/motherboard/bmc' : { 'fru_type' : 'BMC','is_fru' : False, 'manufacturer' : 'ASPEED' },

	'<inventory_root>/system/chassis/motherboard/cpu0' : { 'fru_type' : 'CPU', 'is_fru' : True, },
	'<inventory_root>/system/chassis/motherboard/cpu1' : { 'fru_type' : 'CPU', 'is_fru' : True, },

	'<inventory_root>/system/chassis/motherboard/cpu0/core0' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core1' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core2' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core3' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core4' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core5' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core6' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core7' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core8' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core9' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core10': { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu0/core11': { 'fru_type' : 'CORE', 'is_fru' : False, },

	'<inventory_root>/system/chassis/motherboard/cpu1/core0' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core1' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core2' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core3' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core4' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core5' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core6' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core7' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core8' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core9' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core10' : { 'fru_type' : 'CORE', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/cpu1/core11' : { 'fru_type' : 'CORE', 'is_fru' : False, },

	'<inventory_root>/system/chassis/motherboard/membuf0' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/membuf1' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/membuf2' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/membuf3' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/membuf4' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/membuf5' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/membuf6' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },
	'<inventory_root>/system/chassis/motherboard/membuf7' : { 'fru_type' : 'MEMORY_BUFFER', 'is_fru' : False, },

	'<inventory_root>/system/chassis/motherboard/dimm0' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm1' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm2' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm3' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm4' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm5' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm6' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm7' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm8' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm9' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm10' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm11' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm12' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm13' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm14' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm15' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm16' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm17' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm18' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm19' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm20' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm21' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm22' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm23' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm24' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm25' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm26' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm27' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm28' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm29' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm30' : { 'fru_type' : 'DIMM', 'is_fru' : True,},
	'<inventory_root>/system/chassis/motherboard/dimm31' : { 'fru_type' : 'DIMM', 'is_fru' : True,},

	'<inventory_root>/system/chassis/io_board/pcie_slot0_riser' : { 'fru_type' : 'PCIE_RISER', 'is_fru' : True,},
	'<inventory_root>/system/chassis/io_board/pcie_slot1_riser' : { 'fru_type' : 'PCIE_RISER', 'is_fru' : True,},
	'<inventory_root>/system/chassis/io_board/pcie_slot2_riser' : { 'fru_type' : 'PCIE_RISER', 'is_fru' : True,},
	'<inventory_root>/system/chassis/io_board/pcie_slot0' : { 'fru_type' : 'PCIE_CARD', 'is_fru' : True,},
	'<inventory_root>/system/chassis/io_board/pcie_slot1' :	{ 'fru_type' : 'PCIE_CARD', 'is_fru' : True,},
	'<inventory_root>/system/chassis/io_board/pcie_slot2' :	{ 'fru_type' : 'PCIE_CARD', 'is_fru' : True,},
	'<inventory_root>/system/chassis/io_board/pcie_mezz0' :	{ 'fru_type' : 'PCIE_CARD', 'is_fru' : True,},
	'<inventory_root>/system/chassis/io_board/pcie_mezz1' :	{ 'fru_type' : 'PCIE_CARD', 'is_fru' : True,},
}

ID_LOOKUP = {
	'FRU' : {
		0x03 : '<inventory_root>/system/chassis/motherboard',
		0x40 : '<inventory_root>/system/chassis/io_board',
		0x01 : '<inventory_root>/system/chassis/motherboard/cpu0',
                0x02 : '<inventory_root>/system/chassis/motherboard/cpu1',
		0x04 : '<inventory_root>/system/chassis/motherboard/membuf0',
                0x05 : '<inventory_root>/system/chassis/motherboard/membuf1',
                0x06 : '<inventory_root>/system/chassis/motherboard/membuf2',
                0x07 : '<inventory_root>/system/chassis/motherboard/membuf3',
                0x08 : '<inventory_root>/system/chassis/motherboard/membuf4',
                0x09 : '<inventory_root>/system/chassis/motherboard/membuf5',
                0x0a : '<inventory_root>/system/chassis/motherboard/membuf6',
                0x0b : '<inventory_root>/system/chassis/motherboard/membuf7',
		0x0c : '<inventory_root>/system/chassis/motherboard/dimm0',
		0x0d : '<inventory_root>/system/chassis/motherboard/dimm1',
		0x0e : '<inventory_root>/system/chassis/motherboard/dimm2',
		0x0f : '<inventory_root>/system/chassis/motherboard/dimm3',
                0x10 : '<inventory_root>/system/chassis/motherboard/dimm4',
                0x11 : '<inventory_root>/system/chassis/motherboard/dimm5',
                0x12 : '<inventory_root>/system/chassis/motherboard/dimm6',
                0x13 : '<inventory_root>/system/chassis/motherboard/dimm7',
                0x14 : '<inventory_root>/system/chassis/motherboard/dimm8',
                0x15 : '<inventory_root>/system/chassis/motherboard/dimm9',
		0x16 : '<inventory_root>/system/chassis/motherboard/dimm10',
		0x17 : '<inventory_root>/system/chassis/motherboard/dimm11',
		0x18 : '<inventory_root>/system/chassis/motherboard/dimm12',
		0x19 : '<inventory_root>/system/chassis/motherboard/dimm13',
		0x1a : '<inventory_root>/system/chassis/motherboard/dimm14',
		0x1b : '<inventory_root>/system/chassis/motherboard/dimm15',
		0x1c : '<inventory_root>/system/chassis/motherboard/dimm16',
		0x1d : '<inventory_root>/system/chassis/motherboard/dimm17',
		0x1e : '<inventory_root>/system/chassis/motherboard/dimm18',
		0x1f : '<inventory_root>/system/chassis/motherboard/dimm19',
		0x20 : '<inventory_root>/system/chassis/motherboard/dimm20',
                0x21 : '<inventory_root>/system/chassis/motherboard/dimm21',
                0x22 : '<inventory_root>/system/chassis/motherboard/dimm22',
                0x23 : '<inventory_root>/system/chassis/motherboard/dimm23',
                0x24 : '<inventory_root>/system/chassis/motherboard/dimm24',
                0x25 : '<inventory_root>/system/chassis/motherboard/dimm25',
                0x26 : '<inventory_root>/system/chassis/motherboard/dimm26',
                0x27 : '<inventory_root>/system/chassis/motherboard/dimm27',
                0x28 : '<inventory_root>/system/chassis/motherboard/dimm28',
                0x29 : '<inventory_root>/system/chassis/motherboard/dimm29',
                0x2a : '<inventory_root>/system/chassis/motherboard/dimm30',
                0x2b : '<inventory_root>/system/chassis/motherboard/dimm31',
		0x33 : '<inventory_root>/system',
	},
	'FRU_STR' : {
		'PRODUCT_0'  : '<inventory_root>/system/bios',
		'BOARD_3'    : '<inventory_root>/system/misc',
		'PRODUCT_51' : '<inventory_root>/system/misc',
		'BOARD_100'  : '<inventory_root>/system/chassis/io_board',
		'BOARD_101'  : '<inventory_root>/system/chassis/sas_expander',
		'BOARD_102'  : '<inventory_root>/system/chassis/hdd_backplane',
		'CHASSIS_3'  : '<inventory_root>/system/chassis/motherboard',
		'BOARD_1'    : '<inventory_root>/system/chassis/motherboard/cpu0',
		'BOARD_2'    : '<inventory_root>/system/chassis/motherboard/cpu1',
		'BOARD_4'    : '<inventory_root>/system/chassis/motherboard/membuf0',
		'BOARD_5'    : '<inventory_root>/system/chassis/motherboard/membuf1',
		'BOARD_6'    : '<inventory_root>/system/chassis/motherboard/membuf2',
		'BOARD_7'    : '<inventory_root>/system/chassis/motherboard/membuf3',
		'BOARD_8'    : '<inventory_root>/system/chassis/motherboard/membuf4',
		'BOARD_9'    : '<inventory_root>/system/chassis/motherboard/membuf5',
		'BOARD_10'   : '<inventory_root>/system/chassis/motherboard/membuf6',
		'BOARD_11'   : '<inventory_root>/system/chassis/motherboard/membuf7',
		'PRODUCT_12'   : '<inventory_root>/system/chassis/motherboard/dimm0',
		'PRODUCT_13'   : '<inventory_root>/system/chassis/motherboard/dimm1',
		'PRODUCT_14'   : '<inventory_root>/system/chassis/motherboard/dimm2',
		'PRODUCT_15'   : '<inventory_root>/system/chassis/motherboard/dimm3',
		'PRODUCT_16'   : '<inventory_root>/system/chassis/motherboard/dimm4',
		'PRODUCT_17'   : '<inventory_root>/system/chassis/motherboard/dimm5',
		'PRODUCT_18'   : '<inventory_root>/system/chassis/motherboard/dimm6',
		'PRODUCT_19'   : '<inventory_root>/system/chassis/motherboard/dimm7',
		'PRODUCT_20'   : '<inventory_root>/system/chassis/motherboard/dimm8',
		'PRODUCT_21'   : '<inventory_root>/system/chassis/motherboard/dimm9',
		'PRODUCT_22'   : '<inventory_root>/system/chassis/motherboard/dimm10',
		'PRODUCT_23'   : '<inventory_root>/system/chassis/motherboard/dimm11',
		'PRODUCT_24'   : '<inventory_root>/system/chassis/motherboard/dimm12',
		'PRODUCT_25'   : '<inventory_root>/system/chassis/motherboard/dimm13',
		'PRODUCT_26'   : '<inventory_root>/system/chassis/motherboard/dimm14',
		'PRODUCT_27'   : '<inventory_root>/system/chassis/motherboard/dimm15',
		'PRODUCT_28'   : '<inventory_root>/system/chassis/motherboard/dimm16',
		'PRODUCT_29'   : '<inventory_root>/system/chassis/motherboard/dimm17',
		'PRODUCT_30'   : '<inventory_root>/system/chassis/motherboard/dimm18',
		'PRODUCT_31'   : '<inventory_root>/system/chassis/motherboard/dimm19',
		'PRODUCT_32'   : '<inventory_root>/system/chassis/motherboard/dimm20',
		'PRODUCT_33'   : '<inventory_root>/system/chassis/motherboard/dimm21',
		'PRODUCT_34'   : '<inventory_root>/system/chassis/motherboard/dimm22',
		'PRODUCT_35'   : '<inventory_root>/system/chassis/motherboard/dimm23',
		'PRODUCT_36'   : '<inventory_root>/system/chassis/motherboard/dimm24',
		'PRODUCT_37'   : '<inventory_root>/system/chassis/motherboard/dimm25',
		'PRODUCT_38'   : '<inventory_root>/system/chassis/motherboard/dimm26',
		'PRODUCT_39'   : '<inventory_root>/system/chassis/motherboard/dimm27',
		'PRODUCT_40'   : '<inventory_root>/system/chassis/motherboard/dimm28',
		'PRODUCT_41'   : '<inventory_root>/system/chassis/motherboard/dimm29',
		'PRODUCT_42'   : '<inventory_root>/system/chassis/motherboard/dimm30',
		'PRODUCT_43'   : '<inventory_root>/system/chassis/motherboard/dimm31',
	},
	'SENSOR' : {
		0x35 : '<inventory_root>/system/systemevent',
		0x36 : '<inventory_root>/system/powerlimit',
                0x34 : '<inventory_root>/system/chassis/motherboard',
		0x31 : '<inventory_root>/system/chassis/motherboard/pcielink',
		0x37 : '<inventory_root>/system/chassis/motherboard/refclock',
		0x38 : '<inventory_root>/system/chassis/motherboard/pcieclock',
		0x39 : '<inventory_root>/system/chassis/motherboard/todclock',
		0x3A : '<inventory_root>/system/chassis/motherboard/apss',
		0x0c : '<inventory_root>/system/chassis/motherboard/cpu0',
                0x0e : '<inventory_root>/system/chassis/motherboard/cpu1',
		0xc8 : '<inventory_root>/system/chassis/motherboard/cpu0/core0',
		0xc9 : '<inventory_root>/system/chassis/motherboard/cpu0/core1',
		0xca : '<inventory_root>/system/chassis/motherboard/cpu0/core2',
		0xcb : '<inventory_root>/system/chassis/motherboard/cpu0/core3',
		0xcc : '<inventory_root>/system/chassis/motherboard/cpu0/core4',
		0xcd : '<inventory_root>/system/chassis/motherboard/cpu0/core5',
		0xce : '<inventory_root>/system/chassis/motherboard/cpu0/core6',
		0xcf : '<inventory_root>/system/chassis/motherboard/cpu0/core7',
		0xd0 : '<inventory_root>/system/chassis/motherboard/cpu0/core8',
		0xd1 : '<inventory_root>/system/chassis/motherboard/cpu0/core9',
		0xd2 : '<inventory_root>/system/chassis/motherboard/cpu0/core10',
		0xd3 : '<inventory_root>/system/chassis/motherboard/cpu0/core11',
                0xd4 : '<inventory_root>/system/chassis/motherboard/cpu1/core0',
                0xd5 : '<inventory_root>/system/chassis/motherboard/cpu1/core1',
                0xd6 : '<inventory_root>/system/chassis/motherboard/cpu1/core2',
                0xd7 : '<inventory_root>/system/chassis/motherboard/cpu1/core3',
                0xd8 : '<inventory_root>/system/chassis/motherboard/cpu1/core4',
                0xd9 : '<inventory_root>/system/chassis/motherboard/cpu1/core5',
                0xda : '<inventory_root>/system/chassis/motherboard/cpu1/core6',
                0xdb : '<inventory_root>/system/chassis/motherboard/cpu1/core7',
                0xdc : '<inventory_root>/system/chassis/motherboard/cpu1/core8',
                0xdd : '<inventory_root>/system/chassis/motherboard/cpu1/core9',
                0xde : '<inventory_root>/system/chassis/motherboard/cpu1/core10',
                0xdf : '<inventory_root>/system/chassis/motherboard/cpu1/core11',
		0x40 : '<inventory_root>/system/chassis/motherboard/membuf0',
    	        0x41 : '<inventory_root>/system/chassis/motherboard/membuf1',
       		0x42 : '<inventory_root>/system/chassis/motherboard/membuf2',
       		0x43 : '<inventory_root>/system/chassis/motherboard/membuf3',
       		0x44 : '<inventory_root>/system/chassis/motherboard/membuf4',
                0x45 : '<inventory_root>/system/chassis/motherboard/membuf5',
                0x46 : '<inventory_root>/system/chassis/motherboard/membuf6',
                0x47 : '<inventory_root>/system/chassis/motherboard/membuf7',
		0x10 : '<inventory_root>/system/chassis/motherboard/dimm0',
		0x11 : '<inventory_root>/system/chassis/motherboard/dimm1',
		0x12 : '<inventory_root>/system/chassis/motherboard/dimm2',
		0x13 : '<inventory_root>/system/chassis/motherboard/dimm3',
                0x14 : '<inventory_root>/system/chassis/motherboard/dimm4',
                0x15 : '<inventory_root>/system/chassis/motherboard/dimm5',
                0x16 : '<inventory_root>/system/chassis/motherboard/dimm6',
                0x17 : '<inventory_root>/system/chassis/motherboard/dimm7',
                0x18 : '<inventory_root>/system/chassis/motherboard/dimm8',
                0x19 : '<inventory_root>/system/chassis/motherboard/dimm9',
                0x1a : '<inventory_root>/system/chassis/motherboard/dimm10',
                0x1b : '<inventory_root>/system/chassis/motherboard/dimm11',
                0x1c : '<inventory_root>/system/chassis/motherboard/dimm12',
                0x1d : '<inventory_root>/system/chassis/motherboard/dimm13',
                0x1e : '<inventory_root>/system/chassis/motherboard/dimm14',
                0x1f : '<inventory_root>/system/chassis/motherboard/dimm15',
                0x20 : '<inventory_root>/system/chassis/motherboard/dimm16',
                0x21 : '<inventory_root>/system/chassis/motherboard/dimm17',
                0x22 : '<inventory_root>/system/chassis/motherboard/dimm18',
                0x23 : '<inventory_root>/system/chassis/motherboard/dimm19',
                0x24 : '<inventory_root>/system/chassis/motherboard/dimm20',
                0x25 : '<inventory_root>/system/chassis/motherboard/dimm21',
                0x26 : '<inventory_root>/system/chassis/motherboard/dimm22',
                0x27 : '<inventory_root>/system/chassis/motherboard/dimm23',
                0x28 : '<inventory_root>/system/chassis/motherboard/dimm24',
                0x29 : '<inventory_root>/system/chassis/motherboard/dimm25',
                0x2a : '<inventory_root>/system/chassis/motherboard/dimm26',
                0x2b : '<inventory_root>/system/chassis/motherboard/dimm27',
                0x2c : '<inventory_root>/system/chassis/motherboard/dimm28',
                0x2d : '<inventory_root>/system/chassis/motherboard/dimm29',
                0x2e : '<inventory_root>/system/chassis/motherboard/dimm30',
                0x2f : '<inventory_root>/system/chassis/motherboard/dimm31',
		0x09 : '/org/openbmc/sensors/host/BootCount',
		0x05 : '/org/openbmc/sensors/host/BootProgress',
		0x04 : '/org/openbmc/sensors/host/HostStatus',
		0x32 : '/org/openbmc/sensors/host/OperatingSystemStatus',
	},
	'GPIO_PRESENT' : {
		'SLOT0_RISER_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_slot0_riser',
		'SLOT1_RISER_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_slot1_riser',
		'SLOT2_RISER_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_slot2_riser',
		'SLOT0_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_slot0',
		'SLOT1_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_slot1',
		'SLOT2_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_slot2',
		'MEZZ0_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_mezz0',
		'MEZZ1_PRESENT' : '<inventory_root>/system/chassis/io_board/pcie_mezz1',
	}
}

GPIO_CONFIG = {}
GPIO_CONFIG['FSI_CLK']    =   { 'gpio_pin': 'A4', 'direction': 'out' }
GPIO_CONFIG['FSI_DATA']   =   { 'gpio_pin': 'A5', 'direction': 'out' }
GPIO_CONFIG['FSI_ENABLE'] =   { 'gpio_pin': 'D0', 'direction': 'out' }
GPIO_CONFIG['POWER_PIN']  =   { 'gpio_pin': 'E1', 'direction': 'out'  }
GPIO_CONFIG['CRONUS_SEL'] =   { 'gpio_pin': 'A6', 'direction': 'out'  }
GPIO_CONFIG['PGOOD']      =   { 'gpio_pin': 'C7', 'direction': 'in'  }
GPIO_CONFIG['POWER_BUTTON'] = { 'gpio_pin': 'E0', 'direction': 'both' }
GPIO_CONFIG['PCIE_RESET']   = { 'gpio_pin': 'B5', 'direction': 'out' }
GPIO_CONFIG['USB_RESET']    = { 'gpio_pin': 'B6', 'direction': 'out' }

GPIO_CONFIG['IDBTN']       = { 'gpio_pin': 'Q7', 'direction': 'out' }
GPIO_CONFIG['BMC_THROTTLE']       = { 'gpio_pin': 'J3', 'direction': 'out' }
GPIO_CONFIG['RESET_BUTTON']       = { 'gpio_pin': 'E2', 'direction': 'both' }
GPIO_CONFIG['CPLD_TCK']    	  =   { 'gpio_pin': 'P0', 'direction': 'out' }
GPIO_CONFIG['CPLD_TDO']    	  =   { 'gpio_pin': 'P1', 'direction': 'out' }
GPIO_CONFIG['CPLD_TDI']    	  =   { 'gpio_pin': 'P2', 'direction': 'out' }
GPIO_CONFIG['CPLD_TMS']    	  =   { 'gpio_pin': 'P3', 'direction': 'out' }
GPIO_CONFIG['CHECKSTOP']   	  =   { 'gpio_pin': 'P5', 'direction': 'falling' }

GPIO_CONFIG['SLOT0_RISER_PRESENT'] =   { 'gpio_pin': 'N0', 'direction': 'in' }
GPIO_CONFIG['SLOT1_RISER_PRESENT'] =   { 'gpio_pin': 'N1', 'direction': 'in' }
GPIO_CONFIG['SLOT2_RISER_PRESENT'] =   { 'gpio_pin': 'N2', 'direction': 'in' }
GPIO_CONFIG['SLOT0_PRESENT'] =         { 'gpio_pin': 'N3', 'direction': 'in' }
GPIO_CONFIG['SLOT1_PRESENT'] =         { 'gpio_pin': 'N4', 'direction': 'in' }
GPIO_CONFIG['SLOT2_PRESENT'] =         { 'gpio_pin': 'N5', 'direction': 'in' }
GPIO_CONFIG['MEZZ0_PRESENT'] =         { 'gpio_pin': 'O0', 'direction': 'in' }
GPIO_CONFIG['MEZZ1_PRESENT'] =         { 'gpio_pin': 'O1', 'direction': 'in' }

GPIO_CONFIGS = {
    'power_config' : {
        'power_good_in' : 'PGOOD',
        'power_up_outs' : [
            ('POWER_PIN', False),
        ],
        'reset_outs' : [
            ('USB_RESET', False),
        ],
        'pci_reset_outs': [
            # net name, polarity, reset hold
            ('PCIE_RESET', False, False),
        ],
    },
    'hostctl_config' : {
        'fsi_data' : 'FSI_DATA',
        'fsi_clk' : 'FSI_CLK',
        'fsi_enable' : 'FSI_ENABLE',
        'cronus_sel' : 'CRONUS_SEL',
        'optionals' : [
            ('BMC_THROTTLE', True),
            ('IDBTN', False),
        ],
    },
}

# Miscellaneous non-poll sensor with system specific properties.
# The sensor id is the same as those defined in ID_LOOKUP['SENSOR'].
MISC_SENSORS = {
}

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
