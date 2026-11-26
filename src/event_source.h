/*
 * Where the arrivals of an object at this sensor come from. One of
 * event_tof.c and event_gpio.c is built, as DIRECTION_EVENT_SOURCE says
 * (CMakeLists.txt).
 */
#ifndef EVENT_SOURCE_H
#define EVENT_SOURCE_H

#include <stdint.h>

/** An arrival of the object at this sensor. */
struct event {
	/** The board's time since boot at the arrival, in microseconds. */
	uint64_t time_us;
	/** What the source measured: a distance in mm, or 0. */
	int32_t detail;
};

/** @return Name of the source that is built in: "tof" or "gpio". */
const char *event_source_name(void);

/**
 * Makes the source ready. Call it once, before event_source_wait().
 * @return 0, or a negative errno value.
 */
int event_source_init(void);

/**
 * Blocks until the object arrives at the sensor again.
 * @param event Where the arrival is written.
 * @return 0, or a negative errno value.
 */
int event_source_wait(struct event *event);

#endif /* EVENT_SOURCE_H */
