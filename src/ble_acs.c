// Creator: Yasen Stoyanov
// Athena Configuration Service (ACS) implementation

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
static uint8_t beacon_id;

static ssize_t write_id(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	// LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
	// 	(void *)conn);
	if (len != 1U) {
		//LOG_DBG("Write led: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (offset != 0) {
		//LOG_DBG("Write led: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (acs_cb.id_cb) {
		//Read the received value 
		uint8_t val = *((uint8_t *)buf);		
		acs_cb.id_cb(val);
	}
	return len;
}

static ssize_t read_id(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	// get a pointer to beacon_id which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const char *value = attr->user_data;
	//LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);
	if (acs_cb.id_rd_cb) {
		// Call the application callback function to update the get the current value of the button
		beacon_id = acs_cb.id_rd_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
	}

	return 0;
}

/* Create and add the service to the Bluetooth LE stack.*/
BT_GATT_SERVICE_DEFINE(acs_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_ACS),
/* Create and add the Beacon ID characteristic */
BT_GATT_CHARACTERISTIC(BT_UUID_ACS_ID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
			       read_id, write_id, &beacon_id),

);

/* A function to register application callbacks for the LED and Button characteristics  */
int ble_acs_init(struct  ble_acs_cb *callbacks)
{
	if (callbacks) {
		acs_cb.id_cb = callbacks->id_cb;
		acs_cb.id_rd_cb = callbacks->id_rd_cb;
	}

	return 0;
}