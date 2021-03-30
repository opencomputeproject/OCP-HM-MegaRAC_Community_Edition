// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019, IBM Corp.
 */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/regmap.h>
#include <linux/sched/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define LPC_HICRB            0x080
#define     LPC_HICRB_IBFIF4         BIT(1)
#define     LPC_HICRB_LPC4E          BIT(0)
#define LPC_HICRC            0x084
#define     LPC_KCS4_IRQSEL_MASK     GENMASK(7, 4)
#define     LPC_KCS4_IRQSEL_SHIFT    4
#define     LPC_KCS4_IRQTYPE_MASK    GENMASK(3, 2)
#define     LPC_KCS4_IRQTYPE_SHIFT   2
#define     LPC_KCS4_IRQTYPE_LOW     0b00
#define     LPC_KCS4_IRQTYPE_HIGH    0b01
#define     LPC_KCS4_IRQTYPE_RSVD    0b10
#define     LPC_KCS4_IRQTYPE_RISING  0b11
#define     LPC_KCS4_OBF4_AUTO_CLR   BIT(1)
#define     LPC_KCS4_IRQ_HOST	     BIT(0)
#define LPC_LADR4            0x090
#define LPC_IDR4             0x094
#define LPC_ODR4             0x098
#define LPC_STR4             0x09C
#define     STR4_IBF	     (1 << 1)
#define     STR4_OBF	     (1 << 0)

#define HOST_ODR             0xca2
#define HOST_STR             0xca3
#define HOST_SERIRQ_ID       11
#define HOST_SERIRQ_TYPE     LPC_KCS4_IRQTYPE_LOW

#define RX_BUF_SIZE 1024

struct mctp_lpc {
	struct miscdevice miscdev;
	struct regmap *map;

	wait_queue_head_t rx;
	bool pending;
	u8 idr;
};

static irqreturn_t mctp_lpc_irq(int irq, void *data)
{
	struct mctp_lpc *priv = data;
	unsigned long flags;
	unsigned int hicrb;
	struct device *dev;
	unsigned int str;
	irqreturn_t ret;

	dev = priv->miscdev.this_device;

	spin_lock_irqsave(&priv->rx.lock, flags);

	regmap_read(priv->map, LPC_STR4, &str);
	regmap_read(priv->map, LPC_HICRB, &hicrb);

	if ((str & STR4_IBF) && (hicrb & LPC_HICRB_IBFIF4)) {
		unsigned int val;

		if (priv->pending)
			dev_err(dev, "Storm brewing!");

		/* Mask the IRQ / Enter polling mode */
		dev_dbg(dev, "Received IRQ %d, disabling to provide back-pressure\n",
			irq);
		regmap_update_bits(priv->map, LPC_HICRB, LPC_HICRB_IBFIF4, 0);

		/*
		 * Extract the IDR4 value to ack the IRQ. Reading IDR clears
		 * IBF and allows the host to write another value, however as
		 * we have disabled IRQs the back-pressure is still applied
		 * until userspace starts servicing the interface.
		 */
		regmap_read(priv->map, LPC_IDR4, &val);
		priv->idr = val & 0xff;
		priv->pending = true;

		dev_dbg(dev, "Set pending, waking waiters\n");
		wake_up_locked(&priv->rx);
		ret = IRQ_HANDLED;
	} else {
		dev_dbg(dev, "LPC IRQ triggered, but not for us (str=0x%x, hicrb=0x%x)\n",
			str, hicrb);
		ret = IRQ_NONE;
	}

	spin_unlock_irqrestore(&priv->rx.lock, flags);

	return ret;
}

static inline struct mctp_lpc *to_mctp_lpc(struct file *filp)
{
	return container_of(filp->private_data, struct mctp_lpc, miscdev);
}

static ssize_t mctp_lpc_read(struct file *filp, char __user *buf,
			     size_t count, loff_t *ppos)
{
	struct mctp_lpc *priv;
	struct device *dev;
	size_t remaining;
	ssize_t rc;

	priv = to_mctp_lpc(filp);
	dev = priv->miscdev.this_device;

	if (!count)
		return 0;

	if (count > 2 || *ppos > 1)
		return -EINVAL;

	remaining = count;

	spin_lock_irq(&priv->rx.lock);
	if (*ppos == 0) {
		unsigned int val;
		u8 str;

		/* YOLO blocking, non-block not supported */
		dev_dbg(dev, "Waiting for IBF\n");
		regmap_read(priv->map, LPC_STR4, &val);
		str = val & 0xff;
		rc = wait_event_interruptible_locked(priv->rx, (priv->pending || str & STR4_IBF));
		if (rc < 0)
			goto out;

		if (signal_pending(current)) {
			dev_dbg(dev, "Interrupted waiting for IBF\n");
			rc = -EINTR;
			goto out;
		}

		/*
		 * Re-enable IRQs prior to possible read of IDR (which clears
		 * IBF) to ensure we receive interrupts for subsequent writes
		 * to IDR. Writes to IDR by the host should not occur while IBF
		 * is set.
		 */
		dev_dbg(dev, "Woken by IBF, enabling IRQ\n");
		regmap_update_bits(priv->map, LPC_HICRB, LPC_HICRB_IBFIF4,
				   LPC_HICRB_IBFIF4);

		/* Read data out of IDR into internal storage if necessary */
		if (!priv->pending) {
			WARN(!(str & STR4_IBF), "Unknown reason for wakeup!");

			/* Extract the IDR4 value to ack the IRQ */
			regmap_read(priv->map, LPC_IDR4, &val);
			priv->idr = val & 0xff;
		}

		/* Copy data from internal storage to userspace */
		if (copy_to_user(buf, &priv->idr, sizeof(priv->idr))) {
			rc = -EFAULT;
			goto out;
		}

		/* We're done consuming the internally stored value */
		priv->pending = false;

		remaining--;
		buf++;
	}

	if (remaining) {
		/* Either:
		 *
		 * 1. (count == 1 && *ppos == 1)
		 * 2. (count == 2 && *ppos == 0)
		 */
		unsigned int val;
		u8 str;

		regmap_read(priv->map, LPC_STR4, &val);
		str = val & 0xff;
		if (*ppos == 0 || priv->pending)
			/*
			 * If we got this far with `*ppos == 0` then we've read
			 * data out of IDR, so set IBF when reporting back to
			 * userspace so userspace knows the IDR value is valid.
			 */
			str |= STR4_IBF;

		dev_dbg(dev, "Read status 0x%x\n", str);
		if (copy_to_user(buf, &str, sizeof(str))) {
			rc = -EFAULT;
			goto out;
		}

		remaining--;
	}

	WARN_ON(remaining);

	rc = count;

out:
	spin_unlock_irq(&priv->rx.lock);

	return rc;
}

static ssize_t mctp_lpc_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	uint8_t _data[2], *data = &_data[0];
	struct mctp_lpc *priv;
	struct device *dev;
	size_t remaining;
	unsigned int str;

	priv = to_mctp_lpc(filp);
	dev = priv->miscdev.this_device;

	if (!count)
		return count;

	if (count > 2)
		return -EINVAL;

	if (*ppos >= 2)
		return -EINVAL;

	if (*ppos + count > 2)
		return -EINVAL;

	if (copy_from_user(data, buf, count))
		return -EFAULT;

	remaining = count;

	if (*ppos == 0) {
		/* Wait until OBF is clear - we don't get an IRQ */
		dev_dbg(dev, "Waiting for OBF to clear\n");
		for (;;) {
			if (signal_pending(current))
				return -EINTR;

			regmap_read(priv->map, LPC_STR4, &str);
			if (!(str & STR4_OBF))
				break;

			msleep(1);
		}

		dev_dbg(dev, "Writing 0x%x to ODR\n", *data);
		regmap_write(priv->map, LPC_ODR4, *data);
		remaining--;
		data++;
	}

	if (remaining) {
		if (!(*data & STR4_OBF))
			dev_err(dev, "Clearing OBF with status write: 0x%x\n",
				*data);
		dev_dbg(dev, "Writing status 0x%x\n", *data);
		regmap_write(priv->map, LPC_STR4, *data);
		remaining--;
	}

	WARN_ON(remaining);

	regmap_read(priv->map, LPC_STR4, &str);
	dev_dbg(dev, "Triggering SerIRQ (current str=0x%x)\n", str);

	/*
	 * Trigger Host IRQ on ODR write. Do this after any STR write in case
	 * we need to write ODR to indicate an STR update (which we do).
	 */
	if (*ppos == 0)
		regmap_update_bits(priv->map, LPC_HICRC, LPC_KCS4_IRQ_HOST,
				   LPC_KCS4_IRQ_HOST);

	return count;
}

static __poll_t mctp_lpc_poll(struct file *filp, poll_table *wait)
{
	struct mctp_lpc *priv;
	struct device *dev;
	unsigned int val;
	bool ibf;

	priv = to_mctp_lpc(filp);
	dev = priv->miscdev.this_device;

	regmap_read(priv->map, LPC_STR4, &val);

	spin_lock_irq(&priv->rx.lock);

	ibf = priv->pending || val & STR4_IBF;

	if (!ibf) {
		dev_dbg(dev, "Polling on IBF\n");

		spin_unlock_irq(&priv->rx.lock);

		poll_wait(filp, &priv->rx, wait);
		if (signal_pending(current)) {
			dev_dbg(dev, "Polling IBF was interrupted\n");
			goto out;
		}

		spin_lock_irq(&priv->rx.lock);

		regmap_read(priv->map, LPC_STR4, &val);

		ibf = priv->pending || val & STR4_IBF;
	}

	spin_unlock_irq(&priv->rx.lock);

out:
	dev_dbg(dev, "Polled IBF state: %s\n", ibf ? "set" : "clear");

	return ibf ? EPOLLIN : 0;
}

static const struct file_operations mctp_lpc_fops = {
	.owner          = THIS_MODULE,
	.llseek		= no_seek_end_llseek,
	.read           = mctp_lpc_read,
	.write          = mctp_lpc_write,
	.poll		= mctp_lpc_poll,
};

static int mctp_lpc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	unsigned int mask, val;
	struct mctp_lpc *priv;
	int irq;
	int rc;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->map = syscon_node_to_regmap(dev->parent->of_node);
	if (IS_ERR(priv->map)) {
		dev_err(dev, "Couldn't get regmap\n");
		return -ENODEV;
	}

	/*
	 * Set the LPC address. Simultaneously, test our MMIO regmap works. All
	 * subsequent accesses are assumed to work
	 */
	rc = regmap_write(priv->map, LPC_LADR4, ((HOST_STR) << 16) | HOST_ODR);
	if (rc < 0)
		return rc;

	/* Set up the SerIRQ */
	mask = LPC_KCS4_IRQSEL_MASK
		| LPC_KCS4_IRQTYPE_MASK
		| LPC_KCS4_OBF4_AUTO_CLR;
	val = (HOST_SERIRQ_ID << LPC_KCS4_IRQSEL_SHIFT)
		| (HOST_SERIRQ_TYPE << LPC_KCS4_IRQTYPE_SHIFT);
	val &= ~LPC_KCS4_OBF4_AUTO_CLR; /* Unnecessary, just documentation */
	regmap_update_bits(priv->map, LPC_HICRC, mask, val);

	/* Trigger waiters from IRQ */
	init_waitqueue_head(&priv->rx);

	dev_set_drvdata(dev, priv);

	/* Set up the miscdevice */
	priv->miscdev.minor = MISC_DYNAMIC_MINOR;
	priv->miscdev.name = "mctp0";
	priv->miscdev.fops = &mctp_lpc_fops;

	/* Configure the IRQ handler */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	rc = devm_request_irq(dev, irq, mctp_lpc_irq, IRQF_SHARED,
			      dev_name(dev), priv);
	if (rc < 0)
		return rc;

	/* Register the device */
	rc = misc_register(&priv->miscdev);
	if (rc) {
		dev_err(dev, "Unable to register device\n");
		return rc;
	}

	/* Enable the channel */
	regmap_update_bits(priv->map, LPC_HICRB,
			LPC_HICRB_IBFIF4 | LPC_HICRB_LPC4E,
			LPC_HICRB_IBFIF4 | LPC_HICRB_LPC4E);

	return 0;
}

static int mctp_lpc_remove(struct platform_device *pdev)
{
	struct mctp_lpc *ctx = dev_get_drvdata(&pdev->dev);

	misc_deregister(&ctx->miscdev);

	return 0;
}

static const struct of_device_id mctp_lpc_match[] = {
	{ .compatible = "openbmc,mctp-lpc" },
	{ }
};
MODULE_DEVICE_TABLE(of, mctp_lpc_match);

static struct platform_driver mctp_lpc = {
	.driver = {
		.name           = "mctp-lpc",
		.of_match_table = mctp_lpc_match,
	},
	.probe  = mctp_lpc_probe,
	.remove = mctp_lpc_remove,
};
module_platform_driver(mctp_lpc);

MODULE_LICENSE("GPL v2+");
MODULE_AUTHOR("Andrew Jeffery <andrew@aj.id.au>");
MODULE_DESCRIPTION("OpenBMC MCTP LPC binding on ASPEED KCS");
