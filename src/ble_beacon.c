#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "ble_beacon.h"


#define NON_CONNECTABLE_ADV_IDX 0
#define CONNECTABLE_ADV_IDX     1

static void advertising_work_handle(struct k_work *work);

static K_WORK_DEFINE(advertising_work, advertising_work_handle);

static struct bt_le_ext_adv *ext_adv[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
// static const struct bt_le_adv_param *non_connectable_adv_param =
// 	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME,
// 			0x140, /* 200 ms */
// 			0x190, /* 250 ms */
// 			NULL);

static const struct bt_le_adv_param *non_connectable_adv_param =
	BT_LE_ADV_NCONN_SLOW_ADV;			

static const struct bt_le_adv_param *connectable_adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
			BT_GAP_ADV_FAST_INT_MIN_2, /* 100 ms */
			BT_GAP_ADV_FAST_INT_MAX_2, /* 150 ms */
			NULL);

// static const struct bt_data non_connectable_data[] = {
// 	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
// 	BT_DATA_BYTES(BT_DATA_URI, /* The URI of the https://www.nordicsemi.com website */
// 		      0x17, /* UTF-8 code point for “https:” */
// 		      '/', '/', 'w', 'w', 'w', '.',
// 		      'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.',
// 		      'c', 'o', 'm')
// };

static const struct bt_data non_connectable_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  0xaa, 0xfe,																											  /* Eddystone UUID */
				  0x00,																													  /* Eddystone-UID frame type */
				  0x00,																													  /* Calibrated Tx power at 0m */
				  BT_UUID_16_ENCODE(0x1122), BT_UUID_16_ENCODE(0x3344), BT_UUID_16_ENCODE(0x5566), BT_UUID_16_ENCODE(0x7788), 0x08, 0x09, /* 10-byte Namespace */
				  0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f)	
};

static const struct bt_data connectable_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
};


static void adv_connected_cb(struct bt_le_ext_adv *adv,
			     struct bt_le_ext_adv_connected_info *info)
{
	printk("Advertiser[%d] %p connected conn %p\n", bt_le_ext_adv_get_index(adv),
		adv, info->conn);
}
static const struct bt_le_ext_adv_cb adv_cb = {
	.connected = adv_connected_cb
};

static void connectable_adv_start(void)
{
	int err;

	err = bt_le_ext_adv_start(ext_adv[CONNECTABLE_ADV_IDX], BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start connectable advertising (err %d)\n", err);
	}
}

static void advertising_work_handle(struct k_work *work)
{
	connectable_adv_start();
}

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
const struct bt_data non_connectable_sd[] = {
	
	/*
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
		      0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
			  */
			  
	BT_DATA(BT_DATA_NAME_COMPLETE, BEACON_NAME, BEACON_NAME_LEN),
};

#if (MCUBOOT_DBG == 1)
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//dk_set_led_on(CON_STATUS_LED);

	printk("Connected %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//dk_set_led_off(CON_STATUS_LED);

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	/* Process the disconnect logic in the workqueue so that
	 * the BLE stack is finished with the connection bookkeeping
	 * logic and advertising is possible.
	 */
	k_work_submit(&advertising_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif

//FIXME: old function, no longer to be used!
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
	  					  non_connectable_sd, ARRAY_SIZE(non_connectable_sd));
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


static int advertising_set_create(struct bt_le_ext_adv **adv,
				  const struct bt_le_adv_param *param,
				  const struct bt_data *ad, size_t ad_len)
{
	int err;
	struct bt_le_ext_adv *adv_set;

	err = bt_le_ext_adv_create(param, &adv_cb,
				   adv);
	if (err) {
		return err;
	}

	adv_set = *adv;

	printk("Created adv: %p\n", adv_set);

	err = bt_le_ext_adv_set_data(adv_set, ad, ad_len,
				     NULL,0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return err;
	}

	return bt_le_ext_adv_start(adv_set, BT_LE_EXT_ADV_START_DEFAULT);
}

static int non_connectable_adv_create(void)
{
	int err;

	err = advertising_set_create(&ext_adv[NON_CONNECTABLE_ADV_IDX], non_connectable_adv_param,
				     non_connectable_data, ARRAY_SIZE(non_connectable_data));
	if (err) {
		printk("Failed to create a non-connectable advertising set (err %d)\n", err);
	}

	return err;
}

static int connectable_adv_create(void)
{
	int err;

	err = bt_set_name(CONFIG_BT_DEVICE_NAME);
	if (err) {
		printk("Failed to set device name (err %d)\n", err);
		return err;
	}

	err = advertising_set_create(&ext_adv[CONNECTABLE_ADV_IDX], connectable_adv_param,
				     connectable_data, ARRAY_SIZE(connectable_data));
	if (err) {
		printk("Failed to create a connectable advertising set (err %d)\n", err);
	}

	return err;
}



void ble_beacon_init(void) {

	int err;

	#if (MCUBOOT_DBG == 1)
	printk("build time: " __DATE__ " " __TIME__ "\n");
    smp_bt_register();
	#endif

	printk("Starting Bluetooth multiple advertising sets example\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	err = non_connectable_adv_create();
	if (err) {
		return 0;
	}

	printk("Non-connectable advertising started\n");

	// err = connectable_adv_create();
	// if (err) {
	// 	return 0;
	// }

	printk("Connectable advertising started\n");


}

int ble_beacon_update_adv_data(const struct bt_data *adv_data, size_t adv_data_len)
{
	int err;

	err = bt_le_ext_adv_set_data(ext_adv[NON_CONNECTABLE_ADV_IDX], adv_data, adv_data_len,
				     non_connectable_sd,ARRAY_SIZE(non_connectable_sd));
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return err;
	}

	return err;
}