#include <unity.h>
#include <string.h>
#include <stdlib.h>
#include <json.h>
#include "json_upgrade.c"

void setUp(void)
{
	// Initialize resources or mocks 
}
void tearDown(void)
{
	// Clean up any resources or mocks
}

void test_compare_versions(void) {
    TEST_ASSERT_EQUAL(0, compare_versions("7.18.8", "7.18.8"));
    TEST_ASSERT_TRUE(compare_versions("7.19.0", "7.18.0") > 0);
    TEST_ASSERT_TRUE(compare_versions("7.19.0", "7.21.0") < 0);
    TEST_ASSERT_TRUE(compare_versions("7.18.0", "7.22.0") < 0);
    TEST_ASSERT_TRUE(compare_versions("7.21.4", "7.21.2") > 0);
    TEST_ASSERT_TRUE(compare_versions("7.0.0", "6.0.0") > 0);
}

void test_handle_version_based_addition_removal_add(void) {
    json_object *target = json_object_new_object();
    json_object *property_schema = json_object_new_object();
    // Add/Remove based on created field 
    json_object_object_add(property_schema, "created", json_object_new_int(4));
    json_object_object_add(property_schema, "default_value", json_object_new_int(0));
    handle_version_based_addition_removal(target, property_schema, "reservedIdLR", "7.19.0", 5);
    json_object *val = NULL;
    TEST_ASSERT_TRUE(json_object_object_get_ex(target, "reservedIdLR", &val));
    TEST_ASSERT_EQUAL_INT(0, json_object_get_int(val)); 
    handle_version_based_addition_removal(target, property_schema, "reservedIdLR", "7.19.0", 4);
    // Add/Remove based on createdVersion field
    json_object_object_add(property_schema, "createdVersion", json_object_new_string("7.19.0"));
    handle_version_based_addition_removal(target, property_schema, "reservedIdLR", "7.19.0", 4);
    TEST_ASSERT_TRUE(json_object_object_get_ex(target, "reservedIdLR", &val));
    TEST_ASSERT_EQUAL_INT(0, json_object_get_int(val)); 
    handle_version_based_addition_removal(target, property_schema, "reservedIdLR", "7.18.0", 4);
    val = NULL;
    target = NULL;
    TEST_ASSERT_FALSE(json_object_object_get_ex(target, "reservedIdLR", &val));
    TEST_ASSERT_NULL(val);
    json_object_put(target);
    json_object_put(property_schema);
}

void test_handle_version_based_addition_removal_remove(void) {
    json_object *target = json_object_new_object();
    json_object *property_schema = json_object_new_object();
    json_object_object_add(property_schema, "created", json_object_new_int(2));
    json_object_object_add(property_schema, "removed", json_object_new_int(4));
    json_object_object_add(property_schema, "default_value", json_object_new_int(140));
    json_object_object_add(target, "lrTxpower", json_object_new_int(138));
    json_object *val = NULL;
    handle_version_based_addition_removal(target, property_schema, "lrTxpower", "7.16.0", 3);
    TEST_ASSERT_TRUE(json_object_object_get_ex(target, "lrTxpower", &val));
    TEST_ASSERT_EQUAL(138, json_object_get_int(val));
    val = NULL;
    handle_version_based_addition_removal(target, property_schema, "lrTxpower", "7.19.0", 5);
    TEST_ASSERT_FALSE(json_object_object_get_ex(target, "lrTxpower", &val));
    TEST_ASSERT_NULL(val);
    json_object_put(target);
    json_object_put(property_schema);
}

void test_get_file_system_format_from_version(void) {
    TEST_ASSERT_EQUAL(0, get_file_system_format_from_version("7.11.0"));
    TEST_ASSERT_EQUAL(1, get_file_system_format_from_version("7.12.0"));
    TEST_ASSERT_EQUAL(2, get_file_system_format_from_version("7.15.0"));
    TEST_ASSERT_EQUAL(3, get_file_system_format_from_version("7.16.0"));
    TEST_ASSERT_EQUAL(4, get_file_system_format_from_version("7.17.0"));
    TEST_ASSERT_EQUAL(5, get_file_system_format_from_version("7.19.0"));
    TEST_ASSERT_EQUAL(5, get_file_system_format_from_version("7.20.0"));
    TEST_ASSERT_EQUAL(5, get_file_system_format_from_version("7.21.4"));
    TEST_ASSERT_EQUAL(5, get_file_system_format_from_version("7.23.2"));
}

void test_apply_schema_changes(void)
{
	const char *tc1=
			"{\n"
			"  \"testObject1\": 1,\n"
			"  \"testObject3\": true,\n"
			"  \"testObject4\": false\n"
			"}";
	const char *tc2=
			"{\n"
			"  \"testObject1\": 1,\n"
			"  \"testObject2\": \"serial_api_controller\",\n"
			"  \"testObject3\": true,\n"
			"  \"testObject4\": false\n"
			"}";

	const char *schemaString =
			"{\n"
			"  \"type\": \"object\",\n"
			"  \"properties\": {\n"
			"    \"testObject1\": {\n"
			"      \"type\": \"integer\",\n"
			"      \"created\": 0,\n"
			"      \"default_value\": 1,\n"
			"      \"removed\": 2\n"
			"    },\n"
			"    \"testObject2\": {\n"
			"      \"type\": \"string\",\n"
			"      \"createdVersion\": \"7.20.0\",\n"
			"      \"default_value\": \"serial_api_controller\"\n"
			"    },\n"
			"    \"testObject3\": {\n"
			"      \"type\": \"boolean\",\n"
			"      \"created\": 3,\n"
			"      \"default_value\": true,\n"
			"      \"removed\": 5\n"
			"    },\n"
			"    \"testObject4\": {\n"
			"      \"type\": \"boolean\",\n"
			"      \"created\": 1,\n"
			"      \"default_value\": false\n"
			"    }\n"
			"  },\n"
			"  \"required\": [\n"
			"    \"testObject1\",\n"
			"    \"testObject2\",\n"
			"    \"testObject3\",\n"
			"    \"testObject4\"\n"
			"  ]\n"
			"}";
	json_object *target = json_tokener_parse(tc1);
	json_object *schema = json_tokener_parse(schemaString);
	// # TC1
	apply_schema_changes(target, schema, "7.21.0", 5);
	json_object *val = NULL;
	TEST_ASSERT_FALSE(json_object_object_get_ex(target, "testObject1", &val));
	TEST_ASSERT_NULL(val);
	val = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(target, "testObject2", &val));
	TEST_ASSERT_EQUAL_STRING("serial_api_controller", json_object_get_string(val));
	val = NULL;
	TEST_ASSERT_FALSE(json_object_object_get_ex(target, "testObject3", &val));
	TEST_ASSERT_NULL(val);
	val = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(target, "testObject4", &val));
	TEST_ASSERT_FALSE(json_object_get_boolean(val));
	val = NULL;
	// # TC2
	target = json_tokener_parse(tc2);
	apply_schema_changes(target, schema, "7.19.0", 4);
	TEST_ASSERT_FALSE(json_object_object_get_ex(target, "testObject1", &val));
	TEST_ASSERT_NULL(val);
	val = NULL;
	TEST_ASSERT_FALSE(json_object_object_get_ex(target, "testObject2", &val));
	TEST_ASSERT_NULL(val);
	val = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(target, "testObject3", &val));
	TEST_ASSERT_TRUE(json_object_get_boolean(val));
	val = NULL;
	TEST_ASSERT_TRUE(json_object_object_get_ex(target, "testObject4", &val));
	TEST_ASSERT_FALSE(json_object_get_boolean(val));
	val = NULL;
	json_object_put(target);
	json_object_put(schema);
}

void test_rearrange_keys(void) {
	const char *tc =
			"{\n"
			"  \"testObject2\": \"serial_api_controller\",\n"
			"  \"testObject4\": false,\n"
			"  \"testObject1\": 1,\n"
			"  \"testObject3\": true,\n"
			"}";

	const char *schemaString =
			"{\n"
			"  \"type\": \"object\",\n"
			"  \"properties\": {\n"
			"    \"testObject1\": {\n"
			"      \"type\": \"integer\",\n"
			"      \"created\": 0,\n"
			"      \"default_value\": 1,\n"
			"      \"removed\": 2\n"
			"    },\n"
			"    \"testObject2\": {\n"
			"      \"type\": \"string\",\n"
			"      \"createdVersion\": \"7.20.0\",\n"
			"      \"default_value\": \"serial_api_controller\"\n"
			"    },\n"
			"    \"testObject3\": {\n"
			"      \"type\": \"boolean\",\n"
			"      \"created\": 3,\n"
			"      \"default_value\": true,\n"
			"      \"removed\": 5\n"
			"    },\n"
			"    \"testObject4\": {\n"
			"      \"type\": \"boolean\",\n"
			"      \"created\": 1,\n"
			"      \"default_value\": false\n"
			"    }\n"
			"  },\n"
			"  \"required\": [\n"
			"    \"testObject1\",\n"
			"    \"testObject2\",\n"
			"    \"testObject3\",\n"
			"    \"testObject4\"\n"
			"  ]\n"
			"}";
    json_object *target = json_tokener_parse(tc);
    json_object *schema = json_tokener_parse(schemaString);
    rearrange_keys(target, schema);
    // Check whether the keys are rearranged according to the schema
		const char *expected_order[] = {"testObject1", "testObject2", "testObject3", "testObject4"};
		int i = 0;
    json_object_object_foreach(target, key, target_value) {
			TEST_ASSERT_EQUAL_STRING(expected_order[i], key);
			i++;
    }
		// Check whether values remain unchanged
		TEST_ASSERT_EQUAL_INT(1, json_object_get_int(json_object_object_get(target, "testObject1")));
		TEST_ASSERT_EQUAL_STRING("serial_api_controller", json_object_get_string(json_object_object_get(target, "testObject2")));
		TEST_ASSERT_TRUE(json_object_get_boolean(json_object_object_get(target, "testObject3")));
		TEST_ASSERT_FALSE(json_object_get_boolean(json_object_object_get(target, "testObject4")));
		// Clean up
    json_object_put(target);
    json_object_put(schema);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_compare_versions);
    RUN_TEST(test_handle_version_based_addition_removal_add);
    RUN_TEST(test_handle_version_based_addition_removal_remove);
    RUN_TEST(test_get_file_system_format_from_version);
    RUN_TEST(test_apply_schema_changes);
    RUN_TEST(test_rearrange_keys);
    return UNITY_END();
}
