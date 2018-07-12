include $(TOPDIR)/rules.mk

RSTRIP:=true

PKG_NAME:=libgluonutil-test
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_DEPENDS := libgluonutil libubus libubox

include $(INCLUDE_DIR)/package.mk

define Package/libgluonutil-test
  SECTION:=gluon
  CATEGORY:=Gluon
  TITLE:=libgluonutil-test
  DEPENDS:=+libubus +libgluonutil +libubox
endef

define Package/libgluonutil-test/description
	Local testing package
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include/libgluonutil-0 -ggdb

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS)" CPPFLAGS="$(TARGET_CPPFLAGS)" BUILD_DIR="$(BUILD_DIR)" $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

define Package/libgluonutil-test/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libgluonutil-test $(1)/usr/bin/
endef

$(eval $(call BuildPackage,libgluonutil-test))
