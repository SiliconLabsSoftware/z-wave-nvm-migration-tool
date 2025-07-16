#include <unity.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "base64.h"

void setUp(void)
{
  // Initialize resources or mock
}

void tearDown(void)
{
  // Clean up resources or mock
} 

void test_base64_encode(void)
{
  const uint8_t input_no_padding[] = "Hello, Zorld";
  char *encoded = base64_encode(input_no_padding, sizeof(input_no_padding) - 1, 0);
  TEST_ASSERT_NOT_NULL(encoded);
  TEST_ASSERT_EQUAL_STRING("SGVsbG8sIFpvcmxk", encoded);
  const uint8_t input_2_bytes_padding[] = "Hello, Zorld!";
  encoded = base64_encode(input_2_bytes_padding, sizeof(input_2_bytes_padding) - 1, 0);
  TEST_ASSERT_NOT_NULL(encoded);
  TEST_ASSERT_EQUAL_STRING("SGVsbG8sIFpvcmxkIQ==", encoded);
  const uint8_t input_1_bytes_padding[] = "Hello, Zorlddd";
  encoded = base64_encode(input_1_bytes_padding, sizeof(input_1_bytes_padding) - 1, 0);
  TEST_ASSERT_NOT_NULL(encoded);
  TEST_ASSERT_EQUAL_STRING("SGVsbG8sIFpvcmxkZGQ=", encoded);
  free(encoded);
}

// TODO: Add test with line breaks
// void test_base64_encode_with_line_breaks(void)
// {
// }

void test_base64_decode(void)
{
  const char *input = "SGVsbG8sIFpvcmxkIQ==";
  size_t out_len = 0;
  uint8_t *decoded = base64_decode(input, strlen(input), &out_len);
  
  TEST_ASSERT_NOT_NULL(decoded);
  TEST_ASSERT_EQUAL_STRING("Hello, Zorld!", (char *)decoded);
  TEST_ASSERT_EQUAL(13, out_len);
  free(decoded);
} 

void test_base64_decode_invalid_input(void)
{
  const char *input = "Invalid@String";
  size_t out_len = 0;
  uint8_t *decoded = base64_decode(input, strlen(input), &out_len);
  TEST_ASSERT_NULL(decoded); // Expect NULL for invalid input
  free(decoded);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_base64_encode);
  RUN_TEST(test_base64_decode);
  RUN_TEST(test_base64_decode_invalid_input);
  return UNITY_END();
}