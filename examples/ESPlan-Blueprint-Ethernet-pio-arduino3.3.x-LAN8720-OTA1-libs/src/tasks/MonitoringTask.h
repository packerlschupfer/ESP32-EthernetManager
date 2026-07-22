// MonitoringTask.h
#pragma once

#include <Arduino.h>
#include <EthernetManager.h>

#include "../config/ProjectConfig.h"
#include "../tasks/SensorTask.h"

/**
 * @brief Task for monitoring system health and status
 */
class MonitoringTask {
   public:
    /**
     * @brief Initialize the monitoring task
     *
     * @return true if initialization was successful
     */
    static bool init();

    /**
     * @brief Start the monitoring task
     *
     * @return true if task started successfully
     */
    static bool start();

    /**
     * @brief FreeRTOS task function
     *
     * @param pvParameters Task parameters (not used)
     */
    static void taskFunction(void* pvParameters);

    // Task handle exposed for watchdog monitoring
    static TaskHandle_t taskHandle;

   private:
    /**
     * @brief Log system health information (memory, uptime, etc.)
     */
    static void logSystemHealth();

    /**
     * @brief Log network status information
     */
    static void logNetworkStatus();

    /**
     * @brief Log sensor data information
     */
    static void logSensorData();
};