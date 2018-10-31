include $(TOPDIR)/rules.mk

PKG_NAME:=librespondd
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/librespondd
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=librespondd
  DEPENDS:=
endef

define Package/librespondd/description
	Library for sending respondd requests
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS)" CPPFLAGS="$(TARGET_CPPFLAGS)" $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/librespondd-0
	$(CP) $(PKG_BUILD_DIR)/librespondd.h $(1)/usr/include/librespondd-0
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/librespondd.so.0 $(1)/usr/lib/
	ln -s $(PKG_BUILD_DIR)/librespondd.so.0 $(1)/usr/lib/librespondd.so
endef

define Package/librespondd/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/librespondd.so.0 $(1)/usr/lib/
endef

$(eval $(call BuildPackage,librespondd))
