/* Turns ESP WiFi power save off.
 *
 * The ESP default is WIFI_PS_MIN_MODEM: while a board sleeps, the AP buffers
 * frames to it, so board-to-board round trips sometimes exceed Zephyr's fixed
 * TCP retransmission timeout and every such message is resent once. Zephyr's
 * ESP32 driver does not expose power save, so the ESP API is called directly,
 * as soon as the station has started.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <esp_wifi.h>

static void app_wifi_ps_off_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (esp_wifi_set_ps(WIFI_PS_NONE) != ESP_OK) {
		k_msleep(500);
	}
	printk("#wifi_ps_off\n");
}

K_THREAD_DEFINE(app_wifi_ps_off, 1024, app_wifi_ps_off_main, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
