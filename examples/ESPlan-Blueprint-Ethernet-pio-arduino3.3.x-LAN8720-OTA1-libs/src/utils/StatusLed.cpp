// StatusLed.cpp
#include "../utils/StatusLed.h"

// Initialize static members
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

void StatusLed::init(uint8_t pin) {
    ledPin = pin;
    pinMode(ledPin, OUTPUT);
    setOff();
}

void StatusLed::setOn() {
    isBlinking = false;
    isPattern = false;
    digitalWrite(ledPin, HIGH);
    ledState = true;
}

void StatusLed::setOff() {
    isBlinking = false;
    isPattern = false;
    digitalWrite(ledPin, LOW);
    ledState = false;
}

void StatusLed::setBlink(uint16_t blinkRate) {
    isBlinking = true;
    isPattern = false;
    currentBlinkRate = blinkRate;
    lastChangeTime = millis();
}

void StatusLed::setPattern(uint8_t numBlinks, uint16_t blinkRate, uint16_t pauseTime) {
    isBlinking = false;
    isPattern = true;
    patternBlinks = numBlinks;
    patternBlinkRate = blinkRate;
    patternPauseTime = pauseTime;
    currentPatternStep = 0;
    lastChangeTime = millis();
    digitalWrite(ledPin, HIGH);
    ledState = true;
}

void StatusLed::update() {
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
}
