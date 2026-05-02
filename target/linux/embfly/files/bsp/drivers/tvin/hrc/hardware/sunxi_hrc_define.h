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
#ifndef _SUNXI_HRC_DEFINE_H_
#define _SUNXI_HRC_DEFINE_H_

#include <linux/types.h>

struct hrc_size {
	u32 width;
	u32 height;
};

enum hrc_depth {
	HRC_DEPTH_8 = 0,
	HRC_DEPTH_10,
	HRC_DEPTH_12,
	HRC_DEPTH_16,
};

enum hrc_fmt {
	HRC_FMT_RGB = 0,
	HRC_FMT_YUV444,
	HRC_FMT_YUV422,
	HRC_FMT_YUV420,
	HRC_FMT_YUV444_1PLANE,
};

struct hrc_addr {
	u32 head[2];
	u32 body[2];
};

enum hrc_color_space {
	HRC_CSC_BT601 = 0,
	HRC_CSC_BT709,
	HRC_CSC_BT2020,
};

enum hrc_quantization {
	HRC_QUANTIZATION_DEFAULT = 0,
	HRC_QUANTIZATION_FULL,
	HRC_QUANTIZATION_LIMIT,
};

struct hrc_input_param {
	struct hrc_size       size;
	enum hrc_fmt          format;
	enum hrc_depth        depth;
	enum hrc_color_space  csc;
	enum hrc_quantization quantization;

	struct hrc_addr       ddr_addr;
};

struct hrc_output_param {
	struct hrc_size       y_size;
	struct hrc_size       c_size;
	enum hrc_fmt          format;
	u8                    uncompact;
	u32                   y_stride;
	u32                   c_stride;
	enum hrc_color_space  csc;
	enum hrc_quantization quantization;
};

enum hrc_data_src {
	HRC_DATA_SRC_HDMI_RX = 0,
	HRC_DATA_SRC_DDR,
};

enum hrc_field_mode {
	HRC_FIELD_MODE_FRAME = 0,
	HRC_FIELD_MODE_FIELD,
};

enum hrc_field_order {
	HRC_FIELD_ORDER_BOTTOM = 0,
	HRC_FIELD_ORDER_TOP,
};

struct hrc_ctrl_param {
	enum hrc_data_src    data_src;
	enum hrc_field_mode  field_mode;
	u8                   field_inverse;
	enum hrc_field_order field_order;
	u32                  timeout_cycle;
};

enum hrc_irq {
	HRC_IRQ_FRAME_VSYNC = BIT(0),
	HRC_IRQ_CFG_FINISH  = BIT(1),
	HRC_IRQ_WB_FINISH   = BIT(2),
	HRC_IRQ_OVERFLOW    = BIT(3),
	HRC_IRQ_TIMEOUT     = BIT(4),
	HRC_IRQ_UNUSUAL     = BIT(5),
	HRC_IRQ_ALL         = BIT(6),
};

#endif  /* _SUNXI_HRC_DEFINE_H_ */
