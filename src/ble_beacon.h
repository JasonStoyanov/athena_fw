#ifndef BLE_BEACON_H_
#define BLE_BEACON_H_

#define BEACON_NAME "Athena"
#define BEACON_NAME_LEN (sizeof(BEACON_NAME) - 1)

#define CONNECTABLE_DEVICE_NAME "AthenaCFG"
#define CONNECTABLE_ADV_EN (0)
#define MCUBOOT_DBG  1

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY
 * and 1000ms interval. */
#define BT_LE_ADV_NCONN_SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 BT_GAP_ADV_SLOW_INT_MAX, \
						 NULL)


                     
void ble_beacon_init(void);
int  ble_beacon_update_adv_data(void);
void  ble_beacon_set_adv_frame_data(uint8_t * adv_data);

#endif /* BLE_BEACON_H_ */