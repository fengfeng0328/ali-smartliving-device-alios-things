/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#ifndef __VENDOR_H__
#define __VENDOR_H__

#include <aos/aos.h>

typedef enum {
    OFF = 0,
    ON
} eSwitchState;

typedef enum {
    POWER_OFF = 0,
    POWER_ON = 1,
    LAST_STATE
} eRebootState;

#define KV_KEY_SWITCH_STATE "OUTLET_SWITCH_STATE"

#define REBOOT_STATE LAST_STATE
/**
 * @brief product init.
 *
 * @param [in] None.
 *
 * @return None.
 * @see None.
 */
void vendor_product_init(void);
void product_set_switch(bool state);
bool product_get_switch(void);
void product_toggle_led(void);
void product_set_led(bool state);
bool product_get_key(void);

/**
 * @brief set device meta info.
 *
 * @param [in] None.
 * @param [in] None.
 *
 * @return 0:success, -1:failed.
 * @see None.
 */
int set_device_meta_info(void);

/**
 * @brief vendor operation after receiving device_bind event from Cloud.
 *
 * @param [in] None.
 *
 * @return None.
 * @see None.
 */
void vendor_device_bind(void);

/**
 * @brief vendor operation after receiving device_unbind event from Cloud.
 *
 * @param [in] None.
 *
 * @return None.
 * @see None.
 */
void vendor_device_unbind(void);
/**
 * @brief vendor operation after receiving device_reset event from Cloud.
 *
 * @param [in] None.
 *
 * @return None.
 * @see None.
 */
void vendor_device_reset(void);
/**
 * @brief   strart wifi scan over the air once
 *
 * @param[in] cb @n pass ssid info(scan result) to this callback(awss_wifi_scan_result_cb_t) one by one
 * @return 0 for wifi scan is done, otherwise return -1
 * @see awss_wifi_scan_result_cb_t.
 * @note
 *      This API should NOT exit before the invoking for cb is finished.
 *      This rule is something like the following :
 *      HAL_Wifi_Scan() is invoked...
 *      ...
 *      for (ap = first_ap; ap <= last_ap; ap = next_ap){
 *        cb(ap)
 *      }
 *      ...
 *      HAL_Wifi_Scan() exit...
 */
int vendor_wifi_scan(void* cb);

#endif
