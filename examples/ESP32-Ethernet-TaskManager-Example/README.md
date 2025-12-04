# ESP32 Ethernet Project

This is a complete project template for an ESP32 application with Ethernet connectivity, structured as RTOS tasks. The project is set up for VSCode with PlatformIO. OTA functionality is available in a separate example.

## Project Structure

```
esp32-ethernet-example/
├── platformio.ini             # PlatformIO project configuration
├── src/
│   ├── main.cpp               # Main application file
│   ├── tasks/                 # Task definitions
│   │   ├── MonitoringTask.h   # System monitoring task
│   │   └── MonitoringTask.cpp # System monitoring task implementation
│   ├── config/                # Configuration files
│   │   └── ProjectConfig.h    # Project-specific configuration
│   └── utils/                 # Utility functions
│       ├── StatusLed.h        # Simple LED status indicator
│       └── StatusLed.cpp      # LED status indicator implementation
├── lib/                       # Project libraries (configured in platformio.ini)
└── include/                   # Project include files
    └── README                 # Placeholder
```

## About This Project

This is a complete ESP32 project template that demonstrates how to:

1. Structure a complex ESP32 application using FreeRTOS tasks
2. Implement robust Ethernet connectivity with EthernetManager
3. Use TaskManager for task coordination and watchdog support
4. Monitor system health and resources
5. Provide visual feedback using status LED
6. Implement thread-safe operations with proper synchronization

The project showcases best practices for ESP32 development with Arduino framework.

## Library Configuration

The project uses the git+file protocol for local library dependencies:

```ini
lib_deps = 
    ; Core utilities
    git+file://~/.platformio/lib/ESP32-Logger#remove_esp_log_redirect
    git+file://~/.platformio/lib/ESP32-SemaphoreGuard
    git+file://~/.platformio/lib/ESP32-MutexGuard
    git+file://~/.platformio/lib/ESP32-Watchdog
    git+file://~/.platformio/lib/ESP32-TaskManager#optimized-default
    
    ; Network
    git+file://~/.platformio/lib/ESP32-EthernetManager
```

### Build Flags

- `USE_CUSTOM_LOGGER`: Enables custom Logger for all libraries
- `ETHERNETMANAGER_DEBUG`: Enables debug logging for EthernetManager
- `TASKMANAGER_DEBUG`: Enables debug logging for TaskManager

## Key Features

### Multi-Task Architecture

The application is divided into RTOS tasks:
- **Main Loop Task**: Handles watchdog feeding and status LED updates
- **Monitoring Task**: Logs system health, memory usage, and network status
- **Additional Tasks**: Can be easily added for sensors, web server, etc.

### Task Communication

Tasks communicate via:
- **Semaphores**: For thread-safe data access
- **Function Calls**: Direct API calls between components
- **Status Indicators**: LED patterns indicate system state

### Thread Safety

All shared resources are protected using:
- **Mutexes**: For data access across tasks
- **Task Priorities**: To manage task execution importance

### System Status

The system provides comprehensive status via:
- **Serial Logging**: Detailed logging to the serial console
- **LED Indicators**: Visual status indications (connected, updating, error)
- **Monitoring Task**: Regular health check reports

## Getting Started

1. **Install VSCode and PlatformIO**: If not already installed
2. **Clone/Copy Project Files**: Into a new directory
3. **Connect Hardware**: Wire up your ESP32 with LAN8720A Ethernet PHY
4. **Customize Configuration**: Edit ProjectConfig.h for your settings
5. **Build and Upload**: Using PlatformIO
6. **Monitor**: View logs via Serial Monitor

## Hardware Requirements

- ESP32 Development Board
- LAN8720A Ethernet PHY Module
- Status LED (connected to GPIO2 by default)
- Optional: Sensors (add your own integration)

## First-Time Setup

1. Connect via USB and flash initial firmware
2. Monitor serial output to verify Ethernet connection
3. Check watchdog registration and system health logs

## Customization

Edit ProjectConfig.h to customize:
- Network settings
- Device hostname
- LED pin
- Task priorities and intervals
- Debug logging

## Extension Points

This project template can be extended by:
1. Adding real sensor implementations
2. Adding a web server for configuration and monitoring
3. Implementing data logging to external storage
4. Adding cloud connectivity (MQTT, HTTP, etc.)
5. Creating a user interface (OLED display, web interface, etc.)

# ESP32 Ethernet Project with RTOS Tasks - Implementation Summary

## Changes Made for Your Setup

I've customized the project to work with your specific environment, including:

1. **PlatformIO Configuration**:
   - Used your structured platformio.ini with multiple environments
   - Configured for ESP32 using Arduino core 3.3.x
   - Set up multiple build configurations (debug, release, USB, OTA)
   - Integrated with your symlinked libraries

2. **Library Integration**:
   - Integrated with your existing libraries:
     - `Logger`: For consistent logging
     - `SemaphoreGuard`: For thread-safe access to shared resources
     - `TaskManager`: For task management and debugging

3. **Code Organization**:
   - Organized as RTOS tasks with clear separation of concerns
   - Thread-safe implementations using mutexes and semaphores
   - Extensible architecture for adding more functionality

4. **Ethernet Connectivity**:
   - Used LAN8720A PHY configuration from your platformio.ini
   - Implemented EthernetManager for connection management
   - Added proper error handling and retry mechanisms

5. **Watchdog Support**:
   - Integrated with TaskManager's watchdog functionality
   - Uses singleton Watchdog pattern to avoid conflicts
   - Proper task registration and feeding

## Project Structure

The project is organized according to best practices:

1. **Config**:
   - `ProjectConfig.h`: Central configuration with defaults and logging setup

2. **Tasks**:
   - `MonitoringTask`: Logs system health and network status
   - Main loop task: Handles watchdog and LED updates
   - Extensible for additional tasks (sensors, web server, etc.)

3. **Utils**:
   - `StatusLed`: Provides visual feedback on system state

4. **Libraries**:
   - `EthernetManager`: Manages Ethernet connectivity
   - `TaskManager`: Provides task management and watchdog support
   - `Logger`: Custom logging with zero-overhead when disabled
   - Plus other utility libraries (SemaphoreGuard, MutexGuard, Watchdog)

## Special Features

1. **Thread Safety**:
   - Used `SemaphoreGuard` for protecting shared resources
   - Proper locking around status variables

2. **Task Coordination**:
   - TaskManager handles task creation and monitoring
   - Watchdog protection for critical tasks
   - Clean separation of responsibilities between tasks

3. **System Monitoring**:
   - Comprehensive health reporting (memory, uptime, task status)
   - Network status monitoring
   - CPU usage reporting (when enabled)

4. **Debug Support**:
   - Advanced task statistics when CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS is enabled
   - Configuration for different debug levels
   - Integration with exception decoder

## Getting Started

1. **Copy the Files**:
   - Copy all the source files into your project structure
   - Make sure to place the EthernetManager and OTAManager in your lib directory

2. **Modify Configuration**:
   - Set your device hostname in ProjectConfig.h
   - Configure Ethernet PHY pins if different from defaults

3. **Build and Upload**:
   - First time: `pio run -e esp32dev_usb_debug -t upload`
   - For release build: `pio run -e esp32dev_usb_release -t upload`

4. **Monitor Output**:
   - `pio device monitor`
   - Check the comprehensive logging output

## Next Steps

1. **Add Real Sensors**:
   - Modify SensorTask to connect to your actual sensors
   - Replace the simulated readings with real data

2. **Implement Web Server**:
   - Add a web server task for configuration and monitoring
   - Create a simple dashboard for sensor data

3. **Data Storage**:
   - Add SD card or SPIFFS for data logging
   - Implement time-series data collection

4. **External Connectivity**:
   - Add MQTT for sending data to cloud platforms
   - Implement secure communications

The project provides a solid foundation that you can easily extend with your specific requirements.
