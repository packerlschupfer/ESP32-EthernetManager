#!/bin/bash
# Script to find what's including Network code

echo "=== Searching for Network includes in your source files ==="
# Search for network-related includes in your source
find . -name "*.cpp" -o -name "*.h" -o -name "*.ino" | xargs grep -l -i "network\|wifi\|ethernet\|tcp\|udp\|http" 2>/dev/null

echo -e "\n=== Searching for specific Network.h includes ==="
find . -name "*.cpp" -o -name "*.h" -o -name "*.ino" | xargs grep -n "#include.*[Nn]etwork" 2>/dev/null

echo -e "\n=== Searching for WiFi/Ethernet includes ==="
find . -name "*.cpp" -o -name "*.h" -o -name "*.ino" | xargs grep -n "#include.*WiFi\|#include.*Ethernet" 2>/dev/null

echo -e "\n=== Checking platformio.ini for network libraries ==="
if [ -f "platformio.ini" ]; then
    grep -n -i "network\|wifi\|ethernet\|tcp\|udp\|http" platformio.ini
else
    echo "No platformio.ini found"
fi

echo -e "\n=== Checking lib_deps in platformio.ini ==="
if [ -f "platformio.ini" ]; then
    grep -A 20 "lib_deps" platformio.ini
else
    echo "No platformio.ini found"
fi
