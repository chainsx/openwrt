include $(TOPDIR)/rules.mk

PKG_NAME:=microsocks
PKG_VERSION:=1.0.3
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/rofl0r/microsocks/tar.gz/v$(PKG_VERSION)?
PKG_HASH:=6801559b6f8e17240ed8eef17a36eea8643412b5a7476980fd4e24b02a021b82

PKG_MAINTAINER:=Mateusz Korniak <matkorgithubcom@ant.gliwice.pl>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=COPYING

include $(INCLUDE_DIR)/package.mk

define Package/microsocks
  SECTION:=net
  SUBMENU:=Web Servers/Proxies
  CATEGORY:=Network
  TITLE:=SOCKS5 TCP/IP only proxy
  DEPENDS:=+libpthread
endef

define Package/microsocks/description
  Low resource SOCKS5 proxy.
  Supports only SOCKS5 protocol and forwarding only TCP/IP connections.
endef

define Package/microsocks/conffiles
/etc/config/microsocks
endef

TARGET_CFLAGS+= -flto
TARGET_LDFLAGS+= -flto -Wl,--gc-sections,--as-needed

define Package/microsocks/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/microsocks $(1)/usr/bin

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/microsocks.config $(1)/etc/config/microsocks
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/microsocks.init $(1)/etc/init.d/microsocks
endef

$(eval $(call BuildPackage,microsocks))
