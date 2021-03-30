// SPDX-License-Identifier: GPL-2.0+
// Copyright 2018 IBM Corp.

#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#define DEVICE_NAME "aspeed-bmc-misc"

struct aspeed_bmc_ctrl {
	const char *name;
	u32 offset;
	u32 mask;
	u32 shift;
	struct regmap *map;
	struct kobj_attribute attr;
};

struct aspeed_bmc_misc {
	struct device *dev;
	struct regmap *map;
	struct aspeed_bmc_ctrl *ctrls;
	int nr_ctrls;
};

static int aspeed_bmc_misc_parse_dt_child(struct device_node *child,
					  struct aspeed_bmc_ctrl *ctrl)
{
	int rc;

	/* Example child:
	 *
	 * ilpc2ahb {
	 *     offset = <0x80>;
	 *     bit-mask = <0x1>;
	 *     bit-shift = <6>;
	 *     label = "foo";
	 * }
	 */
	if (of_property_read_string(child, "label", &ctrl->name))
		ctrl->name = child->name;

	rc = of_property_read_u32(child, "offset", &ctrl->offset);
	if (rc < 0)
		return rc;

	rc = of_property_read_u32(child, "bit-mask", &ctrl->mask);
	if (rc < 0)
		return rc;

	rc = of_property_read_u32(child, "bit-shift", &ctrl->shift);
	if (rc < 0)
		return rc;

	ctrl->mask <<= ctrl->shift;

	return 0;
}

static int aspeed_bmc_misc_parse_dt(struct aspeed_bmc_misc *bmc,
				    struct device_node *parent)
{
	struct aspeed_bmc_ctrl *ctrl;
	struct device_node *child;
	int rc;

	bmc->nr_ctrls = of_get_child_count(parent);
	bmc->ctrls = devm_kcalloc(bmc->dev, bmc->nr_ctrls, sizeof(*bmc->ctrls),
				  GFP_KERNEL);
	if (!bmc->ctrls)
		return -ENOMEM;

	ctrl = bmc->ctrls;
	for_each_child_of_node(parent, child) {
		rc = aspeed_bmc_misc_parse_dt_child(child, ctrl++);
		if (rc < 0) {
			of_node_put(child);
			return rc;
		}
	}

	return 0;
}

static ssize_t aspeed_bmc_misc_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	struct aspeed_bmc_ctrl *ctrl;
	unsigned int val;
	int rc;

	ctrl = container_of(attr, struct aspeed_bmc_ctrl, attr);
	rc = regmap_read(ctrl->map, ctrl->offset, &val);
	if (rc)
		return rc;

	val &= ctrl->mask;
	val >>= ctrl->shift;

	return sprintf(buf, "%u\n", val);
}

static ssize_t aspeed_bmc_misc_store(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	struct aspeed_bmc_ctrl *ctrl;
	long val;
	int rc;

	rc = kstrtol(buf, 0, &val);
	if (rc)
		return rc;

	ctrl = container_of(attr, struct aspeed_bmc_ctrl, attr);
	val <<= ctrl->shift;
	rc = regmap_update_bits(ctrl->map, ctrl->offset, ctrl->mask, val);

	return rc < 0 ? rc : count;
}

static int aspeed_bmc_misc_add_sysfs_attr(struct aspeed_bmc_misc *bmc,
					  struct aspeed_bmc_ctrl *ctrl)
{
	ctrl->map = bmc->map;

	sysfs_attr_init(&ctrl->attr.attr);
	ctrl->attr.attr.name = ctrl->name;
	ctrl->attr.attr.mode = 0664;
	ctrl->attr.show = aspeed_bmc_misc_show;
	ctrl->attr.store = aspeed_bmc_misc_store;

	return sysfs_create_file(&bmc->dev->kobj, &ctrl->attr.attr);
}

static int aspeed_bmc_misc_populate_sysfs(struct aspeed_bmc_misc *bmc)
{
	int rc;
	int i;

	for (i = 0; i < bmc->nr_ctrls; i++) {
		rc = aspeed_bmc_misc_add_sysfs_attr(bmc, &bmc->ctrls[i]);
		if (rc < 0)
			return rc;
	}

	return 0;
}

static int aspeed_bmc_misc_probe(struct platform_device *pdev)
{
	struct aspeed_bmc_misc *bmc;
	int rc;

	bmc = devm_kzalloc(&pdev->dev, sizeof(*bmc), GFP_KERNEL);
	if (!bmc)
		return -ENOMEM;

	bmc->dev = &pdev->dev;
	bmc->map = syscon_node_to_regmap(pdev->dev.parent->of_node);
	if (IS_ERR(bmc->map))
		return PTR_ERR(bmc->map);

	rc = aspeed_bmc_misc_parse_dt(bmc, pdev->dev.of_node);
	if (rc < 0)
		return rc;

	return aspeed_bmc_misc_populate_sysfs(bmc);
}

static const struct of_device_id aspeed_bmc_misc_match[] = {
	{ .compatible = "aspeed,bmc-misc" },
	{ },
};

static struct platform_driver aspeed_bmc_misc = {
	.driver = {
		.name		= DEVICE_NAME,
		.of_match_table = aspeed_bmc_misc_match,
	},
	.probe = aspeed_bmc_misc_probe,
};

module_platform_driver(aspeed_bmc_misc);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Jeffery <andrew@aj.id.au>");
