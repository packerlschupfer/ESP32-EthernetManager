# EthernetManager Test Suite

This directory contains unit and integration tests for the EthernetManager library.

## Structure

```
test/
├── unit/                    # Unit tests
│   ├── test_ethernet_manager.cpp    # Core functionality tests
│   └── test_advanced_features.cpp   # Advanced features tests
├── integration/             # Integration tests (future)
├── mocks/                   # Mock objects for testing
│   └── MockETH.h           # Mock ETH class
├── test_config.h           # Test configuration and helpers
├── platformio_test.ini     # PlatformIO test configuration
├── run_tests.sh           # Test runner script
└── README.md              # This file
```

## Running Tests

### From Main Project

The main project can run these tests by including the test configuration and using the test runner:

```bash
# Run unit tests only
./lib/EthernetManager/test/run_tests.sh

# Run with hardware tests (requires actual ESP32 with Ethernet)
./lib/EthernetManager/test/run_tests.sh --with-hardware

# Run native tests (no hardware required)
./lib/EthernetManager/test/run_tests.sh --native
```

### Using PlatformIO Directly

```bash
# Run all tests for a specific environment
pio test -e ethernet_manager_unit

# Run specific test
pio test -e ethernet_manager_unit --filter test_ethernet_manager

# Run with verbose output
pio test -e ethernet_manager_unit -v
```

### Integration with Main Project's platformio.ini

Add this to your main project's `platformio.ini`:

```ini
; Include EthernetManager test configuration
[platformio]
extra_configs = lib/EthernetManager/test/platformio_test.ini

; Or define test environment directly
[env:test_ethernet]
extends = test_env:ethernet_manager_unit
```

## Test Coverage

### Unit Tests

1. **Basic Functionality** (`test_ethernet_manager.cpp`)
   - Initialization states
   - Configuration builder pattern
   - Static IP configuration
   - Callback registration and execution
   - Connection state transitions
   - Error handling
   - Network statistics
   - Auto-reconnect configuration
   - Link monitoring
   - Custom MAC address

2. **Advanced Features** (`test_advanced_features.cpp`)
   - Performance metrics
   - Quick status retrieval
   - Uptime tracking
   - DNS configuration
   - Interface reset
   - Network interface statistics
   - PHY configuration
   - Link status checking
   - Diagnostics dump
   - Debug logging callback

### Mock Objects

The `MockETH` class simulates the ESP32 ETH interface, allowing tests to run without hardware. It provides:
- State management (started, link up, IP configuration)
- Event simulation (link up/down, got IP)
- Configuration tracking
- Test helpers for state manipulation

## Writing New Tests

1. Create a new test file in `test/unit/` or `test/integration/`
2. Include necessary headers:
   ```cpp
   #include <Arduino.h>
   #include <unity.h>
   #include "../test_config.h"
   #include "../mocks/MockETH.h"
   #include "../../src/EthernetManager.h"
   ```

3. Write test functions:
   ```cpp
   void test_my_feature() {
       // Setup
       EthernetManager::cleanup();
       
       // Test
       TEST_ASSERT_TRUE(/* your assertion */);
       
       // Cleanup (if needed)
   }
   ```

4. Add to test runner in `setup()`:
   ```cpp
   RUN_TEST(test_my_feature);
   ```

## Test Configuration

The `test_config.h` file provides:
- Test-specific timeouts (shorter than production)
- Test network configuration (IPs, hostnames)
- Helper macros for async testing
- Unity framework configuration

## Continuous Integration

These tests can be integrated into CI/CD pipelines:

```yaml
# Example GitHub Actions
- name: Run EthernetManager Tests
  run: |
    pio test -e ethernet_manager_unit
    pio test -e ethernet_manager_native
```

## Known Limitations

1. Hardware-dependent features cannot be fully tested without actual hardware
2. Some timing-dependent tests may be flaky in CI environments
3. Mock objects simulate ideal conditions - real hardware may behave differently

## Contributing

When adding new features to EthernetManager:
1. Add corresponding unit tests
2. Update mock objects if needed
3. Document any new test dependencies
4. Ensure all tests pass before submitting PR