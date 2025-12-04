# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-12-04

### Added
- Initial public release
- Ethernet manager for LAN8720A PHY chip
- Connection state machine (7 states: UNINITIALIZED, PHY_STARTING, LINK_DOWN, LINK_UP, OBTAINING_IP, CONNECTED, ERROR_STATE)
- Event callbacks for connection/disconnection
- DHCP and static IP support
- Network statistics tracking (uptime, reconnects, packet stats)
- Thread-safe operations with mutex protection
- Result<T> based error handling
- Automatic reconnection with exponential backoff
- MAC address configuration
- DNS configuration support
- Connection timeout handling

Platform: ESP32 (Arduino/ESP-IDF)
Hardware: LAN8720A Ethernet PHY
License: MIT
Dependencies: MutexGuard, LibraryCommon

### Notes
- Production-tested in industrial application with static IP configuration
- Stable connection over weeks of continuous operation
- Previous internal versions (v1.x) not publicly released
- Reset to v0.1.0 for clean public release start
