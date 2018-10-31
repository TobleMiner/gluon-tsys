cmake_minimum_required(VERSION 2.8.8)

project(MIAU_PROXY C)

set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS _GNU_SOURCE)

find_path(UBOX_INCLUDE_DIR NAMES libubox/uloop.h)
find_library(UBOX_LIBRARY NAMES ubox)
find_library(UCLIENT_LIBRARY NAMES uclient)

find_library(UBUS_LIBRARY NAMES ubus)

find_library(UCI_LIBRARY NAMES uci)
find_library(PLATFORMINFO_LIBRARY NAMES platforminfo)
find_library(GLUONUTIL_LIBRARY NAMES gluonutil)

add_executable(miau_proxy
	proxy.c
	util.c
	fetch.c
	mirrors.c
)
set_property(TARGET miau_proxy PROPERTY COMPILE_FLAGS "-std=gnu99 -Wall")
target_link_libraries(miau_proxy
	m
	${PLATFORMINFO_LIBRARY}
	${UCI_LIBRARY}
	${UBOX_LIBRARY}
	${UCLIENT_LIBRARY}
	${UBUS_LIBRARY}
	${GLUONUTIL_LIBRARY}
)

install(TARGETS miau_proxy RUNTIME DESTINATION sbin)
