// EthernetManager.h
#pragma once

#include <Arduino.h>
#include <ETH.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <esp_netif.h>
#include <functional>

// Include the configuration file
#include "EthernetManagerConfig.h"

// Include common Result type
#include "Result.h"

/**
 * @brief Error codes for EthernetManager operations
 */
enum class EthError {
    OK = 0,                    ///< Operation successful
    INVALID_PARAMETER,         ///< Invalid parameter provided
    MUTEX_TIMEOUT,            ///< Failed to acquire mutex
    ALREADY_INITIALIZED,      ///< Already initialized
    NOT_INITIALIZED,          ///< Not initialized
    PHY_START_FAILED,         ///< PHY start failed
    CONFIG_FAILED,            ///< Configuration failed
    CONNECTION_TIMEOUT,       ///< Connection timeout
    EVENT_HANDLER_FAILED,     ///< Event handler registration failed
    MEMORY_ALLOCATION_FAILED, ///< Memory allocation failed
    NETIF_ERROR,             ///< Network interface error
    UNKNOWN_ERROR            ///< Unknown error
};

/**
 * @brief Result type for EthernetManager operations using common::Result
 * @tparam T The type of the value on success
 */
template<typename T>
using EthResult = common::Result<T, EthError>;

/**
 * @brief Network statistics structure
 */
struct NetworkStats {
    uint32_t connectTime;        ///< Time when connected (ms)
    uint32_t disconnectCount;    ///< Number of disconnections
    uint32_t reconnectCount;     ///< Number of successful reconnections
    uint32_t txPackets;          ///< Transmitted packets
    uint32_t rxPackets;          ///< Received packets
    uint32_t txBytes;            ///< Transmitted bytes
    uint32_t rxBytes;            ///< Received bytes
    uint32_t linkDownEvents;     ///< Link down events
    uint32_t dhcpRenewals;       ///< DHCP renewal count
    uint32_t lastErrorCode;      ///< Last error code
    uint32_t uptimeMs;           ///< Connection uptime in ms
};

/**
 * @brief Connection state enumeration
 */
enum class EthConnectionState {
    UNINITIALIZED,     ///< Not initialized
    PHY_STARTING,      ///< PHY is starting
    LINK_DOWN,         ///< Link is down (cable disconnected)
    LINK_UP,           ///< Link is up but no IP
    OBTAINING_IP,      ///< DHCP in progress
    CONNECTED,         ///< Fully connected with IP
    DISCONNECTING,     ///< Disconnection in progress
    ERROR_STATE        ///< Error state
};

/**
 * @brief Event callback function types
 */
using EthEventCallback = std::function<void(int32_t event, void* eventData)>;
using EthConnectedCallback = std::function<void(IPAddress ip)>;
using EthDisconnectedCallback = std::function<void(uint32_t duration)>;
using EthStateChangeCallback = std::function<void(EthConnectionState oldState, EthConnectionState newState)>;
using EthLinkStatusCallback = std::function<void(bool linkUp)>;

/**
 * @brief Configuration builder for EthernetManager
 * 
 * Provides a fluent interface for configuring Ethernet settings
 */
class EthernetConfig {
public:
    EthernetConfig() : 
        hostname(ETH_HOSTNAME),
        phy_addr(ETH_PHY_ADDR),
        mdc_pin(ETH_PHY_MDC_PIN),
        mdio_pin(ETH_PHY_MDIO_PIN),
        power_pin(ETH_PHY_POWER_PIN),
        clock_mode(ETH_CLOCK_MODE),
        use_static_ip(false),
        enable_link_monitoring(false),
        link_monitor_interval(1000),
        enable_auto_reconnect(false),
        reconnect_max_retries(0),
        reconnect_initial_delay(1000),
        reconnect_max_delay(30000) {}
    
    EthernetConfig& withHostname(const char* name) { hostname = name; return *this; }
    EthernetConfig& withPHYAddress(int8_t addr) { phy_addr = addr; return *this; }
    EthernetConfig& withMDCPin(int8_t pin) { mdc_pin = pin; return *this; }
    EthernetConfig& withMDIOPin(int8_t pin) { mdio_pin = pin; return *this; }
    EthernetConfig& withPowerPin(int8_t pin) { power_pin = pin; return *this; }
    EthernetConfig& withClockMode(eth_clock_mode_t mode) { clock_mode = mode; return *this; }
    EthernetConfig& withMACAddress(const uint8_t* mac) { custom_mac = mac; return *this; }
    
    EthernetConfig& withStaticIP(IPAddress ip, IPAddress gw, IPAddress mask, 
                                 IPAddress dns1 = IPAddress(), IPAddress dns2 = IPAddress()) {
        use_static_ip = true;
        static_ip = ip;
        gateway = gw;
        subnet = mask;
        primary_dns = dns1;
        secondary_dns = dns2;
        return *this;
    }
    
    EthernetConfig& withLinkMonitoring(uint32_t intervalMs = 1000) {
        enable_link_monitoring = true;
        link_monitor_interval = intervalMs;
        return *this;
    }
    
    EthernetConfig& withAutoReconnect(uint8_t maxRetries = 0, uint32_t initialDelay = 1000, 
                                     uint32_t maxDelay = 30000) {
        enable_auto_reconnect = true;
        reconnect_max_retries = maxRetries;
        reconnect_initial_delay = initialDelay;
        reconnect_max_delay = maxDelay;
        return *this;
    }
    
private:
    friend class EthernetManager;
    const char* hostname;
    int8_t phy_addr;
    int8_t mdc_pin;
    int8_t mdio_pin;
    int8_t power_pin;
    eth_clock_mode_t clock_mode;
    const uint8_t* custom_mac = nullptr;
    bool use_static_ip;
    IPAddress static_ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress primary_dns;
    IPAddress secondary_dns;
    bool enable_link_monitoring;
    uint32_t link_monitor_interval;
    bool enable_auto_reconnect;
    uint8_t reconnect_max_retries;
    uint32_t reconnect_initial_delay;
    uint32_t reconnect_max_delay;
};

/**
 * @brief A manager class for ESP32 Ethernet connectivity using LAN8720A PHY
 *
 * This class provides methods to initialize, monitor, and manage Ethernet connections
 * on ESP32 using the LAN8720A PHY. It handles connection events and provides
 * status information about the connection.
 *
 * Optimizations:
 * - Early event handler registration to catch all initialization events
 * - Reduced redundant hostname setting operations
 * - Cached netif handle for faster access
 * - Timing measurements for performance monitoring
 *
 * @par Design Pattern
 * Uses Meyer's singleton pattern for thread-safe initialization. Static methods
 * delegate to getInstance() internally, providing a clean API without requiring
 * users to call getInstance() explicitly. This design is intentional for
 * embedded systems where:
 * - Only one Ethernet interface exists on the hardware
 * - Global access is needed from multiple tasks
 * - Thread safety is handled internally via mutexes
 *
 * @par Testing Considerations
 * For unit testing, mock the underlying ETH class or use integration tests.
 * The singleton design prevents instantiation of multiple instances, which
 * reflects the hardware constraint of a single Ethernet interface.
 */
class EthernetManager {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the EthernetManager singleton
     * @note Thread-safe (C++11 guarantee)
     */
    static EthernetManager& getInstance() {
        static EthernetManager instance;
        return instance;
    }

    // Delete copy/move operations
    EthernetManager(const EthernetManager&) = delete;
    EthernetManager& operator=(const EthernetManager&) = delete;
    EthernetManager(EthernetManager&&) = delete;
    EthernetManager& operator=(EthernetManager&&) = delete;
    /**
     * @brief Initialize with configuration builder (recommended)
     * 
     * @param config Configuration object created with builder pattern
     * @return true if initialization was successful
     */
    [[nodiscard]] static EthResult<void> initialize(const EthernetConfig& config);
    
    /**
     * @brief Initialize asynchronously with configuration builder
     * 
     * @param config Configuration object created with builder pattern
     * @return true if PHY started successfully
     */
    [[nodiscard]] static EthResult<void> initializeAsync(const EthernetConfig& config);
    
    /**
     * @brief Perform early initialization before ETH.begin()
     * 
     * This method sets up event handlers and event groups before starting
     * the Ethernet PHY. Call this early in setup() for best performance.
     * 
     * @return true if early init succeeded
     */
    static bool earlyInit();

    /**
     * @brief Initialize the Ethernet interface (blocking - waits for connection)
     * 
     * @param hostname Optional hostname override (defaults to ETH_HOSTNAME from config)
     * @param phy_addr Optional PHY address override (defaults to ETH_PHY_ADDR from config)
     * @param mdc_pin Optional MDC pin override (defaults to ETH_PHY_MDC_PIN from config)
     * @param mdio_pin Optional MDIO pin override (defaults to ETH_PHY_MDIO_PIN from config)
     * @param power_pin Optional power pin override (defaults to ETH_PHY_POWER_PIN from config)
     * @param clock_mode Optional clock mode override (defaults to ETH_CLOCK_MODE from config)
     * @return EthResult<void> containing error code if failed
     */
    [[nodiscard]] static EthResult<void> initialize(
        const char* hostname = ETH_HOSTNAME,
        int8_t phy_addr = ETH_PHY_ADDR,
        int8_t mdc_pin = ETH_PHY_MDC_PIN,
        int8_t mdio_pin = ETH_PHY_MDIO_PIN,
        int8_t power_pin = ETH_PHY_POWER_PIN,
        eth_clock_mode_t clock_mode = ETH_CLOCK_MODE
    );
    
    /**
     * @brief Initialize the Ethernet interface with static IP configuration
     * 
     * @param hostname Hostname for the device
     * @param local_ip Static IP address
     * @param gateway Gateway IP address
     * @param subnet Subnet mask
     * @param dns1 Primary DNS server (optional)
     * @param dns2 Secondary DNS server (optional)
     * @param phy_addr Optional PHY address override (defaults to ETH_PHY_ADDR from config)
     * @param mdc_pin Optional MDC pin override (defaults to ETH_PHY_MDC_PIN from config)
     * @param mdio_pin Optional MDIO pin override (defaults to ETH_PHY_MDIO_PIN from config)
     * @param power_pin Optional power pin override (defaults to ETH_PHY_POWER_PIN from config)
     * @param clock_mode Optional clock mode override (defaults to ETH_CLOCK_MODE from config)
     * @return EthResult<void> containing error code if failed
     */
    [[nodiscard]] static EthResult<void> initializeStatic(
        const char* hostname,
        IPAddress local_ip,
        IPAddress gateway,
        IPAddress subnet,
        IPAddress dns1 = IPAddress(),
        IPAddress dns2 = IPAddress(),
        int8_t phy_addr = ETH_PHY_ADDR,
        int8_t mdc_pin = ETH_PHY_MDC_PIN,
        int8_t mdio_pin = ETH_PHY_MDIO_PIN,
        int8_t power_pin = ETH_PHY_POWER_PIN,
        eth_clock_mode_t clock_mode = ETH_CLOCK_MODE
    );

    /**
     * @brief Start Ethernet interface asynchronously (non-blocking)
     * 
     * This method starts the Ethernet PHY but does not wait for a connection.
     * Use waitForConnection() or isConnected() to check connection status later.
     * 
     * @param hostname Optional hostname override (defaults to ETH_HOSTNAME from config)
     * @param phy_addr Optional PHY address override (defaults to ETH_PHY_ADDR from config)
     * @param mdc_pin Optional MDC pin override (defaults to ETH_PHY_MDC_PIN from config)
     * @param mdio_pin Optional MDIO pin override (defaults to ETH_PHY_MDIO_PIN from config)
     * @param power_pin Optional power pin override (defaults to ETH_PHY_POWER_PIN from config)
     * @param clock_mode Optional clock mode override (defaults to ETH_CLOCK_MODE from config)
     * @param customMac Optional custom MAC address (6 bytes)
     * @return EthResult<void> containing error code if failed
     */
    [[nodiscard]] static EthResult<void> initializeAsync(
        const char* hostname = ETH_HOSTNAME,
        int8_t phy_addr = ETH_PHY_ADDR,
        int8_t mdc_pin = ETH_PHY_MDC_PIN,
        int8_t mdio_pin = ETH_PHY_MDIO_PIN,
        int8_t power_pin = ETH_PHY_POWER_PIN,
        eth_clock_mode_t clock_mode = ETH_CLOCK_MODE,
        const uint8_t* customMac = nullptr
    );

    /**
     * @brief Clean up resources used by the Ethernet manager
     */
    static void cleanup();
    
    /**
     * @brief Disconnect from Ethernet network
     * 
     * Properly shuts down the Ethernet interface and cleans up resources.
     * This will trigger the disconnected callback if set.
     */
    static void disconnect();

    /**
     * @brief Wait for an Ethernet connection to be established
     * 
     * @param timeout_ms Maximum time to wait in milliseconds
     * @return EthResult<void> - OK if connected within timeout, CONNECTION_TIMEOUT otherwise
     */
    [[nodiscard]] static EthResult<void> waitForConnection(uint32_t timeout_ms = ETH_INIT_TIMEOUT_MS);

    /**
     * @brief Check if Ethernet is currently connected
     * 
     * @return true if connected, false otherwise
     */
    static bool isConnected();

    /**
     * @brief Check if Ethernet PHY has been started
     *
     * @return true if PHY is started (but not necessarily connected)
     */
    static bool isStarted() noexcept { return getInstance().phyStarted; }

    /**
     * @brief Check if Ethernet manager has been initialized
     *
     * Alias for isStarted() to provide consistent API across all managers.
     * @return true if initialize() or initializeAsync() was successfully called
     */
    static bool isInitialized() noexcept { return getInstance().phyStarted; }

    /**
     * @brief Log current Ethernet status (IP, MAC, hostname, etc.)
     */
    static void logEthernetStatus();

    /**
     * @brief Set custom MAC address (must be called before initialize/initializeAsync)
     * 
     * @param mac 6-byte MAC address array
     */
    static void setMacAddress(const uint8_t* mac);

    /**
     * @brief Get the cached esp_netif handle
     *
     * @return esp_netif_t* or nullptr if not initialized
     */
    static esp_netif_t* getNetif() { return getInstance().eth_netif; }
    
    /**
     * @brief Set callback for connection events
     * 
     * @param callback Function to call when connected
     */
    static void setConnectedCallback(EthConnectedCallback callback);
    
    /**
     * @brief Set callback for disconnection events
     * 
     * @param callback Function to call when disconnected
     */
    static void setDisconnectedCallback(EthDisconnectedCallback callback);
    
    /**
     * @brief Set callback for state change events
     * 
     * @param callback Function to call when connection state changes
     */
    static void setStateChangeCallback(EthStateChangeCallback callback);
    
    /**
     * @brief Set callback for link status changes
     * 
     * @param callback Function to call when link status changes
     */
    static void setLinkStatusCallback(EthLinkStatusCallback callback);
    
    /**
     * @brief Get current connection state
     *
     * @return Current EthConnectionState
     */
    static EthConnectionState getConnectionState() { return getInstance().connectionState; }
    
    /**
     * @brief Get connection state as string
     * 
     * @param state State to convert
     * @return String representation of state
     */
    static const char* stateToString(EthConnectionState state);
    
    /**
     * @brief Get network statistics
     * 
     * @return NetworkStats structure with current statistics
     */
    static NetworkStats getStatistics();
    
    /**
     * @brief Reset network statistics
     */
    static void resetStatistics();
    
    /**
     * @brief Enable automatic reconnection with exponential backoff
     * 
     * @param enable Enable or disable auto reconnect
     * @param maxRetries Maximum number of retry attempts (0 = infinite)
     * @param initialDelayMs Initial delay between retries in milliseconds
     * @param maxDelayMs Maximum delay between retries in milliseconds
     */
    static void setAutoReconnect(bool enable, uint8_t maxRetries = 0, 
                                uint32_t initialDelayMs = 1000, uint32_t maxDelayMs = 30000);
    
    /**
     * @brief Get last error code
     *
     * @return Last EthError that occurred
     */
    static EthError getLastError() { return getInstance().lastError; }
    
    /**
     * @brief Convert error code to string
     * 
     * @param error Error code to convert
     * @return String representation of error
     */
    static const char* errorToString(EthError error);
    
    /**
     * @brief Dump diagnostic information
     * 
     * @param output Print function for output (default Serial.print)
     */
    static void dumpDiagnostics(Print* output = &Serial);
    
    /**
     * @brief Enable/disable link monitoring
     * 
     * @param enable Enable or disable link monitoring
     * @param intervalMs Check interval in milliseconds (default 1000)
     */
    static void setLinkMonitoring(bool enable, uint32_t intervalMs = 1000);
    
    /**
     * @brief Force a link status check
     * 
     * @return Current link status
     */
    static bool checkLinkStatus();
    
    /**
     * @brief Get detailed network interface statistics
     * 
     * @param txPackets Output for transmitted packets
     * @param rxPackets Output for received packets
     * @param txErrors Output for transmission errors
     * @param rxErrors Output for reception errors
     * @return true if statistics were retrieved successfully
     */
    static bool getNetworkInterfaceStats(uint32_t& txPackets, uint32_t& rxPackets, 
                                       uint32_t& txErrors, uint32_t& rxErrors);
    
    /**
     * @brief Configure advanced PHY settings
     * 
     * @param autoNegotiate Enable auto-negotiation
     * @param speed Speed in Mbps (10 or 100), ignored if autoNegotiate is true
     * @param fullDuplex Full duplex mode, ignored if autoNegotiate is true
     * @return true if configuration was successful
     */
    static bool configurePHY(bool autoNegotiate = true, uint8_t speed = 100, bool fullDuplex = true);
    
    /**
     * @brief Reset the Ethernet interface
     * 
     * This performs a soft reset of the Ethernet interface, useful for
     * recovering from error states without full re-initialization
     * 
     * @return true if reset was successful
     */
    static bool resetInterface();
    
    /**
     * @brief Set custom DNS servers
     * 
     * @param dns1 Primary DNS server
     * @param dns2 Secondary DNS server (optional)
     * @return true if DNS servers were set successfully
     */
    static bool setDNSServers(IPAddress dns1, IPAddress dns2 = IPAddress());
    
    /**
     * @brief Check if link is up (cable connected)
     *
     * @return true if physical link is up
     */
    static bool isLinkUp() { return getInstance().phyStarted && ETH.linkUp(); }
    
    /**
     * @brief Get connection uptime in milliseconds
     *
     * @return Connection uptime or 0 if not connected
     */
    static uint32_t getUptimeMs() {
        auto& inst = getInstance();
        return (isConnected() && inst.stats.connectTime > 0) ? (millis() - inst.stats.connectTime) : 0;
    }
    
    /**
     * @brief Get formatted uptime string
     * 
     * @return Uptime formatted as "Xd Yh Zm Ws"
     */
    static String getUptimeString();
    
    /**
     * @brief Enable debug logging with custom log function
     *
     * @param logFunc Function pointer for logging
     */
    static void enableDebugLogging(void (*logFunc)(const char*)) { getInstance().debugLogCallback = logFunc; }
    
    /**
     * @brief Quick status check with single call
     * 
     * @param ip Output parameter for IP address
     * @param linkSpeed Output parameter for link speed
     * @param fullDuplex Output parameter for duplex mode
     * @return true if connected and parameters filled
     */
    static bool getQuickStatus(IPAddress& ip, int& linkSpeed, bool& fullDuplex);
    
    /**
     * @brief Configure performance optimizations
     * 
     * @param enableEventBatching Batch multiple events to reduce callback overhead
     * @param mutexTimeoutMs Timeout for mutex operations (default 100ms)
     * @param eventQueueSize Size of event queue for batching (default 10)
     */
    static void configurePerformance(bool enableEventBatching = true, 
                                   uint32_t mutexTimeoutMs = 100,
                                   uint8_t eventQueueSize = 10);
    
    /**
     * @brief Get performance metrics
     * 
     * @param initTimeMs Output for initialization time
     * @param linkUpTimeMs Output for time to link up
     * @param ipObtainTimeMs Output for time to obtain IP
     * @param eventCount Output for total events processed
     * @return true if metrics are available
     */
    static bool getPerformanceMetrics(uint32_t& initTimeMs, uint32_t& linkUpTimeMs, 
                                    uint32_t& ipObtainTimeMs, uint32_t& eventCount);
    
    /**
     * @brief Set verbose logging mode
     * 
     * When enabled, detailed multi-line status is logged.
     * When disabled (default), compact single-line status is logged.
     * 
     * @param enable true to enable verbose logging, false for compact
     */
    static void setVerboseLogging(bool enable);
    
    /**
     * @brief Set the log level for status messages
     * 
     * Allows fine-grained control over status message verbosity.
     * Does not affect error or warning messages.
     * 
     * @param level ESP log level for status messages (default ESP_LOG_INFO)
     */
    void setStatusLogLevel(esp_log_level_t level);

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    EthernetManager();

    /**
     * @brief Destructor
     */
    ~EthernetManager();

    /**
     * @brief Event handler for Ethernet and IP events
     */
    static void onEthEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    /**
     * @brief Internal initialization helper
     */
    bool internalInit(const char* hostname, int8_t phy_addr, int8_t mdc_pin,
                     int8_t mdio_pin, int8_t power_pin, eth_clock_mode_t clock_mode,
                     const uint8_t* customMac, bool waitForConn);

    /**
     * @brief Internal initialization with static IP configuration
     */
    bool internalInitStatic(const char* hostname, IPAddress local_ip, IPAddress gateway,
                           IPAddress subnet, IPAddress dns1, IPAddress dns2,
                           int8_t phy_addr, int8_t mdc_pin, int8_t mdio_pin,
                           int8_t power_pin, eth_clock_mode_t clock_mode);

    // Event group for tracking Ethernet state
    EventGroupHandle_t ethEventGroup = nullptr;
    static constexpr EventBits_t BIT_CONNECTED = BIT0;

    // Connection state tracking
    unsigned long lastGotIpTime = 0;
    unsigned long connectionStartTime = 0;
    bool gotIpAtLeastOnce = false;
    bool phyStarted = false;
    bool eventHandlersRegistered = false;

    // Custom MAC storage
    uint8_t customMacAddress[ETH_MAC_ADDRESS_SIZE] = {0};
    bool hasCustomMac = false;

    // Cached handles for performance
    esp_netif_t* eth_netif = nullptr;
    bool netifCreated = false;

    // Mutex for thread safety
    SemaphoreHandle_t ethMutex = nullptr;
    bool mutexCreated = false;

    // Error tracking
    EthError lastError = EthError::OK;

    // Network statistics
    NetworkStats stats = {0};

    // User callbacks
    EthConnectedCallback connectedCallback = nullptr;
    EthDisconnectedCallback disconnectedCallback = nullptr;
    EthStateChangeCallback stateChangeCallback = nullptr;
    EthLinkStatusCallback linkStatusCallback = nullptr;

    // Auto-reconnect settings
    bool autoReconnectEnabled = false;
    uint8_t reconnectMaxRetries = 0;
    uint8_t reconnectAttempts = 0;
    uint32_t reconnectInitialDelay = 1000;
    uint32_t reconnectMaxDelay = 30000;
    uint32_t reconnectCurrentDelay = 1000;
    TimerHandle_t reconnectTimer = nullptr;

    // Connection state machine
    EthConnectionState connectionState = EthConnectionState::UNINITIALIZED;
    EthConnectionState previousState = EthConnectionState::UNINITIALIZED;

    // Link monitoring
    bool linkMonitoringEnabled = false;
    uint32_t linkMonitoringInterval = 1000;
    TimerHandle_t linkMonitorTimer = nullptr;
    bool lastLinkStatus = false;

    // Performance monitoring
    uint32_t initStartTime = 0;
    uint32_t linkUpTime = 0;
    uint32_t ipObtainedTime = 0;

    // Debug logging
    void (*debugLogCallback)(const char*) = nullptr;

    // Verbose logging control
    bool verboseLogging = false;
    esp_log_level_t statusLogLevel = ESP_LOG_INFO;

    // Performance optimization
    bool eventBatchingEnabled = false;
    uint32_t mutexTimeout = ETH_MUTEX_QUICK_TIMEOUT_MS;
    uint8_t maxEventQueueSize = 10;
    uint32_t totalEventCount = 0;
    QueueHandle_t eventQueue = nullptr;
    TimerHandle_t eventBatchTimer = nullptr;

    // Helper methods
    void updateStatistics();
    static void attemptReconnect(TimerHandle_t xTimer);
    static void linkMonitorTask(TimerHandle_t xTimer);
    void changeState(EthConnectionState newState);
    bool updateLinkStatus();
    static void processBatchedEvents(TimerHandle_t xTimer);
    void queueEvent(esp_event_base_t base, int32_t id, void* data);
};
