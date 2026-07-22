#include <Arduino.h>
#include <unity.h>
#include "../test_config.h"
#include "../mocks/MockETH.h"

// Include after mocks
#include "../../src/EthernetManager.h"

// Mock ETH instance
MockETHClass MockETH;

// Test fixture
class EthernetManagerTest {
public:
    static void setUp() {
        // Reset the mock before each test
        MockETH.reset();
        
        // Clean up EthernetManager state
        EthernetManager::cleanup();
        
        // Reset test state
        connectedCallbackCalled = false;
        disconnectedCallbackCalled = false;
        stateChangeCallbackCalled = false;
        linkStatusCallbackCalled = false;
        lastConnectedIP = IPAddress();
        lastDisconnectDuration = 0;
        lastOldState = EthConnectionState::UNINITIALIZED;
        lastNewState = EthConnectionState::UNINITIALIZED;
        lastLinkStatus = false;
    }
    
    static void tearDown() {
        // Clean up after each test
        EthernetManager::cleanup();
    }
    
    // Callback tracking
    static bool connectedCallbackCalled;
    static bool disconnectedCallbackCalled;
    static bool stateChangeCallbackCalled;
    static bool linkStatusCallbackCalled;
    static IPAddress lastConnectedIP;
    static uint32_t lastDisconnectDuration;
    static EthConnectionState lastOldState;
    static EthConnectionState lastNewState;
    static bool lastLinkStatus;
    
    // Callback implementations
    static void onConnected(IPAddress ip) {
        connectedCallbackCalled = true;
        lastConnectedIP = ip;
    }
    
    static void onDisconnected(uint32_t duration) {
        disconnectedCallbackCalled = true;
        lastDisconnectDuration = duration;
    }
    
    static void onStateChange(EthConnectionState oldState, EthConnectionState newState) {
        stateChangeCallbackCalled = true;
        lastOldState = oldState;
        lastNewState = newState;
    }
    
    static void onLinkStatus(bool linkUp) {
        linkStatusCallbackCalled = true;
        lastLinkStatus = linkUp;
    }
};

// Static member initialization
bool EthernetManagerTest::connectedCallbackCalled = false;
bool EthernetManagerTest::disconnectedCallbackCalled = false;
bool EthernetManagerTest::stateChangeCallbackCalled = false;
bool EthernetManagerTest::linkStatusCallbackCalled = false;
IPAddress EthernetManagerTest::lastConnectedIP;
uint32_t EthernetManagerTest::lastDisconnectDuration = 0;
EthConnectionState EthernetManagerTest::lastOldState = EthConnectionState::UNINITIALIZED;
EthConnectionState EthernetManagerTest::lastNewState = EthConnectionState::UNINITIALIZED;
bool EthernetManagerTest::lastLinkStatus = false;

// Test cases
void test_initialization_default() {
    TEST_ASSERT_FALSE(EthernetManager::isInitialized());
    TEST_ASSERT_FALSE(EthernetManager::isStarted());
    TEST_ASSERT_FALSE(EthernetManager::isConnected());
    TEST_ASSERT_EQUAL(EthConnectionState::UNINITIALIZED, EthernetManager::getConnectionState());
}

void test_initialize_with_config_builder() {
    EthernetConfig config = EthernetConfig()
        .withHostname(TEST_HOSTNAME)
        .withPHYAddress(0)
        .withMDCPin(23)
        .withMDIOPin(18);
    
    EthResult<void> result = EthernetManager::initializeAsync(config);
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(EthernetManager::isStarted());
}

void test_initialize_with_static_ip() {
    EthResult<void> result = EthernetManager::initializeStatic(
        TEST_HOSTNAME,
        TEST_STATIC_IP,
        TEST_GATEWAY,
        TEST_SUBNET,
        TEST_DNS1,
        TEST_DNS2
    );
    
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(EthernetManager::isStarted());
    TEST_ASSERT_EQUAL_STRING(TEST_HOSTNAME, MockETH.getHostname().c_str());
}

void test_callbacks_registration() {
    // Register callbacks
    EthernetManager::setConnectedCallback(EthernetManagerTest::onConnected);
    EthernetManager::setDisconnectedCallback(EthernetManagerTest::onDisconnected);
    EthernetManager::setStateChangeCallback(EthernetManagerTest::onStateChange);
    EthernetManager::setLinkStatusCallback(EthernetManagerTest::onLinkStatus);
    
    // Initialize
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Simulate link up
    MockETH.simulateLinkUp();
    delay(100); // Allow callbacks to process
    
    TEST_ASSERT_TRUE(EthernetManagerTest::linkStatusCallbackCalled);
    TEST_ASSERT_TRUE(EthernetManagerTest::lastLinkStatus);
}

void test_connection_state_transitions() {
    EthernetManager::setStateChangeCallback(EthernetManagerTest::onStateChange);
    
    // Initialize
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Should transition from UNINITIALIZED to PHY_STARTING
    TEST_ASSERT_TRUE(EthernetManagerTest::stateChangeCallbackCalled);
    TEST_ASSERT_EQUAL(EthConnectionState::UNINITIALIZED, EthernetManagerTest::lastOldState);
}

void test_error_handling_invalid_hostname() {
    EthResult<void> result = EthernetManager::initialize(nullptr);
    TEST_ASSERT_FALSE(result.isOk());
    TEST_ASSERT_EQUAL(EthError::INVALID_PARAMETER, result.error);
}

void test_error_handling_already_initialized() {
    // First initialization
    EthResult<void> result1 = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result1.isOk());
    
    // Second initialization should fail
    EthResult<void> result2 = EthernetManager::initializeAsync();
    TEST_ASSERT_FALSE(result2.isOk());
    TEST_ASSERT_EQUAL(EthError::ALREADY_INITIALIZED, result2.error);
}

void test_wait_for_connection_timeout() {
    EthResult<void> initResult = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(initResult.isOk());
    
    // Wait for connection with short timeout
    EthResult<void> waitResult = EthernetManager::waitForConnection(100);
    TEST_ASSERT_FALSE(waitResult.isOk());
    TEST_ASSERT_EQUAL(EthError::CONNECTION_TIMEOUT, waitResult.error);
}

void test_network_statistics() {
    EthResult<void> result = EthernetManager::initializeAsync();
    TEST_ASSERT_TRUE(result.isOk());
    
    // Reset statistics
    EthernetManager::resetStatistics();
    
    // Get initial statistics
    NetworkStats stats = EthernetManager::getStatistics();
    TEST_ASSERT_EQUAL(0, stats.disconnectCount);
    TEST_ASSERT_EQUAL(0, stats.reconnectCount);
    TEST_ASSERT_EQUAL(0, stats.linkDownEvents);
}

void test_auto_reconnect_configuration() {
    EthernetConfig config = EthernetConfig()
        .withHostname(TEST_HOSTNAME)
        .withAutoReconnect(true, 3, 1000, 10000);
    
    EthResult<void> result = EthernetManager::initialize(config);
    TEST_ASSERT_TRUE(result.isOk());
}

void test_link_monitoring_configuration() {
    EthernetConfig config = EthernetConfig()
        .withHostname(TEST_HOSTNAME)
        .withLinkMonitoring(500);
    
    EthResult<void> result = EthernetManager::initialize(config);
    TEST_ASSERT_TRUE(result.isOk());
}

void test_custom_mac_address() {
    uint8_t customMac[] = TEST_MAC_ADDRESS;
    
    EthernetConfig config = EthernetConfig()
        .withHostname(TEST_HOSTNAME)
        .withMACAddress(customMac);
    
    EthResult<void> result = EthernetManager::initializeAsync(config);
    TEST_ASSERT_TRUE(result.isOk());
}

void test_error_to_string() {
    TEST_ASSERT_EQUAL_STRING("Success", EthernetManager::errorToString(EthError::OK));
    TEST_ASSERT_EQUAL_STRING("Invalid parameter", EthernetManager::errorToString(EthError::INVALID_PARAMETER));
    TEST_ASSERT_EQUAL_STRING("Connection timeout", EthernetManager::errorToString(EthError::CONNECTION_TIMEOUT));
}

void test_state_to_string() {
    TEST_ASSERT_EQUAL_STRING("UNINITIALIZED", EthernetManager::stateToString(EthConnectionState::UNINITIALIZED));
    TEST_ASSERT_EQUAL_STRING("CONNECTED", EthernetManager::stateToString(EthConnectionState::CONNECTED));
    TEST_ASSERT_EQUAL_STRING("ERROR_STATE", EthernetManager::stateToString(EthConnectionState::ERROR_STATE));
}

// Main test runner
void setup() {
    // Initialize serial for test output
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    // Wait for stability
    delay(2000);
    
    UNITY_BEGIN();
    
    // Run all tests
    RUN_TEST(test_initialization_default);
    RUN_TEST(test_initialize_with_config_builder);
    RUN_TEST(test_initialize_with_static_ip);
    RUN_TEST(test_callbacks_registration);
    RUN_TEST(test_connection_state_transitions);
    RUN_TEST(test_error_handling_invalid_hostname);
    RUN_TEST(test_error_handling_already_initialized);
    RUN_TEST(test_wait_for_connection_timeout);
    RUN_TEST(test_network_statistics);
    RUN_TEST(test_auto_reconnect_configuration);
    RUN_TEST(test_link_monitoring_configuration);
    RUN_TEST(test_custom_mac_address);
    RUN_TEST(test_error_to_string);
    RUN_TEST(test_state_to_string);
    
    UNITY_END();
}

void loop() {
    // Empty loop
}