/*
 *copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <aos/aos.h>
#include <aos/yloop.h>
#include "netmgr.h"
#include "iot_export.h"
#include "iot_import.h"
#include "vendor.h"
#include "device_state_manger.h"
#include "hal/wifi.h"
#include "factory.h"

#define FACTORY_TEST_AP_SSID "YOUR_SSID"
#define FACTORY_TEST_AP_PSW  "YOUR_PASSWORD"

static bool isfoundAP = false;
static int  wifi_power = 0;

static int wifi_scan_result_cb(
            const char ssid[HAL_MAX_SSID_LEN],
            const uint8_t bssid[ETH_ALEN],
            enum AWSS_AUTH_TYPE auth,
            enum AWSS_ENC_TYPE encry,
            uint8_t channel, signed char rssi,
            int is_last_ap)
{
    unsigned char APTEST[] = FACTORY_TEST_AP_SSID;
    if (strncmp(APTEST, ssid, sizeof(APTEST)) == 0) {
        LOG("[FACTORY]find factory wifi:%s", APTEST);
        isfoundAP = true;
        wifi_power = rssi;
    }
    LOG("[FACTORY]info->ssid = %s, rssi=%d", ssid,rssi);
    return 0;
}

int scan_factory_ap(void)
{
    /* scan wifi */
    vendor_wifi_scan(wifi_scan_result_cb);

    aos_msleep(3000);

    LOG("[FACTORY]scan factory AP result = %d", isfoundAP);

    /* check if enter factory mode */
    if (isfoundAP == true) {
        enter_factory_mode();
        return 0;
    } else {
        return -1;
    }
}

int enter_factory_mode(void)
{
    #ifdef WIFI_PROVISION_ENABLED
    extern int awss_stop(void);
    awss_stop();
    #endif

    /* factory: Don't connect AP */
    #if 0
    netmgr_ap_config_t config;
    strncpy(config.ssid, FACTORY_TEST_AP_SSID, sizeof(config.ssid) - 1);
    strncpy(config.pwd, FACTORY_TEST_AP_PSW, sizeof(config.pwd) - 1);
    netmgr_set_ap_config(&config);
    netmgr_start(false);
    #endif

    /* RSSI > -60dBm */
    if (wifi_power < -60) {
        LOG("[FACTORY]factory AP power < -60dbm");
        set_net_state(FACTORY_FAILED_1);
    } else {
        LOG("[FACTORY]meter calibrate begin");
        set_net_state(FACTORY_BEGIN);
    }

    return 0;
}

int exit_factory_mode()
{
    //need to reboot
}

