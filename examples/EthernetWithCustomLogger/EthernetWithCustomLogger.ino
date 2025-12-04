/**
 * EthernetManager with Custom Logger Example
 * 
 * This example demonstrates how to use EthernetManager with a custom Logger class
 * instead of the default ESP-IDF logging.
 * 
 * Requirements for using custom Logger:
 * 1. Include the Logger library in your project dependencies
 * 2. Define USE_CUSTOM_LOGGER in your build flags
 * 3. Include both Logger.h and LogInterfaceImpl.h in your main application
 * 
 * platformio.ini configuration:
 * [env:esp32dev]
 * platform = espressif32
 * board = esp32dev
 * framework = arduino
 * lib_deps = 
 *     Logger
 *     EthernetManager
 * build_flags = -DUSE_CUSTOM_LOGGER
 */

#include <Arduino.h>
#include <Logger.h>
#include <LogInterfaceImpl.h>  // Required ONCE in main application
#include <EthernetManager.h>

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    
    Serial.println("\n=== EthernetManager with Custom Logger Example ===\n");
    
    // Initialize the Logger (if required by your Logger implementation)
    // Logger::getInstance().begin();
    
    // Configure EthernetManager using the builder pattern
    EthernetConfig config = EthernetConfig()
        .withHostname("esp32-custom-logger")
        .withAutoReconnect(true, 0, 1000, 30000)
        .withLinkMonitoring(1000);
    
    // Initialize Ethernet
    Serial.println("Initializing Ethernet with custom logger...");
    
    EthResult<void> result = EthernetManager::initialize(config);
    
    if (!result.isOk()) {
        Serial.print("Ethernet initialization failed! Error: ");
        Serial.println(EthernetManager::errorToString(result.error));
        return;
    }
    
    Serial.println("Ethernet initialized successfully!");
    
    // The custom logger will now be used for all EthernetManager log output
    // Check your Logger configuration to see where logs are being sent
    // (Serial, SD card, network, etc.)
    
    EthernetManager::logEthernetStatus();
}

void loop() {
    static unsigned long lastCheck = 0;
    
    // Check connection status every 10 seconds
    if (millis() - lastCheck > 10000) {
        lastCheck = millis();
        
        if (EthernetManager::isConnected()) {
            Serial.println("Ethernet connected");
            
            // Get quick status
            IPAddress ip;
            int linkSpeed;
            bool fullDuplex;
            
            if (EthernetManager::getQuickStatus(ip, linkSpeed, fullDuplex)) {
                Serial.print("IP: ");
                Serial.print(ip);
                Serial.print(", Speed: ");
                Serial.print(linkSpeed);
                Serial.print(" Mbps, Duplex: ");
                Serial.println(fullDuplex ? "Full" : "Half");
            }
        } else {
            Serial.println("Ethernet not connected");
        }
    }
    
    // Your application code here
    
    delay(100);
}