#include <unity.h>
#include <json.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "json_helpers.c"
#include "user_message_mock.h"

typedef uint8_t CLASSIC_NODE_MASK_TYPE[MAX_CLASSIC_NODEMASK_LENGTH];

void setUp(void)
{
	user_message_mock_Init();
}
void tearDown(void)
{
	user_message_mock_Verify();
}

void test_json_add_int(void)
{
	json_object *jo = json_object_new_object();
	json_add_int(jo, "int_key", 123);
	json_object *value = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "int_key", &value));
	TEST_ASSERT_EQUAL_INT(123, json_object_get_int(value));
	json_object_put(jo);
}

void test_json_add_bool(void)
{
	json_object *jo = json_object_new_object();
	json_add_bool(jo, "bool_key", true);
	json_object *value = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "bool_key", &value));
	TEST_ASSERT_TRUE(json_object_get_boolean(value));
	json_object_put(jo);
}

void test_json_add_string(void)
{
	json_object *jo = json_object_new_object();
	json_add_string(jo, "str_key", "test_value");
	json_object *value = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "str_key", &value));
	TEST_ASSERT_EQUAL_STRING("test_value", json_object_get_string(value));
	json_object_put(jo);
}

void test_json_add_nodemask(void)
{
	json_object *jo = json_object_new_object();
	uint8_t node_mask[ZW_CLASSIC_MAX_NODES / 8] = {0};
	ZW_NodeMaskSetBit(node_mask, 1);
	ZW_NodeMaskSetBit(node_mask, 8);
	ZW_NodeMaskSetBit(node_mask, 16);
	json_add_nodemask(jo, "mask", node_mask);
	json_object *value = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "mask", &value));
	TEST_ASSERT_EQUAL_INT(3, json_object_array_length(value));
	TEST_ASSERT_EQUAL_INT(1, json_object_get_int(json_object_array_get_idx(value, 0)));
	TEST_ASSERT_EQUAL_INT(8, json_object_get_int(json_object_array_get_idx(value, 1)));
	TEST_ASSERT_EQUAL_INT(16, json_object_get_int(json_object_array_get_idx(value, 2)));
	json_object_put(jo);
}

void test_json_add_byte_array(void)
{
	json_object *jo = json_object_new_object();
	uint8_t byte_array[] = {1, 2, 3, 4, 5};
	json_add_byte_array(jo, "byte_array", byte_array, sizeof(byte_array));
	json_object *value = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(jo, "byte_array", &value));
	TEST_ASSERT_EQUAL_INT(5, json_object_array_length(value));
	for (int i = 0; i < 5; i++)
	{
		TEST_ASSERT_EQUAL_INT(byte_array[i], json_object_get_int(json_object_array_get_idx(value, i)));
	}
	json_object_put(jo);
}

void test_home_id_to_json(void)
{
	uint8_t home_id[HOMEID_LENGTH] = {0x12, 0x34, 0x56, 0x78};
	json_object *jo = home_id_to_json(home_id);
	TEST_ASSERT_NOT_NULL(jo);
	TEST_ASSERT_EQUAL_STRING("0x12345678", json_object_get_string(jo));
	json_object_put(jo);
}

void test_version_to_json(void)
{
	int major = 7;
	int minor = 18;
	int patch = 8;
	uint32_t version = major << 16 | minor << 8 | patch;
	json_object *jo = version_to_json(version);
	TEST_ASSERT_EQUAL_STRING("7.18.8", json_object_get_string(jo));
	json_object_put(jo);
}

void test_nodemask_to_json(void)
{
	uint8_t node_mask[ZW_CLASSIC_MAX_NODES / 8] = {0};
	ZW_NodeMaskSetBit(node_mask, 1);
	ZW_NodeMaskSetBit(node_mask, 8);
	ZW_NodeMaskSetBit(node_mask, 16);
	json_object *jo = nodemask_to_json(node_mask);
	TEST_ASSERT_NOT_NULL(jo);
	TEST_ASSERT_EQUAL_INT(3, json_object_array_length(jo));
	TEST_ASSERT_EQUAL_INT(1, json_object_get_int(json_object_array_get_idx(jo, 0)));
	TEST_ASSERT_EQUAL_INT(8, json_object_get_int(json_object_array_get_idx(jo, 1)));
	TEST_ASSERT_EQUAL_INT(16, json_object_get_int(json_object_array_get_idx(jo, 2)));
	json_object_put(jo);
}

void test_register_root(void)
{
	json_object *jo = json_object_new_object();
	json_register_root(jo);
	// Check if the root is set correctly
	TEST_ASSERT_EQUAL_PTR(jo, json_root);
	json_register_root(NULL);
	TEST_ASSERT_NULL(json_root);
	json_object_put(jo);
}

void test_search_object(void)
{
	static json_path_item_t path_items[MAX_PATH_ITEM_COUNT];
	memset(path_items, 0, sizeof(path_items));
	json_object *jo = json_object_new_object();
	json_object *parent = json_object_new_object();
	json_object *child = json_object_new_string("child_string");
	json_object_object_add(parent, "child_string", child);
	json_object_object_add(jo, "parent", parent);
	TEST_ASSERT_FALSE(search_object(NULL, child, path_items, MAX_PATH_ITEM_COUNT, 0));
	TEST_ASSERT_FALSE(search_object(jo, NULL, path_items, MAX_PATH_ITEM_COUNT, 0));
	TEST_ASSERT_FALSE(search_object(jo, child, path_items, MAX_PATH_ITEM_COUNT, MAX_PATH_ITEM_COUNT + 1));
	TEST_ASSERT_TRUE(search_object(child, child, path_items, MAX_PATH_ITEM_COUNT, 0));
	TEST_ASSERT_TRUE(search_object(jo, child, path_items, MAX_PATH_ITEM_COUNT, 0));
	json_object *child_array = json_object_new_array();
	json_object *array = json_object_new_object();
	json_object_array_add(child_array, array);
	TEST_ASSERT_TRUE(search_object(child_array, array, path_items, MAX_PATH_ITEM_COUNT, 0));
	json_object_put(jo);
}

void test_json_get_object_path(void)
{
	json_object *jo = json_object_new_object();
	json_object *parent = json_object_new_object();
	json_object *child = json_object_new_object();
	json_object_object_add(parent, "child", child);
	json_object_object_add(jo, "parent", parent);
	user_message_Ignore();
	const char *root_not_set = json_get_object_path(child);
	TEST_ASSERT_EQUAL_STRING("(unknown path)", root_not_set);
	json_register_root(jo);
	// Check path for nested objects
	const char *path_child = json_get_object_path(child);
	TEST_ASSERT_EQUAL_STRING("/parent/child", path_child);
	const char *path_parent = json_get_object_path(parent);
	TEST_ASSERT_EQUAL_STRING("/parent", path_parent);
	json_object_put(jo);
	user_message_StopIgnore();
}

void test_json_get_object_error_check(void)
{
	json_object *jo = json_object_new_object();
	json_object_object_add(jo, "int_key", json_object_new_int(42));
	json_object_object_add(jo, "bool_key", json_object_new_boolean(true));
	json_object_object_add(jo, "str_key", json_object_new_string("hello"));
	// Check int value
	json_object *value = NULL;
	TEST_ASSERT_TRUE(json_get_object_error_check(jo, "int_key", &value, json_type_int, JSON_REQUIRED));
	TEST_ASSERT_EQUAL_INT(42, json_object_get_int(value));
	// Check parse error detection: wrong type for int_key
	user_message_Ignore();
	TEST_ASSERT_TRUE(json_get_object_error_check(jo, "int_key", &value, json_type_boolean, JSON_REQUIRED));
	TEST_ASSERT_TRUE(json_parse_error_detected());
	set_json_parse_error_flag(false);
	// Check parse error detection: wrong type for int_key
	TEST_ASSERT_TRUE(json_get_object_error_check(jo, "int_key", &value, json_type_boolean, JSON_OPTIONAL));
	TEST_ASSERT_TRUE(json_parse_error_detected());
	set_json_parse_error_flag(false);
	// Check parse error detection: object not found but is not required
	TEST_ASSERT_TRUE(json_get_object_error_check(jo, "int_key", &value, json_type_boolean, JSON_OPTIONAL));
	TEST_ASSERT_FALSE(json_get_object_error_check(jo, "missing_key", &value, json_type_int, JSON_REQUIRED));
	TEST_ASSERT_TRUE(json_parse_error_detected());
	set_json_parse_error_flag(false);
	// Check parse error detection: object not found and is required
	TEST_ASSERT_FALSE(json_get_object_error_check(jo, "missing_key", &value, json_type_boolean, JSON_OPTIONAL));
	TEST_ASSERT_FALSE(json_parse_error_detected());
	set_json_parse_error_flag(false);
	// Check bool value
	TEST_ASSERT_TRUE(json_get_object_error_check(jo, "bool_key", &value, json_type_boolean, JSON_REQUIRED));
	TEST_ASSERT_TRUE(json_object_get_boolean(value));
	// Check string value
	TEST_ASSERT_TRUE(json_get_object_error_check(jo, "str_key", &value, json_type_string, JSON_REQUIRED));
	TEST_ASSERT_EQUAL_STRING("hello", json_object_get_string(value));
	user_message_StopIgnore();
	json_object_put(jo);
}

void test_json_get_int(void)
{
	json_object *jo = json_object_new_object();
	json_object_object_add(jo, "int_key", json_object_new_int(42));
	// Key exists
	int32_t result = json_get_int(jo, "int_key", 0, JSON_REQUIRED);
	TEST_ASSERT_EQUAL(42, result);
	// Key does not exist, should return default value
	user_message_Ignore();
	result = json_get_int(jo, "int_key_not_exist", 0, JSON_REQUIRED);
	TEST_ASSERT_EQUAL(0, result);
	json_object_put(jo);
	user_message_StopIgnore();
}

void test_json_get_string(void)
{
	json_object *jo = json_object_new_object();
	json_object_object_add(jo, "str_key", json_object_new_string("hello"));
	// Key exists
	const char *result = json_get_string(jo, "str_key", "default_string", JSON_REQUIRED);
	TEST_ASSERT_EQUAL_STRING("hello", result);
	// Key does not exist, should return default value
	user_message_Ignore();
	const char *result_2 = json_get_string(jo, "str_key_not_exist", "default_string", JSON_REQUIRED);
	TEST_ASSERT_EQUAL_STRING("default_string", result_2);
	user_message_StopIgnore();
	json_object_put(jo);
}

void test_json_get_object(void)
{
	json_object *jo = json_object_new_object();
	json_object *child = json_object_new_string("hello");
	json_object_object_add(jo, "child", child);
	// Key exists
	json_object *result = json_get_object(jo, "child", NULL, JSON_REQUIRED);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_PTR(child, result);
	// Key does not exist, should return NULL
	user_message_Ignore();
	result = json_get_object(jo, "child_not_exist", NULL, JSON_REQUIRED);
	TEST_ASSERT_EQUAL_STRING(NULL, result);
	user_message_StopIgnore();
	json_object_put(jo);
}

void test_json_get_bool(void)
{
	json_object *jo = json_object_new_object();
	json_object_object_add(jo, "bool_key", json_object_new_boolean(true));
	// Key exists
	bool result = json_get_bool(jo, "bool_key", false, JSON_REQUIRED);
	TEST_ASSERT_TRUE(result);
	user_message_Ignore();
	// Key does not exist, should return default value
	result = json_get_bool(jo, "bool_key_not_exist", false, JSON_REQUIRED);
	TEST_ASSERT_FALSE(result);
	user_message_StopIgnore();
	json_object_put(jo);
}

void test_json_get_bytearray(void)
{
	json_object *jo = json_object_new_object();
	json_object *arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_int(1));
	json_object_array_add(arr, json_object_new_int(2));
	json_object_array_add(arr, json_object_new_int(3));
	json_object_object_add(jo, "arr", arr);
	uint8_t buf[3] = {0};
	// Check for returned array length and values
	uint8_t arr_len = json_get_bytearray(jo, "arr", buf, 3, false);
	TEST_ASSERT_EQUAL_UINT8(3, arr_len);
	TEST_ASSERT_EQUAL_INT(1, buf[0]);
	TEST_ASSERT_EQUAL_INT(2, buf[1]);
	TEST_ASSERT_EQUAL_INT(3, buf[2]);
	// Check non-existing array, should return len = 0
	user_message_Ignore();
	arr_len = json_get_bytearray(jo, "arr_not_existed", buf, 3, JSON_REQUIRED);
	TEST_ASSERT_EQUAL_UINT8(0, arr_len);
	user_message_StopIgnore();
	json_object_put(jo);
}

void test_json_get_intarray(void)
{
	json_object *jo = json_object_new_object();
	json_object *arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_int(10));
	json_object_array_add(arr, json_object_new_int(20));
	json_object_array_add(arr, json_object_new_int(30));
	json_object_object_add(jo, "arr", arr);
	int buf[3] = {0};
	// Check for returned array length and values
	uint8_t arr_len = json_get_intarray(jo, "arr", buf, 3, JSON_REQUIRED);
	TEST_ASSERT_EQUAL_UINT8(3, arr_len);
	TEST_ASSERT_EQUAL_INT(10, buf[0]);
	TEST_ASSERT_EQUAL_INT(20, buf[1]);
	TEST_ASSERT_EQUAL_INT(30, buf[2]);
	// Check non-existing array, should return len = 0
	user_message_Ignore();
	arr_len = json_get_intarray(jo, "arr_not_existed", buf, 3, JSON_REQUIRED);
	TEST_ASSERT_EQUAL_UINT8(0, arr_len);
	user_message_StopIgnore();
	json_object_put(jo);
}

void test_json_get_nodemask(void)
{
	json_object *jo = json_object_new_object();
	json_object *arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_int(1));
	json_object_array_add(arr, json_object_new_int(8));
	json_object_array_add(arr, json_object_new_int(16));
	json_object_object_add(jo, "mask", arr);
	CLASSIC_NODE_MASK_TYPE mask = {0};
	TEST_ASSERT_TRUE(json_get_nodemask(jo, "mask", mask, JSON_REQUIRED));
	TEST_ASSERT_EQUAL_HEX8(0x81, mask[0]); // bits 0 and 7 set
	TEST_ASSERT_EQUAL_HEX8(0x80, mask[1]); // bit 7 set
	memset(mask, 0, sizeof(CLASSIC_NODE_MASK_TYPE));
	user_message_Ignore();
	TEST_ASSERT_FALSE(json_get_nodemask(jo, "mask_non_existed", mask, JSON_REQUIRED));
	user_message_StopIgnore();
	json_object_put(jo);
}

void test_json_get_home_id(void)
{
	json_object *jo = json_object_new_object();
	json_object_object_add(jo, "HomeId", json_object_new_string("0x12345678"));
	uint8_t buf[4] = {0};
	uint32_t home_id = json_get_home_id(jo, "HomeId", buf, 0, JSON_REQUIRED);
	TEST_ASSERT_EQUAL_HEX32(0x12345678, home_id);
	// Check big-endian storage
	TEST_ASSERT_EQUAL_HEX8(0x12, buf[0]);
	TEST_ASSERT_EQUAL_HEX8(0x34, buf[1]);
	TEST_ASSERT_EQUAL_HEX8(0x56, buf[2]);
	TEST_ASSERT_EQUAL_HEX8(0x78, buf[3]);
	json_object_put(jo);
}

void test_json_object_get_from_path(void)
{
	json_object *jo = json_object_new_object();
	json_object *parent = json_object_new_object();
	json_object *child_string = json_object_new_string("child_string");
	json_object *child_int = json_object_new_int(420);
	json_object *child_bool = json_object_new_boolean(true);
	json_object *child_array = json_object_new_array();
	json_object_array_add(child_array, json_object_new_int(10));
	json_object_array_add(child_array, json_object_new_int(20));
	json_object_array_add(child_array, json_object_new_int(30));
	json_object_object_add(parent, "child_string", child_string);
	json_object_object_add(parent, "child_int", child_int);
	json_object_object_add(parent, "child_bool", child_bool);
	json_object_object_add(parent, "child_array", child_array);
	json_object_object_add(jo, "parent", parent);
	// Get int child object from path
	json_object *result = json_object_get_from_path(jo, "parent.child_int");
	TEST_ASSERT_EQUAL(420, json_object_get_int(result));
	result = json_object_get_from_path(jo, "parent.child_string");
	TEST_ASSERT_EQUAL_STRING("child_string", json_object_get_string(result));
	result = json_object_get_from_path(jo, "parent.child_bool");
	TEST_ASSERT_TRUE(json_object_get_boolean(result));
	result = json_object_get_from_path(jo, "parent.child_array[0]");
	TEST_ASSERT_EQUAL_INT(10, json_object_get_int(result));
	TEST_ASSERT_NULL(json_object_get_from_path(jo, "parent.child_array[4]"));
	TEST_ASSERT_NULL(json_object_get_from_path(jo, "parent.child_string[0]"));
	TEST_ASSERT_NULL(json_object_get_from_path(jo, "parent.child_array_not_existed[0]"));
	json_object_put(jo);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_json_add_int);
	RUN_TEST(test_json_add_bool);
	RUN_TEST(test_json_add_string);
	RUN_TEST(test_json_add_nodemask);
	RUN_TEST(test_json_add_byte_array);
	RUN_TEST(test_home_id_to_json);
	RUN_TEST(test_version_to_json);
	RUN_TEST(test_nodemask_to_json);
	RUN_TEST(test_register_root);
	RUN_TEST(test_search_object);
	RUN_TEST(test_json_get_object_path);
	RUN_TEST(test_json_get_object_error_check);
	RUN_TEST(test_json_get_object);
	RUN_TEST(test_json_get_int);
	RUN_TEST(test_json_get_string);
	RUN_TEST(test_json_get_bool);
	RUN_TEST(test_json_get_bytearray);
	RUN_TEST(test_json_get_intarray);
	RUN_TEST(test_json_get_nodemask);
	RUN_TEST(test_json_get_home_id);
	RUN_TEST(test_json_object_get_from_path);
	return UNITY_END();
}