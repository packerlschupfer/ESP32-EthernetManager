// StatusLed.h
#pragma once

#include <Arduino.h>
#include "../config/ProjectConfig.h"

/**
 * @brief Simple status LED handler for displaying system state
 * 
 * LED Pattern Guide:
 * - Solid ON: Ethernet connected
 * - Slow blink (1s): Normal operation, waiting for connection
 * - Fast blink (100ms): Initialization
 * - Double blink pattern: Connection error
 * - OFF: LED disabled or system shutdown
 */
class StatusLed {
public:
    /**
     * @brief Initialize the status LED
     * 
     * @param pin GPIO pin connected to the LED
     */
    static void init(uint8_t pin);
    
    /**
     * @brief Check if LED support is enabled
     * 
     * @return true if LED is enabled, false otherwise
     */
    static bool isEnabled();
    
    /**
     * @brief Set LED to solid on
     */
    static void setOn();
    
    /**
     * @brief Set LED to solid off
     */
    static void setOff();
    
    /**
     * @brief Blink LED at specific rate
     * 
     * @param blinkRate Blink interval in milliseconds
     */
    static void setBlink(uint16_t blinkRate);
    
    /**
     * @brief Blink LED with pattern (fast blinks followed by pause)
     * 
     * @param numBlinks Number of fast blinks in sequence
     * @param blinkRate Individual blink duration in milliseconds
     * @param pauseTime Pause time after sequence in milliseconds
     */
    static void setPattern(uint8_t numBlinks, uint16_t blinkRate, uint16_t pauseTime);
    
    /**
     * @brief Update LED state (call this regularly)
     */
    static void update();

private:
#ifdef ENABLE_STATUS_LED
    static uint8_t ledPin;
    static bool ledState;
    static uint32_t lastChangeTime;
    static uint16_t currentBlinkRate;
    static bool isBlinking;
    static bool isPattern;
    static uint8_t patternBlinks;
    static uint16_t patternBlinkRate;
    static uint16_t patternPauseTime;
    static uint8_t currentPatternStep;
#endif
};
