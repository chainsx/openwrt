/*
*
* This file is provided under a dual BSD/GPL license.  When using or
* redistributing this file, you may do so under either license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef _CAN_SUNXI_CAN_H_
#define _CAN_SUNXI_CAN_H_

#include <linux/can/core.h>
#include <linux/can/led.h>
#include <linux/can/rx-offload.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/freezer.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/iopoll.h>
#include <linux/can/dev.h>
#include <linux/pinctrl/consumer.h>
#include <linux/phy/phy.h>
#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/led.h>
#include <linux/reset.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/version.h>

#define CAN_TOP_GC					(0x000)/* groble control */
/* RX FIFO0 */
#define CAN_TOP_RF0_CHL0_FN			(0x004)/* channel0 RX FIFO0 frame num */
#define CAN_TOP_RF0_CHL0_DESADD		(0x008)/* channel0 RX FIFO0 destination address */
#define CAN_TOP_RF0_CHL0_RPTRADD	(0x00c)/* channel0 RX FIFO0 read pointer address */
#define CAN_TOP_RF0_CHL0_WRAPADD	(0x010)/* 配置的 DDR 大小 (RF0_WRAP_ADDRESS-RF0_DES_ADDRESS-1) 必须是当前帧大小的整数倍 */
#define CAN_TOP_RF0_CHL0_CURADD		(0x014)/* current address in common RAM that DMA is writing to */
/* RX FIFO1 */
#define CAN_TOP_RF1_CHL0_FN			(0x018)/*  */
#define CAN_TOP_RF1_CHL0_DESADD		(0x01c)/*  */
#define CAN_TOP_RF1_CHL0_RPTRADD	(0x020)/*  */
#define CAN_TOP_RF1_CHL0_WRAPADD	(0x024)/*  */
#define CAN_TOP_RF1_CHL0_CURADD		(0x028)/*  */
/* TX Event FIFO */
#define CAN_TOP_TEF_CHL0_FN			(0x02c)/*  */
#define CAN_TOP_TEF_CHL0_DESADD		(0x030)/*  */
#define CAN_TOP_TEF_CHL0_RPTRADD	(0x034)/*  */
#define CAN_TOP_TEF_CHL0_WRAPADD	(0x038)/*  */
#define CAN_TOP_TEF_CHL0_CURADD		(0x03c)/*  */
/* TX Buffer */
#define CAN_TOP_TB_FN				(0x040)/* CAN 的最大 TB 帧数。帧数必须至少为 2 */
#define CAN_TOP_INT_EN				(0x044)/* interrupt request enable */
#define CAN_TOP_INT_PEND			(0x048)/* 中断请求挂起 */
#define CAN_TOP_CHL0_RX_CFWL		(0x058)/* CANDMA channel0 RX CMDFIFO water level */
#define CAN_TOP_CHL0_DEBUG_STATUS	(0x05c)/* channel0 debug status */

#define	CAN_DMA_TX_START			(0x000)/* enable CAN DMA TX channel */
#define CAN_DMA_OP					(0x004)/* the addree of opcode */
#define CAN_DMA_TX_CMD_LEN			(0x008)/* the TX data transfer length */
#define CAN_DMA_TX_CUR_DESCADDR		(0x028)
#define CAN_DMA_TX_RESUME			(0x02c)/* means desc refresh, DMA resume form wait_mode */
#define CAN_DMA_RX_START			(0x080)/* enable CAN DMA RX channel */
#define CAN_DMA_RX_BRDY_DIS			(0x084)/* RX channel BRDY disable */

#define GC_DMA_MODE					BIT(0)
#define DMA_DONE_ENABLE				BIT(0)
#define INT_PEND_RESET				BIT(0)

#define DMA_TX_ENABLE				BIT(0)
#define DMA_TX_WAIT_EN 				BIT(1)
#define DMA_TX_BLK_LENGTH_128		BIT(0)

#define DMA_RX_ENABLE				BIT(0)
#define DMA_TX_RESUME				BIT(0)

#define TOP_IR_ALL_INT			0xffffffff
#define TOP_IR_TX_DMA_DONE		BIT(0)
#define TOP_IR_TEF_TRI			BIT(2)
#define TOP_IR_RF1_TRI_NUM		BIT(4)
#define TOP_IR_RF0_TRI_NUM		BIT(6)
#define TOP_IR_TX_CAN_DONE		BIT(9)
#define TOP_IR_RF1_TIME_OUT		BIT(10)
#define TOP_IR_RF0_TIME_OUT		BIT(11)
#define TOP_CMD_OVERRUN			BIT(19)
#define TOP_IR_TX_IRQ_WAIT		BIT(27)
#define TOP_IR_TX_ONE_DESC_DONE	BIT(26)

#define RF0_TRI_NUM				((0x20 - 1) << 16)
#define RF0_TIMEOUT_EN			BIT(8)


#define SUNXI_CAN_DMA_DESC_RX		64
#define SUNXI_CAN_DMA_DESC_TX		32

#define SUNXI_CAN_DMA_TERMINATOR	0xFFFFF800

#define WORD_SIZE	4
#define FRAME_NUM_SIZE		0x2400			/* 18 words -> 72 bytes -> 0x2400 */
#define FRAME_SIZE			0x48
#define DDR_LEN				((FRAME_NUM_SIZE) / (FRAME_SIZE))
#define DMA_IP_IRQ			0xFFFFE5FE		/* Disable tx rx interrupt in cpu mode */

/* sunxi_can lec values */
enum sunxi_can_lec_type {
	LEC_NO_ERROR = 0,
	LEC_STUFF_ERROR,
	LEC_FORM_ERROR,
	LEC_ACK_ERROR,
	LEC_BIT1_ERROR,
	LEC_BIT0_ERROR,
	LEC_CRC_ERROR,
	LEC_UNUSED,
};

enum sunxi_can_mram_cfg {
	MRAM_SIDF = 0,
	MRAM_XIDF,
	MRAM_RXF0,
	MRAM_RXF1,
	MRAM_RXB,
	MRAM_TXE,
	MRAM_TXB,
	MRAM_CFG_NUM,
};

/* address offset and element number for each FIFO/Buffer in the Message RAM */
struct mram_cfg {
	u16 off;
	u8  num;
};

struct sunxi_can_classdev;
struct sunxi_can_ops {
	/* Device specific call backs */
	int (*clear_interrupts)(struct sunxi_can_classdev *cdev);
	u32 (*read_reg)(struct sunxi_can_classdev *cdev, int reg);
	int (*write_reg)(struct sunxi_can_classdev *cdev, int reg, int val);
	int (*read_fifo)(struct sunxi_can_classdev *cdev, int addr_offset, void *val, size_t val_count);
	int (*write_fifo)(struct sunxi_can_classdev *cdev, int addr_offset,
			  const void *val, size_t val_count);
	/* CAN TOP REG */
	int (*write_top)(struct sunxi_can_classdev *cdev, int reg, int val);
	u32 (*read_top)(struct sunxi_can_classdev *cdev, int reg);

	/* CAN DMA REG */
	int (*write_dma)(struct sunxi_can_classdev *cdev, int reg, int val);
	u32 (*read_dma)(struct sunxi_can_classdev *cdev, int reg);

	int (*init)(struct sunxi_can_classdev *cdev);
};

struct sunxi_can_classdev {
	struct can_priv can;
	struct can_rx_offload offload;
	struct napi_struct napi;
	struct net_device *net;
	struct device *dev;
	struct pinctrl *pctrl;
	struct clk *hclk;
	struct clk *cclk;
	struct clk *mclk;
	struct reset_control *reset;
	//struct reset_control *mbus_reset;

	struct workqueue_struct *tx_wq;
	struct work_struct tx_work;
	struct sk_buff *tx_skb;
	struct phy *transceiver;

	const struct can_bittiming_const *bit_timing;
	const struct can_bittiming_const *data_timing;

	struct sunxi_can_ops *ops;

	//struct sunxi_can_dma *cdma;

	int version;
	int cpu_irq;
	u32 irqstatus;
	u32 top_irqstatus;

	int pm_clock_support;
	int is_peripheral;

	struct mram_cfg mcfg[MRAM_CFG_NUM];

	struct sunxi_can_dma_desc *dma_tx;
	struct sk_buff **dma_tx_skb;
	unsigned int tx_index;
	unsigned int tx_event;
	dma_addr_t dma_tx_addr;

	struct sunxi_can_dma_desc *dma_rx;
	struct sk_buff **dma_rx_skb;
	unsigned int rx_index;
	dma_addr_t dma_rx_addr;

	char *rx_buffer;

	bool rx_first_recv;
	dma_addr_t dma_mram_addr;

	u32 clk_freq;
};


struct sunxi_can_priv{
	struct sunxi_can_classdev cdev;
	//struct sunxi_can_dma *cdma;

	void __iomem *base;
	void __iomem *mram_base;
	void __iomem *top_base;
	void __iomem *dma_base;
};

typedef union {
	struct {
		unsigned int des_addr:13;	/*  */
		unsigned int reserve1:2;	/*  */
		unsigned int data_len:10;	/*  */
		unsigned int reserve2:1;	/*  */
		unsigned int frame_len:3;	/*  */
		unsigned int reserve3:2;	/*  */
		unsigned int write_back:1;	/*  */
	} desc0;

	unsigned int all;
} sunxi_can_desc0_config;

typedef struct sunxi_can_dma_desc {
	sunxi_can_desc0_config desc0;
	unsigned int desc1_next_addr;
	unsigned int desc2_src_addr;
} sunxi_can_dma_desc;

struct sunxi_can_classdev *sunxi_can_class_allocate_dev(struct device *dev, int sizeof_priv);
void sunxi_can_class_free_dev(struct net_device *net);
int sunxi_can_class_register(struct sunxi_can_classdev *cdev);
void sunxi_can_class_unregister(struct sunxi_can_classdev *cdev);
int sunxi_can_class_get_clocks(struct sunxi_can_classdev *cdev);
int sunxi_can_init_ram(struct sunxi_can_classdev *priv);

int sunxi_can_class_clk_init(struct sunxi_can_classdev *cdev);
void sunxi_can_class_clk_deinit(struct sunxi_can_classdev *cdev);
int sunxi_can_class_suspend(struct device *dev);
int sunxi_can_class_resume(struct device *dev);
int sunxi_can_dma_desc_init(struct sunxi_can_classdev *cdev);
inline struct sunxi_can_priv *cdev_to_priv(struct sunxi_can_classdev *cdev);
#endif	/* _CAN_M_H_ */
