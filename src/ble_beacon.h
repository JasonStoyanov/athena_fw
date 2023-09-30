// Creator: Yasen Stoyanov
// Brief: BLE beacon implementation for the Athena project
// Date: 15/9/2023


#ifndef BLE_BEACON_H_
#define BLE_BEACON_H_

#define BEACON_NAME "Athena"
#define BEACON_NAME_LEN (sizeof(BEACON_NAME) - 1)

#define CONNECTABLE_DEVICE_NAME "AthenaCFG"
#define CONNECTABLE_ADV_EN (0)
#define MCUBOOT_DBG  1

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY
 * and 1000ms interval. */
//FIXME: the beacon should not be scannable!!
#define NCONN_USE_IDENTITY 0
#if  (NCONN_USE_IDENTITY == 1)
#define BT_LE_ADV_NCONN_SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 BT_GAP_ADV_SLOW_INT_MAX, \
						 NULL)
#else
#define BT_LE_ADV_NCONN_SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_SCANNABLE, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 BT_GAP_ADV_SLOW_INT_MAX, \
						 NULL)
#endif

/**
 * @brief Initialize the BLE beacon
 * 
 *  It enable the BT stack and create the advertising sets for non-connectable and connectable advertising
 *  It also start the non-connectable advertising
 * 
 * @return Zero on success or (negative) error code otherwise.
 */                  
int ble_beacon_init(void);

/**
 * @brief Update the advertising data of non-connectable advertisement
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int  ble_beacon_update_adv_data(void);


/**
 * @brief Set the advertising data of non-connectable advertisement
 * 
 * It is used to set the pointer to the advertising data that will be updated
 * 
 * @param[in] adv_data Advertising data
 *
 * @return Zero on success or (negative) error code otherwise.
 */
void  ble_beacon_set_adv_frame_data(uint8_t * adv_data);



int ble_beacon_connectable_adv_stop(void);

/**
 * @brief Start BLE connectable advertising
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ble_beacon_connectable_adv_start(void);

#endif /* BLE_BEACON_H_ */