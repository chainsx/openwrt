/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef SUNXI_RPROC_INTERNAL_H
#define SUNXI_RPROC_INTERNAL_H

#include <linux/mailbox_client.h>

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
#include "sunxi_rproc_subdev.h"
#endif

#ifdef CONFIG_AW_RPROC_TRACE_EVENT_PARSER
#include "trace_event/include/trace_event.h"
#endif

struct sunxi_mbox {
	struct mbox_chan *chan;
	struct mbox_client client;
#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
	struct task_struct *rx_daemon;
#else
	struct work_struct vq_work;
#endif
	int vq_id;
};

struct sunxi_rproc {
	struct sunxi_rproc_priv *rproc_priv;  /* dsp/riscv private resources */
	struct sunxi_rproc_standby *rproc_standby;

	struct sunxi_mbox mb;
	struct workqueue_struct *workqueue;
	struct list_head list;
	struct rproc *rproc;

	struct device_node *np;
	/* whether using the firmware which is provided by kernel remoteproc framework(request_firmware) */
	int is_using_kernel_fw;
	const struct firmware *fw;

	void __iomem *rsc_table_va;
	bool is_booted;
	char *name;
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
	struct sunxi_rproc_subdev subdev;
#endif

#ifdef CONFIG_AW_RPROC_TRACE_EVENT_PARSER
	aw_trace_event_parser_t trace_event_parser;
#endif
};

#endif
