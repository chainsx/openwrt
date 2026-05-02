// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 huangzhenwei@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun251iw1.h"

/* ccu_des_start */
#define SUN251IW1_PLL_DDR_CTRL_REG   0x0010
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0010,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN251IW1_PLL_PERI_CTRL_REG   0x0020
static struct ccu_nkmp pll_peri_parent_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-peri-parent", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri_2x_clk, "pll-peri-2x",
		"pll-peri-parent", 0x020, 16, 3, 0);

static SUNXI_CCU_M(pll_peri_800m_clk, "pll-peri-800m",
		"pll-peri-parent", 0x020, 20, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_1x_clk, "pll-peri-1x",
		&pll_peri_2x_clk.common.hw,
			2, 1, 0);

#define SUN251IW1_PLL_VIDEO0_CTRL_REG   0x0040
static struct ccu_nkmp pll_video0_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0040,
		.hw.init	= CLK_HW_INIT("pll-video0", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN251IW1_PLL_VIDEO1_CTRL_REG   0x0048
static struct ccu_nkmp pll_video1_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0048,
		.hw.init	= CLK_HW_INIT("pll-video1", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN251IW1_PLL_VE_CTRL_REG   0x0058
static struct ccu_nkmp pll_ve_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0058,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN251IW1_PLL_AUDIO1_CTRL_REG   0x0080
static struct ccu_nkmp pll_audio1_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(16, 6),
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0080,
		.hw.init	= CLK_HW_INIT("pll-audio1", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static const char * const psi_parents[] = { "dcxo24M", "clk32k", "clk16m-rc", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX(psi_clk, "psi",
		psi_parents,
		0x0510,
		0, 2,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const apb0_parents[] = { "dcxo24M", "clk32k", "psi-clk", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX(apb0_clk, "apb0",
		apb0_parents,
		0x0520,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const apb1_parents[] = { "dcxo24M", "clk32k", "psi-clk", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX(apb1_clk, "apb1",
		apb1_parents,
		0x0524,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const de_parents[] = { "pll-peri-2x", "video0pll4x", "video1pll4x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
		de_parents, 0x0600,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(de_bus_clk, "de-bus",
		"dcxo24M",
		0x060C, BIT(0), 0);

static SUNXI_CCU_GATE(ksc_clk, "ksc",
		"dcxo24M",
		0x061C, BIT(0), 0);

static const char * const di_parents[] = { "pll-peri-2x", "video0pll4x", "video1pll4x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(di_clk, "di",
		di_parents, 0x0620,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(di_bus_clk, "di-bus",
		"dcxo24M",
		0x062C, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri-2x", "video0pll4x", "video1pll4x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x0630,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(g2d_bus_clk, "g2d-bus",
		"dcxo24M",
		0x063C, BIT(0), 0);

static const char * const ce_parents[] = { "dcxo24M", "pll-peri-2x", "pll-peri-1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce",
		ce_parents, 0x0680,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus",
		"dcxo24M",
		0x068C, BIT(0), 0);

static const char * const ve_parents[] = { "vepll", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_clk, "ve",
		ve_parents, 0x0690,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ve_bus_clk, "ve-bus",
		"dcxo24M",
		0x069C, BIT(0), 0);

static SUNXI_CCU_GATE(dma_clk, "dma",
		"dcxo24M",
		0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock",
		"dcxo24M",
		0x072C, BIT(0), 0);

static SUNXI_CCU_GATE(hstimer_clk, "hstimer",
		"dcxo24M",
		0x073C, BIT(0), 0);

static SUNXI_CCU_GATE(avs_clk, "avs",
		"dcxo24M",
		0x0740, BIT(31), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo24M",
		0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm_clk, "pwm",
		"dcxo24M",
		0x07AC, BIT(0), 0);

static const char * const dram_parents[] = { "ddrpll", "audio1pll-div2", "pll-peri-2x", "peripll-800m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(dram_clk, "dram",
		dram_parents, 0x0800,
		0, 2,			/* M */
		8, 2,			/* P */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(dram_bus_clk, "dram-bus",
		"dcxo24M",
		0x080C, BIT(0), 0);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(smhc0_clk, "smhc0",
		smhc0_parents, 0x0830,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(smhc1_clk, "smhc1",
		smhc1_parents, 0x0834,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const smhc2_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "peripll-800m", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(smhc2_clk, "smhc2",
		smhc2_parents, 0x0838,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(smhc2_bus_clk, "smhc2-bus",
		"dcxo24M",
		0x084C, BIT(2), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
		"dcxo24M",
		0x084C, BIT(1), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
		"dcxo24M",
		0x084C, BIT(0), 0);

static SUNXI_CCU_GATE(uart5_clk, "uart5",
		"dcxo24M",
		0x090C, BIT(5), 0);

static SUNXI_CCU_GATE(uart4_clk, "uart4",
		"dcxo24M",
		0x090C, BIT(4), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"dcxo24M",
		0x090C, BIT(3), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"dcxo24M",
		0x090C, BIT(2), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"dcxo24M",
		0x090C, BIT(1), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"dcxo24M",
		0x090C, BIT(0), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
		"dcxo24M",
		0x091C, BIT(3), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"dcxo24M",
		0x091C, BIT(2), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"dcxo24M",
		0x091C, BIT(1), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"dcxo24M",
		0x091C, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(spi0_clk, "spi0",
		spi0_parents, 0x0940,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(spi1_clk, "spi1",
		spi1_parents, 0x0944,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
		"dcxo24M",
		0x096C, BIT(1), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
		"dcxo24M",
		0x096C, BIT(0), 0);

static const char * const gmac0_phy_parents[] = { "gmac-25m-clk", "gmac-50m-clk" };

static SUNXI_CCU_MUX_WITH_GATE(gmac0_phy_clk, "gmac0-phy",
		gmac0_phy_parents, 0x0970,
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const gmac1_phy_parents[] = { "gmac-25m-clk", "gmac-50m-clk" };

static SUNXI_CCU_MUX_WITH_GATE(gmac1_phy_clk, "gmac1-phy",
		gmac1_phy_parents, 0x0974,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(gmac1_clk, "gmac1",
		"dcxo24M",
		0x097C, BIT(1), 0);

static SUNXI_CCU_GATE(gmac0_clk, "gmac0",
		"dcxo24M",
		0x097C, BIT(0), 0);

static const char * const irtx_parents[] = { "dcxo24M", "pll-peri-1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(irtx_clk, "irtx",
		irtx_parents, 0x09C0,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(irtx_bus_clk, "irtx-bus",
		"dcxo24M",
		0x09CC, BIT(0), 0);

static SUNXI_CCU_GATE(gpadc_clk, "gpadc",
		"dcxo24M",
		0x09EC, BIT(0), 0);

static SUNXI_CCU_GATE(ths_clk, "ths",
		"dcxo24M",
		0x09FC, BIT(0), 0);

static const char * const i2s0_parents[] = { "audio0pll1x", "audio0pll4x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s0_clk, "i2s0",
		i2s0_parents, 0x0A10,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const i2s1_parents[] = { "audio0pll1x", "audio0pll4x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s1_clk, "i2s1",
		i2s1_parents, 0x0A14,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const i2s2_parents[] = { "audio0pll1x", "audio0pll4x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s2_clk, "i2s2",
		i2s2_parents, 0x0A18,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s2_bus_clk, "i2s2-bus",
		"dcxo24M",
		0x0A20, BIT(2), 0);

static SUNXI_CCU_GATE(i2s1_bus_clk, "i2s1-bus",
		"dcxo24M",
		0x0A20, BIT(1), 0);

static SUNXI_CCU_GATE(i2s0_bus_clk, "i2s0-bus",
		"dcxo24M",
		0x0A20, BIT(0), 0);

static const char * const owa_tx_parents[] = { "audio0pll1x", "audio0pll4x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa_tx_clk, "owa-tx",
		owa_tx_parents, 0x0A24,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const owa_rx_parents[] = { "pll-peri-1x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(owa_rx_clk, "owa-rx",
		owa_rx_parents, 0x0A28,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(owa_clk, "owa",
		"dcxo24M",
		0x0A2C, BIT(0), 0);

static const char * const dmic_parents[] = { "audio0pll1x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(dmic_clk, "dmic",
		dmic_parents, 0x0A40,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
		"dcxo24M",
		0x0A4C, BIT(0), 0);

static const char * const audio_codec_dac_parents[] = { "audio0pll1x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_dac_clk, "audio-codec-dac",
		audio_codec_dac_parents, 0x0A50,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const audio_codec_adc_parents[] = { "audio0pll1x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_adc_clk, "audio-codec-adc",
		audio_codec_adc_parents, 0x0A54,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(audio_codec_clk, "audio-codec",
		"dcxo24M",
		0x0A5C, BIT(0), 0);

static SUNXI_CCU_GATE(usb_clk, "usb",
		"dcxo24M",
		0x0A70, BIT(31), 0);

static SUNXI_CCU_GATE(usb1_clk, "usb1",
		"dcxo24M",
		0x0A74, BIT(31), 0);

static SUNXI_CCU_GATE(usbotg0_clk, "usbotg0",
		"dcxo24M",
		0x0A8C, BIT(8), 0);

static SUNXI_CCU_GATE(usbehci1_clk, "usbehci1",
		"dcxo24M",
		0x0A8C, BIT(5), 0);

static SUNXI_CCU_GATE(usbehci0_clk, "usbehci0",
		"dcxo24M",
		0x0A8C, BIT(4), 0);

static SUNXI_CCU_GATE(usbohci1_clk, "usbohci1",
		"dcxo24M",
		0x0A8C, BIT(1), 0);

static SUNXI_CCU_GATE(usbohci0_clk, "usbohci0",
		"dcxo24M",
		0x0A8C, BIT(0), 0);

static SUNXI_CCU_GATE(lradc_clk, "lradc",
		"dcxo24M",
		0x0A9C, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top_clk, "dpss-top",
		"dcxo24M",
		0x0ABC, BIT(0), 0);

static const char * const hdmi_rx_dtl_parents[] = { "pll-peri-1x", "video0pll2x", "video1pll2x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(hdmi_rx_dtl_clk, "hdmi-rx-dtl",
		hdmi_rx_dtl_parents, 0x0B00,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const hdmi_rx_cb_parents[] = { "video0pll1x", "video0pll4x", "video1pll1x", "video1pll4x", "pll-peri-2x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(hdmi_rx_cb_clk, "hdmi-rx-cb",
		hdmi_rx_cb_parents, 0x0B04,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const hdmi_cec_parents[] = { "clk32k", "hdmi-cec-clk32k" };

static SUNXI_CCU_MUX_WITH_GATE(hdmi_cec_clk, "hdmi-cec",
		hdmi_cec_parents, 0x0B08,
		24, 1,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(hdmi_rx_clk, "hdmi-rx",
		"dcxo24M",
		0x0B0C, BIT(0), 0);

static const char * const hrc_parents[] = { "dcxo24M", "pll-peri-2x", "video0pll4x", "video1pll4x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(hrc_clk, "hrc",
		hrc_parents, 0x0B10,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(hrc_bus_clk, "hrc-bus",
		"dcxo24M",
		0x0B1C, BIT(0), 0);

static const char * const dsi_parents[] = { "dcxo24M", "pll-peri-1x", "video0pll2x", "video1pll2x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi_clk, "dsi",
		dsi_parents, 0x0B24,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dsi_bus_clk, "dsi-bus",
		"dcxo24M",
		0x0B4C, BIT(0), 0);

static const char * const tconlcd_parents[] = { "video0pll1x", "video0pll4x", "video1pll1x", "video1pll4x", "pll-peri-2x", "audio1pll-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(tconlcd_clk, "tconlcd",
		tconlcd_parents, 0x0B60,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(tconlcd_bus_clk, "tconlcd-bus",
		"dcxo24M",
		0x0B7C, BIT(0), 0);

static const char * const ledc_parents[] = { "dcxo24M", "pll-peri-1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(ledc_clk, "ledc",
		ledc_parents, 0x0BF0,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ledc_bus_clk, "ledc-bus",
		"dcxo24M",
		0x0BFC, BIT(0), 0);

static const char * const csi_parents[] = { "pll-peri-2x", "video0pll2x", "video1pll2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
		csi_parents, 0x0C04,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const csi_master_parents[] = { "dcxo24M", "pll-peri-1x", "video0pll1x", "video1pll1x", "audio1pll-div2", "audio1pll-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master_clk, "csi-master",
		csi_master_parents, 0x0C08,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(csi_bus_clk, "csi-bus",
		"dcxo24M",
		0x0C1C, BIT(0), 0);

static const char * const tpadc_parents[] = { "dcxo24M", "audio0pll1x" };

static SUNXI_CCU_MUX_WITH_GATE(tpadc_clk, "tpadc",
		tpadc_parents, 0x0C50,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(tpadc_bus_clk, "tpadc-bus",
		"dcxo24M",
		0x0C5C, BIT(0), 0);

static SUNXI_CCU_GATE(riscv_cfg_clk, "riscv-cfg",
		"dcxo24M",
		0x0D0C, BIT(0), 0);
/* ccu_des_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun251iw1_hw_clks = {
	.hws	= {
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI_PARENT]		= &pll_peri_parent_clk.common.hw,
		[CLK_PLL_PERI_2X]		= &pll_peri_2x_clk.common.hw,
		[CLK_PLL_PERI_800M]		= &pll_peri_800m_clk.common.hw,
		[CLK_PLL_PERI_1X]		= &pll_peri_1x_clk.hw,
		[CLK_PLL_VIDEO0]		= &pll_video0_clk.common.hw,
		[CLK_PLL_VIDEO1]		= &pll_video1_clk.common.hw,
		[CLK_PLL_VE]			= &pll_ve_clk.common.hw,
		[CLK_PLL_AUDIO1]		= &pll_audio1_clk.common.hw,
		[CLK_PSI]			= &psi_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_BUS_DE]			= &de_bus_clk.common.hw,
		[CLK_KSC]			= &ksc_clk.common.hw,
		[CLK_DI]			= &di_clk.common.hw,
		[CLK_BUS_DI]			= &di_bus_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_BUS_G2D]			= &g2d_bus_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_BUS_VE]			= &ve_bus_clk.common.hw,
		[CLK_DMA]			= &dma_clk.common.hw,
		[CLK_SPINLOCK]			= &spinlock_clk.common.hw,
		[CLK_HSTIMER]			= &hstimer_clk.common.hw,
		[CLK_AVS]			= &avs_clk.common.hw,
		[CLK_DBGSYS]			= &dbgsys_clk.common.hw,
		[CLK_PWM]			= &pwm_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_BUS_DRAM]			= &dram_bus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_bus_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_UART5]			= &uart5_clk.common.hw,
		[CLK_UART4]			= &uart4_clk.common.hw,
		[CLK_UART3]			= &uart3_clk.common.hw,
		[CLK_UART2]			= &uart2_clk.common.hw,
		[CLK_UART1]			= &uart1_clk.common.hw,
		[CLK_UART0]			= &uart0_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_BUS_SPI0]			= &spi0_bus_clk.common.hw,
		[CLK_GMAC0_PHY]			= &gmac0_phy_clk.common.hw,
		[CLK_GMAC1_PHY]			= &gmac1_phy_clk.common.hw,
		[CLK_GMAC1]			= &gmac1_clk.common.hw,
		[CLK_GMAC0]			= &gmac0_clk.common.hw,
		[CLK_IRTX]			= &irtx_clk.common.hw,
		[CLK_BUS_IRTX]			= &irtx_bus_clk.common.hw,
		[CLK_GPADC]			= &gpadc_clk.common.hw,
		[CLK_THS]			= &ths_clk.common.hw,
		[CLK_I2S0]			= &i2s0_clk.common.hw,
		[CLK_I2S1]			= &i2s1_clk.common.hw,
		[CLK_I2S2]			= &i2s2_clk.common.hw,
		[CLK_BUS_I2S2]			= &i2s2_bus_clk.common.hw,
		[CLK_BUS_I2S1]			= &i2s1_bus_clk.common.hw,
		[CLK_BUS_I2S0]			= &i2s0_bus_clk.common.hw,
		[CLK_OWA_TX]			= &owa_tx_clk.common.hw,
		[CLK_OWA_RX]			= &owa_rx_clk.common.hw,
		[CLK_OWA]			= &owa_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_BUS_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_AUDIO_CODEC_DAC]		= &audio_codec_dac_clk.common.hw,
		[CLK_AUDIO_CODEC_ADC]		= &audio_codec_adc_clk.common.hw,
		[CLK_AUDIO_CODEC]		= &audio_codec_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_USB1]			= &usb1_clk.common.hw,
		[CLK_USBOTG0]			= &usbotg0_clk.common.hw,
		[CLK_USBEHCI1]			= &usbehci1_clk.common.hw,
		[CLK_USBEHCI0]			= &usbehci0_clk.common.hw,
		[CLK_USBOHCI1]			= &usbohci1_clk.common.hw,
		[CLK_USBOHCI0]			= &usbohci0_clk.common.hw,
		[CLK_LRADC]			= &lradc_clk.common.hw,
		[CLK_DPSS_TOP]			= &dpss_top_clk.common.hw,
		[CLK_HDMI_RX_DTL]		= &hdmi_rx_dtl_clk.common.hw,
		[CLK_HDMI_RX_CB]		= &hdmi_rx_cb_clk.common.hw,
		[CLK_HDMI_CEC]			= &hdmi_cec_clk.common.hw,
		[CLK_HDMI_RX]			= &hdmi_rx_clk.common.hw,
		[CLK_HRC]			= &hrc_clk.common.hw,
		[CLK_BUS_HRC]			= &hrc_bus_clk.common.hw,
		[CLK_DSI]			= &dsi_clk.common.hw,
		[CLK_BUS_DSI]			= &dsi_bus_clk.common.hw,
		[CLK_TCONLCD]			= &tconlcd_clk.common.hw,
		[CLK_BUS_TCONLCD]		= &tconlcd_bus_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_BUS_LEDC]			= &ledc_bus_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_CSI_MASTER]		= &csi_master_clk.common.hw,
		[CLK_BUS_CSI]			= &csi_bus_clk.common.hw,
		[CLK_TPADC]			= &tpadc_clk.common.hw,
		[CLK_BUS_TPADC]			= &tpadc_bus_clk.common.hw,
		[CLK_RISCV_CFG]			= &riscv_cfg_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

/* rst_def_start */
static struct ccu_reset_map sun251iw1_ccu_resets[] = {
	[RST_BUS_PLL_SSC_RSTN]		= { 0x0200, BIT(30) },
	[RST_MBUS]			= { 0x0540, BIT(30) },
	[RST_BUS_DE]			= { 0x060c, BIT(16) },
	[RST_BUS_KSC]			= { 0x061c, BIT(16) },
	[RST_BUS_DI]			= { 0x062c, BIT(16) },
	[RST_BUS_G2D]			= { 0x063c, BIT(16) },
	[RST_BUS_CE]			= { 0x068c, BIT(16) },
	[RST_BUS_VE]			= { 0x069c, BIT(16) },
	[RST_BUS_DMA]			= { 0x070c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x072c, BIT(16) },
	[RST_BUS_HSTIME]		= { 0x073c, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x078c, BIT(16) },
	[RST_BUS_PWM]			= { 0x07ac, BIT(16) },
	[RST_BUS_DRAM_MODULE]		= { 0x0800, BIT(30) },
	[RST_BUS_DRAM]			= { 0x080c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x084c, BIT(18) },
	[RST_BUS_SMHC1]			= { 0x084c, BIT(17) },
	[RST_BUS_SMHC0]			= { 0x084c, BIT(16) },
	[RST_BUS_UART5]			= { 0x090c, BIT(21) },
	[RST_BUS_UART4]			= { 0x090c, BIT(20) },
	[RST_BUS_UART3]			= { 0x090c, BIT(19) },
	[RST_BUS_UART2]			= { 0x090c, BIT(18) },
	[RST_BUS_UART1]			= { 0x090c, BIT(17) },
	[RST_BUS_UART0]			= { 0x090c, BIT(16) },
	[RST_BUS_TWI3]			= { 0x091c, BIT(19) },
	[RST_BUS_TWI2]			= { 0x091c, BIT(18) },
	[RST_BUS_TWI1]			= { 0x091c, BIT(17) },
	[RST_BUS_TWI0]			= { 0x091c, BIT(16) },
	[RST_BUS_SPI1]			= { 0x096c, BIT(17) },
	[RST_BUS_SPI0]			= { 0x096c, BIT(16) },
	[RST_BUS_GMAC1]			= { 0x097c, BIT(17) },
	[RST_BUS_GMAC0]			= { 0x097c, BIT(16) },
	[RST_BUS_IRTX]			= { 0x09cc, BIT(16) },
	[RST_BUS_GPADC]			= { 0x09ec, BIT(16) },
	[RST_BUS_TH]			= { 0x09fc, BIT(16) },
	[RST_BUS_I2S2]			= { 0x0a20, BIT(18) },
	[RST_BUS_I2S1]			= { 0x0a20, BIT(17) },
	[RST_BUS_I2S0]			= { 0x0a20, BIT(16) },
	[RST_BUS_OWA]			= { 0x0a2c, BIT(16) },
	[RST_BUS_DMIC]			= { 0x0a4c, BIT(16) },
	[RST_BUS_AUDIO_CODEC]		= { 0x0a5c, BIT(16) },
	[RST_USB_PHY0_RSTN]		= { 0x0a70, BIT(30) },
	[RST_USB_PHY1_RSTN]		= { 0x0a74, BIT(30) },
	[RST_USB_OTG0]			= { 0x0a8c, BIT(24) },
	[RST_USB_EHCI1]			= { 0x0a8c, BIT(21) },
	[RST_USB_EHCI0]			= { 0x0a8c, BIT(20) },
	[RST_USB_OHCI1]			= { 0x0a8c, BIT(17) },
	[RST_USB_OHCI0]			= { 0x0a8c, BIT(16) },
	[RST_BUS_LRADC]			= { 0x0a9c, BIT(16) },
	[RST_BUS_DPSS_TOP]		= { 0x0abc, BIT(16) },
	[RST_BUS_HDMI_RX]		= { 0x0b0c, BIT(16) },
	[RST_BUS_HRC]			= { 0x0b1c, BIT(16) },
	[RST_BUS_DSI]			= { 0x0b4c, BIT(16) },
	[RST_BUS_TCONLCD]		= { 0x0b7c, BIT(16) },
	[RST_BUS_LVDS0]			= { 0x0bac, BIT(16) },
	[RST_BUS_LEDC]			= { 0x0bfc, BIT(16) },
	[RST_BUS_CSI]			= { 0x0c1c, BIT(16) },
	[RST_BUS_TPADC]			= { 0x0c5c, BIT(16) },
	[RST_BUS_RISCV_CFG]		= { 0x0d0c, BIT(16) },
};
/* rst_def_end */

static struct ccu_common *sun251iw1_ccu_clks[] = {
	&pll_ddr_clk.common,
	&pll_peri_parent_clk.common,
	&pll_peri_2x_clk.common,
	&pll_peri_800m_clk.common,
	&pll_video0_clk.common,
	&pll_video1_clk.common,
	&pll_ve_clk.common,
	&pll_audio1_clk.common,
	&psi_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&de_clk.common,
	&de_bus_clk.common,
	&ksc_clk.common,
	&di_clk.common,
	&di_bus_clk.common,
	&g2d_clk.common,
	&g2d_bus_clk.common,
	&ce_clk.common,
	&ce_bus_clk.common,
	&ve_clk.common,
	&ve_bus_clk.common,
	&dma_clk.common,
	&spinlock_clk.common,
	&hstimer_clk.common,
	&avs_clk.common,
	&dbgsys_clk.common,
	&pwm_clk.common,
	&dram_clk.common,
	&dram_bus_clk.common,
	&smhc0_clk.common,
	&smhc1_clk.common,
	&smhc2_clk.common,
	&smhc2_bus_clk.common,
	&smhc1_bus_clk.common,
	&smhc0_bus_clk.common,
	&uart5_clk.common,
	&uart4_clk.common,
	&uart3_clk.common,
	&uart2_clk.common,
	&uart1_clk.common,
	&uart0_clk.common,
	&twi3_clk.common,
	&twi2_clk.common,
	&twi1_clk.common,
	&twi0_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&spi1_bus_clk.common,
	&spi0_bus_clk.common,
	&gmac0_phy_clk.common,
	&gmac1_phy_clk.common,
	&gmac1_clk.common,
	&gmac0_clk.common,
	&irtx_clk.common,
	&irtx_bus_clk.common,
	&gpadc_clk.common,
	&ths_clk.common,
	&i2s0_clk.common,
	&i2s1_clk.common,
	&i2s2_clk.common,
	&i2s2_bus_clk.common,
	&i2s1_bus_clk.common,
	&i2s0_bus_clk.common,
	&owa_tx_clk.common,
	&owa_rx_clk.common,
	&owa_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&audio_codec_dac_clk.common,
	&audio_codec_adc_clk.common,
	&audio_codec_clk.common,
	&usb_clk.common,
	&usb1_clk.common,
	&usbotg0_clk.common,
	&usbehci1_clk.common,
	&usbehci0_clk.common,
	&usbohci1_clk.common,
	&usbohci0_clk.common,
	&lradc_clk.common,
	&dpss_top_clk.common,
	&hdmi_rx_dtl_clk.common,
	&hdmi_rx_cb_clk.common,
	&hdmi_cec_clk.common,
	&hdmi_rx_clk.common,
	&hrc_clk.common,
	&hrc_bus_clk.common,
	&dsi_clk.common,
	&dsi_bus_clk.common,
	&tconlcd_clk.common,
	&tconlcd_bus_clk.common,
	&ledc_clk.common,
	&ledc_bus_clk.common,
	&csi_clk.common,
	&csi_master_clk.common,
	&csi_bus_clk.common,
	&tpadc_clk.common,
	&tpadc_bus_clk.common,
	&riscv_cfg_clk.common,
};

static const struct sunxi_ccu_desc sun251iw1_ccu_desc = {
	.ccu_clks	= sun251iw1_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun251iw1_ccu_clks),

	.hw_clks	= &sun251iw1_hw_clks,

	.resets		= sun251iw1_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun251iw1_ccu_resets),
};

static const u32 pll_regs[] = {
	SUN251IW1_PLL_DDR_CTRL_REG,
	SUN251IW1_PLL_PERI_CTRL_REG,
	SUN251IW1_PLL_VIDEO0_CTRL_REG,
	SUN251IW1_PLL_VIDEO1_CTRL_REG,
	SUN251IW1_PLL_VE_CTRL_REG,
	SUN251IW1_PLL_AUDIO1_CTRL_REG,
};

static int sun251iw1_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	u32 val;
	int i, ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	/* Enable the lock bits on all PLLs */
	for (i = 0; i < ARRAY_SIZE(pll_regs); i++) {
		val = readl(reg + pll_regs[i]);
		val |= BIT(29);
		writel(val, reg + pll_regs[i]);
	}

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun251iw1_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun251iw1_ccu_clks,
			ARRAY_SIZE(sun251iw1_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun251iw1_ccu_ids[] = {
	{ .compatible = "allwinner,sun251iw1-ccu" },
	{ }
};

static struct platform_driver sun251iw1_ccu_driver = {
	.probe	= sun251iw1_ccu_probe,
	.driver	= {
		.name	= "sun251iw1-ccu",
		.of_match_table	= sun251iw1_ccu_ids,
	},
};

static int __init sunxi_ccu_sun251iw1_init(void)
{
	int ret;

	ret = platform_driver_register(&sun251iw1_ccu_driver);
	if (ret)
		pr_err("register ccu sun251iw1 failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun251iw1_init);

static void __exit sunxi_ccu_sun251iw1_exit(void)
{
	return platform_driver_unregister(&sun251iw1_ccu_driver);
}
module_exit(sunxi_ccu_sun251iw1_exit);

MODULE_VERSION("0.0.2");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
