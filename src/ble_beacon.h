#ifndef BLE_BEACON_H_
#define BLE_BEACON_H_

#define BEACON_NAME "Athena"
#define BEACON_NAME_LEN (sizeof(BEACON_NAME) - 1)

#define MCUBOOT_DBG  1

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY
 * and 1000ms interval. */
#define BT_LE_ADV_NCONN_SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 BT_GAP_ADV_SLOW_INT_MAX, \
						 NULL)

// #define BT_LE_ADV_NCONN_SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_SCANNABLE, \
// 						 BT_GAP_ADV_SLOW_INT_MIN, \
// 						 BT_GAP_ADV_SLOW_INT_MAX, \
// 						 NULL)

//#define NON_CONNECTABLE_DEVICE_NAME "Athena"


  void bt_ready();  
  //FIXME: maybe just pass structure containg all the data that can be updated :)
 // void ble_beacon_update_adv(uint8_t new_data, uint8_t data_len);                        
void ble_beacon_init(void);
int ble_beacon_update_adv_data(const struct bt_data *adv_data, size_t adv_data_len);

#endif /* BLE_BEACON_H_ */