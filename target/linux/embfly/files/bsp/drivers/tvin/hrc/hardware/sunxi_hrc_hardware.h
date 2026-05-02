/* SPDX-License-Identifier: GPL-2.0-or-later */
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
#ifndef _SUNXI_HRC_HARDWARE_H_
#define _SUNXI_HRC_HARDWARE_H_

#include <linux/types.h>
#include <linux/platform_device.h>
#include "sunxi_hrc_define.h"

#if defined(HRC_VERSION_110)
#define HARDWARE_NAME	"HRC110"

#define MIN_WIDTH	16
#define MIN_HEIGHT	16
#define MAX_WIDTH	4096
#define MAX_HEIGHT	4096
#define ALIGN_WIDTH	0
#define ALIGN_HEIGHT	0

#define INPUT_NUM	1

#define REG_BITS	32
#define VAL_BITS	32
#define REG_STRIDE	4
#define MAX_REGISTER	0x600
#endif  /* defined(HRC_VERSION_110) */

int sunxi_hrc_hardware_enable(enum hrc_irq irq);
int sunxi_hrc_hardware_disable(void);
int sunxi_hrc_hardware_config(struct hrc_ctrl_param ctrl_param,
			      struct hrc_input_param in_param,
			      struct hrc_output_param out_param);
int sunxi_hrc_hardware_config_addr(struct hrc_addr out_addr);
int sunxi_hrc_hardware_config_ready(void);
int sunxi_hrc_hardware_get_irq_state(u32 *state);
int sunxi_hrc_hardware_clear_irq_state(u32 state);
int sunxi_hrc_hardware_get_irq_field(u32 *field);
int sunxi_hrc_hardware_reg_read(u32 reg, u32 *val);
int sunxi_hrc_hardware_reg_write(u32 reg, u32 val);
int sunxi_hrc_hardware_reg_read_mask(u32 reg, u32 mask, u32 *val);
int sunxi_hrc_hardware_reg_write_mask(u32 reg, u32 mask, u32 val);
int sunxi_hrc_hardware_reg_dump(char *buf, int size);
int sunxi_hrc_hardware_check_format_support(u32 input_format, u32 output_format);
int sunxi_hrc_hardware_init(struct platform_device *pdev);
int sunxi_hrc_hardware_exit(struct platform_device *pdev);

#endif  /* _SUNXI_HRC_HARDWARE_H_ */
