/*
 * direction example, application "sensor_a".
 *
 * One of two sensors that an object passes one after the other. Each time the
 * object arrives at this sensor, the application publishes the number of the
 * arrival on /direction/a. The application "judge" takes the direction of the
 * movement from the order of the two sensors' messages.
 *
 * The arrivals come from an event source, which a thread of its own waits on
 * (event_source.h): a VL53L0X distance sensor that the thread polls, or a
 * pulse on a pin that raises an interrupt. The thread publishes as soon as
 * the source reports an arrival, so the publish is not in a callback of the
 * executor. The executor has one timer, which prints how many arrivals there
 * have been.
 *
 * Serial output:
 *   EVT,<record>,<time_us>,A,<count>,<detail>    before a publish
 *   #STAT,<time_us>,events=<count>                 once a second
 * <time_us> of EVT is the time of the arrival as the event source gives it;
 * <detail> is the distance in mm for the VL53L0X and 0 for the pin.
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

#include "event_source.h"

#define SIDE           'A'
#define TOPIC          "/direction/a"
#define NODE           "direction_sensor_a"
#define STAT_PERIOD_MS 1000

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
static atomic_t events;

K_THREAD_STACK_DEFINE(event_stack, 4096);
static struct k_thread event_thread;

static inline uint64_t now_us(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

/* Waits for arrivals and publishes each one. Runs for as long as the board does. */
static void event_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		struct event event;

		if (event_source_wait(&event) != 0) {
			printk("#ERR,event_source_wait\n");
			k_msleep(1000);
			continue;
		}
		msg.data = (int32_t)atomic_inc(&events);
		printk("EVT,%u,%llu,%c,%d,%d\n", record++, event.time_us, SIDE, (int)msg.data,
		       (int)event.detail);
		if (rcl_publish(&publisher, &msg, NULL) != RCL_RET_OK) {
			printk("#PUBFAIL,%c,%d\n", SIDE, (int)msg.data);
			rcl_reset_error();
		}
	}
}

static void stat_cb(rcl_timer_t *timer, int64_t last_call_time)
{
	ARG_UNUSED(timer);
	ARG_UNUSED(last_call_time);

	printk("#STAT,%llu,events=%d\n", now_us(), (int)atomic_get(&events));
}

int main(void)
{
	printk("\n#BOOT," NODE ",topic=" TOPIC ",source=%s\n", event_source_name());

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
	RCCHECK(rclc_node_init_default(&node, NODE, "", &support));
	RCCHECK(rclc_publisher_init_default(&publisher, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), TOPIC));
	RCCHECK(rclc_timer_init_default2(&timer, &support, RCL_MS_TO_NS(STAT_PERIOD_MS), stat_cb,
					 true));
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_timer(&executor, &timer));

	if (event_source_init() != 0) {
		printk("#ERR,event_source_init,source=%s\n", event_source_name());
		for (;;) { k_sleep(K_FOREVER); }
	}
	k_thread_create(&event_thread, event_stack, K_THREAD_STACK_SIZEOF(event_stack),
			event_main, NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	k_thread_name_set(&event_thread, "event");

	printk("#RUN\n");
	/* Returns only on an error other than a wait timeout */
	rcl_ret_t rc = rclc_executor_spin(&executor);

	printk("#ERR,spin,rc=%d\n", (int)rc);
	return 0;
}
