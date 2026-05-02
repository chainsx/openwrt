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
#ifndef SUNXI_RPROC_TRACE_H
#define SUNXI_RPROC_TRACE_H

#ifdef CONFIG_AW_RPROC_TRACE_EVENT_PARSER
#include "trace_event/include/trace_event.h"
#endif

struct dentry *sunxi_rproc_create_aw_trace_file(const char *name, struct rproc *rproc,
		struct rproc_debug_trace *trace);

int sunxi_rproc_trace_dump(void *trace_mem, int trace_mem_len);

#ifdef CONFIG_AW_RPROC_TRACE_EVENT_PARSER
void *parser_addr_map_on_linux_kernel(const aw_trace_event_parser_t *parser, uint64_t da, size_t len);
#endif

#endif
