/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/* YS Notes */
// BLE data is sent LSB first


#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/settings/settings.h>

#include <zephyr/pm/pm.h>  

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY
 * and 1000ms interval. */
#define BT_LE_ADV_NCONN_SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 BT_GAP_ADV_SLOW_INT_MAX, \
						 NULL)

/*  Select the type of Eddystone frame type
   0 - URL frame type
   1 - UID frame type
   2 - DBG frame
   */
#define EDDYSTONE_FRAME_TYPE 2

#if (EDDYSTONE_FRAME_TYPE == 0)
/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 * https://github.com/google/eddystone/tree/master/eddystone-url
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  0xaa, 0xfe, /* Eddystone UUID */
				  0x10,		  /* Eddystone-URL frame type */
				  0x00,		  /* Calibrated Tx power at 0m */
				  0x00,		  /* URL Scheme Prefix http://www. */
				  'o', 'p', 'e', 'n', '4', 't',
				  'e', 'c', 'h',
				  0x07) /* .com */
};
#elif (EDDYSTONE_FRAME_TYPE == 1)

/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 * https://github.com/google/eddystone/tree/master/eddystone-url
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  0xaa, 0xfe,												  /* Eddystone UUID */
				  0x00,														  /* Eddystone-UID frame type */
				  0x00,														  /* Calibrated Tx power at 0m */
				  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, /* 10-byte Namespace */
				  0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f)						  /* 6-byte Instance */
};

#elif (EDDYSTONE_FRAME_TYPE == 2)

/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 * https://github.com/google/eddystone/tree/master/eddystone-url
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  0xaa, 0xfe,																											  /* Eddystone UUID */
				  0x00,																													  /* Eddystone-UID frame type */
				  0x00,																													  /* Calibrated Tx power at 0m */
				  BT_UUID_16_ENCODE(0x1122), BT_UUID_16_ENCODE(0x3344), BT_UUID_16_ENCODE(0x5566), BT_UUID_16_ENCODE(0x7788), 0x08, 0x09, /* 10-byte Namespace */
				  0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f)																					  /* 6-byte Instance */
};

#endif

static struct bt_data ad_bme280[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  0xaa, 0xfe,																											  /* Eddystone UUID */
				  0x00,																													  /* Eddystone-UID frame type */
				  0x00,																													  /* Calibrated Tx power at 0m */
				  BT_UUID_16_ENCODE(0x1122), BT_UUID_16_ENCODE(0x3344), BT_UUID_16_ENCODE(0x5566), BT_UUID_16_ENCODE(0x7788), 0x08, 0x09, /* 10-byte Namespace */
				  0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f)																					  /* 6-byte Instance */
};

static uint8_t adv_data[] = {BT_DATA_SVC_DATA16,
							 0xaa, 0xfe,																											 /* Eddystone UUID */
							 0x00,																													 /* Eddystone-UID frame type */
							 0x00,																													 /* Calibrated Tx power at 0m */
							 BT_UUID_16_ENCODE(0x1122), BT_UUID_16_ENCODE(0x3344), BT_UUID_16_ENCODE(0x5566), BT_UUID_16_ENCODE(0x7788), 0x08, 0x09, /* 10-byte Namespace */
							 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready()
{
	char addr_s[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr = {0};
	size_t count = 1;
	int err;


	
	/* Start advertising */
	// err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
	// 					  sd, ARRAY_SIZE(sd));
	//NOTE: we can reconfigure the BLE advertising interval here. See first parameter of bt_le_adv_start
	err = bt_le_adv_start(BT_LE_ADV_NCONN_SLOW_ADV, ad, ARRAY_SIZE(ad),
	 					  sd, ARRAY_SIZE(sd));
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	/* For connectable advertising you would use
	 * bt_le_oob_get_local().  For non-connectable non-identity
	 * advertising an non-resolvable private address is used;
	 * there is no API to retrieve that.
	 */

	bt_id_get(&addr, &count);
	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

	printk("Beacon started, advertising as %s\n", addr_s);
}

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

void main(void)
{

	int err;
	const struct device *dev = get_bme280_device();

	if (dev == NULL)
	{
		return;
	}

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	bt_ready();

	while (1)
	{
		struct sensor_value temp, press, humidity;

		sensor_sample_fetch(dev);
/*
			Representation of a sensor readout value per Zepgyr's sensor API.
			 0.5: val1 =  0, val2 =  500000
			-0.5: val1 =  0, val2 = -500000
			-1.0: val1 = -1, val2 =  0
			-1.5: val1 = -1, val2 = -500000
*/
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

		//Print the temperature, pressure and humidity values on the console for debugging purposes
		printk("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
			   temp.val1, temp.val2, press.val1, press.val2,
			   humidity.val1, humidity.val2);


		//Put temp into the advertising data
		//The integer part of the temperature is stored in val1, and the decimal part is stored in val2
		//Signle byte is enough to store the integer part of the temperature (-40 to 80), so we are discarding 3 MSB bytes from val1
		adv_data[5] = ((temp.val1 >> 0) & 0xff);
		//We normalize the decimal part of the temperature by dividing it by 10000, so that it can be stored in a single byte
		adv_data[6] = ((temp.val2 / 10000) & 0xff);

		//Put humidity into the advertising data, 
		//we take just the the LSB byte, as the humidity can take take values only from 0 to 100
		//We are discarding the humidity value after the decimal point, so humidity.val2 is not used
		adv_data[7] = ((humidity.val1 >> 0) & 0xff);

		ad_bme280[2].data = adv_data;

		err = bt_le_adv_update_data(ad_bme280, ARRAY_SIZE(ad_bme280),
									sd, ARRAY_SIZE(sd));
		if (err)
		{
			printk("Adv. data update failed (err %d)\n", err);
			return;
		}

		//Note:
		//We Take a measurement from the sensor every minute. The BLE stack will be active and adveritising every 1s (with the latest data), because as a beacon the perihperal device 
		//should be able to see the advertising packets. If we advertise only once a minute, the central device will not be able to see the advertising packets. 
		//So as a balanced approach we advertise every 1s, but take a measurement every minute. 
		//TODO: put the SPI device in low power mode here, and wake-it up before calling sensor_sample_fetch(dev);

		k_sleep(K_SECONDS(60));
	}
}
