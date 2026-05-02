/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024,
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME	"VE"
#include <sunxi-log.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>

#include "cedar_ve.h"

struct ve_dvfs_pair ve_dvfs[3] = {
	/* dvfs-index   dvfs_name   dvfs_freq_name   dvfs_volt_name */
	{0x00, "opp-vf0000", "opp-hz-0", "opp-microvolt-0"},//vf 0
	{0x01, "opp-vf0100", "opp-hz-0", "opp-microvolt-0"},//vf 1
	{0x02, "opp-vf0200", "opp-hz-0", "opp-microvolt-0"},//vf 2
};

struct dentry *ve_debugfs_root;

struct ve_debugfs_proc {
	unsigned int len;
	char data[VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL];
};

struct ve_debugfs_buffer {
	unsigned char  cur_channel_id;
	unsigned int proc_len[VE_DEBUGFS_MAX_CHANNEL];
	char *proc_buf[VE_DEBUGFS_MAX_CHANNEL];
	char *data;
	struct mutex lock_proc;
};
struct ve_debugfs_buffer ve_debug_proc_info;

/* resource */
int resource_iomap_init(struct device_node *node, struct iomap_para *iomap_addrs)
{
	int ret = 0;
	struct resource res;

	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		VE_LOGE("parse device node resource failed\n");
		return -1;
	}
	iomap_addrs->ve_reg_start = res.start;

	/* map for macc io space */
	iomap_addrs->regs_ve = of_iomap(node, 0);
	if (!iomap_addrs->regs_ve) {
		VE_LOGW("ve Can't map registers\n");
		return -1;
	}

	/* map for sys_config io space */
	iomap_addrs->regs_sys_cfg = (char *)of_iomap(node, 1);
	if (!iomap_addrs->regs_sys_cfg) {
		VE_LOGW("ve Can't map regs_sys_cfg registers, maybe no use\n");
	}

	/* map for ccmu io space */
	iomap_addrs->regs_ccmu = (unsigned int *)of_iomap(node, 2);
	if (!iomap_addrs->regs_ccmu) {
		VE_LOGW("ve Can't map regs_ccmu registers, maybe no use\n");
	}
#if VE_POWER_MANAGE_VALID
	iomap_addrs->prcm_bass_vir = (unsigned int *)of_iomap(node, 3);
	if (!iomap_addrs->prcm_bass_vir) {
		VE_LOGW("ve Can't map prcm_bass_vir registers\n");
	}
#endif

	return 0;
}

/* irq */
void ve_irq_work(struct sunxi_ve_irq *irq_work)
{
	struct cedar_dev *cedar_devp = irq_work->cedar_devp;
	wait_queue_head_t *wait_ve = irq_work->wait_ve;

	unsigned long ve_int_status_reg;
	unsigned long ve_int_ctrl_reg;
	unsigned int status;
	volatile int val;
	int modual_sel;
	unsigned int interrupt_enable;
	struct iomap_para addrs = cedar_devp->iomap_addrs;

	modual_sel = readl(addrs.regs_ve + 0x0);
#if IS_ENABLED(CONFIG_ARCH_SUN3IW1P1)
	if (modual_sel&0xa) {
		if ((modual_sel&0xb) == 0xb) {
			/* jpg enc */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/* isp */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		if (status && interrupt_enable) {
			/* disable interrupt */
			if ((modual_sel & 0xb) == 0xb) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			}

			cedar_devp->en_irq_value = 1;	/* hx modify 2011-8-1 16:08:47 */
			cedar_devp->en_irq_flag = 1;
			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);		  /* ioctl */
		}
	}
#else
	if (modual_sel&(3<<6)) {
		if (modual_sel&(1<<7)) {
			/* avc enc */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/* isp */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		/* modify by fangning 2013-05-22 */
		if (status && interrupt_enable) {
			/* disable interrupt */
			/* avc enc */
			if (modual_sel&(1<<7)) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				/* isp */
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			}
			/* hx modify 2011-8-1 16:08:47 */
			cedar_devp->en_irq_value = 1;
			cedar_devp->en_irq_flag = 1;
			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);
		}
	}
#endif

#if ((defined CONFIG_ARCH_SUN8IW8P1) || (defined CONFIG_ARCH_SUN50I) || \
	(defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1) \
	|| (defined CONFIG_ARCH_SUN8IW16P1) || (defined CONFIG_ARCH_SUN8IW19P1))
	if (modual_sel&(0x20)) {
		ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xe00 + 0x1c);
		ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xe00 + 0x14);
		interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x38);

		status = readl((void *)ve_int_status_reg);

		if ((status&0x7) && interrupt_enable) {
			/* disable interrupt */
			val = readl((void *)ve_int_ctrl_reg);
			writel(val & (~0x38), (void *)ve_int_ctrl_reg);

			cedar_devp->jpeg_irq_value = 1;
			cedar_devp->jpeg_irq_flag = 1;

			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);
		}
	}
#endif

	modual_sel &= 0xf;
	if (modual_sel <= 6) {
		/* estimate Which video format */
		switch (modual_sel) {
		case 0: /* mpeg124 */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7c);
			break;
		case 1: /* h264 */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x200 + 0x28);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x200 + 0x20);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 2: /* vc1 */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve +
				0x300 + 0x2c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x300 + 0x24);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 3: /* rv */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x400 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x400 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 4: /* hevc */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x500 + 0x38);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x500 + 0x30);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 6: /* scaledown */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xF00 +0x08);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xF00 + 0x04);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			break;
		default:
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			VE_LOGW("ve mode :%x " "not defined!\n", modual_sel);
			break;
		}

		status = readl((void *)ve_int_status_reg);

		/* modify by fangning 2013-05-22 */
		if ((status&0xf) && interrupt_enable) {
			/* disable interrupt */
			if (modual_sel == 0) {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7c), (void *)ve_int_ctrl_reg);
			} else if (modual_sel == 6) {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			} else {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0xf), (void *)ve_int_ctrl_reg);
			}
			cedar_devp->de_irq_value = 1;
			cedar_devp->de_irq_flag = 1;
			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);
		}
	}
}

/* debug */
static int ve_debugfs_open(struct inode *inode, struct file *file)
{
	int i = 0;
	char *pData;
	struct ve_debugfs_proc *pVeProc;

	pVeProc = kmalloc(sizeof(*pVeProc), GFP_KERNEL);
	if (pVeProc == NULL) {
		VE_LOGE("kmalloc pVeProc fail\n");
		return -ENOMEM;
	}
	pVeProc->len = 0;
	memset(pVeProc->data, 0, VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL);

	pData = pVeProc->data;
	mutex_lock(&ve_debug_proc_info.lock_proc);
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++) {
		if (ve_debug_proc_info.proc_buf[i] != NULL) {
			memcpy(pData, ve_debug_proc_info.proc_buf[i],
			       ve_debug_proc_info.proc_len[i]);
			pData += ve_debug_proc_info.proc_len[i];
			pVeProc->len += ve_debug_proc_info.proc_len[i];
		}
	}
	mutex_unlock(&ve_debug_proc_info.lock_proc);

	file->private_data = pVeProc;
	return 0;
}

static ssize_t ve_debugfs_read(struct file *file, char __user *user_buf,
			       size_t nbytes, loff_t *ppos)
{
	struct ve_debugfs_proc *pVeProc = file->private_data;

	if (pVeProc->len == 0) {
		VE_LOGD("there is no any codec working currently\n");
		return 0;
	}

	return simple_read_from_buffer(user_buf, nbytes, ppos, pVeProc->data, pVeProc->len);
}

static int ve_debugfs_release(struct inode *inode, struct file *file)
{
	struct ve_debugfs_proc *pVeProc = file->private_data;

	kfree(pVeProc);
	pVeProc = NULL;
	file->private_data = NULL;

	return 0;
}

static const struct file_operations ve_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = ve_debugfs_open,
	.llseek = no_llseek,
	.read = ve_debugfs_read,
	.release = ve_debugfs_release,
};

int ve_debug_register_driver(void)
{
	struct dentry *dent;
	int i;

	/* debugfs_mpp_root defined out of this module */
#if IS_ENABLED(CONFIG_SUNXI_MPP)
	ve_debugfs_root = debugfs_mpp_root;
#else
	ve_debugfs_root = debugfs_create_dir("ve", NULL);
#endif

	if (IS_ERR_OR_NULL(ve_debugfs_root)) {
		VE_LOGE("debugfs root is null please check!\n");
		return -ENOENT;
	}
	dent = debugfs_create_file("ve", 0444, ve_debugfs_root,
				   NULL, &ve_debugfs_fops);
	if (IS_ERR_OR_NULL(dent)) {
		VE_LOGE("Unable to create debugfs status file.\n");
		debugfs_remove_recursive(ve_debugfs_root);
		ve_debugfs_root = NULL;
		return -ENODEV;
	}

	memset(&ve_debug_proc_info, 0, sizeof(ve_debug_proc_info));
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++)
		ve_debug_proc_info.proc_buf[i] = NULL;

	ve_debug_proc_info.data = kmalloc(VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL, GFP_KERNEL);
	if (!ve_debug_proc_info.data) {
		VE_LOGE("kmalloc proc buffer failed!\n");
		return -ENOMEM;
	}
	mutex_init(&ve_debug_proc_info.lock_proc);
	VE_LOGI("ve_debug_proc_info:%p, data:%p, lock:%p\n",
		&ve_debug_proc_info,
		ve_debug_proc_info.data,
		&ve_debug_proc_info.lock_proc);

	return 0;
}

void ve_debug_unregister_driver(void)
{
	if (ve_debugfs_root == NULL) {
		VE_LOGW("note: debug root already is null\n");
		return;
	}
	debugfs_remove_recursive(ve_debugfs_root);
	ve_debugfs_root = NULL;

	mutex_destroy(&ve_debug_proc_info.lock_proc);
	kfree(ve_debug_proc_info.data);
}

void ve_debug_open(struct ve_info *vi)
{
	(void)vi;
	/* NULL */
}

void ve_debug_release(struct ve_info *vi)
{
	if (vi->lock_flags) {
		if (vi->lock_flags & VE_LOCK_PROC_INFO) {
			mutex_unlock(&ve_debug_proc_info.lock_proc);
		}
	}
}

/* ioctl -> IOCTL_FLUSH_CACHE_RANGE */
#if IS_ENABLED(CONFIG_ARM64)
#if (IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW9) || IS_ENABLED(CONFIG_ARCH_SUN50IW10)) \
	&& (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
extern void cedar_dma_flush_range(const void *, const void *);
#else
extern void cedar_dma_flush_range(const void *, size_t);
#endif
#else
extern void cedar_dma_flush_range(const void *, const void *);
#endif

int ioctl_flush_cache_range(unsigned long arg, uint8_t user, struct cedar_dev *cedar_devp)
{
	struct cache_range data;
	#if IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_64BIT)
	#else
	u32 addr_start = 0;
	u32 addr_end = 0;
	#endif

	(void)cedar_devp;

	if (user) {
		if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
			return -EFAULT;
	} else {
		memcpy(&data, (void *)arg, sizeof(data));
	}

	#if IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_64BIT)
	if (IS_ERR_OR_NULL((void *)data.start) || IS_ERR_OR_NULL((void *)data.end)) {
		VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
			(u32)data.start, (u32)data.end);
		return -EFAULT;
	}
	#else
	addr_start = (u32)(data.start & 0xffffffff);
	addr_end = (u32)(data.end & 0xffffffff);

	if (IS_ERR_OR_NULL((void *)addr_start) || IS_ERR_OR_NULL((void *)addr_end)) {
		VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
			(u32)data.start, (u32)data.end);
		return -EFAULT;
	}
	#endif

	/*
	 * VE_LOGI("ion flush_range start:%lx end:%lx size:%lx\n",
	 * data.start, data.end, data.end - data.start);
	 */
	#if IS_ENABLED(CONFIG_ARM64)
	/*
	 * VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
	 * (u32)data.start, (u32)data.end);
	 */
	#if (IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW9) || IS_ENABLED(CONFIG_ARCH_SUN50IW10)) \
		&& (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	cedar_dma_flush_range((void *)data.start, (void *)data.end);
	#else
	cedar_dma_flush_range((void *)data.start, (unsigned long)(data.end - data.start));
	#endif
	/* dma_sync_single_for_cpu(cedar_devp->plat_dev,
	 * (u32)data.start,data.end - data.start,DMA_BIDIRECTIONAL);
	 */
	#else
	#if IS_ENABLED(CONFIG_64BIT) && IS_ENABLED(CONFIG_RISCV)
	dma_usr_va_wb_range((void *)data.start, (unsigned long)(data.end - data.start));
	#else
	/* dmac_flush_range((void *)addr_start, (void *)addr_end); */
	cedar_dma_flush_range((void *)addr_start, (void *)addr_end);
	#endif
	#endif

	if (copy_to_user((void __user *)arg, &data, sizeof(data)))
		return -EFAULT;

	return 0;
}

int ioctl_get_csi_online_related_info(unsigned long arg, uint8_t user, struct cedar_dev *cedar_devp)
{
	(void)arg;
	(void)user;
	(void)cedar_devp;

	VE_LOGW("unsupport api\n");

	return 0;
}

/* ioctl -> IOCTL_SET_PROC_INFO
 *       -> IOCTL_STOP_PROC_INFO
 *       -> IOCTL_COPY_PROC_INFO
 */
int ioctl_set_proc_info(unsigned long arg, struct ve_info *vi)
{
	struct VE_PROC_INFO ve_info;
	unsigned char channel_id = 0;
	u32 lock_type = VE_LOCK_PROC_INFO;

	if (ve_debugfs_root == NULL)
		return 0;

	mutex_lock(&ve_debug_proc_info.lock_proc);
	if (copy_from_user(&ve_info, (void __user *)arg, sizeof(ve_info))) {
		VE_LOGW("IOCTL_SET_PROC_INFO copy_from_user fail\n");
		mutex_unlock(&ve_debug_proc_info.lock_proc);
		return -EFAULT;
	}

	channel_id = ve_info.channel_id;
	if (channel_id >= VE_DEBUGFS_MAX_CHANNEL) {
		VE_LOGW("set channel[%c] is bigger than max channel[%d]\n",
			channel_id, VE_DEBUGFS_MAX_CHANNEL);
		mutex_unlock(&ve_debug_proc_info.lock_proc);
		return -EFAULT;
	}

	mutex_lock(&vi->lock_flag_io);
	vi->lock_flags |= lock_type;
	mutex_unlock(&vi->lock_flag_io);

	ve_debug_proc_info.cur_channel_id = ve_info.channel_id;
	ve_debug_proc_info.proc_len[channel_id] = ve_info.proc_info_len;
	ve_debug_proc_info.proc_buf[channel_id] = ve_debug_proc_info.data
						+ channel_id * VE_DEBUGFS_BUF_SIZE;

	return 0;
}

int ioctl_copy_proc_info(unsigned long arg, struct ve_info *vi)
{
	unsigned char channel_id;
	u32 lock_type = VE_LOCK_PROC_INFO;

	if (ve_debugfs_root == NULL)
		return 0;

	mutex_lock(&vi->lock_flag_io);
	vi->lock_flags &= (~lock_type);
	mutex_unlock(&vi->lock_flag_io);

	channel_id = ve_debug_proc_info.cur_channel_id;
	if (copy_from_user(ve_debug_proc_info.proc_buf[channel_id], (void __user *)arg,
			   ve_debug_proc_info.proc_len[channel_id])) {
		VE_LOGW("IOCTL_COPY_PROC_INFO copy_from_user fail\n");
		mutex_unlock(&ve_debug_proc_info.lock_proc);
		return -EFAULT;
	}
	mutex_unlock(&ve_debug_proc_info.lock_proc);
	return 0;
}

int ioctl_stop_proc_info(unsigned long arg, struct ve_info *vi)
{
	unsigned char channel_id;
	(void)vi;

	if (ve_debugfs_root == NULL)
		return 0;

	channel_id = arg;
	ve_debug_proc_info.proc_buf[channel_id] = NULL;

	return 0;
}
