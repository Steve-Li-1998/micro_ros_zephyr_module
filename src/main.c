#include <version.h>

#if ZEPHYR_VERSION_CODE >= ZEPHYR_VERSION(3,1,0)
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/posix/time.h>
#else
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <posix/time.h>
#endif

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <std_msgs/msg/int32.h>

#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <rmw_microros/rmw_microros.h>
#include <microros_transports.h>

#if defined(CONFIG_MICROROS_TRANSPORT_UDP)
#include <string.h>
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

static void wifi_connect_blocking(void)
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
#endif /* CONFIG_MICROROS_TRANSPORT_UDP */

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc);for(;;){};}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;

void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
	RCLC_UNUSED(last_call_time);
	if (timer != NULL) {
		RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
		msg.data++;
	}
}

int main(void)
{
#if defined(CONFIG_MICROROS_TRANSPORT_UDP)
	wifi_connect_blocking();
#endif

	rmw_uros_set_custom_transport(
		MICRO_ROS_FRAMING_REQUIRED,
		(void *) &default_params,
		zephyr_transport_open,
		zephyr_transport_close,
		zephyr_transport_write,
		zephyr_transport_read
	);

	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;

	// create init_options
	RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

	// create node
	rcl_node_t node;
	RCCHECK(rclc_node_init_default(&node, "zephyr_int32_publisher", "", &support));

	// create publisher
	RCCHECK(rclc_publisher_init_default(
		&publisher,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"zephyr_int32_publisher"));

	// create timer,
	rcl_timer_t timer;
	const unsigned int timer_timeout = 1000;
	RCCHECK(rclc_timer_init_default(
		&timer,
		&support,
		RCL_MS_TO_NS(timer_timeout),
		timer_callback));

	// create executor
	rclc_executor_t executor;
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_timer(&executor, &timer));

	msg.data = 0;

	while(1){
		rclc_executor_spin_some(&executor, 100);
		usleep(100000);
	}

	// free resources
	RCCHECK(rcl_publisher_fini(&publisher, &node))
	RCCHECK(rcl_node_fini(&node))
  return 0;    
}
