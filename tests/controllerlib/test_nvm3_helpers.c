#include <unity.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "nvm3_helpers.c"
#include "nvm3_stubs_mock.h"
#include "user_message_mock.h"

nvm3_file_descriptor_t nvm3_protocol_files[] = {
    {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"},
    {.key = FILE_ID_NODEROUTECAHE, .size = FILE_SIZE_NODEROUTECAHE, .name = "NODEROUTECAHE", .optional = true, .num_keys = ZW_CLASSIC_MAX_NODES},
};
uint32_t nvm3_protocol_files_size = sizeof(nvm3_protocol_files) / sizeof(nvm3_file_descriptor_t);

int nvm3_check_files_case = 0; // Control mock return values for nvm3_check_files
// Mock nvm3_getObjectInfo function to control return values
static Ecode_t nvm3_getObjectInfo_Callback(__attribute__((unused)) nvm3_Handle_t *h, __attribute__((unused)) nvm3_ObjectKey_t key, uint32_t *type, size_t *size, __attribute__((unused)) int cmock_num_calls)
{
  if (nvm3_check_files_case == 0)
  {
    *type = NVM3_OBJECTTYPE_DATA;
    *size = FILE_SIZE_ZW_VERSION;
    return ECODE_NVM3_OK;
  }
  else if (nvm3_check_files_case == 1)
  {
    return ECODE_NVM3_ERR_KEY_NOT_FOUND;
  }
  else if (nvm3_check_files_case == 2)
  {
    *type = 0x01;
    *size = FILE_SIZE_ZW_VERSION;
    return ECODE_NVM3_OK;
  }
  else if (nvm3_check_files_case == 3)
  {
    *type = NVM3_OBJECTTYPE_DATA;
    *size = FILE_SIZE_ZW_VERSION + 1;
    return ECODE_NVM3_OK;
  }
  else if (nvm3_check_files_case == 4)
  {
    *type = NVM3_OBJECTTYPE_DATA;
    *size = FILE_SIZE_APPLICATIONCONFIGURATION_7_18_1 + 1;
    return ECODE_NVM3_OK;
  }
  return ECODE_NVM3_ERR_KEY_NOT_FOUND;
}

void setUp(void)
{
  nvm3_stubs_mock_Init();
  user_message_mock_Init();
  reset_nvm3_error_status();
}

void tearDown(void)
{
  nvm3_stubs_mock_Verify();
  user_message_mock_Verify();
  reset_nvm3_error_status();
}

void test_reset_and_detect_error(void)
{
  nvm3_error_detected = true;
  reset_nvm3_error_status();
  TEST_ASSERT_FALSE(was_nvm3_error_detected());
}

void test_nvm3_check_files(void)
{
  nvm3_Handle_t h;
  user_message_Ignore();
  nvm3_getObjectInfo_StubWithCallback(nvm3_getObjectInfo_Callback);
  nvm3_file_descriptor_t nvm_file[] = {
      {.key = FILE_ID_ZW_VERSION, .size = FILE_SIZE_ZW_VERSION, .name = "ZW_VERSION"}};
  size_t nvm_file_size = sizeof(nvm_file) / sizeof(nvm3_file_descriptor_t);
  // Case 1: All files OK
  nvm3_check_files_case = 0;
  TEST_ASSERT_TRUE(nvm3_check_files(&h, nvm_file, nvm_file_size));
  // Case 2: File not found
  nvm3_check_files_case = 1;
  TEST_ASSERT_FALSE(nvm3_check_files(&h, nvm_file, nvm_file_size));
  // Case 3: File exists but not of type DATA
  nvm3_check_files_case = 2;
  TEST_ASSERT_FALSE(nvm3_check_files(&h, nvm_file, nvm_file_size));
  // Case 4: File exists but size does not match
  nvm3_check_files_case = 3;
  TEST_ASSERT_FALSE(nvm3_check_files(&h, nvm_file, nvm_file_size));
  // Case 5: File exists but size does not match for FILE_ID_APPLICATIONCONFIGURATION
  nvm3_check_files_case = 4;
  nvm3_file_descriptor_t nvm_app_file[] = {
      {.key = FILE_ID_APPLICATIONCONFIGURATION, .size = FILE_SIZE_APPLICATIONCONFIGURATION_7_18_1, .name = "APP_CONFIG"}};
  TEST_ASSERT_TRUE(nvm3_check_files(&h, nvm_app_file, sizeof(nvm_app_file) / sizeof(nvm_file)));
  user_message_StopIgnore();
}

void test_lookup_filename(void)
{
  TEST_ASSERT_EQUAL_STRING("ZW_VERSION", lookup_filename(FILE_ID_ZW_VERSION, nvm3_protocol_files, nvm3_protocol_files_size));
  TEST_ASSERT_EQUAL_STRING("NODEROUTECAHE (node_id: 1)", lookup_filename(FILE_ID_NODEROUTECAHE, nvm3_protocol_files, nvm3_protocol_files_size));
  int FILE_ID_NOT_EXIST = 0x123456; // Non-existing file ID
  TEST_ASSERT_EQUAL_STRING("\0", lookup_filename(FILE_ID_NOT_EXIST, nvm3_protocol_files, nvm3_protocol_files_size));
}

void test_nvm3_dump_keys_with_filename(void)
{
  nvm3_Handle_t h;
  nvm3_enumObjects_ExpectAnyArgsAndReturn(0);
  nvm3_dump_keys_with_filename(&h, nvm3_protocol_files, nvm3_protocol_files_size);
  nvm3_enumObjects_ExpectAnyArgsAndReturn(nvm3_protocol_files_size);
  nvm3_dump_keys_with_filename(&h, nvm3_protocol_files, nvm3_protocol_files_size);
}

void test_key_is_SUC_NODE_LIST_V5(void)
{
  TEST_ASSERT_TRUE(key_is_SUC_NODE_LIST_V5(FILE_ID_SUCNODELIST_BASE_V5));
  TEST_ASSERT_FALSE(key_is_SUC_NODE_LIST_V5(FILE_ID_APPLICATIONDATA));
  TEST_ASSERT_FALSE(key_is_SUC_NODE_LIST_V5(0x56000));
}

void test_key_is_NOUDEROUTECACHE_V1(void)
{
  TEST_ASSERT_TRUE(key_is_NOUDEROUTECACHE_V1(FILE_ID_NODEROUTECAHE_V1));
  TEST_ASSERT_FALSE(key_is_NOUDEROUTECACHE_V1(FILE_ID_APPLICATIONDATA));
  TEST_ASSERT_FALSE(key_is_NOUDEROUTECACHE_V1(0x56000));
}

void test_nvm3_readData_print_error(void)
{
  nvm3_Handle_t h;
  char buf[10];
  // Read returns ECODE_NVM3_OK
  user_message_Ignore();
  is_message_severity_enabled_IgnoreAndReturn(true);
  nvm3_readData_ExpectAnyArgsAndReturn(ECODE_NVM3_OK);
  TEST_ASSERT_EQUAL(ECODE_NVM3_OK, nvm3_readData_print_error(&h, FILE_ID_ZW_VERSION, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  // Read returns ECODE_NVM3_ERR_KEY_NOT_FOUND for FILE_ID_PROPRIETARY_1
  nvm3_readData_ExpectAnyArgsAndReturn(ECODE_NVM3_ERR_KEY_NOT_FOUND);
  TEST_ASSERT_EQUAL(ECODE_NVM3_ERR_KEY_NOT_FOUND, nvm3_readData_print_error(&h, FILE_ID_PROPRIETARY_1, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  // Read returns any code that's not ECODE_NVM3_OK for FILE_ID_PROPRIETARY_1
  nvm3_readData_ExpectAnyArgsAndReturn(ECODE_NVM3_ERR_READ_DATA_SIZE);
  TEST_ASSERT_EQUAL(ECODE_NVM3_ERR_READ_DATA_SIZE, nvm3_readData_print_error(&h, FILE_ID_PROPRIETARY_1, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  TEST_ASSERT_TRUE(was_nvm3_error_detected());
  reset_nvm3_error_status();
  // Read returns ECODE_NVM3_ERR_KEY_NOT_FOUND for FILE_ID_NODEROUTECAHE_V1
  nvm3_readData_ExpectAnyArgsAndReturn(ECODE_NVM3_ERR_KEY_NOT_FOUND);
  TEST_ASSERT_EQUAL(ECODE_NVM3_ERR_KEY_NOT_FOUND, nvm3_readData_print_error(&h, FILE_ID_NODEROUTECAHE_V1, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  // Read
  nvm3_readData_ExpectAnyArgsAndReturn(ECODE_NVM3_ERR_READ_DATA_SIZE);
  TEST_ASSERT_EQUAL(ECODE_NVM3_ERR_READ_DATA_SIZE, nvm3_readData_print_error(&h, FILE_ID_NODEROUTECAHE_V1, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  TEST_ASSERT_TRUE(was_nvm3_error_detected());
  reset_nvm3_error_status();
  user_message_StopIgnore();
  is_message_severity_enabled_StopIgnore();
}

void test_nvm3_writeData_print_error(void)
{
  nvm3_Handle_t h;
  char buf[10] = {0};
  user_message_Ignore();
  is_message_severity_enabled_IgnoreAndReturn(true);
  nvm3_writeData_ExpectAnyArgsAndReturn(ECODE_NVM3_OK);
  TEST_ASSERT_EQUAL(ECODE_NVM3_OK, nvm3_writeData_print_error(&h, FILE_ID_ZW_VERSION, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  // Write returns anything other than ECODE_NVM3_OK
  nvm3_writeData_ExpectAnyArgsAndReturn(ECODE_NVM3_ERR_KEY_NOT_FOUND);
  TEST_ASSERT_EQUAL(ECODE_NVM3_ERR_KEY_NOT_FOUND, nvm3_writeData_print_error(&h, FILE_ID_ZW_VERSION, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  TEST_ASSERT_TRUE(was_nvm3_error_detected());
  reset_nvm3_error_status();
  // Print user message with file name
  nvm3_writeData_ExpectAnyArgsAndReturn(ECODE_NVM3_ERR_KEY_NOT_FOUND);
  TEST_ASSERT_EQUAL(ECODE_NVM3_ERR_KEY_NOT_FOUND, nvm3_writeData_print_error(&h, FILE_ID_ZW_VERSION, buf, 10, nvm3_protocol_files, nvm3_protocol_files_size));
  TEST_ASSERT_TRUE(was_nvm3_error_detected());
  reset_nvm3_error_status();
  user_message_StopIgnore();
  is_message_severity_enabled_StopIgnore();
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_reset_and_detect_error);
  RUN_TEST(test_nvm3_check_files);
  RUN_TEST(test_lookup_filename);
  RUN_TEST(test_nvm3_dump_keys_with_filename);
  RUN_TEST(test_key_is_SUC_NODE_LIST_V5);
  RUN_TEST(test_key_is_NOUDEROUTECACHE_V1);
  RUN_TEST(test_nvm3_readData_print_error);
  RUN_TEST(test_nvm3_writeData_print_error);
  return UNITY_END();
}
