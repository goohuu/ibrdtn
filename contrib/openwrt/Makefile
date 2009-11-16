#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 8444 2007-08-20 16:12:24Z nbd $

include $(TOPDIR)/rules.mk

PKG_NAME:=ibrdtn
PKG_RELEASE:=1
PKG_VERSION:=0.2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

PKG_BUILD_DEPENDS:=libsqlite3

include $(INCLUDE_DIR)/package.mk

define Package/ibrdtn
  SUBMENU:=
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libstdcpp +libpthread +libsqlite3
  TITLE:=DTN Deamon
endef

define Package/ibrdtn/conffiles
/etc/config/ibrdtn
endef

define Package/ibrdtn-tunnel
  $(call Package/ibrdtn)
  DEPENDS:=ibrdtn +kmod-tun
  TITLE:=IP Tunnel over Bundle Protocol
endef

define Package/ibrdtn-tools
  $(call Package/ibrdtn)
  DEPENDS:=ibrdtn 
  TITLE:=DTN Tools
endef

define Package/ibrdtn/description
 The implementation of the bundle protocol of the IBR (TU Braunschweig). This
 contains libraries and a daemon.
endef

define Package/ibrdtn-tools/description
 The IBR-DTN Tools include functionality for sending and receiving files (dtnsend/dtnrecv)
 and a tools to ping a DTN node (dtnping).
endef

define Package/ibrdtn-tunnel/description
 With the DTN tunnel it is possible to build a complete IP tunnel through a DTN.
endef

CONFIGURE_ARGS += \
	--enable-debug 
#	--enable-extended-debug

define Package/ibrdtn/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/dtnd $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/libibrdtn-*.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) files/$(PKG_NAME).config $(1)/etc/config/$(PKG_NAME)
endef

define Package/ibrdtn-tunnel/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/iptunnel $(1)/usr/sbin/dtntunnel
endef

define Package/ibrdtn-tools/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/dtnping $(1)/usr/bin/dtnping
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/dtnsend $(1)/usr/bin/dtnsend
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/dtnrecv $(1)/usr/bin/dtnrecv
endef

$(eval $(call BuildPackage,ibrdtn))
$(eval $(call BuildPackage,ibrdtn-tunnel))
$(eval $(call BuildPackage,ibrdtn-tools))
