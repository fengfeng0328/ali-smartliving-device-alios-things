/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#include "stdio.h"
#include "iot_export_linkkit.h"
#include "cJSON.h"
#include "app_entry.h"
#include "aos/kv.h"
#ifdef AOS_TIMER_SERVICE
#include "iot_export_timer.h"
#endif
#include "smart_outlet.h"
#include "vendor.h"
#include "device_state_manger.h"

#define USER_EXAMPLE_YIELD_TIMEOUT_MS (30)

#ifdef AOS_TIMER_SERVICE
    #define NUM_OF_PROPERTYS 1 /* <=30 dont add timer property */
    const char *control_targets_list[NUM_OF_PROPERTYS] = {"PowerSwitch"};
    static int num_of_tsl_type[NUM_OF_TSL_TYPES] = {1, 0, 0}; /* 1:int/enum/bool; 2:float/double; 3:text/date */
    #define NUM_OF_COUNTDOWN_LIST_TARGET 1  /* <=10 */
    const char *countdownlist_target_list[NUM_OF_COUNTDOWN_LIST_TARGET] = {"PowerSwitch"};
    #define NUM_OF_LOCAL_TIMER_TARGET 1  /* <=5 */
    const char *localtimer_target_list[NUM_OF_LOCAL_TIMER_TARGET] = {"PowerSwitch"};
#endif

static int powerswitch = 1;
static user_example_ctx_t g_user_example_ctx;

user_example_ctx_t *user_example_get_ctx(void)
{
    return &g_user_example_ctx;
}

void *example_malloc(size_t size)
{
    return HAL_Malloc(size);
}

void example_free(void *ptr)
{
    HAL_Free(ptr);
}

void update_power_state(int state)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    user_example_ctx->power_switch = state;
}

static void user_deviceinfo_update(void)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *device_info_update = "[{\"attrKey\":\"OutletFWVersion\",\"attrValue\":\"1.3.1\"}]";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_UPDATE,
                             (unsigned char *)device_info_update, strlen(device_info_update));
    LOG_TRACE("Device Info Update Message ID: %d", res);
}

void user_post_property_after_connected(void);

static int user_connected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    LOG_TRACE("Cloud Connected");

    user_example_ctx->cloud_connected = 1;

    set_net_state(CONNECT_CLOUD_SUCCESS);
    user_post_property_after_connected();

    user_deviceinfo_update();
    return 0;
}

static int user_disconnected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    LOG_TRACE("Cloud Disconnected");

    set_net_state(CONNECT_CLOUD_FAILED);
    user_example_ctx->cloud_connected = 0;

    return 0;
}

static int user_service_request_event_handler(const int devid, const char *serviceid, const int serviceid_len,
        const char *request, const int request_len,
        char **response, int *response_len)
{
    LOG_TRACE("Service Request Received, Devid: %d, Service ID: %.*s, Payload: %s", devid, serviceid_len,
                  serviceid,
                  request);

    return 0;
}

static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    cJSON *root = NULL, *item = NULL;

    LOG_TRACE("Property Set Received, Devid: %d, Request: %s", devid, request);
    if ((root = cJSON_Parse(request)) == NULL) {
        LOG_TRACE("property set payload is not JSON format");
        return -1;
    }

    if ((item = cJSON_GetObjectItem(root, "PowerSwitch")) != NULL && cJSON_IsNumber(item)) {
        if (item->valueint == 1) {
            product_set_switch(ON);
        } else {
            product_set_switch(OFF);
        }
#ifdef AOS_TIMER_SERVICE
    } else {
        timer_service_property_set(request);
#endif
    }
    cJSON_Delete(root);
    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)request, request_len);
    LOG_TRACE("Post Property Message ID: %d", res);

    return 0;
}

#ifdef ALCS_ENABLED
static int user_property_get_event_handler(const int devid, const char *request, const int request_len, char **response,
        int *response_len)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    cJSON *request_root = NULL, *item_propertyid = NULL;
    cJSON *response_root = NULL;

    LOG_TRACE("Property Get Received, Devid: %d, Request: %s", devid, request);
    request_root = cJSON_Parse(request);
    if (request_root == NULL || !cJSON_IsArray(request_root)) {
        LOG_TRACE("JSON Parse Error");
        return -1;
    }

    response_root = cJSON_CreateObject();
    if (response_root == NULL) {
        LOG_TRACE("No Enough Memory");
        cJSON_Delete(request_root);
        return -1;
    }

    for (int index = 0; index < cJSON_GetArraySize(request_root); index++) {
        item_propertyid = cJSON_GetArrayItem(request_root, index);
        if (item_propertyid == NULL || !cJSON_IsString(item_propertyid)) {
            LOG_TRACE("JSON Parse Error");
            cJSON_Delete(request_root);
            cJSON_Delete(response_root);
            return -1;
        }
        LOG_TRACE("Property ID, index: %d, Value: %s", index, item_propertyid->valuestring);
        if (strcmp("PowerSwitch", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "PowerSwitch", user_example_ctx->power_switch);
#ifdef AOS_TIMER_SERVICE
        } else if (strcmp("LocalTimer", item_propertyid->valuestring) == 0) {
            char *local_timer_str = NULL;

            if (NULL != (local_timer_str = timer_service_property_get("[\"LocalTimer\"]"))) {
                LOG_TRACE("local_timer %s", local_timer_str);
                cJSON *property = NULL, *value = NULL;

                property = cJSON_Parse(local_timer_str);
                if (property == NULL) {
                    LOG_TRACE("No Enough Memory");
                    continue;
                }
                value = cJSON_GetObjectItem(property, "LocalTimer");
                if (value == NULL) {
                    LOG_TRACE("No Enough Memory");
                    cJSON_Delete(property);
                    continue;
                }
                cJSON *dup_value = cJSON_Duplicate(value, 1);

                cJSON_AddItemToObject(response_root, "LocalTimer", dup_value);
                cJSON_Delete(property);
                example_free(local_timer_str);
            } else {
                cJSON *array = cJSON_CreateArray();
                cJSON_AddItemToObject(response_root, "LocalTimer", array);
            }
        } else if (strcmp("CountDownList", item_propertyid->valuestring) == 0) {
            char *count_down_list_str = NULL;

            if (NULL != (count_down_list_str = timer_service_property_get("[\"CountDownList\"]"))) {
                LOG_TRACE("CountDownList %s", count_down_list_str);
                cJSON *property = NULL, *value = NULL;

                property = cJSON_Parse(count_down_list_str);
                if (property == NULL) {
                    LOG_TRACE("No Enough Memory");
                    continue;
                }
                value = cJSON_GetObjectItem(property, "CountDownList");
                if (value == NULL) {
                    LOG_TRACE("No Enough Memory");
                    cJSON_Delete(property);
                    continue;
                }
                cJSON *dup_value = cJSON_Duplicate(value, 1);

                cJSON_AddItemToObject(response_root, "CountDownList", dup_value);
                cJSON_Delete(property);
                example_free(count_down_list_str);
            } else {
                cJSON_AddStringToObject(response_root, "CountDownList", "");
            }
#endif
        }
    }

    cJSON_Delete(request_root);

    *response = cJSON_PrintUnformatted(response_root);
    if (*response == NULL) {
        LOG_TRACE("cJSON_PrintUnformatted Error");
        cJSON_Delete(response_root);
        return -1;
    }
    cJSON_Delete(response_root);
    *response_len = strlen(*response);

    LOG_TRACE("Property Get Response: %s", *response);
    return SUCCESS_RETURN;
}
#endif

static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
        const int reply_len)
{
    const char *reply_value = (reply == NULL) ? ("NULL") : (reply);
    const int reply_value_len = (reply_len == 0) ? (strlen("NULL")) : (reply_len);

    LOG_TRACE("Message Post Reply Received, Devid: %d, Message ID: %d, Code: %d, Reply: %.*s", devid, msgid, code,
                  reply_value_len,
                  reply_value);
    return 0;
}

static int user_trigger_event_reply_event_handler(const int devid, const int msgid, const int code, const char *eventid,
        const int eventid_len, const char *message, const int message_len)
{
    LOG_TRACE("Trigger Event Reply Received, Devid: %d, Message ID: %d, Code: %d, EventID: %.*s, Message: %.*s", devid,
                  msgid, code,
                  eventid_len,
                  eventid, message_len, message);

    return 0;
}

#ifdef AOS_TIMER_SERVICE
static void timer_service_cb(const char *report_data, const char *property_name, int i_value,
                             double d_value, const char * s_value, int prop_idx)
{
    if (prop_idx >= NUM_OF_CONTROL_TARGETS){
        LOG_TRACE("ERROR: prop_idx=%d is out of limit=%d", prop_idx, NUM_OF_CONTROL_TARGETS);
    }
    if (report_data != NULL)/* post property to cloud */
        user_post_property_json(report_data);
    if (property_name != NULL){    /* set value to device */
        LOG_TRACE("timer event callback: property_name=%s prop_idx=%d", property_name, prop_idx);
        if (prop_idx < num_of_tsl_type[0] && strcmp(control_targets_list[0],property_name) == 0)
        {
            powerswitch = i_value;
            if(i_value) {
                product_set_switch(ON);
            } else {
                product_set_switch(OFF);
            }
            LOG_TRACE("timer event callback: int_value=%d", i_value);
            /* set int value */
        }
        else if (prop_idx < num_of_tsl_type[0] + num_of_tsl_type[1]){
            LOG_TRACE("timer event callback: double_value=%f", d_value);
            /* set doube value */
        }
        else {
            if (s_value != NULL)
                LOG_TRACE("timer event callback: test_value=%s", s_value);
            /* set string value */
        }

    }

    return;
}
#endif
static int user_initialized(const int devid)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    LOG_TRACE("Device Initialized, Devid: %d", devid);

    if (user_example_ctx->master_devid == devid) {
        user_example_ctx->master_initialized = 1;
    }

    return 0;
}

/** type:
  *
  * 0 - new firmware exist
  *
  */
// static int user_fota_event_handler(int type, const char *version)
// {
//     char buffer[128] = {0};
//     int buffer_length = 128;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();

//     if (type == 0) {
//         LOG_TRACE("New Firmware Version: %s", version);

//         IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_FOTA_DATA, (unsigned char *)buffer, buffer_length);
//     }

//     return 0;
// }

static uint64_t user_update_sec(void)
{
    static uint64_t time_start_ms = 0;

    if (time_start_ms == 0) {
        time_start_ms = HAL_UptimeMs();
    }

    return (HAL_UptimeMs() - time_start_ms) / 1000;
}

void user_post_property_json(char *property)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    int res =
        IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY, (unsigned char *)property,
                strlen(property));

    LOG_TRACE("Property post Response: %s", property);
    return SUCCESS_RETURN;
}

static int notify_msg_handle(const char *request, const int request_len)
{
    cJSON *request_root = NULL;
    cJSON *item = NULL;

    request_root = cJSON_Parse(request);
    if (request_root == NULL) {
        LOG_TRACE("JSON Parse Error");
        return -1;
    }

    item = cJSON_GetObjectItem(request_root, "identifier");
    if (item == NULL || !cJSON_IsString(item)) {
        cJSON_Delete(request_root);
        return -1;
    }
    if (!strcmp(item->valuestring, "awss.BindNotify")) {
        cJSON *value = cJSON_GetObjectItem(request_root, "value");
        if (item == NULL || !cJSON_IsObject(value)) {
            cJSON_Delete(request_root);
            return -1;
        }
        cJSON *op = cJSON_GetObjectItem(value, "Operation");
        if (op != NULL && cJSON_IsString(op)) {
            if (!strcmp(op->valuestring, "Bind")) {
                LOG_TRACE("Device Bind");
                vendor_device_bind();
            }
            else if (!strcmp(op->valuestring, "Unbind")) {
                LOG_TRACE("Device unBind");
                vendor_device_unbind();
            }
            else if (!strcmp(op->valuestring, "Reset")) {
                LOG_TRACE("Device reset");
                vendor_device_reset();
            }
        }
    }

    cJSON_Delete(request_root);
    return 0;
}

static int user_event_notify_handler(const int devid, const char *request, const int request_len)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    LOG_TRACE("Event notify Received, Devid: %d, Request: %s", devid, request);

    notify_msg_handle(request, strlen(request_len));

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_EVENT_NOTIFY_REPLY,
            (unsigned char *)request, request_len);
    LOG_TRACE("Post Property Message ID: %d", res);

    return 0;
}

void user_post_powerstate(int powerstate)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char property_payload[64];

    snprintf(property_payload, sizeof(property_payload), "{\"%s\":%d}", "PowerSwitch", powerstate);
    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
            property_payload, strlen(property_payload));

    LOG_TRACE("Post Event Message ID: %d payload %s", res, property_payload);
}

void user_post_property_after_connected(void)
{
    static int example_index = 0;
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char property_payload[64];

    snprintf(property_payload, sizeof(property_payload), "{\"%s\":%d}", "PowerSwitch", powerswitch);
    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
            property_payload, strlen(property_payload));

    LOG_TRACE("Post Event Message ID: %d payload %s", res, property_payload);
}

static int user_master_dev_available(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (user_example_ctx->cloud_connected && user_example_ctx->master_initialized) {
        return 1;
    }

    return 0;
}

static int max_running_seconds = 0;
int linkkit_main(void *paras)
{

    uint64_t                        time_prev_sec = 0, time_now_sec = 0;
    uint64_t                        time_begin_sec = 0;
    int                             res = 0;
    iotx_linkkit_dev_meta_info_t    master_meta_info;
    user_example_ctx_t             *user_example_ctx = user_example_get_ctx();
    float                           consumption = 0.0;
    int                             len = sizeof(float);
#if defined(__UBUNTU_SDK_DEMO__)
    int                             argc = ((app_main_paras_t *)paras)->argc;
    char                          **argv = ((app_main_paras_t *)paras)->argv;

    if (argc > 1) {
        int     tmp = atoi(argv[1]);

        if (tmp >= 60) {
            max_running_seconds = tmp;
            LOG_TRACE("set [max_running_seconds] = %d seconds\n", max_running_seconds);
        }
    }
#endif

#if !defined(WIFI_PROVISION_ENABLED) || !defined(BUILD_AOS)
    set_device_meta_info();
#endif

    memset(user_example_ctx, 0, sizeof(user_example_ctx_t));

    powerswitch = (int)product_get_switch();

    /* Register Callback */
    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);
    // IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, user_down_raw_data_arrived_event_handler);
    IOT_RegisterCallback(ITE_SERVICE_REQUEST, user_service_request_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);
#ifdef ALCS_ENABLED
    /*Only for local communication service(ALCS)*/
    IOT_RegisterCallback(ITE_PROPERTY_GET, user_property_get_event_handler);
#endif
    IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);
    IOT_RegisterCallback(ITE_TRIGGER_EVENT_REPLY, user_trigger_event_reply_event_handler);
    // IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);
    // IOT_RegisterCallback(ITE_FOTA, user_fota_event_handler);
    IOT_RegisterCallback(ITE_EVENT_NOTIFY, user_event_notify_handler);

    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    HAL_GetProductKey(master_meta_info.product_key);
    HAL_GetDeviceName(master_meta_info.device_name);
    HAL_GetDeviceSecret(master_meta_info.device_secret);
    HAL_GetProductSecret(master_meta_info.product_secret);

	if((0 == strlen(master_meta_info.product_key))||(0 == strlen(master_meta_info.device_name))\
		||(0 == strlen(master_meta_info.device_secret))||(0 == strlen(master_meta_info.product_secret)))
	{
		LOG_TRACE("No device meta info found...\n");
		while (1) {
			aos_msleep(USER_EXAMPLE_YIELD_TIMEOUT_MS);
		}
	}

    /* Choose Login Server, domain should be configured before IOT_Linkkit_Open() */
#ifdef REGION_SINGAPORE
    int domain_type = IOTX_CLOUD_REGION_SINGAPORE;
#else /* Mainland(Shanghai) for default */
    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
#endif
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Choose Login Method */
    int dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* Choose Whether You Need Post Property/Event Reply */
    int post_event_reply = 1;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_event_reply);

    /* Create Master Device Resources */
    do {
        user_example_ctx->master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
        if (user_example_ctx->master_devid < 0) {
            LOG_TRACE("IOT_Linkkit_Open Failed, retry after 5s...\n");
            HAL_SleepMs(5000);
        }
    } while(user_example_ctx->master_devid < 0);
        /* Start Connect Aliyun Server */
    do {
        res = IOT_Linkkit_Connect(user_example_ctx->master_devid);
        if (res < 0) {
            LOG_TRACE("IOT_Linkkit_Connect Failed, retry after 5s...\n");
            HAL_SleepMs(5000);
        }
    } while(res < 0);

#ifdef AOS_TIMER_SERVICE
    static bool ntp_update = false;
    int ret = timer_service_init( control_targets_list, NUM_OF_PROPERTYS,
                        countdownlist_target_list,  NUM_OF_COUNTDOWN_LIST_TARGET,
                        localtimer_target_list,NUM_OF_LOCAL_TIMER_TARGET,
                        timer_service_cb, num_of_tsl_type, user_connected_event_handler );
    if (ret == 0)
        ntp_update = true;
#endif
    time_begin_sec = user_update_sec();
    while (1) {
        IOT_Linkkit_Yield(USER_EXAMPLE_YIELD_TIMEOUT_MS);

        time_now_sec = user_update_sec();
        if (time_prev_sec == time_now_sec) {
            continue;
        }
        if (max_running_seconds && (time_now_sec - time_begin_sec > max_running_seconds)) {
            LOG_TRACE("Example Run for Over %d Seconds, Break Loop!\n", max_running_seconds);
            break;
        }

        /* init timer service */
        if (time_now_sec % 11 == 0 && user_master_dev_available()) {
#ifdef AOS_TIMER_SERVICE
            if (!ntp_update) {
                int ret = timer_service_init( control_targets_list, NUM_OF_PROPERTYS,
                                    countdownlist_target_list,  NUM_OF_COUNTDOWN_LIST_TARGET,
                                    localtimer_target_list,NUM_OF_LOCAL_TIMER_TARGET,
                                    timer_service_cb, num_of_tsl_type, user_connected_event_handler );
                if (ret == 0)
                    ntp_update = true;
            }
#endif
        }

        time_prev_sec = time_now_sec;
    }

    IOT_Linkkit_Close(user_example_ctx->master_devid);

    IOT_DumpMemoryStats(IOT_LOG_DEBUG);
    IOT_SetLogLevel(IOT_LOG_NONE);

    return 0;
}
