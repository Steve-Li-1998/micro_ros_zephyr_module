#include "microros_wifi.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>

/* 连上 AP 且 DHCP 拿到 IPv4 后放行(ESP32_WIFI_STA_AUTO_DHCPV4 自动发起 DHCP) */
static K_SEM_DEFINE(ipv4_ready, 0, 1);
static struct net_mgmt_event_callback ipv4_cb;

static void on_ipv4_addr_add(struct net_mgmt_event_callback *cb,
			     uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		k_sem_give(&ipv4_ready);
	}
}

void microros_wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params params;

	params.ssid = (const uint8_t *)CONFIG_MICROROS_WIFI_SSID;
	params.ssid_length = strlen(CONFIG_MICROROS_WIFI_SSID);
	params.psk = (const uint8_t *)CONFIG_MICROROS_WIFI_PASSWORD;
	params.psk_length = strlen(CONFIG_MICROROS_WIFI_PASSWORD);
	params.security = WIFI_SECURITY_TYPE_PSK;
	params.channel = WIFI_CHANNEL_ANY;
	params.band = WIFI_FREQ_BAND_2_4_GHZ;
	params.mfp = WIFI_MFP_OPTIONAL;

	net_mgmt_init_event_callback(&ipv4_cb, on_ipv4_addr_add,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&ipv4_cb);

	while (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
			&params, sizeof(params)) != 0) {
		printk("WiFi connect request failed, retrying...\n");
		k_sleep(K_SECONDS(2));
	}
	printk("WiFi: connecting to \"%s\"...\n", CONFIG_MICROROS_WIFI_SSID);
	k_sem_take(&ipv4_ready, K_FOREVER);
	printk("WiFi: IPv4 ready\n");
}
