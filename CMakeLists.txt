# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/libmicroros)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../boards/atk-dnesp32s3m-devboard)

# WiFi/agent 凭证放仓库根目录的 wifi_secrets.conf(已 gitignore),存在则叠加,
# 同名配置覆盖 prj.conf 的占位值
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../wifi_secrets.conf)
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../wifi_secrets.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(microros_sample_app)

target_sources(app PRIVATE src/main.c)
