#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

// Test configuration for EthernetManager unit tests
// This file configures the test environment

// Enable test mode - this might disable certain hardware dependencies
#define ETHERNET_MANAGER_TEST_MODE

// Test timeouts (shorter than production for faster tests)
#define TEST_INIT_TIMEOUT_MS 1000
#define TEST_CONNECTION_TIMEOUT_MS 2000
#define TEST_WAIT_DELAY_MS 10

// Test network configuration
#define TEST_HOSTNAME "test-esp32"
#define TEST_STATIC_IP IPAddress(192, 168, 1, 100)
#define TEST_GATEWAY IPAddress(192, 168, 1, 1)
#define TEST_SUBNET IPAddress(255, 255, 255, 0)
#define TEST_DNS1 IPAddress(8, 8, 8, 8)
#define TEST_DNS2 IPAddress(8, 8, 4, 4)

// Test MAC address
#define TEST_MAC_ADDRESS {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}

// Unity test framework configuration
#include <unity.h>

// Helper macros for async tests
#define TEST_ASSERT_WAIT_TRUE(condition, timeout_ms) \
    do { \
        unsigned long start = millis(); \
        while (!(condition) && (millis() - start < timeout_ms)) { \
            delay(TEST_WAIT_DELAY_MS); \
        } \
        TEST_ASSERT_TRUE(condition); \
    } while(0)

#define TEST_ASSERT_WAIT_FALSE(condition, timeout_ms) \
    do { \
        unsigned long start = millis(); \
        while ((condition) && (millis() - start < timeout_ms)) { \
            delay(TEST_WAIT_DELAY_MS); \
        } \
        TEST_ASSERT_FALSE(condition); \
    } while(0)

#endif // TEST_CONFIG_H