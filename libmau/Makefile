include $(TOPDIR)/rules.mk

PKG_NAME:=libmau
PKG_VERSION:=1
CMAKE_INSTALL:=1

PKG_LICENSE:=BSD-2-Clause

PKG_BUILD_DEPENDS := libubox libubus

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libmau
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Gluon modular auto updater library
  DEPENDS:=+libuci +libubox +libubus
endef

define Package/libmau/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/libmau.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libmau))
