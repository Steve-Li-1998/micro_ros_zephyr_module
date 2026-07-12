# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/libmicroros)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../boards/atk-dnesp32s3m-devboard)

# WiFi bootstrap 是超仓库里的共享 Zephyr module(micro-ROS 与 lfc-uc 共用)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/wifi_bootstrap)
    list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/wifi_bootstrap)
endif()

# WiFi/agent 凭证放仓库根目录的 wifi_secrets.conf / microros_agent.conf
# (均已 gitignore),存在则叠加,同名配置覆盖 prj.conf 的占位值
foreach(secret wifi_secrets.conf microros_agent.conf)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../${secret})
        list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../${secret})
    endif()
endforeach()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(microros_sample_app)

target_sources(app PRIVATE src/main.c)
