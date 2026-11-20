#ifndef _WIFI_BOOTSTRAP_H_
#define _WIFI_BOOTSTRAP_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* 以 STA 模式连接 CONFIG_WIFI_BOOTSTRAP_SSID 指定的 AP,阻塞到 DHCP 拿到
 * IPv4 地址才返回。开启 CONFIG_WIFI_BOOTSTRAP_AUTO_CONNECT 时开机自动调用,
 * 应用代码无需再调。 */
void wifi_bootstrap_connect(void);

#ifdef __cplusplus
}
#endif

#endif // _WIFI_BOOTSTRAP_H_
