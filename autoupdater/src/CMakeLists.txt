cmake_minimum_required(VERSION 2.8.8)

project(AUTOUPDATER C)

set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS _GNU_SOURCE)

find_path(UBOX_INCLUDE_DIR NAMES libubox/uloop.h)
find_library(UBOX_LIBRARY NAMES ubox)
find_library(UCLIENT_LIBRARY NAMES uclient)

find_library(UBUS_LIBRARY NAMES ubus)

find_library(UCI_LIBRARY NAMES uci)
find_library(PLATFORMINFO_LIBRARY NAMES platforminfo)
find_library(GLUONUTIL_LIBRARY NAMES gluonutil)
find_library(MESHNEIGHBOUR_LIBRARY NAMES meshneighbour)

find_path(RESPONDD_INCLUDE_DIR NAMES librespondd-0/librespondd.h)
find_library(RESPONDD_LIBRARY NAMES respondd)

find_path(JSONC_INCLUDE_DIR NAMES json-c/json.h)
find_library(JSONC_LIBRARY NAMES json-c)

find_package(PkgConfig REQUIRED QUIET)
pkg_check_modules(ECDSAUTIL REQUIRED ecdsautil)

include_directories(${UBOX_INCLUDE_DIR} ${ECDSAUTIL_INCLUDE_DIRS} ${RESPONDD_INCLUDE_DIR} ${JSONC_INCLUDE_DIR})

add_executable(autoupdater
  autoupdater.c
  hexutil.c
  manifest.c
  settings.c
  uclient.c
  util.c
  version.c
)
set_property(TARGET autoupdater PROPERTY COMPILE_FLAGS "-std=gnu99 -Wall")
#set_property(TARGET autoupdater PROPERTY LINK_FLAGS "")
target_link_libraries(autoupdater
    m
    ${PLATFORMINFO_LIBRARY}
    ${UCI_LIBRARY}
    ${UBOX_LIBRARY}
    ${UCLIENT_LIBRARY}
    ${UBUS_LIBRARY}
    ${GLUONUTIL_LIBRARY}
    ${MESHNEIGHBOUR_LIBRARY}
    ${RESPONDD_LIBRARY}
    ${JSONC_LIBRARY}
    ${ECDSAUTIL_LIBRARIES}
)

install(TARGETS autoupdater RUNTIME DESTINATION sbin)
