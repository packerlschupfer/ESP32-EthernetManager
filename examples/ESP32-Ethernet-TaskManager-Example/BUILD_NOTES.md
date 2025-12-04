# Build Notes for EthernetManager Example

## TaskManager Integration

This example correctly uses TaskManager with the following approach:

1. **Include TaskManagerConfig.h** instead of TaskManager.h directly
   - This is the designed way to use TaskManager
   - TaskManagerConfig.h handles implementation selection

2. **Build Source Filter** in platformio.ini
   ```ini
   build_src_filter = 
       +<*>
       -<.pio/libdeps/*/TaskManager/src/TaskManager_Full.cpp>
   ```
   - Required workaround for PlatformIO's handling of git+file libraries
   - Prevents dual compilation of TaskManager.cpp and TaskManager_Full.cpp

## Known Issues and Solutions

### Issue: Multiple definition errors
- **Cause**: PlatformIO ignores library.json `export` section for git+file references
- **Solution**: Use build_src_filter (already implemented)

### Issue: ESP timer undefined reference
- **Cause**: ESP32 Arduino Core 3.x compatibility
- **Solution**: Use stable pioarduino platform (already implemented)

## Logging Configuration

- Uses `USE_CUSTOM_LOGGER` flag for custom Logger integration
- LogInterface.h provides zero-overhead logging
- Debug flags available:
  - `ETHERNETMANAGER_DEBUG` - Enable EthernetManager debug logs
  - `TASKMANAGER_DEBUG` - Enable TaskManager debug logs

## Dependencies

All libraries use git+file protocol for local development:
- Logger (branch: remove_esp_log_redirect)
- TaskManager (branch: optimized-default)
- Watchdog, SemaphoreGuard, MutexGuard
- EthernetManager (this library)