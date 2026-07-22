// StatusLed.cpp
#include "../utils/StatusLed.h"
#include "../config/ProjectConfig.h"

// Initialize static members
#ifdef ENABLE_STATUS_LED
uint8_t StatusLed::ledPin = 0;
bool StatusLed::ledState = false;
uint32_t StatusLed::lastChangeTime = 0;
uint16_t StatusLed::currentBlinkRate = 500;
bool StatusLed::isBlinking = false;
bool StatusLed::isPattern = false;
uint8_t StatusLed::patternBlinks = 0;
uint16_t StatusLed::patternBlinkRate = 0;
uint16_t StatusLed::patternPauseTime = 0;
uint8_t StatusLed::currentPatternStep = 0;
#endif

void StatusLed::init(uint8_t pin) {
#ifdef ENABLE_STATUS_LED
    ledPin = pin;
    pinMode(ledPin, OUTPUT);
    setOff();
#else
    (void)pin; // Suppress unused parameter warning
#endif
}

bool StatusLed::isEnabled() {
#ifdef ENABLE_STATUS_LED
    return true;
#else
    return false;
#endif
}

void StatusLed::setOn() {
#ifdef ENABLE_STATUS_LED
    isBlinking = false;
    isPattern = false;
    digitalWrite(ledPin, HIGH);
    ledState = true;
#endif
}

void StatusLed::setOff() {
#ifdef ENABLE_STATUS_LED
    isBlinking = false;
    isPattern = false;
    digitalWrite(ledPin, LOW);
    ledState = false;
#endif
}

void StatusLed::setBlink(uint16_t blinkRate) {
#ifdef ENABLE_STATUS_LED
    isBlinking = true;
    isPattern = false;
    currentBlinkRate = blinkRate;
    lastChangeTime = millis();
#else
    (void)blinkRate; // Suppress unused parameter warning
#endif
}

void StatusLed::setPattern(uint8_t numBlinks, uint16_t blinkRate, uint16_t pauseTime) {
#ifdef ENABLE_STATUS_LED
    isBlinking = false;
    isPattern = true;
    patternBlinks = numBlinks;
    patternBlinkRate = blinkRate;
    patternPauseTime = pauseTime;
    currentPatternStep = 0;
    lastChangeTime = millis();
    digitalWrite(ledPin, HIGH);
    ledState = true;
#else
    (void)numBlinks; // Suppress unused parameter warnings
    (void)blinkRate;
    (void)pauseTime;
#endif
}

void StatusLed::update() {
#ifdef ENABLE_STATUS_LED
    uint32_t currentTime = millis();
    
    if (isBlinking) {
        // Simple blinking mode
        if (currentTime - lastChangeTime >= currentBlinkRate) {
            ledState = !ledState;
            digitalWrite(ledPin, ledState ? HIGH : LOW);
            lastChangeTime = currentTime;
        }
    } else if (isPattern) {
        // Pattern blinking mode
        if (currentTime - lastChangeTime >= (currentPatternStep < patternBlinks * 2 ? patternBlinkRate : patternPauseTime)) {
            if (currentPatternStep < patternBlinks * 2) {
                // During blink sequence
                ledState = !ledState;
                digitalWrite(ledPin, ledState ? HIGH : LOW);
                currentPatternStep++;
            } else {
                // After pause, restart pattern
                currentPatternStep = 0;
                ledState = true;
                digitalWrite(ledPin, HIGH);
            }
            lastChangeTime = currentTime;
        }
    }
    // If not blinking or pattern, LED remains in its static state
#endif
}
