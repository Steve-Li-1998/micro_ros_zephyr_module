/*
 * relay_chain example, application "source_b".
 *
 * One timer of 500 ms publishes a counter on /chain/b.
 *
 * Serial output, one record per publish, time stamp taken before the publish:
 *   PUB,<record>,<time_us>,B,<value>
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

#define CHAIN_TOPIC  "/chain/b"
#define CHAIN_LETTER 'B'
#define NODE_NAME    "source_b"

#define PERIOD_MS 500

#define RCCHECK(fn) { \
	rcl_ret_t rc_ = fn; \
	if (rc_ != RCL_RET_OK) { \
		printk("#ERR,line=%d,rc=%d,%s\n", __LINE__, (int)rc_, rcl_get_error_string().str); \
		for (;;) { k_sleep(K_FOREVER); } \
	} \
}

static rcl_publisher_t publisher;
static std_msgs__msg__Int32 msg;
static uint32_t record;

static inline uint64_t now_us(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

static void timer_cb(rcl_timer_t *timer, int64_t last_call_time)
{
	ARG_UNUSED(timer);
	ARG_UNUSED(last_call_time);

	printk("PUB,%u,%llu,%c,%d\n", record++, now_us(), CHAIN_LETTER, (int)msg.data);
	if (rcl_publish(&publisher, &msg, NULL) != RCL_RET_OK) {
		printk("#PUBFAIL,%c,%d\n", CHAIN_LETTER, (int)msg.data);
		rcl_reset_error();
	}
	msg.data++;
}

int main(void)
{
	printk("\n#BOOT,%s,period_ms=%d,topic=%s\n", NODE_NAME, PERIOD_MS, CHAIN_TOPIC);

	wifi_bootstrap_connect();

	rmw_zenoh_pico_set_mode("client");
	rmw_zenoh_pico_set_unicast(ZENOH_ROUTER_IP, ZENOH_ROUTER_PORT, NULL, NULL);
	printk("#ROUTER,tcp/%s:%s\n", ZENOH_ROUTER_IP, ZENOH_ROUTER_PORT);

	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;
	rcl_node_t node;
	rcl_timer_t timer;
	rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();

	RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
	RCCHECK(rclc_node_init_default(&node, NODE_NAME, "", &support));
	RCCHECK(rclc_publisher_init_default(&publisher, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), CHAIN_TOPIC));
	RCCHECK(rclc_timer_init_default2(&timer, &support, RCL_MS_TO_NS(PERIOD_MS), timer_cb,
					 true));
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_timer(&executor, &timer));

	printk("#RUN\n");
	/* Returns only on an error other than a wait timeout */
	rcl_ret_t rc = rclc_executor_spin(&executor);

	printk("#ERR,spin,rc=%d\n", (int)rc);
	return 0;
}
