# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/libmicroros)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../boards/atk-dnesp32s3m-devboard)

# WiFi bootstrap 是超仓库里的共享 Zephyr module(micro-ROS 与 lfc-uc 共用)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/wifi_bootstrap)
    list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/wifi_bootstrap)
endif()

# WiFi/agent 凭证等 CONFIG_* 直接取自超仓库根目录 ulf-uros.conf(单一来源,已
# gitignore),经共享宏按前缀提取到构建目录再叠加,同名配置覆盖 prj.conf 的占位值
# (OPTIONAL + COMMAND 判断:本模块单独 checkout 时无超仓库,退回 Kconfig 默认值)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ulf_uros_conf.cmake OPTIONAL)
if(COMMAND ulf_uros_extra_conf)
    ulf_uros_extra_conf(CONFIG_WIFI_BOOTSTRAP_ CONFIG_MICROROS_)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(microros_sample_app)

target_sources(app PRIVATE src/main.c)
