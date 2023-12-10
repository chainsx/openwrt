# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Build/boot-common
	rm -fR $@.boot
	mkdir -p $@.boot

	$(CP) $(IMAGE_KERNEL) $@.boot/kernel.img
	$(CP) $(DTS_DIR)/$(DEVICE_DTS).dtb $@.boot/tegra.dtb
	mkdir -p $@.boot/extlinux
	$(CP) extlinux.conf $@.boot/extlinux
	$(CP) initrd $@.boot
endef

define Build/tegra-sdcard
	PADDING=1 $(SCRIPT_DIR)/gen_image_generic.sh \
		$@ \
		$(CONFIG_TARGET_KERNEL_PARTSIZE) $@.boot \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) $(IMAGE_ROOTFS) \
		2048

endef

DEVICE_VARS += DEVICE_DTS

define Device/Default
  PROFILES := Default
  KERNEL := kernel-bin
  IMAGES := sysupgrade.img.gz
  DEVICE_DTS = nvidia/$$(SOC)-$(lastword $(subst _, ,$(1)))
endef

include $(SUBTARGET).mk

$(eval $(call BuildImage))
