# SPDX-License-Identifier: Apache-2.0
#
# direction example, application "judge": a micro-ROS application on
# rmw_zenoh_pico. Everything it builds lies in this directory.

cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/libmicroros
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/wifi_bootstrap
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenoh-pico
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/rmw_zenoh_pico)

# Wi-Fi credentials: wifi.conf, a Kconfig fragment with the
# CONFIG_WIFI_BOOTSTRAP_* options. It is not in Git; wifi.conf.example is.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wifi.conf)
  list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/wifi.conf)
else()
  message(WARNING "wifi.conf is missing: copy wifi.conf.example and fill it in")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zenoh_direction_judge)

set(ZENOH_ROUTER_IP "192.168.50.164" CACHE STRING "Address of the Zenoh router")
set(ZENOH_ROUTER_PORT "8889" CACHE STRING "TCP port of the Zenoh router")
message(STATUS "Zenoh router: tcp/${ZENOH_ROUTER_IP}:${ZENOH_ROUTER_PORT}")
target_compile_definitions(app PRIVATE
  ZENOH_ROUTER_IP="${ZENOH_ROUTER_IP}"
  ZENOH_ROUTER_PORT="${ZENOH_ROUTER_PORT}")

target_sources(app PRIVATE src/main.c src/wifi_ps.c)
target_link_libraries(app PRIVATE rmw_zenoh_pico microros)
