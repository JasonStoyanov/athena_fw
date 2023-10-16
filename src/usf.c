//User Setting in Flash (USF) module
//This module is used to store the user settings in flash memory
//The user settings are: the beacon ID 
//The beacon ID is a 1-byte value that is used to identify the beacon
//The beacon ID is stored in flash memory and is loaded at startup
//The beacon ID is written to flash memory when the user changes it
//The beacon ID is used in the advertising data

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include "usf.h"
#include "ble_beacon.h"

static struct nvs_fs fs;

#define NVS_PARTITION		    storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define BEACONID_ID 1
//#define KEY_ID 2

int usf_init(void) {
    int rc = 0;

    struct flash_pages_info info;
    /* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	3 sectors
	 *	starting at NVS_PARTITION_OFFSET
	 */
	fs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device %s is not ready\n", fs.flash_device->name);
		return 0;
	}
	fs.offset = NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info\n");
		return 0;
	}
	fs.sector_size = info.size;
	fs.sector_count = 3U;

	rc = nvs_mount(&fs);
	if (rc) {
		printk("Flash Init failed\n");
		return 0;
	}
}

//Load the user settings from flash
int usf_load(void)
{
	int ret;
    uint8_t beacon_id;

	ret = nvs_read(&fs, BEACONID_ID, &beacon_id, sizeof(beacon_id));
	if (ret > 0) { /* item was found, show it */
	//	printk("Id: %d, Address: %s\n", BEACONID_ID, beacon_id);
		ble_beacon_set_athena_id(beacon_id);
	} else   {/* item was not found*/
		printk("Id: %d, not found\n", BEACONID_ID);
        //TODO: Set the default value for the beacon ID outside this function
	}

}

int usf_write_beacon_id(uint8_t value) {

    (void)nvs_write(&fs, BEACONID_ID, &value,sizeof(value));

}

