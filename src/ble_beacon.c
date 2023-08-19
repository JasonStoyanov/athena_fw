#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "ble_beacon.h"

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

//TODO: replace scan response data with SMP service UUID
/* Set Scan Response data */
// static const struct bt_data sd[] = {
// 	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
// };

//SMP service UUID and Device name
//Note: Scan data is limited to 31 bytes, so the SMP service UUID and device name should not exceed 31 bytes
const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
		      0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

#if (MCUBOOT_DBG == 1)
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	//dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	//dk_set_led_off(CON_STATUS_LED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
};
#endif

void bt_ready()
{
	char addr_s[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr = {0};
	size_t count = 1;
	int err;
	
	/* Start advertising */
	// err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
	// 					  sd, ARRAY_SIZE(sd));
	//NOTE: we can reconfigure the BLE advertising interval here. See first parameter of bt_le_adv_start
	// err = bt_le_adv_start(BT_LE_ADV_NCONN_SLOW_ADV, ad, ARRAY_SIZE(ad),
	//  					  sd, ARRAY_SIZE(sd));
	//Start connectable advertising
	//FIXME: this new connectable advertising should also be configured to 1s interval, as the non-connectable advertising above!
    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
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