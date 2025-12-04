#ifndef MOCK_ETH_H
#define MOCK_ETH_H

#include <Arduino.h>
#include <functional>

/**
 * Mock ETH class for testing EthernetManager without hardware
 */
class MockETHClass {
public:
    // State variables
    bool _started = false;
    bool _linkUp = false;
    IPAddress _localIP;
    IPAddress _gatewayIP;
    IPAddress _subnetMask;
    IPAddress _dnsIP;
    String _hostname;
    uint8_t _macAddress[6] = {0};
    int _linkSpeed = 100;
    bool _fullDuplex = true;
    
    // Callback for simulating events
    std::function<void(system_event_id_t, system_event_info_t)> onEventCallback;
    
    // ETH API methods
    bool begin(uint8_t phy_addr = 0, int8_t power = -1, int8_t mdc = 23, 
               int8_t mdio = 18, eth_phy_type_t type = ETH_PHY_LAN8720, 
               eth_clock_mode_t clock_mode = ETH_CLOCK_GPIO17_OUT) {
        _started = true;
        return true;
    }
    
    bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, 
                IPAddress dns1 = IPAddress(), IPAddress dns2 = IPAddress()) {
        _localIP = local_ip;
        _gatewayIP = gateway;
        _subnetMask = subnet;
        _dnsIP = dns1;
        return true;
    }
    
    bool setHostname(const char* hostname) {
        _hostname = String(hostname);
        return true;
    }
    
    String getHostname() { return _hostname; }
    
    IPAddress localIP() { return _localIP; }
    IPAddress gatewayIP() { return _gatewayIP; }
    IPAddress subnetMask() { return _subnetMask; }
    IPAddress dnsIP() { return _dnsIP; }
    
    void macAddress(uint8_t* mac) {
        memcpy(mac, _macAddress, 6);
    }
    
    String macAddress() {
        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                _macAddress[0], _macAddress[1], _macAddress[2],
                _macAddress[3], _macAddress[4], _macAddress[5]);
        return String(macStr);
    }
    
    bool linkUp() { return _linkUp; }
    int linkSpeed() { return _linkSpeed; }
    bool fullDuplex() { return _fullDuplex; }
    
    // Test helpers
    void simulateLinkUp() {
        _linkUp = true;
        if (onEventCallback) {
            system_event_info_t info = {};
            onEventCallback(SYSTEM_EVENT_ETH_CONNECTED, info);
        }
    }
    
    void simulateLinkDown() {
        _linkUp = false;
        if (onEventCallback) {
            system_event_info_t info = {};
            onEventCallback(SYSTEM_EVENT_ETH_DISCONNECTED, info);
        }
    }
    
    void simulateGotIP(IPAddress ip) {
        _localIP = ip;
        if (onEventCallback) {
            system_event_info_t info = {};
            info.got_ip.ip_info.ip.addr = ip;
            onEventCallback(SYSTEM_EVENT_ETH_GOT_IP, info);
        }
    }
    
    void reset() {
        _started = false;
        _linkUp = false;
        _localIP = IPAddress();
        _gatewayIP = IPAddress();
        _subnetMask = IPAddress();
        _dnsIP = IPAddress();
        _hostname = "";
        memset(_macAddress, 0, 6);
        _linkSpeed = 100;
        _fullDuplex = true;
    }
};

// Global instance to replace ETH
extern MockETHClass MockETH;

// Macro to replace ETH with MockETH in tests
#ifdef ETHERNET_MANAGER_TEST_MODE
    #define ETH MockETH
#endif

#endif // MOCK_ETH_H