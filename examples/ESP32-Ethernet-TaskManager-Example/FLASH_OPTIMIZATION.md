# Flash Optimization Guide

## Current Flash Usage (Debug Build)
- **548KB** - Full debug build with all features
- **42%** of 1.28MB partition

## Major Contributors
1. **Arduino Framework** (~200-250KB base)
   - Core functions, FreeRTOS, HAL
   - Ethernet @ 3.2.0 comes from here
   - Network @ 3.2.0 comes from here

2. **Debug Features** (~150-200KB)
   - `-O0` (no optimization)
   - Debug symbols
   - Verbose logging (level 5)
   - Stack traces
   - FreeRTOS stats

3. **Ethernet Stack** (~100-150KB)
   - lwIP TCP/IP stack
   - Ethernet PHY driver
   - DHCP client
   - Network buffers

4. **Application Code** (~50-100KB)
   - EthernetManager
   - TaskManager with monitoring
   - Logger with ring buffer
   - Network callbacks
   - Serial commands

## Optimization Strategies

### Quick Wins (No Code Changes)
1. Use release build instead of debug
2. Enable `-Os` (optimize for size)
3. Enable LTO (link-time optimization)
4. Remove debug logging

### Platform Configuration
```ini
; For minimal size:
build_flags = 
    -Os                    ; Optimize for size
    -flto                  ; Link time optimization
    -ffunction-sections    ; Granular linking
    -fdata-sections       
    -Wl,--gc-sections     ; Remove unused code
    -DNDEBUG              ; Disable asserts
    -DCORE_DEBUG_LEVEL=0  ; No debug output
```

### Library Optimizations
1. Disable FreeRTOS stats: `-DCONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=0`
2. Reduce logger buffer: `-DLOGGER_RING_BUFFER_SIZE=256`
3. Disable leak detection: Remove from debug builds only

### Expected Savings
- Release build: **~200KB reduction**
- Final size: **~350KB** (typical for Ethernet applications)

## Note on Ethernet Libraries
The Ethernet @ 3.2.0 and Network @ 3.2.0 are part of Arduino ESP32 framework 3.2.0. They provide:
- Hardware Ethernet support (ETH class)
- Network abstraction layer
- Integration with ESP-IDF's lwIP

These are required for Ethernet functionality and cannot be removed.