
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2020 huangzhenwei@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN252IW1_H_
#define _DT_BINDINGS_CLK_SUN252IW1_H_

#define CLK_PLL_PERI_PARENT	0
#define CLK_PLL_PERI_2X		1
#define CLK_PLL_PERI_800M	2
#define CLK_PLL_PERI_480M	3
#define CLK_PLL_PERI_600M	4
#define CLK_PLL_PERI_400M	5
#define CLK_PLL_PERI_300M	6
#define CLK_PLL_PERI_200M	7
#define CLK_PLL_PERI_160M	8
#define CLK_PLL_PERI_150M	9
#define CLK_DDRPLL		10
#define CLK_VIDEOPLL4X		11
#define CLK_CSIPLL4X		12
#define CLK_AUDIOPLL_DIV2	13
#define CLK_CPU_GATIN		14
#define CLK_CPU			15
#define CLK_PIC			16
#define CLK_CPU_CFG		17
#define CLK_AHB			18
#define CLK_APB0		19
#define CLK_APB1		20
#define CLK_APB2		21
#define CLK_DE			22
#define CLK_BUS_DE		23
#define CLK_G2D			24
#define CLK_BUS_G2D		25
#define CLK_CE			26
#define CLK_CE_SYS		27
#define CLK_BUS_CE		28
#define CLK_VE			29
#define CLK_BUS_VE		30
#define CLK_NPU			31
#define CLK_NPU_APB_GATE	32
#define CLK_BUS_NPU		33
#define CLK_DMA1		34
#define CLK_DMA0		35
#define CLK_MSGBOX1		36
#define CLK_MSGBOX0		37
#define CLK_SPINLOCK		38
#define CLK_HSTIMER		39
#define CLK_AVS			40
#define CLK_DBGSYS		41
#define CLK_PWM			42
#define CLK_IOMMU		43
#define CLK_DRAM		44
#define CLK_MBUS_NPU_GATE	45
#define CLK_MBUS_VID_IN_GATE	46
#define CLK_MBUS_VID_OUT_GATE	47
#define CLK_MBUS_CE_GATE	48
#define CLK_MBUS_VE_GATE	49
#define CLK_MBUS_DMA_GATE	50
#define CLK_BUS_DRAM		51
#define CLK_SMHC0		52
#define CLK_SMHC1		53
#define CLK_BUS_SMHC1		54
#define CLK_BUS_SMHC0		55
#define CLK_LPSRAMCTRL_OPI_CLK2X	56
#define CLK_PSRAM_CTRL		57
#define CLK_UART3		58
#define CLK_UART2		59
#define CLK_UART1		60
#define CLK_UART0		61
#define CLK_TWI4		62
#define CLK_TWI3		63
#define CLK_TWI2		64
#define CLK_TWI1		65
#define CLK_TWI0		66
#define CLK_SPI0		67
#define CLK_SPI1		68
#define CLK_SPIF		69
#define CLK_BUS_SPIF		70
#define CLK_BUS_SPI1		71
#define CLK_BUS_SPI0		72
#define CLK_GMAC_25M		73
#define CLK_GMAC_25M_CLK_SRC	74
#define CLK_GMAC		75
#define CLK_GPADC		76
#define CLK_THS			77
#define CLK_I2S0		78
#define CLK_BUS_I2S0		79
#define CLK_AUDIO_CODEC_DAC	80
#define CLK_AUDIO_CODEC_ADC	81
#define CLK_AUDIO_CODEC		82
#define CLK_USB			83
#define CLK_USBOTG0		84
#define CLK_USBEHCI0		85
#define CLK_USBOHCI0		86
#define CLK_DPSS_TOP		87
#define CLK_TCONLCD		88
#define CLK_BUS_TCONLCD		89
#define CLK_CSI			90
#define CLK_CSI_MASTER0		91
#define CLK_CSI_MASTER1		92
#define CLK_CSI_MASTER2		93
#define CLK_BUS_CSI		94
#define CLK_E907_GATING_RS	95
#define CLK_E907		96
#define CLK_RISCV_CFG		97
#define CLK_CPUS_HCLK_GATE	98
#define CLK_MBUS_GMAC_AHB_GATE	99
#define CLK_MBUS_SMHC1_AHB_GATE	100
#define CLK_MBUS_SMHC0_AHB_GATE	101
#define CLK_MBUS_USB_AHB_GATE	102
#define CLK_GMAC_AHB_GATE	103
#define CLK_SMHC1_AHB_GATE	104
#define CLK_SMHC0_AHB_GATE	105
#define CLK_USB_AHB_GATE	106
#define CLK_VID_OUT_AHB_GATE	107
#define CLK_VID_IN_AHB_GATE	108
#define CLK_VE_AHB_GATE		109
#define CLK_RES_DCAP_24M	110
#define CLK_GPADC_24M		111
#define CLK_USB_24M		112
#define CLK_PLL_OUTPUT_GATE	113

#define CLK_MAX_NO		CLK_PLL_OUTPUT_GATE

#endif /* _DT_BINDINGS_CLK_SUN252IW1_H_ */

