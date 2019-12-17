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
#include "factory.h"

#define AWSS_TIMEOUT_MS              (10*60*1000)
#define CONNECT_AP_FAILED_TIMEOUT_MS (2*60*1000)

static aos_timer_t awss_timeout_timer;
static aos_timer_t connect_ap_failed_timeout_timer;

static int device_net_state = UNKNOW_STATE;
int get_net_state(void)
{
    return device_net_state;
}

void set_net_state(int state)
{
    device_net_state = state;
}

static void device_stop_awss(void)
{
    awss_dev_ap_stop();
    awss_stop();
}

static void timer_func_awss_timeout(void *arg1, void *arg2)
{
    LOG("awss timeout, stop awss");
    set_net_state(AWSS_NOT_START);
    device_stop_awss();
    aos_timer_stop(&awss_timeout_timer);
}

static void timer_func_connect_ap_failed_timeout(void *arg1, void *arg2)
{
    LOG("connect ap failed timeout");
    set_net_state(CONNECT_AP_FAILED_TIMEOUT);
    aos_timer_stop(&connect_ap_failed_timeout_timer);
}

extern void do_awss_reset();
void key_detect_event_task(void)
{
    int press_key_cnt = 0;

    while (1) {
        if (!product_get_key()) {
            press_key_cnt++;
            LOG("press_key_cnt :%d", press_key_cnt);
        } else {
            if ((press_key_cnt >= 2) && (press_key_cnt < 100)) {
                if (product_get_switch() == ON) {
                    product_set_switch(OFF);
                    user_post_powerstate(OFF);
                } else {
                    product_set_switch(ON);
                    user_post_powerstate(ON);
                }
                press_key_cnt = 0;
            }
        }
        if (press_key_cnt >= 50) {
            do_awss_reset();
            break;
        }
        aos_msleep(100);
    }
}

static void indicate_net_state_task(void)
{
    uint32_t nCount = 0;
    uint32_t duration = 0;
    int pre_state = UNKNOW_STATE;
    int cur_state = UNKNOW_STATE;
    int switch_stat = 0;

    while (1) {
        pre_state = cur_state;
        cur_state = get_net_state();
        switch (cur_state) {
            case UNCONFIGED:
                nCount++;
                if (nCount >= 2) {
                    product_toggle_led();
                    nCount = 0;
                }
                break;

            case AWSS_NOT_START:
                if (pre_state != cur_state) {
                    switch_stat = (int)product_get_switch();
                    LOG("[net_state]awss timeout, set led %d", switch_stat);
                    product_set_led(switch_stat);
                }
                break;

            case GOT_AP_SSID:
            case CONNECT_CLOUD_FAILED:
                nCount++;
                if (nCount >= 8) {
                    product_toggle_led();
                    nCount = 0;
                }
                break;

            case CONNECT_AP_FAILED_TIMEOUT:
                if (pre_state != cur_state) {
                    switch_stat = (int)product_get_switch();
                    LOG("[net_state]connect AP failed timeout, set led %d", switch_stat);
                    product_set_led(switch_stat);
                }
                break;

            case CONNECT_AP_FAILED:
                nCount++;
                if (nCount >= 5) {
                    product_toggle_led();
                    nCount = 0;
                }
                if (pre_state != cur_state) {
                    LOG("[net_state]connect AP failed");
                    aos_timer_new(&connect_ap_failed_timeout_timer, timer_func_connect_ap_failed_timeout, NULL, CONNECT_AP_FAILED_TIMEOUT_MS, 0);
                    aos_timer_start(&connect_ap_failed_timeout_timer);
                    device_stop_awss();
                }
                break;

            case CONNECT_CLOUD_SUCCESS:
                if (pre_state != cur_state) {
                    aos_timer_stop(&awss_timeout_timer);

                    switch_stat = (int)product_get_switch();
                    LOG("[net_state]connect cloud success, set led %d", switch_stat);
                    product_set_led(switch_stat);
                }
                break;

            case APP_BIND_SUCCESS:
                if (pre_state != cur_state) {
                    set_net_state(CONNECT_CLOUD_SUCCESS);
                }
                break;

            case FACTORY_BEGIN:
                LOG("[net_state]factory begin");
                set_net_state(FACTORY_SUCCESS);
                break;

            case FACTORY_SUCCESS:
                if (pre_state != cur_state) {
                    LOG("[net_state]factory success, set led OFF");
                    product_set_led(OFF);
                }
                break;

            case FACTORY_FAILED_1:
                nCount++;
                if (nCount >= 5) {
                    product_toggle_led();
                    nCount = 0;
                }
                break;

            case FACTORY_FAILED_2:
                nCount++;
                if (nCount >= 2) {
                    product_toggle_led();
                    nCount = 0;
                }
                break;

            default:
                break;
        }
        aos_msleep(100);
    }

    LOG("exit quick_light mode");
    aos_task_exit(0);
}

extern void do_awss_dev_ap();
extern void do_awss();
static void app_start_wifi_awss(void)
{
    aos_timer_new(&awss_timeout_timer, timer_func_awss_timeout, NULL, AWSS_TIMEOUT_MS, 0);
    aos_timer_start(&awss_timeout_timer);

#if defined(AWSS_SUPPORT_DEV_AP)
    LOG("app_start_wifi_awss:softAP");
    do_awss_dev_ap();
#else
    LOG("app_start_wifi_awss:onekey");
    do_awss();
#endif
}

void check_factory_mode(void)
{
    int ret = 0;
    netmgr_ap_config_t ap_config;
    memset(&ap_config, 0, sizeof(netmgr_ap_config_t));

    aos_task_new("indicate net state", indicate_net_state_task, NULL, 4096);

    netmgr_get_ap_config(&ap_config);
    if (strlen(ap_config.ssid) <= 0) {
        LOG("scan factory ap, set led ON");
        product_set_led(ON);

        ret = scan_factory_ap();
        if (0 != ret) {
            set_net_state(UNCONFIGED);
            awss_config_press();

            LOG("not enter factory mode, start awss");
            app_start_wifi_awss();
        }
    } else {
        set_net_state(GOT_AP_SSID);

        LOG("start awss");
        app_start_wifi_awss();
    }
}


