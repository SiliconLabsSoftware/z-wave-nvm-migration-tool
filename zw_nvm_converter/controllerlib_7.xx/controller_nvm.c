/* © 2019 Silicon Laboratories Inc. */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "endian_wrap.h"
#include "controller_nvm.h"
#include "ZW_nodemask_api.h"
#include "json_helpers.h"
#include "nvm3_helpers.h"
#include "nvm3_hal_ram.h"
#include "user_message.h"
#include "base64.h"

#include "zwave_controller_network_info_storage.h"
#include "zwave_controller_application_info_storage.h"

#define UNUSED(x) (void)(x)
#define sizeof_array(ARRAY) ((sizeof ARRAY) / (sizeof ARRAY[0]))

#define APP_CACHE_SIZE 250
#define PROTOCOL_CACHE_SIZE 250

#define FLASH_PAGE_SIZE_700s (2 * 1024)
#define APP_NVM_SIZE_700s (12 * 1024)
#define PROTOCOL_NVM_SIZE_700s (36 * 1024)
#define NVM_SIZE_700s APP_NVM_SIZE_700s + PROTOCOL_NVM_SIZE_700s

#define FLASH_PAGE_SIZE_800s (8 * 1024)
#define APP_NVM_SIZE_800s_PRIOR_719 (24 * 1024)
#define PROTOCOL_NVM_SIZE_800s_PRIOR_719 (40 * 1024)
#define NVM_SIZE_800s_PRIOR_719 APP_NVM_SIZE_800s_PRIOR_719 + PROTOCOL_NVM_SIZE_800s_PRIOR_719

// #define APP_NVM_SIZE_800s_719       (8 * 1024)
// #define PROTOCOL_NVM_SIZE_800s_719  (32 * 1024)
#define NVM_SIZE_800s_FROM_719 (5 * 8 * 1024)

/* This array is not page aligned. Since the nvm3 lib requires
   its memory to be page aligned we allocate one extra page here
   in order to make a page aligned pointer into this area */
#define NVM3_STORAGE_SIZE (72 * 1024)
static uint8_t nvm3_storage[NVM3_STORAGE_SIZE];

typedef struct
{
  uint8_t format;
  uint8_t major;
  uint8_t minor;
  uint8_t patch;
} target_version;

target_version target_protocol_version = {};
target_version target_app_version = {};

static uint32_t prot_version_le = 0;
static uint32_t app_version_le = 0;
static hardware_info_t hardware_info = ZW_700s;
static nvm3_file_descriptor_t *nvm3_current_protocol_files;
static nvm3_file_descriptor_t *nvm3_files_v5_800s_719_720;
static size_t nvm3_files_v5_800s_719_720_size;
static nvm3_file_descriptor_t *nvm3_files_v5_800s_721;
static size_t nvm3_files_v5_800s_721_size;
static size_t nvm3_current_protocol_files_size;

// Page aligned storage address
static nvm3_HalPtr_t nvm3_storage_address;

static const nvm3_HalHandle_t *halHandle;
static const nvm3_HalConfig_t *halConfig;

static nvm3_Handle_t nvm3_app_handle;
static nvm3_CacheEntry_t nvm3_app_cache[APP_CACHE_SIZE];

static nvm3_Handle_t nvm3_protocol_handle;
static nvm3_CacheEntry_t nvm3_protocol_cache[APP_CACHE_SIZE + PROTOCOL_CACHE_SIZE]; // Add more APP_CACHE_SIZE for NVM migration from 7.18->7.19 or 7.2x

static nvm3_file_descriptor_t nvm3_protocol_files_v0[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_NODEINFO, .size = FILE_SIZE_NODEINFO, .name = "NODEINFO", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES},
    {.key = FILE_ID_NODEROUTECAHE, .size = FILE_SIZE_NODEROUTECAHE, .name = "NODEROUTECAHE", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES},
    {.key = FILE_ID_PREFERREDREPEATERS, .size = FILE_SIZE_PREFERREDREPEATERS, .name = "PREFERREDREPEATERS", .optional = true},
    {.key = FILE_ID_SUCNODELIST, .size = FILE_SIZE_SUCNODELIST, .name = "SUCNODELIST"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM711, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
    {.key = FILE_ID_S2_KEYS, .size = FILE_SIZE_S2_KEYS, .name = "S2_KEYS", .optional = true},
    {.key = FILE_ID_S2_KEYCLASSES_ASSIGNED, .size = FILE_SIZE_S2_KEYCLASSES_ASSIGNED, .name = "S2_KEYCLASSES_ASSIGNED", .optional = true},
    {.key = FILE_ID_S2_MPAN, .size = FILE_SIZE_S2_MPAN, .name = "S2_MPAN", .optional = true},
    {.key = FILE_ID_S2_SPAN, .size = FILE_SIZE_S2_SPAN, .name = "S2_SPAN", .optional = true},
};

static nvm3_file_descriptor_t nvm3_protocol_files_v1[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_NODEINFO_V1, .size = FILE_SIZE_NODEINFO_V1, .name = "NODEINFO_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEROUTECAHE_V1, .size = FILE_SIZE_NODEROUTECAHE_V1, .name = "NODEROUTECAHE_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_PREFERREDREPEATERS, .size = FILE_SIZE_PREFERREDREPEATERS, .name = "PREFERREDREPEATERS", .optional = true},
    {.key = FILE_ID_SUCNODELIST, .size = FILE_SIZE_SUCNODELIST, .name = "SUCNODELIST"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM711, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
    {.key = FILE_ID_S2_KEYS, .size = FILE_SIZE_S2_KEYS, .name = "S2_KEYS", .optional = true},
    {.key = FILE_ID_S2_KEYCLASSES_ASSIGNED, .size = FILE_SIZE_S2_KEYCLASSES_ASSIGNED, .name = "S2_KEYCLASSES_ASSIGNED", .optional = true},
    {.key = FILE_ID_S2_MPAN, .size = FILE_SIZE_S2_MPAN, .name = "S2_MPAN", .optional = true},
    {.key = FILE_ID_S2_SPAN, .size = FILE_SIZE_S2_SPAN, .name = "S2_SPAN", .optional = true},
};

static nvm3_file_descriptor_t nvm3_protocol_files_v2[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_NODEINFO_V1, .size = FILE_SIZE_NODEINFO_V1, .name = "NODEINFO_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEINFO_LR, .size = FILE_SIZE_NODEINFO_LR, .name = "NODEINFO_LR", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_NODEINFOS_PER_FILE},
    {.key = FILE_ID_LR_TX_POWER_V2, .size = FILE_SIZE_LR_TX_POWER, .name = "LR_TX_POWER_V2", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_TX_POWER_PER_FILE_V2},
    {.key = FILE_ID_NODEROUTECAHE_V1, .size = FILE_SIZE_NODEROUTECAHE_V1, .name = "NODEROUTECAHE_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_PREFERREDREPEATERS, .size = FILE_SIZE_PREFERREDREPEATERS, .name = "PREFERREDREPEATERS", .optional = true},
    {.key = FILE_ID_SUCNODELIST, .size = FILE_SIZE_SUCNODELIST, .name = "SUCNODELIST"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM715, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_LRANGE_NODE_EXIST, .size = FILE_SIZE_LRANGE_NODE_EXIST, .name = "LRANGE_NODE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
};

static nvm3_file_descriptor_t nvm3_protocol_files_v3[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_NODEINFO_V1, .size = FILE_SIZE_NODEINFO_V1, .name = "NODEINFO_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEINFO_LR, .size = FILE_SIZE_NODEINFO_LR, .name = "NODEINFO_LR", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_NODEINFOS_PER_FILE},
    {.key = FILE_ID_LR_TX_POWER_V3, .size = FILE_SIZE_LR_TX_POWER, .name = "LR_TX_POWER_V3", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_TX_POWER_PER_FILE_V3},
    {.key = FILE_ID_NODEROUTECAHE_V1, .size = FILE_SIZE_NODEROUTECAHE_V1, .name = "NODEROUTECAHE_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_PREFERREDREPEATERS, .size = FILE_SIZE_PREFERREDREPEATERS, .name = "PREFERREDREPEATERS", .optional = true},
    {.key = FILE_ID_SUCNODELIST, .size = FILE_SIZE_SUCNODELIST, .name = "SUCNODELIST"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM715, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_LRANGE_NODE_EXIST, .size = FILE_SIZE_LRANGE_NODE_EXIST, .name = "LRANGE_NODE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
};

static nvm3_file_descriptor_t nvm3_protocol_files_v4[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_NODEINFO_V1, .size = FILE_SIZE_NODEINFO_V1, .name = "NODEINFO_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEINFO_LR, .size = FILE_SIZE_NODEINFO_LR, .name = "NODEINFO_LR", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEROUTECAHE_V1, .size = FILE_SIZE_NODEROUTECAHE_V1, .name = "NODEROUTECAHE_V1", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_PREFERREDREPEATERS, .size = FILE_SIZE_PREFERREDREPEATERS, .name = "PREFERREDREPEATERS", .optional = true},
    {.key = FILE_ID_SUCNODELIST, .size = FILE_SIZE_SUCNODELIST, .name = "SUCNODELIST"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM715, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_LRANGE_NODE_EXIST, .size = FILE_SIZE_LRANGE_NODE_EXIST, .name = "LRANGE_NODE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
};

static nvm3_file_descriptor_t nvm3_files_v5_800s_719_720_xg28[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM715, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_LRANGE_NODE_EXIST, .size = FILE_SIZE_LRANGE_NODE_EXIST, .name = "LRANGE_NODE_EXIST"},
    {.key = FILE_ID_NODEINFO_BASE_V5, .size = FILE_SIZE_NODEINFO_BASE_V5, .name = "NODEINFO_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEINFO_LR_BASE_V5, .size = FILE_SIZE_NODEINFO_LR_BASE_V5, .name = "NODEINFO_LR_BASE_V5", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEROUTECAHE_BASE_V5, .size = FILE_SIZE_NODEROUTECAHE_BASE_V5, .name = "NODEROUTECAHE_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_S2_SPAN_BASE_V5, .size = FILE_SIZE_S2_SPAN_BASE_V5, .name = "S2_SPAN_BASE"},
    {.key = FILE_ID_S2_MPAN_BASE_V5, .size = FILE_SIZE_S2_MPAN_BASE_V5, .name = "S2_MPAN_BASE"},
    {.key = FILE_ID_SUCNODELIST_BASE_V5, .size = FILE_SIZE_SUCNODELIST_BASE_V5, .name = "SUCNODELIST_BASE", .num_keys = SUC_MAX_UPDATES / SUCNODES_PER_FILE},
    {.key = FILE_ID_APPLICATIONSETTINGS, .size = FILE_SIZE_APPLICATIONSETTINGS, .name = "APPLICATIONSETTINGS"},
    {.key = FILE_ID_APPLICATIONCMDINFO, .size = FILE_SIZE_APPLICATIONCMDINFO, .name = "APPLICATIONCMDINFO"},
    {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_7_18_1, .name = "APPLICATIONCONFIGURATION"},
    {.key = FILE_ID_APPLICATIONDATA, .size = FILE_SIZE_APPLICATIONDATA, .name = "applicationData"},
    {.key = FILE_ID_PROPRIETARY_1, .size = FILE_SIZE_PROPRIETARY_1, .name = "PROPRIETARY_1"},
    {.key = ZAF_FILE_ID_APP_VERSION_800s_XG28, .size = ZAF_FILE_SIZE_APP_VERSION, .name = "APP_VERSION_XG28"},
    {.key = ZAF_FILE_ID_APP_NAME_800s_XG28, .size = ZAF_FILE_SIZE_APP_NAME, .name = "APP_NAME"}};

static nvm3_file_descriptor_t nvm3_files_v5_800s_719_720_zgm230[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM715, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_LRANGE_NODE_EXIST, .size = FILE_SIZE_LRANGE_NODE_EXIST, .name = "LRANGE_NODE_EXIST"},
    {.key = FILE_ID_NODEINFO_BASE_V5, .size = FILE_SIZE_NODEINFO_BASE_V5, .name = "NODEINFO_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEINFO_LR_BASE_V5, .size = FILE_SIZE_NODEINFO_LR_BASE_V5, .name = "NODEINFO_LR_BASE_V5", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEROUTECAHE_BASE_V5, .size = FILE_SIZE_NODEROUTECAHE_BASE_V5, .name = "NODEROUTECAHE_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_S2_SPAN_BASE_V5, .size = FILE_SIZE_S2_SPAN_BASE_V5, .name = "S2_SPAN_BASE"},
    {.key = FILE_ID_S2_MPAN_BASE_V5, .size = FILE_SIZE_S2_MPAN_BASE_V5, .name = "S2_MPAN_BASE"},
    {.key = FILE_ID_SUCNODELIST_BASE_V5, .size = FILE_SIZE_SUCNODELIST_BASE_V5, .name = "SUCNODELIST_BASE", .num_keys = SUC_MAX_UPDATES / SUCNODES_PER_FILE},
    {.key = FILE_ID_APPLICATIONSETTINGS, .size = FILE_SIZE_APPLICATIONSETTINGS, .name = "APPLICATIONSETTINGS"},
    {.key = FILE_ID_APPLICATIONCMDINFO, .size = FILE_SIZE_APPLICATIONCMDINFO, .name = "APPLICATIONCMDINFO"},
    {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_7_18_1, .name = "APPLICATIONCONFIGURATION"},
    {.key = FILE_ID_APPLICATIONDATA, .size = FILE_SIZE_APPLICATIONDATA, .name = "applicationData"},
    {.key = FILE_ID_PROPRIETARY_1, .size = FILE_SIZE_PROPRIETARY_1, .name = "PROPRIETARY_1"},
    {.key = ZAF_FILE_ID_APP_VERSION_800s_XG23, .size = ZAF_FILE_SIZE_APP_VERSION, .name = "APP_VERSION"},
    {.key = ZAF_FILE_ID_APP_NAME_800s_XG23, .size = ZAF_FILE_SIZE_APP_NAME, .name = "APP_NAME"}};

static nvm3_file_descriptor_t nvm3_files_v5_800s_721_xg28[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM715, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_LRANGE_NODE_EXIST, .size = FILE_SIZE_LRANGE_NODE_EXIST, .name = "LRANGE_NODE_EXIST"},
    {.key = FILE_ID_NODEINFO_BASE_V5, .size = FILE_SIZE_NODEINFO_BASE_V5, .name = "NODEINFO_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEINFO_LR_BASE_V5, .size = FILE_SIZE_NODEINFO_LR_BASE_V5, .name = "NODEINFO_LR_BASE_V5", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEROUTECAHE_BASE_V5, .size = FILE_SIZE_NODEROUTECAHE_BASE_V5, .name = "NODEROUTECAHE_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_S2_SPAN_BASE_V5, .size = FILE_SIZE_S2_SPAN_BASE_V5, .name = "S2_SPAN_BASE"},
    {.key = FILE_ID_S2_MPAN_BASE_V5, .size = FILE_SIZE_S2_MPAN_BASE_V5, .name = "S2_MPAN_BASE"},
    {.key = FILE_ID_SUCNODELIST_BASE_V5, .size = FILE_SIZE_SUCNODELIST_BASE_V5, .name = "SUCNODELIST_BASE", .num_keys = SUC_MAX_UPDATES / SUCNODES_PER_FILE},
    {.key = FILE_ID_APPLICATIONSETTINGS, .size = FILE_SIZE_APPLICATIONSETTINGS, .name = "APPLICATIONSETTINGS"},
    {.key = FILE_ID_APPLICATIONCMDINFO, .size = FILE_SIZE_APPLICATIONCMDINFO, .name = "APPLICATIONCMDINFO"},
    {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_7_21_x, .name = "APPLICATIONCONFIGURATION"},
    {.key = FILE_ID_APPLICATIONDATA, .size = FILE_SIZE_APPLICATIONDATA, .name = "applicationData"},
    {.key = FILE_ID_PROPRIETARY_1, .size = FILE_SIZE_PROPRIETARY_1, .name = "PROPRIETARY_1"},
    {.key = ZAF_FILE_ID_APP_VERSION_800s_XG28, .size = ZAF_FILE_SIZE_APP_VERSION, .name = "APP_VERSION"},
    {.key = ZAF_FILE_ID_APP_NAME_800s_XG28, .size = ZAF_FILE_SIZE_APP_NAME, .name = "APP_NAME"}};

static nvm3_file_descriptor_t nvm3_files_v5_800s_721_zgm230[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_CONTROLLERINFO, .size = FILE_SIZE_CONTROLLERINFO_NVM715, .name = "CONTROLLERINFO"},
    {.key = FILE_ID_NODE_STORAGE_EXIST, .size = FILE_SIZE_NODE_STORAGE_EXIST, .name = "NODE_STORAGE_EXIST"},
    {.key = FILE_ID_APP_ROUTE_LOCK_FLAG, .size = FILE_SIZE_APP_ROUTE_LOCK_FLAG, .name = "APP_ROUTE_LOCK_FLAG"},
    {.key = FILE_ID_ROUTE_SLAVE_SUC_FLAG, .size = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG, .name = "ROUTE_SLAVE_SUC_FLAG"},
    {.key = FILE_ID_SUC_PENDING_UPDATE_FLAG, .size = FILE_SIZE_SUC_PENDING_UPDATE_FLAG, .name = "SUC_PENDING_UPDATE_FLAG"},
    {.key = FILE_ID_BRIDGE_NODE_FLAG, .size = FILE_SIZE_BRIDGE_NODE_FLAG, .name = "BRIDGE_NODE_FLAG"},
    {.key = FILE_ID_PENDING_DISCOVERY_FLAG, .size = FILE_SIZE_PENDING_DISCOVERY_FLAG, .name = "PENDING_DISCOVERY_FLAG"},
    {.key = FILE_ID_NODE_ROUTECACHE_EXIST, .size = FILE_SIZE_NODE_ROUTECACHE_EXIST, .name = "NODE_ROUTECACHE_EXIST"},
    {.key = FILE_ID_LRANGE_NODE_EXIST, .size = FILE_SIZE_LRANGE_NODE_EXIST, .name = "LRANGE_NODE_EXIST"},
    {.key = FILE_ID_NODEINFO_BASE_V5, .size = FILE_SIZE_NODEINFO_BASE_V5, .name = "NODEINFO_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEINFO_LR_BASE_V5, .size = FILE_SIZE_NODEINFO_LR_BASE_V5, .name = "NODEINFO_LR_BASE_V5", .optional = true, .num_keys = ZW_LR_MAX_NODES / LR_NODEINFOS_PER_FILE},
    {.key = FILE_ID_NODEROUTECAHE_BASE_V5, .size = FILE_SIZE_NODEROUTECAHE_BASE_V5, .name = "NODEROUTECAHE_BASE_V5", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES / NODEROUTECACHES_PER_FILE},
    {.key = FILE_ID_S2_SPAN_BASE_V5, .size = FILE_SIZE_S2_SPAN_BASE_V5, .name = "S2_SPAN_BASE"},
    {.key = FILE_ID_S2_MPAN_BASE_V5, .size = FILE_SIZE_S2_MPAN_BASE_V5, .name = "S2_MPAN_BASE"},
    {.key = FILE_ID_SUCNODELIST_BASE_V5, .size = FILE_SIZE_SUCNODELIST_BASE_V5, .name = "SUCNODELIST_BASE", .num_keys = SUC_MAX_UPDATES / SUCNODES_PER_FILE},
    {.key = FILE_ID_APPLICATIONSETTINGS, .size = FILE_SIZE_APPLICATIONSETTINGS, .name = "APPLICATIONSETTINGS"},
    {.key = FILE_ID_APPLICATIONCMDINFO, .size = FILE_SIZE_APPLICATIONCMDINFO, .name = "APPLICATIONCMDINFO"},
    {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_7_21_x, .name = "APPLICATIONCONFIGURATION"},
    {.key = FILE_ID_APPLICATIONDATA, .size = FILE_SIZE_APPLICATIONDATA, .name = "applicationData"},
    {.key = FILE_ID_PROPRIETARY_1, .size = FILE_SIZE_PROPRIETARY_1, .name = "PROPRIETARY_1"},
    {.key = ZAF_FILE_ID_APP_VERSION_800s_XG23, .size = ZAF_FILE_SIZE_APP_VERSION, .name = "APP_VERSION"},
    {.key = ZAF_FILE_ID_APP_NAME_800s_XG23, .size = ZAF_FILE_SIZE_APP_NAME, .name = "APP_NAME"}};

static nvm3_file_descriptor_t nvm3_app_files_prior_7_15_3[] = {
    {.key = ZAF_FILE_ID_APP_VERSION, .size = ZAF_FILE_SIZE_APP_VERSION, .name = "APP_VERSION"},
    {.key = FILE_ID_APPLICATIONSETTINGS, .size = FILE_SIZE_APPLICATIONSETTINGS, .name = "APPLICATIONSETTINGS"},
    {.key = FILE_ID_APPLICATIONCMDINFO, .size = FILE_SIZE_APPLICATIONCMDINFO, .name = "APPLICATIONCMDINFO"},
    {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_prior_7_15_3, .name = "APPLICATIONCONFIGURATION"},
    {.key = FILE_ID_APPLICATIONDATA, .size = FILE_SIZE_APPLICATIONDATA, .name = "applicationData"}};

static nvm3_file_descriptor_t nvm3_app_files_prior_7_18_1[] = {
    {.key = ZAF_FILE_ID_APP_VERSION, .size = ZAF_FILE_SIZE_APP_VERSION, .name = "APP_VERSION"},
    {.key = FILE_ID_APPLICATIONSETTINGS, .size = FILE_SIZE_APPLICATIONSETTINGS, .name = "APPLICATIONSETTINGS"},
    {.key = FILE_ID_APPLICATIONCMDINFO, .size = FILE_SIZE_APPLICATIONCMDINFO, .name = "APPLICATIONCMDINFO"},
    {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_prior_7_18_1, .name = "APPLICATIONCONFIGURATION"},
    {.key = FILE_ID_APPLICATIONDATA, .size = FILE_SIZE_APPLICATIONDATA, .name = "applicationData"}};

static nvm3_file_descriptor_t nvm3_app_files[] = {
    {.key = ZAF_FILE_ID_APP_VERSION, .size = ZAF_FILE_SIZE_APP_VERSION, .name = "APP_VERSION"},
    {.key = ZAF_FILE_ID_APP_NAME, .size = ZAF_FILE_SIZE_APP_NAME, .name = "APP_NAME"},
    {.key = FILE_ID_APPLICATIONSETTINGS, .size = FILE_SIZE_APPLICATIONSETTINGS, .name = "APPLICATIONSETTINGS"},
    {.key = FILE_ID_APPLICATIONCMDINFO, .size = FILE_SIZE_APPLICATIONCMDINFO, .name = "APPLICATIONCMDINFO"},
    {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_7_18_1, .name = "APPLICATIONCONFIGURATION"},
    {.key = FILE_ID_APPLICATIONDATA, .size = FILE_SIZE_APPLICATIONDATA, .name = "applicationData"}};

#define READ_PROT_NVM(file_id, dest_var) nvm3_readData_print_error(&nvm3_protocol_handle,       \
                                                                   (file_id),                   \
                                                                   &(dest_var),                 \
                                                                   sizeof(dest_var),            \
                                                                   nvm3_current_protocol_files, \
                                                                   nvm3_current_protocol_files_size)

#define READ_APP_NVM_PRIOR_7_15_3(file_id, dest_var) nvm3_readData_print_error(&nvm3_app_handle,            \
                                                                               (file_id),                   \
                                                                               &(dest_var),                 \
                                                                               sizeof(dest_var),            \
                                                                               nvm3_app_files_prior_7_15_3, \
                                                                               sizeof_array(nvm3_app_files_prior_7_15_3))

#define READ_APP_NVM(file_id, dest_var) nvm3_readData_print_error(&nvm3_app_handle, \
                                                                  (file_id),        \
                                                                  &(dest_var),      \
                                                                  sizeof(dest_var), \
                                                                  nvm3_app_files,   \
                                                                  sizeof_array(nvm3_app_files))

#define WRITE_PROT_NVM(file_id, src_var)                               \
  do                                                                   \
  {                                                                    \
    printf("Write key [0x%x] - Len: %ld\n", file_id, sizeof(src_var)); \
    nvm3_writeData_print_error(&nvm3_protocol_handle,                  \
                               (file_id),                              \
                               &(src_var),                             \
                               sizeof(src_var),                        \
                               nvm3_current_protocol_files,            \
                               nvm3_current_protocol_files_size);      \
  } while (0);

#define WRITE_APP_NVM_PRIOR_7_15_3(file_id, src_var) nvm3_writeData_print_error(&nvm3_app_handle,            \
                                                                                (file_id),                   \
                                                                                &(src_var),                  \
                                                                                sizeof(src_var),             \
                                                                                nvm3_app_files_prior_7_15_3, \
                                                                                sizeof_array(nvm3_app_files_prior_7_15_3))

#define WRITE_APP_NVM(file_id, src_var) nvm3_writeData_print_error(&nvm3_app_handle, \
                                                                   (file_id),        \
                                                                   &(src_var),       \
                                                                   sizeof(src_var),  \
                                                                   nvm3_app_files,   \
                                                                   sizeof_array(nvm3_app_files))

/*****************************************************************************/
/****************************  NVM Basic Handling  ***************************/
/*****************************************************************************/

/* Set target protocol and application version for generating nvm image */
static bool set_target_version(const char *protocol_version, const char *app_version)
{
  int prot_major, prot_minor, prot_patch;
  int app_major, app_minor, app_patch;

  if (sscanf(protocol_version, "%d.%d.%d", &prot_major, &prot_minor, &prot_patch) != 3)
  {
    printf("Invalid protocol version format: %s\n", protocol_version);
    return false;
  }

  if (sscanf(app_version, "%d.%d.%d", &app_major, &app_minor, &app_patch) != 3)
  {
    printf("Invalid application version format: %s\n", app_version);
    return false;
  }
  // Set the target application version
  target_app_version.major = app_major;
  target_app_version.minor = app_minor;
  target_app_version.patch = app_patch;

  printf("Generating NVM image...\n");
  printf("Target Protocol Version: %s\n", protocol_version);
  nvm3_files_v5_800s_719_720 = (hardware_info == EFR32XG28) ? (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_719_720_xg28 : (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_719_720_zgm230;
  nvm3_files_v5_800s_721 = (hardware_info == EFR32XG28) ? (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_721_xg28 : (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_721_zgm230;

  // Set the target protocol version
  if (prot_major == 7)
  {
    target_protocol_version.major = 7;
    if (prot_minor == 11)
    {
      target_protocol_version.format = 0;
      target_protocol_version.minor = 11;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_protocol_files_v0;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_protocol_files_v0);
      return true;
    }
    else if (prot_minor == 12)
    {
      target_protocol_version.format = 1;
      target_protocol_version.minor = 12;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_protocol_files_v1;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_protocol_files_v1);
      return true;
    }
    else if (prot_minor == 15)
    {
      target_protocol_version.format = 2;
      target_protocol_version.minor = 15;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_protocol_files_v2;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_protocol_files_v2);
      return true;
    }
    else if (prot_minor == 16)
    {
      target_protocol_version.format = 3;
      target_protocol_version.minor = 16;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_protocol_files_v3;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_protocol_files_v3);
      return true;
    }
    else if (prot_minor == 17)
    {
      target_protocol_version.format = 4;
      target_protocol_version.minor = 17;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_protocol_files_v4;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_protocol_files_v4);
      return true;
    }
    else if (prot_minor == 18)
    {
      target_protocol_version.format = 4;
      target_protocol_version.minor = 18;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_protocol_files_v4;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_protocol_files_v4);
      return true;
    }
    else if (prot_minor == 19 || prot_minor == 20)
    {
      target_protocol_version.format = 5;
      target_protocol_version.minor = prot_minor;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_files_v5_800s_719_720;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_files_v5_800s_719_720_xg28);
      return true;
    }
    else if (prot_minor >= 21 && prot_minor <= 24)
    {
      target_protocol_version.format = 5;
      target_protocol_version.minor = prot_minor;
      target_protocol_version.patch = prot_patch;
      nvm3_current_protocol_files = nvm3_files_v5_800s_721;
      nvm3_current_protocol_files_size = sizeof_array(nvm3_files_v5_800s_721_xg28);
      return true;
    }
  }
  // Return false if protocol version is not supported 
  return false;
}

static bool app_nvm_is_pre_v7_15_3(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor >= 16))
  {
    return false;
  }
  else if ((major == 7) && (minor == 15) && (patch >= 3))
  {
    return false;
  }
  return true;
}

static bool app_nvm_is_pre_v7_18_1(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor >= 19))
  {
    return false;
  }
  else if ((major == 7) && (minor == 18) && (patch >= 1))
  {
    return false;
  }
  return true;
}

static bool app_nvm_is_v7_19(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor == 19))
  {
    return true;
  }
  return false;
}

static bool app_nvm_is_v7_20(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor == 20))
  {
    return true;
  }
  return false;
}

static bool app_nvm_is_v7_21(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor == 21))
  {
    return true;
  }
  return false;
}

static bool app_nvm_is_v7_22(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor == 22))
  {
    return true;
  }
  return false;
}

static bool app_nvm_is_pre_v7_21(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor <= 20))
  {
    return true;
  }
  return false;
}

static bool app_nvm_is_pre_v7_19(uint8_t major, uint8_t minor, uint8_t patch)
{
  if ((major == 7) && (minor <= 18))
  {
    return true;
  }
  return false;
}

/*****************************************************************************/
/* Used to parse the key 0x5A000 to determine whether the hardware is xg23 or xg28 */
bool check_controller_nvm(const uint8_t *nvm_image, size_t nvm_image_size, nvmLayout_t nvm_layout)
{
  uint32_t prot_version_le = 0;
  uint32_t app_version_le = 0;
  uint8_t major = 0, minor = 0, patch = 0;
  uint32_t type;
  size_t size;
  bool check_pass = true;
  if (!open_controller_nvm(nvm_image, nvm_image_size, nvm_layout))
    return false;

  printf("Reading NVM image...\n");
  switch (nvm_image_size)
  {
  case 0xA000:
  case 0x10000:
    if (ECODE_NVM3_OK == nvm3_getObjectInfo(&nvm3_protocol_handle, ZAF_FILE_ID_APP_VERSION_800s_XG28, &type, &size))
    {
      hardware_info = EFR32XG28;
      printf("This is EFR32XG28\n");
    }
    else
    {
      hardware_info = EFR32XG23;
      printf("This is EFR32XG23\n");
    }
    break;
  case 0xC000:
    printf("This is 700 Series\n");
    break;
  // case 0x10000:
  //   hardware_info = EFR32XG23;
  //   printf("[DEBUG_LOG]: Hardware is EFR32XG23\n");
  //   break;
  default:
    printf("Hardware is unknown or NVM Image Size is invalid\n");
    break;
  }
  READ_PROT_NVM(FILE_ID_ZW_VERSION, prot_version_le);
  major = (le32toh(prot_version_le) >> 16) & 0xff;
  minor = (le32toh(prot_version_le) >> 8) & 0xff;
  patch = (le32toh(prot_version_le) >> 0) & 0xff;

  uint8_t protocol_version_format = (le32toh(prot_version_le) >> 24) & 0xff;
  printf("File system format: %u\n", protocol_version_format);
  printf("Protocol Version: %u.%u.%u\n", major, minor, patch);
  // if (((NVM3_800s_PRIOR_719 == nvm_layout) && (minor != 18)) || ((NVM3_800s_FROM_719 == nvm_layout) && (minor < 19)))
  // {
  //   close_controller_nvm();
  //   return check_pass;
  // }

  nvm3_files_v5_800s_719_720 = (hardware_info == EFR32XG28) ? (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_719_720_xg28 : (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_719_720_zgm230;
  nvm3_files_v5_800s_721 = (hardware_info == EFR32XG28) ? (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_721_xg28 : (nvm3_file_descriptor_t *)&nvm3_files_v5_800s_721_zgm230;

  // if (protocol_version_format == 0)
  // {
  //   check_pass = nvm3_check_files(&nvm3_protocol_handle, nvm3_protocol_files_v0, sizeof_array(nvm3_protocol_files_v0));
  // }
  // else if (protocol_version_format == 1)
  // {
  //   check_pass = nvm3_check_files(&nvm3_protocol_handle, nvm3_protocol_files_v1, sizeof_array(nvm3_protocol_files_v1));
  // }
  // else if (protocol_version_format == 2)
  // {
  //   check_pass = nvm3_check_files(&nvm3_protocol_handle, nvm3_protocol_files_v2, sizeof_array(nvm3_protocol_files_v2));
  // }
  // else if (protocol_version_format == 3)
  // {
  //   check_pass = nvm3_check_files(&nvm3_protocol_handle, nvm3_protocol_files_v3, sizeof_array(nvm3_protocol_files_v3));
  // }
  // else if (protocol_version_format == 4)
  // {
  //   check_pass = nvm3_check_files(&nvm3_protocol_handle, nvm3_protocol_files_v4, sizeof_array(nvm3_protocol_files_v4));
  // }
  // else if (protocol_version_format == 5)
  // {
  //   if (app_nvm_is_v7_19(major, minor, patch) || app_nvm_is_v7_20(major, minor, patch))
  //   {
  //     check_pass = nvm3_check_files(&nvm3_protocol_handle, nvm3_files_v5_800s_719_720, sizeof_array(nvm3_files_v5_800s_719_720));
  //   }
  //   else
  //   {
  //     check_pass = nvm3_check_files(&nvm3_protocol_handle, nvm3_files_v5_800s_721, sizeof_array(nvm3_files_v5_800s_721));
  //   }
  // }
  // else
  // {
  //   user_message(MSG_ERROR, "ERROR: Conversion of protocol file system v:%u is not supported\n", protocol_version_format);
  //   check_pass = false;
  // }

  // if (nvm_layout != NVM3_800s_FROM_719 && (nvm_layout != NVM3_800s_PRIOR_719 && minor >= 19))
  // {
  //   if (app_nvm_is_pre_v7_15_3(major, minor, patch))
  //   {
  //     check_pass &= nvm3_check_files(&nvm3_app_handle, nvm3_app_files_prior_7_15_3, sizeof_array(nvm3_app_files_prior_7_15_3));
  //   }
  //   else if (app_nvm_is_pre_v7_18_1(major, minor, patch))
  //   {
  //     check_pass &= nvm3_check_files(&nvm3_app_handle, nvm3_app_files_prior_7_18_1, sizeof_array(nvm3_app_files_prior_7_18_1));
  //   }
  //   else
  //   {
  //     check_pass &= nvm3_check_files(&nvm3_app_handle, nvm3_app_files, sizeof_array(nvm3_app_files));
  //   }
  // }

  close_controller_nvm();
  return check_pass;
}

/*****************************************************************************/
bool open_controller_nvm(const uint8_t *nvm_image, size_t nvm_image_size, nvmLayout_t nvm_layout)
{
  Ecode_t ec;
  size_t flash_page_size;
  uint32_t app_nvm_size;
  uint32_t protocol_nvm_size;
  uint32_t total_nvm_size;

  switch (nvm_layout)
  {
  case NVM3_700s:
    flash_page_size = FLASH_PAGE_SIZE_700s;
    app_nvm_size = APP_NVM_SIZE_700s;
    protocol_nvm_size = PROTOCOL_NVM_SIZE_700s;
    break;

  case NVM3_800s_PRIOR_719:
    flash_page_size = FLASH_PAGE_SIZE_800s;
    app_nvm_size = APP_NVM_SIZE_800s_PRIOR_719;
    protocol_nvm_size = PROTOCOL_NVM_SIZE_800s_PRIOR_719;
    break;

  case NVM3_800s_FROM_719:
    flash_page_size = FLASH_PAGE_SIZE_800s;
    break;

  default: /* Optional */
    user_message(MSG_ERROR, "ERROR: nvm_layout not recognized\n");
    return false;
  }

  if (nvm_layout != NVM3_800s_FROM_719)
    total_nvm_size = app_nvm_size + protocol_nvm_size;
  else
    total_nvm_size = NVM_SIZE_800s_FROM_719;

  // Align on FLASH_PAGE_SIZE boundary
  nvm3_storage_address = (nvm3_HalPtr_t)(((size_t)nvm3_storage & ~(flash_page_size - 1)) + flash_page_size);
  memset(nvm3_storage_address, 0xff, total_nvm_size);

  halHandle = &nvm3_halRamHandle;
  halConfig = &nvm3_halRamConfig;

  nvm3_halSetPageSize(halConfig, flash_page_size);

  ec = nvm3_halOpen(halHandle, nvm3_storage_address, total_nvm_size);
  if (ECODE_NVM3_OK != ec)
  {
    user_message(MSG_ERROR, "ERROR: nvm3_halOpen() returned 0x%x\n", ec);
    return false;
  }

  if (NULL != nvm_image)
  {
    ec = nvm3_halRamSetBin(nvm_image, nvm_image_size);
    if (ECODE_NVM3_OK != ec)
    {
      user_message(MSG_ERROR, "ERROR: nvm3_halSetBin() returned 0x%x\n", ec);
      return false;
    }
  }

  if (nvm_layout != NVM3_800s_FROM_719)
  {
    nvm3_HalPtr_t nvm3_app_address = nvm3_storage_address;
    nvm3_HalPtr_t nvm3_protocol_address = ((uint8_t *)nvm3_app_address) + app_nvm_size;

    nvm3_Init_t nvm3_app_init = (nvm3_Init_t){
        .nvmAdr = nvm3_app_address,
        .nvmSize = app_nvm_size,
        .cachePtr = nvm3_app_cache,
        .cacheEntryCount = sizeof(nvm3_app_cache) / sizeof(nvm3_CacheEntry_t),
        .maxObjectSize = NVM3_MAX_OBJECT_SIZE,
        .repackHeadroom = 0,
        .halHandle = halHandle};

    nvm3_Init_t nvm3_protocol_init = (nvm3_Init_t){
        .nvmAdr = nvm3_protocol_address,
        .nvmSize = protocol_nvm_size,
        .cachePtr = nvm3_protocol_cache,
        .cacheEntryCount = sizeof(nvm3_protocol_cache) / sizeof(nvm3_CacheEntry_t),
        .maxObjectSize = NVM3_MAX_OBJECT_SIZE,
        .repackHeadroom = 0,
        .halHandle = halHandle};

    Ecode_t ec_app = nvm3_open(&nvm3_app_handle, &nvm3_app_init);
    Ecode_t ec_prot = nvm3_open(&nvm3_protocol_handle, &nvm3_protocol_init);

    if ((ECODE_NVM3_OK != ec_app) || (ECODE_NVM3_OK != ec_prot))
    {
      user_message(MSG_ERROR, "nvm3_open(app_nvm) returned 0x%x\n"
                              "nvm3_open(protocol_nvm) returned 0x%x\n",
                   ec_app, ec_prot);
      return false;
    }
  }
  else
  {
    nvm3_Init_t nvm3_protocol_init = (nvm3_Init_t){
        .nvmAdr = nvm3_storage_address,
        .nvmSize = total_nvm_size,
        .cachePtr = nvm3_protocol_cache,
        .cacheEntryCount = sizeof(nvm3_protocol_cache) / sizeof(nvm3_CacheEntry_t),
        .maxObjectSize = NVM3_MAX_OBJECT_SIZE,
        .repackHeadroom = 0,
        .halHandle = halHandle};
    Ecode_t ec_nvm = nvm3_open(&nvm3_protocol_handle, &nvm3_protocol_init);

    if ((ECODE_NVM3_OK != ec_nvm))
    {
      user_message(MSG_ERROR, "nvm3_open(stack_nvm) returned 0x%x\n",
                   ec_nvm);
      return false;
    }
  }

  return true;
}

/*****************************************************************************/
size_t get_controller_nvm_image(uint8_t **image_buf, nvmLayout_t nvm_layout)
{
  if (nvm_layout != NVM3_800s_FROM_719)
  {
    if (nvm3_app_handle.hasBeenOpened && nvm3_protocol_handle.hasBeenOpened)
    {
      return nvm3_halRamGetBin(image_buf);
    }
  }
  else
  {
    if (nvm3_protocol_handle.hasBeenOpened)
    {
      int ret = nvm3_halRamGetBin(image_buf);
      if (ret > 0)
      {
        return nvm3_protocol_handle.nvmSize;
      }
    }
  }
  return 0;
}

/*****************************************************************************/
void close_controller_nvm()
{
  if (nvm3_app_handle.hasBeenOpened)
  {
    nvm3_close(&nvm3_app_handle);
    memset(&nvm3_app_handle, 0, sizeof(nvm3_app_handle));
  }

  if (nvm3_protocol_handle.hasBeenOpened)
  {
    nvm3_close(&nvm3_protocol_handle);
    memset(&nvm3_protocol_handle, 0, sizeof(nvm3_protocol_handle));
  }

  nvm3_storage_address = 0;
}

/*****************************************************************************/
void dump_controller_nvm_keys(void)
{
  if (nvm3_app_handle.hasBeenOpened)
  {
    printf("## Nvm3 object keys in application instance:\n");

    if (app_nvm_is_pre_v7_15_3(target_protocol_version.major, target_protocol_version.minor, target_protocol_version.patch))
    {
      nvm3_dump_keys_with_filename(&nvm3_app_handle,
                                   nvm3_app_files_prior_7_15_3,
                                   sizeof_array(nvm3_app_files_prior_7_15_3));
    }
    else if (app_nvm_is_pre_v7_18_1(target_protocol_version.major, target_protocol_version.minor, target_protocol_version.patch))
    {
      nvm3_dump_keys_with_filename(&nvm3_app_handle,
                                   nvm3_app_files_prior_7_18_1,
                                   sizeof_array(nvm3_app_files_prior_7_18_1));
    }
    else
    {
      nvm3_dump_keys_with_filename(&nvm3_app_handle,
                                   nvm3_app_files,
                                   sizeof_array(nvm3_app_files));
    }
  }
  if (nvm3_protocol_handle.hasBeenOpened)
  {
    printf("## Nvm3 object keys in protocol instance:\n");
    nvm3_dump_keys_with_filename(&nvm3_protocol_handle,
                                 nvm3_current_protocol_files, nvm3_current_protocol_files_size);
  }
}

/*****************************************************************************/
/*******************************  NVM --> JSON  ******************************/
/*****************************************************************************/

/*****************************************************************************/
/* Read protocol version and application version for backing up data to json */
static void backup_info(nvmLayout_t nvm_layout)
{
  json_object *jo = json_object_new_object();
  READ_PROT_NVM(FILE_ID_ZW_VERSION, prot_version_le);
  target_protocol_version.format = (le32toh(prot_version_le) >> 24) & 0xff;
  target_protocol_version.major = (le32toh(prot_version_le) >> 16) & 0xff;
  target_protocol_version.minor = (le32toh(prot_version_le) >> 8) & 0xff;
  target_protocol_version.patch = (le32toh(prot_version_le) >> 0) & 0xff;
  if (NVM3_800s_FROM_719 == nvm_layout || (NVM3_800s_PRIOR_719 == nvm_layout && target_protocol_version.minor >= 19))
  {
    if (EFR32XG28 == hardware_info)
    {
      READ_PROT_NVM(ZAF_FILE_ID_APP_VERSION_800s_XG28, app_version_le);
    }
    else if (EFR32XG23 == hardware_info)
    {
      READ_PROT_NVM(ZAF_FILE_ID_APP_VERSION_800s_XG23, app_version_le);
    }
  }
  else
    READ_APP_NVM(ZAF_FILE_ID_APP_VERSION, app_version_le);
  target_app_version.format = (le32toh(app_version_le) >> 24) & 0xff;
  target_app_version.major = (le32toh(app_version_le) >> 16) & 0xff;
  target_app_version.minor = (le32toh(app_version_le) >> 8) & 0xff;
  target_app_version.patch = (le32toh(app_version_le) >> 0) & 0xff;
}

/*****************************************************************************/
static json_object *cmd_classes_to_json(SApplicationCmdClassInfo ai)
{

  json_object *jo = json_object_new_object();

  json_add_byte_array(jo, "includedInsecurely", ai.UnSecureIncludedCC, ai.UnSecureIncludedCCLen);
  // The following two are empty for a gateway
  json_add_byte_array(jo, "includedSecurelyInsecureCCs", ai.SecureIncludedUnSecureCC, ai.SecureIncludedUnSecureCCLen);
  json_add_byte_array(jo, "includedSecurelySecureCCs", ai.SecureIncludedSecureCC, ai.SecureIncludedSecureCCLen);
  return jo;
}

/*****************************************************************************/
static json_object *app_config_to_json(nvmLayout_t nvm_layout)
{
  Ecode_t ec;
  json_object *jo = json_object_new_object();
  if (app_nvm_is_pre_v7_15_3(target_app_version.major, target_app_version.minor, target_app_version.patch))
  {
    SApplicationConfiguration_prior_7_15_3 ac = {};
    ec = READ_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);

    if (ECODE_NVM3_OK == ec)
    {
      json_add_int(jo, "rfRegion", ac.rfRegion);
      json_add_int(jo, "txPower", ac.iTxPower);
      json_add_int(jo, "measured0dBm", ac.ipower0dbmMeasured);
    }
  }
  else if (app_nvm_is_pre_v7_18_1(target_app_version.major, target_app_version.minor, target_app_version.patch))
  {
    SApplicationConfiguration_prior_7_18_1 ac = {};
    ec = READ_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);

    if (ECODE_NVM3_OK == ec)
    {
      json_add_int(jo, "rfRegion", ac.rfRegion);
      json_add_int(jo, "txPower", ac.iTxPower);
      json_add_int(jo, "measured0dBm", ac.ipower0dbmMeasured);
      json_add_bool(jo, "enablePTI", ac.enablePTI);
      json_add_int(jo, "maxTxPower", ac.maxTxPower);
    }
  }
  else if (app_nvm_is_pre_v7_21(target_app_version.major, target_app_version.minor, target_app_version.patch))
  {
    SApplicationConfiguration_7_18_1 ac = {};
    if (nvm_layout == NVM3_800s_FROM_719 || (nvm_layout == NVM3_800s_PRIOR_719 && target_app_version.minor >= 19))
    {
      ec = READ_PROT_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
    }
    else
    {
      ec = READ_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
    }

    if (ECODE_NVM3_OK == ec)
    {
      json_add_int(jo, "rfRegion", ac.rfRegion);
      json_add_int(jo, "txPower", ac.iTxPower);
      json_add_int(jo, "measured0dBm", ac.ipower0dbmMeasured);
      json_add_bool(jo, "enablePTI", ac.enablePTI);
      json_add_int(jo, "maxTxPower", ac.maxTxPower);
    }
  }
  else
  {
    SApplicationConfiguration_7_21_x ac = {};
    if (nvm_layout == NVM3_800s_FROM_719 || (nvm_layout == NVM3_800s_PRIOR_719 && target_protocol_version.minor >= 19))
    {
      ec = READ_PROT_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
    }
    else
    {
      ec = READ_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
    }

    if (ECODE_NVM3_OK == ec)
    {
      json_add_int(jo, "rfRegion", ac.rfRegion);
      json_add_int(jo, "txPower", ac.iTxPower);
      json_add_int(jo, "measured0dBm", ac.ipower0dbmMeasured);
      json_add_bool(jo, "enablePTI", ac.enablePTI);
      json_add_int(jo, "maxTxPower", ac.maxTxPower);
      json_add_int(jo, "nodeIdType", ac.nodeIdBaseType);
    }
  }

  return jo;
}

/*****************************************************************************/
// static json_object* nvm_800s_from_721_config_to_json(void)
// {
//   json_object* jo = json_object_new_object();
//   nvm3_file_descriptor_t *fdesc;
//   int size = 0;

//   for (uint32_t i = 0; i < sizeof_array(nvm3_files_v5_800s_721); i++)
//   {
//     fdesc = (nvm3_file_descriptor_t*)&nvm3_files_v5_800s_721[i];
//     if (fdesc->key == FILE_ID_APPLICATIONCONFIGURATION) {
//       size = fdesc->size;
//       break;
//     }
//   }

//   if (size == 9) {
//     SApplicationConfiguration_7_21_x ac = {};
//     READ_PROT_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);

//     json_add_int(jo, "rfRegion",          ac.rfRegion);
//     json_add_int(jo, "txPower",           ac.iTxPower);
//     json_add_int(jo, "measured0dBm", ac.ipower0dbmMeasured);
//     json_add_bool(jo, "enablePTI", ac.enablePTI);
//     json_add_int(jo, "maxTxPower",        ac.maxTxPower);
//     json_add_int(jo, "nodeIdType",    ac.nodeIdBaseType);
//   } else if (size == 8) {
//     SApplicationConfiguration_7_18_1 ac = {};
//     READ_PROT_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);

//     json_add_int(jo, "rfRegion",          ac.rfRegion);
//     json_add_int(jo, "txPower",           ac.iTxPower);
//     json_add_int(jo, "measured0dBm", ac.ipower0dbmMeasured);
//     json_add_bool(jo, "enablePTI", ac.enablePTI);
//     json_add_int(jo, "maxTxPower",        ac.maxTxPower);
//   }
//   return jo;
// }

/*****************************************************************************/
static json_object *suc_node_list_to_json(void)
{
  SSucNodeList snl = {};

  READ_PROT_NVM(FILE_ID_SUCNODELIST, snl);

  json_object *jo_suc_node_list = json_object_new_array();

  for (int upd = 0; upd < SUC_MAX_UPDATES; upd++)
  {
    SUC_UPDATE_ENTRY_STRUCT *suc_entry = &snl.aSucNodeList[upd];
    json_object *jo_entry = json_object_new_object();
    if (suc_entry->NodeID == 0x00 || suc_entry->NodeID == 0xFF)
    {
      continue;
    }
    json_add_int(jo_entry, "nodeId", suc_entry->NodeID);
    // changeType: SUC_ADD(1), SUC_DELETE(2), SUC_UPDATE_RANGE(3)
    json_add_int(jo_entry, "changeType", suc_entry->changeType);

    json_object *jo_supportedCCs_list = json_object_new_array();
    json_object *jo_controlledCCs_list = json_object_new_array();
    bool is_after_mark = false;
    for (int npar = 0; npar < SUC_UPDATE_NODEPARM_MAX; npar++)
    {
      if (suc_entry->nodeInfo[npar] == 0xEF)
      {
        is_after_mark = true;
        continue;
      }
      if (0 != suc_entry->nodeInfo[npar])
      {
        is_after_mark ? json_object_array_add(jo_controlledCCs_list, json_object_new_int(suc_entry->nodeInfo[npar])) : json_object_array_add(jo_supportedCCs_list, json_object_new_int(suc_entry->nodeInfo[npar]));
      }
    }

    json_object_object_add(jo_entry, "supportedCCs", jo_supportedCCs_list);
    json_object_object_add(jo_entry, "controlledCCs", jo_controlledCCs_list);
    json_object_array_add(jo_suc_node_list, jo_entry);
  }

  return jo_suc_node_list;
}

/*****************************************************************************/
static json_object *suc_node_list_v5_to_json(void) // SUC_MAX_UPDATES /
{
  json_object *jo_suc_node_list = json_object_new_array();

  for (int num_key = 0; num_key < SUC_MAX_UPDATES / SUCNODES_PER_FILE; num_key++)
  {
    SSucNodeList_V5 snl = {};
    memset(&snl, 0, sizeof(SSucNodeList_V5));

    READ_PROT_NVM(FILE_ID_SUCNODELIST_BASE_V5 + num_key, snl);

    for (int upd = 0; upd < SUCNODES_PER_FILE; upd++)
    {
      SUC_UPDATE_ENTRY_STRUCT *suc_entry = &snl.aSucNodeList[upd];
      json_object *jo_entry = json_object_new_object();
      if (suc_entry->NodeID == 0x00 || suc_entry->NodeID == 0xFF)
      {
        continue;
      }

      json_add_int(jo_entry, "nodeId", suc_entry->NodeID);
      // changeType: SUC_ADD(1), SUC_DELETE(2), SUC_UPDATE_RANGE(3)
      json_add_int(jo_entry, "changeType", suc_entry->changeType);

      json_object *jo_supportedCCs_list = json_object_new_array();
      json_object *jo_controlledCCs_list = json_object_new_array();
      bool is_after_mark = false;
      for (int npar = 0; npar < SUC_UPDATE_NODEPARM_MAX; npar++)
      {
        if (suc_entry->nodeInfo[npar] == 0xEF)
        {
          is_after_mark = true;
          continue;
        }
        if (0 != suc_entry->nodeInfo[npar])
        {
          is_after_mark ? json_object_array_add(jo_controlledCCs_list, json_object_new_int(suc_entry->nodeInfo[npar])) : json_object_array_add(jo_supportedCCs_list, json_object_new_int(suc_entry->nodeInfo[npar]));
        }
      }
      json_object_object_add(jo_entry, "supportedCCs", jo_supportedCCs_list);
      json_object_object_add(jo_entry, "controlledCCs", jo_controlledCCs_list);
      json_object_array_add(jo_suc_node_list, jo_entry);
    }
  }
  return jo_suc_node_list;
}

/*****************************************************************************/
static json_object *route_cache_line_to_json(const ROUTECACHE_LINE *rcl)
{
  json_object *jo = json_object_new_object();

  switch (rcl->routecacheLineConfSize & 0x60)
  {
  case 0x00:
    json_add_bool(jo, "beaming", false);
    break;
  case 0x20:
    json_object_object_add(jo, "beaming", json_object_new_string("250ms"));
    break;
  case 0x40:
    json_object_object_add(jo, "beaming", json_object_new_string("1000ms"));
    break;
  default:
    // TODO impossible case?
    break;
  }
  json_add_int(jo, "protocolRate", (int)(rcl->routecacheLineConfSize & 0x03));

  json_add_byte_array(jo, "repeaterNodeIDs", rcl->repeaterList, MAX_REPEATERS);

  return jo;
}

/*****************************************************************************/
static json_object *node_table_to_json(nvmLayout_t nvm_layout)
{
  CLASSIC_NODE_MASK_TYPE node_info_exists = {};
  CLASSIC_NODE_MASK_TYPE bridge_node_flags = {};
  CLASSIC_NODE_MASK_TYPE suc_pending_update_flags = {};
  CLASSIC_NODE_MASK_TYPE pending_discovery_flags = {};

  CLASSIC_NODE_MASK_TYPE node_routecache_exists = {};
  CLASSIC_NODE_MASK_TYPE app_route_lock_flags = {};
  CLASSIC_NODE_MASK_TYPE route_slave_suc_flags = {};

  LR_NODE_MASK_TYPE lr_node_info_exists = {0};

  SNodeInfo node_info_buf[NODEINFOS_PER_FILE] = {};
  SNodeInfo *node_info;
  SLRNodeInfo lr_node_info_buf[LR_NODEINFOS_PER_FILE] = {};
  SLRNodeInfo *lr_node_info;
  uint8_t lr_tx_power_buf[FILE_SIZE_LR_TX_POWER] = {};
  uint8_t *lr_tx_power;

  SNodeRouteCache route_cache_buf[NODEROUTECACHES_PER_FILE] = {};
  SNodeRouteCache *route_cache;
  Ecode_t ec;

  READ_PROT_NVM(FILE_ID_NODE_STORAGE_EXIST, node_info_exists);
  if (target_protocol_version.format >= 2)
  {
    READ_PROT_NVM(FILE_ID_LRANGE_NODE_EXIST, lr_node_info_exists);
  }

  READ_PROT_NVM(FILE_ID_SUC_PENDING_UPDATE_FLAG, suc_pending_update_flags);
  READ_PROT_NVM(FILE_ID_BRIDGE_NODE_FLAG, bridge_node_flags);
  READ_PROT_NVM(FILE_ID_PENDING_DISCOVERY_FLAG, pending_discovery_flags);

  READ_PROT_NVM(FILE_ID_NODE_ROUTECACHE_EXIST, node_routecache_exists);
  READ_PROT_NVM(FILE_ID_APP_ROUTE_LOCK_FLAG, app_route_lock_flags);
  READ_PROT_NVM(FILE_ID_ROUTE_SLAVE_SUC_FLAG, route_slave_suc_flags);

  json_object *jo_node_list = json_object_new_object();

  for (uint8_t node_id = 1; node_id <= ZW_CLASSIC_MAX_NODES; node_id++)
  {
    if (ZW_NodeMaskNodeIn(node_info_exists, node_id))
    {
      /* FILE_ID_NODEINFO is the nvm3 object key for the nodeId 1 file
       * FILE_ID_NODEINFO+1 is the key for the nodeId 2 file, etc. */
      uint8_t node_index = node_id - 1;
      if (target_protocol_version.format >= 5)
      {
        ec = READ_PROT_NVM(FILE_ID_NODEINFO_BASE_V5 + node_index / NODEINFOS_PER_FILE, node_info_buf);
        node_info = &node_info_buf[node_index % NODEINFOS_PER_FILE];
      }
      else if (target_protocol_version.format > 0 &&
               target_protocol_version.format < 5)
      {
        ec = READ_PROT_NVM(FILE_ID_NODEINFO_V1 + node_index / NODEINFOS_PER_FILE, node_info_buf);
        node_info = &node_info_buf[node_index % NODEINFOS_PER_FILE];
      }
      else
      {
        ec = READ_PROT_NVM(FILE_ID_NODEINFO + node_index, node_info_buf[0]);
        node_info = &node_info_buf[0];
      }

      if (ECODE_NVM3_OK == ec)
      {
        json_object *jo_node_info = json_object_new_object();
        json_object *jo_node = json_object_new_object();

        json_add_bool(jo_node, "isVirtual", ZW_NodeMaskNodeIn(bridge_node_flags, node_id) ? true : false);
        uint8_t node_capability = node_info->NodeInfo.capability;
        uint8_t node_reserved = node_info->NodeInfo.reserved;
        uint8_t node_security = node_info->NodeInfo.security;
        // isListening
        json_add_bool(jo_node, "isListening", (node_capability & ZWAVE_NODEINFO_LISTENING_SUPPORT) != 0);
        // isFrequentlyListening
        switch (node_security & ZWAVE_NODEINFO_SENSOR_MODE_MASK)
        {
        case ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250:
          json_object_object_add(jo_node, "isFrequentListening", json_object_new_string("250ms"));
          break;
        case ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000:
          json_object_object_add(jo_node, "isFrequentListening", json_object_new_string("1000ms"));
          break;
        default:
          json_add_bool(jo_node, "isFrequentListening", false);
          break;
        }
        // isRouting
        json_add_bool(jo_node, "isRouting", (node_capability & ZWAVE_NODEINFO_ROUTING_SUPPORT) != 0);
        // data_rate
        json_object *jo_data_rate = json_object_new_array();
        if (ZWAVE_NODEINFO_BAUD_9600 & node_capability)
          json_object_array_add(jo_data_rate, json_object_new_int(9600));
        if (ZWAVE_NODEINFO_BAUD_40000 & node_capability)
          json_object_array_add(jo_data_rate, json_object_new_int(40000));
        if (ZWAVE_NODEINFO_BAUD_100K & node_reserved)
          json_object_array_add(jo_data_rate, json_object_new_int(100000));
        json_object_object_add(jo_node, "supportedDataRates", jo_data_rate);
        // protocolVersion
        json_add_int(jo_node, "protocolVersion", (int32_t)(node_capability & ZWAVE_NODEINFO_PROTOCOL_VERSION_MASK));
        // optionalFunctionality
        json_add_bool(jo_node, "optionalFunctionality", (node_security & ZWAVE_NODEINFO_OPTIONAL_FUNC) != 0);
        // nodeType
        int nodeType = 0;
        if ((node_security & 0x0A) == 0x02)
          nodeType = 0;
        else if ((node_security & 0x0A) == 0x08)
          nodeType = 1;
        json_add_int(jo_node, "nodeType", nodeType);
        // supportsSecurity
        json_add_bool(jo_node, "supportsSecurity", (ZWAVE_NODEINFO_SECURITY_SUPPORT & node_security) != 0);
        // supportBeaming
        json_add_bool(jo_node, "supportsBeaming", (ZWAVE_NODEINFO_BEAM_CAPABILITY & node_security) != 0);
        json_add_int(jo_node, "genericDeviceClass", node_info->NodeInfo.generic);
        json_add_int(jo_node, "specificDeviceClass", node_info->NodeInfo.specific);

        json_add_nodemask(jo_node, "neighbours", node_info->neighboursInfo);
        // Index into SUC updates table or one of: 0, SUC_UNKNOWN_CONTROLLER(0xFE), SUC_OUT_OF_DATE(0xFF)
        json_add_int(jo_node, "sucUpdateIndex", node_info->ControllerSucUpdateIndex);
        json_add_bool(jo_node, "appRouteLock", ZW_NodeMaskNodeIn(app_route_lock_flags, node_id) ? true : false);
        json_add_bool(jo_node, "routeSlaveSuc", ZW_NodeMaskNodeIn(route_slave_suc_flags, node_id) ? true : false);
        json_add_bool(jo_node, "sucPendingUpdate", ZW_NodeMaskNodeIn(suc_pending_update_flags, node_id) ? true : false);
        json_add_bool(jo_node, "pendingDiscovery", ZW_NodeMaskNodeIn(pending_discovery_flags, node_id) ? true : false);

        /* ---- ROUTE CACHE BEGIN ---- */
        if (ZW_NodeMaskNodeIn(node_routecache_exists, node_id))
        {
          int addr = 0;
          if (target_protocol_version.format >= 5)
          {
            addr = FILE_ID_NODEROUTECAHE_BASE_V5 + node_index / NODEROUTECACHES_PER_FILE;
            ec = READ_PROT_NVM(FILE_ID_NODEROUTECAHE_BASE_V5 + node_index / NODEROUTECACHES_PER_FILE, route_cache_buf);
            route_cache = &route_cache_buf[node_index % NODEROUTECACHES_PER_FILE];
          }
          else if (target_protocol_version.format > 0 &&
                   target_protocol_version.format < 5)
          {
            addr = FILE_ID_NODEROUTECAHE_V1 + node_index / NODEROUTECACHES_PER_FILE;
            ec = READ_PROT_NVM(FILE_ID_NODEROUTECAHE_V1 + node_index / NODEROUTECACHES_PER_FILE, route_cache_buf);
            route_cache = &route_cache_buf[node_index % NODEROUTECACHES_PER_FILE];
          }
          else
          {
            addr = FILE_ID_NODEROUTECAHE + node_index;
            ec = READ_PROT_NVM(FILE_ID_NODEROUTECAHE + node_index, route_cache_buf[0]);
            route_cache = &route_cache_buf[0];
          }

          // if (ECODE_NVM3_OK == ec)
          // {
          json_object_object_add(jo_node, "lwr", route_cache_line_to_json(&route_cache->routeCache));
          json_object_object_add(jo_node, "nlwr", route_cache_line_to_json(&route_cache->routeCacheNlwrSr));
          // }
        }
        else
        {
          json_object_object_add(jo_node, "lwr", json_object_new_null());
          json_object_object_add(jo_node, "nlwr", json_object_new_null());
        }

        /* ---- ROUTE CACHE END ---- */
        char node_id_string[10];
        snprintf(node_id_string, sizeof(node_id_string), "%d", node_id);
        json_object_object_add(jo_node_list, node_id_string, jo_node);
        // json_object_array_add(jo_node_list, jo_node_info);
      }
    }
  }
  return jo_node_list;
}
/*****************************************************************************/
static json_object *lr_node_table_to_json(nvmLayout_t nvm_layout)
{
  LR_NODE_MASK_TYPE lr_node_info_exists = {0};

  SLRNodeInfo lr_node_info_buf[LR_NODEINFOS_PER_FILE] = {};
  SLRNodeInfo *lr_node_info;
  uint8_t lr_tx_power_buf[FILE_SIZE_LR_TX_POWER] = {};
  uint8_t *lr_tx_power;
  Ecode_t ec;

  if (target_protocol_version.format >= 2)
  {
    READ_PROT_NVM(FILE_ID_LRANGE_NODE_EXIST, lr_node_info_exists);
  }

  json_object *jo_lr_node_list = json_object_new_object();

  for (uint16_t node_id = 1; node_id <= ZW_LR_MAX_NODES; node_id++)
  {
    if (ZW_NodeMaskNodeIn(lr_node_info_exists, node_id))
    {
      uint16_t node_index = node_id - 1;
      json_object *jo_lr_node = json_object_new_object();
      /* LR node info */
      if (target_protocol_version.format < 5)
        ec = READ_PROT_NVM(FILE_ID_NODEINFO_LR + node_index / LR_NODEINFOS_PER_FILE, lr_node_info_buf);
      else
        ec = READ_PROT_NVM(FILE_ID_NODEINFO_LR_BASE_V5 + node_index / LR_NODEINFOS_PER_FILE, lr_node_info_buf);
      lr_node_info = &lr_node_info_buf[node_index % LR_NODEINFOS_PER_FILE];
      if (ECODE_NVM3_OK == ec)
      {
        json_add_bool(jo_lr_node, "isListening", lr_node_info->packedInfo & NODEINFO_FLAG_LISTENING);
        json_add_bool(jo_lr_node, "isRouting", lr_node_info->packedInfo & NODEINFO_FLAG_ROUTING);
        switch (lr_node_info->packedInfo & NODEINFO_MASK_SENSOR)
        {
        case ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250:
          json_object_object_add(jo_lr_node, "isFrequentListening", json_object_new_string("250ms"));
          break;
        case ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000:
          json_object_object_add(jo_lr_node, "isFrequentListening", json_object_new_string("1000ms"));
          break;
        default:
          json_add_bool(jo_lr_node, "isFrequentListening", false);
          break;
        }
        json_object *jo_data_rate = json_object_new_array();
        json_object_array_add(jo_data_rate, json_object_new_int(100000));
        json_object_object_add(jo_lr_node, "supportedDataRates", jo_data_rate);
        json_add_int(jo_lr_node, "protocolVersion", ZWAVE_NODEINFO_VERSION_4);
        json_add_bool(jo_lr_node, "optionalFunctionality", lr_node_info->packedInfo & NODEINFO_FLAG_OPTIONAL);
        // There's only 1 controller in Z-Wave LR so nodeType is always 1 - End Node
        json_add_int(jo_lr_node, "nodeType", 1);
        json_add_bool(jo_lr_node, "supportsBeaming", lr_node_info->packedInfo & NODEINFO_FLAG_BEAM);
        // Security support is mandatory
        json_add_bool(jo_lr_node, "supportsSecurity", true);
        json_add_int(jo_lr_node, "genericDeviceClass", lr_node_info->generic);
        json_add_int(jo_lr_node, "specificDeviceClass", lr_node_info->specific);
      }

      /* LR Tx Power */
      if (target_protocol_version.format == 2)
      {
        ec = READ_PROT_NVM(FILE_ID_LR_TX_POWER_V2 + node_index / LR_TX_POWER_PER_FILE_V2, lr_tx_power_buf);
        lr_tx_power = &lr_tx_power_buf[(node_index % LR_TX_POWER_PER_FILE_V2) / 2];
        if (ECODE_NVM3_OK == ec)
        {
          if ((node_id - 1) & 1)
          {
            json_add_int(jo_lr_node, "lrTxPower", (int32_t)(*lr_tx_power & 0xf0));
          }
          else
          {
            json_add_int(jo_lr_node, "lrTxPower", (int32_t)(*lr_tx_power & 0x0f));
          }
        }
      }
      if (target_protocol_version.format == 3)
      {
        ec = READ_PROT_NVM(FILE_ID_LR_TX_POWER_V3 + node_index / LR_TX_POWER_PER_FILE_V3, lr_tx_power_buf);
        lr_tx_power = &lr_tx_power_buf[node_index % LR_TX_POWER_PER_FILE_V3];
        if (ECODE_NVM3_OK == ec)
        {
          json_add_int(jo_lr_node, "txPower", (int32_t)(*lr_tx_power));
        }
      }
      uint16_t lr_node_id = node_index + ZW_LR_MIN_NODE_ID;
      char node_id_string[10];
      snprintf(node_id_string, sizeof(node_id_string), "%d", lr_node_id);
      json_object_object_add(jo_lr_node_list, node_id_string, jo_lr_node);
    }
  }
  return jo_lr_node_list;
}
/*****************************************************************************/
static json_object *nvm711_controller_info_to_json(nvmLayout_t nvm_layout)
{
  json_object *jo = json_object_new_object();

  SControllerInfo_NVM711 ci = {};
  SApplicationCmdClassInfo ai = {};
  SApplicationData ad = {};
  CONTROLLER_CONFIGURATION controller_configuration = {};
  SApplicationSettings as = {};

  Ecode_t ec_ci;
  Ecode_t ec_ai;
  Ecode_t ec_ad;
  Ecode_t ec_as;

  ec_ci = READ_PROT_NVM(FILE_ID_CONTROLLERINFO, ci);
  ec_ai = READ_APP_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
  ec_ad = READ_APP_NVM(FILE_ID_APPLICATIONDATA, ad);
  ec_as = READ_APP_NVM(FILE_ID_APPLICATIONSETTINGS, as);

  json_object_object_add(jo, "protocolVersion", version_to_json(le32toh(prot_version_le)));
  json_object_object_add(jo, "applicationVersion", version_to_json(le32toh(app_version_le)));

  if (ECODE_NVM3_OK == ec_ci)
  {
    json_object_object_add(jo, "homeId", home_id_to_json(ci.HomeID));
    json_add_int(jo, "nodeId", ci.NodeID);
    json_add_int(jo, "staticControllerNodeId", ci.StaticControllerNodeId);
    // json_object_object_add(jo, "learnedHomeId", home_id_to_json(ci.HomeID));
    // json_add_int(jo, "lastNodeIdLR", ci.LastUsedNodeId_LR);
    json_add_int(jo, "lastNodeId", ci.LastUsedNodeId);
    json_add_int(jo, "sucLastIndex", ci.SucLastIndex); // Moved to SUC
    // json_add_int(jo, "controllerConfiguration", ci.ControllerConfiguration);
    // json_add_bool(jo, "sucAwarenessPushNeeded", ci.SucAwarenessPushNeeded); // Currently not used. Don't save to JSON
    // json_add_int(jo, "maxNodeIdLR", ci.MaxNodeId_LR);  // Don't save. Will be derived when reading the node table
    memcpy(&controller_configuration, &ci.ControllerConfiguration, sizeof(CONTROLLER_CONFIGURATION));
    //  "controllerIsSecondary": 0,
    json_add_bool(jo, "controllerIsSecondary", controller_configuration.controller_is_secondary);
    json_add_bool(jo, "controllerOnOtherNetwork", controller_configuration.controller_on_other_network);
    json_add_bool(jo, "controllerNodeIdServerPresent", controller_configuration.controller_nodeid_server_present);
    json_add_bool(jo, "controllerIsRealPrimary", controller_configuration.controller_is_real_primary);
    json_add_bool(jo, "controllerIsSuc", controller_configuration.controller_is_suc);
    json_add_bool(jo, "noNodesIncluded", controller_configuration.no_nodes_included);

    json_add_int(jo, "maxNodeId", ci.MaxNodeId); // Don't save. Will be derived when reading the node table
    // json_add_int(jo, "reservedIdLR", ci.ReservedId_LR); // Currently not used. Don't save to JSON
    json_add_int(jo, "reservedId", ci.ReservedId); // Currently not used. Don't save to JSON
    json_add_int(jo, "systemState", ci.SystemState);
    // json_add_int(jo, "primaryLongRangeChannelId", ci.PrimaryLongRangeChannelId);
    // json_add_int(jo, "longRangeChannelAutoMode", ci.LongRangeChannelAutoMode);
  }

  if (ECODE_NVM3_OK == ec_as)
  {
    json_add_bool(jo, "isListening", as.listening);
    json_add_int(jo, "genericDeviceClass", as.generic);
    json_add_int(jo, "specificDeviceClass", as.specific);
  }

  if (ECODE_NVM3_OK == ec_ai)
    json_object_object_add(jo, "cmdClasses", cmd_classes_to_json(ai));
  json_object_object_add(jo, "rfConfig", app_config_to_json(nvm_layout));
  if (ECODE_NVM3_OK == ec_ad)
  {
    /* Search backwards in the application data section to skip trailing zero
     * values (typically the data is stored at the beginning leaving a large
     * unused section at the end - there's no need to save all those zeros
     * to JSON)
     */
    int n = APPL_DATA_FILE_SIZE;
    while ((n > 0) && (0 == ad.extNvm[n - 1]))
    {
      --n;
    }

    if (n > 0)
    {
      char *b64_encoded_string = base64_encode(ad.extNvm, n, 0);
      if (b64_encoded_string)
      {
        json_add_string(jo, "applicationData", b64_encoded_string);
        free(b64_encoded_string);
      }
    }
  }

  return jo;
}

/*****************************************************************************/
static json_object *nvm715_controller_info_to_json(nvmLayout_t nvm_layout)
{
  json_object *jo = json_object_new_object();

  SControllerInfo_NVM715 ci = {};
  SApplicationCmdClassInfo ai = {};
  SApplicationData ad = {};
  CONTROLLER_CONFIGURATION controller_configuration = {};
  SApplicationSettings as = {};

  Ecode_t ec_ci;
  Ecode_t ec_ai;
  Ecode_t ec_ad;
  Ecode_t ec_as;

  ec_ci = READ_PROT_NVM(FILE_ID_CONTROLLERINFO, ci);
  ec_ai = READ_APP_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
  ec_ad = READ_APP_NVM(FILE_ID_APPLICATIONDATA, ad);
  ec_as = READ_APP_NVM(FILE_ID_APPLICATIONSETTINGS, as);

  json_object_object_add(jo, "protocolVersion", version_to_json(le32toh(prot_version_le)));
  json_object_object_add(jo, "applicationVersion", version_to_json(le32toh(app_version_le)));

  if (ECODE_NVM3_OK == ec_ci)
  {
    json_object_object_add(jo, "homeId", home_id_to_json(ci.HomeID));
    json_add_int(jo, "nodeId", ci.NodeID);
    json_add_int(jo, "staticControllerNodeId", ci.StaticControllerNodeId);
    // json_object_object_add(jo, "learnedHomeId", home_id_to_json(ci.HomeID));
    json_add_int(jo, "lastNodeId", ci.LastUsedNodeId);
    json_add_int(jo, "lastNodeIdLR", ci.LastUsedNodeId_LR);
    json_add_int(jo, "sucLastIndex", ci.SucLastIndex); // Moved to SUC
    // json_add_int(jo, "controllerConfiguration", ci.ControllerConfiguration);
    // json_add_bool(jo, "sucAwarenessPushNeeded", ci.SucAwarenessPushNeeded); // Currently not used. Don't save to JSON
    memcpy(&controller_configuration, &ci.ControllerConfiguration, sizeof(CONTROLLER_CONFIGURATION));
    //  "controllerIsSecondary": 0,
    json_add_bool(jo, "controllerIsSecondary", controller_configuration.controller_is_secondary);
    json_add_bool(jo, "controllerOnOtherNetwork", controller_configuration.controller_on_other_network);
    json_add_bool(jo, "controllerNodeIdServerPresent", controller_configuration.controller_nodeid_server_present);
    json_add_bool(jo, "controllerIsRealPrimary", controller_configuration.controller_is_real_primary);
    json_add_bool(jo, "controllerIsSuc", controller_configuration.controller_is_suc);
    json_add_bool(jo, "noNodesIncluded", controller_configuration.no_nodes_included);
    json_add_int(jo, "maxNodeId", ci.MaxNodeId);      // Don't save. Will be derived when reading the node table
    json_add_int(jo, "maxNodeIdLR", ci.MaxNodeId_LR); // Don't save. Will be derived when reading the node table

    json_add_int(jo, "reservedId", ci.ReservedId);      // Currently not used. Don't save to JSON
    json_add_int(jo, "reservedIdLR", ci.ReservedId_LR); // Currently not used. Don't save to JSON
    json_add_int(jo, "systemState", ci.SystemState);
    json_add_int(jo, "primaryLongRangeChannelId", ci.PrimaryLongRangeChannelId);
  }

  if (ECODE_NVM3_OK == ec_as)
  {
    json_add_bool(jo, "isListening", as.listening);
    json_add_int(jo, "genericDeviceClass", as.generic);
    json_add_int(jo, "specificDeviceClass", as.specific);
  }
  if (ECODE_NVM3_OK == ec_ai)
    json_object_object_add(jo, "cmdClasses", cmd_classes_to_json(ai));
  json_object_object_add(jo, "rfConfig", app_config_to_json(nvm_layout));

  if (ECODE_NVM3_OK == ec_ad)
  {
    /* Search backwards in the application data section to skip trailing zero
     * values (typically the data is stored at the beginning leaving a large
     * unused section at the end - there's no need to save all those zeros
     * to JSON)
     */
    int n = APPL_DATA_FILE_SIZE;
    while ((n > 0) && (0 == ad.extNvm[n - 1]))
    {
      --n;
    }

    if (n > 0)
    {
      char *b64_encoded_string = base64_encode(ad.extNvm, n, 0);
      if (b64_encoded_string)
      {
        json_add_string(jo, "applicationData", b64_encoded_string);
        free(b64_encoded_string);
      }
    }
    else
    {
      json_add_string(jo, "applicationData", "");
    }
  }

  if (ECODE_NVM3_OK == ec_ci)
  {
    json_add_int(jo, "dcdcConfig", ci.DcdcConfig);
  }
  json_object_object_add(jo, "sucUpdateEntries", suc_node_list_to_json());

  /* node info in the node table */

  return jo;
}

/*****************************************************************************/
static json_object *nvm_from_719_controller_info_to_json(nvmLayout_t nvm_layout)
{
  json_object *jo = json_object_new_object();
  Ecode_t ec_ci;
  Ecode_t ec_ai;
  Ecode_t ec_ad;
  Ecode_t ec_as;
  Ecode_t ec_an;
  Ecode_t ec_dcdc;
  SControllerInfo_NVM719 ci = {};
  SApplicationCmdClassInfo ai = {};
  SApplicationData ad = {};
  SApplicationSettings as = {};
  CONTROLLER_CONFIGURATION controller_configuration = {};
  dcdc_configuration_file_t dcdc = {};
  char app_name[ZAF_FILE_SIZE_APP_NAME];
  int FILE_ID_APP_NAME_800s = (hardware_info == EFR32XG23) ? ZAF_FILE_ID_APP_NAME_800s_XG23 : ZAF_FILE_ID_APP_NAME_800s_XG28;

  ec_ci = READ_PROT_NVM(FILE_ID_CONTROLLERINFO, ci);

  if (nvm_layout == NVM3_700s)
  {
    ec_ai = READ_APP_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
    ec_ad = READ_APP_NVM(FILE_ID_APPLICATIONDATA, ad);
    ec_as = READ_APP_NVM(FILE_ID_APPLICATIONSETTINGS, as);
    ec_dcdc = READ_APP_NVM(FILE_ID_PROPRIETARY_1, dcdc);
  }
  else
  {
    ec_ai = READ_PROT_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
    ec_ad = READ_PROT_NVM(FILE_ID_APPLICATIONDATA, ad);
    ec_as = READ_PROT_NVM(FILE_ID_APPLICATIONSETTINGS, as);
    ec_dcdc = READ_PROT_NVM(FILE_ID_PROPRIETARY_1, dcdc);
  }

  json_object_object_add(jo, "protocolVersion", version_to_json(le32toh(prot_version_le)));
  json_object_object_add(jo, "applicationVersion", version_to_json(le32toh(app_version_le)));

  if (target_protocol_version.major == 7 && target_protocol_version.minor >= 20)
  {
    if (nvm_layout == NVM3_700s)
    {
      ec_an = READ_APP_NVM(ZAF_FILE_ID_APP_NAME, app_name);
    }
    else
    {
      ec_an = READ_PROT_NVM(FILE_ID_APP_NAME_800s, app_name);
    }
    if (ECODE_NVM3_OK == ec_an)
    {
      json_object_object_add(jo, "applicationName", json_object_new_string(app_name));
    }
  }
  // Adding Controller Information
  if (ECODE_NVM3_OK == ec_ci)
  {
    json_object_object_add(jo, "homeId", home_id_to_json(ci.HomeID));
    json_add_int(jo, "nodeId", ci.NodeID);
    json_add_int(jo, "staticControllerNodeId", ci.StaticControllerNodeId);
    // json_object_object_add(jo, "learnedHomeId", home_id_to_json(ci.HomeID));
    json_add_int(jo, "lastNodeId", ci.LastUsedNodeId);
    json_add_int(jo, "lastNodeIdLR", ci.LastUsedNodeId_LR);
    json_add_int(jo, "sucLastIndex", ci.SucLastIndex); // Moved to SUC
    // json_add_int(jo,  "controllerConfiguration", ci.ControllerConfiguration);
    // json_add_bool(jo, "sucAwarenessPushNeeded", ci.SucAwarenessPushNeeded); // Currently not used. Don't save to JSON
    memcpy(&controller_configuration, &ci.ControllerConfiguration, sizeof(CONTROLLER_CONFIGURATION));
    //  "controllerIsSecondary": 0,
    json_add_bool(jo, "controllerIsSecondary", controller_configuration.controller_is_secondary);
    json_add_bool(jo, "controllerOnOtherNetwork", controller_configuration.controller_on_other_network);
    json_add_bool(jo, "controllerNodeIdServerPresent", controller_configuration.controller_nodeid_server_present);
    json_add_bool(jo, "controllerIsRealPrimary", controller_configuration.controller_is_real_primary);
    json_add_bool(jo, "controllerIsSuc", controller_configuration.controller_is_suc);
    json_add_bool(jo, "noNodesIncluded", controller_configuration.no_nodes_included);
    json_add_int(jo, "maxNodeId", ci.MaxNodeId);        // Don't save. Will be derived when reading the node table
    json_add_int(jo, "maxNodeIdLR", ci.MaxNodeId_LR);   // Don't save. Will be derived when reading the node table
    json_add_int(jo, "reservedId", ci.ReservedId);      // Currently not used. Don't save to JSON
    json_add_int(jo, "reservedIdLR", ci.ReservedId_LR); // Currently not used. Don't save to JSON
    json_add_int(jo, "systemState", ci.SystemState);
    json_add_int(jo, "primaryLongRangeChannelId", ci.PrimaryLongRangeChannelId);
    json_add_bool(jo, "longRangeChannelAutoMode", ci.LongRangeChannelAutoMode);
  }

  // Adding Application Settings
  if (ECODE_NVM3_OK == ec_as)
  {
    json_add_bool(jo, "isListening", as.listening);
    json_add_int(jo, "genericDeviceClass", as.generic);
    json_add_int(jo, "specificDeviceClass", as.specific);
  }

  // Adding Application Info
  if (ECODE_NVM3_OK == ec_ai)
  {
    json_object_object_add(jo, "cmdClasses", cmd_classes_to_json(ai));
  }
  json_object_object_add(jo, "rfConfig", app_config_to_json(nvm_layout));

  if (ECODE_NVM3_OK == ec_ad)
  {
    /* Search backwards in the application data section to skip trailing zero
     * values (typically the data is stored at the beginning leaving a large
     * unused section at the end - there's no need to save all those zeros
     * to JSON)
     */
    int n = APPL_DATA_FILE_SIZE;
    while ((n > 0) && (0 == ad.extNvm[n - 1]))
    {
      --n;
    }

    if (n > 0)
    {
      char *b64_encoded_string = base64_encode(ad.extNvm, n, 0);
      if (b64_encoded_string)
      {
        json_add_string(jo, "applicationData", b64_encoded_string);
        free(b64_encoded_string);
      }
    }
    else
    {
      json_add_string(jo, "applicationData", "");
    }
  }

  if (ECODE_NVM3_OK == ec_dcdc)
  {
    json_add_int(jo, "dcdcConfig", dcdc.dcdc_config);
  }

  json_object_object_add(jo, "sucUpdateEntries", suc_node_list_v5_to_json());
  return jo;
}

/*****************************************************************************/
/* Read controller information and pack it into json object */
json_object *controller_info_nvm_get_json(nvmLayout_t nvm_layout)
{
  json_object *jo = json_object_new_object();

  backup_info(nvm_layout);
  json_add_int(jo, "format", (int32_t)target_protocol_version.format);
  json_add_int(jo, "applicationFileFormat", (int32_t)target_app_version.format);

  if (target_protocol_version.major == 7 && target_protocol_version.minor >= 19)
  {
    json_object_object_add(jo, "controller", nvm_from_719_controller_info_to_json(nvm_layout));
  }
  else if (target_protocol_version.major == 7 && target_protocol_version.minor >= 15)
  {
    json_object_object_add(jo, "controller", nvm715_controller_info_to_json(nvm_layout));
  }
  else
  {
    json_object_object_add(jo, "controller", nvm711_controller_info_to_json(nvm_layout));
  }

  json_object_object_add(jo, "nodes", node_table_to_json(nvm_layout));
  json_object_object_add(jo, "lrNodes", lr_node_table_to_json(nvm_layout));

  return jo;
}

/*****************************************************************************/
/*******************************  JSON --> NVM  ******************************/
/*****************************************************************************/

/*****************************************************************************/

static bool check_data_rate(const int supported_data_rate[], const int value_to_check, uint8_t size)
{
  for (int i = 0; i < size; i++)
  {
    if (supported_data_rate[i] == value_to_check)
    {
      return true;
    }
  }
  return false;
}

/*****************************************************************************/
static uint8_t parse_node_capability(json_object *jo_node, uint8_t *node_reserved)
{
  uint8_t capability = 0;
  // Support listening?
  capability = (json_get_bool(jo_node, "isListening", true, JSON_REQUIRED)) ? (capability | ZWAVE_NODEINFO_LISTENING_SUPPORT) : capability;
  // Support routing?
  capability = (json_get_bool(jo_node, "isRouting", false, JSON_REQUIRED)) ? (capability | ZWAVE_NODEINFO_ROUTING_SUPPORT) : capability;
  // Data Rate
  int supported_data_rate[3] = {0};

  uint8_t len = json_get_intarray(jo_node, "supportedDataRates", supported_data_rate, 3, JSON_REQUIRED);
  if (check_data_rate(supported_data_rate, 9600, len))
    capability |= ZWAVE_NODEINFO_BAUD_9600;
  if (check_data_rate(supported_data_rate, 40000, len))
    capability |= ZWAVE_NODEINFO_BAUD_40000;
  if (check_data_rate(supported_data_rate, 100000, len))
    *node_reserved |= ZWAVE_NODEINFO_BAUD_100K;
  // Protocol Version
  capability |= json_get_int(jo_node, "protocolVersion", 1, JSON_REQUIRED);

  return capability;
}

/*****************************************************************************/
static uint8_t parse_node_security(json_object *jo_node)
{
  uint8_t security = 0;
  // is Frequent Listening device?
  enum json_type jo_frequent_listening_type = json_object_get_type(json_object_object_get(jo_node, "isFrequentListening"));
  if (jo_frequent_listening_type == json_type_string)
  {
    security |= (json_get_string(jo_node, "isFrequentListening", "250ms", JSON_REQUIRED) == "250ms") ? ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250 : ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000;
  }

  // optional Function?
  security = json_get_bool(jo_node, "optionalFunctionality", false, JSON_REQUIRED) ? (security | ZWAVE_NODEINFO_OPTIONAL_FUNC) : security;

  // nodeType
  security |= json_get_int(jo_node, "nodeType", 0, JSON_REQUIRED) ? 0x08 : 0x02;

  // Security support?
  security = json_get_bool(jo_node, "supportsSecurity", true, JSON_REQUIRED) ? (security | ZWAVE_NODEINFO_SECURITY_SUPPORT) : security;

  // Beaming support?
  security = json_get_bool(jo_node, "supportsBeaming", true, JSON_REQUIRED) ? (security | ZWAVE_NODEINFO_BEAM_CAPABILITY) : security;

  return security;
}
/*****************************************************************************/

static bool parse_route_cache_line_json(json_object *jo_rcl, ROUTECACHE_LINE *rcl)
{
  enum json_type jo_beaming_type = json_object_get_type(json_object_object_get(jo_rcl, "beaming"));
  rcl->routecacheLineConfSize = 0x00; // Default to 0x00
  if (jo_beaming_type == json_type_string)
  {
    rcl->routecacheLineConfSize |= strcmp(json_get_string(jo_rcl, "beaming", "250ms", JSON_OPTIONAL), "250ms") == 0 ? 0x20 : 0x40;
  }

  rcl->routecacheLineConfSize |= json_get_int(jo_rcl, "protocolRate", 0, JSON_OPTIONAL);
  json_get_bytearray(jo_rcl, "repeaterNodeIDs", rcl->repeaterList, MAX_REPEATERS, JSON_OPTIONAL);

  return true;
}
/*****************************************************************************/
static bool parse_node_table_json(json_object *jo_ntbl)
{
  CLASSIC_NODE_MASK_TYPE node_info_exists = {};
  CLASSIC_NODE_MASK_TYPE bridge_node_flags = {};
  CLASSIC_NODE_MASK_TYPE suc_pending_update_flags = {};
  CLASSIC_NODE_MASK_TYPE pending_discovery_flags = {};
  CLASSIC_NODE_MASK_TYPE route_slave_suc_flags = {};

  CLASSIC_NODE_MASK_TYPE node_routecache_exists = {};
  CLASSIC_NODE_MASK_TYPE app_route_lock_flags = {};

  LR_NODE_MASK_TYPE lr_node_info_exists = {0};

  json_object_object_foreach(jo_ntbl, key, jo_node)
  {
    uint16_t node_id = atoi(key);
    if (node_id > 0 && node_id <= ZW_CLASSIC_MAX_NODES)
    {
      SNodeInfo ni_buf[NODEINFOS_PER_FILE] = {0};
      SNodeInfo *ni;
      if (target_protocol_version.format > 0)
      {
        uint32_t file_type;
        size_t file_len;
        nvm3_ObjectKey_t file_id = FILE_ID_NODEINFO_V1 + (node_id - 1) / NODEINFOS_PER_FILE;
        Ecode_t ec = nvm3_getObjectInfo(&nvm3_protocol_handle, file_id, &file_type, &file_len);
        if (ec == ECODE_NVM3_OK)
        {
          READ_PROT_NVM(file_id, ni_buf);
        }
        else 
        {
          memset(ni_buf, 0xFF, FILE_SIZE_NODEINFO_V1);
        }
        ni = &ni_buf[(node_id - 1) % NODEINFOS_PER_FILE];
      }
      else
      {
        ni = &ni_buf[0];
      }
      if (json_get_bool(jo_node, "isVirtual", false, JSON_REQUIRED))
        ZW_NodeMaskSetBit(bridge_node_flags, node_id);
      ni->ControllerSucUpdateIndex = json_get_int(jo_node, "sucUpdateIndex", 0, JSON_REQUIRED);
      json_get_nodemask(jo_node, "neighbours", (uint8_t *)&ni->neighboursInfo, JSON_REQUIRED);
      ni->NodeInfo.reserved = 0;
      ni->NodeInfo.capability = parse_node_capability(jo_node, &ni->NodeInfo.reserved);
      ni->NodeInfo.security = parse_node_security(jo_node);
      ni->NodeInfo.generic = json_get_int(jo_node, "genericDeviceClass", 0, JSON_REQUIRED);
      ni->NodeInfo.specific = json_get_int(jo_node, "specificDeviceClass", 0, JSON_REQUIRED);
      if (ni->NodeInfo.specific)
      {
        ni->NodeInfo.security |= ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;
      }
      ZW_NodeMaskSetBit(node_info_exists, node_id);
      if (target_protocol_version.format > 0)
      {
        WRITE_PROT_NVM(FILE_ID_NODEINFO_V1 + (node_id - 1) / NODEINFOS_PER_FILE, ni_buf);
      }
      else
      {
        WRITE_PROT_NVM(FILE_ID_NODEINFO + node_id - 1, ni_buf[0]);
      }
      // Route Slave SUC set bit mask
      if (json_get_bool(jo_node, "routeSlaveSuc", false, JSON_REQUIRED))
        ZW_NodeMaskSetBit(route_slave_suc_flags, node_id);
      /* ---- ROUTE CACHE BEGIN ---- */
      SNodeRouteCache node_rc_buf[NODEROUTECACHES_PER_FILE] = {};
      SNodeRouteCache *node_rc;

      if (target_protocol_version.format > 0)
      {
        uint32_t file_type;
        size_t file_len;
        nvm3_ObjectKey_t file_id = FILE_ID_NODEROUTECAHE_V1 + (node_id - 1) / NODEROUTECACHES_PER_FILE;
        Ecode_t ec = nvm3_getObjectInfo(&nvm3_protocol_handle, file_id, &file_type, &file_len);
        if (ec == ECODE_NVM3_OK)
        {
          READ_PROT_NVM(file_id, node_rc_buf);
        }
        else 
        {
          memset(node_rc_buf, 0xFF, FILE_SIZE_NODEROUTECAHE_V1);
        }
        node_rc = &node_rc_buf[(node_id - 1) % NODEROUTECACHES_PER_FILE];
      }
      else
      {
        node_rc = &node_rc_buf[0];
      }
      if (json_get_bool(jo_node, "appRouteLock", false, JSON_OPTIONAL))
      {
        ZW_NodeMaskSetBit(app_route_lock_flags, node_id);
      }
      json_object *jo_lwr;
      if (json_object_object_get_ex(jo_node, "lwr", &jo_lwr) && (jo_lwr != NULL))
      {
        parse_route_cache_line_json(jo_lwr, &node_rc->routeCache);
        ZW_NodeMaskSetBit(node_routecache_exists, node_id);
      }

      json_object *jo_nlwr;
      if (json_object_object_get_ex(jo_node, "nlwr", &jo_nlwr) && (jo_nlwr != NULL))
      {
        parse_route_cache_line_json(jo_nlwr, &node_rc->routeCacheNlwrSr);
        ZW_NodeMaskSetBit(node_routecache_exists, node_id);
      }

      if (ZW_NodeMaskNodeIn(node_routecache_exists, node_id))
      {
        if (target_protocol_version.format > 0)
        {
          WRITE_PROT_NVM(FILE_ID_NODEROUTECAHE_V1 + (node_id - 1) / NODEROUTECACHES_PER_FILE, node_rc_buf);
        }
        else
        {
          WRITE_PROT_NVM(FILE_ID_NODEROUTECAHE + node_id - 1, node_rc_buf[0]);
        }
      }
    }
    /* ---- ROUTE CACHE END ---- */
  }
  /* Write all the node mask files */
  WRITE_PROT_NVM(FILE_ID_NODE_STORAGE_EXIST, node_info_exists);
  if (target_protocol_version.format >= 2)
  {
    WRITE_PROT_NVM(FILE_ID_LRANGE_NODE_EXIST, lr_node_info_exists);
  }

  WRITE_PROT_NVM(FILE_ID_BRIDGE_NODE_FLAG, bridge_node_flags);
  WRITE_PROT_NVM(FILE_ID_SUC_PENDING_UPDATE_FLAG, suc_pending_update_flags);
  WRITE_PROT_NVM(FILE_ID_PENDING_DISCOVERY_FLAG, pending_discovery_flags);

  WRITE_PROT_NVM(FILE_ID_NODE_ROUTECACHE_EXIST, node_routecache_exists);
  WRITE_PROT_NVM(FILE_ID_APP_ROUTE_LOCK_FLAG, app_route_lock_flags);
  WRITE_PROT_NVM(FILE_ID_ROUTE_SLAVE_SUC_FLAG, route_slave_suc_flags);
  return true;
}

/*****************************************************************************/
static bool parse_lr_node_table_json(json_object *jo_lrntbl)
{
  LR_NODE_MASK_TYPE lr_node_info_exists = {0};
  SLRNodeInfo ni_buf[LR_NODEINFOS_PER_FILE] = {0};
  SLRNodeInfo *ni;
  uint8_t lr_tx_power_buf[FILE_SIZE_LR_TX_POWER] = {0};
  uint8_t *lr_tx_power;
  json_object_object_foreach(jo_lrntbl, key, jo_lrnode)
  {
    uint16_t node_id = atoi(key);
    if (node_id >= ZW_LR_MIN_NODE_ID && node_id <= ZW_LR_MAX_NODE_ID)
    {
      if (target_protocol_version.format >= 2)
      {
        uint32_t file_type;
        size_t file_len;
        /* LR Node Info */
        nvm3_ObjectKey_t file_id = FILE_ID_NODEINFO_LR + (node_id - ZW_LR_MIN_NODE_ID) / LR_NODEINFOS_PER_FILE;
        Ecode_t ec = nvm3_getObjectInfo(&nvm3_protocol_handle, file_id, &file_type, &file_len);
        if (ec == ECODE_NVM3_OK)
        {
          READ_PROT_NVM(file_id, ni_buf);
        }
        else 
        {
          memset(ni_buf, 0xFF, FILE_SIZE_NODEINFO_LR);
        }
        ZW_NodeMaskSetBit(lr_node_info_exists, node_id - ZW_LR_MIN_NODE_ID + 1);
        ni = &ni_buf[(node_id - ZW_LR_MIN_NODE_ID) % LR_NODEINFOS_PER_FILE];

        enum json_type jo_frequent_listening_type = json_object_get_type(json_object_object_get(jo_lrnode, "isFrequentListening"));
        ni->packedInfo = 0;
        if (jo_frequent_listening_type == json_type_string)
        {
          ni->packedInfo |= (json_get_string(jo_lrnode, "isFrequentListening", "250ms", JSON_REQUIRED) == "250ms") ? ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250 : ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000;
        }

        ni->packedInfo |= json_get_bool(jo_lrnode, "isListening", false, JSON_REQUIRED) ? NODEINFO_FLAG_LISTENING : 0x00;
        ni->packedInfo |= json_get_bool(jo_lrnode, "isRouting", false, JSON_REQUIRED) ? NODEINFO_FLAG_ROUTING : 0x00;
        ni->packedInfo |= json_get_bool(jo_lrnode, "optionalFunctionality", false, JSON_REQUIRED) ? NODEINFO_FLAG_OPTIONAL : 0x00;
        ni->packedInfo |= json_get_bool(jo_lrnode, "supportsBeaming", false, JSON_REQUIRED) ? NODEINFO_FLAG_BEAM : 0x00;

        ni->generic = json_get_int(jo_lrnode, "genericDeviceClass", 0, JSON_REQUIRED);
        ni->specific = json_get_int(jo_lrnode, "specificDeviceClass", 0, JSON_REQUIRED);
        if (ni->specific)
          ni->packedInfo |= NODEINFO_FLAG_SPECIFIC;

        WRITE_PROT_NVM(file_id, ni_buf);

        /* LR Tx Power */
        if (target_protocol_version.format == 2)
        {
          file_id = FILE_ID_LR_TX_POWER_V2 + (node_id - ZW_LR_MIN_NODE_ID) / LR_TX_POWER_PER_FILE_V2;
          ec = nvm3_getObjectInfo(&nvm3_protocol_handle, file_id, &file_type, &file_len);
          if (ec == ECODE_NVM3_OK)
          {
            READ_PROT_NVM(file_id, lr_tx_power_buf);
          }
          lr_tx_power = &lr_tx_power_buf[((node_id - ZW_LR_MIN_NODE_ID) % LR_TX_POWER_PER_FILE_V2) / 2];
          /* Odd and even node ID take first and second half of lr_tx_power respectively */
          if ((node_id - ZW_LR_MIN_NODE_ID) & 1)
          {
            *lr_tx_power |= ((uint8_t)json_get_int(jo_lrnode, "lrTxPower", 0, JSON_REQUIRED) & 0xf0);
          }
          else
          {
            *lr_tx_power |= ((uint8_t)json_get_int(jo_lrnode, "lrTxPower", 0, JSON_REQUIRED) & 0x0f);
          }
          WRITE_PROT_NVM(file_id, lr_tx_power_buf);
        }
        if (target_protocol_version.format == 3)
        {
          file_id = FILE_ID_LR_TX_POWER_V3 + (node_id - ZW_LR_MIN_NODE_ID) / LR_TX_POWER_PER_FILE_V3;
          ec = nvm3_getObjectInfo(&nvm3_protocol_handle, file_id, &file_type, &file_len);
          if (ec == ECODE_NVM3_OK)
          {
            READ_PROT_NVM(file_id, lr_tx_power_buf);
          }
          lr_tx_power = &lr_tx_power_buf[(node_id - ZW_LR_MIN_NODE_ID) % LR_TX_POWER_PER_FILE_V3];
          *lr_tx_power = (uint8_t)json_get_int(jo_lrnode, "lrTxPower", 0, JSON_REQUIRED);
          WRITE_PROT_NVM(file_id, lr_tx_power_buf);
        }
      }
    }
  }
  if (target_protocol_version.format >= 2)
  {
    WRITE_PROT_NVM(FILE_ID_LRANGE_NODE_EXIST, lr_node_info_exists);
  }
  return true;
}

/*****************************************************************************/
static bool parse_suc_state_json(json_object *jo_suc, uint8_t *last_suc_index)
{
  SSucNodeList suc_node_list;
  memset(&suc_node_list, 0xFF, sizeof(suc_node_list));
  {
    for (int i = 0; (i < json_object_array_length(jo_suc)) && (i < SUC_MAX_UPDATES); i++)
    {
      json_object *jo_suc_upd = json_object_array_get_idx(jo_suc, i);
      SUC_UPDATE_ENTRY_STRUCT *suc_entry = &suc_node_list.aSucNodeList[i];

      suc_entry->NodeID = json_get_int(jo_suc_upd, "nodeId", 0, JSON_REQUIRED);
      suc_entry->changeType = json_get_int(jo_suc_upd, "changeType", 0, JSON_REQUIRED);

      // Read supportedCCs and controlledCCs
      json_object *jo_supportedCCs = json_object_object_get(jo_suc_upd, "supportedCCs");
      json_object *jo_controlledCCs = json_object_object_get(jo_suc_upd, "controlledCCs");

      int npar = 0;

      // Add supportedCCs to nodeInfo
      memset(suc_entry->nodeInfo, 0x00, SUC_UPDATE_NODEPARM_MAX);
      if (jo_supportedCCs && json_object_get_type(jo_supportedCCs) == json_type_array)
      {
        for (int k = 0; k < json_object_array_length(jo_supportedCCs) && npar < SUC_UPDATE_NODEPARM_MAX; k++)
        {
          suc_entry->nodeInfo[npar++] = (uint8_t)json_object_get_int(json_object_array_get_idx(jo_supportedCCs, k));
        }
      }

      // Insert 0xEF as a separator
      if (npar < SUC_UPDATE_NODEPARM_MAX)
      {
        suc_entry->nodeInfo[npar++] = 0xEF;
      }

      // Add controlledCCs to nodeInfo
      if (jo_controlledCCs && json_object_get_type(jo_controlledCCs) == json_type_array)
      {
        for (int k = 0; k < json_object_array_length(jo_controlledCCs) && npar < SUC_UPDATE_NODEPARM_MAX; k++)
        {
          suc_entry->nodeInfo[npar++] = (uint8_t)json_object_get_int(json_object_array_get_idx(jo_controlledCCs, k));
        }
      }
    }
  }

  WRITE_PROT_NVM(FILE_ID_SUCNODELIST, suc_node_list);

  return true;
}

/*****************************************************************************/
static bool parse_suc_state_json_nvm719(json_object *jo_suc, uint8_t *last_suc_index)
{
  json_object *jo_suc_updates = NULL;
  // if (json_object_array_length(jo_suc) < SUC_MAX_UPDATES)
  //   return false;
  int jo_suc_len = json_object_array_length(jo_suc);
  for (int i = 0; (i < jo_suc_len) && (i < SUC_MAX_UPDATES); i += 8)
  {
    SSucNodeList_V5 suc_node_list = {};
    memset(&suc_node_list, 0xFF, sizeof(suc_node_list));
    for (int j = 0; (j < SUCNODES_PER_FILE) && (i + j < jo_suc_len); j++)
    {
      json_object *jo_suc_upd = json_object_array_get_idx(jo_suc, i + j);
      SUC_UPDATE_ENTRY_STRUCT *suc_entry = &suc_node_list.aSucNodeList[j];

      suc_entry->NodeID = json_get_int(jo_suc_upd, "nodeId", 0, JSON_REQUIRED);
      suc_entry->changeType = json_get_int(jo_suc_upd, "changeType", 0, JSON_REQUIRED);
      // Read supportedCCs and controlledCCs
      json_object *jo_supportedCCs = json_object_object_get(jo_suc_upd, "supportedCCs");
      json_object *jo_controlledCCs = json_object_object_get(jo_suc_upd, "controlledCCs");

      int npar = 0;

      // Add supportedCCs to nodeInfo
      memset(suc_entry->nodeInfo, 0x00, SUC_UPDATE_NODEPARM_MAX);
      if (jo_supportedCCs && json_object_get_type(jo_supportedCCs) == json_type_array)
      {
        for (int k = 0; k < json_object_array_length(jo_supportedCCs) && npar < SUC_UPDATE_NODEPARM_MAX; k++)
        {
          suc_entry->nodeInfo[npar++] = (uint8_t)json_object_get_int(json_object_array_get_idx(jo_supportedCCs, k));
        }
      }

      // Insert 0xEF as a separator
      if (npar < SUC_UPDATE_NODEPARM_MAX)
      {
        suc_entry->nodeInfo[npar++] = 0xEF;
      }

      // Add controlledCCs to nodeInfo
      if (jo_controlledCCs && json_object_get_type(jo_controlledCCs) == json_type_array)
      {
        for (int k = 0; k < json_object_array_length(jo_controlledCCs) && npar < SUC_UPDATE_NODEPARM_MAX; k++)
        {
          suc_entry->nodeInfo[npar++] = (uint8_t)json_object_get_int(json_object_array_get_idx(jo_controlledCCs, k));
        }
      }
    }
    WRITE_PROT_NVM(FILE_ID_SUCNODELIST_BASE_V5 + i / 8, suc_node_list);
  }

  return true;
}

/*****************************************************************************/
static bool parse_app_config_json(json_object *jo_app_config, nvmLayout_t nvm_layout)
{

  /* Only read in these appConfig properties if the JSON is generated from a
   * controller with application version 7 (the application config properties
   * for e.g. version 6 are not compatible with version 7, so we simply skip
   * them and assume the gateway will re-initialize the correct settings via
   * serial API on startup)
   */
  if (app_nvm_is_pre_v7_15_3(target_protocol_version.major, target_protocol_version.minor, target_protocol_version.patch))
  {
    SApplicationConfiguration_prior_7_15_3 ac = {};
    ac.rfRegion = json_get_int(jo_app_config, "rfRegion", 0, JSON_OPTIONAL);
    ac.iTxPower = json_get_int(jo_app_config, "txPower", 0, JSON_OPTIONAL);
    ac.ipower0dbmMeasured = json_get_int(jo_app_config, "measured0dBm", 0, JSON_OPTIONAL);
    WRITE_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
  }
  else if (app_nvm_is_pre_v7_18_1(target_protocol_version.major, target_protocol_version.minor, target_protocol_version.patch))
  {
    SApplicationConfiguration_prior_7_18_1 ac = {};
    ac.rfRegion = json_get_int(jo_app_config, "rfRegion", 0, JSON_OPTIONAL);
    ac.iTxPower = json_get_int(jo_app_config, "txPower", 0, JSON_OPTIONAL);
    ac.ipower0dbmMeasured = json_get_int(jo_app_config, "measured0dBm", 0, JSON_OPTIONAL);
    ac.enablePTI = json_get_bool(jo_app_config, "enablePTI", 0, JSON_OPTIONAL);
    ac.maxTxPower = json_get_int(jo_app_config, "maxTxPower", 140, JSON_OPTIONAL);
    WRITE_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
  }
  else if (app_nvm_is_pre_v7_19(target_protocol_version.major, target_protocol_version.minor, target_protocol_version.patch))
  {
    SApplicationConfiguration_7_18_1 ac = {};
    ac.rfRegion = json_get_int(jo_app_config, "rfRegion", 0, JSON_OPTIONAL);
    ac.iTxPower = json_get_int(jo_app_config, "txPower", 0, JSON_OPTIONAL);
    ac.ipower0dbmMeasured = json_get_int(jo_app_config, "measured0dBm", 0, JSON_OPTIONAL);
    ac.enablePTI = json_get_bool(jo_app_config, "enablePTI", 0, JSON_OPTIONAL);
    ac.maxTxPower = json_get_int(jo_app_config, "maxTxPower", 140, JSON_OPTIONAL);
    WRITE_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
  }
  else if (app_nvm_is_pre_v7_21(target_protocol_version.major, target_protocol_version.minor, target_protocol_version.patch))
  {
    SApplicationConfiguration_7_18_1 ac = {};
    ac.rfRegion = json_get_int(jo_app_config, "rfRegion", 0, JSON_OPTIONAL);
    ac.iTxPower = json_get_int(jo_app_config, "txPower", 0, JSON_OPTIONAL);
    ac.ipower0dbmMeasured = json_get_int(jo_app_config, "measured0dBm", 0, JSON_OPTIONAL);
    ac.enablePTI = json_get_bool(jo_app_config, "enablePTI", 0, JSON_OPTIONAL);
    ac.maxTxPower = json_get_int(jo_app_config, "maxTxPower", 140, JSON_OPTIONAL);
    if (NVM3_700s == nvm_layout)
    {
      WRITE_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
    }
    else
    {
      WRITE_PROT_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
    }
  }
  else
  {
    int nodeIdBaseType = json_get_int(jo_app_config, "nodeIdType", 0, JSON_OPTIONAL);
    if (nodeIdBaseType >= 0)
    {
      SApplicationConfiguration_7_21_x ac = {};
      ac.rfRegion = json_get_int(jo_app_config, "rfRegion", 0, JSON_OPTIONAL);
      ac.iTxPower = json_get_int(jo_app_config, "txPower", 0, JSON_OPTIONAL);
      ac.ipower0dbmMeasured = json_get_int(jo_app_config, "measured0dBm", 0, JSON_OPTIONAL);
      ac.enablePTI = json_get_bool(jo_app_config, "enablePTI", 0, JSON_OPTIONAL);
      ac.maxTxPower = json_get_int(jo_app_config, "maxTxPower", 140, JSON_OPTIONAL);
      ac.nodeIdBaseType = json_get_int(jo_app_config, "nodeIdType", 0, JSON_OPTIONAL);
      if (NVM3_700s == nvm_layout)
      {
        WRITE_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
      }
      else
      {
        WRITE_PROT_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
      }
    }
    else
    {
      SApplicationConfiguration_7_18_1 ac = {};
      ac.rfRegion = json_get_int(jo_app_config, "rfRegion", 0, JSON_OPTIONAL);
      ac.iTxPower = json_get_int(jo_app_config, "txPower", 0, JSON_OPTIONAL);
      ac.ipower0dbmMeasured = json_get_int(jo_app_config, "measured0dBm", 0, JSON_OPTIONAL);
      ac.enablePTI = json_get_bool(jo_app_config, "enablePTI", 0, JSON_OPTIONAL);
      ac.maxTxPower = json_get_int(jo_app_config, "maxTxPower", 140, JSON_OPTIONAL);
      if (NVM3_700s == nvm_layout)
      {
        WRITE_APP_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
      }
      else
      {
        WRITE_PROT_NVM(FILE_ID_APPLICATIONCONFIGURATION, ac);
      }
    }
  }

  return true;
}

/*****************************************************************************/
static uint32_t file_version(uint8_t format, uint8_t major, uint8_t minor, uint8_t patch)
{
  uint32_t version_host = (format) << 24 | (major << 16) | (minor << 8) | patch;
  // 700 series is little endian, so that's what we're storing to the nvm3 file
  return htole32(version_host);
}

/*****************************************************************************/
static bool create_nvm3_version_files(nvmLayout_t nvm_layout)
{
  /* We're not really parsing the JSOON file descriptor section here since
   * it contains versions information related to the source device.
   * We're instead generating version files for NVM that contains the
   * series 700 versions this converter is built with.
   *
   * NB: It is assumed the JSON file descriptor is validated with
   *     is_json_file_supported()
   */

  uint32_t prot_version = file_version(target_protocol_version.format,
                                       target_protocol_version.major,
                                       target_protocol_version.minor,
                                       target_protocol_version.patch);

  uint32_t app_version = file_version(target_app_version.format,
                                      target_app_version.major,
                                      target_app_version.minor,
                                      target_app_version.patch);

  WRITE_PROT_NVM(FILE_ID_ZW_VERSION, prot_version);
  if ((target_protocol_version.major == 7 && target_protocol_version.minor < 19) || target_protocol_version.major < 7 || NVM3_700s == nvm_layout)
  {
    WRITE_APP_NVM(ZAF_FILE_ID_APP_VERSION, app_version);
  }
  else
  {
    if (hardware_info == EFR32XG28)
    {
      WRITE_PROT_NVM(ZAF_FILE_ID_APP_VERSION_800s_XG28, app_version);
    }
    else if (hardware_info == EFR32XG23)
    {
      WRITE_PROT_NVM(ZAF_FILE_ID_APP_VERSION_800s_XG23, app_version);
    }
  }

  return true;
}

/*****************************************************************************/
static bool is_json_file_supported(json_object *jo_filedesc,
                                   uint8_t *major_prot_version, uint8_t *minor_prot_version, uint8_t *patch_prot_verion,
                                   uint8_t *major_app_version, uint8_t *minor_app_version, uint8_t *patch_app_verion)
{
  int32_t json_file_version = json_get_int(jo_filedesc, "backupFormatVersion", 0, JSON_REQUIRED);

  if (1 != json_file_version)
  {
    user_message(MSG_ERROR, "ERROR: Unsupported backupFormatVersion: %d. Must be 1.\n", json_file_version);
    return false;
  }

  const char *prot_ver = json_get_string(jo_filedesc, "sourceProtocolVersion", "00.00.00", JSON_REQUIRED);
  unsigned int major;
  unsigned int minor;
  unsigned int patch;

  if (sscanf(prot_ver, "%u.%u.%u", &major, &minor, &patch) != 3)
  {
    user_message(MSG_ERROR, "ERROR: Incorrectly formatted sourceProtocolVersion: \"%s\". Must be like \"dd.dd.dd\" (d:0-9).\n", prot_ver);
    return false;
  }
  printf("Protocol version: %d.%d.%d \n", major, minor, patch);
  *major_prot_version = major;
  *minor_prot_version = minor;
  *patch_prot_verion = patch;

  const char *app_ver = json_get_string(jo_filedesc, "sourceAppVersion", "00.00.00", JSON_REQUIRED);

  if (sscanf(app_ver, "%u.%u.%u", &major, &minor, &patch) != 3)
  {
    user_message(MSG_ERROR, "ERROR: Incorrectly formatted sourceAppVersion: \"%s\". Must be like \"dd.dd.dd\" (d:0-9).\n");
    return false;
  }
  printf("App version: %d.%d.%d \n", major, minor, patch);
  *major_app_version = major;
  *minor_app_version = minor;
  *patch_app_verion = patch;

  /* NB: We're not validating the actual versions of the protocol and
   *  application sources. Currently it does not make any difference. */

  // We made it all the way to the end without errors :-)
  return true;
}

/*****************************************************************************/
static bool parse_controller_nvm711_json(json_object *jo_ctrl, nvmLayout_t nvm_layout)
{
  SControllerInfo_NVM711 ci = {};
  SApplicationCmdClassInfo ai = {};
  SApplicationData ad = {};
  SApplicationSettings as = {};
  CONTROLLER_CONFIGURATION controller_configuration = {};

  json_get_home_id(jo_ctrl, "homeId", ci.HomeID, 0, JSON_REQUIRED);
  ci.NodeID = json_get_int(jo_ctrl, "nodeId", 0, JSON_REQUIRED);
  // ci.HomeID                  = json_get_int(jo_ctrl,  STRING_CONTROLLER_HOME_ID, 0, JSON_REQUIRED);
  ci.StaticControllerNodeId = json_get_int(jo_ctrl, "staticControllerNodeId", 0, JSON_REQUIRED);
  ci.LastUsedNodeId = json_get_int(jo_ctrl, "lastNodeId", 0, JSON_REQUIRED);
  ci.SucLastIndex = json_get_int(jo_ctrl, "sucLastIndex", 0, JSON_REQUIRED);
  ci.MaxNodeId = json_get_int(jo_ctrl, "maxNodeId", 0, JSON_REQUIRED);
  ci.ReservedId = json_get_int(jo_ctrl, "reservedId", 0, JSON_REQUIRED);
  ci.SystemState = json_get_int(jo_ctrl, "systemState", 0, JSON_REQUIRED);

  controller_configuration.controller_is_secondary = json_get_bool(jo_ctrl, "controllerIsSecondary", false, JSON_REQUIRED);
  controller_configuration.controller_on_other_network = json_get_bool(jo_ctrl, "controllerOnOtherNetwork", false, JSON_REQUIRED);
  controller_configuration.controller_nodeid_server_present = json_get_bool(jo_ctrl, "controllerNodeIdServerPresent", false, JSON_REQUIRED);
  controller_configuration.controller_is_real_primary = json_get_bool(jo_ctrl, "controllerIsRealPrimary", true, JSON_REQUIRED);
  controller_configuration.controller_is_suc = json_get_bool(jo_ctrl, "controllerIsSuc", false, JSON_REQUIRED);
  controller_configuration.no_nodes_included = json_get_bool(jo_ctrl, "noNodesIncluded", true, JSON_REQUIRED);
  memcpy(&ci.ControllerConfiguration, &controller_configuration, sizeof(CONTROLLER_CONFIGURATION));
  // SucAwarenessPushNeeded is not used anywhere in the protocol source code
  // ci.SucAwarenessPushNeeded  = json_get_bool(jo_ctrl, STSTRING_SUC_AWARENESS_PUSH_NEEDED, false, JSON_REQUIRED);
  // We don't write ReservedId to the JSON file, so no need to try reading it back
  // ci.ReservedId              = json_get_int(jo_ctrl,  "reservedId", 0, JSON_REQUIRED);

  as.listening = json_get_bool(jo_ctrl, "isListening", true, JSON_REQUIRED);
  as.generic = json_get_int(jo_ctrl, "genericDeviceClass", 0, JSON_REQUIRED);
  as.specific = json_get_int(jo_ctrl, "specificDeviceClass", 0, JSON_REQUIRED);

  json_object *jo_cmd_classes = NULL;
  if (false == json_get_object_error_check(jo_ctrl, "cmdClasses", &jo_cmd_classes, json_type_object, JSON_REQUIRED))
  {
    printf("controller_parse_json:json_get_object_error_check() error: cmdClasses \n");
    return false;
  }
  printf("jo_cmd_classes: %s\n", json_object_to_json_string(jo_cmd_classes));
  ai.UnSecureIncludedCCLen = json_get_bytearray(jo_cmd_classes, "includedInsecurely", ai.UnSecureIncludedCC, APPL_NODEPARM_MAX, JSON_REQUIRED);
  ai.SecureIncludedSecureCCLen = json_get_bytearray(jo_cmd_classes, "includedSecurelySecureCCs", ai.SecureIncludedSecureCC, APPL_NODEPARM_MAX, JSON_REQUIRED);
  ai.SecureIncludedUnSecureCCLen = json_get_bytearray(jo_cmd_classes, "includedSecurelyInsecureCCs", ai.SecureIncludedUnSecureCC, APPL_NODEPARM_MAX, JSON_REQUIRED);

  json_object *jo_rf_config = NULL;
  if (false == json_get_object_error_check(jo_ctrl, "rfConfig", &jo_rf_config, json_type_object, JSON_REQUIRED))
  {
    printf("controller_parse_json:json_get_object_error_check() error: rfConfig \n");
    return false;
  }
  parse_app_config_json(jo_rf_config, nvm_layout);
  json_object *jo_suc_state = NULL;
  if (json_get_object_error_check(jo_ctrl, "sucUpdateEntries", &jo_suc_state, json_type_array, JSON_REQUIRED))
  {
    parse_suc_state_json(jo_suc_state, &ci.SucLastIndex);
  }

  const char *b64_app_data_string = json_get_string(jo_ctrl, "applicationData", "", JSON_REQUIRED);

  if (strlen(b64_app_data_string) > 0)
  {
    size_t app_data_len = 0;
    uint8_t *app_data_bin = base64_decode(b64_app_data_string,
                                          strlen(b64_app_data_string),
                                          &app_data_len);

    if (NULL == app_data_bin)
    {
      user_message(MSG_ERROR, "ERROR: Could not base64 decode \"applicationData\".\n");
      return false;
    }

    size_t bytes_to_copy = app_data_len;

    if (bytes_to_copy > APPL_DATA_FILE_SIZE)
    {
      // Only generate the warning if we are throwing away non-zero data
      size_t non_null_bytes_to_copy = bytes_to_copy;
      // Search backwards for non-zero values
      while (non_null_bytes_to_copy > 0 && app_data_bin[non_null_bytes_to_copy - 1] == 0)
      {
        --non_null_bytes_to_copy;
      }

      if (non_null_bytes_to_copy > APPL_DATA_FILE_SIZE)
      {
        user_message(MSG_WARNING, "WARNING: \"applicationData\" will be truncated. "
                                  "Bytes with non-zero values: %zu. Max application data size in generated NVM image: %zu.\n",
                     non_null_bytes_to_copy,
                     APPL_DATA_FILE_SIZE);
      }

      bytes_to_copy = APPL_DATA_FILE_SIZE;
    }

    memcpy(ad.extNvm, app_data_bin, bytes_to_copy);
    free(app_data_bin);
  }

  WRITE_APP_NVM(FILE_ID_APPLICATIONSETTINGS, as);
  WRITE_PROT_NVM(FILE_ID_CONTROLLERINFO, ci);
  WRITE_APP_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
  WRITE_APP_NVM(FILE_ID_APPLICATIONDATA, ad);
  return true;
}

/*****************************************************************************/
static bool parse_controller_nvm715_json(json_object *jo_ctrl, nvmLayout_t nvm_layout)
{
  SControllerInfo_NVM715 ci = {};
  SApplicationCmdClassInfo ai = {};
  SApplicationData ad = {};
  SApplicationSettings as = {};
  CONTROLLER_CONFIGURATION controller_configuration = {};

  json_get_home_id(jo_ctrl, "homeId", ci.HomeID, 0, JSON_REQUIRED);
  ci.NodeID = json_get_int(jo_ctrl, "nodeId", 0, JSON_REQUIRED);
  // ci.HomeID                  = json_get_int(jo_ctrl,  STRING_CONTROLLER_HOME_ID, 0, JSON_REQUIRED);
  ci.StaticControllerNodeId = json_get_int(jo_ctrl, "staticControllerNodeId", 0, JSON_REQUIRED);
  ci.LastUsedNodeId_LR = json_get_int(jo_ctrl, "lastNodeIdLR", 0, JSON_REQUIRED);
  ci.LastUsedNodeId = json_get_int(jo_ctrl, "lastNodeId", 0, JSON_REQUIRED);
  ci.SucLastIndex = json_get_int(jo_ctrl, "sucLastIndex", 0, JSON_REQUIRED);
  ci.MaxNodeId_LR = json_get_int(jo_ctrl, "maxNodeIdLR", 0, JSON_REQUIRED);
  ci.MaxNodeId = json_get_int(jo_ctrl, "maxNodeId", 0, JSON_REQUIRED);
  ci.ReservedId = json_get_int(jo_ctrl, "reservedId", 0, JSON_REQUIRED);
  ci.ReservedId_LR = json_get_int(jo_ctrl, "reservedIdLR", 0, JSON_REQUIRED);
  ci.SystemState = json_get_int(jo_ctrl, "systemState", 0, JSON_REQUIRED);
  ci.PrimaryLongRangeChannelId = json_get_int(jo_ctrl, "primaryLongRangeChannelId", 0, JSON_REQUIRED);
  ci.DcdcConfig = json_get_int(jo_ctrl, "dcdcConfig", 0, JSON_REQUIRED);

  controller_configuration.controller_is_secondary = json_get_bool(jo_ctrl, "controllerIsSecondary", false, JSON_REQUIRED);
  controller_configuration.controller_on_other_network = json_get_bool(jo_ctrl, "controllerOnOtherNetwork", false, JSON_REQUIRED);
  controller_configuration.controller_nodeid_server_present = json_get_bool(jo_ctrl, "controllerNodeIdServerPresent", false, JSON_REQUIRED);
  controller_configuration.controller_is_real_primary = json_get_bool(jo_ctrl, "controllerIsRealPrimary", true, JSON_REQUIRED);
  controller_configuration.controller_is_suc = json_get_bool(jo_ctrl, "controllerIsSuc", false, JSON_REQUIRED);
  controller_configuration.no_nodes_included = json_get_bool(jo_ctrl, "noNodesIncluded", true, JSON_REQUIRED);
  memcpy(&ci.ControllerConfiguration, &controller_configuration, sizeof(CONTROLLER_CONFIGURATION));
  // SucAwarenessPushNeeded is not used anywhere in the protocol source code
  // ci.SucAwarenessPushNeeded  = json_get_bool(jo_ctrl, STSTRING_SUC_AWARENESS_PUSH_NEEDED, false, JSON_REQUIRED);
  // We don't write ReservedId to the JSON file, so no need to try reading it back
  // ci.ReservedId              = json_get_int(jo_ctrl,  "reservedId", 0, JSON_REQUIRED);

  as.listening = json_get_bool(jo_ctrl, "isListening", true, JSON_REQUIRED);
  as.generic = json_get_int(jo_ctrl, "genericDeviceClass", 0, JSON_REQUIRED);
  as.specific = json_get_int(jo_ctrl, "specificDeviceClass", 0, JSON_REQUIRED);

  json_object *jo_cmd_classes = NULL;
  if (false == json_get_object_error_check(jo_ctrl, "cmdClasses", &jo_cmd_classes, json_type_object, JSON_REQUIRED))
  {
    printf("controller_parse_json:json_get_object_error_check() error: cmdClasses \n");
    return false;
  }
  // printf("jo_cmd_classes: %s\n", json_object_to_json_string(jo_cmd_classes));
  ai.UnSecureIncludedCCLen = json_get_bytearray(jo_cmd_classes, "includedInsecurely", ai.UnSecureIncludedCC, APPL_NODEPARM_MAX, JSON_REQUIRED);
  ai.SecureIncludedSecureCCLen = json_get_bytearray(jo_cmd_classes, "includedSecurelySecureCCs", ai.SecureIncludedSecureCC, APPL_NODEPARM_MAX, JSON_REQUIRED);
  ai.SecureIncludedUnSecureCCLen = json_get_bytearray(jo_cmd_classes, "includedSecurelyInsecureCCs", ai.SecureIncludedUnSecureCC, APPL_NODEPARM_MAX, JSON_REQUIRED);

  json_object *jo_rf_config = NULL;
  if (false == json_get_object_error_check(jo_ctrl, "rfConfig", &jo_rf_config, json_type_object, JSON_REQUIRED))
  {
    printf("controller_parse_json:json_get_object_error_check() error: rfConfig \n");
    return false;
  }
  parse_app_config_json(jo_rf_config, nvm_layout);
  json_object *jo_suc_state = NULL;
  if (json_get_object_error_check(jo_ctrl, "sucUpdateEntries", &jo_suc_state, json_type_array, JSON_REQUIRED))
  {
    parse_suc_state_json(jo_suc_state, &ci.SucLastIndex);
  }

  const char *b64_app_data_string = json_get_string(jo_ctrl, "applicationData", "", JSON_REQUIRED);

  if (strlen(b64_app_data_string) > 0)
  {
    size_t app_data_len = 0;
    uint8_t *app_data_bin = base64_decode(b64_app_data_string,
                                          strlen(b64_app_data_string),
                                          &app_data_len);

    if (NULL == app_data_bin)
    {
      user_message(MSG_ERROR, "ERROR: Could not base64 decode \"applicationData\".\n");
      return false;
    }

    size_t bytes_to_copy = app_data_len;

    if (bytes_to_copy > APPL_DATA_FILE_SIZE)
    {
      // Only generate the warning if we are throwing away non-zero data
      size_t non_null_bytes_to_copy = bytes_to_copy;
      // Search backwards for non-zero values
      while (non_null_bytes_to_copy > 0 && app_data_bin[non_null_bytes_to_copy - 1] == 0)
      {
        --non_null_bytes_to_copy;
      }

      if (non_null_bytes_to_copy > APPL_DATA_FILE_SIZE)
      {
        user_message(MSG_WARNING, "WARNING: \"applicationData\" will be truncated. "
                                  "Bytes with non-zero values: %zu. Max application data size in generated NVM image: %zu.\n",
                     non_null_bytes_to_copy,
                     APPL_DATA_FILE_SIZE);
      }

      bytes_to_copy = APPL_DATA_FILE_SIZE;
    }

    memcpy(ad.extNvm, app_data_bin, bytes_to_copy);
    free(app_data_bin);
  }
  WRITE_APP_NVM(FILE_ID_APPLICATIONSETTINGS, as);
  WRITE_PROT_NVM(FILE_ID_CONTROLLERINFO, ci);
  WRITE_APP_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
  WRITE_APP_NVM(FILE_ID_APPLICATIONDATA, ad);
  return true;
}

static bool parse_controller_nvm719_json(json_object *jo_ctrl, nvmLayout_t nvm_layout)
{
  SControllerInfo_NVM719 ci = {};
  SApplicationCmdClassInfo ai = {};
  SApplicationData ad = {};
  SApplicationSettings as = {};
  dcdc_configuration_file_t dcdc = {};
  CONTROLLER_CONFIGURATION controller_configuration = {};
  char app_name[ZAF_FILE_SIZE_APP_NAME];
  int FILE_ID_APP_NAME_800s = (hardware_info == EFR32XG23) ? ZAF_FILE_ID_APP_NAME_800s_XG23 : ZAF_FILE_ID_APP_NAME_800s_XG28;

  json_get_home_id(jo_ctrl, "homeId", ci.HomeID, 0, JSON_REQUIRED);
  ci.NodeID = json_get_int(jo_ctrl, "nodeId", 0, JSON_REQUIRED);
  // ci.HomeID                  = json_get_int(jo_ctrl,  STRING_CONTROLLER_HOME_ID, 0, JSON_REQUIRED);
  ci.StaticControllerNodeId = json_get_int(jo_ctrl, "staticControllerNodeId", 0, JSON_REQUIRED);
  ci.LastUsedNodeId_LR = json_get_int(jo_ctrl, "lastNodeIdLR", 0, JSON_REQUIRED);
  ci.LastUsedNodeId = json_get_int(jo_ctrl, "lastNodeId", 0, JSON_REQUIRED);
  ci.SucLastIndex = json_get_int(jo_ctrl, "sucLastIndex", 0, JSON_REQUIRED);
  ci.MaxNodeId_LR = json_get_int(jo_ctrl, "maxNodeIdLR", 0, JSON_REQUIRED);
  ci.MaxNodeId = json_get_int(jo_ctrl, "maxNodeId", 0, JSON_REQUIRED);
  ci.ReservedId = json_get_int(jo_ctrl, "reservedId", 0, JSON_REQUIRED);
  ci.ReservedId_LR = json_get_int(jo_ctrl, "reservedIdLR", 0, JSON_REQUIRED);
  ci.SystemState = json_get_int(jo_ctrl, "systemState", 0, JSON_REQUIRED);
  ci.PrimaryLongRangeChannelId = json_get_int(jo_ctrl, "primaryLongRangeChannelId", 0, JSON_REQUIRED);

  ci.PrimaryLongRangeChannelId = json_get_int(jo_ctrl, "primaryLongRangeChannelId", 0, JSON_REQUIRED);
  ci.LongRangeChannelAutoMode = json_get_bool(jo_ctrl, "longRangeChannelAutoMode", false, JSON_REQUIRED);
  int16_t dcdc_config = json_get_int(jo_ctrl, "dcdcConfig", 0, JSON_REQUIRED);

  controller_configuration.controller_is_secondary = json_get_bool(jo_ctrl, "controllerIsSecondary", false, JSON_REQUIRED);
  controller_configuration.controller_on_other_network = json_get_bool(jo_ctrl, "controllerOnOtherNetwork", false, JSON_REQUIRED);
  controller_configuration.controller_nodeid_server_present = json_get_bool(jo_ctrl, "controllerNodeIdServerPresent", false, JSON_REQUIRED);
  controller_configuration.controller_is_real_primary = json_get_bool(jo_ctrl, "controllerIsRealPrimary", true, JSON_REQUIRED);
  controller_configuration.controller_is_suc = json_get_bool(jo_ctrl, "controllerIsSuc", false, JSON_REQUIRED);
  controller_configuration.no_nodes_included = json_get_bool(jo_ctrl, "noNodesIncluded", true, JSON_REQUIRED);
  memcpy(&ci.ControllerConfiguration, &controller_configuration, sizeof(CONTROLLER_CONFIGURATION));
  // SucAwarenessPushNeeded is not used anywhere in the protocol source code
  // ci.SucAwarenessPushNeeded  = json_get_bool(jo_ctrl, STSTRING_SUC_AWARENESS_PUSH_NEEDED, false, JSON_REQUIRED);
  // We don't write ReservedId to the JSON file, so no need to try reading it back
  // ci.ReservedId              = json_get_int(jo_ctrl,  "reservedId", 0, JSON_REQUIRED);

  as.listening = json_get_bool(jo_ctrl, "isListening", true, JSON_REQUIRED);
  as.generic = json_get_int(jo_ctrl, "genericDeviceClass", 0, JSON_REQUIRED);
  as.specific = json_get_int(jo_ctrl, "specificDeviceClass", 0, JSON_REQUIRED);

  json_object *jo_cmd_classes = NULL;
  if (false == json_get_object_error_check(jo_ctrl, "cmdClasses", &jo_cmd_classes, json_type_object, JSON_REQUIRED))
  {
    printf("controller_parse_json:json_get_object_error_check() error: cmdClasses \n");
    return false;
  }
  // printf("jo_cmd_classes: %s\n", json_object_to_json_string(jo_cmd_classes));
  ai.UnSecureIncludedCCLen = json_get_bytearray(jo_cmd_classes, "includedInsecurely", ai.UnSecureIncludedCC, APPL_NODEPARM_MAX, JSON_REQUIRED);
  ai.SecureIncludedSecureCCLen = json_get_bytearray(jo_cmd_classes, "includedSecurelySecureCCs", ai.SecureIncludedSecureCC, APPL_NODEPARM_MAX, JSON_REQUIRED);
  ai.SecureIncludedUnSecureCCLen = json_get_bytearray(jo_cmd_classes, "includedSecurelyInsecureCCs", ai.SecureIncludedUnSecureCC, APPL_NODEPARM_MAX, JSON_REQUIRED);

  json_object *jo_rf_config = NULL;
  if (false == json_get_object_error_check(jo_ctrl, "rfConfig", &jo_rf_config, json_type_object, JSON_REQUIRED))
  {
    printf("controller_parse_json:json_get_object_error_check() error: rfConfig \n");
    return false;
  }
  parse_app_config_json(jo_rf_config, nvm_layout);
  json_object *jo_suc_state = NULL;
  if (json_get_object_error_check(jo_ctrl, "sucUpdateEntries", &jo_suc_state, json_type_array, JSON_REQUIRED))
  {
    parse_suc_state_json_nvm719(jo_suc_state, &ci.SucLastIndex);
  }

  const char *b64_app_data_string = json_get_string(jo_ctrl, "applicationData", "", JSON_REQUIRED);

  if (strlen(b64_app_data_string) > 0)
  {
    size_t app_data_len = 0;
    uint8_t *app_data_bin = base64_decode(b64_app_data_string,
                                          strlen(b64_app_data_string),
                                          &app_data_len);

    if (NULL == app_data_bin)
    {
      user_message(MSG_ERROR, "ERROR: Could not base64 decode \"applicationData\".\n");
      return false;
    }

    size_t bytes_to_copy = app_data_len;

    if (bytes_to_copy > APPL_DATA_FILE_SIZE)
    {
      // Only generate the warning if we are throwing away non-zero data
      size_t non_null_bytes_to_copy = bytes_to_copy;
      // Search backwards for non-zero values
      while (non_null_bytes_to_copy > 0 && app_data_bin[non_null_bytes_to_copy - 1] == 0)
      {
        --non_null_bytes_to_copy;
      }

      if (non_null_bytes_to_copy > APPL_DATA_FILE_SIZE)
      {
        user_message(MSG_WARNING, "WARNING: \"applicationData\" will be truncated. "
                                  "Bytes with non-zero values: %zu. Max application data size in generated NVM image: %zu.\n",
                     non_null_bytes_to_copy,
                     APPL_DATA_FILE_SIZE);
      }

      bytes_to_copy = APPL_DATA_FILE_SIZE;
    }

    memcpy(ad.extNvm, app_data_bin, bytes_to_copy);
    free(app_data_bin);
  }

  WRITE_PROT_NVM(FILE_ID_CONTROLLERINFO, ci);
  if (NVM3_700s == nvm_layout)
  {
    WRITE_APP_NVM(FILE_ID_APPLICATIONSETTINGS, as);
    WRITE_APP_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
    WRITE_APP_NVM(FILE_ID_APPLICATIONDATA, ad);
  }
  else
  {
    WRITE_PROT_NVM(FILE_ID_APPLICATIONSETTINGS, as);
    WRITE_PROT_NVM(FILE_ID_APPLICATIONCMDINFO, ai);
    WRITE_PROT_NVM(FILE_ID_APPLICATIONDATA, ad);
  }

  if (target_protocol_version.major == 7 && target_protocol_version.minor >= 20)
  {
    const char *app_name_src = json_get_string(jo_ctrl, "applicationName", "", JSON_OPTIONAL);
    strncpy(app_name, app_name_src, sizeof(app_name) - 1);
    app_name[sizeof(app_name) - 1] = '\0'; // Ensure null termination
    if (NVM3_700s == nvm_layout)
    {
      WRITE_APP_NVM(ZAF_FILE_ID_APP_NAME, app_name);
    }
    else
    {
      WRITE_PROT_NVM(FILE_ID_APP_NAME_800s, app_name);
    }
  }

  if (dcdc_config >= 0)
  {
    dcdc.dcdc_config = dcdc_config;
    WRITE_PROT_NVM(FILE_ID_PROPRIETARY_1, dcdc);
  }
  return true;
}

/*****************************************************************************/
nvmLayout_t json_get_nvm_layout(const char *device_info, json_object *jo)
{
  json_object *jo_ref = NULL;
  const char *protocol_version;
  const char *app_version;
  nvmLayout_t nvm_layout = NVM3_700s;

  if (false == json_get_object_error_check(jo, "controller", &jo_ref, json_type_object, JSON_REQUIRED))
  {
    printf("json_get_nvm_layout: json_get_object_error_check() error: controller\n");
    return nvm_layout;
  }

  protocol_version = json_get_string(jo_ref, "protocolVersion", "", JSON_REQUIRED);
  app_version = json_get_string(jo_ref, "applicationVersion", "", JSON_REQUIRED);
  target_app_version.format = json_get_int(jo, "applicationFileFormat", 0, JSON_REQUIRED);

  if (protocol_version == NULL)
  {
    printf("json_get_nvm_layout: Failed to retrieve protocolVersion\n");
    return nvm_layout;
  }
  if (app_version == NULL)
  {
    printf("json_get_nvm_layout: Failed to retrieve appVersion\n");
    return nvm_layout;
  }

  if (false == set_target_version(protocol_version, app_version))
  {
    printf("json_get_nvm_layout:set_target_version() error\n");
    return false;
  }

  if (strcmp(device_info, "EFR32XG28") == 0 || strcmp(device_info, "EFR32XG23") == 0)
  {
    if (strncmp(protocol_version, "7.19", 4) >= 0)
    {
      nvm_layout = NVM3_800s_FROM_719;
      hardware_info = (strcmp(device_info, "EFR32XG28") == 0) ? EFR32XG28 : EFR32XG23;
    }
    else
    {
      nvm_layout = NVM3_800s_PRIOR_719;
      hardware_info = (strcmp(device_info, "EFR32XG28") == 0) ? EFR32XG28 : EFR32XG23;
    }
  }
  else if (strcmp(device_info, "EFR32XG14") == 0 || strcmp(device_info, "EFR32XG13") == 0)
  {
    nvm_layout = NVM3_700s;
    hardware_info = ZW_700s;
  }

  return nvm_layout;
}

/*****************************************************************************/
bool controller_parse_json(json_object *jo, nvmLayout_t nvm_layout)
{
  json_object *jo_ref = NULL;
  uint8_t major_prot_version = 0;
  uint8_t minor_prot_version = 0;
  uint8_t patch_prot_version = 0;
  uint8_t major_app_version = 0;
  uint8_t minor_app_version = 0;
  uint8_t patch_app_version = 0;

  set_json_parse_error_flag(false);
  reset_nvm3_error_status();

  /* We need this to provide detailed error messages */
  json_register_root(jo);

  if (false == create_nvm3_version_files(nvm_layout))
  {
    printf("controller_parse_json:create_nvm3_version_files() error\n");
    return false;
  }

  if (false == json_get_object_error_check(jo, "controller", &jo_ref, json_type_object, JSON_REQUIRED))
  {
    printf("controller_parse_json:json_get_object_error_check() error: controller\n");
    return false;
  }

  if ((target_protocol_version.major == 7 && target_protocol_version.minor >= 19))
  {
    if (false == parse_controller_nvm719_json(jo_ref, nvm_layout))
    {
      printf("controller_parse_json:parse_controller_nvm719_json() error\n");
      return false;
    }
  }
  else if ((target_protocol_version.major == 7 && target_protocol_version.minor >= 15))
  {
    if (false == parse_controller_nvm715_json(jo_ref, nvm_layout))
    {
      printf("controller_parse_json:parse_controller_nvm715_json() error\n");
      return false;
    }
  }
  else
  {
    if (false == parse_controller_nvm711_json(jo_ref, nvm_layout))
    {
      printf("controller_parse_json:parse_controller_nvm711_json() error\n");
      return false;
    }
  }

  json_object *jo_node_table = NULL;
  if (json_get_object_error_check(jo, "nodes", &jo_node_table, json_type_object, JSON_REQUIRED))
  {
    if (false == parse_node_table_json(jo_node_table))
    {
      /* Here we simply record that the JSON is bad and keep going to reveal
       * any oyther errors. The final status will be "failed" due to this.
       */
      set_json_parse_error_flag(true);
    }
  }

  json_object *jo_lr_node_table = NULL;
  if (json_get_object_error_check(jo, "lrNodes", &jo_lr_node_table, json_type_object, JSON_REQUIRED))
  {
    if (false == parse_lr_node_table_json(jo_lr_node_table))
    {
      /* Here we simply record that the JSON is bad and keep going to reveal
       * any oyther errors. The final status will be "failed" due to this.
       */
      set_json_parse_error_flag(true);
    }
  }

  if (was_nvm3_error_detected())
  {
    printf("controller_parse_json:was_nvm3_error_detected()\n");
  }

  // if (json_parse_error_detected()) {
  //   printf("controller_parse_json:json_parse_error_detected()\n");
  //   return false;
  // }

  return true;
}

// Determine the NVM layout based on the NVM image size
// input: nvm_image_size
// output: NVM layout type (700s, 800s before 7.19, 800s from 7.19)
nvmLayout_t read_layout_from_nvm_size(size_t nvm_image_size)
{
  nvmLayout_t nvm_layout = NVM3_700s;
  if (nvm_image_size == 0xA000)
  {
    nvm_layout = NVM3_800s_FROM_719;
    // printf("[DEBUG LOG]: This is 800s from 7.19\n");
  }
  else if (nvm_image_size == 0xC000)
  {
    nvm_layout = NVM3_700s;
    // printf("[DEBUG LOG]: This is 700s\n");
  }
  else if (nvm_image_size == 0x10000)
  {
    nvm_layout = NVM3_800s_PRIOR_719;
    // printf("[DEBUG LOG]: This is 800s before 7.19\n");
  }
  return nvm_layout;
}
