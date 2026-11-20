/*
 * Copyright(C) 2024 eSOL Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RMW_ZENOH_PICO_OPTIONS_H
#define RMW_ZENOH_PICO_OPTIONS_H

#include <rmw_zenoh_pico/config.h>

#define _Z_LOG_LVL_NONE  0
#define _Z_LOG_LVL_ERROR 1
#define _Z_LOG_LVL_INFO  2
#define _Z_LOG_LVL_DEBUG 3

#define RMW_ZENOH_SERIAL_MODE "serial"
#define RMW_ZENOH_CLIENT_MODE "client"
#define RMW_ZENOH_PEER_MODE   "peer"

#include <rmw/ret_types.h>
#include <rmw/types.h>

#if defined(__cplusplus)
extern "C"
{
#endif  // if defined(__cplusplus)

  extern void rmw_zenoh_pico_set_mode(const char *mode);
  extern const char *rmw_zenoh_pico_get_mode();

#if defined(RMW_ZENOH_PICO_TRANSPORT_UNICAST)
  extern void rmw_zenoh_pico_set_unicast(const char *connect,
					 const char *connect_port,
					 const char *lieten,
					 const char *listen_port);
#elif defined (RMW_ZENOH_PICO_TRANSPORT_MULTICAST)
  extern void rmw_zenoh_pico_set_mcast(const char *mcast,
				       const char *mcast_port,
				       const char *mcast_dev);
#elif defined (RMW_ZENOH_PICO_TRANSPORT_SERIAL)
  extern void rmw_zenoh_pico_set_serial_device(const char *device);
#endif

  // The sequence number of the sample that the publisher sent last, the one a
  // subscriber finds as publication_sequence_number in the sample's message
  // info. It is 0 before the first publish. A publish that fails in the
  // transport has used a number as well. To pair a publish with its number
  // when several threads use one publisher, call both under a lock of the
  // application.
  extern rmw_ret_t rmw_zenoh_pico_get_last_sequence_number(const rmw_publisher_t *publisher,
							   uint64_t *sequence_number);

  extern int rmw_zenoh_pico_debug_level_get(void);
  extern int rmw_zenoh_pico_debug_level_set(int);

#if defined(__cplusplus)
}
#endif  // if defined(__cplusplus)

#endif
