#include <unity.h>
#include "user_message.h"

void setUp(void)
{
	// Initialize resources or mock 
}

void tearDown(void)
{
	// Clean up resources or mock 
}

void test_set_message_level(void)
{
	message_severity_t orig = set_message_level(MSG_TRACE);
	message_severity_t prev = set_message_level(MSG_WARNING);
	TEST_ASSERT_EQUAL(prev, MSG_TRACE);
	set_message_level(orig);
}

void test_get_message_level(void)
{
	set_message_level(MSG_ERROR);
	TEST_ASSERT_EQUAL(get_message_level(), MSG_ERROR);
}

void test_is_message_severity_enabled(void)
{
	set_message_level(MSG_WARNING);
	TEST_ASSERT_TRUE(is_message_severity_enabled(MSG_WARNING));
	TEST_ASSERT_TRUE(is_message_severity_enabled(MSG_ERROR));
	TEST_ASSERT_FALSE(is_message_severity_enabled(MSG_INFO));
	TEST_ASSERT_FALSE(is_message_severity_enabled(MSG_TRACE));
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_set_message_level);
	RUN_TEST(test_get_message_level);
	RUN_TEST(test_is_message_severity_enabled);
	return UNITY_END();
}