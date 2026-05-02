// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 zhaozeyan@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun252iw1_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_BOARD)
/* Pin banks are: A B C D F G */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#else
	/* TODO */
#endif
};

static const unsigned int sun252iw1_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
};

static const struct sunxi_pinctrl_desc sun252iw1_pinctrl_data = {
	.pins = sun252iw1_pins,
	.npins = ARRAY_SIZE(sun252iw1_pins),
	.irq_banks = ARRAY_SIZE(sun252iw1_irq_bank_map),
	.irq_bank_map = sun252iw1_irq_bank_map,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_CTL_V2,
	.pf_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
};

/* PINCTRL power management code */
#if IS_ENABLED(CONFIG_PM_SLEEP)

static void *mem;
static int mem_size;

static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mem_size = resource_size(res);

	if (mem)
		return -ENOMEM;
	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;
	return 0;
}

static int sun252iw1_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	sunxi_info(dev, "pinctrl suspend\n");

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_fromio(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int sun252iw1_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_toio(pctl->membase, mem, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	sunxi_info(dev, "pinctrl resume\n");

	return 0;
}

static const struct dev_pm_ops sun252iw1_pinctrl_pm_ops = {
	.suspend_noirq = sun252iw1_pinctrl_suspend_noirq,
	.resume_noirq = sun252iw1_pinctrl_resume_noirq,
};
#define PINCTRL_PM_OPS	(&sun252iw1_pinctrl_pm_ops)

#else
static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	return 0;
}
#define PINCTRL_PM_OPS	NULL
#endif

static int sun252iw1_pinctrl_probe(struct platform_device *pdev)
{
	int ret;
	ret = pinctrl_pm_alloc_mem(pdev);
	if (ret) {
		sunxi_err(&pdev->dev, "alloc pm mem err\n");
		return ret;
	}
	return sunxi_bsp_pinctrl_init(pdev, &sun252iw1_pinctrl_data);
}

static struct of_device_id sun252iw1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun252iw1-pinctrl", },
	{}
};

MODULE_DEVICE_TABLE(of, sun252iw1_pinctrl_match);

static struct platform_driver sun252iw1_pinctrl_driver = {
	.probe	= sun252iw1_pinctrl_probe,
	.driver	= {
		.name		= "sun252iw1-pinctrl",
		.pm		= PINCTRL_PM_OPS,
		.of_match_table	= sun252iw1_pinctrl_match,
	},
};

static int __init sun252iw1_pio_init(void)
{
	return platform_driver_register(&sun252iw1_pinctrl_driver);
}
fs_initcall(sun252iw1_pio_init);

MODULE_DESCRIPTION("Allwinner sun252iw1 pio pinctrl driver");
MODULE_AUTHOR("<weizhouxiang@allwinnertech>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");
