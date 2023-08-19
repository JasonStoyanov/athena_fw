#ifndef BLE_BEACON_H_
#define BLE_BEACON_H_

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define MCUBOOT_DBG  1

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY
 * and 1000ms interval. */
#define BT_LE_ADV_NCONN_SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 BT_GAP_ADV_SLOW_INT_MAX, \
						 NULL)




  void bt_ready();  
  //FIXME: maybe just pass structure containg all the data that can be updated :)
  void ble_beacon_update_adv(uint8_t new_data, uint8_t data_len);                        


#endif /* BLE_BEACON_H_ */