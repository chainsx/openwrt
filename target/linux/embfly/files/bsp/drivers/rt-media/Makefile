# SPDX-License-Identifier: GPL-2.0

codec_dir = $(PWD)/bsp/drivers/ve/cedar-ve/codec/libcedarc

EXTRA_CFLAGS += -I$(PWD)/bsp/drivers/ve/cedar-ve \
				-I$(codec_dir)/include \
				-I$(codec_dir)/vencoder \
				-I$(codec_dir)/base/include \
				-I$(codec_dir)/memory/kernelMemory \
				-I$(PWD)/../vin/vin-video

obj-$(CONFIG_VIDEO_RT_MEDIA) += rt-media.o

rt-media-objs := rt_media.o
rt-media-objs += component/rt_message.o
rt-media-objs += component/rt_component.o
rt-media-objs += component/rt_venc_component.o
rt-media-objs += component/rt_vi_component.o
rt-media-objs += ../ve/cedar-ve/codec/libcedarc/vencoder/vencoder_tmp.o
#rt-media-objs += libgcc
#rt-media-objs += memory/ion_mem/ion_mem.o
#rt-media-objs += ../cedar-ve/codec/lib/libvencoder
#rt-media-objs += ../cedar-ve/codec/lib/libbase
#rt-media-objs += ../cedar-ve/codec/lib/libve
#rt-media-objs += ../cedar-ve/codec/lib/libgcc
ifeq (${CONFIG_VIDEO_RT_MEDIA},m)
rt-media-objs += ./libmath
endif


