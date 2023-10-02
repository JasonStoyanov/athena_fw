// Creator: Yasen Stoyanov
// BLE Athena Configuration Service
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "ble_acs.h"

static struct ble_acs_cb acs_cb;

static ssize_t write_id(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
		(void *)conn);
	if (len != 1U) {
		LOG_DBG("Write led: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (offset != 0) {
		LOG_DBG("Write led: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (acs_cb.id_cb) {
		//Read the received value 
		uint8_t val = *((uint8_t *)buf);
		if (val == 0x00 || val == 0x01) {
			//Call the application callback function to update the LED state
			acs_cb.id_cb(val ? true : false);
		} else {
			LOG_DBG("Write led: Incorrect value");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}
	return len;
}

/* Create and add the service to the Bluetooth LE stack.*/
BT_GATT_SERVICE_DEFINE(acs_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_ACS),
/* Create and add the Beacon ID characteristic */
BT_GATT_CHARACTERISTIC(BT_UUID_ACS_ID,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_id, NULL),

);

/* A function to register application callbacks for the LED and Button characteristics  */
int ble_acs_init(struct  ble_acs_cb *callbacks)
{
	if (callbacks) {
		acs_cb.id_cb = callbacks->id_cb;
	}

	return 0;
}