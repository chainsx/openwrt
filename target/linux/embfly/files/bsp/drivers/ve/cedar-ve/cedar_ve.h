/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *    Filename: cedarv_ve.h
 *     Version: 0.01alpha
 * Description: Video engine driver API, Don't modify it in user space.
 *     License: GPLv2
 *
 *		Author  : xyliu <xyliu@allwinnertech.com>
 *		Date    : 2016/04/13
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
 /* Notice: It's video engine driver API, Don't modify it in user space. */
#ifndef _CEDAR_VE_H_
#define _CEDAR_VE_H_

#include <asm/io.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <uapi/linux/cedar_ve_uapi.h>

#include "ve_mem_list.h"

#if defined(CONFIG_ARCH_SUN300IW1)
#include "platforms/ve_plat_sun300iw1.h"
#elif defined(CONFIG_ARCH_SUN55IW6)
#include "platforms/ve_plat_sun55iw6.h"
#else
#include "platforms/ve_plat_common.h"
#endif

/* just for decoder case with v5v200 */
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
	#define VE_POWER_MANAGE_VALID	(1)
#else
	#define VE_POWER_MANAGE_VALID	(0)
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	#define VE_SUPPORT_MAP_MAX_VE_FREQ_BY_VF (1)
	#define VE_SUPPORT_DVFS		(1)
	#define VE_SUPPORT_DVFS_DEBUGFS		(1)
#else

	#define VE_SUPPORT_MAP_MAX_VE_FREQ_BY_VF (0)
	#define VE_SUPPORT_DVFS		(0)
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	#define VE_SUPPORT_DVFS_DEBUGFS		(1)
#else
	#define VE_SUPPORT_DVFS_DEBUGFS		(0)
#endif

#endif

#define MAX_VE_DEBUG_INFO_NUM (16)
#define MAX_VE_LOAD_PARAM_CHANNEL (16)

#define CEDAR_DEBUG
#if IS_ENABLED(CONFIG_AW_LOG_VERBOSE)
#define VE_LOGK(fmt, arg...) pr_info(fmt, ## arg)
#define VE_LOGV(fmt, arg...)
#define VE_LOGD(fmt, arg...) sunxi_debug(NULL, fmt, ## arg)
#define VE_LOGI(fmt, arg...) sunxi_info(NULL,  fmt, ## arg)
#define VE_LOGW(fmt, arg...) sunxi_warn(NULL,  fmt, ## arg)
#define VE_LOGE(fmt, arg...) sunxi_err(NULL,   fmt, ## arg)
#else
#define VE_LOGK(fmt, arg...) pr_info(fmt, ##arg)
#define VE_LOGV(fmt, arg...)
#define VE_LOGD(fmt, arg...) sunxi_debug(NULL, "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define VE_LOGI(fmt, arg...) sunxi_info(NULL,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define VE_LOGW(fmt, arg...) sunxi_warn(NULL,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define VE_LOGE(fmt, arg...) sunxi_err(NULL,   "%d %s(): "fmt, __LINE__, __func__, ## arg)
#endif

#define VE_LOCK_VDEC		0x01
#define VE_LOCK_VENC		0x02
#define VE_LOCK_JDEC		0x04
#define VE_LOCK_00_REG		0x08
#define VE_LOCK_04_REG		0x10
#define VE_LOCK_ERR		0x80
#define VE_LOCK_PROC_INFO	0x1000

struct ve_info { /* each object will bind a new file handler */
	unsigned int set_vol_flag;

	struct mutex lock_flag_io;
	u32 lock_flags; /* if flags is 0, means unlock status */
	u32 process_channel_id;
};

struct debug_head_info {
	unsigned int pid;
	unsigned int tid;
	unsigned int length;
};

struct ve_debug_info {
	struct debug_head_info head_info;
	char *data;
};

struct ve_case_load_info {
	struct ve_case_load_param load_param;
	int is_used;
	u32 process_channel_id;
};

struct iomap_para {
	volatile char *regs_ve;
	volatile char *regs_sys_cfg;
	volatile unsigned int *regs_ccmu;
	unsigned int *prcm_bass_vir;/* PRCM: power reset clock management */
	resource_size_t ve_reg_start;
	volatile char *regs_csi0;
	volatile char *regs_csi1;
};

enum VE_MODE {
	VE_MODE_NULL = -1,
	VE_MODE_ENCPP = 0,
	VE_MODE_ENC,
	VE_MODE_DE,
	VE_MODE_VCUENC,
	VE_MODE_CNT,
};

struct ve_dvfs_info {
	unsigned int dvfs_index;
	unsigned int voltage; /* mv */
	unsigned int ve_freq; /* MHz */
};

struct cedar_dev {
	struct cdev cdev;		/* char device struct */
	struct device *dev;		/* ptr to class device struct */
	struct device *plat_dev;	/* ptr to class device struct */
	struct class  *class;		/* class for auto create device node */

	struct semaphore sem;		/* mutual exclusion semaphore */
	spinlock_t lock;
	wait_queue_head_t wq;		/* wait queue for poll ops */

	struct iomap_para iomap_addrs;	/* io remap addrs */

	struct timer_list cedar_engine_timer;
	struct timer_list cedar_engine_timer_rel;

	u32 irq;			/* cedar video engine irq number */
	u32 de_irq_flag;		/* flag of video decoder engine irq generated */
	u32 de_irq_value;		/* value of video decoder engine irq */
	u32 en_irq_flag;		/* flag of video encoder engine irq generated */
	u32 en_irq_value;		/* value of video encoder engine irq */
	u32 irq_has_enable;
	int ref_count;
	int last_min_freq;

	u32 jpeg_irq_flag;		/* flag of video jpeg dec irq generated */
	u32 jpeg_irq_value;		/* value of video jpeg dec  irq */

	struct mutex lock_vdec;
	struct mutex lock_jdec;
	struct mutex lock_venc;
	struct mutex lock_00_reg;
	struct mutex lock_04_reg;
	struct aw_mem_list_head list;	/* buffer list */
	struct mutex lock_mem;
	unsigned char bMemDevAttachFlag;
	u32 power_manage_request_ref;
	struct ve_debug_info debug_info[MAX_VE_DEBUG_INFO_NUM];
	int debug_info_cur_index;
	struct mutex lock_debug_info;
	struct reset_control *reset;
	struct clk *ve_clk;
#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	struct reset_control *reset_ve;
	struct clk *bus_ve3_clk;
	struct clk *bus_ve_clk;
	struct clk *mbus_ve3_clk;
#else
	struct clk *bus_clk;
	struct clk *mbus_clk;
#endif
	struct clk *parent_clk;
	struct clk *power_clk;

	struct regulator *regulator;
	int voltage;			/* mv */
	int bInitEndFlag;

	struct ve_case_load_info load_infos[MAX_VE_LOAD_PARAM_CHANNEL];
	int user_setting_ve_freq;	/* MHz */
	int ve_freq_setup_by_dts_config;
#if VE_SUPPORT_DVFS_DEBUGFS
	struct dentry *dvfs_root;
#endif

	enum VE_MODE ve_mode;
	u32 vcuenc_irq_flag;			/* flag of vcu of encoder engine irq generated */
	u32 vcuenc_csi_irq_flag;

	u32 ve_top_reg_offset;
	unsigned int dvfs_array_num;
	struct ve_dvfs_info *dvfs_array;
	unsigned int ve_freq_dtsi;
};

extern struct dentry *ve_debugfs_root;

/*** for rt media only begain ***/
void *_cedardev_open(void);
void *_cedardev_mmap(void);
int _cedardev_release(void *info);
int _cedardev_ioctl(void *info_handle, unsigned int cmd, unsigned long arg);
/*** for rt media only end    ***/

/*** pltform code new struct definition begain ***/
struct sunxi_ve_irq {
	struct cedar_dev *cedar_devp;
	wait_queue_head_t *wait_ve;
};

int resource_iomap_init(struct device_node *node, struct iomap_para *iomap_addrs);
void ve_irq_work(struct sunxi_ve_irq *irq);
int ioctl_flush_cache_range(unsigned long arg, uint8_t user, struct cedar_dev *cedar_devp);
int ioctl_get_csi_online_related_info(unsigned long arg, uint8_t user, struct cedar_dev *cedar_devp);

/* debug */
int ve_debug_register_driver(void);
void ve_debug_unregister_driver(void);
void ve_debug_open(struct ve_info *vi);
void ve_debug_release(struct ve_info *vi);
int ioctl_set_proc_info(unsigned long arg, struct ve_info *vi);
int ioctl_copy_proc_info(unsigned long arg, struct ve_info *vi);
int ioctl_stop_proc_info(unsigned long arg, struct ve_info *vi);
/*** pltform code new struct definition end    ***/

#endif
