# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright (C) 2021 OpenWrt.org

include $(TOPDIR)/rules.mk

PKG_NAME:=transfer
PKG_VERSION:=0.4.17
PKG_RELEASE:=$(AUTORELEASE)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/Mikubill/transfer/tar.gz/v$(PKG_VERSION)?
PKG_HASH:=7ba0f7de5107b4dcc977e9a6887f223ef7731cc81cbc33fea0344b1b3a065c58

PKG_MAINTAINER:=Tianling Shen <cnsztl@immortalwrt.org>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=golang/host
PKG_BUILD_PARALLEL:=1
PKG_USE_MIPS16:=0

GO_PKG:=github.com/Mikubill/transfer

include $(INCLUDE_DIR)/package.mk
include ../../lang/golang/golang-package.mk

define Package/transfer
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=File Transfer
  TITLE:=Large file transfer tool with multiple file transfer services support
  URL:=https://github.com/Mikubill/transfer
  DEPENDS:=$(GO_ARCH_DEPENDS) +ca-bundle
endef

$(eval $(call GoBinPackage,transfer))
$(eval $(call BuildPackage,transfer))
