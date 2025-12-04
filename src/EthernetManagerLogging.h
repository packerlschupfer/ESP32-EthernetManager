#ifndef ETHERNET_MANAGER_LOGGING_H
#define ETHERNET_MANAGER_LOGGING_H

#define ETH_LOG_TAG "ETH"

// Define log levels based on debug flag
#ifdef ETHERNETMANAGER_DEBUG
    // Debug mode: Show all levels
    #define ETH_LOG_LEVEL_E ESP_LOG_ERROR
    #define ETH_LOG_LEVEL_W ESP_LOG_WARN
    #define ETH_LOG_LEVEL_I ESP_LOG_INFO
    #define ETH_LOG_LEVEL_D ESP_LOG_DEBUG
    #define ETH_LOG_LEVEL_V ESP_LOG_VERBOSE
#else
    // Release mode: Only Error, Warn, Info
    #define ETH_LOG_LEVEL_E ESP_LOG_ERROR
    #define ETH_LOG_LEVEL_W ESP_LOG_WARN
    #define ETH_LOG_LEVEL_I ESP_LOG_INFO
    #define ETH_LOG_LEVEL_D ESP_LOG_NONE  // Suppress
    #define ETH_LOG_LEVEL_V ESP_LOG_NONE  // Suppress
#endif

// Route to custom logger or ESP-IDF
#ifdef USE_CUSTOM_LOGGER
    #include <LogInterface.h>
    #define ETH_LOG_E(...) LOG_WRITE(ETH_LOG_LEVEL_E, ETH_LOG_TAG, __VA_ARGS__)
    #define ETH_LOG_W(...) LOG_WRITE(ETH_LOG_LEVEL_W, ETH_LOG_TAG, __VA_ARGS__)
    #define ETH_LOG_I(...) LOG_WRITE(ETH_LOG_LEVEL_I, ETH_LOG_TAG, __VA_ARGS__)
    #define ETH_LOG_D(...) LOG_WRITE(ETH_LOG_LEVEL_D, ETH_LOG_TAG, __VA_ARGS__)
    #define ETH_LOG_V(...) LOG_WRITE(ETH_LOG_LEVEL_V, ETH_LOG_TAG, __VA_ARGS__)
#else
    // ESP-IDF logging with compile-time suppression
    #include <esp_log.h>
    #define ETH_LOG_E(...) ESP_LOGE(ETH_LOG_TAG, __VA_ARGS__)
    #define ETH_LOG_W(...) ESP_LOGW(ETH_LOG_TAG, __VA_ARGS__)
    #define ETH_LOG_I(...) ESP_LOGI(ETH_LOG_TAG, __VA_ARGS__)
    #ifdef ETHERNETMANAGER_DEBUG
        #define ETH_LOG_D(...) ESP_LOGD(ETH_LOG_TAG, __VA_ARGS__)
        #define ETH_LOG_V(...) ESP_LOGV(ETH_LOG_TAG, __VA_ARGS__)
    #else
        #define ETH_LOG_D(...) ((void)0)
        #define ETH_LOG_V(...) ((void)0)
    #endif
#endif

// Feature-specific debug helpers
#ifdef ETHERNETMANAGER_DEBUG
    // Timing macros for performance debugging
    #define ETH_TIME_START() unsigned long _eth_start = millis()
    #define ETH_TIME_END(msg) ETH_LOG_D("Timing: %s took %lu ms", msg, millis() - _eth_start)
    
    // Hex dump for buffer debugging
    #define ETH_DUMP_BUFFER(msg, buf, len) do { \
        ETH_LOG_D("%s (%d bytes):", msg, len); \
        for (int i = 0; i < len; i++) { \
            ETH_LOG_D("  [%02d] = 0x%02X", i, buf[i]); \
        } \
    } while(0)
#else
    #define ETH_TIME_START() ((void)0)
    #define ETH_TIME_END(msg) ((void)0)
    #define ETH_DUMP_BUFFER(msg, buf, len) ((void)0)
#endif

#endif // ETHERNET_MANAGER_LOGGING_H