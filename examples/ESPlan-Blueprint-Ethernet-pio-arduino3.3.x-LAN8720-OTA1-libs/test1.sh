#!/bin/bash
# Find what's specifically including Network.h and mDNS

echo "=== Searching for Network.h includes ==="
find . -name "*.cpp" -o -name "*.h" -o -name "*.ino" | xargs grep -n "Network\.h\|network\.h" 2>/dev/null

echo -e "\n=== Searching for mDNS includes ==="
find . -name "*.cpp" -o -name "*.h" -o -name "*.ino" | xargs grep -n -i "mdns\|ESPmDNS" 2>/dev/null

echo -e "\n=== Searching for hostname usage ==="
find . -name "*.cpp" -o -name "*.h" -o -name "*.ino" | xargs grep -n -i "hostname\|setHostname\|getHostname" 2>/dev/null

echo -e "\n=== Check ESP32 framework for auto-includes ==="
# Check if EthernetManager or Arduino core automatically includes Network.h
echo "Files that might auto-include Network.h:"
find ~/.platformio/packages/framework-arduinoespressif32 -name "*.h" -exec grep -l "Network\.h\|network\.h" {} \; 2>/dev/null | head -10

echo -e "\n=== Check what includes mDNS automatically ==="
find ~/.platformio/packages/framework-arduinoespressif32 -name "*.h" -exec grep -l -i "mdns" {} \; 2>/dev/null | head -5
