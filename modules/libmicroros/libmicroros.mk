UROS_DIR = $(COMPONENT_PATH)/micro_ros_src
DEBUG ?= 0

ifeq ($(COMPONENT_PATH),)
$(error COMPONENT_PATH 未设置)
endif

# 构建产物全部落在 BUILD_PATH(调用方 app 构建目录下的 libmicroros/,见 CMakeLists.txt),
# 模块目录只放各 app 共用的下载源码(micro_ros_src/src)和主机侧 ament 工具(micro_ros_dev)。
# 只有 get_package_names / distclean 不碰产物,可以不传
ifneq ($(filter-out get_package_names distclean,$(or $(MAKECMDGOALS),all)),)
ifeq ($(BUILD_PATH),)
$(error BUILD_PATH 未设置:构建产物须落在调用方 app 的构建目录)
endif
endif

ifeq ($(DEBUG), 1)
	BUILD_TYPE = Debug
else
	BUILD_TYPE = Release
endif

# LVGL 注入的 -DLV_CONF_PATH="…" 带引号,会撞坏下方 configure_toolchain 里
# sed 模板的 shell 引号配对;micro-ROS 库用不到 LVGL,按词剔除
CFLAGS_INTERNAL := $(filter-out -DLV_CONF_PATH=%,$(X_CFLAGS))
CXXFLAGS_INTERNAL := $(filter-out -DLV_CONF_PATH=%,$(X_CXXFLAGS))

CFLAGS_INTERNAL := -c -I$(ZEPHYR_BASE)/include/posix -I$(PROJECT_BINARY_DIR)/include/generated $(CFLAGS_INTERNAL)
CXXFLAGS_INTERNAL := -c -I$(ZEPHYR_BASE)/include/posix -I$(PROJECT_BINARY_DIR)/include/generated $(CXXFLAGS_INTERNAL)

# The XRCE client's multithread.h uses struct k_mutex on Zephyr without including
# the kernel header. micro_ros_src is downloaded, so the include is forced here.
ifneq ($(shell grep -s '^CONFIG_MICROROS_MULTITHREAD=y' $(PROJECT_BINARY_DIR)/.config),)
CFLAGS_INTERNAL += -include zephyr/kernel.h
CXXFLAGS_INTERNAL += -include zephyr/kernel.h
endif

# Downloaded sources with local patches stay on the commit their patches were tested on.
RMW_MICROXRCEDDS_REF = e64d79f98423c4be77ac24bcb3f14be13144fe6e
XRCE_CLIENT_REF = 83f129a80770a09aac9e823896ecbf6a0eddf0fc

.PHONY: all clean distclean get_package_names patch_sources configure_colcon_meta configure_toolchain colcon_build

all: $(BUILD_PATH)/libmicroros.a

# 只清本 app 的产物;各 app 共用的下载源码由 distclean 清
clean:
	rm -rf $(BUILD_PATH)/libmicroros.a $(BUILD_PATH)/include \
		$(BUILD_PATH)/zephyr_toolchain.cmake $(BUILD_PATH)/configured_colcon.meta \
		$(BUILD_PATH)/build $(BUILD_PATH)/install $(BUILD_PATH)/log

distclean:
	rm -rf $(COMPONENT_PATH)/micro_ros_dev $(UROS_DIR)

ZEPHYR_CONF_FILE := $(PROJECT_BINARY_DIR)/.config

get_package_names: $(UROS_DIR)/src
	@cd $(UROS_DIR)/src; \
	colcon list | awk '{print $$1}' | awk -v d=";" '{s=(NR==1?s:s d)$$0}END{print s}'

configure_colcon_meta: $(COMPONENT_PATH)/colcon.meta $(UROS_DIR)/src
	. $(COMPONENT_PATH)/utils.sh; \
	META_FILE=$(BUILD_PATH)/configured_colcon.meta; \
	cp $(COMPONENT_PATH)/colcon.meta $$META_FILE; \
	ZEPHYR_CONF_FILE=$(ZEPHYR_CONF_FILE); \
	update_meta_from_zephyr_config "CONFIG_MICROROS_NODES" "rmw_microxrcedds" "RMW_UXRCE_MAX_NODES"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_PUBLISHERS" "rmw_microxrcedds" "RMW_UXRCE_MAX_PUBLISHERS"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_SUBSCRIBERS" "rmw_microxrcedds" "RMW_UXRCE_MAX_SUBSCRIPTIONS"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_CLIENTS" "rmw_microxrcedds" "RMW_UXRCE_MAX_CLIENTS"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_SERVERS" "rmw_microxrcedds" "RMW_UXRCE_MAX_SERVICES"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_RMW_HISTORIC" "rmw_microxrcedds" "RMW_UXRCE_MAX_HISTORY"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_XRCE_DDS_HISTORIC" "rmw_microxrcedds" "RMW_UXRCE_STREAM_HISTORY"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_XRCE_DDS_MTU" "microxrcedds_client" "UCLIENT_CUSTOM_TRANSPORT_MTU"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_SERIAL=OFF"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_UDP=OFF"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_TCP=OFF"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_CUSTOM_TRANSPORT=ON"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_STREAM_FRAMING=ON"; \
	update_meta "rmw_microxrcedds" "RMW_UXRCE_TRANSPORT=custom"; \
	if grep -q '^CONFIG_MICROROS_MULTITHREAD=y' $$ZEPHYR_CONF_FILE; then \
		update_meta "microxrcedds_client" "UCLIENT_PROFILE_MULTITHREAD=ON"; \
	fi


# sysroot 指向 BUILD_PATH:模板里 find_library/find_path 是 ONLY 模式,只在 sysroot
# 内查找,colcon 的 install 目录必须在它下面。
# 编译器或 flags 变了就清空 colcon 构建:模板用的 CMAKE_C_FLAGS_INIT 只在各包首次
# configure 时进缓存,增量构建会一直沿用旧 flags(换架构则直接链接失败)
configure_toolchain: $(COMPONENT_PATH)/zephyr_toolchain.cmake.in
	cat $(COMPONENT_PATH)/zephyr_toolchain.cmake.in | \
		sed "s/@CMAKE_C_COMPILER@/$(subst /,\/,$(X_CC))/g" | \
		sed "s/@CMAKE_CXX_COMPILER@/$(subst /,\/,$(X_CXX))/g" | \
		sed "s/@CMAKE_SYSROOT@/$(subst /,\/,$(BUILD_PATH))/g" | \
		sed "s/@CFLAGS@/$(subst /,\/,$(CFLAGS_INTERNAL))/g" | \
		sed "s/@CXXFLAGS@/$(subst /,\/,$(CXXFLAGS_INTERNAL))/g" \
		> $(BUILD_PATH)/zephyr_toolchain.cmake.new; \
	if cmp -s $(BUILD_PATH)/zephyr_toolchain.cmake.new $(BUILD_PATH)/zephyr_toolchain.cmake; then \
		rm $(BUILD_PATH)/zephyr_toolchain.cmake.new; \
	else \
		mv $(BUILD_PATH)/zephyr_toolchain.cmake.new $(BUILD_PATH)/zephyr_toolchain.cmake; \
		rm -rf $(BUILD_PATH)/build $(BUILD_PATH)/install; \
	fi

$(COMPONENT_PATH)/micro_ros_dev/install:
	rm -rf $(COMPONENT_PATH)/micro_ros_dev; \
	mkdir $(COMPONENT_PATH)/micro_ros_dev; cd $(COMPONENT_PATH)/micro_ros_dev; \
	git clone -b jazzy https://github.com/ament/ament_cmake src/ament_cmake; \
	git clone -b jazzy https://github.com/ament/ament_lint src/ament_lint; \
	git clone -b jazzy https://github.com/ament/ament_package src/ament_package; \
	git clone -b jazzy https://github.com/ament/googletest src/googletest; \
	git clone -b jazzy https://github.com/ros2/ament_cmake_ros src/ament_cmake_ros; \
	git clone -b jazzy https://github.com/ament/ament_index src/ament_index; \
	touch src/ament_cmake_ros/rmw_test_fixture/COLCON_IGNORE; \
	colcon build --cmake-args -DBUILD_TESTING=OFF;

$(UROS_DIR)/src:
	@rm -rf $(UROS_DIR); \
	mkdir $(UROS_DIR); cd $(UROS_DIR); \
	git clone -b ros2 https://github.com/eProsima/micro-CDR src/micro-CDR; \
	git clone -b ros2 https://github.com/eProsima/Micro-XRCE-DDS-Client src/Micro-XRCE-DDS-Client; \
	git clone -b jazzy https://github.com/micro-ROS/rcl src/rcl; \
	git clone -b jazzy https://github.com/ros2/rclc src/rclc; \
	git clone -b jazzy https://github.com/micro-ROS/rcutils src/rcutils; \
	git clone -b jazzy https://github.com/micro-ROS/micro_ros_msgs src/micro_ros_msgs; \
	git clone -b jazzy https://github.com/micro-ROS/rmw-microxrcedds src/rmw-microxrcedds; \
	git clone -b jazzy https://github.com/micro-ROS/rosidl_typesupport src/rosidl_typesupport; \
	git clone -b jazzy https://github.com/micro-ROS/rosidl_typesupport_microxrcedds src/rosidl_typesupport_microxrcedds; \
	git clone -b jazzy https://github.com/ros2/rosidl src/rosidl; \
	git clone -b jazzy https://github.com/ros2/rosidl_dynamic_typesupport src/rosidl_dynamic_typesupport; \
	git clone -b jazzy https://github.com/ros2/rmw src/rmw; \
	git clone -b jazzy https://github.com/ros2/rcl_interfaces src/rcl_interfaces; \
	git clone -b jazzy https://github.com/ros2/rosidl_defaults src/rosidl_defaults; \
	git clone -b jazzy https://github.com/ros2/unique_identifier_msgs src/unique_identifier_msgs; \
	git clone -b jazzy https://github.com/ros2/common_interfaces src/common_interfaces; \
	git clone -b jazzy https://github.com/ros2/test_interface_files src/test_interface_files; \
	git clone -b jazzy https://github.com/ros2/rmw_implementation src/rmw_implementation; \
	git clone -b jazzy https://github.com/ros2/rcl_logging src/rcl_logging; \
	git clone -b jazzy https://github.com/ros2/ros2_tracing src/ros2_tracing; \
	git clone -b jazzy https://github.com/micro-ROS/micro_ros_utilities src/micro_ros_utilities; \
	git clone -b jazzy https://github.com/ros2/rosidl_core src/rosidl_core; \
	touch src/ros2_tracing/test_tracetools/COLCON_IGNORE; \
	touch src/ros2_tracing/lttngpy/COLCON_IGNORE; \
    touch src/rosidl/rosidl_typesupport_introspection_cpp/COLCON_IGNORE; \
    touch src/rclc/rclc_examples/COLCON_IGNORE; \
    touch src/common_interfaces/actionlib_msgs/COLCON_IGNORE; \
	touch src/common_interfaces/std_srvs/COLCON_IGNORE; \
	touch src/rcl/rcl_yaml_param_parser/COLCON_IGNORE; \
    touch src/rcl_logging/rcl_logging_spdlog/COLCON_IGNORE; \
    touch src/rcl_interfaces/test_msgs/COLCON_IGNORE;

patch_sources: $(UROS_DIR)/src
	sh $(COMPONENT_PATH)/apply_patches.sh $(UROS_DIR)/src/rmw-microxrcedds $(RMW_MICROXRCEDDS_REF) $(COMPONENT_PATH)/patches/rmw-microxrcedds
	sh $(COMPONENT_PATH)/apply_patches.sh $(UROS_DIR)/src/Micro-XRCE-DDS-Client $(XRCE_CLIENT_REF) $(COMPONENT_PATH)/patches/Micro-XRCE-DDS-Client

# 源码经 --base-paths 引用,build/install/log 落在 cwd 即 BUILD_PATH
colcon_build: configure_colcon_meta configure_toolchain $(COMPONENT_PATH)/micro_ros_dev/install $(UROS_DIR)/src patch_sources
	cd $(BUILD_PATH); \
	. $(COMPONENT_PATH)/micro_ros_dev/install/local_setup.sh; \
	colcon build \
		--base-paths $(UROS_DIR)/src \
		--merge-install \
		--packages-ignore-regex=.*_cpp \
		--metas $(BUILD_PATH)/configured_colcon.meta \
		--cmake-args \
		"--no-warn-unused-cli" \
		-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=OFF \
		-DTHIRDPARTY=ON \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTING=OFF \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_TOOLCHAIN_FILE=$(BUILD_PATH)/zephyr_toolchain.cmake \
		-DCMAKE_VERBOSE_MAKEFILE=OFF;

# 依赖伪目标 colcon_build,每次都重打包。原先依赖 install 目录的 mtime,colcon 原地
# 覆写 .a 不会更新目录 mtime,会静默沿用旧库
$(BUILD_PATH)/libmicroros.a: colcon_build
	mkdir -p $(BUILD_PATH)/repack; cd $(BUILD_PATH)/repack; \
	for file in $$(find $(BUILD_PATH)/install/lib/ -name '*.a'); do \
		folder=$$(echo $$file | sed -E "s/(.+)\/(.+).a/\2/"); \
		mkdir -p $$folder; cd $$folder; $(X_AR) x $$file; \
		for f in *; do \
			mv $$f ../$$folder-$$f; \
		done; \
		cd ..; rm -rf $$folder; \
	done ; \
	$(X_AR) rc libmicroros.a *.obj; cp libmicroros.a $(BUILD_PATH); ${X_RANLIB} $(BUILD_PATH)/libmicroros.a; \
	cd ..; rm -rf repack; \
	rm -rf $(BUILD_PATH)/include; \
	cp -R $(BUILD_PATH)/install/include $(BUILD_PATH)/include;
