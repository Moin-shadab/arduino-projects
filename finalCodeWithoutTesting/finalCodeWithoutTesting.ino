#include <LiquidCrystal.h>
#include <EEPROM.h>

// Pin definitions for LCD
const int LCD_RS = 8;
const int LCD_ENABLE = 9;
const int LCD_D4 = 4;
const int LCD_D5 = 5;
const int LCD_D6 = 6;
const int LCD_D7 = 7;
const int BACKLIGHT = 10;
const int BUTTON_PIN = A0; // Button connected to A0 pin
const int PNP_SENSOR_PIN = A1; // PNP sensor pin
// #define PNP_SENSOR_PIN 4  // Select pin 4 for PNP sensor
// #define PNP_SENSOR_PIN 8  // Try changing to a different pin
#define PNP_SENSOR_PIN A2
// Relay pin definitions
const int RELAY1_PIN = 2;
const int RELAY2_PIN = 3;
const int TOGGLE_PIN_0 = 0; // Pin 0 (for Select Button)
const int TOGGLE_PIN_1 = 1; // Pin 1 (for Left Button)

// Initialize the LCD with the interface pins
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Button value thresholds
int buttonValues[5] = {50, 200, 400, 600, 800}; 

int currentSelection = 0;
bool inSettingsMode = false;
bool inMainMenu = true;
bool inSetR1Mode = false;
bool inSetR2Mode = false;
bool inStartMode = false; // New flag for Start mode
bool isOff = false;
bool buttonPressed = false;
float setR1Time = 0.0; // Time for Relay 1
float setDelayTime = 0.0; // Time for Relay 2
float stepSize = 0.1; // Increment/Decrement step size
unsigned long relay1StartTime = 0;
unsigned long relay2StartTime = 0;
bool isRelay1Active = false; // Track if Relay 1 is active
bool isRelay2Active = false; // Track if Relay 2 is active
bool flag = false; // Track if Relay 2 is active
unsigned long buttonPressStartTime = 0; // Timer for long press
bool longPressDetected = false; // Flag for long press
unsigned long relayDelayStartTime = 0; // Timer for delay before Relay 2 activates

// EEPROM addresses for saving times
const int EEPROM_ADDR_R1_TIME = 0;
const int EEPROM_ADDR_R2_TIME = 4; // Different EEPROM address for Set R2

// Menu items
// char* menuItems[2] = {"1: Settings", "2: Start"}; 
char* menuItems[2] = { "1: Start"}; 

char* settingItems[3] = {"Set R1", "Set D.Time", "Back"}; 
int currentSettingSelection = 0;
bool isTogglePin0Low = false; // Flag to track the state of TOGGLE_PIN_0

void setup() {
    // Serial.begin(9600); // Initialize serial communication
    lcd.begin(16, 2);
    pinMode(BACKLIGHT, OUTPUT);
    digitalWrite(BACKLIGHT, HIGH);
     pinMode(PNP_SENSOR_PIN, INPUT);
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(PNP_SENSOR_PIN, INPUT); // Set PNP sensor pin as input
        pinMode(TOGGLE_PIN_0, OUTPUT); // Pin 0 for Select Button Toggle
    pinMode(TOGGLE_PIN_1, OUTPUT); // Pin 1 for Left Button Toggle

    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
        digitalWrite(TOGGLE_PIN_0, HIGH); // Set Pin 0 to LOW initially
    digitalWrite(TOGGLE_PIN_1, HIGH); // Set Pin 1 to LOW initially
    
    // Load saved times from EEPROM with a default value fallback
    EEPROM.get(EEPROM_ADDR_R1_TIME, setR1Time);
    EEPROM.get(EEPROM_ADDR_R2_TIME, setDelayTime);
    // If R1 or R2 time are uninitialized, assign default values
    if (isnan(setR1Time) || setR1Time < 0.0 || setR1Time > 99.9) {
        setR1Time = 0.0;
    }
    if (isnan(setDelayTime) || setDelayTime < 0.0 || setDelayTime > 99.9) {
        setDelayTime = 0.0;
    }
    
    displayMenu();
}

void loop() {
    int buttonValue = analogRead(BUTTON_PIN);
    
    // Check if button is pressed
    if (buttonValue < buttonValues[0]) {
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressStartTime = millis(); // Start timer for long press
            longPressDetected = false; // Reset long press flag
        }
        // Check if button is being held down for more than 3 seconds
        if (millis() - buttonPressStartTime > 3000 && !longPressDetected) {
            longPressDetected = true; // Set long press detected flag
            if (!inSettingsMode) {
                enterSettingsMode(); // Open settings if not already in settings mode
            }
        }
    } else {
        buttonPressed = false; // Reset button pressed state
        if (longPressDetected) {
            longPressDetected = false; // Reset long press flag
        }
    }
    
    if (inMainMenu) {
        handleMainMenu(buttonValue);
    } else if (inSettingsMode) {
        handleSettingsMode(buttonValue);
    } else if (inSetR1Mode) {
        handleSetR1(buttonValue);
    } else if (inSetR2Mode) {
        handleDelayR2(buttonValue);
    } else if (inStartMode) {
        handleStartMode();
    }

    // Handle button press to navigate back
    if (buttonValue < buttonValues[0] && !buttonPressed) { // Right button
        buttonPressed = true;
        if (inSetR1Mode) {
            inSetR1Mode = false;
            inSettingsMode = true;
            displaySettingsMenu();
        } else if (inSetR2Mode) {
            inSetR2Mode = false;
            inSettingsMode = true;
            displaySettingsMenu();
        } else if (inStartMode) {
            // Turn off both relays
            digitalWrite(RELAY1_PIN, HIGH);
            digitalWrite(RELAY2_PIN, HIGH);
            lcd.clear();
            lcd.print("Relays OFF");
            delay(1000);
            buttonPressed = false;
            inMainMenu = true;
            displayMenu();
        }
    }
    if (buttonValue >= buttonValues[0]) {
        buttonPressed = false;
    }
    delay(200);
}

void handleMainMenu(int buttonValue) {
    if (buttonValue < buttonValues[1] && !buttonPressed) { 
        // No up button action needed since only one item is available
    } else if (buttonValue < buttonValues[2] && !buttonPressed) {
        // No down button action needed since only one item is available
    } else if (buttonValue < buttonValues[4] && !buttonPressed) {
        buttonPressed = true;
        // Directly start without checking selection
        digitalWrite(RELAY1_PIN, HIGH); 
        digitalWrite(RELAY2_PIN, HIGH);
        displayStartMode();
    }
}

void displayMenu() {
    lcd.clear();
    for (int i = 0; i < 2; i++) {
        lcd.setCursor(1, i);
        if (currentSelection + i < 2) { // Changed from 4 to 2
            lcd.print(menuItems[currentSelection + i]);
        }
    }
    lcd.setCursor(0, 0);
    lcd.write('>');
}

void enterSettingsMode() {
    inSettingsMode = true;
    inMainMenu = false;
    currentSettingSelection = 0;
    displaySettingsMenu();
}

void handleSettingsMode(int buttonValue) {
    if (buttonValue < buttonValues[1] && !buttonPressed) {
        if (currentSettingSelection > 0) {
            currentSettingSelection--;
            displaySettingsMenu();
        }
    } else if (buttonValue < buttonValues[2] && !buttonPressed) {
        if (currentSettingSelection < 2) {
            currentSettingSelection++;
            displaySettingsMenu();
        }
    } else if (buttonValue < buttonValues[4] && !buttonPressed) {
        buttonPressed = true;
        if (currentSettingSelection == 2) {
            inSettingsMode = false;
            inMainMenu = true;
            displayMenu();
        } else if (currentSettingSelection == 0) {
            inSetR1Mode = true;
            inSettingsMode = false;
            displaySetR1();
        } else if (currentSettingSelection == 1) {
            inSetR2Mode = true;
            inSettingsMode = false;
            displaySetR2();
        }
    }
}

// New function to display the start mode
void displayStartMode() {
    // Serial.println("start menu...");
    inMainMenu = false;
    inStartMode = true;
    lcd.clear();
    lcd.print(" Machine started ");
    delay(500);
}

// Add these declarations at the top with other global variables
unsigned long delayStartTime = 0; // Timer for delay before Relay 2 activates
float remainingDelayTime = 0.0;    // Remaining delay time before Relay 2 turns on
bool toggleState = true; // Initially set to HIGH
bool buttonPressedD = false; // Initially set to HIGH
void handleStartMode() {


    static bool delayTimerStarted = false;
    static bool relay1Completed = false;
    static bool relay2Started = false;
    static bool cycleCompleted = false;
    static bool metalDetected = false;  // Metal detection flag
    static bool cycleInProgress = false; // Flag to track if the cycle is in progress
    // int buttonValue = analogRead(BUTTON_PIN);  // Read the button value

    // Step 1: Check for metal detection using the PNP sensor pin
    if (digitalRead(PNP_SENSOR_PIN) == HIGH) {  // Metal detected
        metalDetected = true;
    }

    // Step 2: If metal is detected, start/restart the cycle
    if (metalDetected && !cycleInProgress) {
        // Metal detected and cycle is not in progress, start the cycle
        cycleInProgress = true;  // Mark that a cycle has started
        metalDetected = false;   // Reset the metal detection flag

        // Reset all flags and states to restart the cycle
        relay1Completed = false;
        relay2Started = false;
        cycleCompleted = false;
        delayTimerStarted = false;
        isRelay1Active = false;
        digitalWrite(RELAY1_PIN, HIGH);  // Turn off Relay 1
        digitalWrite(RELAY2_PIN, HIGH);  // Turn off Relay 2
        
        // Start the cycle from Relay 1
        digitalWrite(RELAY1_PIN, LOW);        // Turn on Relay 1
        relay1StartTime = millis();            // Note start time for Relay 1
        delayStartTime = relay1StartTime;      // Start delay timer simultaneously
        isRelay1Active = true;
        delayTimerStarted = true;

        // Display R1 ON status and Delay Time text
        lcd.clear();
        lcd.print("R1 ON R2 OFF ");
        lcd.setCursor(13, 0);
        lcd.print(setR1Time, 1);
        lcd.setCursor(0, 1);
        lcd.print("Delay Time     ");           // Show "Delay Time" on line 2
        lcd.setCursor(13, 1);
        lcd.print(setDelayTime, 1);
    }

    // Step 3: If Relay 1 is running, handle Relay 1 timing
    if (isRelay1Active && !relay1Completed && !cycleCompleted) {
        float elapsedR1 = (millis() - relay1StartTime) / 1000.0;
        float remainingR1 = setR1Time - elapsedR1;

        // Update LCD for Relay 1 status
        lcd.setCursor(13, 0);
        lcd.print(remainingR1 > 0 ? remainingR1 : 0.0, 1);

        // Step 4: Turn off Relay 1 when its timer completes
        if (remainingR1 <= 0 && isRelay1Active) {
            digitalWrite(RELAY1_PIN, HIGH);
            isRelay1Active = false;
            relay1Completed = true;

            // Display R1 OFF, delay countdown
            lcd.clear();
            lcd.print("R1 OFF R2 OFF");
            lcd.setCursor(0, 1);
            lcd.print("Delay Time     ");
            lcd.setCursor(13, 1);
            lcd.print(setDelayTime, 1);
        }
    }

    // Step 5: Track delay time for Relay 2
    if (delayTimerStarted) {
        float elapsedDelay = (millis() - delayStartTime) / 1000.0;
        float remainingDelayTime = setDelayTime - elapsedDelay;

        // Update LCD for delay time
        lcd.setCursor(13, 1);
        lcd.print(remainingDelayTime > 0 ? remainingDelayTime : 0.0, 1);

        // Step 6: Start Relay 2 when delay completes
        if (remainingDelayTime <= 0 && relay1Completed && !relay2Started) {
            delayTimerStarted = false;        // Stop delay timer
            digitalWrite(RELAY2_PIN, LOW);    // Turn on Relay 2
            relay2StartTime = millis();       // Note start time for Relay 2
            relay2Started = true;

            // Display R2 ON status
            lcd.clear();
            lcd.print("R1 OFF R2 ON  ");
            lcd.setCursor(13, 0);
            lcd.print(setR1Time, 1);          // Show Relay 2 time (same as R1)
        }
    }

    // Step 7: Track Relay 2’s remaining time
    if (relay2Started) {
        float elapsedR2 = (millis() - relay2StartTime) / 1000.0;
        float remainingR2 = setR1Time - elapsedR2;

        // Update LCD for Relay 2 status
        lcd.setCursor(13, 0);
        lcd.print(remainingR2 > 0 ? remainingR2 : 0.0, 1);

        // Step 8: Turn off Relay 2 and end the cycle when R2's time completes
        if (remainingR2 <= 0) {
            digitalWrite(RELAY2_PIN, HIGH);    // Turn off Relay 2
            relay2Started = false;
            cycleCompleted = true;            // Mark cycle as completed
            cycleInProgress = false;          // Reset cycle in progress flag

            // Display final OFF status for both relays
            lcd.clear();
            lcd.print("R1 OFF R2 OFF ");
            delay(1000); // Optional: brief delay to show final status
        }
        
    }

    // Step 9: If metal is detected during the cycle, restart the cycle
    if (metalDetected && cycleInProgress) {
        // Reset the entire cycle
        cycleInProgress = false;  // Mark that cycle is not in progress
        relay1Completed = false;
        relay2Started = false;
        cycleCompleted = false;
        delayTimerStarted = false;
        isRelay1Active = false;
        digitalWrite(RELAY1_PIN, HIGH);  // Turn off Relay 1
        digitalWrite(RELAY2_PIN, HIGH);  // Turn off Relay 2
        
        // Restart the cycle (from Relay 1)
        digitalWrite(RELAY1_PIN, LOW);        // Turn on Relay 1
        relay1StartTime = millis();            // Note start time for Relay 1
        delayStartTime = relay1StartTime;      // Start delay timer simultaneously
        isRelay1Active = true;
        delayTimerStarted = true;

        // Display R1 ON status and Delay Time text
        lcd.clear();
        lcd.print("R1 ON R2 OFF ");
        lcd.setCursor(13, 0);
        lcd.print(setR1Time, 1);
        lcd.setCursor(0, 1);
        lcd.print("Delay Time     ");           // Show "Delay Time" on line 2
        lcd.setCursor(13, 1);
        lcd.print(setDelayTime, 1);

        metalDetected = false;  // Reset metal detection flag to avoid unwanted multiple triggers
    }
    
}

// Display the settings menu
void displaySettingsMenu() {
    // Serial.println("display menu setting");
    lcd.clear();
    for (int i = 0; i < 2; i++) {
        lcd.setCursor(1, i);
        if (currentSettingSelection + i < 3) {
            lcd.print(settingItems[currentSettingSelection + i]);
        }
    }
    lcd.setCursor(0, 0);
    lcd.write('>');
}

void displaySetR1() {
    lcd.clear();
    lcd.print("Set R1 Time:");
    lcd.setCursor(0, 1);
    lcd.print(setR1Time, 1); // Display time with one decimal place
    lcd.print(" sec");
}

void handleSetR1(int buttonValue) {
    if (buttonValue < buttonValues[1] && !buttonPressed) { // Up button
        setR1Time += stepSize;
        if (setR1Time > 99.9) setR1Time = 99.9; // Max limit
        displaySetR1();
    } else if (buttonValue < buttonValues[2] && !buttonPressed) { // Down button
        setR1Time -= stepSize;
        if (setR1Time < 0.0) setR1Time = 0.0; // Min limit
        displaySetR1();
    } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button
        buttonPressed = true;
        // Save time to EEPROM
        EEPROM.put(EEPROM_ADDR_R1_TIME, setR1Time);
        lcd.clear();
        lcd.print("Successfully set");
        delay(2000);
        inSetR1Mode = false;
        inSettingsMode = true;
        displaySettingsMenu();
    }
}

void displaySetR2() {
    lcd.clear();
    lcd.print("Set Delay Time:");
    lcd.setCursor(0, 1);
    lcd.print(setDelayTime, 1); // Display time with one decimal place
    lcd.print(" sec");
}

void handleDelayR2(int buttonValue) {
    if (buttonValue < buttonValues[1] && !buttonPressed) { // Up button
        setDelayTime += stepSize;
        if (setDelayTime > 99.9) setDelayTime = 99.9; // Max limit
        displaySetR2();
    } else if (buttonValue < buttonValues[2] && !buttonPressed) { // Down button
        setDelayTime -= stepSize;
        if (setDelayTime < 0.0) setDelayTime = 0.0; // Min limit
        displaySetR2();
    } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button
        buttonPressed = true;
        // Save time to EEPROM
        EEPROM.put(EEPROM_ADDR_R2_TIME, setDelayTime);
        lcd.clear();
        lcd.print("Successfully set");
        delay(2000);
        inSetR2Mode = false;
        inSettingsMode = true;
        displaySettingsMenu();
    }
}
