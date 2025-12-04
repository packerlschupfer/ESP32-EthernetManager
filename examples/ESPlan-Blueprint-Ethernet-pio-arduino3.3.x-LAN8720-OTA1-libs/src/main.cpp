// main.cpp
#include <Arduino.h>

#include "config/ProjectConfig.h"

// Include our libraries
#include <EthernetManager.h>
#include <OTAManager.h>

// Include utility functions
#include "utils/StatusLed.h"

// Include tasks
#include "tasks/MonitoringTask.h"
#include "tasks/OTATask.h"
#include "tasks/SensorTask.h"

// Include external libraries
#include <Logger.h>
#include <SemaphoreGuard.h>
#include <TaskManager.h>

// Global objects
Logger logger;
TaskManager taskManager;

// Function prototypes
bool setupEthernet();
void printSystemInfo();

// Main setup function
void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);  // Give serial monitor time to connect

    // Print welcome message
    Serial.println();
    Serial.println("==============================");
    Serial.println("  ESP32 Ethernet OTA Project");
    Serial.println("==============================");
    Serial.println("Initializing...");

    // Initialize logger with buffer size
    logger.init(1024);  // Provide buffer size parameter
    logger.enableLogging(true);
    logger.setLogLevel(ESP_LOG_DEBUG);

    // Initialize status LED
    StatusLed::init(STATUS_LED_PIN);
    StatusLed::setBlink(100);  // Fast blink during initialization

    // Initialize watchdog through TaskManager
    // Note: Arduino framework may have already initialized it
    if (!taskManager.initWatchdog(WATCHDOG_TIMEOUT_SECONDS, true)) {
        LOG_WARN(LOG_TAG_MAIN, "Watchdog initialization returned false, but may still be usable");
    } else {
        LOG_INFO(LOG_TAG_MAIN, "Watchdog initialized with %d second timeout",
                 WATCHDOG_TIMEOUT_SECONDS);
    }

    // Small delay to let watchdog initialize
    delay(100);

    // Initialize sensor task
    if (!SensorTask::init()) {
        LOG_ERROR(LOG_TAG_MAIN, "Failed to initialize sensor task");
    }

    // Start sensor task
    if (!SensorTask::start()) {
        LOG_ERROR(LOG_TAG_MAIN, "Failed to start sensor task");
    }

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

// Initialize task manager for debugging if enabled
#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    TaskManager::WatchdogConfig debugWdtConfig = TaskManager::WatchdogConfig::disabled();
    taskManager.startTask(&TaskManager::debugTask, "DebugTask", 4096, 1, "DBG",
                          &taskManager.debugTaskHandle, debugWdtConfig);
    taskManager.setTaskExecutionInterval(5000);
    taskManager.setResourceLogPeriod(30000);
#endif

    // Register loopTask with watchdog (critical task, 10s interval)
    TaskManager::WatchdogConfig loopWdtConfig = TaskManager::WatchdogConfig::enabled(true, 10000);
    if (!taskManager.configureTaskWatchdog("loopTask", loopWdtConfig)) {
        LOG_WARN(LOG_TAG_MAIN, "Failed to configure watchdog for loopTask");
    } else {
        LOG_INFO(LOG_TAG_MAIN, "Watchdog configured for loopTask");
    }

    LOG_INFO(LOG_TAG_MAIN, "Setup complete - all tasks started");
    LOG_INFO(LOG_TAG_MAIN, "Hostname: %s", DEVICE_HOSTNAME);

    // Log initial watchdog statistics
    taskManager.logWatchdogStats();
}

// Initialize Ethernet and OTA
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
        LOG_INFO(LOG_TAG_MAIN, "Connected to Ethernet!");
        EthernetManager::logEthernetStatus();

        // Set LED to solid on when connected
        StatusLed::setOn();
        delay(1000);

        // Only initialize OTA after network is connected
        LOG_INFO(LOG_TAG_MAIN, "Initializing OTA task");
        if (!OTATask::init()) {
            LOG_ERROR(LOG_TAG_MAIN, "Failed to initialize OTA task");
            return false;
        }

        // Start OTA task
        if (!OTATask::start()) {
            LOG_ERROR(LOG_TAG_MAIN, "Failed to start OTA task");
            return false;
        } else {
            // Configure watchdog for OTA task (critical task with 2 second interval)
            TaskManager::WatchdogConfig otaWdtConfig =
                TaskManager::WatchdogConfig::enabled(true, 2000);
            if (!taskManager.configureTaskWatchdog("OTATask", otaWdtConfig)) {
                LOG_WARN(LOG_TAG_MAIN, "Failed to configure watchdog for OTATask");
            }
        }

        StatusLed::setBlink(1000);  // Back to normal blink
        return true;
    } else {
        LOG_WARN(LOG_TAG_MAIN, "Failed to connect to Ethernet within timeout");
        // Set LED to fast blink pattern to indicate connection issue
        StatusLed::setPattern(2, 100, 1000);  // 2 fast blinks, then pause
        return false;
    }
}

// Main loop function - very simple since we're using tasks
void loop() {
    // Update status LED
    StatusLed::update();

    static unsigned long lastWatchdogStats = 0;
    static unsigned long lastSystemInfoTime = 0;
    static unsigned long bootTime = millis();
    static bool printedUptime = false;

    // Print uptime info after 1 minute
    if (!printedUptime && (millis() - bootTime > 60000)) {
        printedUptime = true;
        unsigned long uptime = millis() / 1000;  // seconds
        LOG_INFO(LOG_TAG_MAIN, "System running for %lu seconds", uptime);
    }

    if (taskManager.isWatchdogInitialized() && millis() - lastWatchdogStats > 60000) {
        lastWatchdogStats = millis();
        taskManager.logWatchdogStats();

        // Add detailed stats for each task
        uint32_t missedFeeds, totalFeeds;

        // Check SensorTask
        if (taskManager.getTaskWatchdogStats("SensorTask", missedFeeds, totalFeeds)) {
            LOG_INFO(LOG_TAG_MAIN, "SensorTask watchdog: %u total feeds, %u missed", totalFeeds,
                     missedFeeds);
        }

        // Check MonitoringTask
        if (taskManager.getTaskWatchdogStats("MonitoringTask", missedFeeds, totalFeeds)) {
            LOG_INFO(LOG_TAG_MAIN, "MonitoringTask watchdog: %u total feeds, %u missed", totalFeeds,
                     missedFeeds);
        }

        // Check OTATask (if it's running)
        if (taskManager.getTaskWatchdogStats("OTATask", missedFeeds, totalFeeds)) {
            LOG_INFO(LOG_TAG_MAIN, "OTATask watchdog: %u total feeds, %u missed", totalFeeds,
                     missedFeeds);
        }
    }

    if (millis() - lastSystemInfoTime > 300000) {
        lastSystemInfoTime = millis();
        printSystemInfo();
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
    uint32_t missedFeeds, totalFeeds;

    if (taskManager.getTaskWatchdogStats("SensorTask", missedFeeds, totalFeeds)) {
        LOG_INFO(LOG_TAG_MAIN, "SensorTask: %u feeds, %u missed (%.1f%% success)", totalFeeds,
                 missedFeeds,
                 totalFeeds > 0 ? (100.0 * (totalFeeds - missedFeeds) / totalFeeds) : 0);
    }

    if (taskManager.getTaskWatchdogStats("MonitoringTask", missedFeeds, totalFeeds)) {
        LOG_INFO(LOG_TAG_MAIN, "MonitoringTask: %u feeds, %u missed (%.1f%% success)", totalFeeds,
                 missedFeeds,
                 totalFeeds > 0 ? (100.0 * (totalFeeds - missedFeeds) / totalFeeds) : 0);
    }

    LOG_INFO(LOG_TAG_MAIN, "-------------------------");
}