#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>


#define BUTTON_NODE DT_ALIAS(sw4)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);


int buttons_init(void) {

    int ret;
	if (!gpio_is_ready_dt(&button)) {
		return -ENOENT;
	}

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
	// if (ret != 0) {
	// 	return ret;
	// }

    return ret;

}