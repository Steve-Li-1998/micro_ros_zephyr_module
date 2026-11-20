#include "wifi_bootstrap.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>

/* 连上 AP 且 DHCP 拿到 IPv4 后放行(ESP32_WIFI_STA_AUTO_DHCPV4 自动发起 DHCP) */
static K_SEM_DEFINE(ipv4_ready, 0, 1);
static struct net_mgmt_event_callback ipv4_cb;
static struct net_mgmt_event_callback wifi_cb;

static void on_ipv4_addr_add(struct net_mgmt_event_callback *cb,
			     uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		k_sem_give(&ipv4_ready);
	}
}

/* 关联结果可见化:status 0=成功,非 0 见 enum wifi_conn_status
 * (如 2=WRONG_PASSWORD,3=AP_NOT_FOUND,超时/失败等) */
static void on_wifi_event(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;
	int code = status ? status->status : -1;

	if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
		printk("WiFi: connect result: %d%s\n", code,
		       code == 0 ? " (associated, waiting DHCP...)" : " (FAILED)");
	} else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		printk("WiFi: disconnected (reason %d)\n", code);
	}
}

void wifi_bootstrap_connect(void)
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params params;
	int ret;

	if (iface == NULL) {
		printk("WiFi: no default net interface, cannot connect\n");
		return;
	}

	params.ssid = (const uint8_t *)CONFIG_WIFI_BOOTSTRAP_SSID;
	params.ssid_length = strlen(CONFIG_WIFI_BOOTSTRAP_SSID);
	params.psk = (const uint8_t *)CONFIG_WIFI_BOOTSTRAP_PASSWORD;
	params.psk_length = strlen(CONFIG_WIFI_BOOTSTRAP_PASSWORD);
	params.security = WIFI_SECURITY_TYPE_PSK;
	params.channel = WIFI_CHANNEL_ANY;
	params.band = WIFI_FREQ_BAND_2_4_GHZ;
	params.mfp = WIFI_MFP_OPTIONAL;

	net_mgmt_init_event_callback(&ipv4_cb, on_ipv4_addr_add,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&ipv4_cb);
	net_mgmt_init_event_callback(&wifi_cb, on_wifi_event,
				     NET_EVENT_WIFI_CONNECT_RESULT |
				     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);

	for (;;) {
		ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
			       &params, sizeof(params));
		if (ret != 0) {
			/* 常见错误码:-115 EINPROGRESS/-16 EBUSY(上次请求还在进行);
			 * -100 ENETDOWN(接口 admin down,多半是开机 esp_wifi_start
			 * 失败,主动 net_if_up 拉起);-22 EINVAL(参数校验被拒) */
			printk("WiFi connect request failed (err %d), retrying...\n", ret);
			if (ret == -ENETDOWN) {
				ret = net_if_up(iface);
				printk("WiFi: net_if_up -> %d\n", ret);
			}
			k_sleep(K_SECONDS(2));
			continue;
		}
		printk("WiFi: connecting to \"%s\"...\n", CONFIG_WIFI_BOOTSTRAP_SSID);
		/* 关联失败不会有任何事件放行信号量,靠超时重发连接请求 */
		if (k_sem_take(&ipv4_ready, K_SECONDS(20)) == 0) {
			break;
		}
		printk("WiFi: no IPv4 after 20s, reconnecting...\n");
	}

	/* 把拿到的地址打出来:LF 联邦的对端地址是代码生成期静态固化的(无发现机制),
	 * 做 server 的那块板必须把这里打印的地址填回 .lf,所以不能只报"ready"。 */
	char buf[NET_IPV4_ADDR_LEN] = "?";

	if (iface != NULL) {
		struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;

		for (int i = 0; ipv4 != NULL && i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (ipv4->unicast[i].ipv4.is_used &&
			    ipv4->unicast[i].ipv4.addr_type == NET_ADDR_DHCP) {
				net_addr_ntop(AF_INET,
					      &ipv4->unicast[i].ipv4.address.in_addr,
					      buf, sizeof(buf));
				break;
			}
		}
	}
	printk("WiFi: IPv4 ready, addr=%s\n", buf);
}

#ifdef CONFIG_WIFI_BOOTSTRAP_AUTO_CONNECT
static int wifi_bootstrap_auto_connect(void)
{
	wifi_bootstrap_connect();
	return 0;
}
SYS_INIT(wifi_bootstrap_auto_connect, APPLICATION, 99);
#endif
