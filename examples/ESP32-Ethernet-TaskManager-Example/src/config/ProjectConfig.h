// ProjectConfig.h
#pragma once

#include <Arduino.h>
#include <esp_log.h>

// If not defined in platformio.ini, set defaults
#ifndef DEVICE_HOSTNAME
#define DEVICE_HOSTNAME "esp32-ethernet-device"
#endif

// Ethernet PHY Settings
#ifndef ETH_PHY_MDC_PIN
#define ETH_PHY_MDC_PIN 23
#endif

#ifndef ETH_PHY_MDIO_PIN
#define ETH_PHY_MDIO_PIN 18
#endif

#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR 0
#endif

#ifndef ETH_PHY_POWER_PIN
#define ETH_PHY_POWER_PIN -1  // No power pin
#endif

#define ETH_CLOCK_MODE ETH_CLOCK_GPIO17_OUT

// Optional custom MAC address (uncomment and set if needed)
// #define ETH_MAC_ADDRESS {0x02, 0xAB, 0xCD, 0xEF, 0x12, 0x34}

// Ethernet connection timeout
#define ETH_CONNECTION_TIMEOUT_MS 15000

// Reserved for future OTA example
// #ifndef OTA_PASSWORD
// #define OTA_PASSWORD "update-password"
// #endif
// 
// #ifndef OTA_PORT
// #define OTA_PORT 3232
// #endif

// Status LED (optional - comment out to disable LED support)
#define ENABLE_STATUS_LED  // Comment this line to disable LED completely
#ifdef ENABLE_STATUS_LED
    #ifndef STATUS_LED_PIN
    #define STATUS_LED_PIN 2  // Onboard LED on most ESP32 dev boards
    #endif
#endif

// Task Settings
#define STACK_SIZE_MONITORING_TASK 4096
#define STACK_SIZE_SENSOR_TASK 4096

#define PRIORITY_MONITORING_TASK 2
#define PRIORITY_SENSOR_TASK 3

// Task Intervals
#define MONITORING_TASK_INTERVAL_MS 5000
#define SENSOR_TASK_INTERVAL_MS 1000

// Watchdog Settings (used by TaskManager now)
#define WATCHDOG_TIMEOUT_SECONDS 30
#define WATCHDOG_MIN_HEAP_BYTES 10000  // Trigger reset if heap below this

// Log Tags
#define LOG_TAG_MAIN "MAIN"
#define LOG_TAG_ETH "ETH"
#define LOG_TAG_MONITORING "MON"
#define LOG_TAG_SENSOR "SENS"

// Logging Configuration
// Since we're using USE_CUSTOM_LOGGER, LogInterface.h provides the LOG_* macros
// We just need to ensure Logger is available for LogInterfaceImpl
#include <Logger.h>
extern Logger logger;

// The LOG_* macros are already defined in LogInterface.h which is included
// by EthernetManager and other libraries. No need to redefine them here.

// Configuration Validation (compile-time checks)
// Stack sizes must be at least 2048 bytes
static_assert(STACK_SIZE_MONITORING_TASK >= 2048, "Monitoring task stack size must be at least 2048 bytes");

// Task priorities must be within valid FreeRTOS range (0-24, with 0 being idle priority)
static_assert(PRIORITY_MONITORING_TASK >= 1 && PRIORITY_MONITORING_TASK <= 24, "Task priority must be between 1 and 24");

// Task intervals must be reasonable (not too fast, not too slow)
static_assert(MONITORING_TASK_INTERVAL_MS >= 100, "Monitoring interval must be at least 100ms");
static_assert(MONITORING_TASK_INTERVAL_MS <= 60000, "Monitoring interval should not exceed 60 seconds");

// Watchdog timeout must be reasonable
static_assert(WATCHDOG_TIMEOUT_SECONDS >= 5, "Watchdog timeout must be at least 5 seconds");
static_assert(WATCHDOG_TIMEOUT_SECONDS <= 300, "Watchdog timeout should not exceed 5 minutes");

// Ethernet connection timeout validation
static_assert(ETH_CONNECTION_TIMEOUT_MS >= 1000, "Ethernet connection timeout must be at least 1 second");
static_assert(ETH_CONNECTION_TIMEOUT_MS <= 60000, "Ethernet connection timeout should not exceed 60 seconds");