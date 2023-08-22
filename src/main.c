/* YS Notes */
// BLE data is sent LSB first


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#include <zephyr/settings/settings.h>

#include <zephyr/pm/pm.h> 
#include <zephyr/pm/device.h> 

#include <zephyr/drivers/gpio.h>

#include "battery.h"
#include "ble_beacon.h"


#define STACKSIZE 500
#define PRIORITY 5

#define BAT_LVL_THRESHOLD_MV 2000

/*Get nodes from the devicetree */
#define SENSOR_SPI DT_NODELABEL(spi2)
#define LED_NODE DT_ALIAS(led4)
#define BUTTON_NODE DT_ALIAS(sw4)

#define APP_VER 0x03
//App Status register bits
#define BME280_INIT_ERR 0x01
#define BATT_LOW     	0x02
#define BATT_INIT_ERR 	0x04
//Flag manipulation macros
#define SET_FLAG(reg, flag) (reg |= (flag))
#define CLEAR_FLAG(reg, flag) (reg &= ~(flag))
#define CHECK_FLAG(reg, flag) (reg & (flag))

#define LED_TEST_EN 0
#if (LED_TEST_EN == 1)
/* 1000 msec = 1 sec */
#define LED_TOGGLE_INTERVAL_MS   1000
#endif

static uint8_t app_stat_reg;
static uint8_t adv_data[] = {BT_DATA_SVC_DATA16,
							 0xaa, 0xfe,																											 /* Eddystone UUID */
							 0x00,																													 /* Eddystone-UID frame type */
							 0x00,																													 /* Calibrated Tx power at 0m */
							 BT_UUID_16_ENCODE(0x1122), BT_UUID_16_ENCODE(0x3344), BT_UUID_16_ENCODE(0x5566), BT_UUID_16_ENCODE(0x7788), 0x08, 0x09, /* 10-byte Namespace */
							 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};


/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
const struct device *spi_dev=DEVICE_DT_GET(SENSOR_SPI);

extern const struct bt_data sd[2];

/*
 * Get a device structure from a devicetree node with compatible
 * "bosch,bme280". (If there are multiple, just pick one.)
 */
static const struct device *get_bme280_device(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(bosch_bme280);

	if (dev == NULL)
	{
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev))
	{
		printk("\nError: Device \"%s\" is not ready; "
			   "check the driver initialization logs for errors.\n",
			   dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}


//Note: The app main() function is called by the Zephyr main thread, after the kernel has benn initialized.
void main(void)
{

	int bat_lvl;
	int stat;
	extern struct k_msgq batt_lvl_msgq; 
    int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}
	if (!gpio_is_ready_dt(&button)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		return 0;
	}
 #if (LED_TEST_EN == 1)	
	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}
		k_msleep(LED_TOGGLE_INTERVAL_MS);
	}
 #endif
  #define DO_NOTHING 0
  #if (DO_NOTHING == 0)
	//BME280 sensor
	int err;
	#if (CONFIG_BME280 == 1)
	const struct device *dev = get_bme280_device();
	if (dev == NULL)
	{
		SET_FLAG(app_stat_reg, BME280_INIT_ERR);
	}
	#endif

	ble_beacon_init();

	//Initialize the battery measurement
	//TODO: maybe this should be in the battery_level_thread
	int rc = battery_measure_enable(true);
	if (rc != 0) {
		printk("Failed initialize battery measurement: %d\n", rc);
		SET_FLAG(app_stat_reg, BATT_INIT_ERR);
	}

	//Set the pointer to the advertising data that will be updated 
	//ad_bme280[2].data = adv_data;
	ble_beacon_set_adv_frame_data(adv_data);

	while (1)
	{
		struct sensor_value temp, press, humidity;

		pm_device_action_run( spi_dev, PM_DEVICE_ACTION_RESUME);

#if (CONFIG_BME280 == 1)
		sensor_sample_fetch(dev);
		/*
		*	Representation of a sensor readout value per Zepgyr's sensor API.
		*	 0.5: val1 =  0, val2 =  500000
		*	-0.5: val1 =  0, val2 = -500000
		*	-1.0: val1 = -1, val2 =  0
		*	-1.5: val1 = -1, val2 = -500000
		*/
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

				//Print the temperature, pressure and humidity values on the console for debugging purposes
		printk("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
			   temp.val1, temp.val2, press.val1, press.val2,
			   humidity.val1, humidity.val2);
#endif

		/*Check if we have new battery level data*/
        stat = k_msgq_get(&batt_lvl_msgq, &bat_lvl, K_NO_WAIT);
		if (stat == 0) {
			if (bat_lvl < BAT_LVL_THRESHOLD_MV) {
				SET_FLAG(app_stat_reg, BATT_LOW);
			}
			else {
				CLEAR_FLAG(app_stat_reg, BATT_LOW);
			}
			printk("Battery level: %d\n", bat_lvl);			
		}

		/*Update the advertising data*/
		/*
		 * Temperature handling
		 * The integer part of the temperature is stored in temp.val1, and the decimal part is stored in temp.val2
		 * Signle byte is enough to store the integer part of the temperature (-40 to 80), so we are discarding 3 MSB bytes from val1
		 * We normalize the decimal part of the temperature (temp.val2) by dividing it by 10000, so that it can be stored in a single byte
		 * 
		 * Humidity handling
		 * We take just the the LSB byte, as the humidity can take take values only from 0 to 100
		 * We are discarding the humidity value after the decimal point, so humidity.val2 is not used
		 */
		adv_data[5] = ((temp.val1 >> 0) & 0xff);
		adv_data[6] = ((temp.val2 / 10000) & 0xff);
		adv_data[7] = ((humidity.val1 >> 0) & 0xff);
		adv_data[8] = app_stat_reg; 
		adv_data[9] = APP_VER;
		

		err = ble_beacon_update_adv_data();
		if (err)
		{
			printk("Adv. data update failed (err %d)\n", err);
			//return; //FIXME: How do we indicate that app update has failed?!?, maybe LED blink pattern?!?
		}
		/*
		* Note:
		* We Take a measurement from the sensor every minute. The BLE stack will be active and adveritising every 1s (with the latest data), because as a beacon the perihperal device 
		* should be able to see the advertising packets. If we advertise only once a minute, the central device will not be able to see the advertising packets. 
		* So as a balanced approach we advertise every 1s, but take a measurement every minute. 
		* Put the SPI device in low power mode here, and wake-it up before calling sensor_sample_fetch(dev);
		*/
		pm_device_action_run( spi_dev, PM_DEVICE_ACTION_SUSPEND);	
		k_sleep(K_SECONDS(60));	
	}
#endif
}

//Static thread for measuring the battery level
K_THREAD_DEFINE(batt_level_id, STACKSIZE, battery_level_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);
