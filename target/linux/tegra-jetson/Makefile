# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk

BOARD := tegra-jetson
BOARDNAME := NVIDIA Tegra Jetson
FEATURES := audio boot-part display ext4 fpu gpio pci pcie rootfs-part rtc squashfs usb
SUBTARGETS := generic

KERNEL_PATCHVER := 6.1

define Target/Description
	Build firmware image for NVIDIA Jetson devices.
endef

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += e2fsprogs mkf2fs partx-utils

KERNELNAME := Image dtbs

$(eval $(call BuildTarget))
