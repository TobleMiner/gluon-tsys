include $(TOPDIR)/rules.mk

PKG_NAME:=libmeshneighbour
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_DEPENDS := libgluonutil libubus librespondd

include $(INCLUDE_DIR)/package.mk

define Package/libmeshneighbour
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=libmeshneighbour
  DEPENDS:=+libgluonutil +libubus +libubox +libjson-c +librespondd
endef

define Package/libmeshneighbour/description
	Library for discovering local mesh neighbours
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include/librespondd-0

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS)" CPPFLAGS="$(TARGET_CPPFLAGS)" $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libmeshneighbour-0
	$(CP) $(PKG_BUILD_DIR)/libmeshneighbour.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libmeshneighbour.so.0 $(1)/usr/lib/
	ln -s $(PKG_BUILD_DIR)/libmeshneighbour.so.0 $(1)/usr/lib/libmeshneighbour.so
endef

define Package/libmeshneighbour/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libmeshneighbour.so.0 $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libmeshneighbour))
