#ifndef __GATEWAY_MAIN_H__
#define __GATEWAY_MAIN_H__

#include <iotx_log.h>

#define gateway_debug(...) log_debug("gateway", __VA_ARGS__)
#define gateway_info(...) log_info("gateway", __VA_ARGS__)
#define gateway_warn(...) log_warning("gateway", __VA_ARGS__)
#define gateway_err(...) log_err("gateway", __VA_ARGS__)
#define gateway_crit(...) log_crit("gateway", __VA_ARGS__)

// for demo only
#define PRODUCT_KEY "a2sQxuXG1cg"
#define PRODUCT_SECRET "ElUhyA29337YQax9"
#define DEVICE_NAME "gateway_device_1"
#define DEVICE_SECRET "zh1W6SRpT0QI5hA54EKilSCyt1epGWDb"

#define GATEWAY_YIELD_TIMEOUT_MS (200)

#define GATEWAY_SUBDEV_MAX_NUM (2)

#define QUEUE_MSG_SIZE sizeof(gateway_msg_t)
#define MAX_QUEUE_SIZE 10 * QUEUE_MSG_SIZE

//You should undefine this Macro in your products
#define GATEWAY_UT_TESTING

typedef struct
{
    int master_devid;
    int cloud_connected;
    int master_initialized;
    int subdev_index;
    int permit_join;
    void *g_user_dispatch_thread;
    int g_user_dispatch_thread_running;
} gateway_ctx_t;

typedef enum _gateway_msg_type_e
{
    GATEWAY_MSG_TYPE_ADD,
    GATEWAY_MSG_TYPE_DEL,
    GATEWAY_MSG_TYPE_RESET,
    GATEWAY_MSG_TYPE_UPDATE,
    GATEWAY_MSG_TYPE_ADDALL,
    GATEWAY_MSG_TYPE_QUERY_SUBDEV_ID,
    GATEWAY_MSG_TYPE_MAX

} gateway_msg_type_e;

typedef struct _gateway_msg_s
{
    gateway_msg_type_e msg_type;
    int devid;
} gateway_msg_t;

extern int linkkit_main(void *paras);
extern int gateway_main_send_msg(gateway_msg_t *, int);

#endif
