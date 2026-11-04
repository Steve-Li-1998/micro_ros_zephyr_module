/*
 * Applies CONFIG_MICROROS_DOMAIN_ID and CONFIG_MICROROS_CLIENT_KEY without changes to the
 * application: rclc_support_init() takes no init options, so they are set where rmw creates
 * them. Linked with --wrap=rmw_init_options_init, only when one of the options is set.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <rmw/init_options.h>
#include <rmw/ret_types.h>
#include <rmw_microros/rmw_microros.h>

rmw_ret_t __real_rmw_init_options_init(rmw_init_options_t *init_options,
				       rcutils_allocator_t allocator);

rmw_ret_t __wrap_rmw_init_options_init(rmw_init_options_t *init_options,
				       rcutils_allocator_t allocator)
{
	rmw_ret_t ret = __real_rmw_init_options_init(init_options, allocator);

	if (ret != RMW_RET_OK) {
		return ret;
	}
#if CONFIG_MICROROS_DOMAIN_ID >= 0
	init_options->domain_id = CONFIG_MICROROS_DOMAIN_ID;
	printk("micro-ROS: domain_id=%d\n", CONFIG_MICROROS_DOMAIN_ID);
#endif
#if CONFIG_MICROROS_CLIENT_KEY != 0
	ret = rmw_uros_options_set_client_key((uint32_t)CONFIG_MICROROS_CLIENT_KEY, init_options);
	printk("micro-ROS: client_key=0x%08x\n", (unsigned int)CONFIG_MICROROS_CLIENT_KEY);
#endif
	return ret;
}
