// EthernetManagerConfig.h
#pragma once

#include <Arduino.h>

// Use this file to customize your EthernetManager settings
// You can copy this to your project folder and modify the values

#ifndef ETHERNET_MANAGER_CONFIG_H
#define ETHERNET_MANAGER_CONFIG_H

// Compile-time configuration validation
#if defined(ETH_PHY_MDC_PIN) && (ETH_PHY_MDC_PIN < -1 || ETH_PHY_MDC_PIN >= 40)
    #error "ETH_PHY_MDC_PIN must be between -1 and 39"
#endif

#if defined(ETH_PHY_MDIO_PIN) && (ETH_PHY_MDIO_PIN < -1 || ETH_PHY_MDIO_PIN >= 40)
    #error "ETH_PHY_MDIO_PIN must be between -1 and 39"
#endif

#if defined(ETH_PHY_POWER_PIN) && (ETH_PHY_POWER_PIN < -1 || ETH_PHY_POWER_PIN >= 40)
    #error "ETH_PHY_POWER_PIN must be between -1 and 39"
#endif

#if defined(ETH_PHY_ADDR) && (ETH_PHY_ADDR < 0 || ETH_PHY_ADDR > 31)
    #error "ETH_PHY_ADDR must be between 0 and 31"
#endif

#if defined(ETH_INIT_TIMEOUT_MS) && ETH_INIT_TIMEOUT_MS < 100
    #warning "ETH_INIT_TIMEOUT_MS is very low, may cause connection failures"
#endif

#if defined(ETH_CONNECTION_TRUST_WINDOW_MS) && ETH_CONNECTION_TRUST_WINDOW_MS < 1000
    #warning "ETH_CONNECTION_TRUST_WINDOW_MS is very low, may cause false disconnections"
#endif

// Default hostname for Ethernet connection
#ifndef ETH_HOSTNAME
#define ETH_HOSTNAME "esp32-ethernet"
#endif

// LAN8720A PHY Settings - Can be overridden by user
#ifndef ETH_PHY_POWER_PIN
#define ETH_PHY_POWER_PIN -1       // No power pin
#endif

#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR 0             // Default PHY address
#endif

#ifndef ETH_PHY_MDC_PIN
#define ETH_PHY_MDC_PIN 23         // Default MDC pin
#endif

#ifndef ETH_PHY_MDIO_PIN
#define ETH_PHY_MDIO_PIN 18        // Default MDIO pin
#endif

// Use GPIO17 for Ethernet clock by default
#ifndef ETH_CLOCK_MODE
#define ETH_CLOCK_MODE ETH_CLOCK_GPIO17_OUT
#endif

// Connection timeouts
#ifndef ETH_INIT_TIMEOUT_MS
#define ETH_INIT_TIMEOUT_MS 5000
#endif

#ifndef ETH_CONNECTION_TRUST_WINDOW_MS
#define ETH_CONNECTION_TRUST_WINDOW_MS 3000
#endif

// Mutex timeouts (milliseconds)
#ifndef ETH_MUTEX_QUICK_TIMEOUT_MS
#define ETH_MUTEX_QUICK_TIMEOUT_MS 100
#endif

#ifndef ETH_MUTEX_STANDARD_TIMEOUT_MS
#define ETH_MUTEX_STANDARD_TIMEOUT_MS 1000
#endif

#ifndef ETH_MUTEX_INIT_TIMEOUT_MS
#define ETH_MUTEX_INIT_TIMEOUT_MS 5000
#endif

// Hardware limits
#ifndef ETH_MAX_PHY_ADDRESS
#define ETH_MAX_PHY_ADDRESS 31
#endif

#ifndef ETH_MAX_HOSTNAME_LENGTH
#define ETH_MAX_HOSTNAME_LENGTH 63
#endif

#ifndef ETH_MAC_ADDRESS_SIZE
#define ETH_MAC_ADDRESS_SIZE 6
#endif

// Wait chunk for connection polling
#ifndef ETH_WAIT_CHUNK_MS
#define ETH_WAIT_CHUNK_MS 100
#endif

// Event batch timer window
#ifndef ETH_EVENT_BATCH_WINDOW_MS
#define ETH_EVENT_BATCH_WINDOW_MS 50
#endif

// Include logging configuration
#include "EthernetManagerLogging.h"

#endif // ETHERNET_MANAGER_CONFIG_H
