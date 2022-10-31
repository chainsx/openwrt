# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright (C) 2021 ImmortalWrt.org

include $(TOPDIR)/rules.mk

PKG_NAME:=rtl8192eu
PKG_RELEASE:=$(AUTORELEASE)

PKG_SOURCE_URL:=https://github.com/Mange/rtl8192eu-linux-driver.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2021-10-23
PKG_SOURCE_VERSION:=744bbe52976e51895fce2c1d4075f97a98dca2b2
PKG_MIRROR_HASH:=8624216a6dabfdfcde122b5f05da5cafae621a165c7b88c6f9cc6e451c000f58

PKG_BUILD_PARALLEL:=1

STAMP_CONFIGURED_DEPENDS := $(STAGING_DIR)/usr/include/mac80211-backport/backport/autoconf.h

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/rtl8192eu
  SUBMENU:=Wireless Drivers
  TITLE:=Realtek RTL8192EU support
  DEPENDS:=+kmod-cfg80211 +kmod-usb-core +@DRIVER_11N_SUPPORT
  FILES:=$(PKG_BUILD_DIR)/rtl8192eu.ko
  AUTOLOAD:=$(call AutoProbe,rtl8192eu)
endef

NOSTDINC_FLAGS = \
	-I$(PKG_BUILD_DIR) \
	-I$(PKG_BUILD_DIR)/include \
	-I$(STAGING_DIR)/usr/include/mac80211-backport \
	-I$(STAGING_DIR)/usr/include/mac80211-backport/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211 \
	-I$(STAGING_DIR)/usr/include/mac80211/uapi \
	-include backport/autoconf.h \
	-include backport/backport.h

EXTRA_CFLAGS:= \
	-DRTW_SINGLE_WIPHY \
	-DRTW_USE_CFG80211_STA_EVENT \
	-DCONFIG_IOCTL_CFG80211 \
	-DCONFIG_CONCURRENT_MODE \
	-DBUILD_OPENWRT

ifeq ($(CONFIG_BIG_ENDIAN),y)
EXTRA_CFLAGS += -DCONFIG_BIG_ENDIAN
else
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
endif

EXTRA_KCONFIG:= \
	CONFIG_RTL8192EU=m \
	USER_MODULE_NAME=rtl8192eu

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)" \
	NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
	USER_EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,rtl8192eu))