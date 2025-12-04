// BasicEthernetConnection.src
// Example of using the EthernetManager library

#include <Arduino.h>
#include <EthernetManager.h>

// EthernetManager uses a C++11 compatible logging pattern
// By default, logs go to ESP-IDF logging (ESP_LOGE, ESP_LOGW, etc.)
// No additional configuration needed for basic usage

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("EthernetManager Example - Basic Connection");
  
  // Initialize Ethernet with default settings
  if (!EthernetManager::initialize()) {
    Serial.println("Failed to initialize Ethernet");
    while (true) {
      delay(1000);
    }
  }
  
  Serial.println("Ethernet initialized successfully!");
  
  // Wait for the connection and IP address
  Serial.println("Waiting for IP address...");
  if (EthernetManager::waitForConnection(10000)) {
    Serial.println("Connected to Ethernet!");
    // Log Ethernet status
    EthernetManager::logEthernetStatus();
  } else {
    Serial.println("Failed to get IP address within timeout");
  }
}

void loop() {
  // Check connection status periodically
  static unsigned long lastCheck = 0;
  static bool wasConnected = false;
  
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    
    bool isConnected = EthernetManager::isConnected();
    
    if (isConnected && !wasConnected) {
      Serial.println("Ethernet is now connected!");
      EthernetManager::logEthernetStatus();
    } else if (!isConnected && wasConnected) {
      Serial.println("Ethernet connection lost!");
    }
    
    wasConnected = isConnected;
  }
  
  // Other application code goes here
  
  delay(10);
}
