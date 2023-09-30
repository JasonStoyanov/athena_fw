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
extern struct k_sem button_hold_sem;

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


//TODO: add a thread for togling the LED when button is pressed

void led_toggle_thread(int unused1, int unused2, int unused3)
{
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);
    while (1) {
		
		//Take semaphore
        k_sem_take(&button_hold_sem, K_FOREVER);

        led_set(1);
		k_sleep(K_MSEC(400)); 
        led_set(0);

    }

}


