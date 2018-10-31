include $(TOPDIR)/rules.mk

PKG_NAME:=autoupdater-proxy
PKG_VERSION:=2
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/autoupdater-proxy
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Cgi script for proxying updates via neighbours
  # Pretty much a hack, but we don't have a cgi meta package
  DEPENDS:=+gluon-status-page +libuclient +libuci
endef

define Package/autoupdater-proxy/install
	$(call Gluon/Build/Install,$(1))
	$(INSTALL_DIR) $(1)/lib/gluon/status-page/www/cgi-bin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/miau_proxy $(1)/lib/gluon/status-page/www/cgi-bin/fwproxy
endef

$(eval $(call BuildPackage,autoupdater-proxy))
