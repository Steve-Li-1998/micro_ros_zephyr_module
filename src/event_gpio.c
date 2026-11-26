/*
 * Arrivals from a pin: every edge to the active level is one arrival.
 *
 * The pin is event-gpios of the zephyr,user node in the board's overlay. Its
 * interrupt takes the time and hands it to the waiting thread, so the time of
 * an arrival is the time of the edge.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "event_source.h"

static const struct gpio_dt_spec pin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), event_gpios);
static struct gpio_callback callback;

/* Times of edges that the thread has not taken yet. An edge that finds the
 * queue full is dropped. */
K_MSGQ_DEFINE(edges, sizeof(uint64_t), 8, 8);

static void on_edge(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	uint64_t time_us = k_cyc_to_us_floor64(k_cycle_get_64());

	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	(void)k_msgq_put(&edges, &time_us, K_NO_WAIT);
}

const char *event_source_name(void)
{
	return "gpio";
}

int event_source_init(void)
{
	int rc;

	if (!gpio_is_ready_dt(&pin)) {
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(&pin, GPIO_INPUT);
	if (rc != 0) {
		return rc;
	}
	gpio_init_callback(&callback, on_edge, BIT(pin.pin));
	rc = gpio_add_callback(pin.port, &callback);
	if (rc != 0) {
		return rc;
	}
	return gpio_pin_interrupt_configure_dt(&pin, GPIO_INT_EDGE_TO_ACTIVE);
}

int event_source_wait(struct event *event)
{
	int rc = k_msgq_get(&edges, &event->time_us, K_FOREVER);

	event->detail = 0;
	return rc;
}
