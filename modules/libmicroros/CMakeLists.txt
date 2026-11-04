# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MICROROS)

set(MICROROS_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
# 构建产物(colcon build/install/log、meta、工具链文件、libmicroros.a、头文件)落在
# 本 app 的构建目录;模块目录只放各 app 共用的下载源码(micro_ros_src/src)和主机侧
# ament 工具(micro_ros_dev)。上游把产物写回模块目录,前提是一份模块只服务一个 app,
# 多个 app 共用时会互相覆盖工具链、编译 flags 和实体数配置(不同架构直接链接失败)
set(MICROROS_BUILD_DIR ${CMAKE_BINARY_DIR}/libmicroros)

if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
set(submake "$(MAKE)")
else()
set(submake "make")
endif()

# micro-ROS library

zephyr_get_include_directories_for_lang_as_string(       C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(       C definitions)
zephyr_get_compile_options_for_lang_as_string(           C options)

zephyr_get_include_directories_for_lang_as_string(       CXX includes_cxx)
zephyr_get_system_include_directories_for_lang_as_string(CXX system_includes_cxx)
zephyr_get_compile_definitions_for_lang_as_string(       CXX definitions_cxx)
zephyr_get_compile_options_for_lang_as_string(           CXX options_cxx)

set(external_project_cflags
    "${includes} ${definitions} ${options} ${system_includes}"
    )

set(external_project_cxxflags
    "${includes_cxx} ${definitions_cxx} ${options_cxx} ${system_includes_cxx}"
    )

include(ExternalProject)

externalproject_add(libmicroros_project
    PREFIX     ${CMAKE_BINARY_DIR}/libmicroros-prefix
    SOURCE_DIR ${MICROROS_DIR}
    BINARY_DIR ${MICROROS_BUILD_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND
        ${submake} -f ${MICROROS_DIR}/libmicroros.mk
            X_CFLAGS=${external_project_cflags}
            X_CXXFLAGS=${external_project_cxxflags}
            X_CC=${CMAKE_C_COMPILER}
            X_AR=${CMAKE_AR}
            X_RANLIB=${CMAKE_RANLIB}
            X_CXX=${CMAKE_CXX_COMPILER}
            COMPONENT_PATH=${MICROROS_DIR}
            BUILD_PATH=${MICROROS_BUILD_DIR}
            ZEPHYR_BASE=${ZEPHYR_BASE}
            PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${MICROROS_BUILD_DIR}/libmicroros.a
    )

# build 步骤有 stamp,只在命令行(编译器/flags)变化时重跑;prj.conf 里改实体数不改
# 命令行,不显式挂依赖就会静默沿用旧库。只挂 libmicroros.mk 的 configure_colcon_meta
# 写进 colcon meta 的这几项(与之一一对应),agent IP 等其它 Kconfig 改动不必重编
set(microros_kconfig "")
foreach(opt NODES PUBLISHERS SUBSCRIBERS CLIENTS SERVERS RMW_HISTORIC XRCE_DDS_HISTORIC XRCE_DDS_MTU MULTITHREAD)
    string(APPEND microros_kconfig "CONFIG_MICROROS_${opt}=${CONFIG_MICROROS_${opt}}\n")
endforeach()
# 内容不变时不重写文件,mtime 不动就不触发重编
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/microros_kconfig.txt
    CONTENT "${microros_kconfig}" @ONLY)
ExternalProject_Add_StepDependencies(libmicroros_project build
    ${CMAKE_CURRENT_BINARY_DIR}/microros_kconfig.txt)

zephyr_link_libraries(${MICROROS_BUILD_DIR}/libmicroros.a)

zephyr_interface_library_named(microros)
add_dependencies(microros libmicroros_project)
target_include_directories(microros INTERFACE ${MICROROS_BUILD_DIR}/include)

execute_process(
    COMMAND
        make -f libmicroros.mk get_package_names
            COMPONENT_PATH=${MICROROS_DIR}
    WORKING_DIRECTORY
        ${MICROROS_DIR}
    OUTPUT_VARIABLE
        INCLUDE_ROS2_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

foreach(pkg ${INCLUDE_ROS2_PACKAGES})
    target_include_directories(microros INTERFACE ${MICROROS_BUILD_DIR}/include/${pkg})
endforeach()

# micro-ROS transport library

if(CONFIG_MICROROS_TRANSPORT_SERIAL)
    set(MICROROS_TRANSPORT_DIR ${MICROROS_DIR}/microros_transports/serial)
elseif(CONFIG_MICROROS_TRANSPORT_SERIAL_USB)
    set(MICROROS_TRANSPORT_DIR ${MICROROS_DIR}/microros_transports/serial-usb)
elseif(CONFIG_MICROROS_TRANSPORT_UDP)
    set(MICROROS_TRANSPORT_DIR ${MICROROS_DIR}/microros_transports/udp)
else()
    message(FATAL_ERROR Please set a micro-ROS transport)
endif()

zephyr_library_named(microros_transports)
zephyr_include_directories(${MICROROS_BUILD_DIR}/include)
zephyr_include_directories(${MICROROS_TRANSPORT_DIR})

# microros_transports.h 里的 microros_transports_init() 依赖 rmw_microros 头,
# 编译本库时同样需要各 package 的 include 子目录
foreach(pkg ${INCLUDE_ROS2_PACKAGES})
    zephyr_library_include_directories(${MICROROS_BUILD_DIR}/include/${pkg})
endforeach()

zephyr_library_sources(
   ${MICROROS_TRANSPORT_DIR}/microros_transports.c
)

# Same forced include as libmicroros.mk: multithread.h needs struct k_mutex
if(CONFIG_MICROROS_MULTITHREAD)
    zephyr_library_compile_options(-include zephyr/kernel.h)
endif()

add_dependencies(microros microros_transports)
add_dependencies(microros_transports libmicroros_project)

# Cleaning

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    ${MICROROS_BUILD_DIR}/include
    ${MICROROS_BUILD_DIR}/configured_colcon.meta
    ${MICROROS_BUILD_DIR}/zephyr_toolchain.cmake
    )

endif()
