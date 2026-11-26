# direction: application `judge`

The direction example has three micro-ROS applications on `rmw_zenoh_pico`: `sensor_a`, `sensor_b` and `judge`. An object passes the two sensors one after the other. Each sensor publishes the number of every arrival, `sensor_a` on `/direction/a` and `sensor_b` on `/direction/b`; `judge` subscribes to both and takes the direction of the movement from the order in which the two messages of a pass reach it. All topics are `std_msgs/msg/Int32`.

This application is `judge`. A message waits for one of the other sensor. If that comes within 1 s, the two are one pass, and the sensor whose message came first is the one the object passed first. A message that gets no partner in that time, or is followed by another one of its own sensor, is given up as an arrival that only one sensor saw.

The direction is right as long as the two messages of a pass reach the callbacks in the order of the two arrivals. On its own, the application sees them in the order the network delivers them.

This directory is a checkout of `micro_ros_zephyr_module` (fork, branch `jazzy-esp32c6`, commit `728cd68`) with the application in `src/` and every Zephyr module it needs in `modules/`:

| Module | From |
|---|---|
| `modules/libmicroros` | this repository |
| `modules/zenoh-pico` | copy of the fork's branch `lf-uros`, commit `b6528f20` |
| `modules/rmw_zenoh_pico` | copy of the fork's branch `lf-uros`, commit `8e8de95` |
| `modules/wifi_bootstrap` | copy from the superproject |

## Build

```
west build -b esp32c6_devkitc <this directory> -d build-zenoh_direction_judge
```

The shell must not have ROS sourced: the build of libmicroros then finds the host's ROS packages and fails. Zephyr needs the patches of the superproject (`patches/README.md`).

The Zenoh router is `tcp/192.168.50.164:8889`; `-DZENOH_ROUTER_IP=` and `-DZENOH_ROUTER_PORT=` change it. The Wi-Fi credentials are in `wifi.conf`, which is not in Git: copy `wifi.conf.example` and fill it in.

## Serial output

```
SUB,<record>,<time_us>,<A|B>,<count>                at the start of a subscription callback
DIR,<pass>,<AB|BA>,<gap_us>,<count_a>,<count_b>    a pass; AB: sensor_a first
LONE,<A|B>,<count>                                  an arrival without the other sensor
```

`<time_us>` is the board's own time since boot. `<gap_us>` is the time between the two callbacks of the pass. Lines that start with `#` are status.

## State

It builds. It has not run on a board yet.
