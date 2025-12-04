// EthernetManager.cpp
#include "EthernetManager.h"
#include "MutexGuard.h"

#include <esp_eth.h>
#include <esp_event.h>
#include <esp_netif.h>

// Fallback for missing macro in older ESP32 Arduino cores
#ifndef IP_EVENT_GOT_IP
#define IP_EVENT_GOT_IP 4
#endif

// Constructor for singleton
EthernetManager::EthernetManager() {
    // Initialize mutex
    ethMutex = xSemaphoreCreateMutex();
    if (ethMutex) {
        mutexCreated = true;
    }

    // Create event group
    ethEventGroup = xEventGroupCreate();
}

// Destructor
EthernetManager::~EthernetManager() {
    cleanup();
    if (ethMutex) {
        vSemaphoreDelete(ethMutex);
        ethMutex = nullptr;
    }
    if (ethEventGroup) {
        vEventGroupDelete(ethEventGroup);
        ethEventGroup = nullptr;
    }
}

EthResult<void> EthernetManager::initialize(const EthernetConfig& config) {
    auto& inst = getInstance();

    // Apply configuration settings
    if (config.custom_mac) {
        setMacAddress(config.custom_mac);
    }

    // Configure auto-reconnect if enabled
    if (config.enable_auto_reconnect) {
        setAutoReconnect(true, config.reconnect_max_retries,
                        config.reconnect_initial_delay, config.reconnect_max_delay);
    }

    // Initialize based on IP configuration
    EthResult<void> result;
    if (config.use_static_ip) {
        result = initializeStatic(config.hostname, config.static_ip, config.gateway,
                                 config.subnet, config.primary_dns, config.secondary_dns,
                                 config.phy_addr, config.mdc_pin, config.mdio_pin,
                                 config.power_pin, config.clock_mode);
    } else {
        bool success = inst.internalInit(config.hostname, config.phy_addr, config.mdc_pin,
                            config.mdio_pin, config.power_pin, config.clock_mode,
                            config.custom_mac, true);
        result = success ? EthResult<void>::ok() : EthResult<void>(inst.lastError);
    }

    // Enable link monitoring if configured
    if (result.isOk() && config.enable_link_monitoring) {
        setLinkMonitoring(true, config.link_monitor_interval);
    }

    return result;
}

EthResult<void> EthernetManager::initializeAsync(const EthernetConfig& config) {
    auto& inst = getInstance();

    // Apply configuration settings
    if (config.custom_mac) {
        setMacAddress(config.custom_mac);
    }

    // Configure auto-reconnect if enabled
    if (config.enable_auto_reconnect) {
        setAutoReconnect(true, config.reconnect_max_retries,
                        config.reconnect_initial_delay, config.reconnect_max_delay);
    }

    bool result;

    // Choose initialization method based on static IP configuration
    if (config.use_static_ip) {
        // Use static IP initialization
        ETH_LOG_I("Using static IP configuration from EthernetConfig");
        result = inst.internalInitStatic(config.hostname, config.static_ip, config.gateway,
                                   config.subnet, config.primary_dns, config.secondary_dns,
                                   config.phy_addr, config.mdc_pin, config.mdio_pin,
                                   config.power_pin, config.clock_mode);
    } else {
        // Use DHCP (async) initialization
        result = inst.internalInit(config.hostname, config.phy_addr, config.mdc_pin,
                             config.mdio_pin, config.power_pin, config.clock_mode,
                             config.custom_mac, false);
    }

    // Enable link monitoring if configured
    if (result && config.enable_link_monitoring) {
        setLinkMonitoring(true, config.link_monitor_interval);
    }

    return result ? EthResult<void>::ok() : EthResult<void>(inst.lastError);
}

EthResult<void> EthernetManager::initialize(const char* hostname, int8_t phy_addr, int8_t mdc_pin,
                                 int8_t mdio_pin, int8_t power_pin, eth_clock_mode_t clock_mode) {
    auto& inst = getInstance();
    bool result = inst.internalInit(hostname, phy_addr, mdc_pin, mdio_pin, power_pin, clock_mode,
                       inst.hasCustomMac ? inst.customMacAddress : nullptr, true);
    if (result) {
        return EthResult<void>::ok();
    }
    return EthResult<void>(inst.lastError);
}

EthResult<void> EthernetManager::initializeStatic(const char* hostname, IPAddress local_ip, IPAddress gateway,
                                      IPAddress subnet, IPAddress dns1, IPAddress dns2,
                                      int8_t phy_addr, int8_t mdc_pin, int8_t mdio_pin,
                                      int8_t power_pin, eth_clock_mode_t clock_mode) {
    auto& inst = getInstance();
    bool result = inst.internalInitStatic(hostname, local_ip, gateway, subnet, dns1, dns2,
                            phy_addr, mdc_pin, mdio_pin, power_pin, clock_mode);
    if (result) {
        return EthResult<void>::ok();
    }
    return EthResult<void>(inst.lastError);
}

EthResult<void> EthernetManager::initializeAsync(const char* hostname, int8_t phy_addr, int8_t mdc_pin,
                                      int8_t mdio_pin, int8_t power_pin, eth_clock_mode_t clock_mode,
                                      const uint8_t* customMac) {
    auto& inst = getInstance();
    bool result = inst.internalInit(hostname, phy_addr, mdc_pin, mdio_pin, power_pin, clock_mode,
                       customMac, false);
    if (result) {
        return EthResult<void>::ok();
    }
    return EthResult<void>(inst.lastError);
}

bool EthernetManager::earlyInit() {
    ETH_LOG_D("Performing early Ethernet initialization");
    auto& inst = getInstance();

    // Mutex is now created in constructor, just verify it exists
    if (!inst.ethMutex) {
        ETH_LOG_E("Mutex not available");
        inst.lastError = EthError::MEMORY_ALLOCATION_FAILED;
        return false;
    }

    // Take mutex for thread safety using RAII guard
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_STANDARD_TIMEOUT_MS));
    if (!guard) {
        ETH_LOG_E("Failed to take mutex");
        return false;
    }

    // Ensure the default event loop exists
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        // ESP_ERR_INVALID_STATE means it already exists, which is fine
        ETH_LOG_E("Failed to create default event loop: %d", err);
        return false;  // MutexGuard auto-releases on return
    }

    // Event group is now created in constructor, just verify it exists
    if (!inst.ethEventGroup) {
        inst.ethEventGroup = xEventGroupCreate();
        if (!inst.ethEventGroup) {
            ETH_LOG_E("Failed to create EventGroup");
            inst.lastError = EthError::MEMORY_ALLOCATION_FAILED;
            return false;  // MutexGuard auto-releases on return
        }
    }

    // Register event handlers early to catch all events
    if (!inst.eventHandlersRegistered) {
        // Use ESP_EVENT_ANY_BASE instead of specific bases for early registration
        err = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, onEthEvent, nullptr);
        if (err != ESP_OK) {
            ETH_LOG_E("Failed to register IP event handler: %d", err);
            // Clean up event group if we created it
            if (inst.ethEventGroup) {
                vEventGroupDelete(inst.ethEventGroup);
                inst.ethEventGroup = nullptr;
            }
            return false;  // MutexGuard auto-releases on return
        }

        err = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, onEthEvent, nullptr);
        if (err != ESP_OK) {
            ETH_LOG_E("Failed to register ETH event handler: %d", err);
            // Unregister IP event handler
            esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, onEthEvent);
            // Clean up event group if we created it
            if (inst.ethEventGroup) {
                vEventGroupDelete(inst.ethEventGroup);
                inst.ethEventGroup = nullptr;
            }
            return false;  // MutexGuard auto-releases on return
        }

        inst.eventHandlersRegistered = true;
        ETH_LOG_D("Event handlers registered early");
    }

    return true;  // MutexGuard auto-releases on return
}

bool EthernetManager::internalInit(const char* hostname, int8_t phy_addr, int8_t mdc_pin,
                                   int8_t mdio_pin, int8_t power_pin, eth_clock_mode_t clock_mode,
                                   const uint8_t* customMac, bool waitForConn) {
    ETH_TIME_START();  // Start timing initialization
    ETH_LOG_D("Initializing Ethernet Manager (async=%s)", waitForConn ? "false" : "true");

    // Check if already initialized (no mutex needed for read)
    if (phyStarted && connectionState != EthConnectionState::UNINITIALIZED) {
        ETH_LOG_W("Ethernet already initialized in state: %s", stateToString(connectionState));
        lastError = EthError::ALREADY_INITIALIZED;
        return false;
    }

    // Validate parameters (no mutex needed)
    if (!hostname || strlen(hostname) == 0) {
        ETH_LOG_E("Invalid hostname (null or empty)");
        lastError = EthError::INVALID_PARAMETER;
        changeState(EthConnectionState::ERROR_STATE);
        return false;
    }

    // Validate hostname length (max 63 chars per RFC)
    if (strlen(hostname) > ETH_MAX_HOSTNAME_LENGTH) {
        ETH_LOG_E("Hostname too long (max %d chars): %d", ETH_MAX_HOSTNAME_LENGTH, strlen(hostname));
        lastError = EthError::INVALID_PARAMETER;
        changeState(EthConnectionState::ERROR_STATE);
        return false;
    }

    if (phy_addr < 0 || phy_addr > ETH_MAX_PHY_ADDRESS) {
        ETH_LOG_E("Invalid PHY address: %d (must be 0-%d)", phy_addr, ETH_MAX_PHY_ADDRESS);
        return false;
    }

    if (mdc_pin < -1 || mdc_pin >= GPIO_NUM_MAX) {
        ETH_LOG_E("Invalid MDC pin: %d", mdc_pin);
        return false;
    }

    if (mdio_pin < -1 || mdio_pin >= GPIO_NUM_MAX) {
        ETH_LOG_E("Invalid MDIO pin: %d", mdio_pin);
        return false;
    }

    if (power_pin < -1 || power_pin >= GPIO_NUM_MAX) {
        ETH_LOG_E("Invalid power pin: %d", power_pin);
        return false;
    }

    // Measure initialization time
    unsigned long startTime = millis();
    (void)startTime; // Suppress unused warning when logging is disabled

    // Early initialization - ensure it's been called (earlyInit handles its own mutex)
    if (!mutexCreated || !ethEventGroup || !eventHandlersRegistered) {
        if (!earlyInit()) {
            return false;
        }
    }

    // Main initialization section - protected by mutex
    {
        MutexGuard guard(ethMutex, pdMS_TO_TICKS(ETH_MUTEX_INIT_TIMEOUT_MS));
        if (!guard) {
            ETH_LOG_E("Failed to take mutex for init");
            return false;
        }

        xEventGroupClearBits(ethEventGroup, BIT_CONNECTED);

        // Set custom MAC if provided before ETH.begin
        if (customMac) {
            memcpy(customMacAddress, customMac, ETH_MAC_ADDRESS_SIZE);
            hasCustomMac = true;
        }

        ETH_LOG_D("Starting ETH.begin at %lu ms", millis() - startTime);

        // Update state to PHY starting
        changeState(EthConnectionState::PHY_STARTING);

        // Start Ethernet PHY with optimized initialization
        bool success = false;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        // Arduino 3.x+ version

        // Pre-configure hostname to avoid multiple sets
        if (hostname) {
            ETH.setHostname(hostname);
        }

        // Start Ethernet with all parameters at once
        success = ETH.begin(ETH_PHY_LAN8720, phy_addr, mdc_pin, mdio_pin, power_pin, clock_mode);

        if (success && hasCustomMac) {
            // Apply custom MAC if needed
            if (!ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE, customMacAddress)) {
                ETH_LOG_W("Failed to set custom MAC address");
                // Continue anyway - not a critical failure
            } else {
                ETH_LOG_I("Custom MAC set: %02X:%02X:%02X:%02X:%02X:%02X",
                            customMacAddress[0], customMacAddress[1], customMacAddress[2],
                            customMacAddress[3], customMacAddress[4], customMacAddress[5]);
            }
        }
#else
        // Arduino 2.x version
        success = ETH.begin(power_pin, mdc_pin, mdio_pin, phy_addr, ETH_PHY_LAN8720, clock_mode);

        if (success && hostname) {
            ETH.setHostname(hostname);

            if (customMac) {
                if (!ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE, customMac)) {
                    ETH_LOG_W("Failed to set custom MAC address");
                    // Continue anyway - not a critical failure
                } else {
                    ETH_LOG_I("Custom MAC set: %02X:%02X:%02X:%02X:%02X:%02X",
                                customMac[0], customMac[1], customMac[2],
                                customMac[3], customMac[4], customMac[5]);
                }
            }
        }
#endif

        if (!success) {
            ETH_LOG_E("ETH.begin failed after %lu ms", millis() - startTime);
            lastError = EthError::PHY_START_FAILED;
            changeState(EthConnectionState::ERROR_STATE);
            return false;  // MutexGuard auto-releases
        }

        phyStarted = true;
        ETH_LOG_D("ETH.begin completed after %lu ms", millis() - startTime);

        // Store netif handle for later use
        eth_netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
        if (eth_netif) {
            netifCreated = true;

            // Set hostname via esp_netif (only if different)
            const char* current_hostname = nullptr;
            esp_netif_get_hostname(eth_netif, &current_hostname);

            if (hostname && (!current_hostname || strcmp(current_hostname, hostname) != 0)) {
                esp_err_t err = esp_netif_set_hostname(eth_netif, hostname);
                if (err == ESP_OK) {
                    ETH_LOG_D("Hostname set to '%s'", hostname);
                } else {
                    ETH_LOG_E("Failed to set hostname (err %d)", err);
                }
            }
        }

        // If async mode, return immediately
        if (!waitForConn) {
            ETH_LOG_I("Ethernet PHY started asynchronously after %lu ms", millis() - startTime);
            return true;  // MutexGuard auto-releases
        }
    }  // MutexGuard released here before long wait

    // Wait for connection (outside mutex to not block other operations)
    ETH_LOG_D("Waiting for connection...");
    if (!waitForConnection(ETH_INIT_TIMEOUT_MS)) {
        ETH_LOG_E("Ethernet connection timeout after %lu ms", millis() - startTime);
        return false;
    }

    ETH_LOG_I("Ethernet Manager initialized successfully after %lu ms", millis() - startTime);
    ETH_TIME_END("internalInit");  // Log total initialization time
    return true;
}

void EthernetManager::cleanup() {
    ETH_LOG_D("Cleaning up Ethernet Manager");
    auto& inst = getInstance();

    // Take mutex for cleanup
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_STANDARD_TIMEOUT_MS));
    if (!guard) {
        ETH_LOG_E("Failed to take mutex for cleanup");
        return;
    }

    // Unregister event handlers if registered
    if (inst.eventHandlersRegistered) {
        esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, onEthEvent);
        esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, onEthEvent);
        inst.eventHandlersRegistered = false;
    }

    inst.phyStarted = false;
    inst.gotIpAtLeastOnce = false;
    inst.hasCustomMac = false;
    inst.netifCreated = false;
    inst.eth_netif = nullptr;
    inst.lastGotIpTime = 0;
    inst.connectionStartTime = 0;

    if (inst.ethEventGroup) {
        vEventGroupDelete(inst.ethEventGroup);
        inst.ethEventGroup = nullptr;
        ETH_LOG_D("EventGroup deleted");
    }

    // Delete timers if they exist
    if (inst.reconnectTimer) {
        xTimerDelete(inst.reconnectTimer, 0);
        inst.reconnectTimer = nullptr;
    }

    if (inst.linkMonitorTimer) {
        xTimerDelete(inst.linkMonitorTimer, 0);
        inst.linkMonitorTimer = nullptr;
    }

    // Clear callbacks
    inst.connectedCallback = nullptr;
    inst.disconnectedCallback = nullptr;
    inst.stateChangeCallback = nullptr;
    inst.linkStatusCallback = nullptr;

    // Reset state
    inst.connectionState = EthConnectionState::UNINITIALIZED;
    inst.previousState = EthConnectionState::UNINITIALIZED;

    // Reset statistics
    memset(&inst.stats, 0, sizeof(inst.stats));
    inst.lastError = EthError::OK;

    // Note: Don't delete the mutex as it's managed by the singleton destructor
    guard.unlock();
}

void EthernetManager::disconnect() {
    auto& inst = getInstance();
    ETH_LOG_I("Disconnecting from Ethernet...");

    uint32_t connectedDuration = 0;

    // Disconnect operations protected by mutex
    {
        MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_STANDARD_TIMEOUT_MS));
        if (!guard) {
            ETH_LOG_E("Failed to take mutex for disconnect");
            return;
        }

        // Only proceed if we're connected or connecting
        if (inst.connectionState == EthConnectionState::UNINITIALIZED) {
            ETH_LOG_W("Cannot disconnect - not initialized");
            return;  // MutexGuard auto-releases
        }

        // Record disconnection time if we were connected
        if (inst.connectionState == EthConnectionState::CONNECTED && inst.connectionStartTime > 0) {
            connectedDuration = millis() - inst.connectionStartTime;
            inst.stats.disconnectCount++;
        }

        // Update state
        inst.changeState(EthConnectionState::DISCONNECTING);

        // Stop the PHY if it was started
        if (inst.phyStarted && ETH.linkUp()) {
            ETH_LOG_D("Stopping Ethernet PHY...");
            // Note: ESP32 Arduino ETH doesn't have a stop() method, but we can simulate disconnect
            // by disabling auto-reconnect and clearing state
        }

        // Disable auto-reconnect
        if (inst.autoReconnectEnabled) {
            inst.autoReconnectEnabled = false;
            if (inst.reconnectTimer) {
                xTimerStop(inst.reconnectTimer, 0);
            }
        }

        // Stop link monitoring
        if (inst.linkMonitorTimer) {
            xTimerStop(inst.linkMonitorTimer, 0);
        }

        // Clear connection state
        inst.phyStarted = false;
        inst.gotIpAtLeastOnce = false;
        inst.connectionStartTime = 0;

        // Update final state
        inst.changeState(EthConnectionState::UNINITIALIZED);
    }  // MutexGuard released here

    // Call disconnected callback outside mutex (callbacks may take time)
    if (inst.disconnectedCallback && connectedDuration > 0) {
        inst.disconnectedCallback(connectedDuration);
    }

    ETH_LOG_I("Ethernet disconnected");
}

EthResult<void> EthernetManager::waitForConnection(uint32_t timeout_ms) {
    auto& inst = getInstance();

    // Validate timeout
    if (timeout_ms == 0) {
        ETH_LOG_E("Invalid timeout: 0 ms");
        inst.lastError = EthError::INVALID_PARAMETER;
        return EthResult<void>(EthError::INVALID_PARAMETER);
    }

    // Quick check first
    if (isConnected()) {
        return EthResult<void>::ok();
    }

    if (!inst.ethEventGroup) {
        ETH_LOG_E("Event group not initialized");
        inst.lastError = EthError::NOT_INITIALIZED;
        return EthResult<void>(EthError::NOT_INITIALIZED);
    }

    if (!inst.phyStarted) {
        ETH_LOG_E("Ethernet PHY not started");
        inst.lastError = EthError::NOT_INITIALIZED;
        return EthResult<void>(EthError::NOT_INITIALIZED);
    }

    ETH_LOG_D("Waiting for Ethernet connection (max %ums)...", timeout_ms);

    // Track start time for timeout calculation
    uint32_t startWait = millis();

    // Wait in chunks to allow for state updates and link monitoring
    const uint32_t WAIT_CHUNK_MS = ETH_WAIT_CHUNK_MS;
    while ((millis() - startWait) < timeout_ms) {
        uint32_t remainingTime = timeout_ms - (millis() - startWait);
        uint32_t waitTime = (remainingTime > WAIT_CHUNK_MS) ? WAIT_CHUNK_MS : remainingTime;

        EventBits_t result = xEventGroupWaitBits(inst.ethEventGroup, BIT_CONNECTED,
                                                 pdFALSE,  // do not clear
                                                 pdTRUE,   // wait for all bits
                                                 pdMS_TO_TICKS(waitTime));

        if (result & BIT_CONNECTED) {
            return EthResult<void>::ok();
        }

        // Check for error state
        if (inst.connectionState == EthConnectionState::ERROR_STATE) {
            ETH_LOG_E("Connection failed - in error state");
            inst.lastError = EthError::CONNECTION_TIMEOUT;
            return EthResult<void>(EthError::CONNECTION_TIMEOUT);
        }

        // Update link status during wait
        if (inst.linkMonitoringEnabled) {
            inst.updateLinkStatus();
        }
    }

    ETH_LOG_W("Connection timeout after %u ms", millis() - startWait);
    inst.lastError = EthError::CONNECTION_TIMEOUT;
    return EthResult<void>(EthError::CONNECTION_TIMEOUT);
}

bool EthernetManager::internalInitStatic(const char* hostname, IPAddress local_ip, IPAddress gateway,
                                        IPAddress subnet, IPAddress dns1, IPAddress dns2,
                                        int8_t phy_addr, int8_t mdc_pin, int8_t mdio_pin,
                                        int8_t power_pin, eth_clock_mode_t clock_mode) {
    ETH_LOG_D("Initializing Ethernet with static IP configuration");

    // Validate parameters (no mutex needed)
    if (!hostname || strlen(hostname) == 0) {
        ETH_LOG_E("Invalid hostname (null or empty)");
        return false;
    }

    if (!local_ip) {
        ETH_LOG_E("Invalid local IP address");
        return false;
    }

    if (!gateway) {
        ETH_LOG_E("Invalid gateway address");
        return false;
    }

    if (!subnet) {
        ETH_LOG_E("Invalid subnet mask");
        return false;
    }

    // Validate hardware parameters
    if (phy_addr < 0 || phy_addr > ETH_MAX_PHY_ADDRESS) {
        ETH_LOG_E("Invalid PHY address: %d (must be 0-%d)", phy_addr, ETH_MAX_PHY_ADDRESS);
        return false;
    }

    if (mdc_pin < -1 || mdc_pin >= GPIO_NUM_MAX) {
        ETH_LOG_E("Invalid MDC pin: %d", mdc_pin);
        return false;
    }

    if (mdio_pin < -1 || mdio_pin >= GPIO_NUM_MAX) {
        ETH_LOG_E("Invalid MDIO pin: %d", mdio_pin);
        return false;
    }

    if (power_pin < -1 || power_pin >= GPIO_NUM_MAX) {
        ETH_LOG_E("Invalid power pin: %d", power_pin);
        return false;
    }

    // Measure initialization time
    unsigned long startTime = millis();
    (void)startTime; // Suppress unused warning when logging is disabled

    // Early initialization - ensure it's been called (earlyInit handles its own mutex)
    if (!mutexCreated || !ethEventGroup || !eventHandlersRegistered) {
        if (!earlyInit()) {
            return false;
        }
    }

    // Main initialization section - protected by mutex
    {
        MutexGuard guard(ethMutex, pdMS_TO_TICKS(ETH_MUTEX_INIT_TIMEOUT_MS));
        if (!guard) {
            ETH_LOG_E("Failed to take mutex for static init");
            return false;
        }

        xEventGroupClearBits(ethEventGroup, BIT_CONNECTED);

        ETH_LOG_D("Starting ETH.begin with static IP at %lu ms", millis() - startTime);

        // Start Ethernet PHY
        bool success = false;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        // Arduino 3.x+ version
        // Pre-configure hostname
        if (hostname) {
            ETH.setHostname(hostname);
        }

        // Start Ethernet
        success = ETH.begin(ETH_PHY_LAN8720, phy_addr, mdc_pin, mdio_pin, power_pin, clock_mode);

        if (success) {
            // Configure static IP with optional custom MAC
            if (hasCustomMac) {
                success = ETH.config(local_ip, gateway, subnet, dns1, dns2, customMacAddress);
            } else {
                success = ETH.config(local_ip, gateway, subnet, dns1, dns2);
            }

            if (!success) {
                ETH_LOG_E("Failed to configure static IP");
            } else {
                ETH_LOG_I("Static IP configured: %s", local_ip.toString().c_str());
            }
        }
#else
        // Arduino 2.x version
        success = ETH.begin(power_pin, mdc_pin, mdio_pin, phy_addr, ETH_PHY_LAN8720, clock_mode);

        if (success) {
            if (hostname) {
                ETH.setHostname(hostname);
            }

            // Configure static IP
            if (hasCustomMac) {
                success = ETH.config(local_ip, gateway, subnet, dns1, customMacAddress);
            } else {
                success = ETH.config(local_ip, gateway, subnet, dns1);
            }

            if (!success) {
                ETH_LOG_E("Failed to configure static IP");
            } else {
                ETH_LOG_I("Static IP configured: %s", local_ip.toString().c_str());
            }
        }
#endif

        if (!success) {
            ETH_LOG_E("ETH.begin or static config failed after %lu ms", millis() - startTime);
            return false;  // MutexGuard auto-releases
        }

        phyStarted = true;
        ETH_LOG_D("ETH.begin with static IP completed after %lu ms", millis() - startTime);

        // Store netif handle for later use
        eth_netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
        if (eth_netif) {
            netifCreated = true;
        }
    }  // MutexGuard released here before long wait

    // Wait for connection (outside mutex to not block other operations)
    ETH_LOG_D("Waiting for connection with static IP...");
    if (!waitForConnection(ETH_INIT_TIMEOUT_MS)) {
        ETH_LOG_E("Ethernet connection timeout after %lu ms", millis() - startTime);
        return false;
    }

    ETH_LOG_I("Ethernet Manager initialized with static IP successfully after %lu ms", millis() - startTime);
    return true;
}

bool EthernetManager::isConnected() {
    auto& inst = getInstance();
    // Quick atomic check without mutex for performance
    // Use cache-friendly ordering: check simple booleans first
    if (!inst.phyStarted || !inst.ethEventGroup) {
        return false;
    }

    // Only check event bits if prerequisites are met
    return (xEventGroupGetBits(inst.ethEventGroup) & BIT_CONNECTED) != 0;
}

void EthernetManager::setMacAddress(const uint8_t* mac) {
    if (!mac) {
        ETH_LOG_E("Null MAC address provided");
        return;
    }
    auto& inst = getInstance();

    // Take mutex for thread safety
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(inst.mutexTimeout));
    if (!guard) {
        ETH_LOG_E("Failed to take mutex for MAC address");
        return;
    }

    memcpy(inst.customMacAddress, mac, ETH_MAC_ADDRESS_SIZE);
    inst.hasCustomMac = true;
    ETH_LOG_I("MAC address set for next initialization: %02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void EthernetManager::logEthernetStatus() {
    auto& inst = getInstance();
    // Take mutex for consistent read
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (!guard) {
        ETH_LOG_E("Failed to take mutex for status");
        return;
    }

    if (!inst.phyStarted) {
        ETH_LOG_I("Ethernet PHY not started");
        return;
    }

    if (inst.verboseLogging) {
        // Verbose mode: detailed multi-line output
        ETH_LOG_I("Ethernet Status:");
        ETH_LOG_I("  IP     : %s", ETH.localIP().toString().c_str());
        ETH_LOG_I("  MAC    : %s", ETH.macAddress().c_str());
        ETH_LOG_I("  Host   : %s", ETH.getHostname());
        ETH_LOG_I("  Speed  : %d Mbps", ETH.linkSpeed());
        ETH_LOG_I("  Duplex : %s", ETH.fullDuplex() ? "FULL" : "HALF");
    } else {
        // Compact mode: single line summary at INFO level
        ETH_LOG_I("Connected: IP=%s, Link=%dMbps/%s",
                  ETH.localIP().toString().c_str(),
                  ETH.linkSpeed(),
                  ETH.fullDuplex() ? "Full" : "Half");
        // Additional details at DEBUG level
        ETH_LOG_D("MAC=%s, Host=%s", ETH.macAddress().c_str(), ETH.getHostname());
    }
}

void EthernetManager::setConnectedCallback(EthConnectedCallback callback) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        inst.connectedCallback = callback;
    }
}

void EthernetManager::setDisconnectedCallback(EthDisconnectedCallback callback) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        inst.disconnectedCallback = callback;
    }
}

void EthernetManager::setStateChangeCallback(EthStateChangeCallback callback) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        inst.stateChangeCallback = callback;
    }
}

void EthernetManager::setLinkStatusCallback(EthLinkStatusCallback callback) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        inst.linkStatusCallback = callback;
    }
}

NetworkStats EthernetManager::getStatistics() {
    auto& inst = getInstance();
    NetworkStats currentStats = {0};
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        currentStats = inst.stats;
        if (isConnected() && inst.stats.connectTime > 0) {
            currentStats.uptimeMs = millis() - inst.stats.connectTime;
        }
    }
    return currentStats;
}

void EthernetManager::resetStatistics() {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        memset(&inst.stats, 0, sizeof(inst.stats));
    }
}

void EthernetManager::setAutoReconnect(bool enable, uint8_t maxRetries,
                                      uint32_t initialDelayMs, uint32_t maxDelayMs) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        inst.autoReconnectEnabled = enable;
        inst.reconnectMaxRetries = maxRetries;
        inst.reconnectInitialDelay = initialDelayMs;
        inst.reconnectMaxDelay = maxDelayMs;
        inst.reconnectAttempts = 0;
        inst.reconnectCurrentDelay = initialDelayMs;

        // Create timer if needed
        if (enable && !inst.reconnectTimer) {
            inst.reconnectTimer = xTimerCreate("EthReconnect", pdMS_TO_TICKS(initialDelayMs),
                                        pdFALSE, nullptr, attemptReconnect);
        }
    }
}

const char* EthernetManager::errorToString(EthError error) {
    switch (error) {
        case EthError::OK: return "OK";
        case EthError::INVALID_PARAMETER: return "Invalid parameter";
        case EthError::MUTEX_TIMEOUT: return "Mutex timeout";
        case EthError::ALREADY_INITIALIZED: return "Already initialized";
        case EthError::NOT_INITIALIZED: return "Not initialized";
        case EthError::PHY_START_FAILED: return "PHY start failed";
        case EthError::CONFIG_FAILED: return "Configuration failed";
        case EthError::CONNECTION_TIMEOUT: return "Connection timeout";
        case EthError::EVENT_HANDLER_FAILED: return "Event handler failed";
        case EthError::MEMORY_ALLOCATION_FAILED: return "Memory allocation failed";
        case EthError::NETIF_ERROR: return "Network interface error";
        case EthError::UNKNOWN_ERROR: return "Unknown error";
        default: return "Invalid error code";
    }
}

const char* EthernetManager::stateToString(EthConnectionState state) {
    switch (state) {
        case EthConnectionState::UNINITIALIZED: return "Uninitialized";
        case EthConnectionState::PHY_STARTING: return "PHY Starting";
        case EthConnectionState::LINK_DOWN: return "Link Down";
        case EthConnectionState::LINK_UP: return "Link Up";
        case EthConnectionState::OBTAINING_IP: return "Obtaining IP";
        case EthConnectionState::CONNECTED: return "Connected";
        case EthConnectionState::DISCONNECTING: return "Disconnecting";
        case EthConnectionState::ERROR_STATE: return "Error";
        default: return "Unknown";
    }
}

void EthernetManager::dumpDiagnostics(Print* output) {
    if (!output) return;

    auto& inst = getInstance();

    output->println("=== Ethernet Manager Diagnostics ===");
    output->print("Connection State: ");
    output->println(stateToString(inst.connectionState));
    output->print("Previous State: ");
    output->println(stateToString(inst.previousState));
    output->print("PHY Started: ");
    output->println(inst.phyStarted ? "Yes" : "No");
    output->print("Last Error: ");
    output->println(errorToString(inst.lastError));

    if (inst.phyStarted) {
        output->print("IP Address: ");
        output->println(ETH.localIP().toString());
        output->print("MAC Address: ");
        output->println(ETH.macAddress());
        output->print("Hostname: ");
        output->println(ETH.getHostname());
        output->print("Link Speed: ");
        output->print(ETH.linkSpeed());
        output->println(" Mbps");
        output->print("Full Duplex: ");
        output->println(ETH.fullDuplex() ? "Yes" : "No");
    }

    NetworkStats currentStats = getStatistics();
    output->println("\n--- Network Statistics ---");
    output->print("Uptime: ");
    output->print(currentStats.uptimeMs / 1000);
    output->println(" seconds");
    output->print("Disconnections: ");
    output->println(currentStats.disconnectCount);
    output->print("Reconnections: ");
    output->println(currentStats.reconnectCount);
    output->print("Link Down Events: ");
    output->println(currentStats.linkDownEvents);

    output->println("\n--- Configuration ---");
    output->print("Auto Reconnect: ");
    output->println(inst.autoReconnectEnabled ? "Enabled" : "Disabled");
    if (inst.autoReconnectEnabled) {
        output->print("Max Retries: ");
        output->println(inst.reconnectMaxRetries == 0 ? "Infinite" : String(inst.reconnectMaxRetries));
        output->print("Current Attempts: ");
        output->println(inst.reconnectAttempts);
        output->print("Current Delay: ");
        output->print(inst.reconnectCurrentDelay);
        output->println(" ms");
    }

    output->println("=================================");
}

void EthernetManager::updateStatistics() {
    auto& inst = getInstance();
    // This method is called from event handlers, mutex should already be held
    // Update statistics based on netif if available
    if (inst.eth_netif) {
        // Note: Actual packet statistics would need esp_netif_get_stats()
        // which may not be available in all ESP-IDF versions
    }
}

void EthernetManager::attemptReconnect(TimerHandle_t xTimer) {
    auto& inst = getInstance();
    ETH_LOG_I("Attempting to reconnect...");

    {
        MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
        if (guard) {
            inst.reconnectAttempts++;

            // Check if we've exceeded max retries
            if (inst.reconnectMaxRetries > 0 && inst.reconnectAttempts > inst.reconnectMaxRetries) {
                ETH_LOG_E("Max reconnection attempts reached");
                inst.autoReconnectEnabled = false;
                return;
            }
        }
    }

    // Attempt to restart Ethernet
    if (inst.phyStarted) {
        // In ESP32 Arduino, we can't easily restart ETH
        // So we'll just wait for the link to come back up
        ETH_LOG_D("Waiting for link to restore...");
    }

    // Schedule next attempt with exponential backoff
    {
        MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
        if (guard) {
            inst.reconnectCurrentDelay = min(inst.reconnectCurrentDelay * 2, inst.reconnectMaxDelay);
            if (inst.reconnectTimer) {
                xTimerChangePeriod(inst.reconnectTimer, pdMS_TO_TICKS(inst.reconnectCurrentDelay), 0);
                xTimerStart(inst.reconnectTimer, 0);
            }
        }
    }
}

void EthernetManager::onEthEvent(void* arg, esp_event_base_t base, int32_t id, void* data) {
    auto& inst = getInstance();

    // Validate event parameters
    if (!base) {
        ETH_LOG_E("Null event base");
        return;
    }

    if (!inst.ethEventGroup) {
        ETH_LOG_W("Event group not initialized, ignoring event");
        return;
    }

    // Track event count for performance metrics
    inst.totalEventCount++;

    ETH_LOG_D("Event: base='%s', id=%d at %lu ms", base, id, millis());

    if (base == IP_EVENT && id == IP_EVENT_GOT_IP) {
        ETH_LOG_I("IP_EVENT_GOT_IP received");
        inst.gotIpAtLeastOnce = true;
        inst.lastGotIpTime = millis();

        // Update statistics
        inst.stats.connectTime = millis();
        if (inst.stats.disconnectCount > 0) {
            inst.stats.reconnectCount++;
        }
        inst.reconnectAttempts = 0;
        inst.reconnectCurrentDelay = inst.reconnectInitialDelay;

        // Update state to connected
        inst.changeState(EthConnectionState::CONNECTED);

        xEventGroupSetBits(inst.ethEventGroup, BIT_CONNECTED);
        logEthernetStatus();

        // Call user callback if set
        if (inst.connectedCallback) {
            inst.connectedCallback(ETH.localIP());
        }

        return;
    }

    if (base == ETH_EVENT) {
        switch (id) {
            case ETHERNET_EVENT_START:
                ETH_LOG_D("ETH Started at %lu ms", millis());
                // Don't set hostname here - already done in begin()
                break;

            case ETHERNET_EVENT_CONNECTED:
                ETH_LOG_D("ETH Connected at %lu ms", millis());
                inst.connectionStartTime = millis();
                // Update state to obtaining IP (link is up but no IP yet)
                inst.changeState(EthConnectionState::OBTAINING_IP);
                inst.updateLinkStatus();
                break;

            case ETHERNET_EVENT_DISCONNECTED:
            case ETHERNET_EVENT_STOP: {
                unsigned long now = millis();

                if (!inst.gotIpAtLeastOnce) {
                    ETH_LOG_W("Ignoring disconnect: no IP was ever assigned");
                    break;
                }

                if (now - inst.connectionStartTime < ETH_CONNECTION_TRUST_WINDOW_MS) {
                    ETH_LOG_W("Ignoring disconnect within trust window (%lu ms)",
                                 now - inst.connectionStartTime);
                    break;
                }

                ETH_LOG_E("ETH Disconnected after stable window");
                inst.gotIpAtLeastOnce = false;
                xEventGroupClearBits(inst.ethEventGroup, BIT_CONNECTED);

                // Update state to link down
                inst.changeState(EthConnectionState::LINK_DOWN);

                // Update statistics
                inst.stats.disconnectCount++;
                inst.stats.linkDownEvents++;
                uint32_t connectionDuration = millis() - inst.stats.connectTime;

                // Call user callback if set
                if (inst.disconnectedCallback) {
                    inst.disconnectedCallback(connectionDuration);
                }

                // Start auto-reconnect if enabled
                if (inst.autoReconnectEnabled && inst.reconnectTimer) {
                    xTimerStart(inst.reconnectTimer, 0);
                }

                break;
            }

            default:
                ETH_LOG_W("Unhandled ETH event ID: %d", id);
                break;
        }
    }
}

void EthernetManager::setLinkMonitoring(bool enable, uint32_t intervalMs) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        inst.linkMonitoringEnabled = enable;
        inst.linkMonitoringInterval = intervalMs;

        if (enable) {
            // Create timer if needed
            if (!inst.linkMonitorTimer) {
                inst.linkMonitorTimer = xTimerCreate("LinkMonitor", pdMS_TO_TICKS(intervalMs),
                                              pdTRUE, nullptr, linkMonitorTask);
            }

            if (inst.linkMonitorTimer) {
                xTimerChangePeriod(inst.linkMonitorTimer, pdMS_TO_TICKS(intervalMs), 0);
                xTimerStart(inst.linkMonitorTimer, 0);
                ETH_LOG_I("Link monitoring enabled with %u ms interval", intervalMs);
            }
        } else {
            // Stop timer if running
            if (inst.linkMonitorTimer) {
                xTimerStop(inst.linkMonitorTimer, 0);
                ETH_LOG_I("Link monitoring disabled");
            }
        }
    }
}

bool EthernetManager::checkLinkStatus() {
    auto& inst = getInstance();
    if (!inst.phyStarted) return false;

    // Get current link status from ETH
    bool linkUp = ETH.linkUp();

    // Update our tracking
    inst.updateLinkStatus();

    return linkUp;
}

bool EthernetManager::getNetworkInterfaceStats(uint32_t& txPackets, uint32_t& rxPackets,
                                              uint32_t& txErrors, uint32_t& rxErrors) {
    auto& inst = getInstance();
    txPackets = rxPackets = txErrors = rxErrors = 0;

    if (!inst.eth_netif) {
        ETH_LOG_E("Network interface not available");
        return false;
    }

    // Note: ESP-IDF doesn't provide easy access to detailed netif stats
    // This is a placeholder for future enhancement when API is available
    // For now, return our tracked stats
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_QUICK_TIMEOUT_MS));
    if (guard) {
        txPackets = inst.stats.txPackets;
        rxPackets = inst.stats.rxPackets;
    }

    return true;
}

bool EthernetManager::configurePHY(bool autoNegotiate, uint8_t speed, bool fullDuplex) {
    auto& inst = getInstance();
    if (!inst.phyStarted) {
        ETH_LOG_E("PHY not started");
        return false;
    }

    // Note: ESP32 Arduino framework doesn't expose direct PHY configuration
    // This would require lower-level ESP-IDF calls
    ETH_LOG_W("PHY configuration not implemented in Arduino framework");
    return false;
}

void EthernetManager::linkMonitorTask(TimerHandle_t xTimer) {
    getInstance().updateLinkStatus();
}

void EthernetManager::changeState(EthConnectionState newState) {
    if (newState == connectionState) return;
    
    previousState = connectionState;
    connectionState = newState;
    
    ETH_LOG_I("State change: %s -> %s", 
                stateToString(previousState), stateToString(newState));
    
    // Call state change callback if set
    if (stateChangeCallback) {
        stateChangeCallback(previousState, newState);
    }
    
    // Track timing for performance monitoring
    switch (newState) {
        case EthConnectionState::PHY_STARTING:
            initStartTime = millis();
            break;
        case EthConnectionState::LINK_UP:
            linkUpTime = millis();
            ETH_LOG_D("Link up after %lu ms", linkUpTime - initStartTime);
            break;
        case EthConnectionState::CONNECTED:
            ipObtainedTime = millis();
            ETH_LOG_D("IP obtained after %lu ms (link up: %lu ms)", 
                         ipObtainedTime - initStartTime, linkUpTime - initStartTime);
            break;
        default:
            break;
    }
}

bool EthernetManager::updateLinkStatus() {
    if (!phyStarted) return false;
    
    bool currentLinkStatus = ETH.linkUp();
    
    if (currentLinkStatus != lastLinkStatus) {
        lastLinkStatus = currentLinkStatus;
        ETH_LOG_I("Link status changed: %s", currentLinkStatus ? "UP" : "DOWN");
        
        // Update connection state based on link status
        if (!currentLinkStatus) {
            changeState(EthConnectionState::LINK_DOWN);
            stats.linkDownEvents++;
        } else {
            // Link is up, but we might not have IP yet
            if (isConnected()) {
                changeState(EthConnectionState::CONNECTED);
            } else {
                changeState(EthConnectionState::LINK_UP);
            }
        }
        
        // Call link status callback if set
        if (linkStatusCallback) {
            linkStatusCallback(currentLinkStatus);
        }
    }
    
    return currentLinkStatus;
}

bool EthernetManager::resetInterface() {
    auto& inst = getInstance();
    ETH_LOG_I("Resetting Ethernet interface");

    if (!inst.phyStarted) {
        ETH_LOG_E("Cannot reset - PHY not started");
        return false;
    }

    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(ETH_MUTEX_STANDARD_TIMEOUT_MS));
    if (!guard) {
        ETH_LOG_E("Failed to take mutex for reset");
        return false;
    }

    // Clear connected state
    xEventGroupClearBits(inst.ethEventGroup, BIT_CONNECTED);
    inst.gotIpAtLeastOnce = false;

    // Update state
    inst.changeState(EthConnectionState::LINK_DOWN);

    // Note: ESP32 Arduino doesn't provide a way to reset ETH without full restart
    // This is a limitation of the framework
    ETH_LOG_W("Full interface reset not available in Arduino framework");

    // We can at least try to trigger reconnection
    if (inst.autoReconnectEnabled && inst.reconnectTimer) {
        inst.reconnectAttempts = 0;
        inst.reconnectCurrentDelay = inst.reconnectInitialDelay;
        xTimerStart(inst.reconnectTimer, 0);
    }

    return true;
}

bool EthernetManager::setDNSServers(IPAddress dns1, IPAddress dns2) {
    auto& inst = getInstance();
    if (!inst.eth_netif) {
        ETH_LOG_E("Network interface not initialized");
        return false;
    }

    esp_netif_dns_info_t dns_info;

    // Set primary DNS
    if (dns1) {
        dns_info.ip.u_addr.ip4.addr = dns1;
        dns_info.ip.type = ESP_IPADDR_TYPE_V4;

        if (esp_netif_set_dns_info(inst.eth_netif, ESP_NETIF_DNS_MAIN, &dns_info) != ESP_OK) {
            ETH_LOG_E("Failed to set primary DNS");
            return false;
        }
        ETH_LOG_I("Primary DNS set to %s", dns1.toString().c_str());
    }

    // Set secondary DNS if provided
    if (dns2) {
        dns_info.ip.u_addr.ip4.addr = dns2;
        dns_info.ip.type = ESP_IPADDR_TYPE_V4;

        if (esp_netif_set_dns_info(inst.eth_netif, ESP_NETIF_DNS_BACKUP, &dns_info) != ESP_OK) {
            ETH_LOG_E("Failed to set secondary DNS");
            return false;
        }
        ETH_LOG_I("Secondary DNS set to %s", dns2.toString().c_str());
    }

    return true;
}

String EthernetManager::getUptimeString() {
    uint32_t uptimeMs = getUptimeMs();
    if (uptimeMs == 0) return "Not connected";
    
    uint32_t seconds = uptimeMs / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    String result;
    if (days > 0) {
        result += String(days) + "d ";
    }
    if (hours > 0 || days > 0) {
        result += String(hours) + "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        result += String(minutes) + "m ";
    }
    result += String(seconds) + "s";
    
    return result;
}

bool EthernetManager::getQuickStatus(IPAddress& ip, int& linkSpeed, bool& fullDuplex) {
    if (!isConnected()) {
        return false;
    }
    
    ip = ETH.localIP();
    linkSpeed = ETH.linkSpeed();
    fullDuplex = ETH.fullDuplex();
    
    return true;
}

void EthernetManager::configurePerformance(bool enableEventBatching, uint32_t mutexTimeoutMs,
                                          uint8_t eventQueueSize) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(mutexTimeoutMs));
    if (guard) {
        inst.eventBatchingEnabled = enableEventBatching;
        inst.mutexTimeout = mutexTimeoutMs;
        inst.maxEventQueueSize = eventQueueSize;

        if (enableEventBatching) {
            // Create event queue if not exists
            if (!inst.eventQueue) {
                inst.eventQueue = xQueueCreate(inst.maxEventQueueSize, sizeof(uint32_t));
                if (!inst.eventQueue) {
                    ETH_LOG_E("Failed to create event queue");
                    inst.eventBatchingEnabled = false;
                }
            }

            // Create batch timer if not exists
            if (!inst.eventBatchTimer && inst.eventQueue) {
                inst.eventBatchTimer = xTimerCreate("EventBatch", pdMS_TO_TICKS(ETH_EVENT_BATCH_WINDOW_MS),
                                             pdFALSE, nullptr, processBatchedEvents);
                if (!inst.eventBatchTimer) {
                    ETH_LOG_E("Failed to create event batch timer");
                    inst.eventBatchingEnabled = false;
                }
            }

            if (inst.eventBatchingEnabled) {
                ETH_LOG_I("Event batching enabled with queue size %u", inst.maxEventQueueSize);
            }
        } else {
            // Delete queue and timer if disabling
            if (inst.eventQueue) {
                vQueueDelete(inst.eventQueue);
                inst.eventQueue = nullptr;
            }
            if (inst.eventBatchTimer) {
                xTimerDelete(inst.eventBatchTimer, 0);
                inst.eventBatchTimer = nullptr;
            }
            ETH_LOG_I("Event batching disabled");
        }
    }
}

bool EthernetManager::getPerformanceMetrics(uint32_t& initTimeMs, uint32_t& linkUpTimeMs,
                                           uint32_t& ipObtainTimeMs, uint32_t& eventCount) {
    auto& inst = getInstance();
    if (inst.initStartTime == 0) {
        return false; // No initialization has occurred
    }

    initTimeMs = inst.linkUpTime > 0 ? (inst.linkUpTime - inst.initStartTime) : 0;
    linkUpTimeMs = inst.ipObtainedTime > 0 && inst.linkUpTime > 0 ? (inst.ipObtainedTime - inst.linkUpTime) : 0;
    ipObtainTimeMs = inst.ipObtainedTime > 0 ? (inst.ipObtainedTime - inst.initStartTime) : 0;
    eventCount = inst.totalEventCount;

    return true;
}

void EthernetManager::processBatchedEvents(TimerHandle_t xTimer) {
    auto& inst = getInstance();
    if (!inst.eventQueue) return;

    uint32_t eventData;
    uint8_t processedCount = 0;

    // Process all queued events
    while (xQueueReceive(inst.eventQueue, &eventData, 0) == pdTRUE && processedCount < inst.maxEventQueueSize) {
        // Events are processed in batch, reducing callback overhead
        processedCount++;
    }

    if (processedCount > 0) {
        ETH_LOG_D("Processed %u batched events", processedCount);
    }
}

void EthernetManager::queueEvent(esp_event_base_t base, int32_t id, void* data) {
    if (!eventBatchingEnabled || !eventQueue) {
        return; // Process immediately if batching disabled
    }

    // Queue event for batch processing
    uint32_t eventData = (uint32_t)id;
    if (xQueueSend(eventQueue, &eventData, 0) == pdTRUE) {
        // Start/restart batch timer
        if (eventBatchTimer) {
            xTimerReset(eventBatchTimer, 0);
        }
    } else {
        ETH_LOG_W("Event queue full, processing immediately");
    }
}

void EthernetManager::setVerboseLogging(bool enable) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(inst.mutexTimeout));
    if (guard) {
        inst.verboseLogging = enable;
        ETH_LOG_D("Verbose logging %s", enable ? "enabled" : "disabled");
    }
}

void EthernetManager::setStatusLogLevel(esp_log_level_t level) {
    auto& inst = getInstance();
    MutexGuard guard(inst.ethMutex, pdMS_TO_TICKS(inst.mutexTimeout));
    if (guard) {
        inst.statusLogLevel = level;
        ETH_LOG_D("Status log level set to %d", level);
    }
}