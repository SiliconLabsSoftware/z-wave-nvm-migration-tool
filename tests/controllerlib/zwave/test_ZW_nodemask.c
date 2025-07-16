#include <unity.h>
#include <string.h>
#include "ZW_nodemask_api.h"

void setUp(void)
{
	// Initialize resources or mock
}
void tearDown(void)
{
	// Clean up resources or mock
}

void test_ZW_NodeMaskSetBit(void) {
	CLASSIC_NODE_MASK_TYPE classic_node_mask = {0};
	// Set bit 5 (should set bit 4 of byte 0)
	ZW_NodeMaskSetBit(classic_node_mask, 5);
	TEST_ASSERT_EQUAL_HEX8(0x10, classic_node_mask[0]); // bit 4 (zero-based) set, corresponds to node 5 (one-based)
	// Set bit 12 (should set bit 4 of byte 1)
	ZW_NodeMaskSetBit(classic_node_mask, 12);
	TEST_ASSERT_EQUAL_HEX8(0x08, classic_node_mask[1]); // bit 3 set (bit 4 one-based)
	// Set bit 16 (mask set bit 0 of byte 2)
	ZW_NodeMaskSetBit(classic_node_mask, 16);
	TEST_ASSERT_EQUAL_HEX8(0x88, classic_node_mask[1]); // bit 0 set

}

void test_ZW_NodeMaskClearBit(void) {
	CLASSIC_NODE_MASK_TYPE classic_node_mask = {0};
	memset(classic_node_mask, 0xFF, MAX_CLASSIC_NODEMASK_LENGTH);
	// Clear bit 8 (bit 7 of byte 0)
	ZW_NodeMaskClearBit(classic_node_mask, 8);
	TEST_ASSERT_EQUAL_HEX8(0x7F, classic_node_mask[0]); // bit 7 cleared
	// Clear bit 12 (bit 4 of byte 1)
	ZW_NodeMaskClearBit(classic_node_mask, 12);
	TEST_ASSERT_EQUAL_HEX8(0xF7, classic_node_mask[1]); // bit 4 cleared
	// Clear bit 16 (bit 0 of byte 2)
	ZW_NodeMaskClearBit(classic_node_mask, 16);
	TEST_ASSERT_EQUAL_HEX8(0x77, classic_node_mask[1]); // bit 0 cleared
}

void test_ZW_NodeMaskClear(void) {
	CLASSIC_NODE_MASK_TYPE classic_node_mask = {0};
	memset(classic_node_mask, 0xFF, MAX_CLASSIC_NODEMASK_LENGTH);
	ZW_NodeMaskClear(classic_node_mask, MAX_CLASSIC_NODEMASK_LENGTH);
	for (int i = 0; i < MAX_CLASSIC_NODEMASK_LENGTH; ++i) {
		TEST_ASSERT_EQUAL_HEX8(0x00, classic_node_mask[i]);
	}
}

void test_ZW_NodeMaskBitsIn(void) {
	CLASSIC_NODE_MASK_TYPE classic_node_mask = {0};
	// Set bits directly without using ZW_NodeMaskSetBit
	classic_node_mask[0] = 0x81; // bits 0 and 7 set (1 and 8)
	classic_node_mask[1] = 0x80; // bit 7 set (16)
	TEST_ASSERT_EQUAL(3, ZW_NodeMaskBitsIn(classic_node_mask, MAX_CLASSIC_NODEMASK_LENGTH)); 
}

void test_ZW_NodeMaskGetNextNode(void) {
	CLASSIC_NODE_MASK_TYPE classic_node_mask = {0};
	// Set bits directly
	classic_node_mask[0] = 0x0A; // bits 1 and 3 set (nodes 2 and 4)
	classic_node_mask[1] = 0x01; // bit 0 set (node 9)

	uint8_t node = 0;
	node = ZW_NodeMaskGetNextNode(node, classic_node_mask);
	TEST_ASSERT_EQUAL(2, node);
	node = ZW_NodeMaskGetNextNode(node, classic_node_mask);
	TEST_ASSERT_EQUAL(4, node);
	node = ZW_NodeMaskGetNextNode(node, classic_node_mask);
	TEST_ASSERT_EQUAL(9, node);
	node = ZW_NodeMaskGetNextNode(node, classic_node_mask);
	TEST_ASSERT_EQUAL(0, node);
}


void test_ZW_NodeMaskNodeIn(void) {
	CLASSIC_NODE_MASK_TYPE classic_node_mask = {0};
	// Set bits directly: bits 0 (node 1), 7 (node 8), 15 (node 16)
	classic_node_mask[0] = 0x81; // bits 0 and 7 set
	classic_node_mask[1] = 0x80; // bit 7 set
	// Check present bits
	TEST_ASSERT_EQUAL(1, ZW_NodeMaskNodeIn(classic_node_mask, 1));
	TEST_ASSERT_EQUAL(1, ZW_NodeMaskNodeIn(classic_node_mask, 8));
	TEST_ASSERT_EQUAL(1, ZW_NodeMaskNodeIn(classic_node_mask, 16));
	// Check absent bits
	TEST_ASSERT_EQUAL(0, ZW_NodeMaskNodeIn(classic_node_mask, 2));
	TEST_ASSERT_EQUAL(0, ZW_NodeMaskNodeIn(classic_node_mask, 9));
	TEST_ASSERT_EQUAL(0, ZW_NodeMaskNodeIn(classic_node_mask, 15));
}


int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_ZW_NodeMaskSetBit);
	RUN_TEST(test_ZW_NodeMaskClearBit);
	RUN_TEST(test_ZW_NodeMaskClear);
	RUN_TEST(test_ZW_NodeMaskBitsIn);
	RUN_TEST(test_ZW_NodeMaskGetNextNode);
	RUN_TEST(test_ZW_NodeMaskNodeIn);
	return UNITY_END();
}
