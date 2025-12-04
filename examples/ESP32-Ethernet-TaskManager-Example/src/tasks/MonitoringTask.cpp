// MonitoringTask.cpp
#include "../tasks/MonitoringTask.h"

#include <SemaphoreGuard.h>
#include <TaskManagerConfig.h>
#include <esp_system.h>

extern TaskManager taskManager;  // Ensure global TaskManager is accessible

// Initialize static members
// TaskHandle_t removed - not needed when using TaskManager

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

    // Use TaskManager to create the task with watchdog support
    bool result = taskManager.startTask(
        taskFunction,                           // Task function
        "MonitoringTask",                       // Task name
        STACK_SIZE_MONITORING_TASK,             // Stack size
        nullptr,                                // Parameters
        PRIORITY_MONITORING_TASK,               // Priority
        TaskManager::WatchdogConfig::enabled(true, 5000)  // Enable watchdog with 5s interval
    );

    if (!result) {
        LOG_ERROR(LOG_TAG_MONITORING, "Failed to create monitoring task");
        return false;
    }

    LOG_INFO(LOG_TAG_MONITORING, "Monitoring task started successfully");
    return true;
}

// MonitoringTask::taskFunction
void MonitoringTask::taskFunction(void* pvParameters) {
    LOG_INFO(LOG_TAG_MONITORING, "Monitoring task started and running");

    // Watchdog is already configured by TaskManager when the task was created
    // Just need to register from our own context
    if (taskManager.registerCurrentTaskWithWatchdog("MonitoringTask", 
                                                  TaskManager::WatchdogConfig::enabled(true, 5000))) {
        wdtRegistered = true;
        LOG_INFO(LOG_TAG_MONITORING, "Successfully registered with watchdog from task context");
    } else {
        // This might fail if already registered during task creation
        // Try to feed anyway to check if we're registered
        if (taskManager.feedWatchdog()) {
            wdtRegistered = true;
            LOG_INFO(LOG_TAG_MONITORING, "Watchdog already registered, feeding successful");
        } else {
            wdtRegistered = false;
            LOG_ERROR(LOG_TAG_MONITORING, "Failed to register with watchdog");
        }
    }

    // Task loop
    for (;;) {
        if (wdtRegistered) {
            if (!taskManager.feedWatchdog()) {
                LOG_ERROR(LOG_TAG_MONITORING, "Failed to feed watchdog");
            }
        }

        // Log system health information
        logSystemHealth();

        // Log network status
        logNetworkStatus();

        // Delay with periodic watchdog feeds
        const int segments = 10;  // Split the delay into segments
        const int delayPerSegment = MONITORING_TASK_INTERVAL_MS / segments;

        for (int i = 0; i < segments; i++) {
            vTaskDelay(pdMS_TO_TICKS(delayPerSegment));
            if (wdtRegistered) taskManager.feedWatchdog();
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
