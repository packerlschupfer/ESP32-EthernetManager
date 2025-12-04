# EthernetManager - CLAUDE.md

## Overview
Static Ethernet management library for ESP32 with W5500/LAN8720 PHY chips. Provides connection state machine, event callbacks, and network statistics.

## Key Features
- All-static design (intentional for embedded simplicity)
- Connection state machine with 7 states
- Event callbacks for connection/disconnection
- Network statistics tracking
- DHCP and static IP support
- Thread-safe with mutex protection

## Architecture

### Connection States
```
UNINITIALIZED → PHY_STARTING → LINK_DOWN ⟷ LINK_UP → OBTAINING_IP → CONNECTED
                                    ↓                                    ↓
                              ERROR_STATE ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
```

### Key Components
- `EthernetManager` - Static class with all functionality
- `EthResult<T>` - Result type using `common::Result`
- `NetworkStats` - Connection statistics structure
- Event callbacks for state changes

## Usage
```cpp
// Initialize with PHY type
EthernetManager::begin(ETH_PHY_W5500);

// Register callbacks
EthernetManager::onConnected([](IPAddress ip) {
    Serial.println(ip);
});

// Wait for connection
if (EthernetManager::waitForConnection(10000)) {
    // Connected
}
```

## Thread Safety
- Mutex protection for all shared state
- Event groups for synchronization
- Safe to call from multiple tasks

## Design Decision
All-static design is intentional - ESP32 typically has one Ethernet interface, and static design simplifies initialization order and reduces memory usage. See LIBRARY_ISSUES_TRACKER.md #5.

## Build Configuration
```ini
build_flags =
    -DETHERNETMANAGER_DEBUG  ; Enable debug logging
```
