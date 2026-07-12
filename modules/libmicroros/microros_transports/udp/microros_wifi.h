#ifndef _MICROROS_WIFI_H_
#define _MICROROS_WIFI_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* 以 STA 模式连接 CONFIG_MICROROS_WIFI_SSID 指定的 AP,阻塞到 DHCP 拿到
 * IPv4 地址才返回,之后 UDP transport 即可使用。 */
void microros_wifi_connect(void);

#ifdef __cplusplus
}
#endif

#endif // _MICROROS_WIFI_H_
