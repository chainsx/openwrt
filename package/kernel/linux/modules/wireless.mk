#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

WIRELESS_MENU:=Wireless Drivers

define KernelPackage/net-prism54
  SUBMENU:=$(WIRELESS_MENU)
  TITLE:=Intersil Prism54 support
  DEPENDS:=@PCI_SUPPORT +@DRIVER_WEXT_SUPPORT +prism54-firmware
  KCONFIG:=CONFIG_PRISM54
  FILES:= \
	$(LINUX_DIR)/drivers/net/wireless/intersil/prism54/prism54.ko
  AUTOLOAD:=$(call AutoProbe,prism54)
endef

define KernelPackage/net-prism54/description
 Kernel modules for Intersil Prism54 support
endef

$(eval $(call KernelPackage,net-prism54))

define KernelPackage/aic8800-wlan
  SUBMENU:=$(WIRELESS_MENU)
  TITLE:=AIC8800 SDIO wireless module
  KCONFIG:=CONFIG_AIC8800_WLAN_SUPPORT
  FILES:= \
         $(LINUX_DIR)/bsp/drivers/net/wireless/aic8800/aic8800_fdrv/aic8800_fdrv.ko \
         $(LINUX_DIR)/bsp/drivers/net/wireless/aic8800/aic8800_bsp/aic8800_bsp.ko
  AUTOLOAD:=$(call AutoProbe,aic8800_fdrv aic8800_bsp)
endef

define KernelPackage/aic8800-wlan/description
  Kernel module for AIC8800 SDIO wireless module
endef

$(eval $(call KernelPackage,aic8800-wlan))

define KernelPackage/aic8800-bt
  SUBMENU:=$(WIRELESS_MENU)
  TITLE:=AIC8800 SDIO bluetooth module
  KCONFIG:=CONFIG_AIC8800_BTLPM_SUPPORT
  FILES:=$(LINUX_DIR)/bsp/drivers/net/wireless/aic8800/aic8800_btlpm/aic8800_btlpm.ko
  AUTOLOAD:=$(call AutoProbe,aic8800_btlpm)
endef

define KernelPackage/aic8800-bt/description
  Kernel module for AIC8800 SDIO bluetooth module
endef

$(eval $(call KernelPackage,aic8800-bt))
