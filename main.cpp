#include "mbed.h"
#include "arm_book_lib.h"

// Pin definitions
DigitalIn enterButton(BUTTON1);
DigitalIn gasDetector(D2);
DigitalIn overTempDetector(D3);
DigitalIn aButton(D4);
DigitalIn bButton(D5);
DigitalIn cButton(D6);
DigitalIn dButton(D7);

DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

// Simple debounce function
bool debounce(DigitalIn& input, int delay_ms = 50) {
    if (input) {
        wait_us(delay_ms * 1000);
        return input; // Return true only if still high after delay
    }
    return false;
}

int main()
{
    // Configure pull-down resistors
    gasDetector.mode(PullDown);
    overTempDetector.mode(PullDown);
    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);

    // Initialize outputs
    alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;

    bool alarmState = OFF;
    bool emergencyMode = OFF;
    int numberOfIncorrectCodes = 0;
    bool systemLocked = false;

    Timer lockoutTimer;
    Timer flashTimer;

    flashTimer.start(); // Always running

    while (true) {
        // --- Alarm Detection ---
        if (debounce(gasDetector) || debounce(overTempDetector)) {
            if (!alarmState) alarmState = ON;
            if (debounce(gasDetector) && debounce(overTempDetector) && !emergencyMode) {
                emergencyMode = ON;
            }
        }

        // --- Alarm LED Behavior ---
        if (alarmState) {
            if (emergencyMode && flashTimer.read() >= 0.1f) {
                alarmLed = !alarmLed; // Rapid blink (100ms)
                flashTimer.reset();
                flashTimer.start(); // Restart the timer
            } else if (!emergencyMode) {
                alarmLed = ON; // Solid ON
            }
        } else {
            alarmLed = OFF;
        }

        // --- Lockout Behavior ---
        if (systemLocked) {
            if (flashTimer.read() >= 0.5f) {
                systemBlockedLed = !systemBlockedLed; // Slow blink (500ms)
                flashTimer.reset();
                flashTimer.start(); // Restart the timer
            }
            if (lockoutTimer.read() >= 60.0f) {
                systemLocked = false;
                systemBlockedLed = OFF;
                numberOfIncorrectCodes = 0;
                lockoutTimer.stop();
                lockoutTimer.reset();
            }
            continue; // Skip code entry
        }

        // --- Code Entry ---
        if (numberOfIncorrectCodes < 5) {
            if (debounce(aButton) && debounce(bButton) && debounce(cButton) && debounce(dButton) && !debounce(enterButton)) {
                incorrectCodeLed = OFF;
            }

            if (debounce(enterButton) && alarmState) {
                bool a = debounce(aButton);
                bool b = debounce(bButton);
                bool c = debounce(cButton);
                bool d = debounce(dButton);

                if (a && b && !c && !d) {
                    alarmState = OFF;
                    emergencyMode = OFF;
                    numberOfIncorrectCodes = 0;
                    incorrectCodeLed = OFF;
                } else {
                    incorrectCodeLed = ON;
                    numberOfIncorrectCodes++;
                    if (numberOfIncorrectCodes >= 5) {
                        systemLocked = true;
                        systemBlockedLed = ON;
                        lockoutTimer.start();
                    }
                }

                while (debounce(enterButton)) {
                    wait_us(10000); // Wait 10ms
                }
            }
        }
    }
}
