/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/regmap.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_dmic.h"

struct sunxi_dmic_clk {
	/* module clk */
	struct clk *clk_dmic;

	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

sunxi_dmic_clk_t *snd_dmic_clk_init(struct platform_device *pdev)
{
	(void)pdev;
	return NULL;
}

void snd_dmic_clk_exit(void *clk_orig)
{
	(void)clk_orig;
}

int snd_dmic_clk_bus_enable(void *clk_orig)
{
	(void)clk_orig;
	return 0;
}

int snd_dmic_clk_enable(void *clk_orig)
{
	(void)clk_orig;
	return 0;
}

void snd_dmic_clk_bus_disable(void *clk_orig)
{
	(void)clk_orig;
}

void snd_dmic_clk_disable(void *clk_orig)
{
	(void)clk_orig;
}

int snd_dmic_clk_rate(void *clk_orig, unsigned int freq_in, unsigned int freq_out)
{
	(void)clk_orig;
	(void)freq_in;
	(void)freq_out;
	return 0;
}
