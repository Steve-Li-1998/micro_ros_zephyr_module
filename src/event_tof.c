/*
 * Arrivals from a VL53L0X time-of-flight distance sensor, polled.
 *
 * The object is at the sensor while the distance is below
 * DIRECTION_TOF_NEAR_MM. An arrival is the first such reading after
 * DIRECTION_TOF_CLEAR_READINGS readings in a row at or above the threshold, so
 * that one passing object is one arrival. A reading takes the sensor about
 * 33 ms; the time of an arrival is the time its reading came back, which is
 * up to one reading after the object got there.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include "event_source.h"

#ifndef DIRECTION_TOF_NEAR_MM
#define DIRECTION_TOF_NEAR_MM 400
#endif
#ifndef DIRECTION_TOF_CLEAR_READINGS
#define DIRECTION_TOF_CLEAR_READINGS 3
#endif

static const struct device *const tof = DEVICE_DT_GET(DT_NODELABEL(vl53l0x));

/* Readings in a row without the object; an arrival needs enough of them first. */
static int clear_readings = DIRECTION_TOF_CLEAR_READINGS;

const char *event_source_name(void)
{
	return "tof";
}

int event_source_init(void)
{
	return device_is_ready(tof) ? 0 : -ENODEV;
}

int event_source_wait(struct event *event)
{
	for (;;) {
		struct sensor_value distance;
		int rc = sensor_sample_fetch(tof);

		if (rc == 0) {
			rc = sensor_channel_get(tof, SENSOR_CHAN_DISTANCE, &distance);
		}
		if (rc != 0) {
			return rc;
		}

		uint64_t time_us = k_cyc_to_us_floor64(k_cycle_get_64());
		/* The driver gives metres: val1 whole, val2 millionths. */
		int32_t mm = distance.val1 * 1000 + distance.val2 / 1000;

		if (mm >= DIRECTION_TOF_NEAR_MM) {
			if (clear_readings < DIRECTION_TOF_CLEAR_READINGS) {
				clear_readings++;
			}
			continue;
		}

		bool arrival = clear_readings >= DIRECTION_TOF_CLEAR_READINGS;

		clear_readings = 0;
		if (arrival) {
			event->time_us = time_us;
			event->detail = mm;
			return 0;
		}
	}
}
