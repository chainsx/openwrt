# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2023 OpenWrt.org

include $(TOPDIR)/rules.mk

LUCI_TITLE:=LuCI support for DDNS-Go
LUCI_DEPENDS:=+ddns-go
LUCI_PKGARCH:=all

PKG_MAINTAINER:=Tianling Shen <cnsztl@immortalwrt.org>

include ../../luci.mk

# call BuildPackage - OpenWrt buildroot signature
