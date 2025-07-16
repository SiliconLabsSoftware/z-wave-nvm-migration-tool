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

// void test_check_controller_nvm(void)
// {
//   /* This function should check the controller NVM and return true if it is valid */
//   size_t nvm_image_size_tc1 = 0xC000;
//   uint8_t nvm_image_tc1[nvm_image_size_tc1];
//   memset(nvm_image_tc1, 0xAB, nvm_image_size_tc1); // Fill with dummy data
//   user_message_Ignore();
//   uint32_t prot_version_le = htole32(0x00071200); // Set protocol version to 7.18.0
//   nvm3_readData_print_error_ExpectAndReturn(&nvm3_protocol_handle, FILE_ID_ZW_VERSION, &prot_version_le, sizeof(prot_version_le), nvm3_current_protocol_files, (uint32_t)nvm3_current_protocol_files, ECODE_NVM3_OK);
//   nvm3_readData_print_error_IgnoreArg_value();
//   nvm3_readData_print_error_ReturnThruPtr_value(&prot_version_le);
//   check_controller_nvm(nvm_image_tc1, nvm_image_size_tc1, NVM3_700s);
//   TEST_ASSERT_EQUAL(ZW_700s, hardware_info);
//   user_message_StopIgnore();
// }

// void test_open_controler_nvm(void)
// {
// /* Open controller nvm image based on nvm layout */
// // 700 Series
// size_t nvm_image_size_tc1 = 0xC000;
// uint8_t nvm_image_tc1[nvm_image_size_tc1];
// memset(nvm_image_tc1, 0xAB, nvm_image_size_tc1); // Fill with dummy data
// user_message_Ignore();
// nvm3_halOpen_ExpectAnyArgsAndReturns(ECODE_NVM3_OK);
// nvm3_halRamSetBin_ExpectAndReturns(nvm_image_tc1, nvm_image_size_tc1);
// nvm3_halSetPageSize_ExpectAnyArgsAndReturns(ECODE_NVM3_OK);
// open_controller_nvm(nvm_image_tc1, nvm_image_size_tc1, NVM3_700s);
// TEST_ASSERT_EQUAL(PROTOCOL_NVM_SIZE_700s, nvm3_protocol_handle.nvmSize);
// TEST_ASSERT_EQUAL(APP_NVM_SIZE_700s, nvm3_app_handle.nvmSize);
// // 800s prior 7.19
// size_t nvm_image_size_tc2 = 0x10000;
// uint8_t nvm_image_tc2[nvm_image_size_tc2];
// memset(nvm_image_tc2, 0xAB, nvm_image_size_tc2); // Fill with dummy data
// user_message_Ignore();
// // nvm3_halOpen_ExpectAnyArgsAndReturns(ECODE_NVM3_OK);
// // nvm3_halRamSetBin_ExpectAndReturns(nvm_image_tc2, nvm_image_size_tc2);
// open_controller_nvm(nvm_image_tc2, nvm_image_size_tc2, NVM3_800s_PRIOR_719);
// TEST_ASSERT_EQUAL(PROTOCOL_NVM_SIZE_800s_PRIOR_719, nvm3_protocol_handle.nvmSize);
// TEST_ASSERT_EQUAL(APP_NVM_SIZE_800s_PRIOR_719, nvm3_app_handle.nvmSize);
// // 800s from 7.19
// size_t nvm_image_size_tc3 = 0xA000;
// uint8_t nvm_image_tc3[nvm_image_size_tc3];
// memset(nvm_image_tc3, 0xAB, nvm_image_size_tc3); // Fill with dummy data
// user_message_Ignore();
// // nvm3_halOpen_ExpectAnyArgsAndReturns(ECODE_NVM3_OK);
// // nvm3_halRamSetBin_ExpectAndReturns(nvm_image_tc3, nvm_image_size_tc3);
// open_controller_nvm(nvm_image_tc3, nvm_image_size_tc3, NVM3_800s_PRIOR_719);
// TEST_ASSERT_NULL(&nvm3_app_handle);
// TEST_ASSERT_EQUAL(NVM_SIZE_800s_FROM_719, nvm3_protocol_handle.nvmSize);
// }

void test_get_controller_nvm_image(void)
{
  /* Get the controller NVM image */

  /* Check whether the image is retrieved successfully and the size is correct */
}

void test_close_controller_nvm(void)
{
  /* Close the controller NVM */

  /* Check whether the NVM is closed successfully */
}

// void test_dump_controller_nvm_keys(void)
// {
//   /* Dump the controller NVM */
//   nvm3_app_handle.hasBeenOpened = true;
//   nvm3_protocol_handle.hasBeenOpened = true;
//   set_target_version("7.15.1", "7.15.1");
//   nvm3_dump_keys_with_filename_ExpectAnyArgs();
//   dump_controller_nvm_keys();
//   set_target_version("7.18.0", "7.18.0");
//   nvm3_dump_keys_with_filename_ExpectAnyArgs();
//   dump_controller_nvm_keys();
//   set_target_version("7.19.0", "7.19.0");
//   nvm3_dump_keys_with_filename_ExpectAnyArgs();
//   dump_controller_nvm_keys();
// }

// void test_backup_info(void)
// {
//   uint32_t mock_protocol_version_718 = htole32(0x00071200); // Set protocol version to 7.18.0
//   uint32_t mock_app_version_718 = htole32(0x00071200); // Set app version to 7.18.0
//   nvm3_readData_print_error_ExpectAnyArgs();
//   nvm3_readData_print_error_ReturnThruPtr_h(&mock_protocol_version_718);
//   nvm3_readData_print_error_ExpectAnyArgs();
//   nvm3_readData_print_error_ReturnThruPtr_h(&mock_protocol_version_718);
//   backup_info(NVM3_700s);
//   TEST_ASSERT_EQUAL(0x0071200, target_app_version);
//   TEST_ASSERT_EQUAL(0x0071200, target_protocol_version);
//   hardware_info = EFR32XG28;
//   nvm3_readData_print_error_ExpectAnyArgs();
//   nvm3_readData_print_error_ReturnThruPtr_h(&mock_protocol_version_718);
//   nvm3_readData_print_error_ExpectAnyArgs();
//   nvm3_readData_print_error_ReturnThruPtr_h(&mock_protocol_version_718);
//   backup_info(NVM3_800s_FROM_719);
//   TEST_ASSERT_EQUAL(0x0071200, target_app_version);
//   TEST_ASSERT_EQUAL(0x0071200, target_protocol_version);
//   hardware_info = EFR32XG23;
//   uint32_t mock_protocol_version_719 = htole32(0x00071300); // Set protocol version to 7.18.0
//   uint32_t mock_app_version_719 = htole32(0x00071300); // Set app version to 7.18.0
//   nvm3_readData_print_error_ExpectAnyArgs();
//   nvm3_readData_print_error_ReturnThruPtr_h(&mock_protocol_version_719);
//   nvm3_readData_print_error_ExpectAnyArgs();
//   nvm3_readData_print_error_ReturnThruPtr_h(&mock_protocol_version_719);
//   backup_info(NVM3_800s_PRIOR_719);
//   TEST_ASSERT_EQUAL(0x0071300, target_app_version);
//   TEST_ASSERT_EQUAL(0x0071300, target_protocol_version);
// }

// void test_cmd_classes_to_json(void)
// {
//   /* Convert command classes to JSON format */
//   const char *expected_json_str = "{\
//   \"cmdClasses\":{\
//     \"includedInsecurely\":[\
//       94,\
//       85,\
//       86,\
//       34,\
//       159,\
//       152,\
//       116,\
//       108,\
//       138\
//     ],\
//     \"includedSecurelyInsecureCCs\":[],\
//     \"includedSecurelySecureCCs\":[]\
//   }\
// }";
//   json_object *expected_jo = json_tokener_parse(expected_json_str);
//   SApplicationCmdClassInfo ai = {9, {94, 85, 86, 34, 159, 152, 116, 108, 138}, 4, {1, 2, 3, 4}, 4, {5, 6, 7, 8}};
//   json_object *jo = cmd_classes_to_json(ai);
//   /* TODO: Check value*/
//   TEST_ASSERT_EQUAL();
// }

// void test_app_config_to_json(void)
// {
//   SApplicationConfiguration_prior_7_15_3 ac_7_15_3 = {};
//   nvm3_readData_print_error_ExpectAnyArgsAndReturns(ECODE_NVM3_OK);
//   nvm3_readData_print_error_ReturnThruPtr_h();
//   app_config_to_json(NVM3_700s);
//   SApplicationConfiguration_prior_7_18_1 ac_prior_7_18_1 = {};

//   app_config_to_json(NVM3_800s_FROM_719);
//   SApplicationConfiguration_7_18_1 ac_7_18_1 = {};

//   app_config_to_json(NVM3_800s_PRIOR_719);
//   SApplicationConfiguration_7_21_x ac_7_21 = {};
//   app_config_to_json(NVM3_800s_PRIOR_719);
// }

void test_suc_node_list_to_json(void)
{
  /* Convert SUC node list to JSON format */

  /* Check whether the conversion is successful and the output is correct */
}

void test_suc_node_list_v5_to_json(void)
{
  /* Convert SUC node list v5 to JSON format */

  /* Check whether the conversion is successful and the output is correct */
}

// void test_route_cache_line_to_json(void)
// {
//   ROUTECACHE_LINE *rcl_tc1 = {0x00 | 0x03, {2,3,4,5}};
//   ROUTECACHE_LINE *rcl_tc2 = {0x20 | 0x03, {2,3,4,5}};
//   ROUTECACHE_LINE *rcl_tc3 = {0x40 | 0x03, {2,3,4,5}};
//   json_object *beaming = NULL;
//   json_object *protocol_rate = NULL;
//   json_object *repeater_list = NULL;

//   // routecacheLineConfSize = 0x00, has no beam capability
//   json_object *jo = route_cache_line_to_json(rcl);
//   TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "beaming", &value));
//   TEST_ASSERT_FALSE(json_object_get_boolean(value))
//   TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "protocolRate", &value));
//   TEST_ASSERT_EQUAL_INT(json_object_get_boolean(value))

//   // routecacheLineConfSize = 0x20, 250ms beaming capability
//   jo = route_cache_line_to_json(rcl);
//   value = NULL;
//   protocol_rate = NULL;
//   TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "beaming", &value));
//   TEST_ASSERT_EQUAL_STRING("250ms", json_object_get_string(value))
//   TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "protocolRate", &value));
//   TEST_ASSERT_EQUAL_INT(json_object_get_boolean(value))

//   // routecacheLineConfSize = 0x40, 1000ms beaming capability
//   jo = route_cache_line_to_json(rcl);
//   value = NULL;
//   protocol_rate = NULL;
//   TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "beaming", &value));
//   TEST_ASSERT_EQUAL_STRING("1000ms", json_object_get_string(value))
//   TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "protocolRate", &value));
//   TEST_ASSERT_EQUAL_INT(json_object_get_boolean(value));
// }

void test_node_table_to_json(void)
{
  /* Convert node table to JSON format */

  /* Check whether the conversion is successful and the output is correct */
}

void test_lr_node_table_to_json(void)
{
  /* Convert node table to JSON format */

  /* Check whether the conversion is successful and the output is correct */
}

void test_nvm711_controller_into_to_json(void)
{
  /* Convert controller info to JSON format for NVM711 */

  /* Check whether the conversion is successful and the output is correct */
}

// nvm715_controller_info_to_json
// nvm_from_719_controller_info_to_json
// controller_info_nvm_get_json
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
// create_nvm3_version_files
// is_json_file_supported
// parse_controller_nvm711_json
// parse_controller_nvm715_json
// parse_controller_nvm719_json
// json_get_nvm_layout
// controller_parse_json

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
  // RUN_TEST(test_check_controller_nvm);
  // RUN_TEST(test_open_controller_nvm);
  // RUN_TEST(test_dump_controller_nvm_keys);
  // RUN_TEST(test_backup_info);
  RUN_TEST(test_parse_app_config_json_prior_7_15_3);
  RUN_TEST(test_parse_app_config_prior_7_18_1);
  RUN_TEST(test_parse_app_config_pre_7_19);
  RUN_TEST(test_parse_app_config_pre_7_21);
  RUN_TEST(test_parse_app_config_from_7_21);
  // RUN_TEST(test_suc_node_list_to_json);
  // RUN_TEST(test_suc_node_list_v5_to_json);
  RUN_TEST(test_parse_suc_state_json);
  // RUN_TEST(test_parse_suc_state_json_from_nvm719);
  RUN_TEST(test_parse_node_capability);
  RUN_TEST(test_parse_node_security);
  RUN_TEST(test_parse_route_cache_line_json);
  RUN_TEST(test_file_version);
  return UNITY_END();
}