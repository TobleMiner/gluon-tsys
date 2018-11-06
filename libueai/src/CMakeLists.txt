cmake_minimum_required(VERSION 2.6)

project(libmau C)

set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS _GNU_SOURCE)

add_library(mau SHARED libmau.c)
set_property(TARGET mau PROPERTY COMPILE_FLAGS "-Wall -std=gnu99")
target_link_libraries(mau uci ubox ubus)
install(TARGETS mau
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/libmau.h DESTINATION include)
