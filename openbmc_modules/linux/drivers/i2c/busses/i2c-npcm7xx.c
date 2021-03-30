// SPDX-License-Identifier: GPL-2.0
/*
 * Nuvoton NPCM7xx SMB Controller driver
 *
 * Copyright (C) 2018 Nuvoton Technologies tali.perry@nuvoton.com
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/jiffies.h>

#define I2C_VERSION "0.1.0"

enum smb_mode {
	SMB_SLAVE = 1,
	SMB_MASTER
};

/*
 * External SMB Interface driver xfer indication values, which indicate status
 * of the bus.
 */
enum smb_state_ind {
	SMB_NO_STATUS_IND = 0,
	SMB_SLAVE_RCV_IND = 1,
	SMB_SLAVE_XMIT_IND = 2,
	SMB_SLAVE_XMIT_MISSING_DATA_IND = 3,
	SMB_SLAVE_RESTART_IND = 4,
	SMB_SLAVE_DONE_IND = 5,
	SMB_MASTER_DONE_IND = 6,
	SMB_NACK_IND = 8,
	SMB_BUS_ERR_IND = 9,
	SMB_WAKE_UP_IND = 10,
	SMB_BLOCK_BYTES_ERR_IND = 12,
	SMB_SLAVE_RCV_MISSING_DATA_IND = 14,
};

// SMBus Operation type values
enum smb_oper {
	SMB_NO_OPER = 0,
	SMB_WRITE_OPER = 1,
	SMB_READ_OPER = 2
};

// SMBus Bank (FIFO mode)
enum smb_bank {
	SMB_BANK_0 = 0,
	SMB_BANK_1 = 1
};

// Internal SMB states values (for the SMB module state machine).
enum smb_state {
	SMB_DISABLE = 0,
	SMB_IDLE,
	SMB_MASTER_START,
	SMB_SLAVE_MATCH,
	SMB_OPER_STARTED,
	SMB_STOP_PENDING
};

// Module supports setting multiple own slave addresses
enum smb_addr {
	SMB_SLAVE_ADDR1 = 0,
	SMB_SLAVE_ADDR2,
	SMB_SLAVE_ADDR3,
	SMB_SLAVE_ADDR4,
	SMB_SLAVE_ADDR5,
	SMB_SLAVE_ADDR6,
	SMB_SLAVE_ADDR7,
	SMB_SLAVE_ADDR8,
	SMB_SLAVE_ADDR9,
	SMB_SLAVE_ADDR10,
	SMB_GC_ADDR,
	SMB_ARP_ADDR
};

// global regs
static struct regmap *gcr_regmap;
static struct regmap *clk_regmap;

#define NPCM_I2CSEGCTL  0xE4
#define I2CSEGCTL_VAL	0x0333F000

// Common regs
#define NPCM_SMBSDA			0x000
#define NPCM_SMBST			0x002
#define NPCM_SMBCST			0x004
#define NPCM_SMBCTL1			0x006
#define NPCM_SMBADDR1			0x008
#define NPCM_SMBCTL2			0x00A
#define NPCM_SMBADDR2			0x00C
#define NPCM_SMBCTL3			0x00E
#define NPCM_SMBCST2			0x018
#define NPCM_SMBCST3			0x019
#define SMB_VER				0x01F

// BANK 0 regs
#define NPCM_SMBADDR3			0x010
#define NPCM_SMBADDR7			0x011
#define NPCM_SMBADDR4			0x012
#define NPCM_SMBADDR8			0x013
#define NPCM_SMBADDR5			0x014
#define NPCM_SMBADDR9			0x015
#define NPCM_SMBADDR6			0x016
#define NPCM_SMBADDR10			0x017

// SMBADDR array: because the addr regs are sprinkled all over the address space
const int  NPCM_SMBADDR[10] = {NPCM_SMBADDR1, NPCM_SMBADDR2, NPCM_SMBADDR3,
			       NPCM_SMBADDR4, NPCM_SMBADDR5, NPCM_SMBADDR6,
			       NPCM_SMBADDR7, NPCM_SMBADDR8, NPCM_SMBADDR9,
			       NPCM_SMBADDR10};

#define NPCM_SMBCTL4			0x01A
#define NPCM_SMBCTL5			0x01B
#define NPCM_SMBSCLLT			0x01C // SCL Low Time
#define NPCM_SMBFIF_CTL			0x01D // FIFO Control
#define NPCM_SMBSCLHT			0x01E // SCL High Time

// BANK 1 regs
#define NPCM_SMBFIF_CTS			0x010 // Both FIFOs Control and status
#define NPCM_SMBTXF_CTL			0x012 // Tx-FIFO Control
#define NPCM_SMBT_OUT			0x014 // Bus T.O.
#define NPCM_SMBPEC			0x016 // PEC Data
#define NPCM_SMBTXF_STS			0x01A // Tx-FIFO Status
#define NPCM_SMBRXF_STS			0x01C // Rx-FIFO Status
#define NPCM_SMBRXF_CTL			0x01E // Rx-FIFO Control

// NPCM_SMBST reg fields
#define NPCM_SMBST_XMIT			BIT(0)
#define NPCM_SMBST_MASTER		BIT(1)
#define NPCM_SMBST_NMATCH		BIT(2)
#define NPCM_SMBST_STASTR		BIT(3)
#define NPCM_SMBST_NEGACK		BIT(4)
#define NPCM_SMBST_BER			BIT(5)
#define NPCM_SMBST_SDAST		BIT(6)
#define NPCM_SMBST_SLVSTP		BIT(7)

// NPCM_SMBCST reg fields
#define NPCM_SMBCST_BUSY		BIT(0)
#define NPCM_SMBCST_BB			BIT(1)
#define NPCM_SMBCST_MATCH		BIT(2)
#define NPCM_SMBCST_GCMATCH		BIT(3)
#define NPCM_SMBCST_TSDA		BIT(4)
#define NPCM_SMBCST_TGSCL		BIT(5)
#define NPCM_SMBCST_MATCHAF		BIT(6)
#define NPCM_SMBCST_ARPMATCH		BIT(7)

// NPCM_SMBCTL1 reg fields
#define NPCM_SMBCTL1_START		BIT(0)
#define NPCM_SMBCTL1_STOP		BIT(1)
#define NPCM_SMBCTL1_INTEN		BIT(2)
#define NPCM_SMBCTL1_EOBINTE		BIT(3)
#define NPCM_SMBCTL1_ACK		BIT(4)
#define NPCM_SMBCTL1_GCMEN		BIT(5)
#define NPCM_SMBCTL1_NMINTE		BIT(6)
#define NPCM_SMBCTL1_STASTRE		BIT(7)

// RW1S fields (inside a RW reg):
#define NPCM_SMBCTL1_RWS_FIELDS	  (NPCM_SMBCTL1_START | NPCM_SMBCTL1_STOP | \
				   NPCM_SMBCTL1_ACK)
// NPCM_SMBADDR reg fields
#define NPCM_SMBADDR_A			GENMASK(6, 0)
#define NPCM_SMBADDR_SAEN		BIT(7)

// NPCM_SMBCTL2 reg fields
#define SMBCTL2_ENABLE			BIT(0)
#define SMBCTL2_SCLFRQ6_0		GENMASK(7, 1)

// NPCM_SMBCTL3 reg fields
#define SMBCTL3_SCLFRQ8_7		GENMASK(1, 0)
#define SMBCTL3_ARPMEN			BIT(2)
#define SMBCTL3_IDL_START		BIT(3)
#define SMBCTL3_400K_MODE		BIT(4)
#define SMBCTL3_BNK_SEL			BIT(5)
#define SMBCTL3_SDA_LVL			BIT(6)
#define SMBCTL3_SCL_LVL			BIT(7)

// NPCM_SMBCST2 reg fields
#define NPCM_SMBCST2_MATCHA1F		BIT(0)
#define NPCM_SMBCST2_MATCHA2F		BIT(1)
#define NPCM_SMBCST2_MATCHA3F		BIT(2)
#define NPCM_SMBCST2_MATCHA4F		BIT(3)
#define NPCM_SMBCST2_MATCHA5F		BIT(4)
#define NPCM_SMBCST2_MATCHA6F		BIT(5)
#define NPCM_SMBCST2_MATCHA7F		BIT(5)
#define NPCM_SMBCST2_INTSTS		BIT(7)

// NPCM_SMBCST3 reg fields
#define NPCM_SMBCST3_MATCHA8F		BIT(0)
#define NPCM_SMBCST3_MATCHA9F		BIT(1)
#define NPCM_SMBCST3_MATCHA10F		BIT(2)
#define NPCM_SMBCST3_EO_BUSY		BIT(7)

// NPCM_SMBCTL4 reg fields
#define SMBCTL4_HLDT			GENMASK(5, 0)
#define SMBCTL4_LVL_WE			BIT(7)

// NPCM_SMBCTL5 reg fields
#define SMBCTL5_DBNCT			GENMASK(3, 0)

// NPCM_SMBFIF_CTS reg fields
#define NPCM_SMBFIF_CTS_RXF_TXE		BIT(1)
#define NPCM_SMBFIF_CTS_RFTE_IE		BIT(3)
#define NPCM_SMBFIF_CTS_CLR_FIFO	BIT(6)
#define NPCM_SMBFIF_CTS_SLVRSTR		BIT(7)

// NPCM_SMBTXF_CTL reg fields
#define NPCM_SMBTXF_CTL_TX_THR		GENMASK(4, 0)
#define NPCM_SMBTXF_CTL_THR_TXIE	BIT(6)

// NPCM_SMBT_OUT reg fields
#define NPCM_SMBT_OUT_TO_CKDIV		GENMASK(5, 0)
#define NPCM_SMBT_OUT_T_OUTIE		BIT(6)
#define NPCM_SMBT_OUT_T_OUTST		BIT(7)

// NPCM_SMBTXF_STS reg fields
#define NPCM_SMBTXF_STS_TX_BYTES	GENMASK(4, 0)
#define NPCM_SMBTXF_STS_TX_THST		BIT(6)

// NPCM_SMBRXF_STS reg fields
#define NPCM_SMBRXF_STS_RX_BYTES	GENMASK(4, 0)
#define NPCM_SMBRXF_STS_RX_THST		BIT(6)

// NPCM_SMBFIF_CTL reg fields
#define NPCM_SMBFIF_CTL_FIFO_EN		BIT(4)

// NPCM_SMBRXF_CTL reg fields
#define NPCM_SMBRXF_CTL_RX_THR		GENMASK(4, 0)
#define NPCM_SMBRXF_CTL_LAST_PEC	BIT(5)
#define NPCM_SMBRXF_CTL_THR_RXIE	BIT(6)

#define SMBUS_FIFO_SIZE			16

// SMB_VER reg fields
#define SMB_VER_VERSION			GENMASK(6, 0)
#define SMB_VER_FIFO_EN			BIT(7)

// stall/stuck timeout
const unsigned int DEFAULT_STALL_COUNT =	25;

// retries in a loop for master abort
const unsigned int RETRIES_NUM =	10000;

// SMBus spec. values in KHZ
const unsigned int SMBUS_FREQ_MIN = 10;
const unsigned int SMBUS_FREQ_MAX = 1000;
const unsigned int SMBUS_FREQ_100KHZ = 100;
const unsigned int SMBUS_FREQ_400KHZ = 400;
const unsigned int SMBUS_FREQ_1MHZ = 1000;

// SCLFRQ min/max field values
const unsigned int SCLFRQ_MIN = 10;
const unsigned int SCLFRQ_MAX = 511;

// SCLFRQ field position
#define SCLFRQ_0_TO_6		GENMASK(6, 0)
#define SCLFRQ_7_TO_8		GENMASK(8, 7)

const unsigned int SMB_NUM_OF_ADDR = 10;

#define NPCM_I2C_EVENT_START	BIT(0)
#define NPCM_I2C_EVENT_STOP	BIT(1)
#define NPCM_I2C_EVENT_ABORT	BIT(2)
#define NPCM_I2C_EVENT_WRITE	BIT(3)

#define NPCM_I2C_EVENT_READ	BIT(4)
#define NPCM_I2C_EVENT_BER	BIT(5)
#define NPCM_I2C_EVENT_NACK	BIT(6)
#define NPCM_I2C_EVENT_TO	BIT(7)

#define NPCM_I2C_EVENT_EOB	BIT(8)
#define NPCM_I2C_EVENT_STALL	BIT(9)
#define NPCM_I2C_EVENT_CB	BIT(10)
#define NPCM_I2C_EVENT_DONE	BIT(11)

#define NPCM_I2C_EVENT_READ1	BIT(12)
#define NPCM_I2C_EVENT_READ2	BIT(13)
#define NPCM_I2C_EVENT_READ3	BIT(14)
#define NPCM_I2C_EVENT_READ4	BIT(15)

#define NPCM_I2C_EVENT_NMATCH_SLV	BIT(16)
#define NPCM_I2C_EVENT_NMATCH_MSTR	BIT(17)
#define NPCM_I2C_EVENT_BER_SLV		BIT(18)

#define NPCM_I2C_EVENT_LOG(event)	(bus->event_log |= event)

// Status of one SMBus module
struct npcm_i2c {
	struct i2c_adapter	adap;
	struct device		*dev;
	unsigned char __iomem	*reg;
	spinlock_t		lock;   /* IRQ synchronization */
	struct completion	cmd_complete;
	int			irq;
	int			cmd_err;
	struct i2c_msg		*msgs;
	int			msgs_num;
	int			num;
	u32			apb_clk;
	struct i2c_bus_recovery_info rinfo;
	enum smb_state		state;
	enum smb_oper		operation;
	enum smb_mode		master_or_slave;
	enum smb_state_ind	stop_ind;
	u8			dest_addr;
	u8			*rd_buf;
	u16			rd_size;
	u16			rd_ind;
	u8			*wr_buf;
	u16			wr_size;
	u16			wr_ind;
	bool			fifo_use;

	// PEC bit mask per slave address.
	//		1: use PEC for this address,
	//		0: do not use PEC for this address
	u16			PEC_mask;
	bool			PEC_use;
	bool			read_block_use;
	u8			int_cnt;
	u32			event_log;
	u32			event_log_prev;
	u32			clk_period_us;
	unsigned long		int_time_stamp;
	unsigned long		bus_freq; // in kHz
	u32			xmits;

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	u8			own_slave_addr;
	struct i2c_client	*slave;

	// currently I2C slave IF only supports single byte operations.
	// in order to utilyze the npcm HW FIFO, the driver will ask for 16bytes
	// at a time, pack them in buffer, and then transmit them all together
	// to the FIFO and onward to the bus .
	// NACK on read will be once reached to bus->adap->quirks->max_read_len
	// sending a NACK whever the backend requests for it is not supported.

	// This module can be master and slave at the same time. separate ptrs
	// and counters:
	int			slv_rd_size;
	int			slv_rd_ind;
	int			slv_wr_size;
	int			slv_wr_ind;
	u8			slv_rd_buf[SMBUS_FIFO_SIZE];
	u8			slv_wr_buf[SMBUS_FIFO_SIZE];
#endif
};

static inline void npcm_smb_select_bank(struct npcm_i2c *bus,
					enum smb_bank bank)
{
	if (bank == SMB_BANK_0)
		iowrite8(ioread8(bus->reg + NPCM_SMBCTL3) & ~SMBCTL3_BNK_SEL,
			 bus->reg + NPCM_SMBCTL3);
	else
		iowrite8(ioread8(bus->reg + NPCM_SMBCTL3) | SMBCTL3_BNK_SEL,
			 bus->reg + NPCM_SMBCTL3);
}

static void npcm_smb_init_params(struct npcm_i2c *bus)
{
	bus->stop_ind = SMB_NO_STATUS_IND;
	bus->rd_size = 0;
	bus->wr_size = 0;
	bus->rd_ind = 0;
	bus->wr_ind = 0;
	bus->int_cnt = 0;
	bus->event_log_prev = bus->event_log;
	bus->event_log = 0;
	bus->read_block_use = false;
	bus->int_time_stamp = 0;
	bus->PEC_use = false;
	bus->PEC_mask = 0;
	if (bus->slave)
		bus->master_or_slave = SMB_SLAVE;
}

static inline void npcm_smb_wr_byte(struct npcm_i2c *bus, u8 data)
{
	iowrite8(data, bus->reg + NPCM_SMBSDA);
}

static inline void npcm_smb_rd_byte(struct npcm_i2c *bus, u8 *data)
{
	*data = ioread8(bus->reg + NPCM_SMBSDA);
}

static inline u16 npcm_smb_get_index(struct npcm_i2c *bus)
{
	u16 index = 0;

	if (bus->operation == SMB_READ_OPER)
		index = bus->rd_ind;
	else if (bus->operation == SMB_WRITE_OPER)
		index = bus->wr_ind;

	return index;
}

// quick protocol (just address):
static inline bool npcm_smb_is_quick(struct npcm_i2c *bus)
{
	if (bus->wr_size == 0 && bus->rd_size == 0)
		return true;
	return false;
}

static void npcm_smb_disable(struct npcm_i2c *bus)
{
	int i;

	// select bank 0 for SMB addresses
	npcm_smb_select_bank(bus, SMB_BANK_0);

	// Slave addresses removal
	for (i = SMB_SLAVE_ADDR1; i < SMB_NUM_OF_ADDR; i++)
		iowrite8(0, bus->reg + NPCM_SMBADDR[i]);

	npcm_smb_select_bank(bus, SMB_BANK_1);

	// Disable module.
	iowrite8(ioread8(bus->reg + NPCM_SMBCTL2) & ~SMBCTL2_ENABLE,
		 bus->reg + NPCM_SMBCTL2);

	bus->state = SMB_DISABLE;
}

static void npcm_smb_enable(struct npcm_i2c *bus)
{
	iowrite8((ioread8(bus->reg + NPCM_SMBCTL2) | SMBCTL2_ENABLE),
		 bus->reg + NPCM_SMBCTL2);

	bus->state = SMB_IDLE;
}

static bool npcm_smb_wait_for_bus_free(struct npcm_i2c *bus, bool may_sleep)
{
	int cnt = 0;
	int max_count = 2; /* wait for 2 ms */

	if (may_sleep)
		might_sleep();
	else
		max_count = max_count * 100; /* since each delay is 10 us */

	while  (ioread8(bus->reg + NPCM_SMBCST) & NPCM_SMBCST_BUSY) {
		if (cnt < max_count) {
			if (may_sleep)
				msleep_interruptible(1);
			else
				udelay(10);
			cnt++;

		} else {
			bus->cmd_err = -EAGAIN;
			return false;
		}
	}
	return true;
}

// enable\disable end of busy (EOB) interrupt
static inline void npcm_smb_eob_int(struct npcm_i2c *bus, bool enable)
{
	// Clear EO_BUSY pending bit:
	iowrite8(ioread8(bus->reg + NPCM_SMBCST3) | NPCM_SMBCST3_EO_BUSY,
		 bus->reg + NPCM_SMBCST3);

	if (enable) {
		iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) |
			 NPCM_SMBCTL1_EOBINTE)  & ~NPCM_SMBCTL1_RWS_FIELDS,
			 bus->reg + NPCM_SMBCTL1);
	} else {
		iowrite8(ioread8(bus->reg + NPCM_SMBCTL1) &
			 ~NPCM_SMBCTL1_EOBINTE & ~NPCM_SMBCTL1_RWS_FIELDS,
			 bus->reg + NPCM_SMBCTL1);
	}
}

static inline bool npcm_smb_tx_fifo_empty(struct npcm_i2c *bus)
{
	u8 tx_fifo_sts = ioread8(bus->reg + NPCM_SMBTXF_STS);

	// check if TX FIFO is not empty
	if ((tx_fifo_sts & NPCM_SMBTXF_STS_TX_BYTES) == 0)
		return false;

	// check if TX FIFO status bit is set:
	return (bool)FIELD_GET(NPCM_SMBTXF_STS_TX_THST, tx_fifo_sts);
}

static inline bool npcm_smb_rx_fifo_full(struct npcm_i2c *bus)
{
	u8 rx_fifo_sts = ioread8(bus->reg + NPCM_SMBRXF_STS);

	// check if RX FIFO is not empty:
	if ((rx_fifo_sts & NPCM_SMBRXF_STS_RX_BYTES) == 0)
		return false;

	// check if rx fifo full status is set:
	return (bool)FIELD_GET(NPCM_SMBRXF_STS_RX_THST, rx_fifo_sts);
}

static inline void npcm_smb_clear_fifo_int(struct npcm_i2c *bus)
{
	iowrite8((ioread8(bus->reg + NPCM_SMBFIF_CTS) &
			NPCM_SMBFIF_CTS_SLVRSTR) |
			NPCM_SMBFIF_CTS_RXF_TXE,
			bus->reg + NPCM_SMBFIF_CTS);
}

static inline void npcm_smb_clear_tx_fifo(struct npcm_i2c *bus)
{
	iowrite8(ioread8(bus->reg + NPCM_SMBTXF_STS) | NPCM_SMBTXF_STS_TX_THST,
		 bus->reg + NPCM_SMBTXF_STS);
}

static inline void npcm_smb_clear_rx_fifo(struct npcm_i2c *bus)
{
	iowrite8(ioread8(bus->reg + NPCM_SMBRXF_STS) | NPCM_SMBRXF_STS_RX_THST,
		 bus->reg + NPCM_SMBRXF_STS);
}

static void npcm_smb_int_enable(struct npcm_i2c *bus, bool enable)
{
	if (enable)
		iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) |
			 NPCM_SMBCTL1_INTEN) & ~NPCM_SMBCTL1_RWS_FIELDS,
			 bus->reg + NPCM_SMBCTL1);
	else
		iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) &
			 ~NPCM_SMBCTL1_INTEN) & ~NPCM_SMBCTL1_RWS_FIELDS,
			 bus->reg + NPCM_SMBCTL1);
}

static inline void npcm_smb_master_start(struct npcm_i2c *bus)
{
	NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_START);

	iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) | NPCM_SMBCTL1_START) &
		 ~(NPCM_SMBCTL1_STOP | NPCM_SMBCTL1_ACK),
		 bus->reg + NPCM_SMBCTL1);
}

static inline void npcm_smb_master_stop(struct npcm_i2c *bus)
{
	NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_STOP);

	// override HW issue: SMBus may fail to supply stop condition in Master
	// Write operation.
	// Need to delay at least 5 us from the last int, before issueing a stop
	udelay(10);

	iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) | NPCM_SMBCTL1_STOP) &
		 ~(NPCM_SMBCTL1_START | NPCM_SMBCTL1_ACK),
		 bus->reg + NPCM_SMBCTL1);

	if (bus->fifo_use) {
		npcm_smb_select_bank(bus, SMB_BANK_1);

		if (bus->operation == SMB_READ_OPER)
			npcm_smb_clear_rx_fifo(bus);
		else
			npcm_smb_clear_tx_fifo(bus);

		npcm_smb_clear_fifo_int(bus);

		iowrite8(0, bus->reg + NPCM_SMBTXF_CTL);
	}
}

static inline void npcm_smb_stall_after_start(struct npcm_i2c *bus, bool stall)
{
	if (stall)
		iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) |
			 NPCM_SMBCTL1_STASTRE)  & ~NPCM_SMBCTL1_RWS_FIELDS,
			 bus->reg + NPCM_SMBCTL1);
	else
		iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) &
			 ~NPCM_SMBCTL1_STASTRE)  & ~NPCM_SMBCTL1_RWS_FIELDS,
			 bus->reg + NPCM_SMBCTL1);
}

static inline void npcm_smb_nack(struct npcm_i2c *bus)
{
	iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) | NPCM_SMBCTL1_ACK) &
		 ~(NPCM_SMBCTL1_STOP | NPCM_SMBCTL1_START),
		 bus->reg + NPCM_SMBCTL1);
}

static int  npcm_smb_slave_enable_l(struct npcm_i2c *bus,
				    enum smb_addr addr_type, u8 addr,
				    bool enable);

static void npcm_smb_reset(struct npcm_i2c *bus)
{
	// Save NPCM_SMBCTL1 relevant bits. It is being cleared when the
	// module is disabled
	u8 smbctl1;

	smbctl1 = ioread8(bus->reg + NPCM_SMBCTL1);

	// Disable the SMB module
	iowrite8((ioread8(bus->reg + NPCM_SMBCTL2) & ~SMBCTL2_ENABLE),
		 bus->reg + NPCM_SMBCTL2);

	// Enable the SMB module
	npcm_smb_enable(bus);

	// Restore NPCM_SMBCTL1 status
	iowrite8(smbctl1 & ~NPCM_SMBCTL1_RWS_FIELDS, bus->reg + NPCM_SMBCTL1);

	// Clear BB (BUS BUSY) bit
	iowrite8(NPCM_SMBCST_BB, bus->reg + NPCM_SMBCST);

	iowrite8(0xFF, bus->reg + NPCM_SMBST);

	// Clear EOB bit
	iowrite8(NPCM_SMBCST3_EO_BUSY, bus->reg + NPCM_SMBCST3);

	// Clear all fifo bits:
	iowrite8(NPCM_SMBFIF_CTS_CLR_FIFO, bus->reg + NPCM_SMBFIF_CTS);

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	if (bus->slave) {
		npcm_smb_slave_enable_l(bus, SMB_SLAVE_ADDR1, bus->slave->addr,
					true);
	}
#endif

	bus->state = SMB_IDLE;
}

static inline bool npcm_smb_is_master(struct npcm_i2c *bus)
{
	return (bool)FIELD_GET(NPCM_SMBST_MASTER,
			       ioread8(bus->reg + NPCM_SMBST));
}

static void npcm_smb_callback(struct npcm_i2c *bus,
			      enum smb_state_ind op_status, u16 info)
{
	struct i2c_msg *msgs = bus->msgs;
	int msgs_num = bus->msgs_num;

	NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_CB);

	if (!msgs)
		return;

	if (completion_done(&bus->cmd_complete) == true)
		return;

	switch (op_status) {
	case SMB_MASTER_DONE_IND:
		bus->cmd_err = bus->msgs_num;
		fallthrough;
	case SMB_BLOCK_BYTES_ERR_IND:
		// Master transaction finished and all transmit bytes were sent
		if (bus->msgs) {
			if (msgs[0].flags & I2C_M_RD)
				msgs[0].len = info;
			else if (msgs_num == 2 &&
				 msgs[1].flags & I2C_M_RD)
				msgs[1].len = info;
		}

		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_DONE);

		if (completion_done(&bus->cmd_complete) == false)
			complete(&bus->cmd_complete);
	break;

	case SMB_NACK_IND:
		// MASTER transmit got a NACK before tx all bytes
		// info: number of transmitted bytes
		bus->cmd_err = -ENXIO;
		if (bus->master_or_slave == SMB_MASTER)
			complete(&bus->cmd_complete);

		break;
	case SMB_BUS_ERR_IND:
		// Bus error
		bus->cmd_err = -EAGAIN;
		if (bus->master_or_slave == SMB_MASTER)
			complete(&bus->cmd_complete);

		break;
	case SMB_WAKE_UP_IND:
		// SMBus wake up
		break;
	default:
		break;
	}

	bus->operation = SMB_NO_OPER;
	if (bus->slave)
		bus->master_or_slave = SMB_SLAVE;
}

static u32 npcm_smb_get_fifo_fullness(struct npcm_i2c *bus)
{
	if (bus->operation == SMB_WRITE_OPER)
		return FIELD_GET(NPCM_SMBTXF_STS_TX_BYTES,
				 ioread8(bus->reg + NPCM_SMBTXF_STS));
	else if (bus->operation == SMB_READ_OPER)
		return FIELD_GET(NPCM_SMBRXF_STS_RX_BYTES,
				 ioread8(bus->reg + NPCM_SMBRXF_STS));
	return 0;
}

static void npcm_smb_write_to_fifo_master(struct npcm_i2c *bus,
					  u16 max_bytes_to_send)
{
	// Fill the FIFO, while the FIFO is not full and there are more bytes to
	// write
	if (max_bytes_to_send == 0)
		return;
	while ((max_bytes_to_send--) && (SMBUS_FIFO_SIZE -
					 npcm_smb_get_fifo_fullness(bus))) {
		if (bus->wr_ind < bus->wr_size)
			npcm_smb_wr_byte(bus, bus->wr_buf[bus->wr_ind++]);
		else
			npcm_smb_wr_byte(bus, 0xFF);
	}
}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static void npcm_smb_write_to_fifo_slave(struct npcm_i2c *bus,
					 u16 max_bytes_to_send)
{
	// Fill the FIFO, while the FIFO is not full and there are more bytes to
	// write
	npcm_smb_clear_fifo_int(bus);
	npcm_smb_clear_tx_fifo(bus);
	iowrite8(0, bus->reg + NPCM_SMBTXF_CTL);

	if (max_bytes_to_send == 0)
		return;

	while ((max_bytes_to_send--) && (SMBUS_FIFO_SIZE -
					 npcm_smb_get_fifo_fullness(bus))) {
		if (bus->slv_wr_size > 0) {
			npcm_smb_wr_byte(bus,
					 bus->slv_wr_buf[bus->slv_wr_ind %
					 SMBUS_FIFO_SIZE]);
			bus->slv_wr_ind = (bus->slv_wr_ind + 1) %
					   SMBUS_FIFO_SIZE;
			bus->slv_wr_size--; // size indicates the # of bytes in
					    // the SW FIFO, not HW.
		} else {
			break;
		}
	}
}
#endif

// configure the FIFO before using it. If nread is -1 RX FIFO will not be
// configured. same for	nwrite
static void npcm_smb_set_fifo(struct npcm_i2c *bus, int nread, int nwrite)
{
	u8 rxf_ctl = 0;

	if (!bus->fifo_use)
		return;
	npcm_smb_select_bank(bus, SMB_BANK_1);
	npcm_smb_clear_tx_fifo(bus);
	npcm_smb_clear_rx_fifo(bus);

	// configure RX FIFO
	if (nread > 0) {
		rxf_ctl = min_t(u16, (u16)nread, (u16)SMBUS_FIFO_SIZE);

		// set LAST bit. if LAST is set enxt FIFO packet is nacked
		// regular read of less then buffer size:
		if (nread <= SMBUS_FIFO_SIZE)
			rxf_ctl |= NPCM_SMBRXF_CTL_LAST_PEC;
		// if we are about to read the first byte in blk rd mode,
		// don't NACK it. BTW, if slave return zero size HW can't NACK
		// it immidiattly, it will read extra byte and then NACK.
		if (bus->rd_ind == 0 && bus->read_block_use) {
			// set fifo to read one byte, no last:
			rxf_ctl = 1;
		}

		// set fifo size:
		iowrite8(rxf_ctl, bus->reg + NPCM_SMBRXF_CTL);
	}

	// configure TX FIFO
	if (nwrite > 0) {
		if (nwrite > SMBUS_FIFO_SIZE)
			// data to send is more then FIFO size.
			// Configure the FIFO int to be after of FIFO is cleared
			iowrite8(SMBUS_FIFO_SIZE, bus->reg + NPCM_SMBTXF_CTL);
		else
			iowrite8(nwrite, bus->reg + NPCM_SMBTXF_CTL);

		npcm_smb_clear_tx_fifo(bus);
	}
}

static void npcm_smb_read_from_fifo(struct npcm_i2c *bus, u8 bytes_in_fifo)
{
	u8 data;

	while (bytes_in_fifo--) {
		npcm_smb_rd_byte(bus, &data);

		if (bus->master_or_slave == SMB_MASTER) {
			if (bus->rd_ind < bus->rd_size)
				bus->rd_buf[bus->rd_ind++] = data;
		} else { // SMB_SLAVE:
#if IS_ENABLED(CONFIG_I2C_SLAVE)
			if (bus->slave) {
				bus->slv_rd_buf[bus->slv_rd_ind %
						SMBUS_FIFO_SIZE] = data;
				bus->slv_rd_ind++;
				if (bus->slv_rd_ind == 1 && bus->read_block_use)
					// 1st byte is length in block protocol
					bus->slv_rd_size = data +
							   (u8)bus->PEC_use +
							(u8)bus->read_block_use;
			}
#endif
		}
	}
}

static int npcm_smb_master_abort(struct npcm_i2c *bus)
{
	int ret = 0;

	NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_ABORT);

	// Only current master is allowed to issue Stop Condition
	if (npcm_smb_is_master(bus)) {
		npcm_smb_eob_int(bus, true);
		npcm_smb_master_stop(bus);

		// Clear NEGACK, STASTR and BER bits
		iowrite8(NPCM_SMBST_BER | NPCM_SMBST_NEGACK | NPCM_SMBST_STASTR,
			 bus->reg + NPCM_SMBST);
	}

	return ret;
}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static irqreturn_t npcm_i2c_bus_irq(int irq, void *dev_id);

static int  npcm_smb_slave_enable_l(struct npcm_i2c *bus,
				    enum smb_addr addr_type, u8 addr,
				    bool enable)
{
	u8 slave_addr_reg = FIELD_PREP(NPCM_SMBADDR_A, addr) |
		FIELD_PREP(NPCM_SMBADDR_SAEN, enable);

	if (addr_type == SMB_GC_ADDR) {
		iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) &
			~NPCM_SMBCTL1_GCMEN) |
			FIELD_PREP(NPCM_SMBCTL1_GCMEN, enable),
			bus->reg + NPCM_SMBCTL1);
		return 0;
	}
	if (addr_type == SMB_ARP_ADDR) {
		iowrite8((ioread8(bus->reg + NPCM_SMBCTL3) &
			~SMBCTL3_ARPMEN) |
			FIELD_PREP(SMBCTL3_ARPMEN, enable),
			bus->reg + NPCM_SMBCTL3);
		return 0;
	}
	if (addr_type >= SMB_ARP_ADDR)
		return -EFAULT;

	// select bank 0 for address 3 to 10
	if (addr_type > SMB_SLAVE_ADDR2)
		npcm_smb_select_bank(bus, SMB_BANK_0);

	// Set and enable the address
	iowrite8(slave_addr_reg, bus->reg + NPCM_SMBADDR[(int)addr_type]);

	// enable interrupt on slave match:
	iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) | NPCM_SMBCTL1_NMINTE) &
		 ~NPCM_SMBCTL1_RWS_FIELDS, bus->reg + NPCM_SMBCTL1);

	if (addr_type > SMB_SLAVE_ADDR2)
		npcm_smb_select_bank(bus, SMB_BANK_1);
	return 0;
}

static u8 npcm_smb_get_slave_addr(struct npcm_i2c *bus,
				  enum smb_addr addr_type)
{
	u8 slave_add;

	// select bank 0 for address 3 to 10
	if (addr_type > SMB_SLAVE_ADDR2)
		npcm_smb_select_bank(bus, SMB_BANK_0);

	slave_add = ioread8(bus->reg + NPCM_SMBADDR[(int)addr_type]);

	if (addr_type > SMB_SLAVE_ADDR2)
		npcm_smb_select_bank(bus, SMB_BANK_1);

	return  slave_add;
}

static int  npcm_smb_remove_slave_addr(struct npcm_i2c *bus, u8 slave_add)
{
	int i;

	slave_add |= 0x80; //Set the enable bit

	npcm_smb_select_bank(bus, SMB_BANK_0);

	for (i = SMB_SLAVE_ADDR1; i < SMB_NUM_OF_ADDR; i++) {
		if (ioread8(bus->reg + NPCM_SMBADDR[i]) == slave_add)
			iowrite8(0, bus->reg + NPCM_SMBADDR[i]);
	}

	npcm_smb_select_bank(bus, SMB_BANK_1);

	return 0;
}

static int npcm_i2c_slave_get_wr_buf(struct npcm_i2c *bus)
{
	u8 value = 0;
	int ret = bus->slv_wr_ind;
	int i;

	// fill a cyclic buffer
	for (i = 0; i < SMBUS_FIFO_SIZE; i++) {
		if (bus->slv_wr_size >= SMBUS_FIFO_SIZE)
			break;
		i2c_slave_event(bus->slave, I2C_SLAVE_READ_REQUESTED, &value);
		bus->slv_wr_buf[(bus->slv_wr_ind + bus->slv_wr_size) %
				 SMBUS_FIFO_SIZE] = value;
		bus->slv_wr_size++;
		i2c_slave_event(bus->slave, I2C_SLAVE_READ_PROCESSED, &value);
	}
	return SMBUS_FIFO_SIZE - ret;
}

static void npcm_i2c_slave_send_rd_buf(struct npcm_i2c *bus)
{
	int i;

	for (i = 0; i < bus->slv_rd_ind; i++)
		i2c_slave_event(bus->slave, I2C_SLAVE_WRITE_RECEIVED,
				&bus->slv_rd_buf[i]);

	// once we send bytes up, need to reset the counter of the wr buf
	// got data from master (new offset in device), ignore wr fifo:
	if (bus->slv_rd_ind) {
		bus->slv_wr_size = 0;
		bus->slv_wr_ind = 0;
	}

	bus->slv_rd_ind = 0;
	bus->slv_rd_size = bus->adap.quirks->max_read_len;

	npcm_smb_clear_fifo_int(bus);
	npcm_smb_clear_rx_fifo(bus);
}

static bool npcm_smb_slave_receive(struct npcm_i2c *bus, u16 nread,
				   u8 *read_data)
{
	bus->state = SMB_OPER_STARTED;
	bus->operation	 = SMB_READ_OPER;
	bus->slv_rd_size = nread;
	bus->slv_rd_ind	= 0;

	iowrite8(0, bus->reg + NPCM_SMBTXF_CTL);
	iowrite8(SMBUS_FIFO_SIZE, bus->reg + NPCM_SMBRXF_CTL);

	npcm_smb_clear_tx_fifo(bus);
	npcm_smb_clear_rx_fifo(bus);

	return true;
}

static bool npcm_smb_slave_xmit(struct npcm_i2c *bus, u16 nwrite,
				u8 *write_data)
{
	if (nwrite == 0)
		return false;

	bus->state = SMB_OPER_STARTED;
	bus->operation = SMB_WRITE_OPER;

	// get the next buffer
	npcm_i2c_slave_get_wr_buf(bus);

	if (nwrite > 0)
		npcm_smb_write_to_fifo_slave(bus, nwrite);

	return true;
}

// currently slave IF only supports single byte operations.
// in order to utilyze the npcm HW FIFO, the driver will ask for 16 bytes
// at a time, pack them in buffer, and then transmit them all together
// to the FIFO and onward to the bus.
// NACK on read will be once reached to bus->adap->quirks->max_read_len.
// sending a NACK wherever the backend requests for it is not supported.
// the next two functions allow reading to local buffer before writing it all
// to the HW FIFO.
// ret val: number of bytes read form the IF:

static int npcm_i2c_slave_wr_buf_sync(struct npcm_i2c *bus)
{
	int left_in_fifo = FIELD_GET(NPCM_SMBTXF_STS_TX_BYTES,
			ioread8(bus->reg + NPCM_SMBTXF_STS));

	if (left_in_fifo >= SMBUS_FIFO_SIZE)
		return left_in_fifo;

	if (bus->slv_wr_size >= SMBUS_FIFO_SIZE)
		return left_in_fifo; // fifo already full

	// update the wr fifo ind, back to the untransmitted bytes:
	bus->slv_wr_ind = bus->slv_wr_ind - left_in_fifo;
	bus->slv_wr_size = bus->slv_wr_size + left_in_fifo;

	if (bus->slv_wr_ind < 0)
		bus->slv_wr_ind += SMBUS_FIFO_SIZE;

	return left_in_fifo;
}

static void npcm_i2c_slave_rd_wr(struct npcm_i2c *bus)
{
	if (FIELD_GET(NPCM_SMBST_XMIT, ioread8(bus->reg + NPCM_SMBST))) {
		// Slave got an address match with direction bit 1 so
		// it should transmit data
		// Write till the master will NACK
		bus->operation = SMB_WRITE_OPER;
		npcm_smb_slave_xmit(bus,
				    bus->adap.quirks->max_write_len,
				    bus->slv_wr_buf);
	} else {
		// Slave got an address match with direction bit 0
		// so it should receive data.
		// this module does not support saying no to bytes.
		// it will always ACK.
		bus->operation = SMB_READ_OPER;
		npcm_smb_read_from_fifo(bus, npcm_smb_get_fifo_fullness(bus));
		bus->stop_ind = SMB_SLAVE_RCV_IND;
		npcm_i2c_slave_send_rd_buf(bus);
		npcm_smb_slave_receive(bus,
				       bus->adap.quirks->max_read_len,
				       bus->slv_rd_buf);
	}
}

static irqreturn_t npcm_smb_int_slave_handler(struct npcm_i2c *bus)
{
	irqreturn_t ret = IRQ_NONE;
	u8 smbst = ioread8(bus->reg + NPCM_SMBST);
	// Slave: A NACK has occurred
	if (FIELD_GET(NPCM_SMBST_NEGACK, smbst)) {
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_NACK);
		bus->stop_ind = SMB_NACK_IND;
		npcm_i2c_slave_wr_buf_sync(bus);
		if (bus->fifo_use)
			// clear the FIFO
			iowrite8(NPCM_SMBFIF_CTS_CLR_FIFO,
				 bus->reg + NPCM_SMBFIF_CTS);

		// In slave write, NACK is OK, otherwise it is a problem
		bus->stop_ind = SMB_NO_STATUS_IND;
		bus->operation = SMB_NO_OPER;
		bus->own_slave_addr = 0xFF;

		// Slave has to wait for SMB_STOP to decide this is the end
		// of the transaction.
		// Therefore transaction is not yet considered as done
		iowrite8(NPCM_SMBST_NEGACK, bus->reg + NPCM_SMBST);

		ret = IRQ_HANDLED;
	}

	// Slave mode: a Bus Error (BER) has been identified
	if (FIELD_GET(NPCM_SMBST_BER, smbst)) {
		// Check whether bus arbitration or Start or Stop during data
		// xfer bus arbitration problem should not result in recovery
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_BER_SLV);
		bus->stop_ind = SMB_BUS_ERR_IND;

		// wait for bus busy before clear fifo
		iowrite8(NPCM_SMBFIF_CTS_CLR_FIFO, bus->reg + NPCM_SMBFIF_CTS);

		bus->state = SMB_IDLE;

		// in BER case we might get 2 interrupts: one for slave one for
		// master ( for a channel which is master\slave switching)
		if (completion_done(&bus->cmd_complete) == false) {
			bus->cmd_err = -EIO;
			complete(&bus->cmd_complete);
		}
		bus->own_slave_addr = 0xFF;
		iowrite8(NPCM_SMBST_BER, bus->reg + NPCM_SMBST);
		ret =  IRQ_HANDLED;
	}

	// A Slave Stop Condition has been identified
	if (FIELD_GET(NPCM_SMBST_SLVSTP, smbst)) {
		int bytes_in_fifo = npcm_smb_get_fifo_fullness(bus);

		bus->stop_ind = SMB_SLAVE_DONE_IND;

		if (bus->operation == SMB_READ_OPER) {
			npcm_smb_read_from_fifo(bus, bytes_in_fifo);

			// Slave done transmitting or receiving
			// if the buffer is empty nothing will be sent
		}

		// Slave done transmitting or receiving
		// if the buffer is empty nothing will be sent
		npcm_i2c_slave_send_rd_buf(bus);

		bus->stop_ind = SMB_NO_STATUS_IND;

		// Note, just because we got here, it doesn't mean we through
		// away the wr buffer.
		// we keep it until the next received offset.
		bus->operation = SMB_NO_OPER;
		bus->int_cnt = 0;
		bus->event_log_prev = bus->event_log;
		bus->event_log = 0;
		bus->own_slave_addr = 0xFF;

		i2c_slave_event(bus->slave, I2C_SLAVE_STOP, 0);

		iowrite8(NPCM_SMBST_SLVSTP, bus->reg + NPCM_SMBST);

		if (bus->fifo_use) {
			npcm_smb_clear_fifo_int(bus);
			npcm_smb_clear_rx_fifo(bus);
			npcm_smb_clear_tx_fifo(bus);

			iowrite8(NPCM_SMBFIF_CTS_CLR_FIFO,
				 bus->reg + NPCM_SMBFIF_CTS);
		}

		bus->state = SMB_IDLE;
		ret =  IRQ_HANDLED;
	}

	// restart condition occurred and Rx-FIFO was not empty
	if (bus->fifo_use && FIELD_GET(NPCM_SMBFIF_CTS_SLVRSTR,
				       ioread8(bus->reg + NPCM_SMBFIF_CTS))) {
		bus->stop_ind = SMB_SLAVE_RESTART_IND;

		bus->master_or_slave = SMB_SLAVE;

		if (bus->operation == SMB_READ_OPER)
			npcm_smb_read_from_fifo(bus,
						npcm_smb_get_fifo_fullness(bus)
						);

		bus->operation = SMB_WRITE_OPER;

		iowrite8(0, bus->reg + NPCM_SMBRXF_CTL);

		iowrite8(NPCM_SMBFIF_CTS_CLR_FIFO | NPCM_SMBFIF_CTS_SLVRSTR |
			 NPCM_SMBFIF_CTS_RXF_TXE, bus->reg + NPCM_SMBFIF_CTS);

		npcm_i2c_slave_rd_wr(bus);

		ret =  IRQ_HANDLED;
	}

	// A Slave Address Match has been identified
	if (FIELD_GET(NPCM_SMBST_NMATCH, smbst)) {
		u8 info = 0;

		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_NMATCH_SLV);
		// Address match automatically implies slave mode
		bus->master_or_slave = SMB_SLAVE;

		npcm_smb_clear_fifo_int(bus);
		npcm_smb_clear_rx_fifo(bus);
		npcm_smb_clear_tx_fifo(bus);
		iowrite8(0, bus->reg + NPCM_SMBTXF_CTL);
		iowrite8(SMBUS_FIFO_SIZE, bus->reg + NPCM_SMBRXF_CTL);

		if (FIELD_GET(NPCM_SMBST_XMIT, smbst)) {
			bus->operation = SMB_WRITE_OPER;
		} else {
			i2c_slave_event(bus->slave, I2C_SLAVE_WRITE_REQUESTED,
					&info);
			bus->operation = SMB_READ_OPER;
		}

		if (bus->own_slave_addr == 0xFF) { // unknown address
			// Check which type of address match
			if (FIELD_GET(NPCM_SMBCST_MATCH,
				      ioread8(bus->reg + NPCM_SMBCST))) {
				u16 addr;
				enum smb_addr eaddr;

				addr = ((ioread8(bus->reg + NPCM_SMBCST3) &
					 0x7) << 7) |
					(ioread8(bus->reg + NPCM_SMBCST2) &
					 0x7F);

				info = ffs(addr);
				eaddr = (enum smb_addr)info;

				addr = FIELD_GET(NPCM_SMBADDR_A,
						 npcm_smb_get_slave_addr(bus,
									 eaddr)
						);
				bus->own_slave_addr = addr;

				if (bus->PEC_mask & BIT(info))
					bus->PEC_use = true;
				else
					bus->PEC_use = false;
			} else {
				if (FIELD_GET(NPCM_SMBCST_GCMATCH,
					      ioread8(bus->reg + NPCM_SMBCST)))
					bus->own_slave_addr = 0;
				if (FIELD_GET(NPCM_SMBCST_ARPMATCH,
					      ioread8(bus->reg + NPCM_SMBCST)))
					bus->own_slave_addr = 0x61;
			}
		} else {
			//  Slave match can happen in two options:
			//  1. Start, SA, read	(slave read without further ado)
			//  2. Start, SA, read, data, restart, SA, read,  ...
			//     (slave read in fragmented mode)
			//  3. Start, SA, write, data, restart, SA, read, ..
			//     (regular write-read mode)
			if ((bus->state == SMB_OPER_STARTED &&
			     bus->operation == SMB_READ_OPER &&
			     bus->stop_ind == SMB_SLAVE_XMIT_IND) ||
			     bus->stop_ind == SMB_SLAVE_RCV_IND) {
				// slave transmit after slave receive w/o Slave
				// Stop implies repeated start
				bus->stop_ind = SMB_SLAVE_RESTART_IND;
			}
		}

		if (FIELD_GET(NPCM_SMBST_XMIT, smbst))
			bus->stop_ind = SMB_SLAVE_XMIT_IND;
		else
			bus->stop_ind = SMB_SLAVE_RCV_IND;

		bus->state = SMB_SLAVE_MATCH;

		npcm_i2c_slave_rd_wr(bus);

		iowrite8(NPCM_SMBST_NMATCH, bus->reg + NPCM_SMBST);
		ret =  IRQ_HANDLED;
	}

	// Slave SDA status is set - transmit or receive, slave
	if (FIELD_GET(NPCM_SMBST_SDAST, smbst) ||
	    (bus->fifo_use   &&
	    (npcm_smb_tx_fifo_empty(bus) || npcm_smb_rx_fifo_full(bus)))) {
		npcm_i2c_slave_rd_wr(bus);

		iowrite8(NPCM_SMBST_SDAST, bus->reg + NPCM_SMBST);

		ret =  IRQ_HANDLED;
	} //SDAST

	return ret;
}

static int  npcm_i2c_reg_slave(struct i2c_client *client)
{
	unsigned long lock_flags;
	struct npcm_i2c *bus = i2c_get_adapdata(client->adapter);

	bus->slave = client;

	if (!bus->slave)
		return -EINVAL;

	if (client->flags & I2C_CLIENT_TEN)
		return -EAFNOSUPPORT;

	spin_lock_irqsave(&bus->lock, lock_flags);

	npcm_smb_init_params(bus);
	bus->slv_rd_size = 0;
	bus->slv_wr_size = 0;
	bus->slv_rd_ind = 0;
	bus->slv_wr_ind = 0;
	if (client->flags & I2C_CLIENT_PEC)
		bus->PEC_use = true;

	dev_info(bus->dev, "I2C%d register slave SA=0x%x, PEC=%d\n", bus->num,
		 client->addr, bus->PEC_use);

	npcm_smb_slave_enable_l(bus, SMB_SLAVE_ADDR1, client->addr, true);

	npcm_smb_clear_fifo_int(bus);
	npcm_smb_clear_rx_fifo(bus);
	npcm_smb_clear_tx_fifo(bus);

	spin_unlock_irqrestore(&bus->lock, lock_flags);

	return 0;
}

static int  npcm_i2c_unreg_slave(struct i2c_client *client)
{
	struct npcm_i2c *bus = client->adapter->algo_data;
	unsigned long lock_flags;

	spin_lock_irqsave(&bus->lock, lock_flags);
	if (!bus->slave) {
		spin_unlock_irqrestore(&bus->lock, lock_flags);
		return -EINVAL;
	}

	npcm_smb_remove_slave_addr(bus, client->addr);

	bus->slave = NULL;
	spin_unlock_irqrestore(&bus->lock, lock_flags);

	return 0;
}
#endif // CONFIG_I2C_SLAVE

static void npcm_smb_master_fifo_read(struct npcm_i2c *bus)
{
	int rcount;
	int fifo_bytes;
	enum smb_state_ind ind = SMB_MASTER_DONE_IND;

	fifo_bytes = npcm_smb_get_fifo_fullness(bus);

	rcount = bus->rd_size - bus->rd_ind;

	// In order not to change the RX_TRH during transaction (we found that
	// this might be problematic if it takes too much time to read the FIFO)
	//  we read the data in the following way. If the number of bytes to
	// read == FIFO Size + C (where C < FIFO Size)then first read C bytes
	// and in the next int we read rest of the data.
	if (rcount < (2 * SMBUS_FIFO_SIZE) && rcount > SMBUS_FIFO_SIZE)
		fifo_bytes = (u8)(rcount - SMBUS_FIFO_SIZE);

	if ((rcount - fifo_bytes) <= 0) {
		// last bytes are about to be read - end of transaction.
		// Stop should be set before reading last byte.
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_READ4);

		bus->state = SMB_STOP_PENDING;
		bus->stop_ind = ind;

		npcm_smb_eob_int(bus, true);
		npcm_smb_master_stop(bus);
		npcm_smb_read_from_fifo(bus, fifo_bytes);
	} else {
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_READ3);
		npcm_smb_read_from_fifo(bus, fifo_bytes);
		rcount = bus->rd_size - bus->rd_ind;
		npcm_smb_set_fifo(bus, rcount, -1);
	}
}

static void npcm_smb_int_master_handler_write(struct npcm_i2c *bus)
{
	u16 wcount;

	NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_WRITE);
	if (bus->fifo_use)
		npcm_smb_clear_tx_fifo(bus); // clear the TX fifo status bit

	// Master write operation - last byte handling
	if (bus->wr_ind == bus->wr_size) {
		if (bus->fifo_use && npcm_smb_get_fifo_fullness(bus) > 0)
			// No more bytes to send (to add to the FIFO),
			// however the FIFO is not empty yet. It is
			// still in the middle of tx. Currently there's nothing
			// to do except for waiting to the end of the tx.
			// We will get an int when the FIFO will get empty.
			return;

		if (bus->rd_size == 0) {
			// all bytes have been written, in a pure wr operation
			npcm_smb_eob_int(bus, true);

			bus->state = SMB_STOP_PENDING;
			bus->stop_ind = SMB_MASTER_DONE_IND;

			npcm_smb_master_stop(bus);
			// Clear SDA Status bit (by writing dummy byte)
			npcm_smb_wr_byte(bus, 0xFF);

		} else {
			// last write-byte written on previous int - need to
			// restart & send slave address
			npcm_smb_set_fifo(bus, bus->rd_size, -1);

			// Generate repeated start upon next write to SDA
			npcm_smb_master_start(bus);

			if (bus->rd_size == 1)
				// Receiving one byte only - stall after
				// successful completion of send
				// address byte. If we NACK here,
				// and slave doesn't ACK the address, we
				// might unintentionally NACK the next
				// multi-byte read
				npcm_smb_stall_after_start(bus, true);

			// Next int will occur on read
			bus->operation = SMB_READ_OPER;

			// send the slave address in read direction
			npcm_smb_wr_byte(bus, bus->dest_addr | 0x1);
		}
	} else {
		// write next byte not last byte and not slave address
		if (!bus->fifo_use || bus->wr_size == 1) {
			npcm_smb_wr_byte(bus, bus->wr_buf[bus->wr_ind++]);
		} else {
			wcount = bus->wr_size - bus->wr_ind;
			npcm_smb_set_fifo(bus, -1, wcount);
			npcm_smb_write_to_fifo_master(bus, wcount);
		}
	}
}

static void npcm_smb_int_master_handler_read(struct npcm_i2c *bus)
{
	u16 block_extra_bytes_size;
	u8 data;

	// Master read operation (pure read or following a write operation).
	NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_READ);

	// added bytes to the packet:
	block_extra_bytes_size = (u8)bus->read_block_use + (u8)bus->PEC_use;

	// Perform master read, distinguishing between last byte and the rest of
	// the bytes. The last byte should be read when the clock is stopped
	if (bus->rd_ind == 0) { //first byte handling:
		// in block protocol first byte is the size
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_READ1);
		if (bus->read_block_use) {
			// first byte in block protocol is the size:
			npcm_smb_rd_byte(bus, &data);

			// if slave returned illegal size. read up to 32 bytes.
			if (data >= I2C_SMBUS_BLOCK_MAX)
				data = I2C_SMBUS_BLOCK_MAX;

			// is data is 0 -> not supported. read at least one byte
			if (data == 0)
				data = 1;

			bus->rd_size = data + block_extra_bytes_size;

			bus->rd_buf[bus->rd_ind++] = data;

			// clear RX FIFO interrupt status:
			if (bus->fifo_use) {
				iowrite8(NPCM_SMBFIF_CTS_RXF_TXE |
					 ioread8(bus->reg + NPCM_SMBFIF_CTS),
					 bus->reg + NPCM_SMBFIF_CTS);
			}

			npcm_smb_set_fifo(bus, (bus->rd_size - 1), -1);
			npcm_smb_stall_after_start(bus, false);
		} else {
			npcm_smb_clear_tx_fifo(bus);
			npcm_smb_master_fifo_read(bus);
		}
	} else {
		if (bus->rd_size == block_extra_bytes_size &&
		    bus->read_block_use) {
			bus->state = SMB_STOP_PENDING;
			bus->stop_ind = SMB_BLOCK_BYTES_ERR_IND;
			bus->cmd_err = -EIO;
			npcm_smb_eob_int(bus, true);
			npcm_smb_master_stop(bus);
			npcm_smb_read_from_fifo(bus,
						npcm_smb_get_fifo_fullness(bus)
						);
		} else {
			NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_READ2);
			npcm_smb_master_fifo_read(bus);
		}
	}
}

static irqreturn_t npcm_smb_int_master_handler(struct npcm_i2c *bus)
{
	irqreturn_t ret = IRQ_NONE;
	u8 fif_cts;

	if (FIELD_GET(NPCM_SMBST_NMATCH, ioread8(bus->reg + NPCM_SMBST))) {
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_NMATCH_MSTR);
		iowrite8(NPCM_SMBST_NMATCH, bus->reg + NPCM_SMBST);
		npcm_smb_nack(bus);
		bus->stop_ind = SMB_BUS_ERR_IND;
		npcm_smb_callback(bus, bus->stop_ind, npcm_smb_get_index(bus));

		return IRQ_HANDLED;
	}
	// A NACK has occurred
	if (FIELD_GET(NPCM_SMBST_NEGACK, ioread8(bus->reg + NPCM_SMBST))) {
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_NACK);
		if (bus->fifo_use) {
			// if there are still untransmitted bytes in TX FIFO
			// reduce them from wr_ind
			if (bus->operation == SMB_WRITE_OPER)
				bus->wr_ind -= npcm_smb_get_fifo_fullness(bus);

			// clear the FIFO
			iowrite8(NPCM_SMBFIF_CTS_CLR_FIFO,
				 bus->reg + NPCM_SMBFIF_CTS);
		}

		// In master write operation, NACK is a problem
		// number of bytes sent to master less than required
		bus->stop_ind = SMB_NACK_IND;
		// Only current master is allowed to issue Stop Condition
		if (npcm_smb_is_master(bus)) {
			// stopping in the middle, not waiting for ints anymore
			npcm_smb_eob_int(bus,  false);

			npcm_smb_master_stop(bus);

			// Clear NEGACK, STASTR and BER bits
			// In Master mode, NEGACK should be cleared only after
			// generating STOP.
			// In such case, the bus is released from stall only
			// after the software clears NEGACK bit.
			// Then a Stop condition is sent.
			iowrite8(NPCM_SMBST_BER | NPCM_SMBST_NEGACK |
				 NPCM_SMBST_STASTR, bus->reg + NPCM_SMBST);

			npcm_smb_wait_for_bus_free(bus, false);
		}
		bus->state = SMB_IDLE;

		// In Master mode, NACK should be cleared only after
		// generating STOP.
		// In such case, the bus is released from stall only after the
		// software clears NACK bit.
		// Then a Stop condition is sent.
		npcm_smb_callback(bus, bus->stop_ind, bus->wr_ind);
		return IRQ_HANDLED;
	}

	// Master mode: a Bus Error has been identified
	if (FIELD_GET(NPCM_SMBST_BER, ioread8(bus->reg + NPCM_SMBST))) {
		// Check whether bus arbitration or Start or Stop during data
		// xfer bus arbitration problem should not result in recovery
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_BER);
		bus->stop_ind = SMB_BUS_ERR_IND;
		if (npcm_smb_is_master(bus)) {
			npcm_smb_master_abort(bus);
		} else {
			// Clear NEGACK, STASTR and BER bits
			iowrite8(NPCM_SMBST_BER | NPCM_SMBST_NEGACK |
				 NPCM_SMBST_STASTR, bus->reg + NPCM_SMBST);

			// Clear BB (BUS BUSY) bit
			iowrite8(NPCM_SMBCST_BB, bus->reg + NPCM_SMBCST);

			bus->cmd_err = -EAGAIN;
			npcm_smb_callback(bus, bus->stop_ind,
					  npcm_smb_get_index(bus));
		}
		bus->state = SMB_IDLE;
		ret =  IRQ_HANDLED;
		return ret;
	}

	// A Master End of Busy (meaning Stop Condition happened)
	// End of Busy int is on and End of Busy is set
	if ((FIELD_GET(NPCM_SMBCTL1_EOBINTE,
		       ioread8(bus->reg + NPCM_SMBCTL1)) == 1) &&
	    (FIELD_GET(NPCM_SMBCST3_EO_BUSY,
		       ioread8(bus->reg + NPCM_SMBCST3)))) {
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_EOB);
		npcm_smb_eob_int(bus, false);
		bus->state = SMB_IDLE;
		npcm_smb_callback(bus, bus->stop_ind, bus->rd_ind);
		return IRQ_HANDLED;
	}

	// Address sent and requested stall occurred (Master mode)
	if (FIELD_GET(NPCM_SMBST_STASTR, ioread8(bus->reg + NPCM_SMBST))) {
		NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_STALL);

		if (npcm_smb_is_quick(bus)) {
			bus->state = SMB_STOP_PENDING;
			bus->stop_ind = SMB_MASTER_DONE_IND;
			npcm_smb_eob_int(bus, true);
			npcm_smb_master_stop(bus);

		} else if ((bus->rd_size == 1) && !bus->read_block_use) {
			// Receiving one byte only - set NACK after ensuring
			// slave ACKed the address byte
			npcm_smb_nack(bus);
		}

		// Reset stall-after-address-byte
		npcm_smb_stall_after_start(bus, false);

		// Clear stall only after setting STOP
		iowrite8(NPCM_SMBST_STASTR, bus->reg + NPCM_SMBST);

		ret =  IRQ_HANDLED;
	}

	// SDA status is set - TX or RX, master
	if (FIELD_GET(NPCM_SMBST_SDAST, ioread8(bus->reg + NPCM_SMBST)) ||
	    (bus->fifo_use &&
	    (npcm_smb_tx_fifo_empty(bus) || npcm_smb_rx_fifo_full(bus)))) {
		// Status Bit is cleared by writing to or reading from SDA
		// (depending on current direction)

		// Send address:
		if (bus->state == SMB_IDLE) {
			if (npcm_smb_is_master(bus)) {
				bus->stop_ind = SMB_WAKE_UP_IND;

				// test stall on start
				if (npcm_smb_is_quick(bus) ||
				    bus->read_block_use)
					// Need to stall after successful
					// completion of sending address byte
					npcm_smb_stall_after_start(bus, true);
				else
					npcm_smb_stall_after_start(bus, false);

				// Receiving one byte only - stall after
				// successful completion of sending address byte
				// If we NACK here, and slave doesn't ACK the
				// address, we might unintentionally NACK
				// the next multi-byte read
				if (bus->wr_size == 0 && bus->rd_size == 1)
					npcm_smb_stall_after_start(bus, true);

				// Initiate SMBus master transaction
				// Generate a Start condition on the SMBus

				// select bank 1 for FIFO regs
				npcm_smb_select_bank(bus, SMB_BANK_1);

				fif_cts = ioread8(bus->reg + NPCM_SMBFIF_CTS);

				// clear FIFO and relevant status bits.
				iowrite8((fif_cts & ~NPCM_SMBFIF_CTS_SLVRSTR)
					| NPCM_SMBFIF_CTS_CLR_FIFO,
					 bus->reg + NPCM_SMBFIF_CTS);

				// and enable it
				iowrite8((fif_cts & ~NPCM_SMBFIF_CTS_SLVRSTR)
					| NPCM_SMBFIF_CTS_RXF_TXE,
					 bus->reg + NPCM_SMBFIF_CTS);

				// Configure the FIFO threshold
				// according to the needed # of bytes to read.
				// Note: due to HW limitation can't config the
				// rx fifo before
				// got and ACK on the restart. LAST bit will not
				// be reset unless RX completed.
				// It will stay set on the next tx.
				if (bus->wr_size)
					npcm_smb_set_fifo(bus, -1,
							  bus->wr_size);
				else
					npcm_smb_set_fifo(bus, bus->rd_size,
							  -1);

				bus->state = SMB_OPER_STARTED;

				if (npcm_smb_is_quick(bus) || bus->wr_size)
					npcm_smb_wr_byte(bus, bus->dest_addr);
				else
					npcm_smb_wr_byte(bus, bus->dest_addr |
							      0x01);
			}

			return IRQ_HANDLED;
		// SDA status is set - transmit or receive: Handle master mode
		} else {
			if ((NPCM_SMBST_XMIT &
			     ioread8(bus->reg + NPCM_SMBST)) == 0) {
				bus->operation = SMB_READ_OPER;
				npcm_smb_int_master_handler_read(bus);
			} else {
				bus->operation = SMB_WRITE_OPER;
				npcm_smb_int_master_handler_write(bus);
			}
		}
		ret =  IRQ_HANDLED;
	}

	return ret;
}

static int npcm_smb_get_SCL(struct i2c_adapter *_adap)
{
	unsigned int ret = 0;
	struct npcm_i2c *bus = container_of(_adap, struct npcm_i2c, adap);
	u32 offset = 0;

	offset = 0;
	ret = FIELD_GET(SMBCTL3_SCL_LVL, ioread32(bus->reg + NPCM_SMBCTL3));

	pr_debug("i2c%d get SCL 0x%08X\n", bus->num, ret);

	return (ret >> (offset)) & 0x01;
}

static int npcm_smb_get_SDA(struct i2c_adapter *_adap)
{
	unsigned int ret = 0;
	struct npcm_i2c *bus = container_of(_adap, struct npcm_i2c, adap);
	u32 offset = 0;

	offset = 0;
	ret = FIELD_GET(SMBCTL3_SDA_LVL, ioread32(bus->reg + NPCM_SMBCTL3));

	pr_debug("i2c%d get SDA 0x%08X\n", bus->num, ret);

	return (ret >> (offset)) & 0x01;
}

// recovery using TGCLK functionality of the module
static int npcm_smb_recovery_tgclk(struct i2c_adapter *_adap)
{
	int  iter = 27;	  // Allow 3 bytes to be sent by the Slave
	int  retries = 0;
	bool done = false;
	int  status = -(ENOTRECOVERABLE);
	u8   fif_cts;
	struct npcm_i2c *bus = container_of(_adap, struct npcm_i2c, adap);

	dev_dbg(bus->dev, "TGCLK recovery bus%d\n", bus->num);

	if ((npcm_smb_get_SDA(_adap) == 1) && (npcm_smb_get_SCL(_adap) == 1)) {
		dev_dbg(bus->dev, "TGCLK recovery bus%d: skipped bus not stuck",
			bus->num);
		npcm_smb_reset(bus);
		return status;
	}

	// Disable int
	npcm_smb_int_enable(bus, false);

	npcm_smb_disable(bus);
	npcm_smb_enable(bus);
	iowrite8(NPCM_SMBCST_BB, bus->reg + NPCM_SMBCST);
	npcm_smb_clear_tx_fifo(bus);
	npcm_smb_clear_rx_fifo(bus);
	iowrite8(0, bus->reg + NPCM_SMBRXF_CTL);
	iowrite8(0, bus->reg + NPCM_SMBTXF_CTL);
	npcm_smb_stall_after_start(bus, false);

	// select bank 1 for FIFO regs
	npcm_smb_select_bank(bus, SMB_BANK_1);

	fif_cts = ioread8(bus->reg + NPCM_SMBFIF_CTS);

	// clear FIFO and relevant status bits.
	iowrite8((fif_cts & ~NPCM_SMBFIF_CTS_SLVRSTR) |
		  NPCM_SMBFIF_CTS_CLR_FIFO,
		  bus->reg + NPCM_SMBFIF_CTS);

	npcm_smb_set_fifo(bus, -1, 0);

	// Check If the SDA line is active (low)
	if (npcm_smb_get_SDA(_adap) == 0) {
		// Repeat the following sequence until SDA is released
		do {
			// Issue a single SCL cycle
			iowrite8(NPCM_SMBCST_TGSCL, bus->reg + NPCM_SMBCST);
			retries = 10;
			while (retries != 0 &&
			       FIELD_GET(NPCM_SMBCST_TGSCL,
					 ioread8(bus->reg + NPCM_SMBCST))) {
				udelay(20);
				retries--;
			}

			// tgclk failed to toggle
			if (retries == 0)
				dev_dbg(bus->dev, "\t toggle timeout\n");
			// If SDA line is inactive (high), stop
			if (npcm_smb_get_SDA(_adap))
				done = true;
		} while ((!done) && (--iter != 0));

		// If SDA line is released: send start-addr-stop, to re-sync.
		if (done) {
			npcm_smb_master_start(bus);

			// Wait until START condition is sent, or RETRIES_NUM
			retries = RETRIES_NUM;
			while (retries && !npcm_smb_is_master(bus)) {
				udelay(20);
				retries--;
			}

			// If START condition was sent
			if (retries > 0) {
				// Send an address byte in write direction:
				npcm_smb_wr_byte(bus, bus->dest_addr);
				udelay(200);
				npcm_smb_master_stop(bus);
				udelay(200);
				status = 0;
			}
		}
	}

	// if bus is still stuck: total reset: set SCL low for 35ms:
	if (unlikely(npcm_smb_get_SDA(_adap) == 0)) {
		// Generate a START, to synchronize Master and Slave
		npcm_smb_master_start(bus);

		// Wait until START condition is sent, or RETRIES_NUM
		retries = RETRIES_NUM;
		while (retries && !npcm_smb_is_master(bus))
			retries--;

		// set SCL low for a long time (note: this is unlikely)
		usleep_range(25000, 35000);
		npcm_smb_master_stop(bus);
		udelay(200);
		status = 0;
	}

	dev_dbg(bus->dev, "TGCLK done, iter = %d, done = %d, retries = %d\n",
		27 - iter, done, retries);
	// Enable SMB int and New Address Match int source
	iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) | NPCM_SMBCTL1_NMINTE) &
		 ~NPCM_SMBCTL1_RWS_FIELDS,
		 bus->reg + NPCM_SMBCTL1);
	npcm_smb_reset(bus);
	npcm_smb_int_enable(bus, true);
	return status;
}

// recovery using bit banging functionality of the module
static int npcm_smb_recovery_init(struct i2c_adapter *_adap)
{
	struct npcm_i2c *bus = container_of(_adap, struct npcm_i2c, adap);
	struct i2c_bus_recovery_info *rinfo = &bus->rinfo;

	rinfo->recover_bus = npcm_smb_recovery_tgclk;
	rinfo->prepare_recovery = NULL;
	rinfo->unprepare_recovery = NULL;
	rinfo->set_scl = NULL;
	rinfo->set_sda = NULL;

	dev_dbg(bus->dev, "i2c gpio recovery using TGCLK\n");

	rinfo->get_scl = npcm_smb_get_SCL;
	rinfo->get_sda = npcm_smb_get_SDA;

	_adap->bus_recovery_info = rinfo;

	return 0;
}

static bool npcm_smb_init_clk(struct npcm_i2c *bus, u32 bus_freq)
{
	u32  k1 = 0;
	u32  k2 = 0;
	u8   dbnct = 0;
	u32  sclfrq = 0;
	u8   hldt = 7;
	bool fast_mode = false;
	u32  src_clk_freq; // in KHz

	src_clk_freq = bus->apb_clk / 1000;
	bus->bus_freq = bus_freq;

	if (bus_freq <= SMBUS_FREQ_100KHZ) {
		sclfrq = src_clk_freq / (bus_freq * 4);

		if (sclfrq < SCLFRQ_MIN || sclfrq > SCLFRQ_MAX)
			return false;

		if (src_clk_freq >= 40000)
			hldt = 17;
		else if (src_clk_freq >= 12500)
			hldt = 15;
		else
			hldt = 7;
	}

	else if (bus_freq == SMBUS_FREQ_400KHZ) {
		sclfrq = 0;
		fast_mode = true;

		if (src_clk_freq < 7500)
			// 400KHZ cannot be supported for core clock < 7.5 MHZ
			return false;

		else if (src_clk_freq >= 50000) {
			k1 = 80;
			k2 = 48;
			hldt = 12;
			dbnct = 7;
		}

		// Master or Slave with frequency > 25 MHZ
		else if (src_clk_freq > 25000) {
			hldt = (u8)__KERNEL_DIV_ROUND_UP(src_clk_freq * 300,
							 1000000) + 7;

			k1 = __KERNEL_DIV_ROUND_UP(src_clk_freq * 1600,
						   1000000);
			k2 = __KERNEL_DIV_ROUND_UP(src_clk_freq * 900,
						   1000000);
			k1 = round_up(k1, 2);
			k2 = round_up(k2 + 1, 2);
			if (k1 < SCLFRQ_MIN || k1 > SCLFRQ_MAX ||
			    k2 < SCLFRQ_MIN || k2 > SCLFRQ_MAX)
				return false;
		}
	}

	else if (bus_freq == SMBUS_FREQ_1MHZ) {
		sclfrq = 0;
		fast_mode = true;

		if (src_clk_freq < 24000)
		// 1MHZ cannot be supported for master core clock < 15 MHZ
		// or slave core clock < 24 MHZ
			return false;

		k1 = round_up((__KERNEL_DIV_ROUND_UP(src_clk_freq * 620,
						     1000000)), 2);
		k2 = round_up((__KERNEL_DIV_ROUND_UP(src_clk_freq * 380,
						     1000000) + 1), 2);
		if (k1 < SCLFRQ_MIN || k1 > SCLFRQ_MAX ||
		    k2 < SCLFRQ_MIN || k2 > SCLFRQ_MAX)
			return false;

		// Master or Slave with frequency > 40 MHZ
		if (src_clk_freq > 40000) {
			// Set HLDT:
			// SDA hold time:  (HLDT-7) * T(CLK) >= 120
			// HLDT = 120/T(CLK) + 7 = 120 * FREQ(CLK) + 7
			hldt = (u8)__KERNEL_DIV_ROUND_UP(src_clk_freq * 120,
							 1000000) + 7;
		} else {
			hldt = 7;
			dbnct = 2;
		}
	}

	// Frequency larger than 1 MHZ
	else
		return false;

	// After clock parameters calculation update reg (ENABLE should be 0):
	iowrite8(FIELD_PREP(SMBCTL2_SCLFRQ6_0, sclfrq & 0x7F),
		 bus->reg + NPCM_SMBCTL2);

	// force to bank 0, set SCL and fast mode
	iowrite8(FIELD_PREP(SMBCTL3_400K_MODE, fast_mode) |
		 FIELD_PREP(SMBCTL3_SCLFRQ8_7, (sclfrq >> 7) & 0x3),
		 bus->reg + NPCM_SMBCTL3);

	// Select Bank 0 to access NPCM_SMBCTL4/NPCM_SMBCTL5
	npcm_smb_select_bank(bus, SMB_BANK_0);

	if (bus_freq >= SMBUS_FREQ_400KHZ) {
		// Set SCL Low/High Time:
		// k1 = 2 * SCLLT7-0 -> Low Time  = k1 / 2
		// k2 = 2 * SCLLT7-0 -> High Time = k2 / 2
		iowrite8((u8)k1 / 2, bus->reg + NPCM_SMBSCLLT);
		iowrite8((u8)k2 / 2, bus->reg + NPCM_SMBSCLHT);

		iowrite8(dbnct, bus->reg + NPCM_SMBCTL5);
	}

	iowrite8(hldt, bus->reg + NPCM_SMBCTL4);

	// Return to Bank 1, and stay there by default:
	npcm_smb_select_bank(bus, SMB_BANK_1);

	return true;
}

static bool npcm_smb_init_module(struct npcm_i2c *bus, enum smb_mode mode,
				 u32 bus_freq)
{
	// Check whether module already enabled or frequency is out of bounds
	if ((bus->state != SMB_DISABLE && bus->state != SMB_IDLE) ||
	    bus_freq < SMBUS_FREQ_MIN || bus_freq > SMBUS_FREQ_MAX)
		return false;

	npcm_smb_disable(bus);

	// Configure FIFO mode :
	if (FIELD_GET(SMB_VER_FIFO_EN, ioread8(bus->reg + SMB_VER))) {
		bus->fifo_use = true;
		npcm_smb_select_bank(bus, SMB_BANK_0);
		iowrite8(ioread8(bus->reg + NPCM_SMBFIF_CTL) |
			 NPCM_SMBFIF_CTL_FIFO_EN, bus->reg + NPCM_SMBFIF_CTL);
		npcm_smb_select_bank(bus, SMB_BANK_1);
	} else {
		bus->fifo_use = false;
	}

	// Configure SMB module clock frequency
	if (!npcm_smb_init_clk(bus, bus_freq)) {
		dev_err(bus->dev, "npcm_smb_init_clk failed\n");
		return false;
	}

	// Enable module (before configuring CTL1)
	npcm_smb_enable(bus);
	bus->state = SMB_IDLE;

	// Enable SMB int and New Address Match int source
	iowrite8((ioread8(bus->reg + NPCM_SMBCTL1) | NPCM_SMBCTL1_NMINTE) &
		 ~NPCM_SMBCTL1_RWS_FIELDS,
		 bus->reg + NPCM_SMBCTL1);

	npcm_smb_int_enable(bus, true);

	npcm_smb_reset(bus);

	return true;
}

static int __npcm_i2c_init(struct npcm_i2c *bus, struct platform_device *pdev)
{
	u32 clk_freq;
	int ret;

	// Initialize the internal data structures
	bus->state = SMB_DISABLE;
	bus->master_or_slave = SMB_SLAVE;
	bus->int_time_stamp = 0;
	bus->slave = NULL;
	bus->xmits = 0;

	ret = of_property_read_u32(pdev->dev.of_node,
				   "bus-frequency", &clk_freq);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not read bus-frequency property\n");
		clk_freq = 100000;
	}

	ret = npcm_smb_init_module(bus, SMB_MASTER, clk_freq / 1000);
	if (!ret) {
		dev_err(&pdev->dev,
			"npcm_smb_init_module() failed\n");
		return -1;
	}

	return 0;
}

static irqreturn_t npcm_i2c_bus_irq(int irq, void *dev_id)
{
	irqreturn_t ret;
	struct npcm_i2c *bus = dev_id;

	bus->int_cnt++;

	if (npcm_smb_is_master(bus))
		bus->master_or_slave = SMB_MASTER;

	if (bus->master_or_slave == SMB_MASTER)	{
		bus->int_time_stamp = jiffies;
		ret = npcm_smb_int_master_handler(bus);
		if (ret == IRQ_HANDLED)
			return ret;
	}
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	if (bus->slave) {
		bus->master_or_slave = SMB_SLAVE;
		ret = npcm_smb_int_slave_handler(bus);
		if (ret == IRQ_HANDLED)
			return ret;
	}
#endif
	return IRQ_HANDLED;
}

static bool npcm_smb_master_start_xmit(struct npcm_i2c *bus,
				       u8 slave_addr, u16 nwrite, u16 nread,
				       u8 *write_data, u8 *read_data,
				       bool use_PEC, bool use_read_block)
{
	if (bus->state != SMB_IDLE) {
		bus->cmd_err = -(EBUSY);
		return false;
	}

	bus->xmits++;

	bus->dest_addr = (u8)(slave_addr << 1);// Translate 7bit to 8bit format
	bus->wr_buf = write_data;
	bus->wr_size = nwrite;
	bus->wr_ind = 0;
	bus->rd_buf = read_data;
	bus->rd_size = nread;
	bus->rd_ind = 0;
	bus->PEC_use = 0;

	// for write, PEC is appended to buffer from i2c IF. PEC flag is ignored
	if (nread)
		bus->PEC_use = use_PEC;
	bus->read_block_use = use_read_block;
	if (nread && !nwrite)
		bus->operation = SMB_READ_OPER;
	else
		bus->operation = SMB_WRITE_OPER;

	bus->int_cnt = 0;
	bus->event_log = 0;

	if (bus->fifo_use) {
		u8 smbfif_cts;
		// select bank 1 for FIFO regs
		npcm_smb_select_bank(bus, SMB_BANK_1);

		smbfif_cts = ioread8(bus->reg + NPCM_SMBFIF_CTS);

		// clear FIFO and relevant status bits.
		iowrite8((smbfif_cts & (~NPCM_SMBFIF_CTS_SLVRSTR)) |
			 NPCM_SMBFIF_CTS_CLR_FIFO,
			 bus->reg + NPCM_SMBFIF_CTS);
	}

	bus->state = SMB_IDLE;

	npcm_smb_stall_after_start(bus, true);
	npcm_smb_master_start(bus);

	return true;
}

static int npcm_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
				int num)
{
	struct npcm_i2c *bus = container_of(adap, struct npcm_i2c, adap);
	struct i2c_msg *msg0, *msg1;
	unsigned long time_left, flags;
	u16 nwrite, nread;
	u8 *write_data, *read_data;
	u8 slave_addr;
	int timeout;
	int ret = 0;
	bool read_block = false;
	bool read_PEC = false;
	u8 bus_busy;
	unsigned long timeout_usec;

	if (unlikely(bus->state == SMB_DISABLE)) {
		dev_err(bus->dev, "I2C%d module is disabled", bus->num);
		return -EINVAL;
	}

	if (num > 2 || num < 1) {
		dev_err(bus->dev, "I2C cmd not supported num of msgs=%d", num);
		return -EINVAL;
	}

	msg0 = &msgs[0];
	slave_addr = msg0->addr;
	if (msg0->flags & I2C_M_RD) { // read
		if (num == 2) {
			dev_err(bus->dev, "num=2 but 1st msg rd instead of wr");
			return -EINVAL;
		}
		nwrite = 0;
		write_data = NULL;
		read_data = msg0->buf;
		if (msg0->flags & I2C_M_RECV_LEN) {
			nread = 1;
			read_block = true;
			if (msg0->flags & I2C_CLIENT_PEC)
				read_PEC = true;
		} else {
			nread = msg0->len;
		}
	} else { // write
		nwrite = msg0->len;
		write_data = msg0->buf;
		nread = 0;
		read_data = NULL;
		if (num == 2) {
			msg1 = &msgs[1];
			read_data = msg1->buf;
			if (slave_addr != msg1->addr) {
				dev_err(bus->dev,
					"SA==%02x but msg1->addr==%02x\n",
				       slave_addr, msg1->addr);
				return -EINVAL;
			}
			if ((msg1->flags & I2C_M_RD) == 0) {
				dev_err(bus->dev,
					"num = 2 but both msg are write.\n");
				return -EINVAL;
			}
			if (msg1->flags & I2C_M_RECV_LEN) {
				nread = 1;
				read_block = true;
				if (msg1->flags & I2C_CLIENT_PEC)
					read_PEC = true;
			} else {
				nread = msg1->len;
				read_block = false;
			}
		}
	}

	/* Adaptive TimeOut: astimated time in usec  + 100% margin */
	timeout_usec = (2 * 10000 / bus->bus_freq) * (2 + nread + nwrite);
	timeout = max(msecs_to_jiffies(35), usecs_to_jiffies(timeout_usec));
	if (nwrite >= 32 * 1024 ||  nread >= 32 * 1024) {
		dev_err(bus->dev, "i2c%d buffer too big\n", bus->num);
		return -EINVAL;
	}

	time_left = jiffies +
		    msecs_to_jiffies(DEFAULT_STALL_COUNT) + 1;
	do {
		/* we must clear slave address immediately when the bus is not
		 * busy, so we spinlock it, but we don't keep the lock for the
		 * entire while since it is too long.
		 */
		spin_lock_irqsave(&bus->lock, flags);
		bus_busy = ioread8(bus->reg + NPCM_SMBCST) & NPCM_SMBCST_BB;
		if (!bus_busy && bus->slave)
			iowrite8((bus->slave->addr & 0x7F),
				 bus->reg + NPCM_SMBADDR1);
		spin_unlock_irqrestore(&bus->lock, flags);

		if (!bus_busy)
			break;
	} while (time_is_after_jiffies(time_left));

	if (bus_busy) {
		iowrite8(NPCM_SMBCST_BB, bus->reg + NPCM_SMBCST);
		npcm_smb_reset(bus);
		i2c_recover_bus(adap);
		return -EAGAIN;
	}

	npcm_smb_init_params(bus);
	bus->dest_addr = slave_addr;
	bus->msgs = msgs;
	bus->msgs_num = num;
	bus->read_block_use = read_block;

	reinit_completion(&bus->cmd_complete);

	if (!npcm_smb_master_start_xmit(bus, slave_addr, nwrite, nread,
					write_data, read_data, read_PEC,
					read_block))
		ret = -(EBUSY);

	if (ret != -(EBUSY)) {
		time_left = wait_for_completion_timeout(&bus->cmd_complete,
							timeout);

		if (time_left == 0) {
			NPCM_I2C_EVENT_LOG(NPCM_I2C_EVENT_TO);
			if (bus->master_or_slave == SMB_MASTER) {
				dev_dbg(bus->dev,
					"i2c%d TO = %d\n", bus->num, timeout);
				i2c_recover_bus(adap);
				bus->cmd_err = -EIO;
				bus->state = SMB_IDLE;
			}
		}
	}
	ret = bus->cmd_err;

	// if there was BER, check if need to recover the bus:
	if (bus->cmd_err == -EAGAIN)
		i2c_recover_bus(adap);

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	// reenable slave if it was enabled
	if (bus->slave)
		iowrite8((bus->slave->addr & 0x7F) | NPCM_SMBADDR_SAEN,
			 bus->reg + NPCM_SMBADDR1);
#endif

	// If nothing went wrong, return number of messages x-ferred.
	if (ret >= 0)
		return num;

	// print errors apart from NACK
	if (bus->cmd_err == -ENXIO)
		dev_dbg(bus->dev, "cmd failed cmd_err = %d\n", ret);
	return ret;
}

static u32 npcm_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_BLOCK_DATA |
			I2C_FUNC_SLAVE | I2C_FUNC_SMBUS_PEC;
}

static const struct i2c_adapter_quirks npcm_i2c_quirks = {
	.max_read_len = 32768,
	.max_write_len = 32768,
	.max_num_msgs = 2,
	.flags = I2C_AQ_COMB_WRITE_THEN_READ
};

static const struct i2c_algorithm npcm_i2c_algo = {
	.master_xfer = npcm_i2c_master_xfer,
	.functionality = npcm_i2c_functionality,
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	.reg_slave	= npcm_i2c_reg_slave,
	.unreg_slave	= npcm_i2c_unreg_slave,
#endif
};

static int  npcm_i2c_probe_bus(struct platform_device *pdev)
{
	struct npcm_i2c *bus;
	struct i2c_adapter *adap;
	struct resource *res;
	struct clk *i2c_clk;
	int ret;
	int num;

	bus = devm_kzalloc(&pdev->dev, sizeof(*bus), GFP_KERNEL);
	if (!bus)
		return -ENOMEM;

#ifdef CONFIG_OF
	num = of_alias_get_id(pdev->dev.of_node, "i2c");
	bus->num = num;
	i2c_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(i2c_clk)) {
		dev_err(bus->dev, " I2C probe failed: can't read clk\n");
		return	-EPROBE_DEFER;
	}
	bus->apb_clk = clk_get_rate(i2c_clk);
	dev_dbg(bus->dev, "I2C APB clock is %d\n", bus->apb_clk);
#endif //  CONFIG_OF

	gcr_regmap = syscon_regmap_lookup_by_compatible("nuvoton,npcm750-gcr");
	if (IS_ERR(gcr_regmap)) {
		dev_err(bus->dev, "%s: failed to find nuvoton,npcm750-gcr\n",
			__func__);
		return IS_ERR(gcr_regmap);
	}
	regmap_write(gcr_regmap, NPCM_I2CSEGCTL, I2CSEGCTL_VAL);
	dev_dbg(bus->dev, "I2C%d: gcr mapped\n", bus->num);

	clk_regmap = syscon_regmap_lookup_by_compatible("nuvoton,npcm750-clk");
	if (IS_ERR(clk_regmap)) {
		dev_err(bus->dev, "%s: failed to find nuvoton,npcm750-clk\n",
			__func__);
		return IS_ERR(clk_regmap);
	}
	dev_dbg(bus->dev, "I2C%d: clk mapped\n", bus->num);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev_dbg(bus->dev, "resource: %pR\n", res);
	bus->reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR((bus)->reg))
		return PTR_ERR((bus)->reg);
	dev_dbg(bus->dev, "base = %p\n", bus->reg);

	// Initialize the I2C adapter
	spin_lock_init(&bus->lock);
	init_completion(&bus->cmd_complete);

	adap = &bus->adap;
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON | I2C_CLASS_SPD | I2C_CLIENT_SLAVE;
	adap->retries = 3;
	adap->timeout = HZ;
	adap->algo = &npcm_i2c_algo;
	adap->quirks = &npcm_i2c_quirks;
	adap->algo_data = bus;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;
	adap->nr = pdev->id;

	bus->dev = &pdev->dev;
	bus->slave = NULL;

	bus->irq = platform_get_irq(pdev, 0);
	if (bus->irq < 0) {
		dev_err(bus->dev, "I2C platform_get_irq error\n");
		return -ENODEV;
	}
	dev_dbg(bus->dev, "irq = %d\n", bus->irq);

	ret = devm_request_irq(&pdev->dev, bus->irq, npcm_i2c_bus_irq, 0,
			       dev_name(&pdev->dev), (void *)bus);

	if (ret) {
		dev_err(&pdev->dev, "I2C%d: request_irq fail\n", bus->num);
		return ret;
	}

	ret = __npcm_i2c_init(bus, pdev);
	if (ret < 0)
		return ret;

	ret = npcm_smb_recovery_init(adap);
	if (ret)
		return ret;

	i2c_set_adapdata(adap, bus);

	snprintf(bus->adap.name, sizeof(bus->adap.name), "Nuvoton i2c");

	ret = i2c_add_numbered_adapter(&bus->adap);
	if (ret < 0) {
		dev_err(&pdev->dev, "I2C%d: i2c_add_numbered_adapter fail\n",
			bus->num);
		return ret;
	}

	platform_set_drvdata(pdev, bus);

	pr_info("npcm7xx I2C bus is %d registered\n", bus->adap.nr);

	return 0;
}

static int  npcm_i2c_remove_bus(struct platform_device *pdev)
{
	unsigned long lock_flags;
	struct npcm_i2c *bus = platform_get_drvdata(pdev);

	spin_lock_irqsave(&bus->lock, lock_flags);
	npcm_smb_disable(bus);
	spin_unlock_irqrestore(&bus->lock, lock_flags);
	i2c_del_adapter(&bus->adap);

	return 0;
}

static const struct of_device_id npcm_i2c_bus_of_table[] = {
	{ .compatible = "nuvoton,npcm750-i2c", },
	{},
};
MODULE_DEVICE_TABLE(of, npcm_i2c_bus_of_table);

static struct platform_driver npcm_i2c_bus_driver = {
	.probe = npcm_i2c_probe_bus,
	.remove = npcm_i2c_remove_bus,
	.driver = {
		.name = "nuvoton-i2c",
		.of_match_table = npcm_i2c_bus_of_table,
	}
};
module_platform_driver(npcm_i2c_bus_driver);

MODULE_AUTHOR("Avi Fishman <avi.fishman@gmail.com>");
MODULE_AUTHOR("Tali Perry <tali.perry@nuvoton.com>");
MODULE_AUTHOR("Tyrone Ting <kfting@nuvoton.com>");
MODULE_DESCRIPTION("Nuvoton I2C Bus Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(I2C_VERSION);
