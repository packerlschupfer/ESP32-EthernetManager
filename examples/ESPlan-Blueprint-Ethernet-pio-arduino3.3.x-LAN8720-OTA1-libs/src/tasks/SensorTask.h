// SensorTask.h
#pragma once

#include <Arduino.h>

#include "../config/ProjectConfig.h"

/**
 * @brief Example task for reading sensors and collecting data
 */
class SensorTask {
   public:
    /**
     * @brief Initialize the sensor task
     *
     * @return true if initialization was successful
     */
    static bool init();

    /**
     * @brief Start the sensor task
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

    /**
     * @brief Get the latest temperature reading
     *
     * @return float Latest temperature in Celsius
     */
    static float getTemperature();

    /**
     * @brief Get the latest humidity reading
     *
     * @return float Latest humidity percentage
     */
    static float getHumidity();

    /**
     * @brief Suspend sensor readings (e.g., during OTA updates)
     */
    static void suspend();

    /**
     * @brief Resume sensor readings
     */
    static void resume();

    // Task handle exposed for watchdog monitoring
    static TaskHandle_t taskHandle;

   private:
    static SemaphoreHandle_t dataMutex;
    static bool suspended;

    // Sensor data
    static float temperature;
    static float humidity;

    /**
     * @brief Read sensor data (simulated in this example)
     */
    static void readSensors();
};