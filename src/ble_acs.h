#ifndef BLE_ACS_H_
#define BLE_ACS_H_

#include <zephyr/types.h>

/* Define the 128-bit UUIDs for the GATT service and its characteristics. */
#define BT_UUID_ACS_VAL \
	BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)
/** @brief Beacon Custom ID Characteristic UUID. */
#define BT_UUID_ACS_ID_VAL \
	BT_UUID_128_ENCODE(0x00001524, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_ACS           BT_UUID_DECLARE_128(BT_UUID_ACS_VAL)
#define BT_UUID_ACS_ID    BT_UUID_DECLARE_128(BT_UUID_ACS_ID_VAL)

/** @brief Callback type for when an LED state change is received. */
typedef void (*id_cb_t)(const bool led_state);

/** @brief Callback struct used by the LBS Service. */
struct ble_acs_cb {
	/** ID state change callback. */
	id_cb_t id_cb;
};

/** @brief Initialize the ACS Service.
 *
 * This function registers application callback functions with the Ahena configuration
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int ble_acs_init(struct ble_acs_cb *callbacks);

#endif /* BLE_ACS_H_ */