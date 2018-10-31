include $(TOPDIR)/rules.mk

PKG_NAME:=libmeshutil
PKG_VERSION:=2
CMAKE_INSTALL:=1

PKG_LICENSE:=BSD-2-Clause

PKG_BUILD_DEPENDS := libubox libubus

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libmeshutil
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Gluon mesh utility library
  DEPENDS:=+libuci +libubox +libubus
endef

define Package/libmeshutil/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/libmeshutil.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libmeshutil))
