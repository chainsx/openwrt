/*
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 */

#include <linux/bitfield.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/iopoll.h>
#include <linux/can/dev.h>
#include <linux/pinctrl/consumer.h>
#include <linux/phy/phy.h>
#include <uapi/linux/sched/types.h>

#include "sunxi_can.h"

/* registers definition */
enum sunxi_can_reg {
	SUNXI_CAN_CREL	= 0x0,
	SUNXI_CAN_ENDN	= 0x4,
	SUNXI_CAN_CUST	= 0x8,
	SUNXI_CAN_DBTP	= 0xc,
	SUNXI_CAN_TEST	= 0x10,
	SUNXI_CAN_RWD	= 0x14,
	SUNXI_CAN_CCCR	= 0x18,
	SUNXI_CAN_NBTP	= 0x1c,
	SUNXI_CAN_TSCC	= 0x20,
	SUNXI_CAN_TSCV	= 0x24,
	SUNXI_CAN_TOCC	= 0x28,
	SUNXI_CAN_TOCV	= 0x2c,
	SUNXI_CAN_ECR	= 0x40,
	SUNXI_CAN_PSR	= 0x44,
	/* TDCR Register only available for version >=3.1.x */
	SUNXI_CAN_TDCR	= 0x48,
	SUNXI_CAN_IR	= 0x50,
	SUNXI_CAN_IE	= 0x54,
	SUNXI_CAN_ILS	= 0x58,
	SUNXI_CAN_ILE	= 0x5c,
	SUNXI_CAN_GFC	= 0x80,
	SUNXI_CAN_SIDFC	= 0x84,
	SUNXI_CAN_XIDFC	= 0x88,
	SUNXI_CAN_XIDAM	= 0x90,
	SUNXI_CAN_HPMS	= 0x94,
	SUNXI_CAN_NDAT1	= 0x98,
	SUNXI_CAN_NDAT2	= 0x9c,
	SUNXI_CAN_RXF0C	= 0xa0,
	SUNXI_CAN_RXF0S	= 0xa4,
	SUNXI_CAN_RXF0A	= 0xa8,
	SUNXI_CAN_RXBC	= 0xac,
	SUNXI_CAN_RXF1C	= 0xb0,
	SUNXI_CAN_RXF1S	= 0xb4,
	SUNXI_CAN_RXF1A	= 0xb8,
	SUNXI_CAN_RXESC	= 0xbc,
	SUNXI_CAN_TXBC	= 0xc0,
	SUNXI_CAN_TXFQS	= 0xc4,
	SUNXI_CAN_TXESC	= 0xc8,
	SUNXI_CAN_TXBRP	= 0xcc,
	SUNXI_CAN_TXBAR	= 0xd0,
	SUNXI_CAN_TXBCR	= 0xd4,
	SUNXI_CAN_TXBTO	= 0xd8,
	SUNXI_CAN_TXBCF	= 0xdc,
	SUNXI_CAN_TXBTIE	= 0xe0,
	SUNXI_CAN_TXBCIE	= 0xe4,
	SUNXI_CAN_TXEFC	= 0xf0,
	SUNXI_CAN_TXEFS	= 0xf4,
	SUNXI_CAN_TXEFA	= 0xf8,
};

#define DEFAULT_CAN_CLK_FREQ 40000000

/* napi related */
#define SUNXI_CAN_NAPI_WEIGHT	64

/* message ram configuration data length */
#define MRAM_CFG_LEN	8

/* Core Release Register (CREL) */
#define CREL_REL_MASK		GENMASK(31, 28)
#define CREL_STEP_MASK		GENMASK(27, 24)
#define CREL_SUBSTEP_MASK	GENMASK(23, 20)

/* Data Bit Timing & Prescaler Register (DBTP) */
#define DBTP_TDC		BIT(23)
#define DBTP_DBRP_MASK		GENMASK(20, 16)
#define DBTP_DTSEG1_MASK	GENMASK(12, 8)
#define DBTP_DTSEG2_MASK	GENMASK(7, 4)
#define DBTP_DSJW_MASK		GENMASK(3, 0)

/* Transmitter Delay Compensation Register (TDCR) */
#define TDCR_TDCO_MASK		GENMASK(14, 8)
#define TDCR_TDCF_MASK		GENMASK(6, 0)

/* Test Register (TEST) */
#define TEST_LBCK		BIT(4)

/* CC Control Register (CCCR) */
#define CCCR_TXP		BIT(14)
#define CCCR_TEST		BIT(7)
#define CCCR_DAR		BIT(6)
#define CCCR_MON		BIT(5)
#define CCCR_CSR		BIT(4)
#define CCCR_CSA		BIT(3)
#define CCCR_ASM		BIT(2)
#define CCCR_CCE		BIT(1)
#define CCCR_INIT		BIT(0)
/* for version 3.0.x */
#define CCCR_CMR_MASK		GENMASK(11, 10)
#define CCCR_CMR_CANFD		0x1
#define CCCR_CMR_CANFD_BRS	0x2
#define CCCR_CMR_CAN		0x3
#define CCCR_CME_MASK		GENMASK(9, 8)
#define CCCR_CME_CAN		0
#define CCCR_CME_CANFD		0x1
#define CCCR_CME_CANFD_BRS	0x2
/* for version >=3.1.x */
#define CCCR_EFBI		BIT(13)
#define CCCR_PXHD		BIT(12)
#define CCCR_BRSE		BIT(9)
#define CCCR_FDOE		BIT(8)
/* for version >=3.2.x */
#define CCCR_NISO		BIT(15)
/* for version >=3.3.x */
#define CCCR_WMM		BIT(11)
#define CCCR_UTSU		BIT(10)

/* Nominal Bit Timing & Prescaler Register (NBTP) */
#define NBTP_NSJW_MASK		GENMASK(31, 25)
#define NBTP_NBRP_MASK		GENMASK(24, 16)
#define NBTP_NTSEG1_MASK	GENMASK(15, 8)
#define NBTP_NTSEG2_MASK	GENMASK(6, 0)

/* Timestamp Counter Configuration Register (TSCC) */
#define TSCC_TCP_MASK		GENMASK(19, 16)
#define TSCC_TSS_MASK		GENMASK(1, 0)
#define TSCC_TSS_DISABLE	0x0
#define TSCC_TSS_INTERNAL	0x1
#define TSCC_TSS_EXTERNAL	0x2

/* Timestamp Counter Value Register (TSCV) */
#define TSCV_TSC_MASK		GENMASK(15, 0)

/* Error Counter Register (ECR) */
#define ECR_RP			BIT(15)
#define ECR_REC_MASK		GENMASK(14, 8)
#define ECR_TEC_MASK		GENMASK(7, 0)

/* Protocol Status Register (PSR) */
#define PSR_BO		BIT(7)
#define PSR_EW		BIT(6)
#define PSR_EP		BIT(5)
#define PSR_LEC_MASK	GENMASK(2, 0)

/* Interrupt Register (IR) */
#define IR_ALL_INT	0xffffffff

/* Renamed bits for versions > 3.1.x */
#define IR_ARA		BIT(29)
#define IR_PED		BIT(28)
#define IR_PEA		BIT(27)

/* Bits for version 3.0.x */
#define IR_STE		BIT(31)
#define IR_FOE		BIT(30)
#define IR_ACKE		BIT(29)
#define IR_BE		BIT(28)
#define IR_CRCE		BIT(27)
#define IR_WDI		BIT(26)
#define IR_BO		BIT(25)
#define IR_EW		BIT(24)
#define IR_EP		BIT(23)
#define IR_ELO		BIT(22)
#define IR_BEU		BIT(21)
#define IR_BEC		BIT(20)
#define IR_DRX		BIT(19)
#define IR_TOO		BIT(18)
#define IR_MRAF		BIT(17)
#define IR_TSW		BIT(16)
#define IR_TEFL		BIT(15)
#define IR_TEFF		BIT(14)
#define IR_TEFW		BIT(13)
#define IR_TEFN		BIT(12)
#define IR_TFE		BIT(11)
#define IR_TCF		BIT(10)
#define IR_TC		BIT(9)
#define IR_HPM		BIT(8)
#define IR_RF1L		BIT(7)
#define IR_RF1F		BIT(6)
#define IR_RF1W		BIT(5)
#define IR_RF1N		BIT(4)
#define IR_RF0L		BIT(3)
#define IR_RF0F		BIT(2)
#define IR_RF0W		BIT(1)
#define IR_RF0N		BIT(0)
#define IR_ERR_STATE	(IR_BO | IR_EW | IR_EP)

/* Interrupts for version 3.0.x */
#define IR_ERR_LEC_30X	(IR_STE	| IR_FOE | IR_ACKE | IR_BE | IR_CRCE)
#define IR_ERR_BUS_30X	(IR_ERR_LEC_30X | IR_WDI | IR_BEU | IR_BEC | \
			 IR_TOO | IR_MRAF | IR_TSW | IR_TEFL | IR_RF1L | \
			 IR_RF0L | IR_RF0F)
#define IR_ERR_ALL_30X	(IR_ERR_STATE | IR_ERR_BUS_30X)

/* Interrupts for version >= 3.1.x */
#define IR_ERR_LEC_31X		(IR_PED | IR_PEA)
#define IR_ERR_BUS_31X		(IR_ERR_LEC_31X | IR_WDI | IR_BEU | IR_BEC | \
			 IR_TOO | IR_MRAF | IR_TSW | IR_TEFL | IR_RF1L | \
			 IR_RF0L | IR_RF0F)
#define IR_ERR_ALL_31X	(IR_ERR_STATE | IR_ERR_BUS_31X)

/* Interrupt Line Select (ILS) */
#define ILS_ALL_INT0	0x0
#define ILS_ALL_INT1	0xFFFFFFFF

/* Interrupt Line Enable (ILE) */
#define ILE_EINT1	BIT(1)
#define ILE_EINT0	BIT(0)

/* Rx FIFO 0/1 Configuration (RXF0C/RXF1C) */
#define RXFC_FWM_MASK	GENMASK(30, 24)
#define RXFC_FS_MASK	GENMASK(22, 16)

/* Rx FIFO 0/1 Status (RXF0S/RXF1S) */
#define RXFS_RFL	BIT(25)
#define RXFS_FF		BIT(24)
#define RXFS_FPI_MASK	GENMASK(21, 16)
#define RXFS_FGI_MASK	GENMASK(13, 8)
#define RXFS_FFL_MASK	GENMASK(6, 0)

/* Rx Buffer / FIFO Element Size Configuration (RXESC) */
#define RXESC_RBDS_MASK		GENMASK(10, 8)
#define RXESC_F1DS_MASK		GENMASK(6, 4)
#define RXESC_F0DS_MASK		GENMASK(2, 0)
#define RXESC_64B		0x7

/* Tx Buffer Configuration (TXBC) */
#define TXBC_TFQS_MASK		GENMASK(29, 24)
#define TXBC_NDTB_MASK		GENMASK(21, 16)

/* Tx FIFO/Queue Status (TXFQS) */
#define TXFQS_TFQF		BIT(21)
#define TXFQS_TFQPI_MASK	GENMASK(20, 16)
#define TXFQS_TFGI_MASK		GENMASK(12, 8)
#define TXFQS_TFFL_MASK		GENMASK(5, 0)

/* Tx Buffer Element Size Configuration (TXESC) */
#define TXESC_TBDS_MASK		GENMASK(2, 0)
#define TXESC_TBDS_64B		0x7

/* Tx Event FIFO Configuration (TXEFC) */
#define TXEFC_EFS_MASK		GENMASK(21, 16)

/* Tx Event FIFO Status (TXEFS) */
#define TXEFS_TEFL		BIT(25)
#define TXEFS_EFF		BIT(24)
#define TXEFS_EFGI_MASK		GENMASK(12, 8)
#define TXEFS_EFFL_MASK		GENMASK(5, 0)

/* Tx Event FIFO Acknowledge (TXEFA) */
#define TXEFA_EFAI_MASK		GENMASK(4, 0)

/* Message RAM Configuration (in bytes) */
#define SIDF_ELEMENT_SIZE	4
#define XIDF_ELEMENT_SIZE	8
#define RXF0_ELEMENT_SIZE	72
#define RXF1_ELEMENT_SIZE	72
#define RXB_ELEMENT_SIZE	72
#define TXE_ELEMENT_SIZE	8
#define TXB_ELEMENT_SIZE	72

/* Message RAM Elements */
#define SUNXI_CAN_FIFO_ID		0x0
#define SUNXI_CAN_FIFO_DLC		0x4
#define SUNXI_CAN_FIFO_DATA		0x8

/* Rx Buffer Element */
/* R0 */
#define RX_BUF_ESI		BIT(31)
#define RX_BUF_XTD		BIT(30)
#define RX_BUF_RTR		BIT(29)
/* R1 */
#define RX_BUF_ANMF		BIT(31)
#define RX_BUF_FDF		BIT(21)
#define RX_BUF_BRS		BIT(20)
#define RX_BUF_RXTS_MASK	GENMASK(15, 0)

/* Tx Buffer Element */
/* T0 */
#define TX_BUF_ESI		BIT(31)
#define TX_BUF_XTD		BIT(30)
#define TX_BUF_RTR		BIT(29)
/* T1 */
#define TX_BUF_EFC		BIT(23)
#define TX_BUF_FDF		BIT(21)
#define TX_BUF_BRS		BIT(20)
#define TX_BUF_MM_MASK		GENMASK(31, 24)
#define TX_BUF_DLC_MASK		GENMASK(19, 16)

/* Tx event FIFO Element */
/* E1 */
#define TX_EVENT_MM_MASK	GENMASK(31, 24)
#define TX_EVENT_TXTS_MASK	GENMASK(15, 0)

/* The ID and DLC registers are adjacent in SUNXI_CAN FIFO memory,
 * and we can save a (potentially slow) bus round trip by combining
 * reads and writes to them.
 */
struct id_and_dlc {
	u32 id;
	u32 dlc;
};

static inline u32 sunxi_can_read(struct sunxi_can_classdev *cdev, enum sunxi_can_reg reg)
{
	return cdev->ops->read_reg(cdev, reg);
}

static inline void sunxi_can_write(struct sunxi_can_classdev *cdev, enum sunxi_can_reg reg,
				u32 val)
{
	cdev->ops->write_reg(cdev, reg, val);
}

static int sunxi_can_fifo_write(struct sunxi_can_classdev *cdev,
		 u32 fpi, unsigned int offset, const void *val, size_t val_count)
{
	u32 addr_offset = cdev->mcfg[MRAM_TXB].off + fpi * TXB_ELEMENT_SIZE +
		offset;

	if (val_count == 0)
		return 0;

	return cdev->ops->write_fifo(cdev, addr_offset, val, val_count);
}

static inline int sunxi_can_fifo_write_no_off(struct sunxi_can_classdev *cdev,
					  u32 fpi, u32 val)
{
	return cdev->ops->write_fifo(cdev, fpi, &val, 1);
}

static inline bool sunxi_can_tx_fifo_full(struct sunxi_can_classdev *cdev)
{
	return !!(sunxi_can_read(cdev, SUNXI_CAN_TXFQS) & TXFQS_TFQF);
}

static void sunxi_can_config_endisable(struct sunxi_can_classdev *cdev, bool enable)
{
	u32 cccr = sunxi_can_read(cdev, SUNXI_CAN_CCCR);
	u32 timeout = 10;
	u32 val = 0;

	/* Clear the Clock stop request if it was set */
	if (cccr & CCCR_CSR)
		cccr &= ~CCCR_CSR;

	if (enable) {
		/* enable sunxi_can configuration */
		sunxi_can_write(cdev, SUNXI_CAN_CCCR, cccr | CCCR_INIT);
		udelay(5);
		/* CCCR.CCE can only be set/reset while CCCR.INIT = '1' */
		sunxi_can_write(cdev, SUNXI_CAN_CCCR, cccr | CCCR_INIT | CCCR_CCE);
	} else {
		sunxi_can_write(cdev, SUNXI_CAN_CCCR, cccr & ~(CCCR_INIT | CCCR_CCE));
	}

	/* there's a delay for module initialization */
	if (enable)
		val = CCCR_INIT | CCCR_CCE;

	while ((sunxi_can_read(cdev, SUNXI_CAN_CCCR) & (CCCR_INIT | CCCR_CCE)) != val) {
		if (timeout == 0) {
			netdev_warn(cdev->net, "Failed to init module\n");
			return;
		}
		timeout--;
		udelay(1);
	}
}

static inline void sunxi_can_enable_all_interrupts(struct sunxi_can_classdev *cdev)
{
	/* Only interrupt line 0 is used in this driver */
	sunxi_can_write(cdev, SUNXI_CAN_ILE, ILE_EINT0);
}

static inline void sunxi_can_disable_all_interrupts(struct sunxi_can_classdev *cdev)
{
	sunxi_can_write(cdev, SUNXI_CAN_ILE, 0x0);
}

static void sunxi_can_clean(struct net_device *net)
{
	struct sunxi_can_classdev *cdev = netdev_priv(net);

	if (cdev->tx_skb) {
		int putidx = 0;

		net->stats.tx_errors++;
		if (cdev->version > 30)
			putidx = FIELD_GET(TXFQS_TFQPI_MASK,
					   sunxi_can_read(cdev, SUNXI_CAN_TXFQS));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		can_free_echo_skb(cdev->net, putidx, NULL);
#else
		can_free_echo_skb(cdev->net, putidx);
#endif
		cdev->tx_skb = NULL;
	}
}

static int sunxi_can_handle_lost_msg(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	struct net_device_stats *stats = &cdev->net->stats;
	struct sk_buff *skb;
	struct can_frame *frame;

	netdev_err(dev, "msg lost in rxf0\n");
	netdev_dbg(dev, "rx bytes is %lu, rx packets is %lu \n", stats->rx_bytes, stats->rx_packets);

	stats->rx_errors++;
	stats->rx_over_errors++;

	skb = alloc_can_err_skb(dev, &frame);
	if (unlikely(!skb))
		return 0;

	frame->can_id |= CAN_ERR_CRTL;
	frame->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;

	netif_receive_skb(skb);

	return 1;
}

static int sunxi_can_handle_lec_err(struct net_device *dev,
				enum sunxi_can_lec_type lec_type)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;

	cdev->can.can_stats.bus_error++;
	stats->rx_errors++;

	/* propagate the error condition to the CAN stack */
	skb = alloc_can_err_skb(dev, &cf);
	if (unlikely(!skb))
		return 0;

	/* check for 'last error code' which tells us the
	 * type of the last error to occur on the CAN bus
	 */
	cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;

	switch (lec_type) {
	case LEC_STUFF_ERROR:
		netdev_dbg(dev, "stuff error\n");
		cf->data[2] |= CAN_ERR_PROT_STUFF;
		break;
	case LEC_FORM_ERROR:
		netdev_dbg(dev, "form error\n");
		cf->data[2] |= CAN_ERR_PROT_FORM;
		break;
	case LEC_ACK_ERROR:
		netdev_dbg(dev, "ack error\n");
		cf->data[3] = CAN_ERR_PROT_LOC_ACK;
		break;
	case LEC_BIT1_ERROR:
		netdev_dbg(dev, "bit1 error\n");
		cf->data[2] |= CAN_ERR_PROT_BIT1;
		break;
	case LEC_BIT0_ERROR:
		netdev_dbg(dev, "bit0 error\n");
		cf->data[2] |= CAN_ERR_PROT_BIT0;
		break;
	case LEC_CRC_ERROR:
		netdev_dbg(dev, "CRC error\n");
		cf->data[3] = CAN_ERR_PROT_LOC_CRC_SEQ;
		break;
	default:
		break;
	}

	stats->rx_packets++;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	stats->rx_bytes += cf->len;
#else
	stats->rx_bytes += cf->can_dlc;
#endif

	netif_receive_skb(skb);

	return 1;
}

static int __sunxi_can_get_berr_counter(const struct net_device *dev,
					struct can_berr_counter *bec)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	unsigned int ecr;

	ecr = sunxi_can_read(cdev, SUNXI_CAN_ECR);
	bec->rxerr = FIELD_GET(ECR_REC_MASK, ecr);
	bec->txerr = FIELD_GET(ECR_TEC_MASK, ecr);

	return 0;
}

static int sunxi_can_clk_start(struct sunxi_can_classdev *cdev)
{
	if (cdev->pm_clock_support == 0)
		return 0;

	return pm_runtime_resume_and_get(cdev->dev);
}

static void sunxi_can_clk_stop(struct sunxi_can_classdev *cdev)
{
	if (cdev->pm_clock_support)
		pm_runtime_put_sync(cdev->dev);
}

static int sunxi_can_get_berr_counter(const struct net_device *dev,
				  struct can_berr_counter *bec)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	int err;

	err = sunxi_can_clk_start(cdev);
	if (err)
		return err;

	__sunxi_can_get_berr_counter(dev, bec);

	sunxi_can_clk_stop(cdev);

	return 0;
}

static int sunxi_can_handle_state_change(struct net_device *dev,
					 enum can_state new_state)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	struct can_berr_counter bec;
	unsigned int ecr;

	switch (new_state) {
	case CAN_STATE_ERROR_WARNING:
		/* error warning state */
		cdev->can.can_stats.error_warning++;
		cdev->can.state = CAN_STATE_ERROR_WARNING;
		break;
	case CAN_STATE_ERROR_PASSIVE:
		/* error passive state */
		cdev->can.can_stats.error_passive++;
		cdev->can.state = CAN_STATE_ERROR_PASSIVE;
		break;
	case CAN_STATE_BUS_OFF:
		/* bus-off state */
		cdev->can.state = CAN_STATE_BUS_OFF;
		sunxi_can_disable_all_interrupts(cdev);
		cdev->can.can_stats.bus_off++;
		can_bus_off(dev);
		break;
	default:
		break;
	}

	/* propagate the error condition to the CAN stack */
	skb = alloc_can_err_skb(dev, &cf);
	if (unlikely(!skb))
		return 0;

	__sunxi_can_get_berr_counter(dev, &bec);

	switch (new_state) {
	case CAN_STATE_ERROR_WARNING:
		/* error warning state */
		cf->can_id |= CAN_ERR_CRTL;
		cf->data[1] = (bec.txerr > bec.rxerr) ?
			CAN_ERR_CRTL_TX_WARNING :
			CAN_ERR_CRTL_RX_WARNING;
		cf->data[6] = bec.txerr;
		cf->data[7] = bec.rxerr;
		break;
	case CAN_STATE_ERROR_PASSIVE:
		/* error passive state */
		cf->can_id |= CAN_ERR_CRTL;
		ecr = sunxi_can_read(cdev, SUNXI_CAN_ECR);
		if (ecr & ECR_RP)
			cf->data[1] |= CAN_ERR_CRTL_RX_PASSIVE;
		if (bec.txerr > 127)
			cf->data[1] |= CAN_ERR_CRTL_TX_PASSIVE;
		cf->data[6] = bec.txerr;
		cf->data[7] = bec.rxerr;
		break;
	case CAN_STATE_BUS_OFF:
		/* bus-off state */
		cf->can_id |= CAN_ERR_BUSOFF;
		break;
	default:
		break;
	}

	stats->rx_packets++;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	stats->rx_bytes += cf->len;
#else
	stats->rx_bytes += cf->can_dlc;
#endif
	netif_receive_skb(skb);

	return 1;
}

static int sunxi_can_handle_state_errors(struct net_device *dev, u32 psr)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	int work_done = 0;

	if (psr & PSR_EW && cdev->can.state != CAN_STATE_ERROR_WARNING) {
		netdev_dbg(dev, "entered error warning state\n");
		work_done += sunxi_can_handle_state_change(dev,
							   CAN_STATE_ERROR_WARNING);
	}

	if (psr & PSR_EP && cdev->can.state != CAN_STATE_ERROR_PASSIVE) {
		netdev_dbg(dev, "entered error passive state\n");
		work_done += sunxi_can_handle_state_change(dev,
							   CAN_STATE_ERROR_PASSIVE);
	}

	if (psr & PSR_BO && cdev->can.state != CAN_STATE_BUS_OFF) {
		netdev_dbg(dev, "entered error bus off state\n");
		work_done += sunxi_can_handle_state_change(dev,
							   CAN_STATE_BUS_OFF);
	}

	return work_done;
}

static void sunxi_can_handle_other_err(struct net_device *dev, u32 irqstatus)
{
	if (irqstatus & IR_WDI)
		netdev_err(dev, "Message RAM Watchdog event due to missing READY\n");
	if (irqstatus & IR_BEU)
		netdev_err(dev, "Bit Error Uncorrected\n");
	if (irqstatus & IR_BEC)
		netdev_err(dev, "Bit Error Corrected\n");
	if (irqstatus & IR_TOO)
		netdev_err(dev, "Timeout reached\n");
	if (irqstatus & IR_MRAF)
		netdev_err(dev, "Message RAM access failure occurred\n");
}

static inline bool is_lec_err(u32 psr)
{
	psr &= LEC_UNUSED;

	return psr && (psr != LEC_UNUSED);
}

static inline bool sunxi_can_is_protocol_err(u32 irqstatus)
{
	return irqstatus & IR_ERR_LEC_31X;
}

static int sunxi_can_handle_protocol_error(struct net_device *dev, u32 irqstatus)
{
	struct net_device_stats *stats = &dev->stats;
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	struct can_frame *cf;
	struct sk_buff *skb;

	/* propagate the error condition to the CAN stack */
	skb = alloc_can_err_skb(dev, &cf);

	/* update tx error stats since there is protocol error */
	stats->tx_errors++;

	/* update arbitration lost status */
	if (cdev->version >= 31 && (irqstatus & IR_PEA)) {
		netdev_dbg(dev, "Protocol error in Arbitration fail\n");
		cdev->can.can_stats.arbitration_lost++;
		if (skb) {
			cf->can_id |= CAN_ERR_LOSTARB;
			cf->data[0] |= CAN_ERR_LOSTARB_UNSPEC;
		}
	}

	if (unlikely(!skb)) {
		netdev_dbg(dev, "allocation of skb failed\n");
		return 0;
	}

	netif_receive_skb(skb);

	return 1;
}

static int sunxi_can_handle_bus_errors(struct net_device *dev, u32 irqstatus, u32 psr)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	int work_done = 0;

	if (irqstatus & IR_RF0L)
		work_done += sunxi_can_handle_lost_msg(dev);

	if (irqstatus & IR_RF0F)
		netdev_dbg(dev, "rx fifo0 full.\n");

	/* handle lec errors on the bus */
	if ((cdev->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) &&
		is_lec_err(psr))
		work_done += sunxi_can_handle_lec_err(dev, psr & LEC_UNUSED);

	/* handle protocol errors in arbitration phase */
	if ((cdev->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) &&
		sunxi_can_is_protocol_err(irqstatus))
		work_done += sunxi_can_handle_protocol_error(dev, irqstatus);

	/* other unproccessed error interrupts */
	sunxi_can_handle_other_err(dev, irqstatus);

	return work_done;
}

static int sunxi_can_rx_error_handler(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	int work_done = 0;
	u32 irqstatus, psr;

	if (!(cdev->irqstatus & IR_ERR_ALL_30X))
		goto end;

	irqstatus = cdev->irqstatus | sunxi_can_read(cdev, SUNXI_CAN_IR);
	if (!irqstatus)
		goto end;

	psr = sunxi_can_read(cdev, SUNXI_CAN_PSR);

	if (irqstatus & IR_ERR_STATE)
		work_done += sunxi_can_handle_state_errors(dev, psr);

	if (irqstatus & IR_ERR_BUS_30X)
		work_done += sunxi_can_handle_bus_errors(dev, irqstatus, psr);

end:
	return work_done;
}

static int sunxi_can_fifo_read(struct sunxi_can_classdev *cdev,
		u32 fgi, unsigned int offset, void *val, size_t val_count)
{
	u32 addr_offset = cdev->mcfg[MRAM_RXF0].off + fgi * RXF0_ELEMENT_SIZE +
		offset;

	if (val_count == 0)
		return 0;

	return cdev->ops->read_fifo(cdev, addr_offset, val, val_count);
}

static int sunxi_can_read_fifo(struct net_device *dev, u32 rxfs)
{
	struct net_device_stats *stats = &dev->stats;
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	struct canfd_frame *cf;
	struct sk_buff *skb;
	struct id_and_dlc fifo_header;
	u32 fgi;
	//u32 timestamp = 0;
	int err;

	//pr_info("rx read fifo.");
	/* calculate the fifo get index for where to read data */
	fgi = FIELD_GET(RXFS_FGI_MASK, rxfs);
	err = sunxi_can_fifo_read(cdev, fgi, SUNXI_CAN_FIFO_ID, &fifo_header, 2);
	if (err)
		goto out_fail;

	if (fifo_header.dlc & RX_BUF_FDF)
		skb = alloc_canfd_skb(dev, &cf);
	else
		skb = alloc_can_skb(dev, (struct can_frame **)&cf);
	if (!skb) {
		stats->rx_dropped++;
		return 0;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	if (fifo_header.dlc & RX_BUF_FDF)
		cf->len = can_fd_dlc2len((fifo_header.dlc >> 16) & 0x0F);
	else
		cf->len = can_cc_dlc2len((fifo_header.dlc >> 16) & 0x0F);
#else
	if (fifo_header.dlc & RX_BUF_FDF)
		cf->len = can_dlc2len((fifo_header.dlc >> 16) & 0x0F);
	else
		cf->len = get_can_dlc((fifo_header.dlc >> 16) & 0x0F);
#endif

	if (fifo_header.id & RX_BUF_XTD)
		cf->can_id = (fifo_header.id & CAN_EFF_MASK) | CAN_EFF_FLAG;
	else
		cf->can_id = (fifo_header.id >> 18) & CAN_SFF_MASK;

	if (fifo_header.id & RX_BUF_ESI) {
		cf->flags |= CANFD_ESI;
		netdev_dbg(dev, "ESI Error\n");
	}

	if (!(fifo_header.dlc & RX_BUF_FDF) && (fifo_header.id & RX_BUF_RTR)) {
		cf->can_id |= CAN_RTR_FLAG;
	} else {
		if (fifo_header.dlc & RX_BUF_BRS)
			cf->flags |= CANFD_BRS;

		err = sunxi_can_fifo_read(cdev, fgi, SUNXI_CAN_FIFO_DATA,
					  cf->data, DIV_ROUND_UP(cf->len, 4));
		if (err)
			goto out_free_skb;
	}

	/* acknowledge rx fifo 0 */
	sunxi_can_write(cdev, SUNXI_CAN_RXF0A, fgi);

	stats->rx_packets++;
	stats->rx_bytes += cf->len;

	//timestamp = FIELD_GET(RX_BUF_RXTS_MASK, fifo_header.dlc);

	netif_receive_skb(skb);

	return 0;

out_free_skb:
	kfree_skb(skb);
out_fail:
	netdev_err(dev, "FIFO read returned %d\n", err);
	return err;
}

static int sunxi_can_do_rx_poll(struct net_device *dev, int quota)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	u32 pkts = 0;
	u32 rxfs;
	int err;

	rxfs = sunxi_can_read(cdev, SUNXI_CAN_RXF0S);
	if (!(rxfs & RXFS_FFL_MASK)) {
		netdev_dbg(dev, "no messages in fifo0\n");
		return 0;
	}

	while ((rxfs & RXFS_FFL_MASK) && (quota > 0)) {
		err = sunxi_can_read_fifo(dev, rxfs);
		if (err)
			return err;

		quota--;
		pkts++;
		rxfs = sunxi_can_read(cdev, SUNXI_CAN_RXF0S);
	}

	if (pkts)
		can_led_event(dev, CAN_LED_EVENT_RX);

	return pkts;
}

static int sunxi_can_rx_handler(struct net_device *dev, int quota)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	int rx_work_or_err;
	int work_done = 0;
	u32 irqstatus;

	irqstatus = cdev->irqstatus | sunxi_can_read(cdev, SUNXI_CAN_IR);
	work_done = sunxi_can_rx_error_handler(dev);

	if (irqstatus & IR_RF0N) {
		rx_work_or_err = sunxi_can_do_rx_poll(dev, (quota - work_done));
		if (rx_work_or_err < 0)
			return rx_work_or_err;

		work_done += rx_work_or_err;
	}
	return work_done;
}

static int sunxi_can_poll(struct napi_struct *napi, int quota)
{
	struct net_device *dev = napi->dev;
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	int work_done = 0;

	work_done = sunxi_can_rx_handler(dev, quota);

	/* Don't re-enable interrupts if the driver had a fatal error
	 * (e.g., FIFO read failure).
	 */
	if (work_done >= 0 && work_done < quota) {
		napi_complete_done(napi, work_done);
		sunxi_can_enable_all_interrupts(cdev);
	}

	return work_done;
}

/* Echo tx skb and update net stats. Peripherals use rx-offload for
 * echo. timestamp is used for peripherals to ensure correct ordering
 * by rx-offload, and is ignored for non-peripherals.
 */
static void sunxi_can_tx_update_stats(struct sunxi_can_classdev *cdev,
				  unsigned int msg_mark,
				  u32 timestamp)
{
	struct net_device *dev = cdev->net;
	struct net_device_stats *stats = &dev->stats;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	stats->tx_bytes += can_get_echo_skb(dev, msg_mark, NULL);
#else
	stats->tx_bytes += can_get_echo_skb(dev, msg_mark);
#endif
	stats->tx_packets++;
}

static int sunxi_can_txe_fifo_read(struct sunxi_can_classdev *cdev, u32 fgi, u32 offset, u32 *val)
{
	u32 addr_offset = cdev->mcfg[MRAM_TXE].off + fgi * TXE_ELEMENT_SIZE +
		offset;

	return cdev->ops->read_fifo(cdev, addr_offset, val, 1);
}

static int sunxi_can_echo_tx_event(struct net_device *dev)
{
	u32 txe_count = 0;
	u32 sunxi_can_txefs;
	u32 fgi = 0;
	int i = 0;
	unsigned int msg_mark;

	struct sunxi_can_classdev *cdev = netdev_priv(dev);

	/* read tx event fifo status */
	sunxi_can_txefs = sunxi_can_read(cdev, SUNXI_CAN_TXEFS);

	/* Get Tx Event fifo element count */
	txe_count = FIELD_GET(TXEFS_EFFL_MASK, sunxi_can_txefs);

	/* Get and process all sent elements */
	for (i = 0; i < txe_count; i++) {
		u32 txe, timestamp = 0;
		int err;

		/* retrieve get index */
		fgi = FIELD_GET(TXEFS_EFGI_MASK, sunxi_can_read(cdev, SUNXI_CAN_TXEFS));

		/* get message marker, timestamp */
		err = sunxi_can_txe_fifo_read(cdev, fgi, 4, &txe);
		if (err) {
			netdev_err(dev, "TXE FIFO read returned %d\n", err);
			return err;
		}

		msg_mark = FIELD_GET(TX_EVENT_MM_MASK, txe);/* 由 CPU 在 Tx 缓冲区配置期间写入。复制到 Tx 事件 FIFO 元素以识别 Tx 消息状态 */
		timestamp = FIELD_GET(TX_EVENT_TXTS_MASK, txe);/* 帧传输开始时捕获的时间戳计数器。分辨率取决于时间戳计数器预分频器的配置 */

		/* ack txe element */
		sunxi_can_write(cdev, SUNXI_CAN_TXEFA, FIELD_PREP(TXEFA_EFAI_MASK, fgi));
		//pr_info("tx event count is %d, %s, %d \n", i, __func__, __LINE__);
		/* update stats */
		sunxi_can_tx_update_stats(cdev, msg_mark, timestamp);
	}

	return 0;
}

static irqreturn_t sunxi_can_isr(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	u32 ir;

	//pr_info("irq start %s, %d \n", __func__, __LINE__);
	if (pm_runtime_suspended(cdev->dev))
		return IRQ_NONE;

	ir = sunxi_can_read(cdev, SUNXI_CAN_IR);
	if (!ir)
		return IRQ_NONE;

	/* ACK all irqs */
	if (ir & IR_ALL_INT)
		sunxi_can_write(cdev, SUNXI_CAN_IR, ir);

	if (cdev->ops->clear_interrupts)
		cdev->ops->clear_interrupts(cdev);

	//pr_info("irq_reg is %x %s, %d \n",ir,  __func__, __LINE__);
	/* schedule NAPI in case of 在以下情况调度NAPI
	 * - rx IRQ  接收中断请求
	 * - state change IRQ  状态改变IRQ
	 * - bus error IRQ and bus error reporting  总线错误IRQ和总线错误报告
	 */

	if ((ir & IR_RF0N) || (ir & IR_ERR_ALL_30X)) {
		cdev->irqstatus = ir;
		sunxi_can_disable_all_interrupts(cdev);
		napi_schedule(&cdev->napi);
	}

	if (ir & IR_TEFN) {
		//pr_info("tx irq trigger %s, %d \n", __func__, __LINE__);
		/* New TX FIFO Element arrived */
		if (sunxi_can_echo_tx_event(dev) != 0)
			goto out_fail;

		can_led_event(dev, CAN_LED_EVENT_TX);
		if (netif_queue_stopped(dev) &&
			!sunxi_can_tx_fifo_full(cdev))
			netif_wake_queue(dev);
	}

	return IRQ_HANDLED;

out_fail:
	sunxi_can_disable_all_interrupts(cdev);
	return IRQ_HANDLED;
}

static const struct can_bittiming_const sunxi_can_bittiming_const_30X = {
	.name = KBUILD_MODNAME,
	.tseg1_min = 2,		/* Time segment 1 = prop_seg + phase_seg1 */
	.tseg1_max = 64,
	.tseg2_min = 1,		/* Time segment 2 = phase_seg2 */
	.tseg2_max = 16,
	.sjw_max = 16,
	.brp_min = 1,
	.brp_max = 1024,
	.brp_inc = 1,
};

static const struct can_bittiming_const sunxi_can_data_bittiming_const_30X = {
	.name = KBUILD_MODNAME,
	.tseg1_min = 2,		/* Time segment 1 = prop_seg + phase_seg1 */
	.tseg1_max = 16,
	.tseg2_min = 1,		/* Time segment 2 = phase_seg2 */
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 32,
	.brp_inc = 1,
};

static const struct can_bittiming_const sunxi_can_bittiming_const_31X = {
	.name = KBUILD_MODNAME,
	.tseg1_min = 2,		/* Time segment 1 = prop_seg + phase_seg1 */
	.tseg1_max = 256,
	.tseg2_min = 2,		/* Time segment 2 = phase_seg2 */
	.tseg2_max = 128,
	.sjw_max = 128,
	.brp_min = 1,
	.brp_max = 512,
	.brp_inc = 1,
};

static const struct can_bittiming_const sunxi_can_data_bittiming_const_31X = {
	.name = KBUILD_MODNAME,
	.tseg1_min = 1,		/* Time segment 1 = prop_seg + phase_seg1 */
	.tseg1_max = 32,
	.tseg2_min = 1,		/* Time segment 2 = phase_seg2 */
	.tseg2_max = 16,
	.sjw_max = 16,
	.brp_min = 1,
	.brp_max = 32,
	.brp_inc = 1,
};

static int sunxi_can_set_bittiming(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	const struct can_bittiming *bt = &cdev->can.bittiming;
	const struct can_bittiming *dbt = &cdev->can.data_bittiming;
	u16 brp, sjw, tseg1, tseg2;
	u32 reg_btp;

	brp = bt->brp - 1;
	sjw = bt->sjw - 1;
	tseg1 = bt->prop_seg + bt->phase_seg1 - 1;
	tseg2 = bt->phase_seg2 - 1;
	reg_btp = FIELD_PREP(NBTP_NBRP_MASK, brp) |
		  FIELD_PREP(NBTP_NSJW_MASK, sjw) |
		  FIELD_PREP(NBTP_NTSEG1_MASK, tseg1) |
		  FIELD_PREP(NBTP_NTSEG2_MASK, tseg2);
	sunxi_can_write(cdev, SUNXI_CAN_NBTP, reg_btp);

	if (cdev->can.ctrlmode & CAN_CTRLMODE_FD) {
		reg_btp = 0;
		brp = dbt->brp - 1;
		sjw = dbt->sjw - 1;
		tseg1 = dbt->prop_seg + dbt->phase_seg1 - 1;
		tseg2 = dbt->phase_seg2 - 1;

		/* TDC is only needed for bitrates beyond 2.5 MBit/s.
		 * 只有超过 2.5 MBit/s 的比特率才需要 TDC。
		 * This is mentioned in the "Bit Time Requirements for CAN FD"
		 * paper presented at the International CAN Conference 2013
		 * 在 2013 年国际 CAN 会议上发表的论文“CAN FD 的位时间要求”中提到了这一点
		 */
		if (dbt->bitrate > 2500000) {
			u32 tdco, ssp;

			/* Use the same value of secondary sampling point
			 * as the data sampling point
			 * 使用与数据采样点相同的二次采样点值
			 */
			ssp = dbt->sample_point;

			/* Equation based on Allwinner's CAN User Manual's
			 * Transmitter Delay Compensation Section
			 */
			tdco = (cdev->can.clock.freq / 1000) *
				ssp / dbt->bitrate;

			/* Max valid TDCO value is 127 */
			if (tdco > 127) {
				netdev_warn(dev, "TDCO value of %u is beyond maximum. Using maximum possible value\n",
						tdco);
				tdco = 127;
			}

			reg_btp |= DBTP_TDC;
			sunxi_can_write(cdev, SUNXI_CAN_TDCR,
					FIELD_PREP(TDCR_TDCO_MASK, tdco));
		}

		reg_btp |= FIELD_PREP(DBTP_DBRP_MASK, brp) |
			FIELD_PREP(DBTP_DSJW_MASK, sjw) |
			FIELD_PREP(DBTP_DTSEG1_MASK, tseg1) |
			FIELD_PREP(DBTP_DTSEG2_MASK, tseg2);

		sunxi_can_write(cdev, SUNXI_CAN_DBTP, reg_btp);
	}

	return 0;
}

/* Configure can chip:
 * - set rx buffer/fifo element size——设置 rx 缓冲区/fifo 元素大小
 * - configure rx fifo——配置接收FIFO
 * - accept non-matching frame into fifo 0—— 将不匹配的帧接收到fifo 0
 * - configure tx buffer——配置tx缓冲区
 *		- >= v3.1.x: TX FIFO is used—— >= v3.1.x: 使用 TX FIFO
 * - configure mode—— 配置模式
 * - setup bittiming —— 设置位计时
 * - configure timestamp generation —— 配置时间戳生成
 */
static void sunxi_can_chip_config(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	u32 cccr, test;

	sunxi_can_config_endisable(cdev, true);/* 配置寄存器之前必须要把这两个位置1 */

	/* RX Buffer/FIFO Element Size 64 bytes data field */
	sunxi_can_write(cdev, SUNXI_CAN_RXESC,
			FIELD_PREP(RXESC_RBDS_MASK, RXESC_64B) |
			FIELD_PREP(RXESC_F1DS_MASK, RXESC_64B) |
			FIELD_PREP(RXESC_F0DS_MASK, RXESC_64B));

	/* Accept Non-matching Frames Into FIFO 0 */
	sunxi_can_write(cdev, SUNXI_CAN_GFC, 0x0);

	/* TX FIFO is used for newer IP Core versions */
	sunxi_can_write(cdev, SUNXI_CAN_TXBC,
				FIELD_PREP(TXBC_TFQS_MASK,
				cdev->mcfg[MRAM_TXB].num) |
				cdev->mcfg[MRAM_TXB].off);

	/* support 64 bytes payload */
	sunxi_can_write(cdev, SUNXI_CAN_TXESC,
			FIELD_PREP(TXESC_TBDS_MASK, TXESC_TBDS_64B));


	/* Full TX Event FIFO is used */
	sunxi_can_write(cdev, SUNXI_CAN_TXEFC,
				FIELD_PREP(TXEFC_EFS_MASK,
				cdev->mcfg[MRAM_TXE].num) |
				cdev->mcfg[MRAM_TXE].off);


	/* rx fifo configuration, blocking mode, fifo size 1 */
	sunxi_can_write(cdev, SUNXI_CAN_RXF0C,
			FIELD_PREP(RXFC_FS_MASK, cdev->mcfg[MRAM_RXF0].num) |
			cdev->mcfg[MRAM_RXF0].off);

	sunxi_can_write(cdev, SUNXI_CAN_RXF1C,
			FIELD_PREP(RXFC_FS_MASK, cdev->mcfg[MRAM_RXF1].num) |
			cdev->mcfg[MRAM_RXF1].off);

	cccr = sunxi_can_read(cdev, SUNXI_CAN_CCCR);
	test = sunxi_can_read(cdev, SUNXI_CAN_TEST);
	test &= ~TEST_LBCK;

	/* Version 3.1.x or 3.2.x */
	cccr &= ~(CCCR_TEST | CCCR_MON | CCCR_BRSE | CCCR_FDOE |
			  CCCR_NISO | CCCR_DAR);

	/* Only 3.2.x has NISO Bit implemented */
	if (cdev->can.ctrlmode & CAN_CTRLMODE_FD_NON_ISO)
		cccr |= CCCR_NISO;

	if (cdev->can.ctrlmode & CAN_CTRLMODE_FD)
		cccr |= (CCCR_BRSE | CCCR_FDOE);

	/* Loopback Mode */
	if (cdev->can.ctrlmode & CAN_CTRLMODE_LOOPBACK) {
		cccr |= CCCR_TEST | CCCR_MON;
		test |= TEST_LBCK;
	}

	/* Enable Monitoring (all versions) */
	if (cdev->can.ctrlmode & CAN_CTRLMODE_LISTENONLY)
		cccr |= CCCR_MON;

	/* Disable Auto Retransmission (all versions) */
	if (cdev->can.ctrlmode & CAN_CTRLMODE_ONE_SHOT)
		cccr |= CCCR_DAR;

	/* Write config */
	sunxi_can_write(cdev, SUNXI_CAN_CCCR, cccr);
	sunxi_can_write(cdev, SUNXI_CAN_TEST, test);

	/* Enable interrupts */
	sunxi_can_write(cdev, SUNXI_CAN_IR, IR_ALL_INT);
	if (!(cdev->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING))
			sunxi_can_write(cdev, SUNXI_CAN_IE, IR_ALL_INT & ~(IR_ERR_LEC_31X));
	else
		sunxi_can_write(cdev, SUNXI_CAN_IE, IR_ALL_INT);

	/* route all interrupts to INT0 */
	sunxi_can_write(cdev, SUNXI_CAN_ILS, ILS_ALL_INT0);

	/* set bittiming params */
	sunxi_can_set_bittiming(dev);

	/* enable internal timestamp generation, with a prescalar of 16. The
	 * prescalar is applied to the nominal bit timing
	 */
	sunxi_can_write(cdev, SUNXI_CAN_TSCC, FIELD_PREP(TSCC_TCP_MASK, 0xf));

	sunxi_can_config_endisable(cdev, false);

	if (cdev->ops->init)
		cdev->ops->init(cdev);
}

static void sunxi_can_start(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);

	/* basic sunxi_can configuration */
	sunxi_can_chip_config(dev);

	cdev->can.state = CAN_STATE_ERROR_ACTIVE;

	sunxi_can_enable_all_interrupts(cdev);
}

static int sunxi_can_set_mode(struct net_device *dev, enum can_mode mode)
{
	switch (mode) {
	case CAN_MODE_START:
		sunxi_can_clean(dev);
		sunxi_can_start(dev);
		netif_wake_queue(dev);
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

/* Checks core release number of can —— 检查 can 的核心版本号
 * returns 0 if an unsupported device is detected —— 如果检测到不支持的设备则返回 0
 * else it returns the release and step coded as: —— 否则它返回版本和步骤编码为
 * return value = 10 * <release> + 1 * <step> —— 返回值 = 10 * <release> + 1 * <step>
 */
static int sunxi_can_check_core_release(struct sunxi_can_classdev *cdev)
{
	u32 crel_reg;
	u8 rel;
	u8 step;
	int res;

	/* Read Core Release Version and split into version number
	 * Example: Version 3.2.1 => rel = 3; step = 2; substep = 1;
	 */
	crel_reg = sunxi_can_read(cdev, SUNXI_CAN_CREL);
	rel = (u8)FIELD_GET(CREL_REL_MASK, crel_reg);
	step = (u8)FIELD_GET(CREL_STEP_MASK, crel_reg);

	if (rel == 3) {
		/* can v3.x.y: create return value */
		res = 30 + step;
	} else {
		/* Unsupported can version */
		res = 0;
	}

	return res;
}

/* Selectable Non ISO support only in version 3.2.x
 * This function checks if the bit is writable.
 * 可选非 ISO 支持仅在版本 3.2.x 中。此函数检查该位是否可写。
 */
static bool sunxi_can_niso_supported(struct sunxi_can_classdev *cdev)
{
	u32 cccr_reg, cccr_poll = 0;
	int niso_timeout = -ETIMEDOUT;
	int i;

	sunxi_can_config_endisable(cdev, true);
	cccr_reg = sunxi_can_read(cdev, SUNXI_CAN_CCCR);
	cccr_reg |= CCCR_NISO;/* 如果此位设置，则 can 使用 Bosch CAN FD 规范 V1.0 指定的 CAN FD 帧格式 */
	sunxi_can_write(cdev, SUNXI_CAN_CCCR, cccr_reg);

	for (i = 0; i <= 10; i++) {
		cccr_poll = sunxi_can_read(cdev, SUNXI_CAN_CCCR);
		if (cccr_poll == cccr_reg) {
			niso_timeout = 0;
			break;
		}

		usleep_range(1, 5);
	}

	/* Clear NISO */
	cccr_reg &= ~(CCCR_NISO);
	sunxi_can_write(cdev, SUNXI_CAN_CCCR, cccr_reg);

	sunxi_can_config_endisable(cdev, false);

	/* return false if time out (-ETIMEDOUT), else return true */
	return !niso_timeout;
}

static int sunxi_can_dev_setup(struct sunxi_can_classdev *cdev)
{
	struct net_device *dev = cdev->net;
	int sunxi_can_version;

	sunxi_can_version = sunxi_can_check_core_release(cdev);
	/* return if unsupported version */
	if (!sunxi_can_version) {
		dev_err(cdev->dev, "Unsupported version number: %2d",
			sunxi_can_version);
		return -EINVAL;
	}

	netif_napi_add(dev, &cdev->napi, sunxi_can_poll, SUNXI_CAN_NAPI_WEIGHT);/*  */

	/* Shared properties of all can versions */
	cdev->version = sunxi_can_version;
	cdev->can.do_set_mode = sunxi_can_set_mode;
	cdev->can.do_get_berr_counter = sunxi_can_get_berr_counter;

	/* Set can supported operations */
	cdev->can.ctrlmode_supported = CAN_CTRLMODE_LOOPBACK |
		CAN_CTRLMODE_LISTENONLY |
		CAN_CTRLMODE_BERR_REPORTING |
		CAN_CTRLMODE_FD |
		CAN_CTRLMODE_3_SAMPLES |
		CAN_CTRLMODE_ONE_SHOT;

	/* Set properties depending on can version */
	switch (cdev->version) {
	case 33:
		/* Support both MCAN version v3.2.x and v3.3.0 */
		cdev->can.bittiming_const = cdev->bit_timing ?
			cdev->bit_timing : &sunxi_can_bittiming_const_31X;

		cdev->can.data_bittiming_const = cdev->data_timing ?
			cdev->data_timing :
			&sunxi_can_data_bittiming_const_31X;

		cdev->can.ctrlmode_supported |=
			(sunxi_can_niso_supported(cdev) ?
			 CAN_CTRLMODE_FD_NON_ISO : 0);
		break;
	default:
		dev_err(cdev->dev, "Unsupported version number: %2d", cdev->version);
		return -EINVAL;
	}

	if (cdev->ops->init)
		cdev->ops->init(cdev);

	return 0;
}

static void sunxi_can_stop(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);

	/* disable all interrupts */
	sunxi_can_disable_all_interrupts(cdev);

	/* Set init mode to disengage from the network */
	sunxi_can_config_endisable(cdev, true);

	/* set the state as STOPPED */
	cdev->can.state = CAN_STATE_STOPPED;
}

static int sunxi_can_close(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);

	pr_info("here is close.\n");
	netif_stop_queue(dev);
	napi_disable(&cdev->napi);

	sunxi_can_stop(dev);
	sunxi_can_clk_stop(cdev);
	free_irq(dev->irq, dev);

	close_candev(dev);
	can_led_event(dev, CAN_LED_EVENT_STOP);

	phy_power_off(cdev->transceiver);
	sunxi_can_class_clk_deinit(cdev);

	return 0;
}

static int sunxi_can_next_echo_skb_occupied(struct net_device *dev, int putidx)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	/*get wrap around for loopback skb index */
	unsigned int wrap = cdev->can.echo_skb_max;
	int next_idx;

	/* calculate next index */
	next_idx = (++putidx >= wrap ? 0 : putidx);

	/* check if occupied */
	return !!cdev->can.echo_skb[next_idx];
}

static int check_echo_skb_occupied(struct net_device *dev, int idx)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	unsigned int wrap = cdev->can.echo_skb_max;
	if (idx > wrap)
		netdev_err(dev, "invalid putidx: %d\n", idx);

	return !!cdev->can.echo_skb[idx];
}

static netdev_tx_t sunxi_can_tx_handler(struct sunxi_can_classdev *cdev)
{
	struct canfd_frame *cf = (struct canfd_frame *)cdev->tx_skb->data;
	struct net_device *dev = cdev->net;
	struct sk_buff *skb = cdev->tx_skb;
	struct id_and_dlc fifo_header;
	u32 fdflags;
	int err;
	int putidx;

	cdev->tx_skb = NULL;

	/* Generate ID field for TX buffer Element */
	/* Common to all supported can versions */
	if (cf->can_id & CAN_EFF_FLAG) {
		fifo_header.id = cf->can_id & CAN_EFF_MASK;
		fifo_header.id |= TX_BUF_XTD;
	} else {
		fifo_header.id = ((cf->can_id & CAN_SFF_MASK) << 18);
	}

	if (cf->can_id & CAN_RTR_FLAG)
		fifo_header.id |= TX_BUF_RTR;

	/* Check if FIFO full */
	if (sunxi_can_tx_fifo_full(cdev)) {
		/* This shouldn't happen */
		netif_stop_queue(dev);
		netdev_warn(dev, "TX queue active although FIFO is full.");
		return NETDEV_TX_BUSY;
	}

	/* get put index for frame */
	putidx = FIELD_GET(TXFQS_TFQPI_MASK, sunxi_can_read(cdev, SUNXI_CAN_TXFQS));

	if (check_echo_skb_occupied(dev, putidx)) {
		netif_stop_queue(dev);
		return NETDEV_TX_BUSY;
	}
	/* Construct DLC Field, with CAN-FD configuration.构造 DLC Field，带 CAN-FD 配置
	 * Use the put index of the fifo as the message marker,
	 * used in the TX interrupt for sending the correct echo frame.
	 * 使用 fifo 的 put 索引作为消息标记，在 TX 中断中用于发送正确的回显帧
	 */

	/* get CAN FD configuration of frame */
	fdflags = 0;
	if (can_is_canfd_skb(skb)) {
		fdflags |= TX_BUF_FDF;
		if (cf->flags & CANFD_BRS)
			fdflags |= TX_BUF_BRS;
	}

		fifo_header.dlc = FIELD_PREP(TX_BUF_MM_MASK, putidx) |
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
			FIELD_PREP(TX_BUF_DLC_MASK, can_fd_len2dlc(cf->len)) |
#else
			FIELD_PREP(TX_BUF_DLC_MASK, can_len2dlc(cf->len)) |
#endif
			fdflags | TX_BUF_EFC;

		err = sunxi_can_fifo_write(cdev, putidx, SUNXI_CAN_FIFO_ID, &fifo_header, 2);
		if (err)
			goto out_fail;

		err = sunxi_can_fifo_write(cdev, putidx, SUNXI_CAN_FIFO_DATA,
					   cf->data, DIV_ROUND_UP(cf->len, 4));
		if (err)
			goto out_fail;

	/* Push loopback echo.
	 * Will be looped back on TX interrupt based on message marker
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	can_put_echo_skb(skb, dev, putidx, 0);
#else
	can_put_echo_skb(skb, dev, putidx);
#endif

	/* Enable TX FIFO element to start transfer  */
	sunxi_can_write(cdev, SUNXI_CAN_TXBAR, (1 << putidx));

	/* stop network queue if fifo full */
	if (sunxi_can_tx_fifo_full(cdev) ||
		sunxi_can_next_echo_skb_occupied(dev, putidx))
		netif_stop_queue(dev);

	//pr_info("xmit complete %s, %d \n", __func__, __LINE__);

	return NETDEV_TX_OK;

out_fail:
	netdev_err(dev, "FIFO write returned %d\n", err);
	sunxi_can_disable_all_interrupts(cdev);
	return NETDEV_TX_BUSY;
}

static netdev_tx_t sunxi_can_start_xmit(struct sk_buff *skb,
					struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);

	if (can_dropped_invalid_skb(dev, skb))
		return NETDEV_TX_OK;

	cdev->tx_skb = skb;
	return sunxi_can_tx_handler(cdev);
}

static int sunxi_can_open(struct net_device *dev)
{
	struct sunxi_can_classdev *cdev = netdev_priv(dev);
	int err;
	u32 tmp_reg;
#ifdef CONFIG_PREEMPT_RT
	struct irq_desc *desc;
	struct sched_param sp = { .sched_priority = 97 };
#endif

	sunxi_can_class_clk_init(cdev);
	err = phy_power_on(cdev->transceiver);
	if (err)
		return err;

	err = sunxi_can_clk_start(cdev);
	if (err)
		goto out_phy_power_off;

	/* open the can device */
	err = open_candev(dev);
	if (err) {
		netdev_err(dev, "failed to open can device\n");
		goto exit_disable_clks;
	}

	/* register interrupt handler */
	err = request_irq(dev->irq, sunxi_can_isr, IRQF_SHARED, dev->name, dev);

	if (err < 0) {
		netdev_err(dev, "failed to request interrupt\n");
		goto out_wq_fail;
	}

#ifdef CONFIG_PREEMPT_RT
	desc = irq_to_desc(dev->irq);
	if (IS_ERR_OR_NULL(desc->action->thread)) {
		netdev_err(dev, "cannot get thread of irq\n");
	} else {
		WARN_ON_ONCE(sched_setscheduler_nocheck(desc->action->thread, SCHED_FIFO, &sp) != 0);
		netdev_info(dev, "set irq thread sched priority to %d\n", sp.sched_priority);
	}
#endif

	/* start the sunxi_can controller */
	sunxi_can_start(dev);

	can_led_event(dev, CAN_LED_EVENT_OPEN);

	napi_enable(&cdev->napi);

	tmp_reg = cdev->ops->read_top(cdev, CAN_TOP_GC);
	tmp_reg &= (~GC_DMA_MODE);
	cdev->ops->write_top(cdev, CAN_TOP_GC, tmp_reg);

	netif_start_queue(dev);
	netif_wake_queue(dev);

	return 0;

out_wq_fail:
	close_candev(dev);
exit_disable_clks:
	sunxi_can_clk_stop(cdev);
out_phy_power_off:
	phy_power_off(cdev->transceiver);
	return err;
}

static const struct net_device_ops sunxi_can_netdev_ops = {
	.ndo_open = sunxi_can_open,
	.ndo_stop = sunxi_can_close,
	.ndo_start_xmit = sunxi_can_start_xmit,
	.ndo_change_mtu = can_change_mtu,
};

static int register_sunxi_can_dev(struct net_device *dev)
{
	dev->flags |= IFF_ECHO;	/* we support local echo */
	dev->netdev_ops = &sunxi_can_netdev_ops;

	return register_candev(dev);
}

static void sunxi_can_of_parse_mram(struct sunxi_can_classdev *cdev,
				const u32 *mram_config_vals)
{
	cdev->mcfg[MRAM_SIDF].off = mram_config_vals[0];
	cdev->mcfg[MRAM_SIDF].num = mram_config_vals[1];
	cdev->mcfg[MRAM_XIDF].off = cdev->mcfg[MRAM_SIDF].off +
		cdev->mcfg[MRAM_SIDF].num * SIDF_ELEMENT_SIZE;
	cdev->mcfg[MRAM_XIDF].num = mram_config_vals[2];
	cdev->mcfg[MRAM_RXF0].off = cdev->mcfg[MRAM_XIDF].off +
		cdev->mcfg[MRAM_XIDF].num * XIDF_ELEMENT_SIZE;
	cdev->mcfg[MRAM_RXF0].num = mram_config_vals[3] &
		FIELD_MAX(RXFC_FS_MASK);
	cdev->mcfg[MRAM_RXF1].off = cdev->mcfg[MRAM_RXF0].off +
		cdev->mcfg[MRAM_RXF0].num * RXF0_ELEMENT_SIZE;
	cdev->mcfg[MRAM_RXF1].num = mram_config_vals[4] &
		FIELD_MAX(RXFC_FS_MASK);
	cdev->mcfg[MRAM_RXB].off = cdev->mcfg[MRAM_RXF1].off +
		cdev->mcfg[MRAM_RXF1].num * RXF1_ELEMENT_SIZE;
	cdev->mcfg[MRAM_RXB].num = mram_config_vals[5];
	cdev->mcfg[MRAM_TXE].off = cdev->mcfg[MRAM_RXB].off +
		cdev->mcfg[MRAM_RXB].num * RXB_ELEMENT_SIZE;
	cdev->mcfg[MRAM_TXE].num = mram_config_vals[6];
	cdev->mcfg[MRAM_TXB].off = cdev->mcfg[MRAM_TXE].off +
		cdev->mcfg[MRAM_TXE].num * TXE_ELEMENT_SIZE;
	cdev->mcfg[MRAM_TXB].num = mram_config_vals[7] &
		FIELD_MAX(TXBC_NDTB_MASK);

	dev_dbg(cdev->dev,
		"sidf 0x%x %d xidf 0x%x %d rxf0 0x%x %d rxf1 0x%x %d rxb 0x%x %d txe 0x%x %d txb 0x%x %d\n",
		cdev->mcfg[MRAM_SIDF].off, cdev->mcfg[MRAM_SIDF].num,
		cdev->mcfg[MRAM_XIDF].off, cdev->mcfg[MRAM_XIDF].num,
		cdev->mcfg[MRAM_RXF0].off, cdev->mcfg[MRAM_RXF0].num,
		cdev->mcfg[MRAM_RXF1].off, cdev->mcfg[MRAM_RXF1].num,
		cdev->mcfg[MRAM_RXB].off, cdev->mcfg[MRAM_RXB].num,
		cdev->mcfg[MRAM_TXE].off, cdev->mcfg[MRAM_TXE].num,
		cdev->mcfg[MRAM_TXB].off, cdev->mcfg[MRAM_TXB].num);
}

int sunxi_can_init_ram(struct sunxi_can_classdev *cdev)
{
	int end, i, start;
	int err = 0;

	/* initialize the entire Message RAM in use to avoid possible
	 * ECC/parity checksum errors when reading an uninitialized buffer
	 * 初始化正在使用的整个消息 RAM，以避免在读取未初始化的缓冲区时可能出现的 ECC/奇偶校验和错误
	 */
	start = cdev->mcfg[MRAM_SIDF].off;
	end = cdev->mcfg[MRAM_TXB].off +
		cdev->mcfg[MRAM_TXB].num * TXB_ELEMENT_SIZE;

	for (i = start; i < end; i += 4) {
		err = sunxi_can_fifo_write_no_off(cdev, i, 0x0);
		if (err)
			break;
	}

	return err;
}
EXPORT_SYMBOL_GPL(sunxi_can_init_ram);

int sunxi_can_class_get_clocks(struct sunxi_can_classdev *cdev)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(cdev) || IS_ERR_OR_NULL(cdev->dev)
			|| IS_ERR_OR_NULL(cdev->dev->of_node))
		return -EINVAL;

	cdev->hclk = devm_clk_get(cdev->dev, "can_clk");
	cdev->cclk = devm_clk_get(cdev->dev, "can_bus");
	cdev->mclk = devm_clk_get(cdev->dev, "can_mbus");

	if (IS_ERR(cdev->hclk)) {
		dev_err(cdev->dev, "no h_clock found\n");
	}

	if (IS_ERR(cdev->cclk)) {
		dev_err(cdev->dev, "no c_clock found\n");
	}

	if (IS_ERR(cdev->mclk)) {
		dev_err(cdev->dev, "no clock found\n");
		ret = -ENODEV;
	}

	cdev->reset = devm_reset_control_get(cdev->dev, "can_rst");
	if (IS_ERR(cdev->reset)) {
		dev_err(cdev->dev, "Error: Get can rst failed\n");
		return -EINVAL;
	}

	if (of_property_read_u32(cdev->dev->of_node, "clock-frequency", &cdev->clk_freq)) {
		dev_info(cdev->dev, "failed to get clk freq, set to default %u Hz\n",
				DEFAULT_CAN_CLK_FREQ);
		cdev->clk_freq = DEFAULT_CAN_CLK_FREQ;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_can_class_get_clocks);

int sunxi_can_class_clk_init(struct sunxi_can_classdev *cdev)
{
	int ret = 0;
	u32 rate = 0;

	if (!IS_ERR_OR_NULL(cdev->reset)) {
		ret = reset_control_deassert(cdev->reset);
		if (ret)
			return ret;
	}

	if (!IS_ERR_OR_NULL(cdev->hclk)) {
		rate = clk_round_rate(cdev->hclk, cdev->clk_freq);
		ret = clk_set_rate(cdev->hclk, rate);
		if (ret) {
			pr_err("clk_set_rate:%u can_clk_freq:%u failed\n", rate, cdev->clk_freq);
			return ret;
		}
	}

	if (!IS_ERR_OR_NULL(cdev->cclk)) {
		ret = clk_prepare_enable(cdev->cclk);	// bus_clk
		if (ret)
			return ret;
	}

	if (!IS_ERR_OR_NULL(cdev->mclk)) {
		ret = clk_prepare_enable(cdev->mclk);	// mbus_clk
		if (ret)
			return ret;
	}

	if (!IS_ERR_OR_NULL(cdev->hclk)) {
		ret = clk_prepare_enable(cdev->hclk);	// clk
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_can_class_clk_init);

void sunxi_can_class_clk_deinit(struct sunxi_can_classdev *cdev)
{
	if (!IS_ERR_OR_NULL(cdev->hclk)) {
		clk_disable_unprepare(cdev->hclk);
	}

	if (!IS_ERR_OR_NULL(cdev->mclk)) {
		clk_disable_unprepare(cdev->mclk);
	}

	if (!IS_ERR_OR_NULL(cdev->cclk)) {
		clk_disable_unprepare(cdev->cclk);
	}

	if (!IS_ERR_OR_NULL(cdev->reset)) {
		reset_control_assert(cdev->reset);
	}
}
EXPORT_SYMBOL_GPL(sunxi_can_class_clk_deinit);

struct sunxi_can_classdev *sunxi_can_class_allocate_dev(struct device *dev,
						int sizeof_priv)
{
	struct sunxi_can_classdev *class_dev = NULL;
	u32 mram_config_vals[MRAM_CFG_LEN];
	struct net_device *net_dev;
	u32 tx_fifo_size;
	int ret;

	ret = fwnode_property_read_u32_array(dev_fwnode(dev),
						 "allwinner,ram-cfg",
						 mram_config_vals,
						 sizeof(mram_config_vals) / 4);
	if (ret) {
		dev_err(dev, "Could not get Message RAM configuration.");
		goto out;
	}

	/* Get TX FIFO size
	 * Defines the total amount of echo buffers for loopback
	 */
	tx_fifo_size = mram_config_vals[7];

	/* allocate the sunxi_can device */
	net_dev = alloc_candev(sizeof_priv, tx_fifo_size);
	if (!net_dev) {
		dev_err(dev, "Failed to allocate CAN device");
		goto out;
	}

	class_dev = netdev_priv(net_dev);
	class_dev->net = net_dev;
	class_dev->dev = dev;
	SET_NETDEV_DEV(net_dev, dev);

	sunxi_can_of_parse_mram(class_dev, mram_config_vals);
out:
	return class_dev;
}
EXPORT_SYMBOL_GPL(sunxi_can_class_allocate_dev);

void sunxi_can_class_free_dev(struct net_device *net)
{
	free_candev(net);
}
EXPORT_SYMBOL_GPL(sunxi_can_class_free_dev);

int sunxi_can_class_register(struct sunxi_can_classdev *cdev)
{
	int ret;

	if (cdev->pm_clock_support) {
		ret = sunxi_can_clk_start(cdev);
		if (ret)
			return ret;
	}

	ret = sunxi_can_dev_setup(cdev);
	if (ret)
		goto clk_disable;

	ret = register_sunxi_can_dev(cdev->net);
	if (ret) {
		dev_err(cdev->dev, "registering %s failed (err=%d)\n",
			cdev->net->name, ret);
		goto clk_disable;
	}

	devm_can_led_init(cdev->net);

	of_can_transceiver(cdev->net);

	dev_info(cdev->dev, "%s device registered (irq=%d, version=%d)\n",
		 KBUILD_MODNAME, cdev->net->irq, cdev->version);

	/* Probe finished
	 * Stop clocks. They will be reactivated once the CAN device is opened
	 */
	sunxi_can_clk_stop(cdev);

	return 0;

clk_disable:
	sunxi_can_clk_stop(cdev);

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_can_class_register);

int sunxi_can_class_suspend(struct device *dev)
{
	struct sunxi_can_classdev *cdev = dev_get_drvdata(dev);
	struct net_device *ndev = cdev->net;

	if (netif_running(ndev)) {
		netif_stop_queue(ndev);
		netif_device_detach(ndev);
		sunxi_can_stop(ndev);
		sunxi_can_clk_stop(cdev);
	}

	pinctrl_pm_select_sleep_state(dev);

	cdev->can.state = CAN_STATE_SLEEPING;

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_can_class_suspend);

int sunxi_can_class_resume(struct device *dev)
{
	struct sunxi_can_classdev *cdev = dev_get_drvdata(dev);
	struct net_device *ndev = cdev->net;

	pinctrl_pm_select_default_state(dev);

	cdev->can.state = CAN_STATE_ERROR_ACTIVE;

	if (netif_running(ndev)) {
		int ret;

		ret = sunxi_can_clk_start(cdev);
		if (ret)
			return ret;

		sunxi_can_init_ram(cdev);
		sunxi_can_start(ndev);
		netif_device_attach(ndev);
		netif_start_queue(ndev);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_can_class_resume);

MODULE_AUTHOR("Dong Aisheng <b29396@freescale.com>");
MODULE_AUTHOR("Dan Murphy <dmurphy@ti.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CAN bus driver for allwinner T536 CAN controller");
