// MonitoringTask.cpp
#include "../tasks/MonitoringTask.h"

#include <SemaphoreGuard.h>
#include <TaskManager.h>
#include <esp_system.h>

extern TaskManager taskManager;  // Ensure global TaskManager is accessible

// Initialize static members
TaskHandle_t MonitoringTask::taskHandle = nullptr;

// File-scope flag to track watchdog registration
static bool wdtRegistered = false;

bool MonitoringTask::init() {
    LOG_INFO(LOG_TAG_MONITORING, "Initializing monitoring task");

    // Add any initialization code here

    LOG_INFO(LOG_TAG_MONITORING, "Monitoring task initialized successfully");
    return true;
}

bool MonitoringTask::start() {
    LOG_INFO(LOG_TAG_MONITORING, "Starting monitoring task");

    // Create the FreeRTOS task
    BaseType_t result = xTaskCreate(taskFunction,                // Task function
                                    "MonitoringTask",            // Task name
                                    STACK_SIZE_MONITORING_TASK,  // Stack size
                                    nullptr,                     // Parameters
                                    PRIORITY_MONITORING_TASK,    // Priority
                                    &taskHandle                  // Task handle
    );

    if (result != pdPASS) {
        LOG_ERROR(LOG_TAG_MONITORING, "Failed to create monitoring task");
        return false;
    }

    LOG_INFO(LOG_TAG_MONITORING, "Monitoring task started successfully");
    return true;
}

// MonitoringTask::taskFunction
void MonitoringTask::taskFunction(void* pvParameters) {
    LOG_DEBUG(LOG_TAG_MONITORING, "Monitoring task running");

    // Wait before registering
    vTaskDelay(pdMS_TO_TICKS(500));

    wdtRegistered = taskManager.registerCurrentTaskWithWatchdog(
        "MonitoringTask", TaskManager::WatchdogConfig::enabled(false, 10000)  // 10s
    );

    // Task loop
    for (;;) {
        if (wdtRegistered) esp_task_wdt_reset();

        // Log system health information
        logSystemHealth();

        // Log network status
        logNetworkStatus();

        // Log sensor data
        logSensorData();

        // Delay with periodic watchdog feeds
        const int segments = 10;  // Split the delay into segments
        const int delayPerSegment = MONITORING_TASK_INTERVAL_MS / segments;

        for (int i = 0; i < segments; i++) {
            vTaskDelay(pdMS_TO_TICKS(delayPerSegment));
            if (wdtRegistered) esp_task_wdt_reset();
        }
    }
}

void MonitoringTask::logSystemHealth() {
    // Get free heap memory
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t heapSize = ESP.getHeapSize();
    float heapPercent = (float)freeHeap / heapSize * 100.0;

    // Get minimum free heap since boot
    uint32_t minFreeHeap = ESP.getMinFreeHeap();

    // Get uptime
    uint32_t uptime = millis() / 1000;  // seconds
    uint32_t days = uptime / (24 * 3600);
    uptime %= (24 * 3600);
    uint32_t hours = uptime / 3600;
    uptime %= 3600;
    uint32_t minutes = uptime / 60;
    uint32_t seconds = uptime % 60;

    // Get chip info
    uint32_t chipId = ESP.getEfuseMac() & 0xFFFFFFFF;
    uint8_t chipRev = ESP.getChipRevision();

    // Log the information
    LOG_INFO(LOG_TAG_MONITORING, "System Health Report:");
    LOG_INFO(LOG_TAG_MONITORING, "  Uptime: %u days, %02u:%02u:%02u", days, hours, minutes,
             seconds);
    LOG_INFO(LOG_TAG_MONITORING, "  Free Heap: %u bytes (%.1f%%)", freeHeap, heapPercent);
    LOG_INFO(LOG_TAG_MONITORING, "  Min Free Heap: %u bytes", minFreeHeap);
    LOG_INFO(LOG_TAG_MONITORING, "  Chip: ID=0x%08X, Rev=%u", chipId, chipRev);

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    char* taskStatusBuffer = (char*)malloc(2048);
    if (taskStatusBuffer) {
        vTaskList(taskStatusBuffer);
        LOG_INFO(LOG_TAG_MONITORING, "Task Status:");
        LOG_INFO(LOG_TAG_MONITORING, "%s", taskStatusBuffer);
        free(taskStatusBuffer);

        // Get runtime statistics
        taskStatusBuffer = (char*)malloc(2048);
        if (taskStatusBuffer) {
            vTaskGetRunTimeStats(taskStatusBuffer);
            LOG_INFO(LOG_TAG_MONITORING, "CPU Usage:");
            LOG_INFO(LOG_TAG_MONITORING, "%s", taskStatusBuffer);
            free(taskStatusBuffer);
        }
    }
#endif
}

void MonitoringTask::logNetworkStatus() {
    if (EthernetManager::isConnected()) {
        // Log Ethernet status directly using the EthernetManager
        EthernetManager::logEthernetStatus();
    } else {
        LOG_INFO(LOG_TAG_MONITORING, "Ethernet is not connected");
    }
}

void MonitoringTask::logSensorData() {
    // Get sensor readings
    float temp = SensorTask::getTemperature();
    float humidity = SensorTask::getHumidity();

    // Log sensor data
    LOG_INFO(LOG_TAG_MONITORING, "Sensor Data:");
    LOG_INFO(LOG_TAG_MONITORING, "  Temperature: %.1f°C", temp);
    LOG_INFO(LOG_TAG_MONITORING, "  Humidity: %.1f%%", humidity);

    // Feed watchdog periodically during long operations
    esp_task_wdt_reset();
}
