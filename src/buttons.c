#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "buttons.h"

#define BUTTON_NODE DT_ALIAS(sw4)
#define DEBOUNCE_WINDOW_MS 15

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;
static button_event_handler_t user_cb;


/* After button debounce time (cooldown) expires, check the button state and pass it to the user provided callback function
 * Note: The user callback function will be executed the workqueue handler contex, therefore: actions on this context shall be kept brief to allow proper functioning of other parts of the system. */
static void debounce_window_expired(struct k_work *work)
{
    ARG_UNUSED(work);

    int val = gpio_pin_get_dt(&button);
    enum button_evt evt = val ? BUTTON_EVT_PRESSED : BUTTON_EVT_RELEASED;
    if (user_cb) {
        user_cb(evt);
    }
}

static K_WORK_DELAYABLE_DEFINE(debounce_work, debounce_window_expired);

static void button_pressed(const struct device *gpio_dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	int err;

 	k_work_reschedule(&debounce_work, K_MSEC(DEBOUNCE_WINDOW_MS));
  
}

int buttons_init(button_event_handler_t handler) {

    int ret;

 	if (!handler) {
        return -EINVAL;
    }

    user_cb = handler;

	if (!gpio_is_ready_dt(&button)) {
		return -ENOENT;
	}

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
	 if (ret) {
	 	return ret;
	 }

	ret = gpio_pin_interrupt_configure_dt(&button,
						      GPIO_INT_EDGE_BOTH);
	if (ret) {
	 	return ret;
	 }

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	ret = gpio_add_callback(button.port, &button_cb_data);
	if (ret) {
	 	return ret;
	 }


    return ret;

}

//TODO: make sure we are using PORT event and not GPIOTE IN event
//This is done by adding the following to the devicetree: sense-edge-mask = < 0xffffffff >; to the gpio
/*You use the SENSE bits in the PIN_CNF register to enable GPIO sensing and DETECT signal generation, and then enable the PORT interrupt in the GPIOTE module*/
//NOTE: this will probably require enabling GPIOTE module in the devicetree!!