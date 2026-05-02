# SPDX-License-Identifier: GPL-2.0
ccflags-y := -DDYNAMIC_DEBUG_MODULE

obj-$(CONFIG_AW_SPI_NG) += spi-sunxi-ng.o
spi-sunxi-ng-objs := spi-sunxi.o dbi/spi-sunxi-dbi.o bit/spi-sunxi-bit.o camera/spi-sunxi-camera.o calibrate/spi-sunxi-calibrate.o

obj-$(CONFIG_AW_SPI_SLAVETEST) += spi-sunxi-slave-test.o
obj-$(CONFIG_AW_SPI_CAMERATEST) += spi-sunxi-camera-test.o
