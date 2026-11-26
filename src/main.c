/*
 * direction example, application "judge".
 *
 * Two sensors, which an object passes one after the other, each publish the
 * number of every arrival: sensor_a on /direction/a, sensor_b on
 * /direction/b. This application takes the direction of the movement from
 * the order in which the two messages of one pass get here: the sensor whose
 * message comes first is the one the object passed first.
 *
 * A message waits for one of the other sensor. If that comes within
 * PASS_WINDOW_MS, the two are one pass and its direction is printed. If
 * another message of the same sensor comes first, or none in time, the
 * waiting one is given up as a pass that only one sensor saw.
 *
 * Serial output, time stamp taken at the start of the callback:
 *   SUB,<record>,<time_us>,<A|B>,<count>
 *   DIR,<pass>,<AB|BA>,<gap_us>,<count_a>,<count_b>    a pass, first sensor first
 *   LONE,<A|B>,<count>                                 an arrival without the other
 * <gap_us> is the time between the two callbacks of the pass.
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

#define PASS_WINDOW_MS 1000

#define RCCHECK(fn) { \
	rcl_ret_t rc_ = fn; \
	if (rc_ != RCL_RET_OK) { \
		printk("#ERR,line=%d,rc=%d,%s\n", __LINE__, (int)rc_, rcl_get_error_string().str); \
		for (;;) { k_sleep(K_FOREVER); } \
	} \
}

static rcl_subscription_t subscription_a, subscription_b;
static std_msgs__msg__Int32 msg_a, msg_b;
static uint32_t record;
static uint32_t pass;

/* The arrival that waits for the other sensor: 0 for none, else 'A' or 'B'. */
static char waiting;
static uint64_t waiting_since_us;
static int32_t waiting_count;

static inline uint64_t now_us(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

static void arrival(char sensor, int32_t count)
{
	uint64_t t = now_us();

	printk("SUB,%u,%llu,%c,%d\n", record++, t, sensor, (int)count);

	if (waiting != 0 && t - waiting_since_us > PASS_WINDOW_MS * 1000ULL) {
		printk("LONE,%c,%d\n", waiting, (int)waiting_count);
		waiting = 0;
	}
	if (waiting != 0 && waiting != sensor) {
		int32_t count_a = sensor == 'A' ? count : waiting_count;
		int32_t count_b = sensor == 'B' ? count : waiting_count;

		printk("DIR,%u,%c%c,%llu,%d,%d\n", pass++, waiting, sensor,
		       t - waiting_since_us, (int)count_a, (int)count_b);
		waiting = 0;
		return;
	}
	if (waiting == sensor) {
		printk("LONE,%c,%d\n", waiting, (int)waiting_count);
	}
	waiting = sensor;
	waiting_since_us = t;
	waiting_count = count;
}

static void sub_a_cb(const void *msgin)
{
	ARG_UNUSED(msgin);
	arrival('A', msg_a.data);
}

static void sub_b_cb(const void *msgin)
{
	ARG_UNUSED(msgin);
	arrival('B', msg_b.data);
}

int main(void)
{
	printk("\n#BOOT,direction_judge,window_ms=%d\n", PASS_WINDOW_MS);

	wifi_bootstrap_connect();

	rmw_zenoh_pico_set_mode("client");
	rmw_zenoh_pico_set_unicast(ZENOH_ROUTER_IP, ZENOH_ROUTER_PORT, NULL, NULL);
	printk("#ROUTER,tcp/%s:%s\n", ZENOH_ROUTER_IP, ZENOH_ROUTER_PORT);

	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;
	rcl_node_t node;
	rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();

	RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
	RCCHECK(rclc_node_init_default(&node, "direction_judge", "", &support));
	RCCHECK(rclc_subscription_init_default(&subscription_a, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "/direction/a"));
	RCCHECK(rclc_subscription_init_default(&subscription_b, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "/direction/b"));
	RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
	RCCHECK(rclc_executor_add_subscription(&executor, &subscription_a, &msg_a, &sub_a_cb,
					       ON_NEW_DATA));
	RCCHECK(rclc_executor_add_subscription(&executor, &subscription_b, &msg_b, &sub_b_cb,
					       ON_NEW_DATA));

	printk("#RUN\n");
	/* Returns only on an error other than a wait timeout */
	rcl_ret_t rc = rclc_executor_spin(&executor);

	printk("#ERR,spin,rc=%d\n", (int)rc);
	return 0;
}
