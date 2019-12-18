NAME := comboapp

# Cloud server region: MAINLAND or SINGAPORE, GERMANY
CONFIG_SERVER_REGION ?= MAINLAND

# Cloud server env: ON_DAILY, ON_PRE, ONLINE
CONFIG_SERVER_ENV ?= ONLINE

# Compile date and time
CONFIG_COMPILE_DATE ?= $(shell ${DATE} +%Y%m%d.%H%M%S)
ifeq (xx${CONFIG_COMPILE_DATE}, xx)
	CONFIG_COMPILE_DATE:=$(shell date +%Y%m%d.%H%M%S)
endif

# Firmware type: 0 - release, 1 -  debug
CONFIG_DEBUG ?= 0

$(NAME)_SOURCES :=   app_entry.c \
                     combo_net.c \
                     linkkit_comboapp.c

$(NAME)_COMPONENTS += framework/protocol/linkkit/sdk \
                      framework/protocol/linkkit/hal \
                      framework/netmgr \
                      framework/common \
                      utility/cjson \
                      framework/uOTA 

GLOBAL_INCLUDES += ../../../../framework/protocol/linkkit/include \
                    ../../../../framework/protocol/linkkit/include/imports \
                    ../../../../framework/protocol/linkkit/include/exports \
                   
GLOBAL_CFLAGS += -DCONFIG_YWSS \
                 -DBUILD_AOS \
                 -DAWSS_SUPPORT_STATIS

ifeq ($(CONFIG_SERVER_REGION), MAINLAND)
GLOBAL_CFLAGS += -DMQTT_DIRECT
endif

GLOBAL_CFLAGS += -DREGION_${CONFIG_SERVER_REGION}

GLOBAL_CFLAGS += -D${CONFIG_SERVER_ENV}

GLOBAL_CFLAGS += -DCONFIG_SDK_THREAD_COST=1
$(info server region: ${CONFIG_SERVER_REGION})
$(info server env: ${CONFIG_SERVER_ENV})

BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
GLOBAL_CFLAGS += -DGIT_BRANCH=\"${BRANCH}\"
COMMIT_HASH := $(shell git rev-parse HEAD)
GLOBAL_CFLAGS += -DGIT_HASH=\"${COMMIT_HASH}\"
COMPILE_USER := ${USER}
GLOBAL_CFLAGS += -DCOMPILE_HOST=\"${COMPILE_USER}\"
SERVER_CONF_STRING := "${CONFIG_SERVER_REGION}-${CONFIG_SERVER_ENV}"
GLOBAL_CFLAGS += -DREGION_ENV_STRING=\"${SERVER_CONF_STRING}\"
GLOBAL_CFLAGS += -DAPP_NAME=\"${APP}\"
GLOBAL_CFLAGS += -DPLATFORM=\"${PLATFORM}\"
CONFIG_SYSINFO_APP_VERSION = app-1.2.0-${CONFIG_COMPILE_DATE}
GLOBAL_CFLAGS += -DSYSINFO_APP_VERSION=\"$(CONFIG_SYSINFO_APP_VERSION)\"

$(info APP: ${APP} Board: ${PLATFORM})
$(info host user: ${COMPILE_USER})
$(info branch: ${BRANCH})
$(info hash: ${COMMIT_HASH})
$(info app_version_new:${CONFIG_SYSINFO_APP_VERSION})

ifeq ($(CONFIG_DEBUG), 1)
GLOBAL_DEFINES += DEBUG
GLOBAL_CFLAGS += -DCONFIG_BLDTIME_MUTE_DBGLOG=0
$(info firmware type: DEBUG)
else
GLOBAL_CFLAGS += -DCONFIG_BLDTIME_MUTE_DBGLOG=1
$(info firmware type: RELEASE)
endif

ifeq ($(LWIP),1)
$(NAME)_COMPONENTS  += protocols.net
no_with_lwip := 0
endif

ifeq ($(print_heap),1)
$(NAME)_DEFINES += CONFIG_PRINT_HEAP
endif

ifneq ($(HOST_MCU_FAMILY),esp8266)
$(NAME)_COMPONENTS  += cli
GLOBAL_DEFINES += CONFIG_AOS_CLI
else
GLOBAL_DEFINES += ESP8266_CHIPSET
endif

en_comboapp := 1
ifeq ($(en_comboapp), 1)
$(NAME)_COMPONENTS += framework.bluetooth.breeze
GLOBAL_DEFINES += EN_COMBO_NET
bz_en_auth := 1
bz_en_awss := 1
ble := 1
endif

GLOBAL_INCLUDES += ./

include example/${APP_FULL}/make.settings

SWITCH_VARS := FEATURE_MQTT_COMM_ENABLED   FEATURE_ALCS_ENABLED FEATURE_MQTT_DIRECT FEATURE_DEVICE_MODEL_ENABLED  \
    FEATURE_DEVICE_MODEL_GATEWAY  FEATURE_DEPRECATED_LINKKIT   FEATURE_DEV_BIND_ENABLED \
    FEATURE_DEVICE_MODEL_RAWDATA_SOLO  FEATURE_COAP_COMM_ENABLED FEATURE_HTTP2_COMM_ENABLED \
    FEATURE_HTTP_COMM_ENABLED FEATURE_SAL_ENABLED  FEATURE_WIFI_PROVISION_ENABLED FEATURE_AWSS_SUPPORT_SMARTCONFIG\
    FEATURE_AWSS_SUPPORT_ZEROCONFIG FEATURE_AWSS_SUPPORT_SMARTCONFIG FEATURE_AWSS_SUPPORT_ZEROCONFIG FEATURE_AWSS_SUPPORT_PHONEASAP \
    FEATURE_AWSS_SUPPORT_ROUTER FEATURE_AWSS_SUPPORT_DEV_AP FEATURE_OTA_ENABLED FEATURE_MQTT_AUTO_SUBSCRIBE FEATURE_MQTT_PREAUTH_SUPPORT_HTTPS_CDN

$(foreach v, \
    $(SWITCH_VARS), \
    $(if $(filter y,$($(v))), \
        $(eval GLOBAL_DEFINES += $(subst FEATURE_,,$(v)))) \
)