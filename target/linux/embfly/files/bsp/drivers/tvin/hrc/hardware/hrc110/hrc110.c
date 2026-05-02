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
#include "../../sunxi_hrc_log.h"
#include "../sunxi_hrc_define.h"
#include "hrc110.h"
#include "hrc110_reg.h"
#include "hrc110_table.h"

#include <linux/kernel.h>
#include <linux/io.h>

static void __iomem *hrc110_reg;

struct hrc_format_list {
	u32 input_format;
	u32 output_format_size;
	u32 output_format[16];
};

static struct hrc_format_list hrc110_support_format[] = {
	{
		.input_format = HRC_FMT_RGB,
		.output_format_size = 5,
		.output_format = {
			HRC_FMT_RGB, HRC_FMT_YUV444, HRC_FMT_YUV422,
			HRC_FMT_YUV420, HRC_FMT_YUV444_1PLANE,
		},
	}, {
		.input_format = HRC_FMT_YUV444,
		.output_format_size = 4,
		.output_format = {
			HRC_FMT_YUV444, HRC_FMT_YUV422, HRC_FMT_YUV420,
			HRC_FMT_YUV444_1PLANE,
		},
	}, {
		.input_format = HRC_FMT_YUV422,
		.output_format_size = 2,
		.output_format = {
			HRC_FMT_YUV422, HRC_FMT_YUV420,
		},
	}, {
		.input_format = HRC_FMT_YUV420,
		.output_format_size = 1,
		.output_format = {
			HRC_FMT_YUV420,
		},
	},
};

/**
 * hrc110_read() - Read value from register
 *
 * @reg: Register offset
 * @val: Pointer to store value
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_read(u32 reg, u32 *val)
{
	*val = readl(hrc110_reg + reg);
	return 0;
}

/**
 * hrc110_write() - Write value to register
 *
 * @reg: Register offset
 * @val: Value to write
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_write(u32 reg, u32 val)
{
	writel(val, hrc110_reg + reg);
	return 0;
}

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
int hrc110_read_mask(u32 reg, u32 mask, u32 *val)
{
	u32 reg_val = readl(hrc110_reg + reg);

	*val = (reg_val & mask) >> (ffs(mask) - 1);
	return 0;
}

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
int hrc110_write_mask(u32 reg, u32 mask, u32 val)
{
	u32 reg_val = readl(hrc110_reg + reg);

	reg_val &= ~(mask);
	reg_val |= val << (ffs(mask) - 1);
	writel(reg_val, hrc110_reg + reg);
	return 0;
}

/**
 * hrc110_reg_dump() - Dump register value to buffer
 *
 * @buf: Pointer of buffer to store value
 * @n: Buffer offset
 *
 * A value of size will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_reg_dump(char *buf, int n)
{
	int i;
	u32 val1, val2, val3, val4;

	for (i = 0; i < 0x100; i += 16) {
		hrc110_read(i + 0, &val1);
		hrc110_read(i + 4, &val2);
		hrc110_read(i + 8, &val3);
		hrc110_read(i + 12, &val4);
		n += sprintf(buf + n, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			     i, val1, val2, val3, val4);
	}

	for (i = 0x100; i < 0x180; i += 16) {
		hrc110_read(i + 0, &val1);
		hrc110_read(i + 4, &val2);
		hrc110_read(i + 8, &val3);
		hrc110_read(i + 12, &val4);
		n += sprintf(buf + n, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			     i, val1, val2, val3, val4);
	}

	for (i = 0x200; i < 0x280; i += 16) {
		hrc110_read(i + 0, &val1);
		hrc110_read(i + 4, &val2);
		hrc110_read(i + 8, &val3);
		hrc110_read(i + 12, &val4);
		n += sprintf(buf + n, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			     i, val1, val2, val3, val4);
	}

	for (i = 0x400; i < 0x480; i += 16) {
		hrc110_read(i + 0, &val1);
		hrc110_read(i + 4, &val2);
		hrc110_read(i + 8, &val3);
		hrc110_read(i + 12, &val4);
		n += sprintf(buf + n, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			     i, val1, val2, val3, val4);
	}

	for (i = 0x500; i < 0x580; i += 16) {
		hrc110_read(i + 0, &val1);
		hrc110_read(i + 4, &val2);
		hrc110_read(i + 8, &val3);
		hrc110_read(i + 12, &val4);
		n += sprintf(buf + n, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			     i, val1, val2, val3, val4);
	}

	return n;
}

/**
 * __hrc110_clr_irq_state() - Clear irq state flag
 *
 * @irq: IRQ Flag you want to clear
 *
 * return None
 */
static void __hrc110_clr_irq_state(u32 irq)
{
	u32 mask = 0;

	if ((irq & HRC_IRQ_FRAME_VSYNC) || (irq & HRC_IRQ_ALL))
		mask |= FRM_IRQ_MASK;

	if ((irq & HRC_IRQ_CFG_FINISH) || (irq & HRC_IRQ_ALL))
		mask |= CFG_FINISH_IRQ_MASK;

	if ((irq & HRC_IRQ_WB_FINISH) || (irq & HRC_IRQ_ALL))
		mask |= WB_FINISH_IRQ_MASK;

	if ((irq & HRC_IRQ_TIMEOUT) || (irq & HRC_IRQ_ALL))
		mask |= TIMEOUT_IRQ_MASK;

	if ((irq & HRC_IRQ_OVERFLOW) || (irq & HRC_IRQ_ALL))
		mask |= OVERFLOW_IRQ_MASK;

	if ((irq & HRC_IRQ_UNUSUAL) || (irq & HRC_IRQ_ALL))
		mask |= OVERFLOW_IRQ_MASK;

	hrc110_write(HRC_INT_STAT_REG, mask);
}

/**
 * __hrc110_set_irq_enable() - Enable/Disable HRC IRQ
 *
 * @irq: IRQ you want to enable/disable
 * @enable: Enable (1) / Disable (0)
 *
 * return None
 */
static void __hrc110_set_irq_enable(enum hrc_irq irq, u8 enable)
{
	u32 mask = 0;

	if (!enable) {
		hrc110_write(HRC_INT_CTRL_REG, 0);
		return;
	}

	/* if ((irq & HRC_IRQ_FRAME_VSYNC) || (irq & HRC_IRQ_ALL)) */
	/*         mask |= FRM_IRQ_EN_MASK; */

	if ((irq & HRC_IRQ_CFG_FINISH) || (irq & HRC_IRQ_ALL))
		mask |= CFG_FINISH_IRQ_EN_MASK;

	if ((irq & HRC_IRQ_WB_FINISH) || (irq & HRC_IRQ_ALL))
		mask |= WB_FINISH_IRQ_EN_MASK;

	if ((irq & HRC_IRQ_TIMEOUT) || (irq & HRC_IRQ_ALL))
		mask |= TIMEOUT_IRQ_EN_MASK;

	if ((irq & HRC_IRQ_OVERFLOW) || (irq & HRC_IRQ_ALL))
		mask |= OVERFLOW_IRQ_EN_MASK;

	if ((irq & HRC_IRQ_UNUSUAL) || (irq & HRC_IRQ_ALL))
		mask |= UNUSUAL_IRQ_EN_MASK;

	hrc110_write(HRC_INT_CTRL_REG, mask);
}

/**
 * __hrc110_config_csc_const_coeff() - Configure HRC CSC value
 *
 * @idx: Index of table (see @hrc110_csc_tables)
 *
 * return None
 */
static void __hrc110_config_csc_const_coeff(u32 idx)
{
	int i, j;

	if (idx >= ARRAY_SIZE(hrc110_csc_tables))
		return;

	for (i = 0; i < CSC_COEFF_CONST_LEN + CSC_CONSTANT_LEN; i++) {
		if (i < CSC_COEFF_CONST_LEN) {
			if (!((i + 1) % 4))
				hrc110_write_mask(HRC_CSC_COEFF_XX_REG(i),
						  CSC_CX3_MASK,
						  hrc110_csc_tables[idx][i]);
			else
				hrc110_write_mask(HRC_CSC_COEFF_XX_REG(i),
						  CSC_CXX_MASK,
						  hrc110_csc_tables[idx][i]);
		} else {
			j = i - CSC_COEFF_CONST_LEN;
			hrc110_write_mask(HRC_CSC_DX_REG(j),
					  CSC_DX_MASK,
					  hrc110_csc_tables[idx][i]);
		}
	}
}

/**
 * __hrc110_vsu_calc_fir_coef() - Calculate offset of coeff table
 *
 * @step: horizontal scale ratio of vsu
 *
 * return offset (in word) of coefficient table
 */
static u32 __hrc110_vsu_calc_fir_coef(u32 step)
{
	u32 pt_coef;
	u32 scale_ratio, int_part, float_part, fir_coef_ofst;

	scale_ratio = step >> (SCALER_Y_HSTEP_INT_OFFSET - 3);  /* Y == C */
	int_part = scale_ratio >> 3;
	float_part = scale_ratio & 0x7;
	if (int_part == 0)
		fir_coef_ofst = VSU_ZOOM0_SIZE;
	else if (int_part == 1)
		fir_coef_ofst = VSU_ZOOM0_SIZE + float_part;
	else if (int_part == 2)
		fir_coef_ofst =
		    VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE + (float_part >> 1);
	else if (int_part == 3)
		fir_coef_ofst =
		    VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE + VSU_ZOOM2_SIZE;
	else if (int_part == 4)
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
				VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE;
	else
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
				VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE +
				VSU_ZOOM4_SIZE;

	pt_coef = fir_coef_ofst * VSU_PHASE_NUM;

	return pt_coef;
}

/**
 * hrc110_enable() - Enable HRC110
 *
 * @irq: The IRQ you want to enable
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_enable(enum hrc_irq irq)
{
	u32 val;

	__hrc110_set_irq_enable(irq, 1);

	hrc110_write_mask(HRC_TOP_REG, TOP_RESET_MASK, 0);
	hrc110_write_mask(HRC_TOP_REG, SRAM_GATE_MODE_MASK, 0);
	hrc110_write_mask(HRC_TOP_REG, MBUS_CLK_EN_MASK, 1);

	hrc110_read_mask(HRC_CTRL_REG, DATA_SRC_SEL_MASK, &val);
	if (val == DATA_SRC_DDR)
		hrc110_write_mask(HRC_TOP_REG, SELF_VSYN_START_MASK, 1);

	return 0;
}

/**
 * hrc110_disable() - Disable HRC110
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_disable(void)
{
	hrc110_write_mask(HRC_CTRL_REG, HRC_EN_MASK, 0);

	__hrc110_set_irq_enable(HRC_IRQ_ALL, 0);

	__hrc110_clr_irq_state(HRC_IRQ_ALL);

	return 0;
}

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
		  struct hrc_output_param out_param)
{
	int i;
	u64 step;
	u32 pt_coef;
	u32 index;
	u8 csc_en = 0;
	u8 y_hscaler_en = 0;
	u8 c_hscaler_en = 0;
	u32 input_quantization = 0;
	u32 output_quantization = 0;

	hrc110_write_mask(HRC_CTRL_REG, HRC_EN_MASK, 1);

	/* data src */
	if (ctrl_param.data_src == HRC_DATA_SRC_DDR) {
		hrc110_write_mask(HRC_CTRL_REG, DATA_SRC_SEL_MASK, DATA_SRC_DDR);

		hrc110_write(HRC_IN_Y_BUF_ADDR_REG, in_param.ddr_addr.head[0]);
		hrc110_write(HRC_IN_C_BUF_ADDR_REG, in_param.ddr_addr.body[0]);

		hrc110_write_mask(HRC_COUNT_REG, RESET_CYCLE_MASK, 0x20);
		hrc110_write_mask(HRC_COUNT_REG, SELF_SYN_LEN_MASK, 0x20);
	} else {
		hrc110_write_mask(HRC_CTRL_REG, DATA_SRC_SEL_MASK, DATA_SRC_HDMI_RX);
	}

	/* field */
	if (ctrl_param.field_mode == HRC_FIELD_MODE_FIELD)
		hrc110_write_mask(HRC_CTRL_REG, FIELD_MODE_MASK, FIELD_MODE_FIELD);
	else
		hrc110_write_mask(HRC_CTRL_REG, FIELD_MODE_MASK, FIELD_MODE_FRAME);

	hrc110_write_mask(HRC_CTRL_REG, FIELD_INVERSE_MASK, ctrl_param.field_inverse);

	if (ctrl_param.field_order == HRC_FIELD_ORDER_TOP)
		hrc110_write_mask(HRC_CTRL_REG, FIELD_ORDER_MASK, FIELD_ORDER_TOP);
	else
		hrc110_write_mask(HRC_CTRL_REG, FIELD_ORDER_MASK, FIELD_ORDER_BOTTOM);

	/* ctrl */
	if (out_param.format == HRC_FMT_YUV444_1PLANE) {
		if (in_param.format == HRC_FMT_YUV444) {
			csc_en = 0;
			hrc110_write_mask(HRC_CTRL_REG, CSC_EN_MASK, 0);
		} else {
			csc_en = 1;
			hrc110_write_mask(HRC_CTRL_REG, CSC_EN_MASK, 1);
		}
	} else {
		if (in_param.format != out_param.format && in_param.format == HRC_FMT_RGB) {
			csc_en = 1;
			hrc110_write_mask(HRC_CTRL_REG, CSC_EN_MASK, 1);
		} else {
			csc_en = 0;
			hrc110_write_mask(HRC_CTRL_REG, CSC_EN_MASK, 0);
		}
	}

	if (in_param.format != HRC_FMT_YUV420 && out_param.format == HRC_FMT_YUV420)
		hrc110_write_mask(HRC_CTRL_REG, C_VSAMPLE_EN_MASK, 1);
	else
		hrc110_write_mask(HRC_CTRL_REG, C_VSAMPLE_EN_MASK, 0);

	if (in_param.size.width != out_param.y_size.width) {
		y_hscaler_en = 1;
		hrc110_write_mask(HRC_CTRL_REG, Y_HSCALER_EN_MASK, 1);
	} else {
		y_hscaler_en = 0;
		hrc110_write_mask(HRC_CTRL_REG, Y_HSCALER_EN_MASK, 0);
	}

	if (in_param.size.width != out_param.c_size.width) {
		c_hscaler_en = 1;
		hrc110_write_mask(HRC_CTRL_REG, C_HSCALER_EN_MASK, 1);
	} else {
		c_hscaler_en = 0;
		hrc110_write_mask(HRC_CTRL_REG, C_HSCALER_EN_MASK, 0);
	}

	hrc110_write_mask(HRC_READY_REG, TIMEOUT_CYCLE_MASK, ctrl_param.timeout_cycle);

	/* input */
	hrc110_write_mask(HRC_SIZE_REG, SRC_WIDTH_MASK, in_param.size.width - 1);
	hrc110_write_mask(HRC_SIZE_REG, SRC_HEIGHT_MASK, in_param.size.height - 1);

	if (in_param.format == HRC_FMT_YUV444)
		hrc110_write_mask(HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_YUV444);
	else if (in_param.format == HRC_FMT_YUV422)
		hrc110_write_mask(HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_YUV422);
	else if (in_param.format == HRC_FMT_YUV420)
		hrc110_write_mask(HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_YUV420);
	else
		hrc110_write_mask(HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_RGB);

	if (in_param.depth == HRC_DEPTH_10)
		hrc110_write_mask(HRC_FMT_REG, BIT_DEPTH_MASK, DEPTH_10);
	if (in_param.depth == HRC_DEPTH_12)
		hrc110_write_mask(HRC_FMT_REG, BIT_DEPTH_MASK, DEPTH_12);
	else
		hrc110_write_mask(HRC_FMT_REG, BIT_DEPTH_MASK, DEPTH_8);

	if (in_param.depth == HRC_DEPTH_10)
		hrc110_write_mask(HRC_FMT_REG, OUT_UNCOMPACT_MASK, out_param.uncompact);

	/* output */
	hrc110_write_mask(HRC_SCALER_Y_SIZE_REG, SCALER_Y_OUT_WIDTH_MASK,
			  out_param.y_size.width - 1);
	hrc110_write_mask(HRC_SCALER_Y_SIZE_REG, SCALER_Y_OUT_HEIGHT_MASK,
			  out_param.y_size.height - 1);
	hrc110_write_mask(HRC_SCALER_C_SIZE_REG, SCALER_C_OUT_WIDTH_MASK,
			  out_param.c_size.width - 1);
	hrc110_write_mask(HRC_SCALER_C_SIZE_REG, SCALER_C_OUT_HEIGHT_MASK,
			  out_param.c_size.height - 1);

	if (out_param.format == HRC_FMT_YUV444_1PLANE) {
		hrc110_write_mask(HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_RGB);
	} else {
		if (out_param.format == HRC_FMT_YUV444)
			hrc110_write_mask(HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_YUV444);
		else if (out_param.format == HRC_FMT_YUV422)
			hrc110_write_mask(HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_YUV422);
		else if (out_param.format == HRC_FMT_YUV420)
			hrc110_write_mask(HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_YUV420);
		else
			hrc110_write_mask(HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_RGB);
	}

	hrc110_write_mask(HRC_OUT_BUF_STRIDE_REG, OUTPUT_Y_STRIDE_MASK, out_param.y_stride);
	hrc110_write_mask(HRC_OUT_BUF_STRIDE_REG, OUTPUT_C_STRIDE_MASK, out_param.c_stride);

	hrc110_write_mask(HRC_RSC_REG, RSC_SEED_MASK, 0);
	hrc110_write_mask(HRC_RSC_REG, RSC_EN_MASK, 0);

	hrc_dbg("csc_en: %d y_hscaler_en: %d c_hscaler_en: %d\n",
		csc_en, y_hscaler_en, c_hscaler_en);

	/* csc */
	if (csc_en) {
		input_quantization = in_param.quantization;
		output_quantization = out_param.quantization;

		if (input_quantization == HRC_QUANTIZATION_DEFAULT) {
			if (in_param.format == HRC_FMT_RGB)
				input_quantization = HRC_QUANTIZATION_FULL;
			else
				input_quantization = HRC_QUANTIZATION_LIMIT;
		}
		if (output_quantization == HRC_QUANTIZATION_DEFAULT) {
			if (out_param.format == HRC_FMT_RGB)
				output_quantization = HRC_QUANTIZATION_FULL;
			else
				output_quantization = HRC_QUANTIZATION_LIMIT;
		}

		hrc_dbg("inquan: %d outquan: %d incsc: %d outcsc: %d\n",
			input_quantization, output_quantization,
			in_param.csc, out_param.csc);

		if (input_quantization == HRC_QUANTIZATION_FULL &&
		    output_quantization == HRC_QUANTIZATION_LIMIT) {
			if (in_param.csc == HRC_CSC_BT601 &&
			    out_param.csc == HRC_CSC_BT601)
				index = CSC_FULL_RGB_601_LIMIT_YUV_601;
			else if (in_param.csc == HRC_CSC_BT709 &&
				 out_param.csc == HRC_CSC_BT709)
				index = CSC_FULL_RGB_709_LIMIT_YUV_709;
			else if (in_param.csc == HRC_CSC_BT2020 &&
				 out_param.csc == HRC_CSC_BT2020)
				index = CSC_FULL_RGB_2020_LIMIT_YUV_2020;
		} else if (input_quantization == HRC_QUANTIZATION_LIMIT &&
			   output_quantization == HRC_QUANTIZATION_LIMIT) {
			if (in_param.csc == HRC_CSC_BT601 &&
			    out_param.csc == HRC_CSC_BT601)
				index = CSC_LIMIT_RGB_601_LIMIT_YUV_601;
			else if (in_param.csc == HRC_CSC_BT709 &&
				 out_param.csc == HRC_CSC_BT709)
				index = CSC_LIMIT_RGB_709_LIMIT_YUV_709;
			else if (in_param.csc == HRC_CSC_BT2020 &&
				 out_param.csc == HRC_CSC_BT2020)
				index = CSC_LIMIT_RGB_2020_LIMIT_YUV_2020;
		} else if (input_quantization == HRC_QUANTIZATION_FULL &&
			   output_quantization == HRC_QUANTIZATION_FULL) {
			if (in_param.csc == HRC_CSC_BT601 &&
			    out_param.csc == HRC_CSC_BT601)
				index = CSC_FULL_RGB_601_FULL_YUV_601;
			else if (in_param.csc == HRC_CSC_BT709 &&
				 out_param.csc == HRC_CSC_BT709)
				index = CSC_FULL_RGB_709_FULL_YUV_709;
			else if (in_param.csc == HRC_CSC_BT2020 &&
				 out_param.csc == HRC_CSC_BT2020)
				index = CSC_FULL_RGB_2020_FULL_YUV_2020;
		}

		hrc_dbg("csc index: %d\n", index);
		__hrc110_config_csc_const_coeff(index);
	}

	/* scaler */
	if (y_hscaler_en) {
		step = (u64)in_param.size.width << SCALER_Y_HSTEP_INT_OFFSET;

		do_div(step, out_param.y_size.width);

		hrc110_write(HRC_SCALER_Y_HSTEP_REG, (u32)step);

		/* FIXME */
		hrc110_write(HRC_SCALER_Y_HPHASE_REG, 0);

		pt_coef = __hrc110_vsu_calc_fir_coef((u32)step);
		hrc_dbg("y_hscaler pt_coef: %d\n", pt_coef);
		for (i = 0; i < VSU_PHASE_NUM; i++) {
			hrc110_write(HRC_VSU_Y_COEFF_0_REG + i * 4,
				     lan3coefftab32_left[pt_coef + i]);
			hrc110_write(HRC_VSU_Y_COEFF_1_REG + i * 4,
				     lan3coefftab32_right[pt_coef + i]);
		}
	}

	if (c_hscaler_en) {
		step = (u64)in_param.size.width << SCALER_C_HSTEP_INT_OFFSET;

		do_div(step, out_param.c_size.width);

		hrc110_write(HRC_SCALER_C_HSTEP_REG, (u32)step);

		/* FIXME */
		hrc110_write(HRC_SCALER_C_HPHASE_REG, 0);

		pt_coef = __hrc110_vsu_calc_fir_coef((u32)step);
		hrc_dbg("c_hscaler pt_coef: %d\n", pt_coef);
		if (in_param.format == HRC_FMT_RGB) {
			for (i = 0; i < VSU_PHASE_NUM; i++) {
				hrc110_write(HRC_VSU_C_COEFF_0_REG + i * 4,
					     lan3coefftab32_left[pt_coef + i]);
				hrc110_write(HRC_VSU_C_COEFF_1_REG + i * 4,
					     lan3coefftab32_right[pt_coef + i]);
			}
		} else {
			for (i = 0; i < VSU_PHASE_NUM; i++) {
				hrc110_write(HRC_VSU_C_COEFF_0_REG + i * 4,
					     bicubic8coefftab32_left[pt_coef + i]);
				hrc110_write(HRC_VSU_C_COEFF_1_REG + i * 4,
					     bicubic8coefftab32_right[pt_coef + i]);
			}
		}
	}

	return 0;
}

/**
 * hrc110_config_addr() - Set the buffer address for loading data next time
 *
 * @out_addr: Buffer address
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_config_addr(struct hrc_addr out_addr)
{
	hrc110_write(HRC_OUT_Y_BUF_LADDR_REG, out_addr.head[0]);
	hrc110_write_mask(HRC_OUT_Y_BUF_HADDR_REG, OUTPUT_Y_BUF_HADDR_MASK,
			  out_addr.head[1]);

	hrc110_write(HRC_OUT_C_BUF_LADDR_REG, out_addr.body[0]);
	hrc110_write_mask(HRC_OUT_C_BUF_HADDR_REG, OUTPUT_C_BUF_HADDR_MASK,
			  out_addr.body[1]);

	return 0;
}

/**
 * hrc110_config_ready() - Notify that the hardware configuration has been updated
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_config_ready(void)
{
	hrc110_write_mask(HRC_READY_REG, REG_RDY_MASK, 1);
	return 0;
}

/**
 * hrc110_get_irq_state() - Get irq flag from register
 *
 * @state: Pointer to store irq state
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_get_irq_state(u32 *state)
{
	u32 val = 0;

	if (!state)
		return -1;

	hrc110_read(HRC_INT_STAT_REG, &val);

	*state = 0;

	if (val & FRM_IRQ_MASK)
		*state |= HRC_IRQ_FRAME_VSYNC;

	if (val & CFG_FINISH_IRQ_MASK)
		*state |= HRC_IRQ_CFG_FINISH;

	if (val & WB_FINISH_IRQ_MASK)
		*state |= HRC_IRQ_WB_FINISH;

	if (val & TIMEOUT_IRQ_MASK)
		*state |= HRC_IRQ_TIMEOUT;

	if (val & OVERFLOW_IRQ_MASK)
		*state |= HRC_IRQ_OVERFLOW;

	if (val & UNUSUAL_IRQ_MASK)
		*state |= HRC_IRQ_UNUSUAL;

	return 0;
}

/**
 * hrc110_clr_irq_state() - Clear irq flag from register
 *
 * @state: The IRQ state you want to clear
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_clr_irq_state(u32 state)
{
	if (!state)
		return 0;

	__hrc110_clr_irq_state(state);

	return 0;
}

/**
 * hrc110_get_irq_field() - Get field flag from register
 *
 * @irq: Pointer to store field flag
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_get_irq_field(u32 *field)
{
	hrc110_read_mask(HRC_INT_STAT_REG, FIELD_FLAG_MASK, field);
	return 0;
}

/**
 * hrc110_check_format_support() - Check whether the input and output mode is supported
 *
 * @input_format: Input format
 * @output_format: Output format
 *
 * A value of 1 will be returned on support input and output format,
 * 0 will be returned in not support.
 */
int hrc110_check_format_support(u32 input_format, u32 output_format)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(hrc110_support_format); i++) {
		if (input_format != hrc110_support_format[i].input_format)
			continue;

		for (j = 0; j < hrc110_support_format[i].output_format_size; j++)
			if (output_format == hrc110_support_format[i].output_format[j])
				return 1;
	}

	return 0;
}

/**
 * hrc110_init() - Initialize hrc device
 *
 * @reg_base: Pointer of register base address
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_init(void __iomem *reg_base)
{
	if (!reg_base)
		return -1;

	hrc110_reg = reg_base;

	hrc110_write_mask(HRC_TOP_REG, TOP_RESET_MASK, 0);

	return 0;
}

/**
 * hrc110_exit() - Deinitialize hrc device
 *
 * @pdev: Pointer of platform device
 *
 * A value of zero will be returned on success,
 * a negative errno will be returned in error cases.
 */
int hrc110_exit(void)
{
	return 0;
}
