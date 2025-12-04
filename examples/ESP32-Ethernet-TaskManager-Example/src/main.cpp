// main.cpp
#include <Arduino.h>

#include "config/ProjectConfig.h"

// Include our libraries
#include <EthernetManager.h>

// Include utility functions
#include "utils/StatusLed.h"

// Include tasks
#include "tasks/MonitoringTask.h"

// Include external libraries
#include <Logger.h>
#include <LogInterfaceImpl.h>  // Provides the custom logger implementation
#include <SemaphoreGuard.h>
#include <TaskManagerConfig.h>  // Use TaskManager config header instead of direct include

// Global objects
Logger logger;
TaskManager taskManager;

// System state
static volatile bool shutdownRequested = false;
static bool ethernetConnected = false;

// Function prototypes
bool setupEthernet();
void printSystemInfo();
void handleShutdown();
void checkSerialCommands();

// Network event callbacks
void onEthernetConnected(IPAddress ip);
void onEthernetDisconnected(uint32_t duration);
void onEthernetStateChange(EthConnectionState oldState, EthConnectionState newState);

// Main setup function
void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);  // Give serial monitor time to connect

    // Print welcome message
    Serial.println();
    Serial.println("==============================");
    Serial.println("  ESP32 Ethernet Project");
    Serial.println("==============================");
    Serial.println("Initializing...");

    // Initialize logger with buffer size
    logger.init(1024);  // Provide buffer size parameter
    logger.enableLogging(true);
    logger.setLogLevel(ESP_LOG_DEBUG);

    // Initialize status LED if enabled
#ifdef ENABLE_STATUS_LED
    StatusLed::init(STATUS_LED_PIN);
    StatusLed::setBlink(100);  // Fast blink during initialization
#endif

    // Initialize watchdog through TaskManager
    // Note: Arduino framework may have already initialized it
    if (!taskManager.initWatchdog(WATCHDOG_TIMEOUT_SECONDS, true)) {
        LOG_WARN(LOG_TAG_MAIN, "Watchdog initialization returned false, but may still be usable");
    } else {
        LOG_INFO(LOG_TAG_MAIN, "Watchdog initialized with %d second timeout",
                 WATCHDOG_TIMEOUT_SECONDS);
    }
    
    // Early init for EthernetManager to ensure event handlers are ready
    EthernetManager::earlyInit();
    
    // Set up network event callbacks
    EthernetManager::setConnectedCallback(onEthernetConnected);
    EthernetManager::setDisconnectedCallback(onEthernetDisconnected);
    EthernetManager::setStateChangeCallback(onEthernetStateChange);
    
    // Enable auto-reconnect with exponential backoff
    EthernetManager::setAutoReconnect(true, 10, 1000, 30000); // 10 retries, 1s initial, 30s max

    // Small delay to let watchdog initialize
    delay(100);

    // Initialize monitoring task
    if (!MonitoringTask::init()) {
        LOG_ERROR(LOG_TAG_MAIN, "Failed to initialize monitoring task");
    }

    // Start monitoring task
    if (!MonitoringTask::start()) {
        LOG_ERROR(LOG_TAG_MAIN, "Failed to start monitoring task");
    }

    if (!setupEthernet()) {
        LOG_WARN(LOG_TAG_MAIN, "Ethernet setup failed - OTA will not start unless reconnected");
    }
    
    // Initialize connection state after setup
    ethernetConnected = EthernetManager::isConnected();

// Initialize task manager debug monitoring if enabled
#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    // Enable resource monitoring with 30 second log period
    taskManager.setResourceLogPeriod(30000);
    // Enable leak detection for debugging
    taskManager.enableLeakDetection(true);
#endif

    // Register the main loop task with watchdog
    // Note: Arduino framework may have already registered loopTask with ESP-IDF watchdog
    // TaskManager will handle this gracefully and return true if already registered
    if (!taskManager.registerCurrentTaskWithWatchdog("loopTask", 
                                                   TaskManager::WatchdogConfig::enabled(true, 5000))) {
        LOG_WARN(LOG_TAG_MAIN, "Failed to register loopTask with watchdog");
    } else {
        LOG_INFO(LOG_TAG_MAIN, "loopTask registered with watchdog");
    }

    LOG_INFO(LOG_TAG_MAIN, "Setup complete - all tasks started");
    LOG_INFO(LOG_TAG_MAIN, "Hostname: %s", DEVICE_HOSTNAME);

    // Log initial watchdog statistics
    taskManager.logWatchdogStats();
}

// Initialize Ethernet
bool setupEthernet() {
    LOG_INFO(LOG_TAG_MAIN, "Initializing Ethernet");

// Set custom MAC address if defined
#ifdef ETH_MAC_ADDRESS
    uint8_t mac[] = ETH_MAC_ADDRESS;

// Handle different APIs in ESP32 Arduino 2.x vs 3.x
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    // Arduino 3.x+ version with eth_phy_type_t as first parameter and MAC as last parameter
    if (!ETH.begin(ETH_PHY_LAN8720, ETH_PHY_ADDR, ETH_PHY_MDC_PIN, ETH_PHY_MDIO_PIN,
                   ETH_PHY_POWER_PIN, ETH_CLOCK_MODE, mac)) {
        LOG_ERROR(LOG_TAG_MAIN, "ETH.begin with custom MAC failed");
        return false;
    }
#else
    // Older Arduino 2.x version
    ETH.begin(ETH_PHY_POWER_PIN, ETH_PHY_MDC_PIN, ETH_PHY_MDIO_PIN, ETH_PHY_ADDR, ETH_PHY_LAN8720,
              ETH_CLOCK_MODE);
    // Set MAC address separately
    if (!ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE, mac)) {
        LOG_ERROR(LOG_TAG_MAIN, "ETH.config with custom MAC failed");
        // Continue anyway - MAC address setting is not critical
    }
#endif

    LOG_INFO(LOG_TAG_MAIN, "Using custom MAC address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5]);
#else
    // Initialize Ethernet with default MAC
    if (!EthernetManager::initialize(DEVICE_HOSTNAME, ETH_PHY_ADDR, ETH_PHY_MDC_PIN,
                                     ETH_PHY_MDIO_PIN, ETH_PHY_POWER_PIN, ETH_CLOCK_MODE)) {
        LOG_ERROR(LOG_TAG_MAIN, "Failed to initialize Ethernet");
        return false;
    }
#endif

    // Wait for Ethernet connection to establish
    LOG_INFO(LOG_TAG_MAIN, "Waiting for Ethernet connection...");
    if (EthernetManager::waitForConnection(ETH_CONNECTION_TIMEOUT_MS)) {
        // Connection callback will handle LED and logging
        return true;
    } else {
        LOG_WARN(LOG_TAG_MAIN, "Failed to connect to Ethernet within timeout");
#ifdef ENABLE_STATUS_LED
        // Set LED to fast blink pattern to indicate connection issue
        StatusLed::setPattern(2, 100, 1000);  // 2 fast blinks, then pause
#endif
        return false;
    }
}

// Main loop function - very simple since we're using tasks
void loop() {
    // Check for shutdown request
    if (shutdownRequested) {
        handleShutdown();
        return; // Should never reach here after shutdown
    }
    
    // Feed the watchdog for loopTask
    taskManager.feedWatchdog();
    
    // Update status LED if enabled
#ifdef ENABLE_STATUS_LED
    StatusLed::update();
#endif
    
    // Check serial commands
    checkSerialCommands();

    static unsigned long lastWatchdogStats = 0;
    static unsigned long lastSystemInfoTime = 0;
    static unsigned long lastConnectionCheck = 0;
    static unsigned long bootTime = millis();
    static bool printedUptime = false;

    // Print uptime info after 1 minute
    if (!printedUptime && (millis() - bootTime > 60000)) {
        printedUptime = true;
        unsigned long uptime = millis() / 1000;  // seconds
        LOG_INFO(LOG_TAG_MAIN, "System running for %lu seconds", uptime);
    }

    // Log watchdog statistics periodically
    if (millis() - lastWatchdogStats > 60000) {
        lastWatchdogStats = millis();
        taskManager.logWatchdogStats();
    }

    if (millis() - lastSystemInfoTime > 300000) {
        lastSystemInfoTime = millis();
        printSystemInfo();
    }
    
    // Periodic connection check (every 10 seconds)
    if (millis() - lastConnectionCheck > 10000) {
        lastConnectionCheck = millis();
        bool connected = EthernetManager::isConnected();
        if (connected != ethernetConnected) {
            LOG_WARN(LOG_TAG_MAIN, "Connection state mismatch detected");
            ethernetConnected = connected;
        }
    }

    // Small delay to prevent watchdog issues
    delay(10);
}

// Print system information
void printSystemInfo() {
    LOG_INFO(LOG_TAG_MAIN, "--- System Information ---");
    LOG_INFO(LOG_TAG_MAIN, "Uptime: %lu seconds", millis() / 1000);
    LOG_INFO(LOG_TAG_MAIN, "Free heap: %u bytes", ESP.getFreeHeap());
    LOG_INFO(LOG_TAG_MAIN, "Hostname: %s", DEVICE_HOSTNAME);

    if (EthernetManager::isConnected()) {
        LOG_INFO(LOG_TAG_MAIN, "Ethernet connected - IP: %s", ETH.localIP().toString().c_str());
    } else {
        LOG_INFO(LOG_TAG_MAIN, "Ethernet not connected");
    }

    // Add watchdog statistics here
    LOG_INFO(LOG_TAG_MAIN, "--- Watchdog Statistics ---");
    taskManager.logWatchdogStats();

    LOG_INFO(LOG_TAG_MAIN, "-------------------------");
}

// Network event callbacks
void onEthernetConnected(IPAddress ip) {
    ethernetConnected = true;
    LOG_INFO(LOG_TAG_MAIN, "=== ETHERNET CONNECTED ===");
    LOG_INFO(LOG_TAG_MAIN, "IP Address: %s", ip.toString().c_str());
    EthernetManager::logEthernetStatus();
    
#ifdef ENABLE_STATUS_LED
    // Set LED to solid on when connected
    StatusLed::setOn();
#endif
}

void onEthernetDisconnected(uint32_t duration) {
    ethernetConnected = false;
    LOG_WARN(LOG_TAG_MAIN, "=== ETHERNET DISCONNECTED ===");
    LOG_WARN(LOG_TAG_MAIN, "Was connected for %lu seconds", duration / 1000);
    
#ifdef ENABLE_STATUS_LED
    // Set LED to pattern to indicate disconnection
    StatusLed::setPattern(2, 100, 1000);  // 2 fast blinks, then pause
#endif
}

void onEthernetStateChange(EthConnectionState oldState, EthConnectionState newState) {
    LOG_INFO(LOG_TAG_MAIN, "Ethernet state change: %s -> %s",
             EthernetManager::stateToString(oldState),
             EthernetManager::stateToString(newState));
    
#ifdef ENABLE_STATUS_LED
    // Update LED based on state
    switch (newState) {
        case EthConnectionState::CONNECTED:
            StatusLed::setOn();
            break;
        case EthConnectionState::OBTAINING_IP:
        case EthConnectionState::LINK_UP:
            StatusLed::setBlink(500); // Medium blink
            break;
        case EthConnectionState::PHY_STARTING:
            StatusLed::setBlink(100); // Fast blink
            break;
        case EthConnectionState::LINK_DOWN:
        case EthConnectionState::ERROR_STATE:
            StatusLed::setPattern(2, 100, 1000);
            break;
        default:
            StatusLed::setOff();
            break;
    }
#endif
}

void checkSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "shutdown" || command == "stop") {
            LOG_WARN(LOG_TAG_MAIN, "Shutdown requested via serial command");
            shutdownRequested = true;
        } else if (command == "status") {
            printSystemInfo();
        } else if (command == "reboot" || command == "restart") {
            LOG_WARN(LOG_TAG_MAIN, "Reboot requested via serial command");
            ESP.restart();
        } else if (command == "help" || command == "?") {
            Serial.println("\nAvailable commands:");
            Serial.println("  status   - Print system status");
            Serial.println("  shutdown - Gracefully shutdown system");
            Serial.println("  reboot   - Restart ESP32");
            Serial.println("  help     - Show this help\n");
        }
    }
}

void handleShutdown() {
    LOG_WARN(LOG_TAG_MAIN, "=== SYSTEM SHUTDOWN INITIATED ===");
    
#ifdef ENABLE_STATUS_LED
    // Turn off LED
    StatusLed::setOff();
#endif
    
    // Stop monitoring task
    LOG_INFO(LOG_TAG_MAIN, "Stopping monitoring task...");
    TaskHandle_t monitoringHandle = taskManager.getTaskHandleByName("MonitoringTask");
    if (monitoringHandle != nullptr) {
        taskManager.stopTask(monitoringHandle);
    }
    
    // Unregister main loop from watchdog
    LOG_INFO(LOG_TAG_MAIN, "Unregistering from watchdog...");
    taskManager.unregisterCurrentTaskFromWatchdog();
    
    // Clean up network resources
    LOG_INFO(LOG_TAG_MAIN, "Disconnecting Ethernet...");
    EthernetManager::disconnect();
    
    // Final message
    LOG_WARN(LOG_TAG_MAIN, "=== SHUTDOWN COMPLETE ===");
    LOG_WARN(LOG_TAG_MAIN, "System halted. Reset to restart.");
    
    // Infinite loop to halt
    while (true) {
        delay(1000);
    }
}