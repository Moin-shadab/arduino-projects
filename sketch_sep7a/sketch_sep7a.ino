#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

// Pin definitions
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 

// LCD and Keypad
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Relay pins
int relayPins[3] = {3, 4, 5};
int relayStates[3] = {LOW, LOW, LOW};

// Timer variables
unsigned int onTime = 60;   // Default 1 min
unsigned int offTime = 120; // Default 2 min
bool manualMode = false;
bool settingsChanged = false;

// Valid ranges
const unsigned int MIN_TIME = 10;    // Minimum time in seconds
const unsigned int MAX_TIME = 3600;  // Maximum time in seconds
const unsigned long DEBOUNCE_DELAY = 50; // Debounce time in milliseconds
const unsigned long INPUT_TIMEOUT = 10000; // Timeout after 10 seconds

void setup() {
  lcd.begin(16, 2);
  for(int i = 0; i < 3; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], relayStates[i]);
  }
  
  // Load saved settings from EEPROM with validation
  loadSettings();
  
  displayMenu();
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    switch(key) {
      case '1': // Start Timer
        if (manualMode) {
          manualMode = false;
          displayMenu();
        } else {
          startTimer();
        }
        break;
      case '2': // Manual Mode
        manualMode = true;
        manualModeMenu();
        break;
      case '3': // Settings
        settingsMenu();
        break;
    }
  }

  if (settingsChanged) {
    // Save settings to EEPROM if changed
    saveSettings();
    settingsChanged = false;
  }
}

void loadSettings() {
  unsigned int loadedOnTime = EEPROM.read(0) + (EEPROM.read(1) << 8);
  unsigned int loadedOffTime = EEPROM.read(2) + (EEPROM.read(3) << 8);
  
  onTime = (loadedOnTime >= MIN_TIME && loadedOnTime <= MAX_TIME) ? loadedOnTime : 60;
  offTime = (loadedOffTime >= MIN_TIME && loadedOffTime <= MAX_TIME) ? loadedOffTime : 120;
}

void saveSettings() {
  EEPROM.write(0, onTime & 0xFF);
  EEPROM.write(1, (onTime >> 8) & 0xFF);
  EEPROM.write(2, offTime & 0xFF);
  EEPROM.write(3, (offTime >> 8) & 0xFF);
}

void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1: Start Timer");
  lcd.setCursor(0, 1);
  lcd.print("2: Manual Mode");
  lcd.setCursor(0, 2);
  lcd.print("3: Settings");
}

void startTimer() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timer Active");

  unsigned long onMillis = millis();
  unsigned long offMillis = millis();
  bool relaysState = LOW;

  while (!manualMode) {
    unsigned long currentMillis = millis();

    if (currentMillis - onMillis >= (onTime * 1000)) {
      relaysState = HIGH;
      toggleRelays(relaysState);
      onMillis = currentMillis;  // Reset timer
    }

    if (currentMillis - offMillis >= (offTime * 1000)) {
      relaysState = LOW;
      toggleRelays(relaysState);
      offMillis = currentMillis;  // Reset timer
    }
  }
  displayMenu();
}

void manualModeMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Manual Mode");
  lcd.setCursor(0, 1);
  lcd.print("1: Relay 1");
  lcd.setCursor(0, 2);
  lcd.print("2: Relay 2");
  lcd.setCursor(0, 3);
  lcd.print("3: Relay 3");

  char key;
  unsigned long lastDebounceTime = 0;
  
  while (manualMode) {
    key = keypad.getKey();
    if (key) {
      if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (key >= '1' && key <= '3') {
          int relayIndex = key - '1';
          relayStates[relayIndex] = !relayStates[relayIndex];
          digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);
        } else if (key == '*') {
          manualMode = false;
          displayMenu();
        }
        lastDebounceTime = millis();
      }
    }
  }
}

void settingsMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("On Time (sec): ");
  lcd.setCursor(0, 1);
  lcd.print(onTime);

  unsigned long inputStartMillis = millis();
  char key;
  
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        onTime = (onTime * 10) + (key - '0'); // Append digit
        if (onTime > MAX_TIME) onTime = MAX_TIME; // Cap at max value
        lcd.setCursor(0, 1);
        lcd.print("        "); // Clear previous value
        lcd.setCursor(0, 1);
        lcd.print(onTime);
        inputStartMillis = millis();
      } else if (key == '#') {
        break;
      }
    }
    if (millis() - inputStartMillis > INPUT_TIMEOUT) { // Timeout after 10 seconds
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Timeout!");
      delay(2000);
      return;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Off Time (sec): ");
  lcd.setCursor(0, 1);
  lcd.print(offTime);

  inputStartMillis = millis();
  
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        offTime = (offTime * 10) + (key - '0'); // Append digit
        if (offTime > MAX_TIME) offTime = MAX_TIME; // Cap at max value
        lcd.setCursor(0, 1);
        lcd.print("        "); // Clear previous value
        lcd.setCursor(0, 1);
        lcd.print(offTime);
        inputStartMillis = millis();
      } else if (key == '#') {
        break;
      }
    }
    if (millis() - inputStartMillis > INPUT_TIMEOUT) { // Timeout after 10 seconds
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Timeout!");
      delay(2000);
      return;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Settings Saved");
  settingsChanged = true;
  delay(2000);  // Brief delay to show message
}

void toggleRelays(bool state) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(relayPins[i], state); 
  }
}