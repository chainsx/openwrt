# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright (C) 2020-2021 ImmortalWrt.org

include $(TOPDIR)/rules.mk

PKG_NAME:=ykdl
PKG_VERSION:=1.8.2
PKG_RELEASE:=1

PYPI_NAME:=ykdl
PKG_HASH:=c689b8e4bf303d1582e40d5039539a1a754f7cf897bce73ec57c7e874e354b19

PKG_MAINTAINER:=Tianling Shen <cnsztl@immortalwrt.org>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE.txt

include ../../lang/python/pypi.mk
include $(INCLUDE_DIR)/package.mk
include ../../lang/python/python3-package.mk

define Package/ykdl
  SECTION:=multimedia
  CATEGORY:=Multimedia
  TITLE:=a video downloader focus on China mainland video sites
  URL:=https://github.com/SeaHOH/ykdl
  DEPENDS:= \
    +ca-certificates \
    +ffmpeg \
    +python3 \
    +python3-email \
    +python3-xml \
    +python3-codecs \
    +python3-ctypes \
    +python3-setuptools
endef

$(eval $(call Py3Package,ykdl))
$(eval $(call BuildPackage,ykdl))
$(eval $(call BuildPackage,ykdl-src))
