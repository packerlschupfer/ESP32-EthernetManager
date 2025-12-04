/**
 * EthernetManager with ESP-IDF Logging Example
 * 
 * This example demonstrates the default logging behavior of EthernetManager
 * using ESP-IDF logging without any custom Logger dependency.
 * 
 * This is the simplest way to use EthernetManager - no additional
 * configuration or dependencies are required.
 */

#include <Arduino.h>
#include <EthernetManager.h>

// Optional: Configure ESP-IDF log level
// #include <esp_log.h>
// void configureLogging() {
//     esp_log_level_set("*", ESP_LOG_INFO);
//     esp_log_level_set("ETH", ESP_LOG_DEBUG);  // Set EthernetManager to debug level
// }

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    
    Serial.println("\n=== EthernetManager with ESP-IDF Logging ===\n");
    
    // Optional: Configure ESP-IDF logging levels
    // configureLogging();
    
    // Create configuration using builder pattern
    EthernetConfig config = EthernetConfig()
        .withHostname("esp32-default-logging")
        .withAutoReconnect(true, 5, 1000, 30000);
    
    // Initialize Ethernet
    Serial.println("Initializing Ethernet...");
    
    EthResult<void> result = EthernetManager::initialize(config);
    
    if (!result.isOk()) {
        Serial.print("Ethernet initialization failed! Error: ");
        Serial.println(EthernetManager::errorToString(result.error));
        
        // All error logs are automatically sent to ESP-IDF logging
        // Check serial monitor for ESP_LOGE messages
        return;
    }
    
    Serial.println("Ethernet initialized successfully!");
    
    // Logs from EthernetManager will appear as:
    // E (12345) ETH: Error message
    // W (12346) ETH: Warning message
    // I (12347) ETH: Info message
    // D (12348) ETH: Debug message (if debug level enabled)
    
    EthernetManager::logEthernetStatus();
}

void loop() {
    static unsigned long lastCheck = 0;
    
    // Check connection status every 10 seconds
    if (millis() - lastCheck > 10000) {
        lastCheck = millis();
        
        if (EthernetManager::isConnected()) {
            Serial.println("Ethernet connected");
            
            // Show current network status
            NetworkStats stats = EthernetManager::getStatistics();
            Serial.print("Uptime: ");
            Serial.println(EthernetManager::getUptimeString());
            Serial.print("Disconnections: ");
            Serial.println(stats.disconnectCount);
        } else {
            Serial.println("Ethernet not connected");
        }
    }
    
    delay(100);
}