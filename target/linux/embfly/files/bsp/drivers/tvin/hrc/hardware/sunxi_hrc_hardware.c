// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs HRC Driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "../sunxi_hrc_log.h"
#include "sunxi_hrc_hardware.h"
#include <linux/platform_device.h>
#include <linux/io.h>

#if defined(HRC_VERSION_110)
#include "hrc110/hrc110.h"
#endif  /* defined(HRC_VERSION_110) */

struct hrc_dev_func {
	/* control */
	int (*init)(void __iomem *reg_base);
	int (*exit)(void);
	int (*enable)(enum hrc_irq irq);
	int (*disable)(void);

	/* configure */
	int (*config)(struct hrc_ctrl_param ctrl_param,
		      struct hrc_input_param in_param,
		      struct hrc_output_param out_param);
	int (*config_addr)(struct hrc_addr out_addr);
	int (*config_ready)(void);

	/* interrupt */
	int (*get_irq_state)(u32 *state);
	int (*clr_irq_state)(u32 state);
	int (*get_irq_field)(u32 *field);

	/* register */
	int (*reg_read)(u32 reg, u32 *val);
	int (*reg_write)(u32 reg, u32 val);
	int (*reg_read_mask)(u32 reg, u32 mask, u32 *val);
	int (*reg_write_mask)(u32 reg, u32 mask, u32 val);
	int (*reg_dump)(char *buf, int n);

	int (*check_format_support)(u32 input_format, u32 output_format);
};

static struct hrc_dev_func sunxi_hrc_func = {
#if defined(HRC_VERSION_110)
	.init                 = hrc110_init,
	.exit                 = hrc110_exit,
	.enable               = hrc110_enable,
	.disable              = hrc110_disable,
	.config               = hrc110_config,
	.config_addr          = hrc110_config_addr,
	.config_ready         = hrc110_config_ready,
	.get_irq_state        = hrc110_get_irq_state,
	.clr_irq_state        = hrc110_clr_irq_state,
	.get_irq_field        = hrc110_get_irq_field,
	.reg_read             = hrc110_read,
	.reg_write            = hrc110_write,
	.reg_read_mask        = hrc110_read_mask,
	.reg_write_mask       = hrc110_write_mask,
	.reg_dump             = hrc110_reg_dump,
	.check_format_support = hrc110_check_format_support,
#endif  /* defined(HRC_VERSION_110) */
};

int sunxi_hrc_hardware_enable(enum hrc_irq irq)
{
	if (!sunxi_hrc_func.enable) {
		hrc_err("%s not support enable!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg("irq: 0x%x\n", irq);

	return sunxi_hrc_func.enable(irq);
}

int sunxi_hrc_hardware_disable(void)
{
	if (!sunxi_hrc_func.disable) {
		hrc_err("%s not support disable!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg();

	return sunxi_hrc_func.disable();
}

int sunxi_hrc_hardware_config(struct hrc_ctrl_param ctrl_param,
			      struct hrc_input_param in_param,
			      struct hrc_output_param out_param)
{
	if (!sunxi_hrc_func.config) {
		hrc_err("%s not support config!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg();

	return sunxi_hrc_func.config(ctrl_param, in_param, out_param);
}

int sunxi_hrc_hardware_config_addr(struct hrc_addr out_addr)
{
	if (!sunxi_hrc_func.config_addr) {
		hrc_err("%s not support config_addr!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg();

	return sunxi_hrc_func.config_addr(out_addr);
}

int sunxi_hrc_hardware_config_ready(void)
{
	if (!sunxi_hrc_func.config_ready) {
		hrc_err("%s not support config_ready!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg();

	return sunxi_hrc_func.config_ready();
}

int sunxi_hrc_hardware_get_irq_state(u32 *state)
{
	if (!sunxi_hrc_func.get_irq_state) {
		hrc_err("%s not support get_irq_state!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg();

	return sunxi_hrc_func.get_irq_state(state);
}

int sunxi_hrc_hardware_clear_irq_state(u32 state)
{
	if (!sunxi_hrc_func.clr_irq_state) {
		hrc_err("%s not support clr_irq_state!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg("state: 0x%x\n", state);

	return sunxi_hrc_func.clr_irq_state(state);
}

int sunxi_hrc_hardware_get_irq_field(u32 *field)
{
	if (!sunxi_hrc_func.get_irq_field) {
		hrc_err("%s not support get_irq_field!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg();

	return sunxi_hrc_func.get_irq_field(field);
}

int sunxi_hrc_hardware_reg_read(u32 reg, u32 *val)
{
	if (!sunxi_hrc_func.reg_read) {
		hrc_err("%s not support reg_read!\n", HARDWARE_NAME);
		return -1;
	}

	return sunxi_hrc_func.reg_read(reg, val);
}

int sunxi_hrc_hardware_reg_write(u32 reg, u32 val)
{
	if (!sunxi_hrc_func.reg_write) {
		hrc_err("%s not support reg_write!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg("reg: 0x%x val: 0x%x\n", reg, val);

	return sunxi_hrc_func.reg_write(reg, val);
}

int sunxi_hrc_hardware_reg_read_mask(u32 reg, u32 mask, u32 *val)
{
	if (!sunxi_hrc_func.reg_read_mask) {
		hrc_err("%s not support reg_read_mask!\n", HARDWARE_NAME);
		return -1;
	}

	return sunxi_hrc_func.reg_read_mask(reg, mask, val);
}

int sunxi_hrc_hardware_reg_write_mask(u32 reg, u32 mask, u32 val)
{
	if (!sunxi_hrc_func.reg_write_mask) {
		hrc_err("%s not support reg_write_mask!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg("reg: 0x%x mask: 0x%x val: 0x%x\n", reg, mask, val);

	return sunxi_hrc_func.reg_write_mask(reg, mask, val);
}

int sunxi_hrc_hardware_reg_dump(char *buf, int n)
{
	if (!sunxi_hrc_func.reg_dump) {
		hrc_err("%s not support reg_dump!\n", HARDWARE_NAME);
		return -1;
	}

	return sunxi_hrc_func.reg_dump(buf, n);
}

int sunxi_hrc_hardware_check_format_support(u32 input_format, u32 output_format)
{
	if (!sunxi_hrc_func.check_format_support) {
		hrc_err("%s not support check_format_support!\n", HARDWARE_NAME);
		return -1;
	}

	return sunxi_hrc_func.check_format_support(input_format, output_format);
}

static void __iomem *reg_base;
int sunxi_hrc_hardware_init(struct platform_device *pdev)
{
	int ret;
	struct resource *iores = NULL;

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_base = devm_ioremap_resource(&pdev->dev, iores);
	if (IS_ERR(reg_base)) {
		hrc_err("devm_ioremap_resource error!\n");
		return -EINVAL;
	}

	if (!sunxi_hrc_func.init) {
		hrc_err("%s not support init!\n", HARDWARE_NAME);
		goto ERR_DEV_INIT;
	}

	hrc_dbg();

	return sunxi_hrc_func.init(reg_base);

ERR_DEV_INIT:
	devm_iounmap(&pdev->dev, reg_base);
	return ret;
}

int sunxi_hrc_hardware_exit(struct platform_device *pdev)
{
	devm_iounmap(&pdev->dev, reg_base);

	if (!sunxi_hrc_func.exit) {
		hrc_err("%s not support exit!\n", HARDWARE_NAME);
		return -1;
	}

	hrc_dbg();

	return sunxi_hrc_func.exit();
}
