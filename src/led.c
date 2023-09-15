// Creator: Yasen Stoyanov
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "led.h"

#define LED_NODE DT_ALIAS(led4)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int led_init(void) 
{
    int ret;

	if (!gpio_is_ready_dt(&led)) {
		return -ENOENT;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_HIGH);
	 if (ret) {
	 	return ret;
	 }


    return ret;

}

//NOTE: these functions can be a MACRO or inlined
int led_set(uint32_t val) 
{

    int ret;
    ret = gpio_pin_set_dt(&led, val);
    return ret;
}


int led_toggle(void) 
{

    int ret;
    ret = gpio_pin_toggle_dt(&led);
    return ret;
}
