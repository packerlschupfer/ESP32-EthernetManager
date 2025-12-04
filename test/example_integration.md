# Integrating EthernetManager Tests into Main Project

This document shows how to integrate EthernetManager tests into your main project.

## Method 1: Direct Integration in platformio.ini

Add this to your main project's `platformio.ini`:

```ini
; Your existing environments
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    EthernetManager
    ; other dependencies...

; Test environment for EthernetManager
[env:test_ethernet]
platform = espressif32
board = esp32dev
framework = arduino
test_framework = unity
build_flags = 
    -DETHERNETMANAGER_DEBUG
    -DETHERNET_MANAGER_TEST_MODE
    -I${PROJECT_DIR}/lib/EthernetManager/test
    -I${PROJECT_DIR}/lib/EthernetManager/test/mocks
lib_deps = 
    EthernetManager
test_build_src = yes
test_filter = 
    lib/EthernetManager/test/unit/*
```

## Method 2: Using Test Script

Create a test script in your main project:

```bash
#!/bin/bash
# File: scripts/test_libraries.sh

echo "Testing EthernetManager Library..."
cd lib/EthernetManager
./test/run_tests.sh
cd ../..
```

## Method 3: Custom Test Runner

Create a custom test file in your project's `test/` directory:

```cpp
// File: test/test_all_libraries.cpp
#include <Arduino.h>
#include <unity.h>

// Forward declare test functions from library
extern void test_ethernet_manager();
extern void test_advanced_features();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    UNITY_BEGIN();
    
    // Run EthernetManager tests
    Serial.println("=== EthernetManager Tests ===");
    test_ethernet_manager();
    test_advanced_features();
    
    // Add other library tests here
    
    UNITY_END();
}

void loop() {}
```

## Method 4: GitHub Actions CI

Add to `.github/workflows/test.yml`:

```yaml
name: Run Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Set up Python
      uses: actions/setup-python@v2
      
    - name: Install PlatformIO
      run: |
        pip install platformio
        
    - name: Run EthernetManager Tests
      run: |
        cd lib/EthernetManager
        pio test -e ethernet_manager_unit
        
    - name: Run Integration Tests
      run: |
        pio test -e test_ethernet
```

## Method 5: VSCode Tasks

Add to `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Test EthernetManager",
            "type": "shell",
            "command": "./lib/EthernetManager/test/run_tests.sh",
            "group": {
                "kind": "test",
                "isDefault": false
            },
            "problemMatcher": ["$platformio"]
        }
    ]
}
```

## Running Specific Tests

```bash
# Run only EthernetManager unit tests
pio test -f "lib/EthernetManager/test/unit/test_ethernet_manager.cpp"

# Run with specific build flags
pio test -e test_ethernet -DUSE_CUSTOM_LOGGER

# Run with verbose output
pio test -e test_ethernet -v
```

## Test Results Integration

The test results can be exported in various formats:

```bash
# JUnit format for CI integration
pio test -e test_ethernet --junit-output-path test-results.xml

# JSON format
pio test -e test_ethernet --json-output-path test-results.json
```

## Best Practices

1. **Run tests before deployment**: Add to your build script
2. **Include in CI/CD**: Automate testing on every commit
3. **Test with different configurations**: Test both with and without custom logger
4. **Monitor test performance**: Track test execution time
5. **Keep tests updated**: Update tests when adding new features