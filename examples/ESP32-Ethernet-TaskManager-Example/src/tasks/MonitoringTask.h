// MonitoringTask.h
#pragma once

#include <Arduino.h>
#include <EthernetManager.h>

#include "../config/ProjectConfig.h"

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

    // Task handle removed - TaskManager handles task management

   private:
    /**
     * @brief Log system health information (memory, uptime, etc.)
     */
    static void logSystemHealth();

    /**
     * @brief Log network status information
     */
    static void logNetworkStatus();

};