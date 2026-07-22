#!/bin/bash
# Test runner script for EthernetManager library
# This script can be called from the main project to run all tests

echo "==============================================="
echo "Running EthernetManager Library Tests"
echo "==============================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results
PASSED=0
FAILED=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_env=$2
    
    echo -e "\n${YELLOW}Running: ${test_name}${NC}"
    echo "Environment: ${test_env}"
    echo "-----------------------------------------------"
    
    if pio test -e ${test_env} --filter "${test_name}" -v; then
        echo -e "${GREEN}✓ ${test_name} PASSED${NC}"
        ((PASSED++))
    else
        echo -e "${RED}✗ ${test_name} FAILED${NC}"
        ((FAILED++))
    fi
}

# Check if PlatformIO is installed
if ! command -v pio &> /dev/null; then
    echo -e "${RED}Error: PlatformIO CLI not found. Please install it first.${NC}"
    exit 1
fi

# Unit tests
echo -e "\n${YELLOW}=== Unit Tests ===${NC}"
run_test "test_ethernet_manager" "ethernet_manager_unit"
run_test "test_advanced_features" "ethernet_manager_unit"

# Integration tests (if hardware is available)
if [ "$1" == "--with-hardware" ]; then
    echo -e "\n${YELLOW}=== Integration Tests ===${NC}"
    echo "Note: These tests require actual ESP32 hardware with Ethernet"
    run_test "test_integration_*" "ethernet_manager_integration"
fi

# Native tests (optional)
if [ "$1" == "--native" ]; then
    echo -e "\n${YELLOW}=== Native Tests ===${NC}"
    run_test "test_ethernet_manager" "ethernet_manager_native"
fi

# Summary
echo -e "\n==============================================="
echo -e "Test Summary:"
echo -e "${GREEN}Passed: ${PASSED}${NC}"
echo -e "${RED}Failed: ${FAILED}${NC}"
echo "==============================================="

# Exit with appropriate code
if [ ${FAILED} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi