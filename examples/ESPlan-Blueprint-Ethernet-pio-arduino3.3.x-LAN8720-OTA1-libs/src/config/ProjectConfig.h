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

// OTA Settings
#ifndef OTA_PASSWORD
#define OTA_PASSWORD "update-password"  // Set your OTA password here
#endif

#ifndef OTA_PORT
#define OTA_PORT 3232
#endif

// Status LED
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 2  // Onboard LED on most ESP32 dev boards
#endif

// Task Settings
#define STACK_SIZE_OTA_TASK 4096
#define STACK_SIZE_MONITORING_TASK 4096
#define STACK_SIZE_SENSOR_TASK 4096

#define PRIORITY_OTA_TASK 1
#define PRIORITY_MONITORING_TASK 2
#define PRIORITY_SENSOR_TASK 3

// Task Intervals
#define OTA_TASK_INTERVAL_MS 250
#define MONITORING_TASK_INTERVAL_MS 5000
#define SENSOR_TASK_INTERVAL_MS 1000

// Watchdog Settings (used by TaskManager now)
#define WATCHDOG_TIMEOUT_SECONDS 30
#define WATCHDOG_MIN_HEAP_BYTES 10000  // Trigger reset if heap below this

// Log Tags
#define LOG_TAG_MAIN "MAIN"
#define LOG_TAG_OTA "OTA"
#define LOG_TAG_ETH "ETH"
#define LOG_TAG_MONITORING "MON"
#define LOG_TAG_SENSOR "SENS"

// Logging Macros - integrating with your external Logger lib
// These will be called directly by our code
#include <Logger.h>
extern Logger logger;

#define LOG_DEBUG(tag, format, ...) logger.log(ESP_LOG_DEBUG, tag, format, ##__VA_ARGS__)
#define LOG_INFO(tag, format, ...) logger.log(ESP_LOG_INFO, tag, format, ##__VA_ARGS__)
#define LOG_WARN(tag, format, ...) logger.log(ESP_LOG_WARN, tag, format, ##__VA_ARGS__)
#define LOG_ERROR(tag, format, ...) logger.log(ESP_LOG_ERROR, tag, format, ##__VA_ARGS__)

// DO NOT redefine ETH_LOG_* or OTA_LOG_* macros here, as they're already defined in the respective
// libraries Instead, just use LOG_* macros for project-specific logging