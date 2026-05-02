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
#ifndef _HRC_110_H_
#define _HRC_110_H_

/**
 * hrc110_enable() - Enable HRC110
 *
 * @irq: The IRQ you want to enable
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_enable(enum hrc_irq irq);

/**
 * hrc110_disable() - Disable HRC110
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_disable(void);

/**
 * hrc110_config() - Configure HRC110
 *
 * @ctrl_param: Control parameters to be set
 * @in_param: Input parameters to be set
 * @out_param: Output parameters to be set
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_config(struct hrc_ctrl_param ctrl_param,
		  struct hrc_input_param in_param,
		  struct hrc_output_param out_param);

/**
 * hrc110_config_addr() - Set the buffer address for loading data next time
 *
 * @out_addr: Buffer address
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_config_addr(struct hrc_addr out_addr);

/**
 * hrc110_config_ready() - Notify that the hardware configuration has been updated
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_config_ready(void);

/**
 * hrc110_get_irq_state() - Get irq flag from register
 *
 * @state: Pointer to store irq state
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_get_irq_state(u32 *state);

/**
 * hrc110_clr_irq_state() - Clear irq flag from register
 *
 * @state: The IRQ state you want to clear
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_clr_irq_state(u32 state);

/**
 * hrc110_get_irq_field() - Get field flag from register
 *
 * @irq: Pointer to store field flag
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_get_irq_field(u32 *field);

/**
 * hrc110_init() - Initialize hrc device
 *
 * @reg_base: Pointer of register base address
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_init(void __iomem *reg_base);

/**
 * hrc110_exit() - Deinitialize hrc device
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_exit(void);

/**
 * hrc110_read() - Read value from register
 *
 * @reg: Register offset
 * @val: Pointer to store value
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_read(u32 reg, u32 *val);

/**
 * hrc110_write() - Write value to register
 *
 * @reg: Register offset
 * @val: Value to write
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_write(u32 reg, u32 val);

/**
 * hrc110_read_mask() - Read value from register (mask)
 *
 * @reg: Register offset
 * @mask: Register mask
 * @val: Pointer to store value
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_read_mask(u32 reg, u32 mask, u32 *val);

/**
 * hrc110_write_mask() - Write value to register (mask)
 *
 * @reg: Register offset
 * @mask: Register mask
 * @val: Value to write
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_write_mask(u32 reg, u32 mask, u32 val);

/**
 * hrc110_reg_dump() - Dump register value to buffer
 *
 * @buf: Pointer of buffer to store value
 * @n: Buffer offset
 *
 * A value of size will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_reg_dump(char *buf, int n);

/**
 * hrc110_check_format_support() - Check whether the input and output mode is supported
 *
 * @input_format: Input format
 * @output_format: Output format
 *
 * A value of 1 will be returned on support input and output format,
 * 0 will be returned in not support.
 */
int hrc110_check_format_support(u32 input_format, u32 output_format);

#endif  /* _HRC_110_H_ */
