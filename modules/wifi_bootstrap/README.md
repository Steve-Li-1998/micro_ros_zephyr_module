# wifi_bootstrap

与协议栈无关的 WiFi STA 联网 Zephyr module:`wifi_bootstrap_connect()` 连接
`CONFIG_WIFI_BOOTSTRAP_SSID` 指定的 AP(WPA2-PSK,2.4GHz),阻塞到 DHCP 拿到
IPv4 才返回。`uros_ws` 的 micro-ROS app 和 `lingua_franca_ws` 的 ulfc(reactor-uc)
联邦节点共用这一份代码。

WiFi 凭证统一在仓库根目录 `ulf-uros.conf` 维护(已 gitignore,模板见
`ulf-uros.conf.example`),由共享宏 `cmake/ulf_uros_conf.cmake` 在
configure 时按前缀提取叠加,无中间文件。

## 接入方式(任意 Zephyr app)

CMakeLists.txt 在 `find_package(Zephyr ...)` **之前**:

```cmake
list(APPEND ZEPHYR_EXTRA_MODULES /workspaces/modules/wifi_bootstrap)
include(/workspaces/cmake/ulf_uros_conf.cmake)
ulf_uros_extra_conf(CONFIG_WIFI_BOOTSTRAP_)   # micro-ROS app 再加 CONFIG_MICROROS_
```

prj.conf(除本模块开关外,还需常规的 WiFi/网络栈配置,ESP32 参考
`uros_ws/lcd_int32_subscriber/prj.conf`):

```
CONFIG_WIFI=y
CONFIG_NET_MGMT=y
CONFIG_WIFI_BOOTSTRAP=y
```

代码里在需要网络前调用:

```c
#include <wifi_bootstrap.h>
wifi_bootstrap_connect();
```

## ulfc 联邦节点(改不了生成代码入口)用自动联网

ulfc 首次生成的 federate 工程(CMakeLists.txt/prj.conf)不会被后续
`run_lfc.sh` 覆盖,可以直接修改。除上述接入外,再打开:

```
CONFIG_WIFI_BOOTSTRAP_AUTO_CONNECT=y
```

联网动作发生在 APPLICATION 级 `SYS_INIT`,`main()`(reactor-uc 运行时)启动前
网络已就绪,因此无需改动任何生成的 C 代码;同时删掉模板里 `native_sim` 的
`CONFIG_NET_CONFIG_*` 静态 IP 配置即可。
