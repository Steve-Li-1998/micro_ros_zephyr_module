// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
// Copyright(C) 2024 eSOL Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <rmw/rmw.h>
#include <rmw_zenoh_pico/rmw_zenoh_pico.h>

rmw_ret_t
rmw_serialize(
  const void * ros_message,
  const rosidl_message_type_support_t * type_support,
  rmw_serialized_message_t * serialized_message)
{
  RMW_ZENOH_FUNC_ENTRY(NULL);

  RMW_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_support, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(serialized_message, RMW_RET_INVALID_ARGUMENT);

  const rosidl_message_type_support_t * ts = find_message_type_support(type_support);
  if(ts == NULL)
    return RMW_RET_ERROR;	// error was already set by find_message_type_support

  size_t size = 0;
  uint8_t * msg_bytes = rmw_zenoh_pico_serialize((const message_type_support_callbacks_t *)ts->data,
						 ros_message,
						 &size);
  if(msg_bytes == NULL)
    return RMW_RET_ERROR;

  if((serialized_message->buffer_capacity < size) &&
     (rmw_serialized_message_resize(serialized_message, size) != RMW_RET_OK)){
    TOPIC_FREE(msg_bytes);
    RMW_SET_ERROR_MSG("serialized message buffer is too small and cannot be resized");
    return RMW_RET_ERROR;
  }

  memcpy(serialized_message->buffer, msg_bytes, size);
  serialized_message->buffer_length = size;
  TOPIC_FREE(msg_bytes);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_deserialize(
  const rmw_serialized_message_t * serialized_message,
  const rosidl_message_type_support_t * type_support,
  void * ros_message)
{
  RMW_ZENOH_FUNC_ENTRY(NULL);

  RMW_CHECK_ARGUMENT_FOR_NULL(serialized_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_support, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);

  const rosidl_message_type_support_t * ts = find_message_type_support(type_support);
  if(ts == NULL)
    return RMW_RET_ERROR;	// error was already set by find_message_type_support

  if(!rmw_zenoh_pico_deserialize(serialized_message->buffer,
				 serialized_message->buffer_length,
				 (const message_type_support_callbacks_t *)ts->data,
				 ros_message)){
    RMW_SET_ERROR_MSG("Typesupport deserialize error.");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_get_serialized_message_size(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  size_t * size)
{
  RMW_ZENOH_FUNC_ENTRY(NULL);

  (void)type_support;
  (void)message_bounds;
  (void)size;
  RMW_ZENOH_LOG_INFO("function not implemented");
  return RMW_RET_UNSUPPORTED;
}
