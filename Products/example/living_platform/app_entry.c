/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
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
#include "app_entry.h"
#include "aos/kv.h"

#ifdef CSP_LINUXHOST
#include <signal.h>
#endif

#include <k_api.h>

#if defined(OTA_ENABLED) && defined(BUILD_AOS)
#include "ota_service.h"
#endif

static char linkkit_started = 0;
static char awss_running    = 0;

void set_iotx_info();
void do_awss_active();

#ifdef CONFIG_PRINT_HEAP
void print_heap()
{
    extern k_mm_head *g_kmm_head;
    int               free = g_kmm_head->free_size;
    LOG("============free heap size =%d==========", free);
}
#endif

static void wifi_service_event(input_event_t *event, void *priv_data)
{
    if (event->type != EV_WIFI) {
        return;
    }

    if (event->code != CODE_WIFI_ON_GOT_IP) {
        return;
    }

    netmgr_ap_config_t config;
    memset(&config, 0, sizeof(netmgr_ap_config_t));
    netmgr_get_ap_config(&config);
    LOG("wifi_service_event config.ssid %s", config.ssid);
    if (strcmp(config.ssid, "adha") == 0 || strcmp(config.ssid, "aha") == 0) {
        // clear_wifi_ssid();
        return;
    }

    if (!linkkit_started) {
#ifdef CONFIG_PRINT_HEAP
        print_heap();
#endif
        aos_task_new("linkkit", (void (*)(void *))linkkit_main, NULL, 1024 * 8);
        linkkit_started = 1;
    }
}

static void cloud_service_event(input_event_t *event, void *priv_data)
{
    if (event->type != EV_YUNIO) {
        return;
    }

    LOG("cloud_service_event %d", event->code);

    if (event->code == CODE_YUNIO_ON_CONNECTED) {
        LOG("user sub and pub here");
        return;
    }

    if (event->code == CODE_YUNIO_ON_DISCONNECTED) {
    }
}

/*
 * Note:
 * the linkkit_event_monitor must not block and should run to complete fast
 * if user wants to do complex operation with much time,
 * user should post one task to do this, not implement complex operation in
 * linkkit_event_monitor
 */

static void linkkit_event_monitor(int event)
{
    switch (event) {
        case IOTX_AWSS_START: // AWSS start without enbale, just supports device discover
            // operate led to indicate user
            LOG("IOTX_AWSS_START");
            break;
        case IOTX_AWSS_ENABLE: // AWSS enable, AWSS doesn't parse awss packet until AWSS is enabled.
            LOG("IOTX_AWSS_ENABLE");
            // operate led to indicate user
            break;
        case IOTX_AWSS_LOCK_CHAN: // AWSS lock channel(Got AWSS sync packet)
            LOG("IOTX_AWSS_LOCK_CHAN");
            // operate led to indicate user
            break;
        case IOTX_AWSS_PASSWD_ERR: // AWSS decrypt passwd error
            LOG("IOTX_AWSS_PASSWD_ERR");
            // operate led to indicate user
            break;
        case IOTX_AWSS_GOT_SSID_PASSWD:
            LOG("IOTX_AWSS_GOT_SSID_PASSWD");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_ADHA: // AWSS try to connnect adha (device
            // discover, router solution)
            LOG("IOTX_AWSS_CONNECT_ADHA");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_ADHA_FAIL: // AWSS fails to connect adha
            LOG("IOTX_AWSS_CONNECT_ADHA_FAIL");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_AHA: // AWSS try to connect aha (AP solution)
            LOG("IOTX_AWSS_CONNECT_AHA");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_AHA_FAIL: // AWSS fails to connect aha
            LOG("IOTX_AWSS_CONNECT_AHA_FAIL");
            // operate led to indicate user
            break;
        case IOTX_AWSS_SETUP_NOTIFY: // AWSS sends out device setup information
            // (AP and router solution)
            LOG("IOTX_AWSS_SETUP_NOTIFY");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_ROUTER: // AWSS try to connect destination router
            LOG("IOTX_AWSS_CONNECT_ROUTER");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_ROUTER_FAIL: // AWSS fails to connect destination
            // router.
            LOG("IOTX_AWSS_CONNECT_ROUTER_FAIL");
            // operate led to indicate user
            break;
        case IOTX_AWSS_GOT_IP: // AWSS connects destination successfully and got
            // ip address
            LOG("IOTX_AWSS_GOT_IP");
            // operate led to indicate user
            break;
        case IOTX_AWSS_SUC_NOTIFY: // AWSS sends out success notify (AWSS
            // sucess)
            LOG("IOTX_AWSS_SUC_NOTIFY");
            // operate led to indicate user
            break;
        case IOTX_AWSS_BIND_NOTIFY: // AWSS sends out bind notify information to
            // support bind between user and device
            LOG("IOTX_AWSS_BIND_NOTIFY");
            // operate led to indicate user
            break;
        case IOTX_AWSS_ENABLE_TIMEOUT: // AWSS enable timeout
            // user needs to enable awss again to support get ssid & passwd of router
            LOG("IOTX_AWSS_ENALBE_TIMEOUT");
            // operate led to indicate user
            break;
        case IOTX_CONN_CLOUD: // Device try to connect cloud
            LOG("IOTX_CONN_CLOUD");
            // operate led to indicate user
            break;
        case IOTX_CONN_CLOUD_FAIL: // Device fails to connect cloud, refer to
            // net_sockets.h for error code
            LOG("IOTX_CONN_CLOUD_FAIL");
            // operate led to indicate user
            break;
        case IOTX_CONN_CLOUD_SUC: // Device connects cloud successfully
            LOG("IOTX_CONN_CLOUD_SUC");
            // operate led to indicate user
            break;
        case IOTX_RESET: // Linkkit reset success (just got reset response from
            // cloud without any other operation)
            LOG("IOTX_RESET");
            break;
        case IOTX_CONN_REPORT_TOKEN_SUC:
            LOG("---- report token success ----");
            break;
        default:
            break;
    }
}

#ifdef AWSS_BATCH_DEVAP_ENABLE
#define DEV_AP_ZCONFIG_TIMEOUT_MS  120000 // (ms)
extern void awss_set_config_press(uint8_t press);
extern uint8_t awss_get_config_press(void);
extern void zconfig_80211_frame_filter_set(uint8_t filter, uint8_t fix_channel);
void do_awss_dev_ap();

static aos_timer_t dev_ap_zconfig_timeout_timer;
static uint8_t g_dev_ap_zconfig_timer = 0;   // this timer create once and can restart
static uint8_t g_dev_ap_zconfig_run = 0;

static void timer_func_devap_zconfig_timeout(void *arg1, void *arg2)
{
    LOG("%s run\n", __func__);

    if (awss_get_config_press()) {
        // still in zero wifi provision stage, should stop and switch to dev ap
        do_awss_dev_ap();
    } else {
        // zero wifi provision finished
    }

    awss_set_config_press(0);
    zconfig_80211_frame_filter_set(0xFF, 0xFF);
    g_dev_ap_zconfig_run = 0;
    aos_timer_stop(&dev_ap_zconfig_timeout_timer);
}

static void awss_dev_ap_switch_to_zeroconfig(void *p)
{
    LOG("%s run\n", __func__);
    // Stop dev ap wifi provision
    awss_dev_ap_stop();
    // Start and enable zero wifi provision
    iotx_event_regist_cb(linkkit_event_monitor);
    awss_set_config_press(1);

    // Start timer to count duration time of zero provision timeout
    if (!g_dev_ap_zconfig_timer) {
        aos_timer_new(&dev_ap_zconfig_timeout_timer, timer_func_devap_zconfig_timeout, NULL, DEV_AP_ZCONFIG_TIMEOUT_MS, 0);
        g_dev_ap_zconfig_timer = 1;
    }
    aos_timer_start(&dev_ap_zconfig_timeout_timer);

    // This will hold thread, when awss is going
    netmgr_start(true);

    LOG("%s exit\n", __func__);
    aos_task_exit(0);
}

int awss_dev_ap_modeswitch_cb(uint8_t awss_new_mode, uint8_t new_mode_timeout, uint8_t fix_channel)
{
    if ((awss_new_mode == 0) && !g_dev_ap_zconfig_run) {
        g_dev_ap_zconfig_run = 1;
        // Only receive zero provision packets
        zconfig_80211_frame_filter_set(0x00, fix_channel);
        LOG("switch to awssmode %d, mode_timeout %d, chan %d\n", 0x00, new_mode_timeout, fix_channel);
        // switch to zero config
        aos_task_new("devap_to_zeroconfig", awss_dev_ap_switch_to_zeroconfig, NULL, 2048);
    }
}
#endif

static void awss_close_dev_ap(void *p)
{
    awss_dev_ap_stop();
    LOG("%s exit\n", __func__);
    aos_task_exit(0);
}

void awss_open_dev_ap(void *p)
{
    iotx_event_regist_cb(linkkit_event_monitor);
    LOG("%s\n", __func__);
    if (netmgr_start(false) != 0) {
        aos_msleep(2000);
#ifdef AWSS_BATCH_DEVAP_ENABLE
        awss_dev_ap_reg_modeswit_cb(awss_dev_ap_modeswitch_cb);
#endif
        awss_dev_ap_start();
    }
    aos_task_exit(0);
}

static void stop_netmgr(void *p)
{
    awss_stop();
    LOG("%s\n", __func__);
    aos_task_exit(0);
}

static void start_netmgr(void *p)
{
    aos_msleep(2000);
    iotx_event_regist_cb(linkkit_event_monitor);
    netmgr_start(true);
    aos_task_exit(0);
}

void do_awss_active()
{
    LOG("do_awss_active %d\n", awss_running);
    awss_running = 1;
    #ifdef WIFI_PROVISION_ENABLED
    extern int awss_config_press();
    awss_config_press();
    #endif
}

void do_awss_dev_ap()
{
    aos_task_new("netmgr_stop", stop_netmgr, NULL, 4096);
    aos_task_new("dap_open", awss_open_dev_ap, NULL, 4096);
}

void do_awss()
{
    aos_task_new("dap_close", awss_close_dev_ap, NULL, 2048);
    aos_task_new("netmgr_start", start_netmgr, NULL, 5120);
}

static void linkkit_reset(void *p)
{
    netmgr_clear_ap_config();
    HAL_Reboot();
}


extern int  awss_report_reset();
static void do_awss_reset()
{
#ifdef WIFI_PROVISION_ENABLED
#if defined(SUPPORT_ITLS)
    aos_task_new("reset", (void (*)(void *))awss_report_reset, NULL, 6144);  // stack taken by iTLS is more than taken by TLS.
#else
    aos_task_new("reset", (void (*)(void *))awss_report_reset, NULL, 2048);
#endif
#endif
    aos_post_delayed_action(2000, linkkit_reset, NULL);
}

static void do_clear_awss()
{
    aos_post_delayed_action(2000, linkkit_reset, NULL);
}

void linkkit_key_process(input_event_t *eventinfo, void *priv_data)
{
    if (eventinfo->type != EV_KEY) {
        return;
    }
    LOG("awss config press %d\n", eventinfo->value);

    if (eventinfo->code == CODE_BOOT) {
        if (eventinfo->value == VALUE_KEY_CLICK) {

            do_awss_active();
        } else if (eventinfo->value == VALUE_KEY_LTCLICK) {
            do_awss_reset();
        }
    }
}

#ifdef MANUFACT_AP_FIND_ENABLE
void manufact_ap_find_process(int result)
{
    // Informed manufact ap found or not.
    // If manufact ap found, lower layer will auto connect the manufact ap
    // IF manufact ap not found, lower layer will enter normal awss state
    if (result == 0) {
        LOG("%s ap found.\n", __func__);
    } else {
        LOG("%s ap not found.\n", __func__);
    }
}
#endif

#ifdef CONFIG_AOS_CLI
static void handle_reset_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    aos_schedule_call(do_awss_reset, NULL);
}

static void handle_active_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    aos_schedule_call(do_awss_active, NULL);
}

static void handle_dev_ap_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    aos_schedule_call(do_awss_dev_ap, NULL);
}

static void handle_awss_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    aos_schedule_call(do_awss, NULL);
}

static void handle_clear_awss_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    aos_schedule_call(do_clear_awss, NULL);
}

static void handle_linkkey_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    if (argc == 1)
    {
        int len = 0;
        char product_key[PRODUCT_KEY_LEN + 1] = {0};
        char product_secret[PRODUCT_SECRET_LEN + 1] = {0};
        char device_name[DEVICE_NAME_LEN + 1] = {0};
        char device_secret[DEVICE_SECRET_LEN + 1] = { 0 };

        len = PRODUCT_KEY_LEN+1;
        aos_kv_get("linkkit_product_key", product_key, &len);

        len = PRODUCT_SECRET_LEN+1;
        aos_kv_get("linkkit_product_secret", product_secret, &len);

        len = DEVICE_NAME_LEN+1;
        aos_kv_get("linkkit_device_name", device_name, &len);

        len = DEVICE_SECRET_LEN+1;
        aos_kv_get("linkkit_device_secret", device_secret, &len);

        aos_cli_printf("Product Key=%s.\r\n", product_key);
        aos_cli_printf("Device Name=%s.\r\n", device_name);
        aos_cli_printf("Device Secret=%s.\r\n", device_secret);
        aos_cli_printf("Product Secret=%s.\r\n", product_secret);
    }
    else if (argc == 5)
    {
        aos_kv_set("linkkit_product_key", argv[1], strlen(argv[1]) + 1, 1);
        aos_kv_set("linkkit_device_name", argv[2], strlen(argv[2]) + 1, 1);
        aos_kv_set("linkkit_device_secret", argv[3], strlen(argv[3]) + 1, 1);
        aos_kv_set("linkkit_product_secret", argv[4], strlen(argv[4]) + 1, 1);

        aos_cli_printf("Done");
    }
    else
    {
        aos_cli_printf("Error: %d\r\n", __LINE__);
        return;
    }
}

#ifdef DEV_ERRCODE_ENABLE
#define DEV_DIAG_CLI_SET_ERRCODE                "set_err"
#define DEV_DIAG_CLI_ERRCODE_MANUAL_MSG         "errcode was manually set"
#define DEV_DIAG_CLI_GET_ERRCODE                "get_err"
#define DEV_DIAG_CLI_DEL_ERRCODE                "del_err"

static void handle_dev_serv_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    uint16_t err_code = 0;
    char err_msg[DEV_ERRCODE_MSG_MAX_LEN] = {0};
    uint8_t ret = 0;
    if (argc == 1)
    {
        aos_cli_printf("device diagnosis service test, please input more parameters.\r\n");
    }
    else if (argc > 1)
    {
        if ((argc == 3) && (!strcmp(argv[1], DEV_DIAG_CLI_SET_ERRCODE))) {
            err_code = (uint16_t)strtoul(argv[2], NULL, 0);
            ret = dev_errcode_kv_set(err_code, DEV_DIAG_CLI_ERRCODE_MANUAL_MSG);
            if (ret ==0) {
                aos_cli_printf("manually set err_code=%d\r\n", err_code);
            } else {
                aos_cli_printf("manually set err_code failed!\r\n");
            }
        } else if ((argc == 2) && (!strcmp(argv[1], DEV_DIAG_CLI_GET_ERRCODE))) {
            ret = dev_errcode_kv_get(&err_code, err_msg);
            if (ret == 0) {
                aos_cli_printf("get err_code=%d, err_msg=%s\r\n", err_code, err_msg);
            } else {
                aos_cli_printf("get err_code fail or no err_code found!\r\n");
            }
        } else if ((argc == 2) && (!strcmp(argv[1], DEV_DIAG_CLI_DEL_ERRCODE))) {
            dev_errcode_kv_del();
            aos_cli_printf("del err_code\r\n");
        }
    }
    else
    {
        aos_cli_printf("Error: %d\r\n", __LINE__);
        return;
    }
}
#endif

#ifdef MANUFACT_AP_FIND_ENABLE

#define MANUAP_CONTROL_KEY                      "manuap"
#define MANUAP_CLI_OPEN                         "open"
#define MANUAP_CLI_CLOSE                        "close"

static void handle_manuap_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    int ret = 0;
    uint8_t manuap_flag = 0;
    int manu_flag_len = sizeof(manuap_flag);
    if (argc == 1)
    {
        ret = aos_kv_get(MANUAP_CONTROL_KEY, &manuap_flag, &manu_flag_len);
        if (ret == 0) {
            if (manuap_flag) {
                aos_cli_printf("manuap state is open\r\n");
            } else {
                aos_cli_printf("manuap state is close\r\n");
            }
        } else {
            aos_cli_printf("no manuap state stored\r\n");
        }
    }
    else if (argc == 2)
    {
        if (!strcmp(argv[1], MANUAP_CLI_OPEN)) {
            manuap_flag = 1;
            ret = aos_kv_set(MANUAP_CONTROL_KEY, &manuap_flag, sizeof(manuap_flag), 1);
            if (ret == 0) {
                aos_cli_printf("manuap opened\r\n");
            }
        } else if (!strcmp(argv[1], MANUAP_CLI_CLOSE)) {
            manuap_flag = 0;
            ret = aos_kv_set(MANUAP_CONTROL_KEY, &manuap_flag, sizeof(manuap_flag), 1);
            if (ret == 0) {
                aos_cli_printf("manuap closed\r\n");
            }
        } else {
            aos_cli_printf("param input err\r\n");
        }
    }
    else
    {
        aos_cli_printf("param input err\r\n");
        return;
    }
}
#endif

static struct cli_command resetcmd = { .name     = "reset",
                                       .help     = "factory reset",
                                       .function = handle_reset_cmd };

static struct cli_command awss_enable_cmd = { .name     = "active_awss",
                                              .help     = "active_awss [start]",
                                              .function = handle_active_cmd };
static struct cli_command awss_dev_ap_cmd = { .name     = "dev_ap",
                                              .help     = "awss_dev_ap [start]",
                                              .function = handle_dev_ap_cmd };
static struct cli_command awss_cmd = { .name     = "awss",
                                       .help     = "awss [start]",
                                       .function = handle_awss_cmd };
static struct cli_command awss_clear_cmd = { .name     = "clear_awss",
                                       .help     = "clear_awss [start]",
                                       .function = handle_clear_awss_cmd };
static struct cli_command linkkeycmd = { .name     = "linkkey",
                                   .help     = "set/get linkkit keys. linkkey [<Product Key> <Device Name> <Device Secret> <Product Secret>]",
                                   .function = handle_linkkey_cmd };
#ifdef DEV_ERRCODE_ENABLE
static struct cli_command dev_service_cmd = { .name     = "dev_serv",
                                   .help     = "device diagnosis service tests. like errcode, and log report",
                                   .function = handle_dev_serv_cmd };
#endif
#ifdef MANUFACT_AP_FIND_ENABLE
static struct cli_command manuap_cmd = { .name     = "manuap",
                                   .help     = "manuap [open] or manuap [close]",
                                   .function = handle_manuap_cmd };
#endif
#endif

#ifdef CONFIG_PRINT_HEAP
static void duration_work(void *p)
{
    print_heap();
    aos_post_delayed_action(5000, duration_work, NULL);
}
#endif

#ifdef POWER_CYCLE_AWSS

#ifndef DEFAULT_CYCLE_TIMEOUT
#define DEFAULT_CYCLE_TIMEOUT	5000
#endif
static int network_start(void)
{
    netmgr_ap_config_t my_config={0};
    netmgr_get_ap_config(&my_config);
    if(strlen(my_config.ssid)) {
        aos_task_new("netmgr_start", start_netmgr, NULL, 5120);
    } else {
        printf("INFO: %s no valied ap confing\r\n", __func__);
    }
    return 0;
}

static int check_and_start_awss(void)
{
    int ret, len = 0;
    int awss_type = 0;	/* 0 -- unknown, 1 -- dev_ap, 2 -- one key */
    char value_string[8];
    netmgr_ap_config_t my_config={0};

    memset(value_string, 0, sizeof(value_string));
    len = sizeof(value_string);
    /* check awss type */
    ret = aos_kv_get("awss_type", value_string, &len);
    if(0 == strncmp("dev_ap", value_string, strlen("dev_ap"))) {
        printf("INFO: %s awss type: device ap\r\n", __func__);
        do_awss_dev_ap();
        awss_type = 1;
    } else if(0 == strncmp("one_key", value_string, strlen("dev_ap"))) {
        printf("INFO: %s awss type: one key\r\n", __func__);
        do_awss();
        do_awss_active();
        awss_type = 2;
    } else {
        printf("WARN: %s awss type: unknown\r\n", __func__);
        awss_type = 0;
    }
    /* Clesr awss type */
    ret = aos_kv_del("awss_type");
    return awss_type;
}

static int power_cycle_check(void *p)
{
    int ret = -1;
    int count;

    if(NULL == p){
        printf("ERR: %s data pointer is NULL\r\n", __func__);
        return ret;
	}
    count = *(int*)p;
    printf("INFO: %s power cycle %d timeout clear\r\n", __func__, count);
    /* Reset cycle count */
    ret = aos_kv_set("power_cycle_count", "1", strlen("1") + 1, 1);
    if(0 != ret) {
        printf("ERR: %s aos_kv_set return: %d\r\n", __func__, ret);
        return ret;
    }
    if(count == 3){	/* Three times for device ap awss */
        ret = aos_kv_set("awss_type", "dev_ap", strlen("dev_ap") + 1, 1);
        printf("INFO: %s prepare for device soft ap awss\r\n", __func__);
        aos_schedule_call(do_awss_reset, NULL);
    } else if(count == 5) {	/* Five times for one key awss */
        aos_kv_set("awss_type", "one_key", strlen("one_key") + 1, 1);
        printf("INFO: %s prepare for smart config awss\r\n", __func__);
        aos_schedule_call(do_awss_reset, NULL);
    }
    return ret;
}

static int power_cycle_awss(void)
{
    int ret, len = 0;
    int awss_type = 0;
    int cycle_count = 0;
    char value_string[8];

    memset(value_string, 0, sizeof(value_string));
    len = sizeof(value_string);

    /* read stored cycle value */
    ret = aos_kv_get("power_cycle_count", value_string, &len);
    if(0 != ret) {
        printf("INFO: %s init power cycle count\r\n", __func__);
        snprintf(value_string, sizeof(value_string), "%d", 1);	/* Set initial value */
        ret = aos_kv_set("power_cycle_count", value_string, strlen(value_string), 1);
        if(0 != ret) {
            printf("ERR: %s, aos_kv_set return: %d\r\n", __func__, ret);
            return ret;
        }
    } else {
        cycle_count = atoi(value_string);
        printf("INFO: %s power cycle count: %d\r\n", __func__, cycle_count);
        memset(value_string, 0, sizeof(value_string));
        len = sizeof(value_string);
        if(cycle_count == 1) {	/* AWSS always start after a timeout clear reboot */
            awss_type = check_and_start_awss();
            snprintf(value_string, sizeof(value_string), "%d", 1);
	    }
        if(cycle_count > 4) {	/* Cycle count max value is 5 */
            snprintf(value_string, sizeof(value_string), "%d", 1);
        } else {	/* Increase cycle count */
            snprintf(value_string, sizeof(value_string), "%d", cycle_count + 1);
        }
        if(0 == awss_type) {	/* Do not store cycle count when doing awss */
            ret = aos_kv_set("power_cycle_count", value_string, strlen(value_string) + 1, 1);
            aos_post_delayed_action(DEFAULT_CYCLE_TIMEOUT, power_cycle_check, (void*)(&cycle_count));
        }
        return ret;
    }
}
#endif

static int ota_init(void);
static ota_service_t ctx = {0};
static int mqtt_connected_event_handler(void)
{
#if defined(OTA_ENABLED) && defined(BUILD_AOS)
    static bool ota_service_inited = false;

    if (ota_service_inited == true) {
        int ret = 0;

        LOG("MQTT reconnected, let's redo OTA upgrade");
        if ((ctx.h_tr) && (ctx.h_tr->upgrade)) {
            LOG("Redoing OTA upgrade");
            ret = ctx.h_tr->upgrade(&ctx);
            if (ret < 0) LOG("Failed to do OTA upgrade");
        }

        return ret;
    }

    LOG("MQTT Construct  OTA start to inform");
#ifdef DEV_OFFLINE_OTA_ENABLE
    ota_service_inform(&ctx);
#else
    ota_init();
#endif
    ota_service_inited = true;
#endif
    return 0;
}

static int ota_init(void)
{
    char product_key[PRODUCT_KEY_LEN + 1] = {0};
    char device_name[DEVICE_NAME_LEN + 1] = {0};
    char device_secret[DEVICE_SECRET_LEN + 1] = {0};
    HAL_GetProductKey(product_key);
    HAL_GetDeviceName(device_name);
    HAL_GetDeviceSecret(device_secret);
    memset(&ctx, 0, sizeof(ota_service_t));
    strncpy(ctx.pk, product_key, sizeof(ctx.pk)-1);
    strncpy(ctx.dn, device_name, sizeof(ctx.dn)-1);
    strncpy(ctx.ds, device_secret, sizeof(ctx.ds)-1);
    ctx.trans_protcol = 0;
    ctx.dl_protcol = 3;
    ota_service_init(&ctx);
    return 0;
}

static void show_firmware_version(void)
{
    printf("\n--------Firmware info--------");
    printf("\napp: %s,  board: %s", APP_NAME, PLATFORM);
#ifdef DEBUG
    printf("\nHost: %s", COMPILE_HOST);
#endif
    printf("\nBranch: %s", GIT_BRANCH);
    printf("\nHash: %s", GIT_HASH);
    printf("\nDate: %s %s", __DATE__, __TIME__);
    printf("\nKernel: %s", aos_get_kernel_version());
    printf("\nLinkKit: %s", LINKKIT_VERSION);
    printf("\nAPP ver: %s", aos_get_app_version());

#ifdef MQTT_DIRECT
    printf("\nMQTT direct: on");
#else
    printf("\nMQTT direct: off");
#endif

    printf("\nRegion env: %s\n\n", REGION_ENV_STRING);
}

int application_start(int argc, char **argv)
{
#ifdef CONFIG_PRINT_HEAP
    print_heap();
    aos_post_delayed_action(5000, duration_work, NULL);
#endif

#ifdef CSP_LINUXHOST
    signal(SIGPIPE, SIG_IGN);
#endif

#ifdef WITH_SAL
    sal_init();
#endif

#ifdef MDAL_MAL_ICA_TEST
    HAL_MDAL_MAL_Init();
#endif

#ifdef DEFAULT_LOG_LEVEL_DEBUG
    IOT_SetLogLevel(IOT_LOG_DEBUG);
#else
    IOT_SetLogLevel(IOT_LOG_INFO);
#endif

    show_firmware_version();
    set_iotx_info();
    dev_diagnosis_module_init();
#ifdef DEV_OFFLINE_OTA_ENABLE
    ota_init();
#endif
    netmgr_init();
#ifdef MANUFACT_AP_FIND_ENABLE
    {
        uint8_t m_manu_flag = 0;
        int m_manu_flag_len = sizeof(m_manu_flag);
        aos_kv_get(MANUAP_CONTROL_KEY, &m_manu_flag, &m_manu_flag_len);
        if (m_manu_flag) {
            netmgr_manuap_info_set("TEST_AP", "TEST_PASSWORD", manufact_ap_find_process);
        }
    }
#endif
    aos_register_event_filter(EV_KEY, linkkit_key_process, NULL);
    aos_register_event_filter(EV_WIFI, wifi_service_event, NULL);
    aos_register_event_filter(EV_YUNIO, cloud_service_event, NULL);
    IOT_RegisterCallback(ITE_MQTT_CONNECT_SUCC,mqtt_connected_event_handler);

#ifdef CONFIG_AOS_CLI
    aos_cli_register_command(&resetcmd);
    aos_cli_register_command(&awss_enable_cmd);
    aos_cli_register_command(&awss_dev_ap_cmd);
    aos_cli_register_command(&awss_cmd);
    aos_cli_register_command(&awss_clear_cmd);
    aos_cli_register_command(&linkkeycmd);
#ifdef DEV_ERRCODE_ENABLE
    aos_cli_register_command(&dev_service_cmd);
#endif
#ifdef MANUFACT_AP_FIND_ENABLE
    aos_cli_register_command(&manuap_cmd);
#endif
#endif

#ifdef POWER_CYCLE_AWSS
    network_start();
    power_cycle_awss();
#else
    aos_task_new("netmgr_start", start_netmgr, NULL, 5120);
#endif

    aos_loop_run();

    return 0;
}
