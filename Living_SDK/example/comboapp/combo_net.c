/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "aos/kernel.h"
#include <aos/yloop.h>
#include "netmgr.h"

#include <breeze_export.h>
#include "iot_export.h"
#include "iot_import.h"

#define HAL_IEEE80211_IS_BEACON        ieee80211_is_beacon
#define HAL_IEEE80211_IS_PROBE_RESP    ieee80211_is_probe_resp
#define HAL_IEEE80211_GET_SSID         ieee80211_get_ssid
#define HAL_IEEE80211_GET_BSSID        ieee80211_get_bssid
#define HAL_AWSS_OPEN_MONITOR          HAL_Awss_Open_Monitor
#define HAL_AWSS_CLOSE_MONITOR         HAL_Awss_Close_Monitor
#define HAL_AWSS_SWITCH_CHANNEL        HAL_Awss_Switch_Channel

struct ieee80211_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];
    uint8_t addr2[ETH_ALEN];
    uint8_t addr3[ETH_ALEN];
    uint16_t seq_ctrl;
    uint8_t addr4[ETH_ALEN];
};

extern char awss_running;
static breeze_apinfo_t apinfo;
static char monitor_got_bssid = 0;
static netmgr_ap_config_t config;

/* Device info(five elements) will be used by ble breeze */
/* ProductKey, ProductSecret, DeviceName, DeviceSecret, ProductID */
char g_combo_pk[PRODUCT_KEY_LEN + 1] = {0};
char g_combo_ps[PRODUCT_SECRET_LEN + 1] = {0};
char g_combo_dn[DEVICE_NAME_LEN + 1] = {0};
char g_combo_ds[DEVICE_SECRET_LEN + 1] = {0};
uint32_t g_combo_pid = 0;

static void apinfo_ready_handler(breeze_apinfo_t *ap)
{
    if (!ap)
        return;

    memcpy(&apinfo, ap, sizeof(apinfo));
    aos_post_event(EV_BZ_COMBO, CODE_COMBO_AP_INFO_READY, (unsigned long)&apinfo);
}

int simple_80211_frame_handler(char *buf, int length, enum AWSS_LINK_TYPE link_type, int with_fcs, signed char rssi)
{
    uint8_t ssid[MAX_SSID_LEN] = {0}, bssid[ETH_ALEN] = {0};
    struct ieee80211_hdr *hdr;
    int fc;
    int ret = -1;

    if (link_type != AWSS_LINK_TYPE_NONE) {
        return -1;
    }
    hdr = (struct ieee80211_hdr *)buf;

    fc = hdr->frame_control;
    if(!HAL_IEEE80211_IS_BEACON(fc) && !HAL_IEEE80211_IS_PROBE_RESP(fc)){
        return -1;
    }
    ret = HAL_IEEE80211_GET_BSSID(hdr, bssid);
    if(ret < 0) {
        return -1;
    }
    if(memcmp(config.bssid, bssid, ETH_ALEN) != 0){
        return -1;
    }
    ret = HAL_IEEE80211_GET_SSID(hdr, length, ssid);
    if(ret < 0){
        return -1;
    }
    monitor_got_bssid = 1;
    strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
    HAL_AWSS_CLOSE_MONITOR();
    return 0;
}

static char is_str_asii(char* str)
{
    for(uint32_t i = 0; str[i] != '\0'; ++i){
        if((uint8_t)str[i] > 0x7F){
            return 0;
        }
    }
    return 1;
}

static void linkkit_work(breeze_apinfo_t *info)
{
    uint64_t pre_chn_time = HAL_UptimeMs();
    memset(&config, 0, sizeof(netmgr_ap_config_t));
    if (!info)
        return;

    strncpy(config.ssid, info->ssid, sizeof(config.ssid) - 1);
    strncpy(config.pwd, info->pw, sizeof(config.pwd) - 1);

    if(info->apptoken_len > 0){
        memcpy(config.bssid, info->bssid, ETH_ALEN);
        awss_set_token(info->apptoken);
        if(!is_str_asii(config.ssid)){
            printf("SSID not asicci encode\n");
            HAL_AWSS_OPEN_MONITOR(simple_80211_frame_handler);
            while(1){
                aos_msleep(50);
                if(monitor_got_bssid){
                    break;
                }
                uint64_t cur_chn_time = HAL_UptimeMs();
                if(cur_chn_time - pre_chn_time > 250){
                    int channel = aws_next_channel();
                    HAL_AWSS_SWITCH_CHANNEL(channel, 0, NULL);
                    pre_chn_time = cur_chn_time;
                }
            }
        }
    }
    netmgr_set_ap_config(&config);
    hal_wifi_suspend_station(NULL);
    printf("aos_awss Will connect WiFi AP: %s", config.ssid);
    netmgr_reconnect_wifi();
}

static void combo_service_event(input_event_t *event, void *priv_data)
{
    if (event->type != EV_BZ_COMBO) {
        return;
    }

    printf("%s %d", __func__, event->code);

    awss_running = 1;

    if (event->code == CODE_COMBO_AP_INFO_READY) {
        linkkit_work((breeze_apinfo_t *)event->value);
    }
}

int combo_net_init()
{
    breeze_dev_info_t dinfo = { 0 };
    aos_register_event_filter(EV_BZ_COMBO, combo_service_event, NULL);
    if ((strlen(g_combo_pk) > 0) && (strlen(g_combo_ps) > 0) \
    && (strlen(g_combo_dn) > 0) && (strlen(g_combo_ds) > 0) && g_combo_pid > 0) {
        dinfo.product_id = g_combo_pid;
        dinfo.product_key = g_combo_pk;
        dinfo.product_secret = g_combo_ps;
        dinfo.device_name = g_combo_dn;
        dinfo.device_secret = g_combo_ds;
        breeze_awss_init(apinfo_ready_handler, &dinfo);
        breeze_awss_start();
    } else {
        printf("combo device info not set!");
    }
    return 0;
}

int combo_net_deinit()
{
    breeze_awss_stop();
    return 0;
}

