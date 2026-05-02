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
#include "sunxi_hrc.h"
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mc.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>

#include <linux/timer.h>

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dengguiling");
MODULE_DESCRIPTION("Allwinner HDMI Rx Capture Driver");

struct hrc_format {
	u8  name[32];
	u32 fourcc;
	u32 depth;
	u8  buffers;
};

static struct hrc_format hrc_support_fmt[] = {
	{
		.name       = "BGR888_24",
		.fourcc     = V4L2_PIX_FMT_BGR24,
		.depth      = 24,
	}, {
		.name       = "YUV444_24",
		.fourcc     = V4L2_PIX_FMT_YUV24,
		.depth      = 24,
	}, {
		/* special format, DE and G2D not support! */
		.name       = "YUV444_NV24",
		.fourcc     = V4L2_PIX_FMT_NV24,
		.depth      = 24,
	}, {
		.name       = "YUV422_NV16",
		.fourcc     = V4L2_PIX_FMT_NV16,
		.depth      = 16,
	}, {
		.name       = "YUV420_NV12",
		.fourcc     = V4L2_PIX_FMT_NV12,
		.depth      = 12,
	},
};

struct hrc_buffer {
	struct vb2_buffer vb;
	struct list_head list;
};

struct hrc_counter {
	unsigned long long vsync;
	unsigned long long cfg_finish;
	unsigned long long wb_finish;
	unsigned long long overflow;
	unsigned long long unusal;
	unsigned long long no_buffer;
	unsigned long long error;
};

struct hrc_fps {
	unsigned long jiffies;
	unsigned long ms;
};

struct hrc_drv {
	struct platform_device *pdev;
	struct v4l2_device     v4l2_dev;
	struct video_device    *video_dev;
	struct vb2_queue       vb2_q;
	struct list_head       buf_list;
	struct list_head       buf_active_list;
	struct workqueue_struct *buf_wq;
	struct work_struct     buf_work;
	wait_queue_head_t      buf_wait_queue;

	/* mutex lock for video device */
	struct mutex mlock;
	/* spin lock for buffer */
	spinlock_t   slock;

	/* clk */
	struct clk *clk_hrc;
	struct clk *clk_bus_hrc;
	struct reset_control *rst_bus_hrc;
	struct reset_control *rst_bus_hdmi_rx;

	/* dts */
	u8 fpga;

	/* var */
	int irq;
	u32 input;
	u32 caps;
	u32 denominator;

	struct v4l2_pix_format  pix_fmt;
	struct hrc_ctrl_param   ctrl_param;
	struct hrc_input_param  in_param;
	struct hrc_output_param out_param;

	/* status */
	u8                 capturing;
	struct hrc_counter counter;
	struct hrc_fps     fps;

	/* v4l2 debug: only test v4l2 framework */
	u8                v4l2_debug;
	struct timer_list timer;
	/* ddr debug */
	u8                params_custom;
	u8		  ddr_debug;
};

u32 hrc_loglevel;
static dev_t hrc_devid;
static struct cdev *hrc_cdev;
static struct class *hrc_class;
static struct device *hrc_device;
static void *dbg_dma_buffer;
static dma_addr_t dbg_dma_handle;
static u32 dbg_buf_size;
static u8 cap_start;
static u8 cap_done;
static void *dbg_out_dma_buffer;
static dma_addr_t dbg_out_dma_handle;
static u32 dbg_out_buf_size;
#define CTRL_PARAMS_CUSTOM	BIT(0)
#define INPUT_PARAMS_CUSTOM	BIT(1)
#define OUTPUT_PARAMS_CUSTOM	BIT(2)

static int sunxi_hrc_enable_resource(struct hrc_drv *hrc_drv)
{
	if (hrc_drv->rst_bus_hrc) {
		if (reset_control_deassert(hrc_drv->rst_bus_hrc)) {
			hrc_err("rst_bus_hrc deassert failed!\n");
			return -1;
		}
	}

	/* If rst_bus_hdmi_rx is not enabled, HRC cannot work. */
	if (hrc_drv->rst_bus_hdmi_rx && reset_control_status(hrc_drv->rst_bus_hdmi_rx)) {
		if (hrc_drv->fpga) {
			if (reset_control_deassert(hrc_drv->rst_bus_hdmi_rx)) {
				hrc_err("rst_bus_hdmi_rx deassert failed!\n");
				return -1;
			}
		} else {
			hrc_wrn("rst_bus_hdmi_rx is assert!!!\n");
		}
	}

	if (hrc_drv->clk_hrc) {
		if (clk_prepare_enable(hrc_drv->clk_hrc)) {
			hrc_err("clk_hrc enable failed!\n");
			return -1;
		}
	}

	if (hrc_drv->clk_bus_hrc) {
		if (clk_prepare_enable(hrc_drv->clk_bus_hrc)) {
			hrc_err("clk_bus_hrc enable failed!\n");
			return -1;
		}
	}

	return 0;
}

static int sunxi_hrc_disable_resource(struct hrc_drv *hrc_drv)
{
	if (hrc_drv->clk_hrc)
		clk_disable_unprepare(hrc_drv->clk_hrc);

	if (hrc_drv->clk_bus_hrc)
		clk_disable_unprepare(hrc_drv->clk_bus_hrc);

	if (hrc_drv->rst_bus_hrc) {
		if (reset_control_assert(hrc_drv->rst_bus_hrc)) {
			hrc_err("rst_bus_hrc assert failed!\n");
			return -1;
		}
	}

	return 0;
}

static int sunxi_hrc_set_next_buf_addr(struct hrc_drv *hrc_drv)
{
	int ret = -1;
	struct hrc_buffer *buf = NULL;
	dma_addr_t buf_addr;
	u32 offset = 0;
	struct hrc_addr out_addr;

	if (!hrc_drv) {
		hrc_err("invalid params\n");
		return -EINVAL;
	}

	if (list_empty(&hrc_drv->buf_list)) {
		schedule_work(&hrc_drv->buf_work);
		return -1;
	}

	buf = list_entry(hrc_drv->buf_list.next, struct hrc_buffer, list);

	list_del(&buf->list);

	list_add_tail(&buf->list, &hrc_drv->buf_active_list);

	buf_addr = vb2_dma_contig_plane_dma_addr(&buf->vb, 0);

	switch (hrc_drv->pix_fmt.pixelformat) {
	case V4L2_PIX_FMT_NV24:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
		offset = hrc_drv->pix_fmt.width * hrc_drv->pix_fmt.height;
		break;
	default:
		offset = 0;
		break;
	}

	memset(&out_addr, 0, sizeof(out_addr));
	out_addr.head[0] = buf_addr & 0xFFFFFFFF;
	out_addr.head[1] = (buf_addr >> 32) & 0xFF;
	out_addr.body[0] = (buf_addr + offset) & 0xFFFFFFFF;
	out_addr.body[1] = ((buf_addr + offset) >> 32) & 0xFF;

	ret = sunxi_hrc_hardware_config_addr(out_addr);
	if (ret) {
		hrc_err("hardware set addr error!\n");
		return ret;
	}

	return 0;
}

static int sunxi_hrc_config_params(struct hrc_drv *hrc_drv)
{
	int ret = -1;
	struct hrc_ctrl_param *ctrl_param = &hrc_drv->ctrl_param;
	struct hrc_input_param *in_param = &hrc_drv->in_param;
	struct hrc_output_param *out_param = &hrc_drv->out_param;

	if (hrc_drv->ddr_debug) {
		if (!sunxi_hrc_hardware_check_format_support(in_param->format,
							     out_param->format)) {
			hrc_err("hardware do not support intput: %d output: %d\n",
				in_param->format, out_param->format);
			return -1;
		}

		goto CONFIG_TO_HARDWARE;
	}

	if (!(hrc_drv->params_custom & INPUT_PARAMS_CUSTOM)) {
		memset(ctrl_param, 0, sizeof(*ctrl_param));
		memset(in_param, 0, sizeof(*in_param));

		ctrl_param->data_src = HRC_DATA_SRC_HDMI_RX;

		/* TODO: input params: read from HDMI RX */
		in_param->size.width   = 1920;
		in_param->size.height  = 1080;
		in_param->format       = HRC_FMT_RGB;
		in_param->depth        = HRC_DEPTH_8;
		in_param->csc          = HRC_CSC_BT601;
		in_param->quantization = HRC_QUANTIZATION_FULL;
	}

	memset(out_param, 0, sizeof(*out_param));

	/* TODO: output params */
	switch (hrc_drv->pix_fmt.pixelformat) {
	case V4L2_PIX_FMT_BGR24:
		out_param->format = HRC_FMT_RGB;
		break;
	case V4L2_PIX_FMT_YUV24:
		out_param->format = HRC_FMT_YUV444_1PLANE;
		break;
	case V4L2_PIX_FMT_NV24:
		out_param->format = HRC_FMT_YUV444;
		break;
	case V4L2_PIX_FMT_NV16:
		out_param->format = HRC_FMT_YUV422;
		break;
	case V4L2_PIX_FMT_NV12:
		out_param->format = HRC_FMT_YUV420;
		break;
	default:
		hrc_err("not support format!\n");
		return -1;
	}

	if (!sunxi_hrc_hardware_check_format_support(in_param->format,
						     out_param->format)) {
		hrc_err("hardware do not support intput: %d output: %d\n",
			in_param->format, out_param->format);
		return -1;
	}

	out_param->y_size.width  = hrc_drv->pix_fmt.width;
	out_param->y_size.height = hrc_drv->pix_fmt.height;

	if ((in_param->format == HRC_FMT_RGB || in_param->format == HRC_FMT_YUV444) &&
	    (out_param->format == HRC_FMT_YUV422 || out_param->format == HRC_FMT_YUV420))
		out_param->c_size.width  = hrc_drv->pix_fmt.width / 2;
	else
		out_param->c_size.width  = hrc_drv->pix_fmt.width;

	out_param->c_size.height = hrc_drv->pix_fmt.height;

	out_param->uncompact = 0;

	/* TODO: get form user */
	if (out_param->format == HRC_FMT_RGB || out_param->format == HRC_FMT_YUV444_1PLANE) {
		out_param->y_stride = hrc_drv->pix_fmt.width * 3;
		out_param->c_stride = 0;
	} else if (out_param->format == HRC_FMT_YUV444) {
		out_param->y_stride = hrc_drv->pix_fmt.width;
		out_param->c_stride = hrc_drv->pix_fmt.width * 2;
	} else {
		out_param->y_stride = hrc_drv->pix_fmt.width;
		out_param->c_stride = hrc_drv->pix_fmt.width;
	}

	switch (hrc_drv->pix_fmt.ycbcr_enc) {
	case V4L2_YCBCR_ENC_601:
		out_param->csc = HRC_CSC_BT709;
		break;
	case V4L2_YCBCR_ENC_709:
		out_param->csc = HRC_CSC_BT709;
		break;
	case V4L2_YCBCR_ENC_BT2020:
		out_param->csc = HRC_CSC_BT2020;
		break;
	default:
		out_param->csc = HRC_CSC_BT709;
	}

	switch (hrc_drv->pix_fmt.quantization) {
	case V4L2_QUANTIZATION_FULL_RANGE:
		out_param->quantization = HRC_QUANTIZATION_FULL;
		break;
	case V4L2_QUANTIZATION_LIM_RANGE:
		out_param->quantization = HRC_QUANTIZATION_LIMIT;
		break;
	default:
		out_param->quantization = HRC_QUANTIZATION_DEFAULT;
	}

	switch (hrc_drv->pix_fmt.field) {
	case V4L2_FIELD_TOP:
		ctrl_param->field_mode    = HRC_FIELD_MODE_FIELD;
		ctrl_param->field_inverse = 0;
		ctrl_param->field_order   = HRC_FIELD_ORDER_TOP;
		break;
	case V4L2_FIELD_BOTTOM:
		ctrl_param->field_mode    = HRC_FIELD_MODE_FIELD;
		ctrl_param->field_inverse = 0;
		ctrl_param->field_order   = HRC_FIELD_ORDER_BOTTOM;
		break;
	default:
		ctrl_param->field_mode    = HRC_FIELD_MODE_FRAME;
		ctrl_param->field_inverse = 0;
		ctrl_param->field_order   = HRC_FIELD_ORDER_BOTTOM;
	}

	/* TODO: control params */
	ctrl_param->timeout_cycle = 0;  /* Ref: input fps */

CONFIG_TO_HARDWARE:
	ret = sunxi_hrc_hardware_config(*ctrl_param, *in_param, *out_param);
	if (ret) {
		hrc_err("hardware config params error!\n");
		return ret;
	}

	return 0;
}

/* --- TEST FUNCTION START --- */

static void sunxi_hrc_fill_buff(struct hrc_drv *hrc_drv, char *vbuf)
{
	static int repeat;
	static int color_start;
	static unsigned int color[3] = {0xFF, 0xFF << 8, 0xFF << 16};
	int i, j;
	/* int w = hrc_drv->pix_fmt.width; */
	int h = hrc_drv->pix_fmt.height;
	int bpp = hrc_drv->pix_fmt.bytesperline;
	/* int depth = hrc_drv->pix_fmt.bytesperline / hrc_drv->pix_fmt.width; */
	int color_idx = color_start;
#define COLOR_HEIGHT 32
#define REPEAT_FRAME 15

	/* !!! FIXME: only support RGB888 !!! */

	for (i = 0 ; i < h; i++) {
		if (!(i % COLOR_HEIGHT) && i != 0) {
			color_idx++;
			color_idx %= ARRAY_SIZE(color);
		}

		for (j = 0; j < bpp; j += 3) {
			vbuf[(bpp * i) + j + 0] = (color[color_idx] >> 0) & 0xFF;
			vbuf[(bpp * i) + j + 1] = (color[color_idx] >> 8) & 0xFF;
			vbuf[(bpp * i) + j + 2] = (color[color_idx] >> 16) & 0xFF;
		}
	}

	/* slow down */
	repeat++;
	repeat %= REPEAT_FRAME;
	if (!repeat) {
		color_start++;
		color_start %= ARRAY_SIZE(color);
	}
}

static void sunxi_hrc_timer_handler(struct timer_list *t)
{
	struct hrc_drv *hrc_drv = container_of(t, struct hrc_drv, timer);
	struct hrc_buffer *buf = NULL;
	char *vbuf;

	spin_lock(&hrc_drv->slock);
	if (!list_empty(&hrc_drv->buf_list)) {
		buf = list_entry(hrc_drv->buf_list.next, struct hrc_buffer, list);
		if (buf->vb.state != VB2_BUF_STATE_ACTIVE) {
			hrc_err("buffer no active!!!\n");
			return;
		}
		list_del(&buf->list);
	} else {
		spin_unlock(&hrc_drv->slock);
		goto out;
	}
	spin_unlock(&hrc_drv->slock);

	vbuf = vb2_plane_vaddr(&buf->vb, 0);

	memset(vbuf, 0xff, hrc_drv->pix_fmt.sizeimage);
	sunxi_hrc_fill_buff(hrc_drv, vbuf);

	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

out:
	mod_timer(&hrc_drv->timer, jiffies + HZ / 30);
}

/* --- TEST FUNCTION END --- */

static void sunxi_hrc_wait_buf_queue(struct work_struct *work)
{
	struct hrc_drv *hrc_drv = container_of(work, struct hrc_drv, buf_work);
	int ret;

	ret = wait_event_timeout(hrc_drv->buf_wait_queue,
				 !hrc_drv->capturing ||
				 !list_empty(&hrc_drv->buf_list),
				 msecs_to_jiffies(5000));

	if (!hrc_drv->capturing)
		return;

	if (ret && !list_empty(&hrc_drv->buf_list)) {
		ret = sunxi_hrc_set_next_buf_addr(hrc_drv);
		if (ret) {
			hrc_err("set addr error!\n");
			return;
		}

		ret = sunxi_hrc_hardware_config_ready();
		if (ret) {
			hrc_err("set config ready error!\n");
			return;
		}
	} else {
		hrc_wrn("buffer queue too slow!!!\n");
		schedule_work(&hrc_drv->buf_work);
	}
}

static int sunxi_hrc_handle_unusual_irq(struct hrc_drv *hrc_drv)
{
	int ret;

	/* unusual irq: reg setting not match input source */

	ret = sunxi_hrc_config_params(hrc_drv);
	if (ret) {
		hrc_dbg("[UNUSUAL]: reconfigure params error!\n");
		return ret;
	}

	ret = sunxi_hrc_hardware_config_ready();
	if (ret) {
		hrc_dbg("[UNUSUAL]: set config ready error!\n");
		return ret;
	}

	return 0;
}

static int sunxi_hrc_handle_timeout_irq(struct hrc_drv *hrc_drv)
{
	/* TODO */
	return 0;
}

static int sunxi_hrc_handle_overflow_irq(struct hrc_drv *hrc_drv)
{
	/* TODO */
	return 0;
}

static int sunxi_hrc_handle_vsync_irq(struct hrc_drv *hrc_drv)
{
	/* TODO */
	return 0;
}

static int sunxi_hrc_handle_cfg_finish_irq(struct hrc_drv *hrc_drv)
{
	int ret;

	if (hrc_drv->ddr_debug)
		return 0;

	if (list_empty(&hrc_drv->buf_list)) {
		hrc_drv->counter.no_buffer++;
		schedule_work(&hrc_drv->buf_work);
		return 0;
	}

	ret = sunxi_hrc_set_next_buf_addr(hrc_drv);
	if (ret) {
		hrc_err("set addr error\n");
		return -1;
	}

	ret = sunxi_hrc_hardware_config_ready();
	if (ret) {
		hrc_err("set config ready error\n");
		return -1;
	}
	return 0;
}

static int sunxi_hrc_handle_wb_finish_irq(struct hrc_drv *hrc_drv)
{
	struct hrc_buffer *buf;

	if (hrc_drv->ddr_debug) {
		cap_start = 0;
		cap_done = 1;
		return 0;
	}

	if (list_empty(&hrc_drv->buf_active_list)) {
		hrc_err("list_empty!\n");
		return -1;
	}

	buf = list_entry(hrc_drv->buf_active_list.next, struct hrc_buffer, list);
	if (buf->vb.state == VB2_BUF_STATE_ACTIVE) {
		list_del(&buf->list);

		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

		hrc_drv->fps.ms = jiffies_to_msecs(jiffies - hrc_drv->fps.jiffies);
		hrc_drv->fps.jiffies = jiffies;
	} else {
		hrc_err("buf not active!\n");
	}
	return 0;
}

static irqreturn_t sunxi_hrc_irq_handler(int irq, void *data)
{
	struct hrc_drv *hrc_drv = (struct hrc_drv *)data;
	int ret = 0;
	u32 state = 0;
	unsigned long flags = 0;

	ret = sunxi_hrc_hardware_get_irq_state(&state);
	if (ret) {
		hrc_err("get irq state error!\n");
		hrc_drv->counter.error++;
		return IRQ_NONE;
	}
	if (!state) {
		hrc_err("unknown irq trigger!\n");
		hrc_drv->counter.error++;
		return IRQ_NONE;
	}

	spin_lock_irqsave(&hrc_drv->slock, flags);

	if (state & HRC_IRQ_UNUSUAL) {
		hrc_dbg("unusual!\n");
		hrc_drv->counter.unusal++;

		if (sunxi_hrc_handle_unusual_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_TIMEOUT) {
		hrc_dbg("timeout!\n");

		if (sunxi_hrc_handle_timeout_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_OVERFLOW) {
		hrc_dbg("overflow!\n");
		hrc_drv->counter.overflow++;

		if (sunxi_hrc_handle_overflow_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_FRAME_VSYNC) {
		hrc_drv->counter.vsync++;

		if (sunxi_hrc_handle_vsync_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_CFG_FINISH) {
		hrc_dbg("cfg done!\n");
		hrc_drv->counter.cfg_finish++;

		if (sunxi_hrc_handle_cfg_finish_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_WB_FINISH) {
		hrc_dbg("wb done!\n");
		hrc_drv->counter.wb_finish++;

		if (sunxi_hrc_handle_wb_finish_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	spin_unlock_irqrestore(&hrc_drv->slock, flags);
	sunxi_hrc_hardware_clear_irq_state(state);
	return IRQ_HANDLED;
}

static int sunxi_hrc_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	strscpy(cap->driver, DRV_NAME, sizeof(cap->driver));
	strscpy(cap->card, DRV_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", DRV_NAME);

	cap->capabilities = hrc_drv->caps | V4L2_CAP_DEVICE_CAPS;
	cap->device_caps |= V4L2_CAP_VIDEO_CAPTURE;

	hrc_dbg("caps: %x dev_caps: %x\n", cap->capabilities, cap->device_caps);
	return 0;
}

static int sunxi_hrc_enum_fmt_vid_cap(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(hrc_support_fmt))
		return -EINVAL;

	strscpy(f->description, hrc_support_fmt[f->index].name, sizeof(f->description));

	f->pixelformat = hrc_support_fmt[f->index].fourcc;

	return 0;
}

static int sunxi_hrc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct hrc_drv *hrc_drv = video_drvdata(file);

	memcpy(pix, &hrc_drv->pix_fmt, sizeof(*pix));

	return 0;
}

static int sunxi_hrc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int i;
	u32 depth = 0;

	/* width and height */
	v4l_bound_align_image(&pix->width, MIN_WIDTH, MAX_WIDTH, ALIGN_WIDTH,
			      &pix->height, MIN_HEIGHT, MAX_HEIGHT, ALIGN_HEIGHT, 0);

	/* format */
	for (i = 0; i < ARRAY_SIZE(hrc_support_fmt); i++) {
		if (f->fmt.pix.pixelformat == hrc_support_fmt[i].fourcc) {
			pix->pixelformat = hrc_support_fmt[i].fourcc;
			depth = hrc_support_fmt[i].depth;
			break;
		}
	}
	if (i == ARRAY_SIZE(hrc_support_fmt)) {
		/* TODO: if not found, use input format? */
		hrc_err("Do not support format: %c%c%c%c!\n",
			(pix->pixelformat >> 0) & 0xFF, (pix->pixelformat >> 8) & 0xFF,
			(pix->pixelformat >> 16) & 0xFF, (pix->pixelformat >> 24) & 0xFF);
		pix->pixelformat = hrc_support_fmt[0].fourcc;
		depth = hrc_support_fmt[0].depth;
	}

	/* field */
	if (pix->field != V4L2_FIELD_NONE) {
		if (pix->field != V4L2_FIELD_TOP || pix->field != V4L2_FIELD_BOTTOM)
			pix->field = V4L2_FIELD_NONE;
	}

	/* bytesperline */
	switch (pix->pixelformat) {
	case V4L2_PIX_FMT_NV24:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
		pix->bytesperline = pix->width;
		break;
	default:
		pix->bytesperline = pix->width * depth / 8;
	}

	/* sizeimage */
	pix->sizeimage = f->fmt.pix.width * f->fmt.pix.height * depth / 8;

	/* ycbcr_enc */
	if (pix->ycbcr_enc != V4L2_YCBCR_ENC_601 &&
	    pix->ycbcr_enc != V4L2_YCBCR_ENC_709 &&
	    pix->ycbcr_enc != V4L2_YCBCR_ENC_BT2020 &&
	    pix->ycbcr_enc != V4L2_YCBCR_ENC_DEFAULT) {
		pix->ycbcr_enc = V4L2_YCBCR_ENC_709;
	}

	return 0;
}

static int sunxi_hrc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret;

	hrc_dbg("[s] wxh: %dx%d fmt: %c%c%c%c field: %d bpp: %d size: %d ycbcr: %d quan: %d\n",
		pix->width, pix->height,
		(pix->pixelformat >> 0) & 0xFF, (pix->pixelformat >> 8) & 0xFF,
		(pix->pixelformat >> 16) & 0xFF, (pix->pixelformat >> 24) & 0xFF,
		pix->field, pix->bytesperline, pix->sizeimage,
		pix->ycbcr_enc, pix->quantization);

	ret = sunxi_hrc_try_fmt_vid_cap(file, priv, f);
	if (ret < 0)
		return ret;

	memcpy(&hrc_drv->pix_fmt, pix, sizeof(hrc_drv->pix_fmt));

	hrc_dbg("[e] wxh: %dx%d fmt: %c%c%c%c field: %d bpp: %d size: %d ycbcr: %d quan: %d\n",
		pix->width, pix->height,
		(pix->pixelformat >> 0) & 0xFF, (pix->pixelformat >> 8) & 0xFF,
		(pix->pixelformat >> 16) & 0xFF, (pix->pixelformat >> 24) & 0xFF,
		pix->field, pix->bytesperline, pix->sizeimage,
		pix->ycbcr_enc, pix->quantization);
	return 0;
}

static int sunxi_hrc_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_reqbufs(&hrc_drv->vb2_q, p);
}

static int sunxi_hrc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_querybuf(&hrc_drv->vb2_q, p);
}

static int sunxi_hrc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_qbuf(&hrc_drv->vb2_q, hrc_drv->v4l2_dev.mdev, p);
}

static int sunxi_hrc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_dqbuf(&hrc_drv->vb2_q, p, file->f_flags & O_NONBLOCK);
}

static int sunxi_hrc_enum_input(struct file *file, void *priv, struct v4l2_input *inp)
{
	if (inp->index > INPUT_NUM - 1)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	snprintf(inp->name, sizeof(inp->name), "hrc-%d", inp->index);
	return 0;
}

static int sunxi_hrc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	*i = hrc_drv->input;
	return 0;
}

static int sunxi_hrc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	hrc_dbg("set input: %d\n", i);

	hrc_drv->input = i;
	return 0;
}

static int sunxi_hrc_g_parm(struct file *file, void *priv, struct v4l2_streamparm *parms)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	hrc_dbg("\n");

	if (parms->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		/* TODO: read from HDMI RX */
		hrc_drv->denominator = 30;
		parms->parm.capture.timeperframe.numerator = 1;
		parms->parm.capture.timeperframe.denominator = hrc_drv->denominator;
	}

	return 0;
}

static int sunxi_hrc_s_parm(struct file *file, void *priv, struct v4l2_streamparm *parms)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	hrc_dbg("\n");

	if (parms->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		/* TODO: read from HDMI RX */
		hrc_drv->denominator = 30;
		parms->parm.capture.timeperframe.numerator = 1;
		parms->parm.capture.timeperframe.denominator = hrc_drv->denominator;
	}

	return 0;
}

static int sunxi_hrc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret = 0;

	hrc_dbg("\n");

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (hrc_drv->capturing)
		return -1;

	ret = vb2_streamon(&hrc_drv->vb2_q, i);
	if (ret) {
		hrc_err("vb2_streamon error\n");
		goto STREAMON_ERR;
	}

	if (hrc_drv->v4l2_debug) {
		timer_setup(&hrc_drv->timer, sunxi_hrc_timer_handler, 0);
		hrc_drv->timer.expires = jiffies + HZ / 2;
		add_timer(&hrc_drv->timer);
	} else {
		ret = sunxi_hrc_config_params(hrc_drv);
		if (ret) {
			hrc_err("sunxi_hrc_config_params error\n");
			goto STREAMON_ERR;
		}

		if (list_empty(&hrc_drv->buf_list)) {
			hrc_err("buf_list is empty!\n");
			goto STREAMON_ERR;
		}

		ret = sunxi_hrc_set_next_buf_addr(hrc_drv);
		if (ret) {
			hrc_err("sunxi_hrc_set_next_buf_addr error\n");
			goto STREAMON_ERR;
		}

		ret = sunxi_hrc_hardware_config_ready();
		if (ret) {
			hrc_err("hrc set config ready error\n");
			goto STREAMON_ERR;
		}

		ret = sunxi_hrc_hardware_enable(HRC_IRQ_ALL);
		if (ret) {
			hrc_err("hrc enable error\n");
			goto STREAMON_ERR;
		}

		/* reset fps */
		hrc_drv->fps.jiffies = jiffies;
		hrc_drv->fps.ms = 0;
	}

	hrc_drv->capturing = 1;

STREAMON_ERR:
	return ret;
}

static int sunxi_hrc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret = 0;

	hrc_dbg("\n");

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (!hrc_drv->capturing)
		return -1;

	if (hrc_drv->v4l2_debug)
		del_timer(&hrc_drv->timer);
	else
		sunxi_hrc_hardware_disable();

	ret = vb2_streamoff(&hrc_drv->vb2_q, i);
	if (ret) {
		hrc_err("vb2_streamoff error!\n");
		goto STREAMOFF_ERR;
	}

	hrc_drv->capturing = 0;
	cancel_work_sync(&hrc_drv->buf_work);
	wake_up(&hrc_drv->buf_wait_queue);

STREAMOFF_ERR:
	return ret;
}

static int sunxi_hrc_enum_framesizes(struct file *file, void *fh, struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index > 0)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
	fsize->stepwise.min_width  = MIN_WIDTH;
	fsize->stepwise.min_height = MIN_HEIGHT;
	fsize->stepwise.max_width  = MAX_WIDTH;
	fsize->stepwise.max_height = MAX_HEIGHT;

	return 0;
}

static const struct v4l2_ioctl_ops v4l2_hrc_ioctl_ops = {
	.vidioc_querycap         = sunxi_hrc_querycap,

	.vidioc_enum_fmt_vid_cap = sunxi_hrc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap    = sunxi_hrc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap  = sunxi_hrc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap    = sunxi_hrc_s_fmt_vid_cap,

	.vidioc_reqbufs          = sunxi_hrc_reqbufs,
	.vidioc_querybuf         = sunxi_hrc_querybuf,
	.vidioc_qbuf             = sunxi_hrc_qbuf,
	.vidioc_dqbuf            = sunxi_hrc_dqbuf,

	.vidioc_enum_input       = sunxi_hrc_enum_input,
	.vidioc_g_input          = sunxi_hrc_g_input,
	.vidioc_s_input          = sunxi_hrc_s_input,
	.vidioc_g_parm           = sunxi_hrc_g_parm,
	.vidioc_s_parm           = sunxi_hrc_s_parm,

	.vidioc_streamon         = sunxi_hrc_streamon,
	.vidioc_streamoff        = sunxi_hrc_streamoff,

	.vidioc_enum_framesizes  = sunxi_hrc_enum_framesizes,
};

static int sunxi_hrc_v4l2_open(struct file *file)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	hrc_dbg("\n");

	sunxi_hrc_enable_resource(hrc_drv);

	return 0;
}

static int sunxi_hrc_v4l2_close(struct file *file)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	hrc_dbg("\n");

	sunxi_hrc_disable_resource(hrc_drv);

	if (hrc_drv->capturing) {
		if (hrc_drv->v4l2_debug)
			del_timer(&hrc_drv->timer);
		else
			sunxi_hrc_hardware_disable();
		hrc_drv->capturing = 0;
		cancel_work_sync(&hrc_drv->buf_work);
		wake_up(&hrc_drv->buf_wait_queue);
	}

	vb2_queue_release(&hrc_drv->vb2_q);

	return 0;
}

static ssize_t sunxi_hrc_v4l2_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_read(&hrc_drv->vb2_q, data, count, ppos, file->f_flags & O_NONBLOCK);
}

static unsigned int sunxi_hrc_v4l2_poll(struct file *file, struct poll_table_struct *wait)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_poll(&hrc_drv->vb2_q, file, wait);
}

static int sunxi_hrc_v4l2_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret;

	/* hrc_dbg("mmap called, vma=0x%08lx\n", (unsigned long)vma); */
	ret = vb2_mmap(&hrc_drv->vb2_q, vma);
	/* hrc_dbg("vma start=0x%08lx, size=%ld, ret=%d\n", (unsigned long)vma->vm_start, */
	/*                 (unsigned long)vma->vm_end - (unsigned long)vma->vm_start, ret); */
	return ret;
}

static const struct v4l2_file_operations v4l2_hrc_fops = {
	.owner		= THIS_MODULE,
	.open           = sunxi_hrc_v4l2_open,
	.release        = sunxi_hrc_v4l2_close,
	.read           = sunxi_hrc_v4l2_read,
	.poll		= sunxi_hrc_v4l2_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap           = sunxi_hrc_v4l2_mmap,
};

static int sunxi_hrc_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				 unsigned int *nplanes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vq);

	hrc_dbg("width: %d height: %d sizeimage: %d\n",
		hrc_drv->pix_fmt.width, hrc_drv->pix_fmt.height,
		hrc_drv->pix_fmt.sizeimage);

	sizes[0] = hrc_drv->pix_fmt.sizeimage;

	if (*nbuffers < 3)
		*nbuffers = 3;
	else if (*nbuffers > 10)
		*nbuffers = 10;

	*nplanes = 1;

	hrc_dbg("nbuffers: %d nplanes: %d\n", *nbuffers, *nplanes);

	return 0;
}

static int sunxi_hrc_buf_prepare(struct vb2_buffer *vb)
{
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size;

	size = hrc_drv->pix_fmt.sizeimage;

	if (vb2_plane_size(vb, 0) < size) {
		hrc_err("data will not fit into plane (%lu < %lu)\n",
			vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vb2_set_plane_payload(vb, 0, size);

	vb->planes[0].m.offset = vb2_dma_contig_plane_dma_addr(vb, 0);

	return 0;
}

static void sunxi_hrc_buf_queue(struct vb2_buffer *vb)
{
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vb->vb2_queue);
	struct hrc_buffer *buf = container_of(vb, struct hrc_buffer, vb);
	unsigned long flags = 0;

	hrc_dbg("\n");

	spin_lock_irqsave(&hrc_drv->slock, flags);
	list_add_tail(&buf->list, &hrc_drv->buf_list);
	spin_unlock_irqrestore(&hrc_drv->slock, flags);

	wake_up(&hrc_drv->buf_wait_queue);
}

static int sunxi_hrc_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	/* struct hrc_drv *hrc_drv = vb2_get_drv_priv(vq); */

	hrc_dbg("\n");

	return 0;
}

static void sunxi_hrc_stop_streaming(struct vb2_queue *vq)
{
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vq);
	unsigned long flags = 0;

	hrc_dbg("\n");

	/* clear buffer */
	spin_lock_irqsave(&hrc_drv->slock, flags);
	while (!list_empty(&hrc_drv->buf_list)) {
		struct hrc_buffer *buf;

		buf = list_entry(hrc_drv->buf_list.next, struct hrc_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
	while (!list_empty(&hrc_drv->buf_active_list)) {
		struct hrc_buffer *buf;

		buf = list_entry(hrc_drv->buf_active_list.next, struct hrc_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&hrc_drv->slock, flags);
}

static const struct vb2_ops vb2_hrc_ops = {
	.queue_setup     = sunxi_hrc_queue_setup,
	.buf_prepare     = sunxi_hrc_buf_prepare,
	.buf_queue       = sunxi_hrc_buf_queue,
	.start_streaming = sunxi_hrc_start_streaming,
	.stop_streaming  = sunxi_hrc_stop_streaming,
};

static ssize_t loglevel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "loglevel: %d\n", hrc_loglevel);
	return n;
}

static ssize_t loglevel_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long long value;

	if (!kstrtoull(buf, 10, &value))
		hrc_loglevel = value > 8 ? 8 : (u32)value;
	return count;
}

static DEVICE_ATTR_RW(loglevel);

static ssize_t reg_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sunxi_hrc_hardware_reg_dump(buf, 0) % PAGE_SIZE;
}

static DEVICE_ATTR_RO(reg_dump);

static u32 reg_addr;
static u32 reg_val;
static int sunxi_hrc_parse_reg_params(const char *buf)
{
	char *ptr = (char *)buf;
	char *sep = ", ";
	char *token;
	unsigned long long value;
	int i;

	for (i = 0; i < 2; i++) {
		token = strsep(&ptr, sep);
		if (token) {
			if (token[0] == '\0') {
				token = strsep(&ptr, sep);
				if (!token)
					return -1;
			}

			if (!kstrtoull(token, 0, &value)) {
				if (i == 0)
					reg_addr = (u32)value;
				else
					reg_val = (u32)value;
			} else {
				return -1;
			}

		} else {
			if (i == 1)
				reg_val = 1;
			else
				return -1;
		}
	}

	return 0;
}

static ssize_t reg_read_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i;
	u32 val;

	if (!reg_val) {
		n += sprintf(buf + n, "read value from register:\n");
		n += sprintf(buf + n, "    echo <reg_addr>,<size> > reg_read\n");
		n += sprintf(buf + n, "    echo <reg_addr> <size> > reg_read\n");
		return n;
	}

	for (i = reg_addr; i < reg_addr + (reg_val * 4); i += 4) {
		if (!(i % 16) || i == reg_addr)
			n += sprintf(buf + n, "\n0x%08x:", i);

		sunxi_hrc_hardware_reg_read(i, &val);
		n += sprintf(buf + n, " 0x%08x", val);
	}
	n += sprintf(buf + n, "\n");

	reg_addr = 0;
	reg_val = 0;

	return n;
}

static ssize_t reg_read_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (!sunxi_hrc_parse_reg_params(buf) && !(reg_addr % 4) && reg_val) {
		hrc_inf("reg_addr: 0x%x size: 0x%x\n", reg_addr, reg_val);
	} else {
		hrc_err("Reg params error! buf: %s\n", buf);
		reg_addr = 0;
		reg_val = 0;
	}

	return count;
}

static DEVICE_ATTR_RW(reg_read);

static ssize_t reg_write_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "write value to register:\n");
	n += sprintf(buf + n, "    echo <reg_addr>,<val> > reg_write\n");
	n += sprintf(buf + n, "    echo <reg_addr> <val> > reg_write\n");

	return n;
}

static ssize_t reg_write_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	if (!sunxi_hrc_parse_reg_params(buf) && !(reg_addr % 4) && reg_val) {
		hrc_inf("reg_addr: 0x%x reg_val: 0x%x\n", reg_addr, reg_val);
		sunxi_hrc_hardware_reg_write(reg_addr, reg_val);
	} else {
		hrc_err("Reg params error! buf: %s\n", buf);
	}

	reg_addr = 0;
	reg_val = 0;

	return count;
}

static DEVICE_ATTR_RW(reg_write);

static ssize_t dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	ssize_t n = 0;

	static const char * const fmt_name[]    = {"rgb", "yuv444", "yuv422", "yuv420",
						   "yuv444_1planer"};
	static const char * const depth_name[]  = {"8bits", "10bits", "12bits", "16bits"};
	static const char * const src_name[]    = {"hdmi rx", "ddr"};
	static const char * const field_mode[]  = {"frame", "field"};
	static const char * const field_order[] = {"bottom", "top"};

	n += sprintf(buf + n, "[status]: %s vsync:%llu cfg:%llu wb:%llu ",
		     hrc_drv->capturing ? "enable" : "disable",
		     hrc_drv->counter.vsync,
		     hrc_drv->counter.cfg_finish,
		     hrc_drv->counter.wb_finish);
	n += sprintf(buf + n, "overflow:%llu unusal:%llu no_buffer:%llu error:%llu\n",
		     hrc_drv->counter.overflow,
		     hrc_drv->counter.unusal,
		     hrc_drv->counter.no_buffer,
		     hrc_drv->counter.error);

	n += sprintf(buf + n, "[input] : %dx%d fps:%d fmt:%s depth:%s\n",
		     hrc_drv->in_param.size.width,
		     hrc_drv->in_param.size.height,
		     hrc_drv->denominator,
		     fmt_name[hrc_drv->in_param.format],
		     depth_name[hrc_drv->in_param.depth]);

	n += sprintf(buf + n, "[output]: %dx%d(y) %dx%d(c) ms:%ld fmt:%s stride:%dx%d\n",
		     hrc_drv->out_param.y_size.width,
		     hrc_drv->out_param.y_size.height,
		     hrc_drv->out_param.c_size.width,
		     hrc_drv->out_param.c_size.height,
		     hrc_drv->fps.ms,
		     fmt_name[hrc_drv->out_param.format],
		     hrc_drv->out_param.y_stride,
		     hrc_drv->out_param.c_stride);

	n += sprintf(buf + n, "[params]\n");
	n += sprintf(buf + n, "\tsrc    : %s\n",
		     src_name[hrc_drv->ctrl_param.data_src]);
	n += sprintf(buf + n, "\tfield  :\n");
	n += sprintf(buf + n, "\t\tmode   : %s\n",
		     field_mode[hrc_drv->ctrl_param.field_mode]);
	n += sprintf(buf + n, "\t\tinverse: %s\n",
		     hrc_drv->ctrl_param.field_inverse ? "true" : "false");
	n += sprintf(buf + n, "\t\torder  : %s\n",
		     field_order[hrc_drv->ctrl_param.field_order]);
	n += sprintf(buf + n, "\ttimeout: %d\n",
		     hrc_drv->ctrl_param.timeout_cycle);

	return n;
}

static DEVICE_ATTR_RO(dump);

static ssize_t v4l2_dbg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	ssize_t n = 0;

	n += sprintf(buf + n, "v4l2_debug: %d\n", hrc_drv->v4l2_debug);
	return n;
}

static ssize_t v4l2_dbg_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	unsigned long long value;

	if (!kstrtoull(buf, 10, &value))
		hrc_drv->v4l2_debug = !!(u8)(value);
	return count;
}

static DEVICE_ATTR_RW(v4l2_dbg);

static ssize_t params_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	ssize_t n = 0;
	struct hrc_ctrl_param *ctrl_param = &hrc_drv->ctrl_param;
	struct hrc_input_param *in_param = &hrc_drv->in_param;
	struct hrc_output_param *out_param = &hrc_drv->out_param;

	n += sprintf(buf + n, "Usage:\n");
	n += sprintf(buf + n, "ctrl_param=<data_src>\n");
	n += sprintf(buf + n, "in_param=<input_width>,<input_height>,<input_format>");
	n += sprintf(buf + n, "<input_bit_depth>,<input_csc>,<input_quan>\n");
	n += sprintf(buf + n, "out_param=<output_y_width>,<output_y_height>,");
	n += sprintf(buf + n, "<output_c_width>,<output_c_height>,");
	n += sprintf(buf + n, "<output_format>,<output_y_stride>,<output_c_stride>,");
	n += sprintf(buf + n, "<output_csc>,<output_quan>\n");

	n += sprintf(buf + n, "[ctrl]\n");
	if (hrc_drv->params_custom & CTRL_PARAMS_CUSTOM)
		n += sprintf(buf + n, "data_src: %d\n", ctrl_param->data_src);
	else
		n += sprintf(buf + n, "ctrl parameters not customized!\n");

	n += sprintf(buf + n, "[input]\n");
	if (hrc_drv->params_custom & INPUT_PARAMS_CUSTOM) {
		n += sprintf(buf + n, "width: %d\n", in_param->size.width);
		n += sprintf(buf + n, "height: %d\n", in_param->size.height);
		n += sprintf(buf + n, "format: %d\n", in_param->format);
		n += sprintf(buf + n, "depth: %d\n", in_param->depth);
		n += sprintf(buf + n, "csc: %d\n", in_param->csc);
		n += sprintf(buf + n, "quantization: %d\n", in_param->quantization);
		n += sprintf(buf + n, "dbg_buf_size: %d\n", dbg_buf_size);
	} else {
		n += sprintf(buf + n, "input parameters not customized!\n");
	}

	n += sprintf(buf + n, "[output]\n");
	if (hrc_drv->params_custom & OUTPUT_PARAMS_CUSTOM) {
		n += sprintf(buf + n, "y-width: %d\n", out_param->y_size.width);
		n += sprintf(buf + n, "y-height: %d\n", out_param->y_size.height);
		n += sprintf(buf + n, "c-width: %d\n", out_param->c_size.width);
		n += sprintf(buf + n, "c-height: %d\n", out_param->c_size.height);
		n += sprintf(buf + n, "format: %d\n", out_param->format);
		n += sprintf(buf + n, "y-stride: %d\n", out_param->y_stride);
		n += sprintf(buf + n, "c-stride: %d\n", out_param->c_stride);
		n += sprintf(buf + n, "csc: %d\n", out_param->csc);
		n += sprintf(buf + n, "quantization: %d\n", out_param->quantization);
		n += sprintf(buf + n, "dbg_out_buf_size: %d\n", dbg_out_buf_size);
	} else {
		n += sprintf(buf + n, "output parameters not customized!\n");
	}

	return n;
}

static ssize_t params_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	unsigned long long value;
	int i;
#define CTRL_PARAM_NUM	1
#define IN_PARAM_NUM	6
#define OUT_PARAM_NUM	9
	u32 ctrl_params[CTRL_PARAM_NUM] = {0};
	u32 in_params[IN_PARAM_NUM] = {0};
	u32 out_params[OUT_PARAM_NUM] = {0};
	char *token, *ptr;
	struct hrc_ctrl_param *ctrl_param = &hrc_drv->ctrl_param;
	struct hrc_input_param *in_param = &hrc_drv->in_param;
	struct hrc_output_param *out_param = &hrc_drv->out_param;

	if (!strncmp(buf, "ctrl_param=", 11)) {
		ptr = (char *)buf + 11;
		for (i = 0; i < CTRL_PARAM_NUM; i++) {
			token = strsep(&ptr, ",");
			if (token) {
				/* hrc_inf("token: %s\n", token); */
				if (!kstrtoull(token, 10, &value))
					ctrl_params[i] = value;
				else
					break;
			} else {
				break;
			}
		}
		if (i != CTRL_PARAM_NUM) {
			hrc_err("parse ctrl_param failed!\n");
			return count;
		}

		memset(ctrl_param, 0, sizeof(*ctrl_param));

		ctrl_param->data_src = ctrl_params[0];

		hrc_drv->params_custom |= CTRL_PARAMS_CUSTOM;
	} else if (!strncmp(buf, "in_param=", 9)) {
		if (!(hrc_drv->params_custom & CTRL_PARAMS_CUSTOM)) {
			hrc_err("Please setup ctrl_param first!\n");
			return count;
		}

		ptr = (char *)buf + 9;
		for (i = 0; i < IN_PARAM_NUM; i++) {
			token = strsep(&ptr, ",");
			if (token) {
				/* hrc_inf("token: %s\n", token); */
				if (!kstrtoull(token, 10, &value))
					in_params[i] = value;
				else
					break;
			} else {
				break;
			}
		}
		if (i != IN_PARAM_NUM) {
			hrc_err("parse in_param failed!\n");
			return count;
		}

		memset(in_param, 0, sizeof(*in_param));

		in_param->size.width   = in_params[0];
		in_param->size.height  = in_params[1];
		in_param->format       = in_params[2];
		in_param->depth        = in_params[3];
		in_param->csc          = in_params[4];
		in_param->quantization = in_params[5];

		if (ctrl_param->data_src == HRC_DATA_SRC_DDR) {
			dbg_buf_size = in_param->size.width * in_param->size.height * 4 * 2;

			if (!dbg_dma_buffer) {
				dbg_dma_buffer = dma_alloc_coherent(dev, PAGE_ALIGN(dbg_buf_size),
								    &dbg_dma_handle, GFP_KERNEL);
				if (!dbg_dma_buffer)
					return -ENOMEM;
			}

			in_param->ddr_addr.head[0] = dbg_dma_handle;
			in_param->ddr_addr.head[1] = 0;
			in_param->ddr_addr.body[0] = dbg_dma_handle + dbg_buf_size / 2;
			in_param->ddr_addr.body[1] = 0;
		}

		hrc_drv->params_custom |= INPUT_PARAMS_CUSTOM;
	} else if (!strncmp(buf, "out_param=", 10)) {
		ptr = (char *)buf + 10;
		for (i = 0; i < OUT_PARAM_NUM; i++) {
			token = strsep(&ptr, ",");
			if (token) {
				/* hrc_inf("token: %s\n", token); */
				if (!kstrtoull(token, 10, &value))
					out_params[i] = value;
				else
					break;
			} else {
				break;
			}
		}
		if (i != OUT_PARAM_NUM) {
			hrc_err("parse out params failed!\n");
			return count;
		}

		memset(out_param, 0, sizeof(*out_param));
		out_param->y_size.width  = out_params[0];
		out_param->y_size.height = out_params[1];
		out_param->c_size.width  = out_params[2];
		out_param->c_size.height = out_params[3];
		out_param->format        = out_params[4];
		out_param->y_stride      = out_params[5];
		out_param->c_stride      = out_params[6];
		out_param->csc           = out_params[7];
		out_param->quantization  = out_params[8];

		if (out_param->format == HRC_FMT_YUV420)
			dbg_out_buf_size = out_param->y_stride * out_param->y_size.height +
					   out_param->c_stride * out_param->c_size.height / 2;
		else if (out_param->format == HRC_FMT_YUV422 ||
			 out_param->format == HRC_FMT_YUV444)
			dbg_out_buf_size = out_param->y_stride * out_param->y_size.height +
					   out_param->c_stride * out_param->c_size.height;
		else if (out_param->format == HRC_FMT_RGB ||
			 out_param->format == HRC_FMT_YUV444_1PLANE)
			dbg_out_buf_size = out_param->y_stride * out_param->y_size.height;
		else
			return -EINVAL;

		if (!dbg_out_dma_buffer) {
			dbg_out_dma_buffer = dma_alloc_coherent(dev, PAGE_ALIGN(dbg_out_buf_size),
								&dbg_out_dma_handle, GFP_KERNEL);
			if (!dbg_out_dma_buffer)
				return -ENOMEM;
		}

		memset(dbg_out_dma_buffer, 0, dbg_out_buf_size);

		hrc_drv->params_custom |= OUTPUT_PARAMS_CUSTOM;
	} else {
		hrc_inf("clean parameters, free memory!\n");
		memset(ctrl_param, 0, sizeof(*ctrl_param));
		memset(in_param, 0, sizeof(*in_param));
		memset(out_param, 0, sizeof(*out_param));
		hrc_drv->params_custom = 0;
		hrc_drv->ddr_debug = 0;
		cap_start = 0;
		cap_done = 0;
		if (dbg_dma_buffer) {
			dma_free_coherent(dev, PAGE_ALIGN(dbg_buf_size),
					  dbg_dma_buffer, dbg_dma_handle);
			dbg_dma_buffer = NULL;
		}
		if (dbg_out_dma_buffer) {
			dma_free_coherent(dev, PAGE_ALIGN(dbg_out_buf_size),
					  dbg_out_dma_buffer, dbg_out_dma_handle);
			dbg_out_dma_buffer = NULL;
		}
	}

	return count;
}

static DEVICE_ATTR_RW(params);

static ssize_t ddr_dbg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "cap_start: %d cap_done: %d\n", cap_start, cap_done);
	return n;
}

static ssize_t ddr_dbg_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	struct hrc_addr out_addr;
	u32 offset = 0;
	struct hrc_output_param *out_param = &hrc_drv->out_param;

	if (buf[0] == '0') {
		hrc_inf("close ddr debug, free memory!\n");
		sunxi_hrc_disable_resource(hrc_drv);
		hrc_drv->params_custom = 0;
		hrc_drv->ddr_debug = 0;
		cap_start = 0;
		cap_done = 0;
		if (dbg_dma_buffer) {
			dma_free_coherent(dev, PAGE_ALIGN(dbg_buf_size),
					  dbg_dma_buffer, dbg_dma_handle);
			dbg_dma_buffer = NULL;
		}
		if (dbg_out_dma_buffer) {
			dma_free_coherent(dev, PAGE_ALIGN(dbg_out_buf_size),
					  dbg_out_dma_buffer, dbg_out_dma_handle);
			dbg_out_dma_buffer = NULL;
		}
		return count;
	}

	if (!(hrc_drv->params_custom & INPUT_PARAMS_CUSTOM) ||
	    !(hrc_drv->params_custom & OUTPUT_PARAMS_CUSTOM) ||
	    !(hrc_drv->params_custom & CTRL_PARAMS_CUSTOM)) {
		hrc_err("Enter ctrl,input,output params first!\n");
		return -1;
	}

	hrc_drv->ddr_debug = 1;

	sunxi_hrc_enable_resource(hrc_drv);

	ret = sunxi_hrc_config_params(hrc_drv);
	if (ret) {
		hrc_err("sunxi_hrc_config_params error\n");
		return ret;
	}

	offset = out_param->y_stride * out_param->y_size.height;

	memset(&out_addr, 0, sizeof(out_addr));
	out_addr.head[0] = dbg_out_dma_handle & 0xFFFFFFFF;
	out_addr.head[1] = (dbg_out_dma_handle >> 32) & 0xFF;
	out_addr.body[0] = (dbg_out_dma_handle + offset) & 0xFFFFFFFF;
	out_addr.body[1] = ((dbg_out_dma_handle + offset) >> 32) & 0xFF;

	ret = sunxi_hrc_hardware_config_addr(out_addr);
	if (ret) {
		hrc_err("hardware set addr error!\n");
		return ret;
	}

	ret = sunxi_hrc_hardware_config_ready();
	if (ret) {
		hrc_err("hrc set config ready error\n");
		return ret;
	}

	ret = sunxi_hrc_hardware_enable(HRC_IRQ_ALL);
	if (ret) {
		hrc_err("hrc enable error\n");
		return ret;
	}

	cap_start = 1;

	return count;
}

static DEVICE_ATTR_RW(ddr_dbg);

static struct attribute *hrc_attributes[] = {
	&dev_attr_loglevel.attr,
	&dev_attr_reg_dump.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_reg_write.attr,
	&dev_attr_dump.attr,
	&dev_attr_v4l2_dbg.attr,
	&dev_attr_params.attr,
	&dev_attr_ddr_dbg.attr,
	NULL,
};

static struct attribute_group hrc_attribute_group = {
	.name  = "attr",
	.attrs = hrc_attributes,
};

static int sunxi_hrc_drv_resource_request(struct platform_device *pdev)
{
	int ret = 0;
	struct hrc_drv *hrc_drv = dev_get_drvdata(&pdev->dev);

	/* init lock */
	mutex_init(&hrc_drv->mlock);
	spin_lock_init(&hrc_drv->slock);
	INIT_LIST_HEAD(&hrc_drv->buf_list);
	INIT_LIST_HEAD(&hrc_drv->buf_active_list);

	/* get dts params */
	if (device_property_read_bool(&pdev->dev, "fpga")) {
		hrc_drv->fpga = 1;
		hrc_wrn("hrc use fpga mode!!!\n");
	} else {
		hrc_drv->fpga = 0;
	}

	/* init dts clk */
	hrc_drv->clk_hrc = devm_clk_get(&pdev->dev, "clk_hrc");
	if (IS_ERR(hrc_drv->clk_hrc)) {
		hrc_err("get clk_hrc error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	hrc_drv->clk_bus_hrc = devm_clk_get(&pdev->dev, "clk_bus_hrc");
	if (IS_ERR(hrc_drv->clk_bus_hrc)) {
		hrc_err("get clk_bus_hrc error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	hrc_drv->rst_bus_hrc = devm_reset_control_get(&pdev->dev, "rst_bus_hrc");
	if (IS_ERR(hrc_drv->rst_bus_hrc)) {
		hrc_err("get rst_bus_hrc error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	hrc_drv->rst_bus_hdmi_rx = devm_reset_control_get(&pdev->dev, "rst_bus_hdmi_rx");
	if (IS_ERR(hrc_drv->rst_bus_hdmi_rx)) {
		hrc_err("get rst_bus_hdmi_rx error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	/* TODO: init dts power */

	/* init irq */
	hrc_drv->irq = platform_get_irq(pdev, 0);
	if (hrc_drv->irq < 0) {
		hrc_err("platform_get_irq error!\n");
		ret = hrc_drv->irq;
		goto ERR_IRQ_INIT;
	}

	ret = devm_request_threaded_irq(&pdev->dev, hrc_drv->irq,
					sunxi_hrc_irq_handler, NULL,
					IRQF_SHARED, "hrc-irq", hrc_drv);
	if (ret < 0) {
		hrc_err("devm_request_threaded_irq error!\n");
		goto ERR_IRQ_INIT;
	}

	/* init hardware device (register) */
	if (sunxi_hrc_hardware_init(pdev)) {
		hrc_err("hrc_device_init error!\n");
		ret = -1;
		goto ERR_DEVICE_INIT;
	}

	return 0;

ERR_IRQ_INIT:
ERR_DEVICE_INIT:
ERR_CLK_INIT:
	return ret;
}

static void sunxi_hrc_drv_resource_release(struct platform_device *pdev)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(&pdev->dev);

	sunxi_hrc_disable_resource(hrc_drv);
	sunxi_hrc_hardware_exit(pdev);
}

static int sunxi_hrc_probe(struct platform_device *pdev)
{
	int ret;
	struct hrc_drv *hrc_drv;
	struct vb2_queue *vb2_q;
	struct video_device *video_dev;

	hrc_dbg("\n");

	hrc_drv = devm_kzalloc(&pdev->dev, sizeof(*hrc_drv), GFP_KERNEL);
	if (!hrc_drv)
		return -ENOMEM;
	dev_set_drvdata(&pdev->dev, hrc_drv);
	hrc_drv->pdev = pdev;

	/* driver resource */
	ret = sunxi_hrc_drv_resource_request(pdev);
	if (ret) {
		hrc_err("sunxi_hrc_drv_resource_request error!\n");
		goto ERR_DRVIER_INIT;
	}

	/* v4l2 device */
	snprintf(hrc_drv->v4l2_dev.name, sizeof(hrc_drv->v4l2_dev.name), "%s_v4l2", DRV_NAME);

	ret = v4l2_device_register(&pdev->dev, &hrc_drv->v4l2_dev);
	if (ret < 0) {
		hrc_err("v4l2_device_register error: %d\n", ret);
		goto ERR_V4L2_DEVICE_REGISTER;
	}

	/* vb2 */
	vb2_q                  = &hrc_drv->vb2_q;
	vb2_q->type            = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vb2_q->io_modes        = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	vb2_q->drv_priv        = hrc_drv;
	vb2_q->buf_struct_size = sizeof(struct hrc_buffer);
	vb2_q->ops             = &vb2_hrc_ops;
	vb2_q->mem_ops         = &vb2_dma_contig_memops;
	vb2_q->lock            = &hrc_drv->mlock;
	vb2_q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	vb2_q->dev             = hrc_drv->v4l2_dev.dev;
	ret = vb2_queue_init(vb2_q);
	if (ret) {
		hrc_err("vb2_queue_init error: %d\n", ret);
		goto ERR_VB2_QUEUE_INIT_ERR;
	}

	/* video device */
	video_dev = video_device_alloc();
	if (!video_dev) {
		hrc_err("video_device_alloc error!\n");
		goto ERR_VIDEO_DEVICE_ALLOC;
	}
	snprintf(video_dev->name, sizeof(video_dev->name), "%s_vid", DRV_NAME);

	hrc_drv->caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;

	video_dev->fops        = &v4l2_hrc_fops;
	video_dev->ioctl_ops   = &v4l2_hrc_ioctl_ops;
	video_dev->device_caps = hrc_drv->caps;
	video_dev->release     = video_device_release_empty;
	video_dev->v4l2_dev    = &hrc_drv->v4l2_dev;
	video_dev->queue       = &hrc_drv->vb2_q;
	video_dev->lock        = &hrc_drv->mlock;

	video_set_drvdata(video_dev, hrc_drv);

	ret = video_register_device(video_dev, VFL_TYPE_VIDEO, -1);
	if (ret < 0) {
		hrc_err("video_register_device error: %d\n", ret);
		goto ERR_VIDEO_REGISTER_DEVICE;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &hrc_attribute_group);
	if (ret) {
		hrc_err("sysfs_create_group error: %d\n", ret);
		goto ERR_SYSFS_CREATE;
	}

	hrc_drv->buf_wq = create_workqueue("hrc_buf_workqueue");
	if (!hrc_drv->buf_wq) {
		hrc_err("create_workqueue error!\n");
		goto ERR_WORKQUEUE_CREATE;
	}
	INIT_WORK(&hrc_drv->buf_work, sunxi_hrc_wait_buf_queue);

	init_waitqueue_head(&hrc_drv->buf_wait_queue);

	return 0;

ERR_WORKQUEUE_CREATE:
	sysfs_remove_group(&pdev->dev.kobj, &hrc_attribute_group);
ERR_SYSFS_CREATE:
	video_unregister_device(video_dev);
ERR_VIDEO_REGISTER_DEVICE:
	video_device_release(video_dev);
ERR_VIDEO_DEVICE_ALLOC:
	vb2_queue_release(vb2_q);
ERR_VB2_QUEUE_INIT_ERR:
	v4l2_device_unregister(&hrc_drv->v4l2_dev);
ERR_V4L2_DEVICE_REGISTER:
	sunxi_hrc_drv_resource_release(pdev);
ERR_DRVIER_INIT:
	devm_kfree(&pdev->dev, hrc_drv);
	return ret;
}

static int sunxi_hrc_remove(struct platform_device *pdev)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(&pdev->dev);

	cancel_work_sync(&hrc_drv->buf_work);
	destroy_workqueue(hrc_drv->buf_wq);
	sysfs_remove_group(&pdev->dev.kobj, &hrc_attribute_group);
	video_unregister_device(hrc_drv->video_dev);
	video_device_release(hrc_drv->video_dev);
	vb2_queue_release(&hrc_drv->vb2_q);
	v4l2_device_unregister(&hrc_drv->v4l2_dev);
	sunxi_hrc_drv_resource_release(pdev);
	devm_kfree(&pdev->dev, hrc_drv);
	return 0;
}

static const struct of_device_id hrc_of_match[] = {
	{ .compatible = "allwinner,sunxi-hrc" },
	{ }
};
MODULE_DEVICE_TABLE(of, hrc_of_match);

static struct platform_driver hrc_pdrv = {
	.probe  = sunxi_hrc_probe,
	.remove = sunxi_hrc_remove,
	.driver = {
		.name = "allwinner,sunxi-hrc",
		.owner = THIS_MODULE,
		.of_match_table = hrc_of_match,
	},
};

static int sunxi_hrc_dbg_open(struct inode *inode, struct file *file)
{
	if (!dbg_dma_buffer) {
		hrc_err("Please enter parameters to params first!\n");
		return -ENOMEM;
	}

	return 0;
}

static int sunxi_hrc_dbg_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t sunxi_hrc_dbg_write(struct file *file,
				   const char __user *user,
				   size_t count, loff_t *ppos)
{
	ssize_t bytes = 0;

	if (*ppos >= dbg_buf_size)
		return -ENOSPC;

	if (count > dbg_buf_size - *ppos)
		count = dbg_buf_size - *ppos;

	if (copy_from_user(dbg_dma_buffer + *ppos, user, count))
		return -EFAULT;

	*ppos += count;
	bytes = count;

	return bytes;
}

static ssize_t sunxi_hrc_dbg_read(struct file *file,
				  char __user *user,
				  size_t count, loff_t *ppos)
{
	ssize_t bytes = 0;

	if (cap_done) {
		if (*ppos >= dbg_out_buf_size)
			return 0;
	} else {
		if (*ppos >= dbg_buf_size)
			return 0;
	}

	if (cap_done) {
		if (count > dbg_out_buf_size - *ppos)
			count = dbg_out_buf_size - *ppos;
	} else {
		if (count > dbg_buf_size - *ppos)
			count = dbg_buf_size - *ppos;
	}

	if (cap_done) {
		if (copy_to_user(user, dbg_out_dma_buffer + *ppos, count))
			return -EFAULT;
	} else {
		if (copy_to_user(user, dbg_dma_buffer + *ppos, count))
			return -EFAULT;
	}

	*ppos += count;
	bytes = count;

	return bytes;
}

static const struct file_operations hrc_dbg_fops = {
	.owner   = THIS_MODULE,
	.open    = sunxi_hrc_dbg_open,
	.release = sunxi_hrc_dbg_release,
	.write   = sunxi_hrc_dbg_write,
	.read    = sunxi_hrc_dbg_read,
};

static int __init sunxi_hrc_module_init(void)
{
	int ret;

	alloc_chrdev_region(&hrc_devid, 0, 1, "hrc");

	if (hrc_cdev) {
		cdev_del(hrc_cdev);
		hrc_cdev = NULL;
	}
	hrc_cdev = cdev_alloc();
	if (!hrc_cdev) {
		hrc_err("cdev_alloc failed!\n");
		ret = -1;
		goto ERR_CDEV_ALLOC;
	}

	cdev_init(hrc_cdev, &hrc_dbg_fops);
	if (cdev_add(hrc_cdev, hrc_devid, 1)) {
		hrc_err("cdev_add failed!\n");
		ret = -1;
		goto ERR_CDEV_ADD;
	}

	hrc_class = class_create("hrc");
	if (IS_ERR(hrc_class)) {
		hrc_err("class_create failed!\n");
		ret = -1;
		goto ERR_CLASS_CREATE;
	}

	hrc_device = device_create(hrc_class, NULL, hrc_devid, NULL, "hrc");
	if (!hrc_device) {
		hrc_err("device_create failed!\n");
		ret = -1;
		goto ERR_DEVICE_CREATE;
	}

	ret = platform_driver_register(&hrc_pdrv);
	if (ret) {
		hrc_err("platform_driver_register error!\n");
		goto ERR_PLATFORM_REG;
	}

	return 0;

ERR_PLATFORM_REG:
	device_destroy(hrc_class, hrc_devid);
ERR_DEVICE_CREATE:
	class_destroy(hrc_class);
ERR_CLASS_CREATE:
ERR_CDEV_ADD:
	cdev_del(hrc_cdev);
ERR_CDEV_ALLOC:
	return ret;
}

static void __exit sunxi_hrc_module_exit(void)
{
	platform_driver_unregister(&hrc_pdrv);
	device_destroy(hrc_class, hrc_devid);
	class_destroy(hrc_class);
	cdev_del(hrc_cdev);
}

late_initcall(sunxi_hrc_module_init);
module_exit(sunxi_hrc_module_exit);
