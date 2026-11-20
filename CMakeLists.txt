# SPDX-License-Identifier: Apache-2.0
#
# relay_chain example, application "relay": a micro-ROS application on
# rmw_zenoh_pico. Everything it builds lies in this directory.

cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/libmicroros
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/wifi_bootstrap
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenoh-pico
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/rmw_zenoh_pico)

# Wi-Fi credentials are not in Git. Inside the superproject they are the
# CONFIG_WIFI_BOOTSTRAP_* lines of its ulf-uros.conf; elsewhere, pass them
# with -DEXTRA_CONF_FILE=<file>.
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ulf_uros_conf.cmake OPTIONAL)
if(COMMAND ulf_uros_extra_conf)
  ulf_uros_extra_conf(CONFIG_WIFI_BOOTSTRAP_)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zenoh_relay_chain_relay)

set(ZENOH_ROUTER_IP "192.168.50.164" CACHE STRING "Address of the Zenoh router")
set(ZENOH_ROUTER_PORT "8889" CACHE STRING "TCP port of the Zenoh router")
message(STATUS "Zenoh router: tcp/${ZENOH_ROUTER_IP}:${ZENOH_ROUTER_PORT}")
target_compile_definitions(app PRIVATE
  ZENOH_ROUTER_IP="${ZENOH_ROUTER_IP}"
  ZENOH_ROUTER_PORT="${ZENOH_ROUTER_PORT}")

target_sources(app PRIVATE src/main.c src/wifi_ps.c)
target_link_libraries(app PRIVATE rmw_zenoh_pico microros)
