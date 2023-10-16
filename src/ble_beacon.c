// Creator: Yasen Stoyanov
// Brief: BLE beacon implementation for the Athena project
// Date: 15/9/2023
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "ble_beacon.h"
#include "ble_acs.h"
#include "usf.h"

#define NON_CONNECTABLE_ADV_IDX 0
#define CONNECTABLE_ADV_IDX     1

static void advertising_work_handle(struct k_work *work);

static K_WORK_DEFINE(advertising_work, advertising_work_handle);

static struct bt_le_ext_adv *ext_adv[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

static const struct bt_le_adv_param *non_connectable_adv_param =
	BT_LE_ADV_NCONN_SLOW_ADV;			

static const struct bt_le_adv_param *connectable_adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
			BT_GAP_ADV_FAST_INT_MIN_2, /* 100 ms */
			BT_GAP_ADV_FAST_INT_MAX_2, /* 150 ms */
			NULL);


static const struct bt_data connectable_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
	//TODO: maybe add the SMT DFU service here
};

/*  Select the type of Eddystone frame type
   0 - URL frame type
   1 - UID frame type
   2 - Athena Beacon frame
   */
#define EDDYSTONE_FRAME_TYPE 2
#if (EDDYSTONE_FRAME_TYPE == 0)
/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 * https://github.com/google/eddystone/tree/master/eddystone-url
 */
static const struct bt_data non_connectable_data[] = {
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
static const struct bt_data non_connectable_data[] = {
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
static struct bt_data non_connectable_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  0xaa, 0xfe,																											  /* Eddystone UUID */
				  0x00,																													  /* Eddystone-UID frame type */
				  0x00,																													  /* Calibrated Tx power at 0m */
				  0x11,  																												  /* [5] Temperature MSB*/
				  0x22 , 																												  /* [6] Temperature LSB*/
				  0x33,  																												  /* [7] Humidity */
				  0x44,  																												  /* [8] APP Status register*/
				  0x55,  																												  /* [9] App Version */
				  0x66, 																												  /* [10] Unused */
				  BT_UUID_16_ENCODE(0x7788), 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f)											  /*  Unused */
};

#endif

//SMP service UUID and Device name
//Scan data returned by the non connectable device (Beacon)
//Note: Scan data is limited to 31 bytes, so the SMP service UUID and device name should not exceed 31 bytes


//FIXME: Avoid using Scan response, as it increases the current consumption of the beacon
//Also the scan should be initiated by the mobile device, which drains its battery
//For the non-connectable beacon, the scan response is not needed
const struct bt_data non_connectable_sd[] = {	
	/*
	//SMP service UUID
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
		      0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
			  */
			  
	BT_DATA(BT_DATA_NAME_COMPLETE, BEACON_NAME, BEACON_NAME_LEN),
};


static uint8_t g_athena_id;
static void athena_id_cb(uint8_t beacon_id)
{
	//g_athena_id = beacon_id;
	ble_beacon_set_athena_id(beacon_id);
	usf_write_beacon_id(beacon_id);

}

static uint8_t athena_id_rd_cb(void)
{
	return ble_beacon_get_athena_id();
	//return g_athena_id;
}

static struct ble_acs_cb acs_callbacks = {
	.id_cb = athena_id_cb,
	.id_rd_cb = athena_id_rd_cb
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


static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
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

	//return bt_le_ext_adv_start(adv_set, BT_LE_EXT_ADV_START_DEFAULT);
	return 0;
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

	err = bt_set_name(CONNECTABLE_DEVICE_NAME);
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


int ble_beacon_init(void) {

	int err;

	#if (MCUBOOT_DBG == 1)
	printk("build time: " __DATE__ " " __TIME__ "\n");
    smp_bt_register();
	#endif

	printk("Starting Bluetooth multiple advertising sets example\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");


	err = ble_acs_init(&acs_callbacks);
	if (err) {
		printk("Failed to init Athena Configuration Service (err:%d)\n", err);
		return err;
	}

	err = non_connectable_adv_create();
	if (err) {
		return err;
	}
	//TODO: add return value check!
	bt_le_ext_adv_start(ext_adv[NON_CONNECTABLE_ADV_IDX], BT_LE_EXT_ADV_START_DEFAULT);

	printk("Non-connectable advertising started\n");

	//#if (CONNECTABLE_ADV_EN == 1)
	err = connectable_adv_create();
	if (err) {
		return err;
	}
	printk("Connectable advertising Created\n");
	//#endif

	return 0;

}

int ble_beacon_update_adv_data(void)
{
	int err;

	err = bt_le_ext_adv_set_data(ext_adv[NON_CONNECTABLE_ADV_IDX], non_connectable_data, ARRAY_SIZE(non_connectable_data),
	 			     non_connectable_sd,ARRAY_SIZE(non_connectable_sd));

	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return err;
	}

	return err;
}


void  ble_beacon_set_adv_frame_data(uint8_t * adv_data) {

	non_connectable_data[2].data = adv_data;
}



int ble_beacon_connectable_adv_start(void) {
	int ret;
	ret = bt_le_ext_adv_start(ext_adv[CONNECTABLE_ADV_IDX], BT_LE_EXT_ADV_START_DEFAULT);
	if (ret) {
		printk("Failed to Start connectable advertising\n");
	}
	printk("Connectable advertising started\n");
	return ret;

}


int ble_beacon_connectable_adv_stop(void) {

	return bt_le_ext_adv_stop(ext_adv[CONNECTABLE_ADV_IDX]);
}


//function that sets the value of g_athena_id
int ble_beacon_set_athena_id(uint8_t value) {
	g_athena_id = value;
	return 0;
}

//function that gets the value of g_athena_id
uint8_t ble_beacon_get_athena_id(void) {
	return g_athena_id;
}