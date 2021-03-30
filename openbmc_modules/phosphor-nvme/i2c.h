#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <sys/ioctl.h>

#define I2C_DATA_MAX 256

static inline __s32 i2c_read_after_write(int file, __u8 slave_addr, __u8 tx_len,
                                         __u8* tx_buf, int rx_len, __u8* rx_buf)
{
    struct i2c_rdwr_ioctl_data msgst;
    struct i2c_msg msg[2];
    int ret;

    msg[0].addr = slave_addr & 0xFF;
    msg[0].flags = 0;
    msg[0].buf = (__u8*)tx_buf;
    msg[0].len = tx_len;

    msg[1].addr = slave_addr & 0xFF;
    msg[1].flags = I2C_M_RD | I2C_M_RECV_LEN;
    msg[1].buf = (__u8*)rx_buf;
    msg[1].len = rx_len;

    msgst.msgs = msg;
    msgst.nmsgs = 2;

    ret = ioctl(file, I2C_RDWR, &msgst);

    if (ret < 0)
        return ret;

    return ret;
}
