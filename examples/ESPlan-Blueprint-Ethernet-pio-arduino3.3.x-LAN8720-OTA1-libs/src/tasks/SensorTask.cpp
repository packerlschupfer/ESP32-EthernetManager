// SensorTask.cpp
#include "../tasks/SensorTask.h"

#include <SemaphoreGuard.h>
#include <TaskManager.h>  // Add this include

extern TaskManager taskManager;

// Initialize static members
TaskHandle_t SensorTask::taskHandle = nullptr;
SemaphoreHandle_t SensorTask::dataMutex = nullptr;
float SensorTask::temperature = 0.0f;
float SensorTask::humidity = 0.0f;
bool SensorTask::suspended = false;  // Initialize the suspended flag
static bool wdtRegistered = false;

bool SensorTask::init() {
    LOG_INFO(LOG_TAG_SENSOR, "Initializing sensor task");

    // Create mutex for thread-safe data access
    dataMutex = xSemaphoreCreateMutex();
    if (!dataMutex) {
        LOG_ERROR(LOG_TAG_SENSOR, "Failed to create data mutex");
        return false;
    }

    // Initialize sensor hardware here
    // This is a placeholder for actual sensor initialization
    // Example: if (dht.begin() == false) { return false; }

    LOG_INFO(LOG_TAG_SENSOR, "Sensor task initialized successfully");
    return true;
}

bool SensorTask::start() {
    LOG_INFO(LOG_TAG_SENSOR, "Starting sensor task");

    // Create the FreeRTOS task
    BaseType_t result = xTaskCreate(taskFunction,            // Task function
                                    "SensorTask",            // Task name
                                    STACK_SIZE_SENSOR_TASK,  // Stack size
                                    nullptr,                 // Parameters
                                    PRIORITY_SENSOR_TASK,    // Priority
                                    &taskHandle              // Task handle
    );

    if (result != pdPASS) {
        LOG_ERROR(LOG_TAG_SENSOR, "Failed to create sensor task");
        return false;
    }

    LOG_INFO(LOG_TAG_SENSOR, "Sensor task started successfully");
    return true;
}

void SensorTask::taskFunction(void* pvParameters) {
    LOG_DEBUG(LOG_TAG_SENSOR, "Sensor task running");

    // Initial delay to allow system to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Register with TaskManager and configure watchdog
    wdtRegistered = taskManager.registerCurrentTaskWithWatchdog(
        "SensorTask", TaskManager::WatchdogConfig::enabled(false, 5000)  // non-critical, 5s
    );

    // Task loop
    for (;;) {
        esp_task_wdt_reset();  // Always feed the watchdog

        bool isSuspended = false;

        // Check if we're suspended using mutex for thread safety
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            isSuspended = suspended;
            xSemaphoreGive(dataMutex);
        }

        if (!isSuspended) {
            // Read sensor data
            readSensors();

            // Log sensor readings
            LOG_INFO(LOG_TAG_SENSOR, "Temperature: %.1f°C, Humidity: %.1f%%", temperature,
                     humidity);
        } else {
            LOG_DEBUG(LOG_TAG_SENSOR, "Sensor readings suspended");
        }

        // Delay with periodic watchdog feeds
        const int segments = 5;
        const int delayPerSegment = SENSOR_TASK_INTERVAL_MS / segments;

        for (int i = 0; i < segments; i++) {
            vTaskDelay(pdMS_TO_TICKS(delayPerSegment));
            esp_task_wdt_reset();
        }
    }
}

float SensorTask::getTemperature() {
    float value = 0.0f;

    // Use mutex for thread-safe access
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        value = temperature;
        xSemaphoreGive(dataMutex);
    }

    return value;
}

float SensorTask::getHumidity() {
    float value = 0.0f;

    // Use mutex for thread-safe access
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        value = humidity;
        xSemaphoreGive(dataMutex);
    }

    return value;
}

void SensorTask::suspend() {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        suspended = true;
        xSemaphoreGive(dataMutex);
    }
    LOG_INFO(LOG_TAG_SENSOR, "Sensor readings suspended");
}

void SensorTask::resume() {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        suspended = false;
        xSemaphoreGive(dataMutex);
    }
    LOG_INFO(LOG_TAG_SENSOR, "Sensor readings resumed");
}

void SensorTask::readSensors() {
    // Feed watchdog before potentially lengthy operation
    if (wdtRegistered) esp_task_wdt_reset();

    // This is a simulation - replace with actual sensor reading code
    // Example:
    // float newTemp = dht.readTemperature();
    // float newHumid = dht.readHumidity();

    // Simulate temperature between 20 and 25°C with minor variations
    float newTemp = 22.5f + (random(0, 50) - 25) / 10.0f;

    // Simulate humidity between 40% and 60% with minor variations
    float newHumid = 50.0f + (random(0, 200) - 100) / 10.0f;

    // Check if readings are valid (not NaN)
    if (isnan(newTemp) || isnan(newHumid)) {
        LOG_ERROR(LOG_TAG_SENSOR, "Failed to read sensor data");
        return;
    }

    // Use mutex for thread-safe access
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        temperature = newTemp;
        humidity = newHumid;
        xSemaphoreGive(dataMutex);
    } else {
        LOG_ERROR(LOG_TAG_SENSOR, "Failed to acquire mutex for updating sensor data");
    }

    // Feed watchdog after operation
    if (wdtRegistered) esp_task_wdt_reset();
}
