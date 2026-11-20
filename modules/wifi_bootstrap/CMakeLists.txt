# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_BOOTSTRAP)

zephyr_library()
zephyr_library_sources(wifi_bootstrap.c)
# 头文件平铺在模块根目录(仓库 .gitignore 全局排除了 **/include)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

endif()
