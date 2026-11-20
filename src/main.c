/*
 * relay_chain example, application "relay".
 *
 * Subscribes to /chain/a and /chain/b. For every sample of either topic it
 * publishes one sample on /chain/relayed, from inside the subscription
 * callback. The relayed value names the sample it answers: 2 * value for a
 * sample of /chain/a, 2 * value + 1 for a sample of /chain/b.
 *
 * Serial output, time stamps taken at the start of the callback:
 *   SUB,<record>,<time_us>,<A|B>,<value>       sample given to the callback
 *   PUB,<record>,<time_us>,R,<relayed value>   sample published in answer
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_zenoh_pico/rmw_zenoh_pico_options.h>
#include <std_msgs/msg/int32.h>

#include <wifi_bootstrap.h>

#define RCCHECK(fn) { \
	rcl_ret_t rc_ = fn; \
	if (rc_ != RCL_RET_OK) { \
		printk("#ERR,line=%d,rc=%d,%s\n", __LINE__, (int)rc_, rcl_get_error_string().str); \
		for (;;) { k_sleep(K_FOREVER); } \
	} \
}

static rcl_subscription_t sub_a, sub_b;
static rcl_publisher_t publisher;
static std_msgs__msg__Int32 msg_a, msg_b, msg_relayed;
static uint32_t sub_record, pub_record;

static inline uint64_t now_us(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

/* Both subscriptions share one callback; the message pointer tells them apart */
static void sub_cb(const void *msgin)
{
	uint64_t t = now_us();
	bool is_a = msgin == &msg_a;
	int32_t value = is_a ? msg_a.data : msg_b.data;

	printk("SUB,%u,%llu,%c,%d\n", sub_record++, t, is_a ? 'A' : 'B', (int)value);

	msg_relayed.data = 2 * value + (is_a ? 0 : 1);
	printk("PUB,%u,%llu,R,%d\n", pub_record++, t, (int)msg_relayed.data);
	if (rcl_publish(&publisher, &msg_relayed, NULL) != RCL_RET_OK) {
		printk("#PUBFAIL,R,%d\n", (int)msg_relayed.data);
		rcl_reset_error();
	}
}

int main(void)
{
	printk("\n#BOOT,relay\n");

	wifi_bootstrap_connect();

	rmw_zenoh_pico_set_mode("client");
	rmw_zenoh_pico_set_unicast(ZENOH_ROUTER_IP, ZENOH_ROUTER_PORT, NULL, NULL);
	printk("#ROUTER,tcp/%s:%s\n", ZENOH_ROUTER_IP, ZENOH_ROUTER_PORT);

	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;
	rcl_node_t node;
	rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();

	RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
	RCCHECK(rclc_node_init_default(&node, "relay", "", &support));
	RCCHECK(rclc_publisher_init_default(&publisher, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "/chain/relayed"));
	RCCHECK(rclc_subscription_init_default(&sub_a, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "/chain/a"));
	RCCHECK(rclc_subscription_init_default(&sub_b, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "/chain/b"));
	RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
	RCCHECK(rclc_executor_add_subscription(&executor, &sub_a, &msg_a, &sub_cb, ON_NEW_DATA));
	RCCHECK(rclc_executor_add_subscription(&executor, &sub_b, &msg_b, &sub_cb, ON_NEW_DATA));

	printk("#RUN\n");
	/* Returns only on an error other than a wait timeout */
	rcl_ret_t rc = rclc_executor_spin(&executor);

	printk("#ERR,spin,rc=%d\n", (int)rc);
	return 0;
}
