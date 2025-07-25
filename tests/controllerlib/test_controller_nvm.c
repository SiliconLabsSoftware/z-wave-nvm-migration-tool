#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "ZW_nodemask_api_mock.h"
#include "json_helpers_mock.h"
#include "nvm3_helpers_mock.h"
#include "nvm3_stubs_mock.h"
#include "user_message_mock.h"
#include "base64_mock.h"

#include "controller_nvm.c"

#define RESET_TARGET_VERSION()\
  memset(&target_app_version, 0, sizeof(target_version)); \
  memset(&target_protocol_version, 0, sizeof(target_version));

/* Global mock variables */
int mock_int_array[10] = {};
uint8_t mock_byte_array[10] = {};

/* Mocked functions */
/* json get functions */
uint8_t json_get_intarray_Callback(json_object *parent, const char *key, int *array, uint32_t length, bool is_required, int cmock_num_calls)
{
  for (uint32_t i = 0; i < length; i++)
  {
    array[i] = mock_int_array[i];
  }
  return length;
}

uint8_t json_get_bytearray_Callback(json_object *parent, const char *key, uint8_t *array, uint32_t length, bool is_required, int cmock_num_calls)
{
  for (uint32_t i = 0; i < length; i++)
  {
    array[i] = mock_byte_array[i];
  }
  return length;
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_set_target_version(void)
{
  /* True if version is supported and set target_protocol_version
  target_app_version accordingly */
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.11.0", "7.11.0"));
  TEST_ASSERT_EQUAL_INT(0, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.12.0", "7.12.0"));
  TEST_ASSERT_EQUAL_INT(1, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.15.0", "7.15.0"));
  TEST_ASSERT_EQUAL_INT(2, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.16.0", "7.16.0"));
  TEST_ASSERT_EQUAL_INT(3, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.17.0", "7.17.0"));
  TEST_ASSERT_EQUAL_INT(4, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.18.0", "7.18.0"));
  TEST_ASSERT_EQUAL_INT(4, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.19.0", "7.19.0"));
  TEST_ASSERT_EQUAL_INT(5, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.20.0", "7.20.0"));
  TEST_ASSERT_EQUAL_INT(5, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.21.0", "7.21.0"));
  TEST_ASSERT_EQUAL_INT(5, target_protocol_version.format);
  TEST_ASSERT_EQUAL_INT(5, target_protocol_version.format);
  RESET_TARGET_VERSION();
  TEST_ASSERT_TRUE(set_target_version("7.23.0", "7.23.0"));
  TEST_ASSERT_EQUAL_INT(5, target_protocol_version.format);
  RESET_TARGET_VERSION();
  /* False if version is not supported */
  TEST_ASSERT_FALSE(set_target_version("8.23.0", "8.23.0"));
  TEST_ASSERT_FALSE(set_target_version("7.24.0", "7.24.0"));
  TEST_ASSERT_FALSE(set_target_version("7.24", "7.24.0"));
  TEST_ASSERT_FALSE(set_target_version("7.24.0", "7"));
}

void test_app_nvm_is_pre_v7_15_3(void)
{
  /* True if app_nvm_version is less than 7.15.3 */
  TEST_ASSERT_TRUE(app_nvm_is_pre_v7_15_3(7, 15, 2));
  TEST_ASSERT_TRUE(app_nvm_is_pre_v7_15_3(7, 11, 0));
  /* False if app_nvm_version is 7.15.3 or greater */
  TEST_ASSERT_FALSE(app_nvm_is_pre_v7_15_3(7, 15, 4));
  TEST_ASSERT_FALSE(app_nvm_is_pre_v7_15_3(7, 16, 0));
}

void test_app_nvm_is_pre_v7_18_1(void)
{
  /* True if app_nvm_version is less than 7.18.1 */
  TEST_ASSERT_TRUE(app_nvm_is_pre_v7_18_1(7, 18, 0));
  TEST_ASSERT_TRUE(app_nvm_is_pre_v7_18_1(7, 17, 0));
  /* False if app_nvm_version is 7.18.1 or greater */
  TEST_ASSERT_FALSE(app_nvm_is_pre_v7_18_1(7, 18, 2));
  TEST_ASSERT_FALSE(app_nvm_is_pre_v7_18_1(7, 19, 0));
}

void test_app_nvm_is_v7_19(void)
{
  /* True if app_nvm_version is exactly 7.19.0 */
  TEST_ASSERT_TRUE(app_nvm_is_v7_19(7, 19, 0));
  TEST_ASSERT_TRUE(app_nvm_is_v7_19(7, 19, 1));
  /* False if app_nvm_version is not 7.19.0 */
  TEST_ASSERT_FALSE(app_nvm_is_v7_19(7, 18, 0));
}

void test_app_nvm_is_v7_20(void)
{
  /* True if app_nvm_version is exactly 7.20.0 */
  TEST_ASSERT_TRUE(app_nvm_is_v7_20(7, 20, 0));
  TEST_ASSERT_TRUE(app_nvm_is_v7_20(7, 20, 1));
  /* False if app_nvm_version is not 7.20.0 */
  TEST_ASSERT_FALSE(app_nvm_is_v7_20(7, 18, 0));
}

void test_app_nvm_is_v7_21(void)
{
  /* True if app_nvm_version is exactly 7.21.0 */
  TEST_ASSERT_TRUE(app_nvm_is_v7_21(7, 21, 0));
  TEST_ASSERT_TRUE(app_nvm_is_v7_21(7, 21, 1));
  /* False if app_nvm_version is not 7.21.0 */
  TEST_ASSERT_FALSE(app_nvm_is_v7_21(7, 20, 0));
}

void test_app_nvm_is_v7_22(void)
{
  /* True if app_nvm_version is exactly 7.22.0 */
  TEST_ASSERT_TRUE(app_nvm_is_v7_22(7, 22, 0));
  TEST_ASSERT_TRUE(app_nvm_is_v7_22(7, 22, 1));
  /* False if app_nvm_version is not 7.22.0 */
  TEST_ASSERT_FALSE(app_nvm_is_v7_22(7, 21, 1));
}

void test_app_nvm_is_pre_v7_19(void)
{
  /* True if app_nvm_version is less than 7.19.0 */
  TEST_ASSERT_TRUE(app_nvm_is_pre_v7_19(7, 18, 0));
  /* False if app_nvm_version is 7.19.0 or greater */
  TEST_ASSERT_FALSE(app_nvm_is_pre_v7_19(7, 19, 0));
}

void test_app_nvm_is_pre_v7_21(void)
{
  /* True if app_nvm_version is less than 7.21.0 */
  TEST_ASSERT_TRUE(app_nvm_is_pre_v7_21(7, 20, 0));
  /* False if app_nvm_version is 7.21.0 or greater */
  TEST_ASSERT_FALSE(app_nvm_is_pre_v7_21(7, 21, 0));
}

void test_check_data_rate(void)
{
  const int supported_data_rate[] = {9600, 40000};
  TEST_ASSERT_TRUE(check_data_rate(supported_data_rate, 9600, 2));
  TEST_ASSERT_TRUE(check_data_rate(supported_data_rate, 40000, 2));
  TEST_ASSERT_FALSE(check_data_rate(supported_data_rate, 100000, 2));
}
void test_parse_node_capability(void)
{
  const char *json_capability = "{\
    \"isListening\": true,\
    \"isRouting\": false,\
    \"supportedDataRates\": [9600, 40000, 100000],\
    \"protocolVersion\": \"3\"\
}";
  json_object *jo_node = json_tokener_parse(json_capability);
  uint8_t node_reserved = 0;
  mock_int_array[0] = 9600;
  mock_int_array[1] = 40000;
  mock_int_array[2] = 100000;
  json_get_bool_ExpectAndReturn(jo_node, "isListening", true, JSON_REQUIRED, true);
  json_get_bool_ExpectAndReturn(jo_node, "isRouting", false, JSON_REQUIRED, false);
  json_get_intarray_StubWithCallback(json_get_intarray_Callback);
  json_get_int_ExpectAndReturn(jo_node, "protocolVersion", 1, JSON_REQUIRED, 3);
  uint8_t capability = parse_node_capability(jo_node, &node_reserved);
  TEST_ASSERT_EQUAL_HEX(ZWAVE_NODEINFO_LISTENING_SUPPORT |
                            ZWAVE_NODEINFO_BAUD_9600 |
                            ZWAVE_NODEINFO_BAUD_40000 |
                            0x03,
                        capability);
  TEST_ASSERT_TRUE(node_reserved & ZWAVE_NODEINFO_BAUD_100K);
  json_object_put(jo_node);
}
void test_parse_node_security(void)
{
  const char *json_security_tc1 = "{\
    \"isFrequentListening\": false,\
    \"optionalFunctionality\": true,\
    \"nodeType\": 1,\
    \"supportesSecurity\": true,\
    \"supportesBeaming\": true,\
}";
  json_object *jo = json_tokener_parse(json_security_tc1);
  json_get_bool_ExpectAndReturn(jo, "optionalFunctionality", false, JSON_REQUIRED, true);
  json_get_int_ExpectAndReturn(jo, "nodeType", 0, JSON_REQUIRED, 1);
  json_get_bool_ExpectAndReturn(jo, "supportsSecurity", true, JSON_REQUIRED, true);
  json_get_bool_ExpectAndReturn(jo, "supportsBeaming", true, JSON_REQUIRED, true);
  uint8_t security = parse_node_security(jo);
  TEST_ASSERT_EQUAL_HEX(ZWAVE_NODEINFO_OPTIONAL_FUNC |
                            0x08 |
                            ZWAVE_NODEINFO_SECURITY_SUPPORT |
                            ZWAVE_NODEINFO_BEAM_CAPABILITY,
                        security);
  const char *json_security_tc2 = "{\
    \"isFrequentListening\": \"250ms\",\
    \"optionalFunctionality\": true,\
    \"nodeType\": 1,\
    \"supportesSecurity\": true,\
    \"supportesBeaming\": true,\
}";
  jo = json_tokener_parse(json_security_tc2);
  json_get_string_ExpectAndReturn(jo, "isFrequentListening", "250ms", JSON_REQUIRED, "250ms");
  json_get_bool_ExpectAndReturn(jo, "optionalFunctionality", false, JSON_REQUIRED, true);
  json_get_int_ExpectAndReturn(jo, "nodeType", 0, JSON_REQUIRED, 1);
  json_get_bool_ExpectAndReturn(jo, "supportsSecurity", true, JSON_REQUIRED, true);
  json_get_bool_ExpectAndReturn(jo, "supportsBeaming", true, JSON_REQUIRED, true);
  security = parse_node_security(jo);
  TEST_ASSERT_EQUAL_HEX(ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250 |
                            ZWAVE_NODEINFO_OPTIONAL_FUNC |
                            0x08 |
                            ZWAVE_NODEINFO_SECURITY_SUPPORT |
                            ZWAVE_NODEINFO_BEAM_CAPABILITY,
                        security);
  const char *json_security_tc3 = "{\
    \"isFrequentListening\": \"1000ms\",\
    \"optionalFunctionality\": true,\
    \"nodeType\": 1,\
    \"supportesSecurity\": true,\
    \"supportesBeaming\": true,\
}";
  jo = json_tokener_parse(json_security_tc3);
  json_get_string_ExpectAndReturn(jo, "isFrequentListening", "250ms", JSON_REQUIRED, "1000ms");
  json_get_bool_ExpectAndReturn(jo, "optionalFunctionality", false, JSON_REQUIRED, true);
  json_get_int_ExpectAndReturn(jo, "nodeType", 0, JSON_REQUIRED, 1);
  json_get_bool_ExpectAndReturn(jo, "supportsSecurity", true, JSON_REQUIRED, true);
  json_get_bool_ExpectAndReturn(jo, "supportsBeaming", true, JSON_REQUIRED, true);
  security = parse_node_security(jo);
  TEST_ASSERT_EQUAL_HEX(ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000 |
                            ZWAVE_NODEINFO_OPTIONAL_FUNC |
                            0x08 |
                            ZWAVE_NODEINFO_SECURITY_SUPPORT |
                            ZWAVE_NODEINFO_BEAM_CAPABILITY,
                        security);
  json_object_put(jo);
}
void test_parse_route_cache_line_json(void)
{
  const char *json_route_cache_line_tc1 = "{\
    \"beaming\": \"250ms\",\
    \"protocolRate\": 3,\
    \"repeaterNodeIDs\": [2, 3, 4, 5]\
  }";
  mock_byte_array[0] = 2;
  mock_byte_array[1] = 3;
  mock_byte_array[2] = 4;
  mock_byte_array[3] = 5;
  json_object *jo = json_tokener_parse(json_route_cache_line_tc1);
  ROUTECACHE_LINE rcl = {0};
  json_get_string_ExpectAndReturn(jo, "beaming", "250ms", JSON_OPTIONAL, "250ms");
  json_get_int_ExpectAndReturn(jo, "protocolRate", 0, JSON_OPTIONAL, 3);
  json_get_bytearray_StubWithCallback(json_get_bytearray_Callback);
  TEST_ASSERT_TRUE(parse_route_cache_line_json(jo, &rcl));
  TEST_ASSERT_EQUAL_HEX(0x20 | 0x03, rcl.routecacheLineConfSize); // 0x20 for beaming capability
  TEST_ASSERT_EQUAL_INT(4, sizeof(rcl.repeaterList) / sizeof(rcl.repeaterList[0]));
  TEST_ASSERT_EQUAL_INT(2, rcl.repeaterList[0]);
  TEST_ASSERT_EQUAL_INT(3, rcl.repeaterList[1]);
  TEST_ASSERT_EQUAL_INT(4, rcl.repeaterList[2]);
  TEST_ASSERT_EQUAL_INT(5, rcl.repeaterList[3]);
  const char *json_route_cache_line_tc2 = "{\
    \"beaming\": \"1000ms\",\
    \"protocolRate\": 2,\
    \"repeaterNodeIDs\": [2, 3, 4, 5]\
  }";
  jo = json_tokener_parse(json_route_cache_line_tc2);
  json_get_string_ExpectAndReturn(jo, "beaming", "250ms", JSON_OPTIONAL, "1000ms");
  json_get_int_ExpectAndReturn(jo, "protocolRate", 0, JSON_OPTIONAL, 2);
  json_get_bytearray_StubWithCallback(json_get_bytearray_Callback);
  TEST_ASSERT_TRUE(parse_route_cache_line_json(jo, &rcl));
  TEST_ASSERT_EQUAL_HEX(0x40 | 0x02, rcl.routecacheLineConfSize); // 0x20 for beaming capability
  TEST_ASSERT_EQUAL_INT(4, sizeof(rcl.repeaterList) / sizeof(rcl.repeaterList[0]));
  TEST_ASSERT_EQUAL_INT(2, rcl.repeaterList[0]);
  TEST_ASSERT_EQUAL_INT(3, rcl.repeaterList[1]);
  TEST_ASSERT_EQUAL_INT(4, rcl.repeaterList[2]);
  TEST_ASSERT_EQUAL_INT(5, rcl.repeaterList[3]);
  const char *json_route_cache_line_tc3 = "{\
    \"beaming\": false,\
    \"protocolRate\": 2,\
    \"repeaterNodeIDs\": [2, 3, 4, 5]\
  }";
  jo = json_tokener_parse(json_route_cache_line_tc3);
  json_get_int_ExpectAndReturn(jo, "protocolRate", 0, JSON_OPTIONAL, 2);
  json_get_bytearray_StubWithCallback(json_get_bytearray_Callback);
  TEST_ASSERT_TRUE(parse_route_cache_line_json(jo, &rcl));
  TEST_ASSERT_EQUAL(0x02, rcl.routecacheLineConfSize); // 0x20 for beaming capability
  TEST_ASSERT_EQUAL_INT(4, sizeof(rcl.repeaterList) / sizeof(rcl.repeaterList[0]));
  TEST_ASSERT_EQUAL_INT(2, rcl.repeaterList[0]);
  TEST_ASSERT_EQUAL_INT(3, rcl.repeaterList[1]);
  TEST_ASSERT_EQUAL_INT(4, rcl.repeaterList[2]);
  TEST_ASSERT_EQUAL_INT(5, rcl.repeaterList[3]);
  json_object_put(jo);
}
// parse_node_table_json
// parse_lr_node_table_json
void test_parse_suc_state_json(void)
{
  RESET_TARGET_VERSION();
  set_target_version("7.11.0", "7.11.0");
  const char *json_str =
      "        ["
      "        {\"nodeId\":5,\n"
      "        \"changeType\":1,\n"
      "        \"supportedCCs\":[\n"
      "          94,\n"
      "          85,\n"
      "          152,\n"
      "          159,\n"
      "          108\n"
      "        ],\n"
      "        \"controlledCCs\":[\n"
      "          94,\n"
      "          85,\n"
      "          152,\n"
      "          159\n"
      "        ],\n"
      "      }"
      "      ]";
  json_object *jo_suc = json_tokener_parse(json_str);
  uint8_t last_suc_index = 255;
  SSucNodeList suc_node_list;
  memset(&suc_node_list, 0xFF, sizeof(suc_node_list));
  SUC_UPDATE_ENTRY_STRUCT aSucNodeList = {5, 1, {94, 85, 152, 159, 108, 239, 94, 85, 152, 159}};
  memcpy(&suc_node_list, &aSucNodeList, sizeof(SUC_UPDATE_ENTRY_STRUCT));
  json_get_int_ExpectAndReturn(json_object_array_get_idx(jo_suc, 0), "nodeId", 0, JSON_REQUIRED, 5);
  json_get_int_ExpectAndReturn(json_object_array_get_idx(jo_suc, 0), "changeType", 0, JSON_REQUIRED, 1);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_protocol_handle, FILE_ID_SUCNODELIST, &suc_node_list, sizeof(suc_node_list), nvm3_current_protocol_files, nvm3_current_protocol_files_size, ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_suc_state_json(jo_suc, &last_suc_index));
  json_object_put(jo_suc);
}

void test_parse_suc_state_json_from_nvm719(void)
{
  RESET_TARGET_VERSION();
  set_target_version("7.19.0", "7.19.0");
  const char *json_str =
      "        ["
      "        {\"nodeId\":5,\n"
      "        \"changeType\":1,\n"
      "        \"supportedCCs\":[\n"
      "          94,\n"
      "          85,\n"
      "          152,\n"
      "          159,\n"
      "          108\n"
      "        ],\n"
      "        \"controlledCCs\":[\n"
      "          94,\n"
      "          85,\n"
      "          152,\n"
      "          159\n"
      "        ],\n"
      "      }"
      "      ]";
  json_object *jo_suc = json_tokener_parse(json_str);
  uint8_t last_suc_index = 255;
  SSucNodeList_V5 suc_node_list;
  memset(&suc_node_list, 0xFF, sizeof(suc_node_list));
  SUC_UPDATE_ENTRY_STRUCT aSucNodeList = {5, 1, {94, 85, 152, 159, 108, 239, 94, 85, 152, 159}};
  memcpy(&suc_node_list, &aSucNodeList, sizeof(SUC_UPDATE_ENTRY_STRUCT));
  json_get_int_ExpectAndReturn(json_object_array_get_idx(jo_suc, 0), "nodeId", 0, JSON_REQUIRED, 5);
  json_get_int_ExpectAndReturn(json_object_array_get_idx(jo_suc, 0), "changeType", 0, JSON_REQUIRED, 1);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_protocol_handle, FILE_ID_SUCNODELIST_BASE_V5, &suc_node_list, sizeof(suc_node_list), nvm3_current_protocol_files, sizeof_array(nvm3_current_protocol_files), ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_suc_state_json(jo_suc, &last_suc_index));
  json_object_put(jo_suc);
}
void test_parse_app_config_json_prior_7_15_3(void)
{
  RESET_TARGET_VERSION();
  set_target_version("7.11.0", "7.11.0");
  const char *json_app_config = "{\
    \"rfRegion\": 0,\
    \"txPower\": 138,\
    \"measured0dBm\": 4\
  }";
  json_object *jo = json_tokener_parse(json_app_config);
  SApplicationConfiguration_prior_7_15_3 ac= {
    .rfRegion = 0,
    .iTxPower = 138,
    .ipower0dbmMeasured = 4
  };
  json_get_int_ExpectAndReturn(jo, "rfRegion", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "txPower", 0, JSON_OPTIONAL, 138);
  json_get_int_ExpectAndReturn(jo, "measured0dBm", 0, JSON_OPTIONAL, 4);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_app_handle, FILE_ID_APPLICATIONCONFIGURATION, &ac, sizeof(ac), nvm3_app_files, sizeof_array(nvm3_app_files), ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_app_config_json(jo, NVM3_700s));
  json_object_put(jo);
}
void test_parse_app_config_prior_7_18_1(void)
{
  RESET_TARGET_VERSION();
  set_target_version("7.17.0", "7.17.0");
  const char *json_app_config= "{\
    \"rfRegion\": 0,\
    \"txPower\": 138,\
    \"enablePTI\": 0,\
    \"maxTxPower\": 140,\
    \"measured0dBm\": 4\
  }";
  json_object *jo = json_tokener_parse(json_app_config);
  SApplicationConfiguration_prior_7_18_1 ac= {
    .rfRegion = 0,
    .iTxPower = 138,
    .enablePTI = 0, 
    .maxTxPower = 140, 
    .ipower0dbmMeasured = 4
  };
  json_get_int_ExpectAndReturn(jo, "rfRegion", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "txPower", 0, JSON_OPTIONAL, 138);
  json_get_int_ExpectAndReturn(jo, "measured0dBm", 0, JSON_OPTIONAL, 4);
  json_get_bool_ExpectAndReturn(jo, "enablePTI", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "maxTxPower", 140, JSON_OPTIONAL, 140);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_app_handle, FILE_ID_APPLICATIONCONFIGURATION, &ac, sizeof(ac), nvm3_app_files, sizeof_array(nvm3_app_files), ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_app_config_json(jo, NVM3_700s));
  json_object_put(jo);
}
void test_parse_app_config_pre_7_19(void)
{
  RESET_TARGET_VERSION();
  set_target_version("7.18.8", "7.18.8");
  const char *json_app_config= "{\
    \"rfRegion\": 0,\
    \"txPower\": 138,\
    \"enablePTI\": 0,\
    \"maxTxPower\": 140,\
    \"measured0dBm\": 4\
  }";
  json_object *jo = json_tokener_parse(json_app_config);
  SApplicationConfiguration_7_18_1 ac = {
    .rfRegion = 0,
    .iTxPower = 138,
    .ipower0dbmMeasured = 4,
    .enablePTI = 0,
    .maxTxPower = 140
  };
  json_get_int_ExpectAndReturn(jo, "rfRegion", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "txPower", 0, JSON_OPTIONAL, 138);
  json_get_int_ExpectAndReturn(jo, "measured0dBm", 0, JSON_OPTIONAL, 4);
  json_get_bool_ExpectAndReturn(jo, "enablePTI", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "maxTxPower", 140, JSON_OPTIONAL, 140);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_app_handle, FILE_ID_APPLICATIONCONFIGURATION, &ac, sizeof(ac), nvm3_app_files, sizeof_array(nvm3_app_files), ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_app_config_json(jo, NVM3_700s));
  json_object_put(jo);
}

void test_parse_app_config_pre_7_21(void)
{
  RESET_TARGET_VERSION();
  set_target_version("7.19.0", "7.19.0");
  const char *json_app_config= "{\
    \"rfRegion\": 0,\
    \"txPower\": 138,\
    \"enablePTI\": 0,\
    \"maxTxPower\": 140,\
    \"measured0dBm\": 4\
  }";
  json_object *jo = json_tokener_parse(json_app_config);
  SApplicationConfiguration_7_18_1 ac = {
    .rfRegion = 0,
    .iTxPower = 138,
    .ipower0dbmMeasured = 4,
    .enablePTI = 0,
    .maxTxPower = 140
  };
  // 700 series, write to app region
  json_get_int_ExpectAndReturn(jo, "rfRegion", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "txPower", 0, JSON_OPTIONAL, 138);
  json_get_int_ExpectAndReturn(jo, "measured0dBm", 0, JSON_OPTIONAL, 4);
  json_get_bool_ExpectAndReturn(jo, "enablePTI", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "maxTxPower", 140, JSON_OPTIONAL, 140);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_app_handle, FILE_ID_APPLICATIONCONFIGURATION, &ac, sizeof(ac), nvm3_app_files, sizeof_array(nvm3_app_files), ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_app_config_json(jo, NVM3_700s));
  // 800 series, write to 1 single region 
  json_get_int_ExpectAndReturn(jo, "rfRegion", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "txPower", 0, JSON_OPTIONAL, 138);
  json_get_int_ExpectAndReturn(jo, "measured0dBm", 0, JSON_OPTIONAL, 4);
  json_get_bool_ExpectAndReturn(jo, "enablePTI", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "maxTxPower", 140, JSON_OPTIONAL, 140);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_protocol_handle, FILE_ID_APPLICATIONCONFIGURATION, &ac, sizeof(ac), nvm3_current_protocol_files, nvm3_current_protocol_files_size, ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_app_config_json(jo, NVM3_800s_FROM_719));
  json_object_put(jo);
}

void test_parse_app_config_from_7_21(void)
{
  RESET_TARGET_VERSION();
  set_target_version("7.21.0", "7.21.0");
  const char *json_app_config= "{\
    \"rfRegion\": 0,\
    \"txPower\": 138,\
    \"enablePTI\": 0,\
    \"maxTxPower\": 140,\
    \"measured0dBm\": 4,\
    \"nodeIdType\": 1,\
  }";
  json_object *jo = json_tokener_parse(json_app_config);
  SApplicationConfiguration_7_21_x ac= {
    .rfRegion = 0,
    .iTxPower = 138,
    .ipower0dbmMeasured = 4,
    .enablePTI = 0,
    .maxTxPower = 140,
    .nodeIdBaseType= 1
  };
  // 700 series, write to app region
  json_get_int_ExpectAndReturn(jo, "nodeIdType", 0, JSON_OPTIONAL, 1);
  json_get_int_ExpectAndReturn(jo, "rfRegion", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "txPower", 0, JSON_OPTIONAL, 138);
  json_get_int_ExpectAndReturn(jo, "measured0dBm", 0, JSON_OPTIONAL, 4);
  json_get_bool_ExpectAndReturn(jo, "enablePTI", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "maxTxPower", 140, JSON_OPTIONAL, 140);
  json_get_int_ExpectAndReturn(jo, "nodeIdType", 0, JSON_OPTIONAL, 1);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_app_handle, FILE_ID_APPLICATIONCONFIGURATION, &ac, sizeof(ac), nvm3_app_files, sizeof_array(nvm3_app_files), ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_app_config_json(jo, NVM3_700s));
  // 800 series, write to 1 single region 
  json_get_int_ExpectAndReturn(jo, "nodeIdType", 0, JSON_OPTIONAL, 1);
  json_get_int_ExpectAndReturn(jo, "rfRegion", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "txPower", 0, JSON_OPTIONAL, 138);
  json_get_int_ExpectAndReturn(jo, "measured0dBm", 0, JSON_OPTIONAL, 4);
  json_get_bool_ExpectAndReturn(jo, "enablePTI", 0, JSON_OPTIONAL, 0);
  json_get_int_ExpectAndReturn(jo, "maxTxPower", 140, JSON_OPTIONAL, 140);
  json_get_int_ExpectAndReturn(jo, "nodeIdType", 0, JSON_OPTIONAL, 1);
  nvm3_writeData_print_error_ExpectAndReturn(&nvm3_protocol_handle, FILE_ID_APPLICATIONCONFIGURATION, &ac, sizeof(ac), nvm3_current_protocol_files, nvm3_current_protocol_files_size, ECODE_NVM3_OK);
  TEST_ASSERT_TRUE(parse_app_config_json(jo, NVM3_800s_FROM_719));
  json_object_put(jo);
}

void test_file_version(void)
{
  TEST_ASSERT_EQUAL_HEX(0x01070B00, file_version(1, 7, 11, 0));
  TEST_ASSERT_EQUAL_HEX(0x05071304, file_version(5, 7, 19, 4));
}

void test_read_layout_from_nvm_size(void)
{
  TEST_ASSERT_EQUAL(NVM3_700s, read_layout_from_nvm_size(0));                 // Default case
  TEST_ASSERT_EQUAL(NVM3_800s_FROM_719, read_layout_from_nvm_size(0xA000));   // Default case
  TEST_ASSERT_EQUAL(NVM3_800s_PRIOR_719, read_layout_from_nvm_size(0x10000)); // Default case
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_read_layout_from_nvm_size);
  RUN_TEST(test_app_nvm_is_pre_v7_15_3);
  RUN_TEST(test_app_nvm_is_pre_v7_18_1);
  RUN_TEST(test_app_nvm_is_v7_19);
  RUN_TEST(test_app_nvm_is_v7_20);
  RUN_TEST(test_app_nvm_is_v7_21);
  RUN_TEST(test_app_nvm_is_v7_22);
  RUN_TEST(test_app_nvm_is_pre_v7_19);
  RUN_TEST(test_app_nvm_is_pre_v7_21);
  RUN_TEST(test_set_target_version);
  RUN_TEST(test_parse_app_config_json_prior_7_15_3);
  RUN_TEST(test_parse_app_config_prior_7_18_1);
  RUN_TEST(test_parse_app_config_pre_7_19);
  RUN_TEST(test_parse_app_config_pre_7_21);
  RUN_TEST(test_parse_app_config_from_7_21);
  RUN_TEST(test_parse_suc_state_json);
  RUN_TEST(test_parse_node_capability);
  RUN_TEST(test_parse_node_security);
  RUN_TEST(test_parse_route_cache_line_json);
  RUN_TEST(test_file_version);
  return UNITY_END();
}