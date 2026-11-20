# relay_chain: application `relay`

One of the four micro-ROS applications of the relay_chain example (`source_a`, `source_b`, `relay`, `monitor`), on `rmw_zenoh_pico`. Subscribes to `/chain/a` and `/chain/b` and publishes `/chain/relayed` for every sample: `2 * value` for a sample of `/chain/a`, `2 * value + 1` for a sample of `/chain/b`. All topics are `std_msgs/msg/Int32`.

This directory is a checkout of `micro_ros_zephyr_module` (fork, branch `jazzy-esp32c6`, commit `728cd68`) with the application in `src/` and every Zephyr module it needs in `modules/`:

| Module | From |
|---|---|
| `modules/libmicroros` | this repository |
| `modules/zenoh-pico` | copy of the fork's branch `lf-uros`, commit `da31674a` |
| `modules/rmw_zenoh_pico` | copy of the fork's branch `lf-uros`, commit `8e8de95` |
| `modules/wifi_bootstrap` | copy from the superproject |

## Build

```
west build -b esp32c6_devkitc <this directory> -d build-zenoh_relay_chain_relay
```

The shell must not have ROS sourced: the build of libmicroros then finds the host's ROS packages and fails. Zephyr needs the patches of the superproject (`patches/README.md`).

The Zenoh router is `tcp/192.168.50.164:8889`; `-DZENOH_ROUTER_IP=` and `-DZENOH_ROUTER_PORT=` change it. The Wi-Fi credentials are in `wifi.conf`, which is not in Git: copy `wifi.conf.example` and fill it in.

## Serial output

```
PUB,<record>,<time_us>,<A|B|R>,<value>    before a publish
SUB,<record>,<time_us>,<A|B|R>,<value>    at the start of a subscription callback
```

`<time_us>` is the board's own time since boot. Lines that start with `#` are status.
