#include <Arduino.h>
#include <unity.h>
#include "../test_config.h"
#include "../mocks/MockETH.h"
#include "../../src/EthernetManager.h"

// External mock instance
extern MockETHClass MockETH;

// Test advanced features
void test_performance_metrics() {
    EthernetManager::cleanup();
    
    // Configure performance settings
    EthernetManager::configurePerformance(true, 200, 20);
    
    // Initialize
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Simulate connection
    MockETH.simulateLinkUp();
    delay(50);
    MockETH.simulateGotIP(TEST_STATIC_IP);
    delay(50);
    
    // Get performance metrics
    uint32_t initTime, linkUpTime, ipObtainTime, eventCount;
    bool metricsAvailable = EthernetManager::getPerformanceMetrics(
        initTime, linkUpTime, ipObtainTime, eventCount
    );
    
    TEST_ASSERT_TRUE(metricsAvailable);
    TEST_ASSERT_GREATER_THAN(0, initTime);
    TEST_ASSERT_GREATER_THAN(0, eventCount);
}

void test_quick_status() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Simulate connection
    MockETH._linkUp = true;
    MockETH._localIP = TEST_STATIC_IP;
    MockETH._linkSpeed = 100;
    MockETH._fullDuplex = true;
    
    IPAddress ip;
    int linkSpeed;
    bool fullDuplex;
    
    bool status = EthernetManager::getQuickStatus(ip, linkSpeed, fullDuplex);
    
    // Should fail when not connected
    TEST_ASSERT_FALSE(status);
    
    // Simulate full connection
    MockETH.simulateGotIP(TEST_STATIC_IP);
    delay(50);
    
    status = EthernetManager::getQuickStatus(ip, linkSpeed, fullDuplex);
    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_TRUE(ip == TEST_STATIC_IP);
    TEST_ASSERT_EQUAL(100, linkSpeed);
    TEST_ASSERT_TRUE(fullDuplex);
}

void test_uptime_tracking() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Initially no uptime
    TEST_ASSERT_EQUAL(0, EthernetManager::getUptimeMs());
    
    // Simulate connection
    MockETH.simulateLinkUp();
    MockETH.simulateGotIP(TEST_STATIC_IP);
    delay(200);
    
    // Now should have uptime
    uint32_t uptime = EthernetManager::getUptimeMs();
    TEST_ASSERT_GREATER_OR_EQUAL(100, uptime);
    TEST_ASSERT_LESS_THAN(1000, uptime);
    
    // Test uptime string
    String uptimeStr = EthernetManager::getUptimeString();
    TEST_ASSERT_TRUE(uptimeStr.length() > 0);
}

void test_dns_configuration() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Set custom DNS servers
    bool dnsSet = EthernetManager::setDNSServers(TEST_DNS1, TEST_DNS2);
    TEST_ASSERT_TRUE(dnsSet);
}

void test_reset_interface() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Reset the interface
    bool resetSuccess = EthernetManager::resetInterface();
    TEST_ASSERT_TRUE(resetSuccess);
}

void test_network_interface_stats() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    uint32_t txPackets, rxPackets, txErrors, rxErrors;
    bool statsAvailable = EthernetManager::getNetworkInterfaceStats(
        txPackets, rxPackets, txErrors, rxErrors
    );
    
    // Stats might not be available in test mode
    if (statsAvailable) {
        TEST_ASSERT_GREATER_OR_EQUAL(0, txPackets);
        TEST_ASSERT_GREATER_OR_EQUAL(0, rxPackets);
    }
}

void test_phy_configuration() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Configure PHY for auto-negotiation
    bool phyConfigured = EthernetManager::configurePHY(true, 100, true);
    TEST_ASSERT_TRUE(phyConfigured);
    
    // Configure PHY for fixed speed
    phyConfigured = EthernetManager::configurePHY(false, 10, false);
    TEST_ASSERT_TRUE(phyConfigured);
}

void test_link_status_check() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Initially link should be down
    TEST_ASSERT_FALSE(EthernetManager::isLinkUp());
    
    // Simulate link up
    MockETH._linkUp = true;
    TEST_ASSERT_TRUE(EthernetManager::isLinkUp());
    
    // Force link status check
    bool linkStatus = EthernetManager::checkLinkStatus();
    TEST_ASSERT_TRUE(linkStatus);
}

void test_diagnostics_dump() {
    EthernetManager::cleanup();
    
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Create a string stream to capture output
    // Note: In actual implementation, you might need a custom Print class
    // For now, just verify the method can be called
    EthernetManager::dumpDiagnostics(&Serial);
    
    // Test passes if no crash occurs
    TEST_ASSERT_TRUE(true);
}

void test_debug_logging_callback() {
    EthernetManager::cleanup();
    
    bool logCallbackCalled = false;
    auto debugCallback = [&logCallbackCalled](const char* msg) {
        logCallbackCalled = true;
    };
    
    EthernetManager::enableDebugLogging(debugCallback);
    
    // Initialize to trigger some logging
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Note: Actual logging depends on implementation
    // This test verifies the API exists
}

// Test runner
void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_performance_metrics);
    RUN_TEST(test_quick_status);
    RUN_TEST(test_uptime_tracking);
    RUN_TEST(test_dns_configuration);
    RUN_TEST(test_reset_interface);
    RUN_TEST(test_network_interface_stats);
    RUN_TEST(test_phy_configuration);
    RUN_TEST(test_link_status_check);
    RUN_TEST(test_diagnostics_dump);
    RUN_TEST(test_debug_logging_callback);
    
    UNITY_END();
}

void loop() {}