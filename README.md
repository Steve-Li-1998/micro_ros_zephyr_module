# direction: application `sensor_b`

The direction example has three micro-ROS applications on `rmw_zenoh_pico`: `sensor_a`, `sensor_b` and `judge`. An object passes the two sensors one after the other. Each sensor publishes the number of every arrival, `sensor_a` on `/direction/a` and `sensor_b` on `/direction/b`; `judge` subscribes to both and takes the direction of the movement from the order in which the two messages of a pass reach it. All topics are `std_msgs/msg/Int32`.

This application is `sensor_b`. A thread of its own waits for arrivals and publishes each one at once, so the publish is not in a callback of the executor. The executor has one timer of 1 s, which prints the number of arrivals so far. `sensor_a` is the same application with the other topic.

This directory is a checkout of `micro_ros_zephyr_module` (fork, branch `jazzy-esp32c6`, commit `728cd68`) with the application in `src/` and every Zephyr module it needs in `modules/`:

| Module | From |
|---|---|
| `modules/libmicroros` | this repository |
| `modules/zenoh-pico` | copy of the fork's branch `lf-uros`, commit `b6528f20` |
| `modules/rmw_zenoh_pico` | copy of the fork's branch `lf-uros`, commit `8e8de95` |
| `modules/wifi_bootstrap` | copy from the superproject |

## Where the arrivals come from

`-DDIRECTION_EVENT_SOURCE=` chooses the event source when the application is built:

| Source | An arrival is | Time of the arrival | Wiring on the ESP32-C6-DevKitC |
|---|---|---|---|
| `tof` (default) | the first reading of a VL53L0X distance sensor below 400 mm after three readings at or above it | when the reading came back; a reading takes about 33 ms | sensor VIN to 3V3, GND to GND, SDA to GPIO6, SCL to GPIO7 |
| `gpio` | an edge to high on a pin | the edge, taken in the interrupt | the pulse to GPIO10, which is pulled down; grounds connected |

The sensor is polled: the VL53L0X driver of Zephyr has no interrupt. `src/event_tof.c` and `src/event_gpio.c` are the two sources, `tof.conf` and `gpio.conf` their Kconfig, and `boards/esp32c6_devkitc.overlay` has the sensor and the pin.

## Build

```
west build -b esp32c6_devkitc <this directory> -d build-zenoh_direction_sensor_b
west build -b esp32c6_devkitc <this directory> -d build-zenoh_direction_sensor_b -- -DDIRECTION_EVENT_SOURCE=gpio
```

The shell must not have ROS sourced: the build of libmicroros then finds the host's ROS packages and fails. Zephyr needs the patches of the superproject (`patches/README.md`).

The Zenoh router is `tcp/192.168.50.164:8889`; `-DZENOH_ROUTER_IP=` and `-DZENOH_ROUTER_PORT=` change it. The Wi-Fi credentials are in `wifi.conf`, which is not in Git: copy `wifi.conf.example` and fill it in.

## Serial output

```
EVT,<record>,<time_us>,B,<count>,<detail>    before a publish
#STAT,<time_us>,events=<count>                 once a second
```

`<time_us>` is the board's own time since boot; for `EVT` it is the time of the arrival. `<count>` is the number of the arrival, from 0, and the value that is published. `<detail>` is the distance in mm with `tof` and 0 with `gpio`. Lines that start with `#` are status.

## State

Both event sources build. Neither has run on a board yet.
