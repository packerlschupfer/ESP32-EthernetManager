/**
 * Advanced EthernetManager Example
 * 
 * This example demonstrates:
 * - Event callbacks for connection/disconnection
 * - Auto-reconnect with exponential backoff
 * - Network statistics monitoring
 * - Error handling with detailed error codes
 * - Diagnostic information output
 */

#include <EthernetManager.h>

// Connection state tracking
bool wasConnected = false;
unsigned long lastStatsTime = 0;
const unsigned long STATS_INTERVAL = 30000; // Print stats every 30 seconds

// Callback when Ethernet connects
void onEthernetConnected(IPAddress ip) {
    Serial.println("\n=== ETHERNET CONNECTED ===");
    Serial.print("IP Address: ");
    Serial.println(ip);
    Serial.println("========================\n");
    
    // Your connection logic here
    // e.g., start MQTT client, HTTP server, etc.
}

// Callback when Ethernet disconnects
void onEthernetDisconnected(uint32_t durationMs) {
    Serial.println("\n=== ETHERNET DISCONNECTED ===");
    Serial.print("Was connected for: ");
    Serial.print(durationMs / 1000);
    Serial.println(" seconds");
    Serial.println("===========================\n");
    
    // Your disconnection logic here
    // e.g., stop services, save state, etc.
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    
    Serial.println("\n=== Advanced EthernetManager Example ===\n");
    
    // Set up callbacks before initialization
    EthernetManager::setConnectedCallback(onEthernetConnected);
    EthernetManager::setDisconnectedCallback(onEthernetDisconnected);
    
    // Enable auto-reconnect with exponential backoff
    // Parameters: enable, max retries (0=infinite), initial delay, max delay
    EthernetManager::setAutoReconnect(true, 0, 1000, 30000);
    
    // Initialize Ethernet
    Serial.println("Initializing Ethernet...");
    if (!EthernetManager::initialize("advanced-esp32")) {
        Serial.print("Ethernet initialization failed! Error: ");
        Serial.println(EthernetManager::errorToString(EthernetManager::getLastError()));
        
        // Dump diagnostics even on failure
        EthernetManager::dumpDiagnostics(&Serial);
        return;
    }
    
    Serial.println("Ethernet initialized successfully!");
    wasConnected = true;
    
    // Reset statistics to start fresh
    EthernetManager::resetStatistics();
    
    // Print initial diagnostics
    EthernetManager::dumpDiagnostics(&Serial);
}

void loop() {
    // Monitor connection state changes
    bool isConnected = EthernetManager::isConnected();
    
    if (isConnected != wasConnected) {
        wasConnected = isConnected;
        
        if (!isConnected) {
            Serial.println("Connection lost - auto-reconnect enabled");
        }
    }
    
    // Periodically print statistics
    if (millis() - lastStatsTime > STATS_INTERVAL) {
        lastStatsTime = millis();
        
        Serial.println("\n--- Periodic Statistics ---");
        NetworkStats stats = EthernetManager::getStatistics();
        
        Serial.print("Uptime: ");
        Serial.print(stats.uptimeMs / 1000);
        Serial.println(" seconds");
        
        Serial.print("Disconnections: ");
        Serial.println(stats.disconnectCount);
        
        Serial.print("Reconnections: ");
        Serial.println(stats.reconnectCount);
        
        if (stats.linkDownEvents > 0) {
            Serial.print("Link down events: ");
            Serial.println(stats.linkDownEvents);
        }
        
        if (isConnected) {
            Serial.print("Current IP: ");
            Serial.println(ETH.localIP());
            
            Serial.print("Link Speed: ");
            Serial.print(ETH.linkSpeed());
            Serial.println(" Mbps");
        }
        
        Serial.println("------------------------\n");
    }
    
    // Your main application logic here
    if (isConnected) {
        // Do connected tasks
    } else {
        // Handle disconnected state
    }
    
    delay(100);
}

// Optional: Handle serial commands for testing
void serialEvent() {
    while (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 'd':
            case 'D':
                // Dump full diagnostics
                Serial.println("\n=== Manual Diagnostic Dump ===");
                EthernetManager::dumpDiagnostics(&Serial);
                break;
                
            case 'r':
            case 'R':
                // Reset statistics
                Serial.println("\n=== Resetting Statistics ===");
                EthernetManager::resetStatistics();
                Serial.println("Statistics reset!");
                break;
                
            case 's':
            case 'S':
                // Show current statistics
                Serial.println("\n=== Current Statistics ===");
                NetworkStats stats = EthernetManager::getStatistics();
                Serial.print("Uptime: ");
                Serial.print(stats.uptimeMs / 1000);
                Serial.println(" seconds");
                Serial.print("Disconnections: ");
                Serial.println(stats.disconnectCount);
                Serial.print("Reconnections: ");
                Serial.println(stats.reconnectCount);
                break;
                
            case '?':
            case 'h':
            case 'H':
                // Show help
                Serial.println("\n=== Serial Commands ===");
                Serial.println("d/D - Dump diagnostics");
                Serial.println("r/R - Reset statistics");
                Serial.println("s/S - Show statistics");
                Serial.println("?/h/H - Show this help");
                Serial.println("====================");
                break;
        }
    }
}