# EthernetManager

A robust and optimized Ethernet manager library for ESP32 devices using the LAN8720A PHY. This library provides simplified initialization, automatic reconnection, event handling, and comprehensive configuration options for Ethernet connectivity in industrial and IoT applications.

## Features

- **LAN8720A PHY Support**: Full support for the popular LAN8720A Ethernet PHY
- **Automatic DHCP Configuration**: Seamless network configuration with DHCP
- **Static IP Support**: Complete static IP configuration with DNS support
- **Thread Safety**: Mutex-protected public methods for multi-threaded applications
- **Robust Error Handling**: Comprehensive parameter validation and error recovery
- **Connection Monitoring**: Real-time connection status tracking
- **Automatic Reconnection**: Handles network disruptions gracefully
- **Event Callbacks**: Event-driven architecture with comprehensive event handling
- **Optimized Performance**: Early event registration, cached handles, and reduced redundant operations
- **Flexible Configuration**: Customizable pins, hostname, and network settings
- **Custom MAC Address**: Support for setting custom MAC addresses
- **Dual Initialization Modes**: Blocking and non-blocking initialization options
- **Arduino Core Compatibility**: Works with both Arduino ESP32 2.x and 3.x
- **Memory Safe**: Proper cleanup and resource management
- **Advanced Error Handling**: Detailed error codes for debugging
- **Auto-Reconnection**: Configurable retry with exponential backoff
- **Network Statistics**: Track uptime, disconnections, and reconnections
- **Event Callbacks**: Custom handlers for connection state changes
- **Diagnostic Tools**: Comprehensive diagnostic dump functionality

## Requirements

- ESP32 board with LAN8720A PHY
- Arduino ESP32 Core (2.0.0 or later)
- FreeRTOS (included with ESP32 Arduino Core)

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps = EthernetManager
```

### Arduino IDE

1. Download the library as a ZIP file
2. In Arduino IDE: Sketch -> Include Library -> Add .ZIP Library
3. Select the downloaded file

## Hardware Setup

### Default Pin Configuration

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| MDC      | GPIO 23  | Management Data Clock |
| MDIO     | GPIO 18  | Management Data Input/Output |
| CLK      | GPIO 17  | Ethernet Reference Clock (50MHz) |
| Power    | -1       | No power control pin (optional) |

### Typical ESP32 + LAN8720A Wiring

```
ESP32         LAN8720A
------        --------
GPIO23   -->  MDC
GPIO18   <->  MDIO
GPIO17   -->  CLK (50MHz output)
3.3V     -->  VCC
GND      -->  GND
GPIO19   <--  TX0
GPIO21   -->  TX_EN
GPIO26   <--  RX0
GPIO27   <--  RX1
GPIO25   <--  CRS_DV
```

## Usage

### Basic Example

```cpp
#include <Arduino.h>
#include <EthernetManager.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize Ethernet (blocking mode - waits for connection)
  if (!EthernetManager::initialize()) {
    Serial.println("Failed to initialize Ethernet");
    return;
  }
  
  Serial.println("Ethernet connected!");
  EthernetManager::logEthernetStatus();
}

void loop() {
  // Check connection status
  if (EthernetManager::isConnected()) {
    // Network operations here
  }
  delay(1000);
}
```

### Asynchronous Initialization

```cpp
void setup() {
  Serial.begin(115200);
  
  // Start Ethernet without waiting for connection
  if (!EthernetManager::initializeAsync()) {
    Serial.println("Failed to start Ethernet PHY");
    return;
  }
  
  // Continue with other initialization
  // ...
  
  // Later, wait for connection
  if (EthernetManager::waitForConnection(10000)) {
    Serial.println("Connected!");
  }
}
```

### Custom Configuration

```cpp
// Define custom settings before including the library
#define ETH_HOSTNAME "my-boiler-controller"
#define ETH_PHY_MDC_PIN 16
#define ETH_PHY_MDIO_PIN 17
#define ETH_INIT_TIMEOUT_MS 10000

#include <EthernetManager.h>

void setup() {
  // Or pass parameters directly
  EthernetManager::initialize(
    "custom-hostname",    // hostname
    0,                   // PHY address
    23,                  // MDC pin
    18,                  // MDIO pin
    -1,                  // Power pin
    ETH_CLOCK_GPIO17_OUT // Clock mode
  );
}
```

### Custom MAC Address

```cpp
void setup() {
  // Set custom MAC before initialization
  uint8_t customMac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  EthernetManager::setMacAddress(customMac);
  
  // Initialize normally
  EthernetManager::initialize();
}
```

### Static IP Configuration

```cpp
void setup() {
  Serial.begin(115200);
  
  // Configure static IP addresses
  IPAddress local_ip(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns1(8, 8, 8, 8);
  IPAddress dns2(8, 8, 4, 4);
  
  // Initialize with static IP
  if (EthernetManager::initializeStatic("my-device", local_ip, gateway, subnet, dns1, dns2)) {
    Serial.println("Ethernet connected with static IP");
    EthernetManager::logEthernetStatus();
  } else {
    Serial.println("Failed to initialize Ethernet");
  }
}
```

### Logging Configuration

EthernetManager supports flexible logging configuration with zero overhead in production builds.

#### Using ESP-IDF Logging (Default)
No configuration needed. The library will use ESP-IDF logging.

#### Using Custom Logger
Define `USE_CUSTOM_LOGGER` in your build flags:
```ini
build_flags = -DUSE_CUSTOM_LOGGER
lib_deps = 
    Logger  ; Required when using custom logger
    EthernetManager
```

In your main.cpp:
```cpp
#include <Logger.h>
#include <LogInterfaceImpl.h>
#include <EthernetManager.h>

void setup() {
    Logger::getInstance().init(1024);
    // EthernetManager now logs through custom Logger
}
```

#### Debug Logging
To enable debug/verbose logging for EthernetManager:
```ini
build_flags = -DETHERNETMANAGER_DEBUG
```

When `ETHERNETMANAGER_DEBUG` is defined:
- Debug (D) and Verbose (V) log levels are enabled
- Performance timing macros become available
- Buffer dump utilities are enabled

#### Complete Example
```ini
[env:debug]
platform = espressif32
board = esp32dev
framework = arduino
build_flags = 
    -DUSE_CUSTOM_LOGGER     ; Use custom logger
    -DETHERNETMANAGER_DEBUG ; Enable debug for EthernetManager
lib_deps = 
    Logger
    EthernetManager
```

#### Production Build
For production, simply omit the debug flag:
```ini
[env:production]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = EthernetManager
; No debug overhead - debug/verbose logs are completely removed at compile time
```

## Advanced Features (v1.2.0+)

### Builder Pattern Configuration

The library now supports a fluent configuration API for easier setup:

```cpp
// Create configuration with builder pattern
EthernetConfig config = EthernetConfig()
    .withHostname("my-device")
    .withPHYAddress(0)
    .withMDCPin(23)
    .withMDIOPin(18)
    .withLinkMonitoring(1000)      // Monitor link every 1s
    .withAutoReconnect(5, 2000);   // 5 retries, 2s initial delay

// Initialize with configuration
EthernetManager::initialize(config);
```

### State Machine Monitoring

Track detailed connection states:

```cpp
// Set state change callback
EthernetManager::setStateChangeCallback([](EthConnectionState oldState, EthConnectionState newState) {
    Serial.printf("State: %s -> %s\n", 
        EthernetManager::stateToString(oldState),
        EthernetManager::stateToString(newState));
});

// Available states:
// - UNINITIALIZED: Not initialized
// - PHY_STARTING: PHY is starting
// - LINK_DOWN: Cable disconnected
// - LINK_UP: Cable connected, no IP
// - OBTAINING_IP: DHCP in progress
// - CONNECTED: Fully connected
// - DISCONNECTING: Disconnection in progress
// - ERROR_STATE: Error occurred
```

### Performance Monitoring

Track initialization and connection performance:

```cpp
// Enable performance optimizations
EthernetManager::configurePerformance(
    true,   // Enable event batching
    150,    // 150ms mutex timeout
    20      // Event queue size
);

// Get performance metrics
uint32_t initTime, linkUpTime, ipTime, eventCount;
if (EthernetManager::getPerformanceMetrics(initTime, linkUpTime, ipTime, eventCount)) {
    Serial.printf("Init: %ums, Link: %ums, IP: %ums, Events: %u\n",
        initTime, linkUpTime, ipTime, eventCount);
}
```

### Error Recovery

Implement robust error recovery:

```cpp
// Configure auto-reconnect with exponential backoff
config.withAutoReconnect(
    10,     // Max 10 retries (0 = infinite)
    1000,   // 1s initial delay
    30000   // 30s maximum delay
);

// Manual recovery
if (EthernetManager::getConnectionState() == EthConnectionState::ERROR_STATE) {
    EthernetManager::resetInterface();
}

// Set custom DNS servers for recovery
EthernetManager::setDNSServers(
    IPAddress(8, 8, 8, 8),
    IPAddress(8, 8, 4, 4)
);
```

## API Reference

### Initialization Methods

#### `earlyInit()`
```cpp
static bool earlyInit()
```
Performs early initialization of event handlers and event groups. This is called automatically by other init methods but can be called manually for earliest possible event registration.

#### `initialize()`
```cpp
static bool initialize(
    const char* hostname = ETH_HOSTNAME,
    int8_t phy_addr = ETH_PHY_ADDR,
    int8_t mdc_pin = ETH_PHY_MDC_PIN,
    int8_t mdio_pin = ETH_PHY_MDIO_PIN,
    int8_t power_pin = ETH_PHY_POWER_PIN,
    eth_clock_mode_t clock_mode = ETH_CLOCK_MODE
)
```
Initializes Ethernet in blocking mode. Waits for connection before returning.

#### `initializeAsync()`
```cpp
static bool initializeAsync(
    const char* hostname = ETH_HOSTNAME,
    int8_t phy_addr = ETH_PHY_ADDR,
    int8_t mdc_pin = ETH_PHY_MDC_PIN,
    int8_t mdio_pin = ETH_PHY_MDIO_PIN,
    int8_t power_pin = ETH_PHY_POWER_PIN,
    eth_clock_mode_t clock_mode = ETH_CLOCK_MODE,
    const uint8_t* customMac = nullptr
)
```
Initializes Ethernet in non-blocking mode. Returns immediately after starting PHY.

#### `initializeStatic()`
```cpp
static bool initializeStatic(
    const char* hostname,
    IPAddress local_ip,
    IPAddress gateway,
    IPAddress subnet,
    IPAddress dns1 = IPAddress(),
    IPAddress dns2 = IPAddress(),
    int8_t phy_addr = ETH_PHY_ADDR,
    int8_t mdc_pin = ETH_PHY_MDC_PIN,
    int8_t mdio_pin = ETH_PHY_MDIO_PIN,
    int8_t power_pin = ETH_PHY_POWER_PIN,
    eth_clock_mode_t clock_mode = ETH_CLOCK_MODE
)
```
Initializes Ethernet with static IP configuration. Blocks until connected.

### Connection Management

#### `waitForConnection()`
```cpp
static bool waitForConnection(uint32_t timeout_ms = ETH_INIT_TIMEOUT_MS)
```
Waits for Ethernet connection to be established with DHCP IP assignment.

#### `isConnected()`
```cpp
static bool isConnected()
```
Returns true if Ethernet is connected with a valid IP address.

#### `isStarted()`
```cpp
static bool isStarted()
```
Returns true if the Ethernet PHY has been started (but not necessarily connected).

### Configuration

#### `setMacAddress()`
```cpp
static void setMacAddress(const uint8_t* mac)
```
Sets a custom MAC address. Must be called before initialization.

### Status and Debugging

#### `logEthernetStatus()`
```cpp
static void logEthernetStatus()
```
Logs current Ethernet status including IP, MAC, hostname, link speed, and duplex mode.

#### `getNetif()`
```cpp
static esp_netif_t* getNetif()
```
Returns the ESP-NETIF handle for advanced network operations.

#### `getStatistics()`
```cpp
static NetworkStats getStatistics()
```
Returns comprehensive network statistics including connection time, packet counts, and error metrics.

#### `getQuickStatus()`
```cpp
static bool getQuickStatus(IPAddress& ip, int& linkSpeed, bool& fullDuplex)
```
Retrieves current IP address, link speed, and duplex mode in a single call. Returns true if connected.

### Data Structures

#### `NetworkStats`
```cpp
struct NetworkStats {
    uint32_t connectTime;        // Time when connected (ms)
    uint32_t disconnectCount;    // Number of disconnections
    uint32_t reconnectCount;     // Number of successful reconnections
    uint32_t txPackets;          // Transmitted packets
    uint32_t rxPackets;          // Received packets
    uint32_t txBytes;            // Transmitted bytes
    uint32_t rxBytes;            // Received bytes
    uint32_t linkDownEvents;     // Link down events
    uint32_t dhcpRenewals;       // DHCP renewal count
};
```
Structure containing comprehensive network statistics for monitoring connection health and performance.

### Cleanup

#### `cleanup()`
```cpp
static void cleanup()
```
Releases all resources used by the Ethernet manager.

## Configuration Options

The library can be configured through preprocessor defines:

| Define | Default | Description |
|--------|---------|-------------|
| `ETH_HOSTNAME` | "esp32-ethernet" | Network hostname |
| `ETH_PHY_ADDR` | 0 | PHY I2C address |
| `ETH_PHY_MDC_PIN` | 23 | MDC pin number |
| `ETH_PHY_MDIO_PIN` | 18 | MDIO pin number |
| `ETH_PHY_POWER_PIN` | -1 | Power control pin (-1 = disabled) |
| `ETH_CLOCK_MODE` | ETH_CLOCK_GPIO17_OUT | Clock generation mode |
| `ETH_INIT_TIMEOUT_MS` | 5000 | Connection timeout in milliseconds |
| `ETH_CONNECTION_TRUST_WINDOW_MS` | 3000 | Time before trusting connection stability |

## Event Handling

The library handles the following events internally:

- **ETHERNET_EVENT_START**: PHY initialized
- **ETHERNET_EVENT_CONNECTED**: Link up
- **ETHERNET_EVENT_DISCONNECTED**: Link down
- **IP_EVENT_GOT_IP**: DHCP IP received

Events are processed with optimizations to prevent false disconnection reports during initialization.

## Integration with Boiler Controller

This library is designed for industrial applications like boiler controllers where reliable network connectivity is critical:

1. **Reliable Connection**: Automatic reconnection handles network interruptions
2. **Status Monitoring**: Real-time connection status for system health checks
3. **Event-Driven**: Integrates well with event-based control systems
4. **Low Latency**: Optimized for minimal connection establishment time
5. **Flexible Logging**: Integrates with existing logging infrastructure

### Example Integration

```cpp
class BoilerController {
private:
  bool networkReady = false;

public:
  void initializeNetwork() {
    // Early init for fastest event handling
    EthernetManager::earlyInit();
    
    // Start Ethernet asynchronously
    if (!EthernetManager::initializeAsync("boiler-unit-01")) {
      LOG_ERROR("Failed to start Ethernet");
      return;
    }
    
    // Continue with other initialization while Ethernet connects
    initializeSensors();
    initializeControllers();
    
    // Wait for network with timeout
    if (EthernetManager::waitForConnection(10000)) {
      networkReady = true;
      LOG_INFO("Network ready");
      EthernetManager::logEthernetStatus();
    } else {
      LOG_WARN("Network not ready - continuing in offline mode");
    }
  }
  
  void updateNetworkStatus() {
    bool currentStatus = EthernetManager::isConnected();
    if (currentStatus != networkReady) {
      networkReady = currentStatus;
      if (networkReady) {
        onNetworkConnected();
      } else {
        onNetworkDisconnected();
      }
    }
  }
};
```

## Troubleshooting

### Common Issues

1. **Connection Timeout**
   - Check physical connections (especially RX/TX pairs)
   - Verify PHY power supply (3.3V, adequate current)
   - Ensure 50MHz clock is properly configured
   - Check DHCP server availability
   - Use state monitoring to identify where connection fails

2. **Incorrect Pin Configuration**
   - Verify MDC/MDIO pins match your hardware
   - Check clock mode setting (GPIO17_OUT for most boards)
   - Ensure no pin conflicts with other peripherals

3. **Link Up but No IP**
   - Check DHCP server configuration
   - Verify network cable and switch connection
   - Try static IP configuration
   - Check for VLAN configuration issues
   - Monitor state transitions to see if stuck in OBTAINING_IP

4. **Compilation Errors**
   - Ensure ESP32 Arduino Core 2.0.0+ is installed
   - Check that all required headers are included
   - For Arduino 3.x compatibility issues, check ETH API changes

5. **Intermittent Disconnections**
   - Enable link monitoring to detect cable issues
   - Check for electromagnetic interference
   - Verify power supply stability
   - Use auto-reconnect with appropriate delays

6. **Poor Performance**
   - Enable event batching for high-traffic scenarios
   - Adjust mutex timeout for better responsiveness
   - Monitor performance metrics to identify bottlenecks

### Performance Optimization Tips

1. **Fastest Initialization**
   ```cpp
   // Call earlyInit() as soon as possible
   EthernetManager::earlyInit();
   
   // Use async initialization
   EthernetManager::initializeAsync();
   
   // Continue with other setup while connecting
   ```

2. **Reduce Event Overhead**
   ```cpp
   // Enable event batching for busy networks
   EthernetManager::configurePerformance(true, 100, 20);
   ```

3. **Optimize State Checking**
   ```cpp
   // Cache connection state to avoid repeated checks
   static bool wasConnected = false;
   bool isConnected = EthernetManager::isConnected();
   
   if (isConnected != wasConnected) {
       // Handle state change
       wasConnected = isConnected;
   }
   ```

4. **Memory Optimization**
   ```cpp
   // Use quick status check for multiple values
   IPAddress ip;
   int speed;
   bool duplex;
   EthernetManager::getQuickStatus(ip, speed, duplex);
   ```

### Debug Output

Enable debug logging to see detailed connection information:

```cpp
#define ETH_LOG_DEBUG(format, ...) Serial.printf("[ETH] " format "\n", ##__VA_ARGS__)
#define ETH_LOG_INFO(format, ...)  Serial.printf("[ETH] " format "\n", ##__VA_ARGS__)
#define ETH_LOG_WARN(format, ...)  Serial.printf("[ETH] " format "\n", ##__VA_ARGS__)
#define ETH_LOG_ERROR(format, ...) Serial.printf("[ETH] " format "\n", ##__VA_ARGS__)
```

### Advanced Diagnostics

Use the diagnostic dump for detailed troubleshooting:

```cpp
// Full diagnostic dump
EthernetManager::dumpDiagnostics(&Serial);

// Check specific metrics
NetworkStats stats = EthernetManager::getStatistics();
Serial.printf("Disconnections: %u, Link downs: %u\n", 
    stats.disconnectCount, stats.linkDownEvents);

// Monitor performance
uint32_t init, link, ip, events;
if (EthernetManager::getPerformanceMetrics(init, link, ip, events)) {
    Serial.printf("Times - Init:%ums Link:%ums IP:%ums\n", init, link, ip);
}
```

## License

This library is released under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please submit pull requests or open issues on the project repository.

## Version History

- **1.2.0** (2025-06-12) - Major update with significant enhancements:
  - Added comprehensive state machine (8 states) for connection tracking
  - Implemented builder pattern API for fluent configuration
  - Added link monitoring with configurable intervals
  - Performance optimizations: event batching, configurable mutex timeouts
  - Enhanced error recovery with interface reset capability
  - Added network statistics tracking (NetworkStats struct)
  - Implemented convenience methods (getQuickStatus, resetInterface)
  - Added custom DNS server configuration
  - Improved diagnostic tools (dumpDiagnostics)
  - Added advanced error recovery example
  - Comprehensive documentation updates
- **1.1.0** - Intermediate improvements (incorporated into 1.2.0)
- **1.0.0** - Initial release with full LAN8720A support and event handling
