/**
 * Advanced Error Recovery Example for EthernetManager
 * 
 * This example demonstrates:
 * - Builder pattern configuration
 * - State machine monitoring
 * - Link status monitoring
 * - Error recovery patterns
 * - Performance monitoring
 * - Custom DNS configuration
 * - Event batching for performance
 */

#include <EthernetManager.h>

// Configuration
const char* HOSTNAME = "esp32-recovery-demo";
const uint32_t RECOVERY_ATTEMPT_DELAY = 5000; // 5 seconds between recovery attempts
const uint32_t MAX_RECOVERY_ATTEMPTS = 3;

// State tracking
EthConnectionState lastState = EthConnectionState::UNINITIALIZED;
uint32_t recoveryAttempts = 0;
unsigned long lastRecoveryAttempt = 0;
bool performanceLogged = false;

// Custom logging function
void customLog(const char* message) {
    Serial.print("[CUSTOM LOG] ");
    Serial.println(message);
}

// State change callback
void onStateChange(EthConnectionState oldState, EthConnectionState newState) {
    Serial.println("\n=== STATE CHANGE ===");
    Serial.print("From: ");
    Serial.print(EthernetManager::stateToString(oldState));
    Serial.print(" -> To: ");
    Serial.println(EthernetManager::stateToString(newState));
    
    // Handle specific state transitions
    switch (newState) {
        case EthConnectionState::ERROR_STATE:
            Serial.println("ERROR STATE: Attempting recovery...");
            recoveryAttempts++;
            break;
            
        case EthConnectionState::LINK_DOWN:
            Serial.println("LINK DOWN: Check cable connection");
            break;
            
        case EthConnectionState::OBTAINING_IP:
            Serial.println("OBTAINING IP: DHCP in progress...");
            break;
            
        case EthConnectionState::CONNECTED:
            Serial.println("CONNECTED: Network ready!");
            recoveryAttempts = 0; // Reset recovery counter
            logPerformanceMetrics();
            break;
            
        default:
            break;
    }
    Serial.println("==================\n");
}

// Link status callback
void onLinkStatusChange(bool linkUp) {
    Serial.print("\n[LINK STATUS] ");
    Serial.println(linkUp ? "Cable connected" : "Cable disconnected");
    
    if (!linkUp) {
        Serial.println("Tip: Check physical cable connection");
    }
}

// Connection callback
void onConnected(IPAddress ip) {
    Serial.println("\n=== CONNECTION ESTABLISHED ===");
    Serial.print("IP Address: ");
    Serial.println(ip);
    Serial.print("Subnet Mask: ");
    Serial.println(ETH.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(ETH.gatewayIP());
    Serial.print("DNS 1: ");
    Serial.println(ETH.dnsIP(0));
    Serial.print("DNS 2: ");
    Serial.println(ETH.dnsIP(1));
    Serial.print("MAC Address: ");
    Serial.println(ETH.macAddress());
    Serial.print("Hostname: ");
    Serial.println(ETH.getHostname());
    Serial.print("Link Speed: ");
    Serial.print(ETH.linkSpeed());
    Serial.println(" Mbps");
    Serial.print("Full Duplex: ");
    Serial.println(ETH.fullDuplex() ? "Yes" : "No");
    Serial.println("=============================\n");
}

// Disconnection callback
void onDisconnected(uint32_t durationMs) {
    Serial.println("\n=== DISCONNECTED ===");
    Serial.print("Was connected for: ");
    Serial.print(durationMs / 1000.0, 2);
    Serial.println(" seconds");
    Serial.println("===================\n");
}

// Log performance metrics
void logPerformanceMetrics() {
    if (performanceLogged) return; // Log only once per session
    
    uint32_t initTime, linkUpTime, ipObtainTime, eventCount;
    if (EthernetManager::getPerformanceMetrics(initTime, linkUpTime, ipObtainTime, eventCount)) {
        Serial.println("\n=== PERFORMANCE METRICS ===");
        Serial.print("PHY Init Time: ");
        Serial.print(initTime);
        Serial.println(" ms");
        Serial.print("Link Up Time: ");
        Serial.print(linkUpTime);
        Serial.println(" ms");
        Serial.print("IP Obtain Time: ");
        Serial.print(ipObtainTime);
        Serial.println(" ms");
        Serial.print("Total Events: ");
        Serial.println(eventCount);
        Serial.println("=========================\n");
        performanceLogged = true;
    }
}

// Attempt recovery from error state
void attemptRecovery() {
    if (recoveryAttempts >= MAX_RECOVERY_ATTEMPTS) {
        Serial.println("Max recovery attempts reached. Manual intervention required.");
        return;
    }
    
    if (millis() - lastRecoveryAttempt < RECOVERY_ATTEMPT_DELAY) {
        return; // Wait before next attempt
    }
    
    lastRecoveryAttempt = millis();
    Serial.print("Recovery attempt ");
    Serial.print(recoveryAttempts);
    Serial.print(" of ");
    Serial.println(MAX_RECOVERY_ATTEMPTS);
    
    // Try to reset the interface
    if (EthernetManager::resetInterface()) {
        Serial.println("Interface reset initiated");
    } else {
        Serial.println("Interface reset failed");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
        ; // Wait for serial port
    }
    
    Serial.println("\n=== Advanced Error Recovery Example ===\n");
    
    // Enable custom logging
    EthernetManager::enableDebugLogging(customLog);
    
    // Set up callbacks
    EthernetManager::setStateChangeCallback(onStateChange);
    EthernetManager::setLinkStatusCallback(onLinkStatusChange);
    EthernetManager::setConnectedCallback(onConnected);
    EthernetManager::setDisconnectedCallback(onDisconnected);
    
    // Configure performance optimizations
    Serial.println("Configuring performance optimizations...");
    EthernetManager::configurePerformance(
        true,   // Enable event batching
        150,    // 150ms mutex timeout
        20      // Event queue size
    );
    
    // Build configuration with error recovery features
    EthernetConfig config = EthernetConfig()
        .withHostname(HOSTNAME)
        .withLinkMonitoring(500)        // Check link every 500ms
        .withAutoReconnect(5, 2000, 30000); // 5 retries, 2s initial, 30s max
    
    // Optional: Configure static IP (uncomment to use)
    // config.withStaticIP(
    //     IPAddress(192, 168, 1, 100),  // IP
    //     IPAddress(192, 168, 1, 1),    // Gateway
    //     IPAddress(255, 255, 255, 0),  // Subnet
    //     IPAddress(8, 8, 8, 8),        // DNS1
    //     IPAddress(8, 8, 4, 4)         // DNS2
    // );
    
    // Initialize with configuration
    Serial.println("Initializing Ethernet with advanced configuration...");
    if (!EthernetManager::initialize(config)) {
        Serial.print("Initialization failed! Error: ");
        Serial.println(EthernetManager::errorToString(EthernetManager::getLastError()));
        Serial.println("\nTroubleshooting tips:");
        Serial.println("1. Check PHY wiring (MDC, MDIO, power pins)");
        Serial.println("2. Verify PHY address (default: 0)");
        Serial.println("3. Check clock mode configuration");
        Serial.println("4. Ensure LAN8720A is properly powered");
        
        // Still set up link monitoring to detect when cable is connected
        EthernetManager::setLinkMonitoring(true, 1000);
    }
    
    // Optional: Set custom DNS servers after initialization
    // EthernetManager::setDNSServers(
    //     IPAddress(1, 1, 1, 1),    // Cloudflare DNS
    //     IPAddress(1, 0, 0, 1)
    // );
    
    Serial.println("\nSetup complete. Monitoring connection...\n");
}

void loop() {
    static unsigned long lastStatusTime = 0;
    static unsigned long lastDiagnosticTime = 0;
    const unsigned long STATUS_INTERVAL = 10000; // 10 seconds
    const unsigned long DIAGNOSTIC_INTERVAL = 60000; // 1 minute
    
    // Get current state
    EthConnectionState currentState = EthernetManager::getConnectionState();
    
    // Handle error recovery
    if (currentState == EthConnectionState::ERROR_STATE) {
        attemptRecovery();
    }
    
    // Periodic status update
    if (millis() - lastStatusTime > STATUS_INTERVAL) {
        lastStatusTime = millis();
        
        Serial.println("\n--- Status Update ---");
        Serial.print("State: ");
        Serial.println(EthernetManager::stateToString(currentState));
        Serial.print("Connected: ");
        Serial.println(EthernetManager::isConnected() ? "Yes" : "No");
        Serial.print("Link Up: ");
        Serial.println(EthernetManager::isLinkUp() ? "Yes" : "No");
        
        if (EthernetManager::isConnected()) {
            Serial.print("Uptime: ");
            Serial.println(EthernetManager::getUptimeString());
            
            // Quick status check
            IPAddress ip;
            int linkSpeed;
            bool fullDuplex;
            if (EthernetManager::getQuickStatus(ip, linkSpeed, fullDuplex)) {
                Serial.print("IP: ");
                Serial.print(ip);
                Serial.print(" | Speed: ");
                Serial.print(linkSpeed);
                Serial.print(" Mbps | Duplex: ");
                Serial.println(fullDuplex ? "Full" : "Half");
            }
        }
        
        Serial.println("-------------------\n");
    }
    
    // Periodic diagnostic dump
    if (millis() - lastDiagnosticTime > DIAGNOSTIC_INTERVAL) {
        lastDiagnosticTime = millis();
        Serial.println("\n=== DIAGNOSTIC DUMP ===");
        EthernetManager::dumpDiagnostics(&Serial);
        
        // Also show network stats
        NetworkStats stats = EthernetManager::getStatistics();
        Serial.println("\n--- Network Statistics ---");
        Serial.print("Disconnections: ");
        Serial.println(stats.disconnectCount);
        Serial.print("Reconnections: ");
        Serial.println(stats.reconnectCount);
        Serial.print("Link Down Events: ");
        Serial.println(stats.linkDownEvents);
        Serial.println("------------------------\n");
    }
    
    // Handle serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        handleCommand(cmd);
    }
    
    delay(100);
}

void handleCommand(char cmd) {
    switch (cmd) {
        case 'r':
        case 'R':
            Serial.println("\n=== Manual Reset Requested ===");
            if (EthernetManager::resetInterface()) {
                Serial.println("Reset initiated");
            } else {
                Serial.println("Reset failed");
            }
            break;
            
        case 'd':
        case 'D':
            Serial.println("\n=== Full Diagnostics ===");
            EthernetManager::dumpDiagnostics(&Serial);
            break;
            
        case 's':
        case 'S':
            Serial.println("\n=== Statistics Reset ===");
            EthernetManager::resetStatistics();
            Serial.println("Statistics cleared");
            break;
            
        case 'p':
        case 'P':
            performanceLogged = false; // Allow re-logging
            logPerformanceMetrics();
            break;
            
        case 'm':
        case 'M':
            Serial.println("\n=== Memory Info ===");
            Serial.print("Free heap: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");
            Serial.print("Minimum free heap: ");
            Serial.print(ESP.getMinFreeHeap());
            Serial.println(" bytes");
            break;
            
        case '?':
        case 'h':
        case 'H':
            Serial.println("\n=== Commands ===");
            Serial.println("r/R - Reset interface");
            Serial.println("d/D - Dump diagnostics");
            Serial.println("s/S - Reset statistics");
            Serial.println("p/P - Show performance metrics");
            Serial.println("m/M - Show memory info");
            Serial.println("?/h/H - Show this help");
            Serial.println("===============\n");
            break;
            
        default:
            // Ignore other characters
            break;
    }
}