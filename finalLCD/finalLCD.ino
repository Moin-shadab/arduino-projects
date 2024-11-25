#include <LiquidCrystal.h>
#include <EEPROM.h> // To store the settings in non-volatile memory

// Pin definitions for LCD
const int LCD_RS = 8;
const int LCD_ENABLE = 9;
const int LCD_D4 = 4;
const int LCD_D5 = 5;
const int LCD_D6 = 6; 
const int LCD_D7 = 7;
const int BACKLIGHT = 10;
const int BUTTON_PIN = A0; // Button connected to A0 pin
// testing    
// Relay pin definitions
const int RELAY1_PIN = 2;
const int RELAY2_PIN = 3;
const int RELAY3_PIN = 4;

// Initialize the LCD with the interface pins
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Button value thresholds (based on analog values read)
int buttonValues[5] = {50, 200, 400, 600, 800}; // Approximate thresholds for Right, Up, Down, Left, Select
int currentSelection = 0; // 0: Start Machine, 1: Manual Start, 2: Settings
int topItem = 0; // Tracks the top item displayed
char* menuItems[3] = {"1: Start Machine", "2: Manual Start", "3: Settings"};
bool machineStarted = false; // Track if the machine is running
bool inManualMode = false; // Track if we're in manual mode
bool inSettingsMode = false; // Track if we're in the settings menu
int currentRelaySelection = 0; // For selecting relays in manual mode
char* relayItems[4] = {"Relay 1", "Relay 2", "Relay 3", "Back"}; // Added "Back" option

// Settings variables
int onTime = 5; // Default On Time in minutes
int offTime = 3; // Default Off Time in minutes
char* settingItems[3] = {"Set On Time", "Set Off Time", "Back"};
int currentSettingSelection = 0; // For settings menu navigation
bool settingOnTime = true; // Track whether setting On Time or Off Time

// Button press state
bool buttonPressed = false;
bool adjustTimeMode = false; // Flag for adjusting time setting
bool adjustOnTime = true; // True if adjusting On Time, false for Off Time

// Machine state
bool machinePaused = false; // Track if the machine is paused
unsigned long lastRunTime = 0; // Track when the machine was last running
unsigned long runDuration = 0; // Track how long the machine has been running
unsigned long onEndTime = 0; // Track when the on time ends
unsigned long offEndTime = 0; // Track when the off time ends
bool isOnTime = true; // Track whether the current period is On Time or Off Time
bool wasOnTime = true; // Track whether it was On Time or Off Time before pausing
bool relaysOn = false; // Track if relays are on

// Company Name
const char companyName[] = "AUMAUTOMATION ENGINEERING By MULTAN AHMED"; // Your company name
const int scrollSpeed = 200; // Adjust this value to change the scrolling speed

void setup() {
  lcd.begin(16, 2);
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  // Set relay pins as output
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);

  // Initialize relays to be off
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);

  // Load settings from EEPROM
  onTime = EEPROM.read(0); // Stored On Time
  offTime = EEPROM.read(1); // Stored Off Time
  if (onTime == 255) onTime = 5; // Default value if EEPROM is empty
  if (offTime == 255) offTime = 3; // Default value if EEPROM is empty

  // Display company name with scrolling effect
  displayScrollingCompanyName();
  delay(7000); // Show company name for 7 seconds

  displayMenu(); // Show the menu
}

void loop() {
  int buttonValue = analogRead(BUTTON_PIN); // Read the analog value from A0

  if (adjustTimeMode) {
    handleAdjustTime(buttonValue); // Handle time adjustment logic
  } else if (inManualMode) {
    handleManualMode(buttonValue); // Handle manual mode logic
  } else if (inSettingsMode) {
    handleSettingsMode(buttonValue); // Handle settings mode logic
  } else {
    handleMainMenu(buttonValue); // Handle main menu logic
  }

  // Manage machine state if it's running
  if (machineStarted) {
    unsigned long currentTime = millis();
    unsigned long remainingTime;

    if (machinePaused) {
      // Machine is paused, display status
      lcd.setCursor(0, 1);
      lcd.print("Paused       ");
      // Hold the relay state while paused
      if (relaysOn) {
        digitalWrite(RELAY1_PIN, HIGH);
        digitalWrite(RELAY2_PIN, HIGH);
        digitalWrite(RELAY3_PIN, HIGH);
      } else {
        digitalWrite(RELAY1_PIN, LOW);
        digitalWrite(RELAY2_PIN, LOW);
        digitalWrite(RELAY3_PIN, LOW);
      }
    } else {
      if (isOnTime) {
        remainingTime = (onEndTime - currentTime) / 1000; // Remaining time in seconds
        if (remainingTime <= 0) {
          // Time's up for On Time, switch to Off Time
          digitalWrite(RELAY1_PIN, LOW);
          digitalWrite(RELAY2_PIN, LOW);
          digitalWrite(RELAY3_PIN, LOW);
          relaysOn = false;
          isOnTime = false;
          offEndTime = millis() + offTime * 60000; // Set end time for Off Time
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Off Time     ");
        } else {
          // Display remaining On Time
          lcd.setCursor(0, 0);
          lcd.print("On Time: ");
          displayTime(remainingTime);
        }
      } else {
        remainingTime = (offEndTime - currentTime) / 1000; // Remaining time in seconds
        if (remainingTime <= 0) {
          // Time's up for Off Time, switch to On Time
          digitalWrite(RELAY1_PIN, HIGH);
          digitalWrite(RELAY2_PIN, HIGH);
          digitalWrite(RELAY3_PIN, HIGH);
          relaysOn = true;
          isOnTime = true;
          onEndTime = millis() + onTime * 60000; // Set end time for On Time
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("On Time      ");
        } else {
          // Display remaining Off Time
          lcd.setCursor(0, 0);
          lcd.print("Off Time: ");
          displayTime(remainingTime);
        }
      }
    }
  }

  // Pause or resume the machine when the right button is pressed
  if (buttonValue < buttonValues[0] && !buttonPressed) { // Right button pressed
    buttonPressed = true;
    if (machineStarted) {
      machinePaused = !machinePaused;
      if (machinePaused) {
        // Record state before pausing
        wasOnTime = isOnTime;
        relaysOn = (digitalRead(RELAY1_PIN) == HIGH);
        // Stop the countdown
        runDuration = millis() - lastRunTime;
      } else {
        // Resume countdown
        if (wasOnTime) {
          onEndTime = millis() + onTime * 60000 - runDuration;
        } else {
          offEndTime = millis() + offTime * 60000 - runDuration;
        }
      }
      lcd.clear();
      lcd.print(machinePaused ? "Paused       " : "Running      ");
      delay(2000); // Show pause/resume message for 2 seconds
      lcd.clear();
    }
  }

  if (buttonValue >= buttonValues[0]) {
    buttonPressed = false; // Reset button pressed state when button is released
  }

  delay(200); // Small delay to avoid multiple presses
}

void displayScrollingCompanyName() {
  int length = strlen(companyName);
  for (int position = 0; position < length - 15; position++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(companyName + position);
    delay(scrollSpeed);
  }
}

void handleMainMenu(int buttonValue) {
  // Move the cursor based on button input
  if (buttonValue < buttonValues[1]) { // Up button pressed
    if (currentSelection > 0) {
      currentSelection--;
      displayMenu();
    }
  } else if (buttonValue < buttonValues[2]) { // Down button pressed
    if (currentSelection < 2) {
      currentSelection++;
      displayMenu();
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed
    buttonPressed = true;
    switch (currentSelection) {
      case 0:
        startMachine();
        break;
      case 1:
        enterManualMode();
        break;
      case 2:
        enterSettingsMode();
        break;
    }
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false;
  }
}

void displayMenu() {
  lcd.clear();
  for (int i = 0; i < 2; i++) {
    lcd.setCursor(1, i);
    lcd.print(menuItems[currentSelection + i]);
  }
  lcd.setCursor(0, 0);
  lcd.write('>');
}

void startMachine() {
  machineStarted = true;
  machinePaused = false;
  lastRunTime = millis();
  onEndTime = lastRunTime + onTime * 60000; // Set end time for On Time
  offEndTime = 0; // Reset Off End Time
  isOnTime = true;
  lcd.clear();
  lcd.print("Machine Started");
  delay(2000); // Show "Started" message for 2 seconds
  lcd.clear();
}

void stopMachine() {
  machineStarted = false;
  machinePaused = false; // Ensure machine is not paused when stopped
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  lcd.clear();
  lcd.print("Machine Stopped");
  delay(2000); // Show "Stopped" message for 2 seconds
  lcd.clear();
  displayMenu(); // Return to menu
}

void enterManualMode() {
  inManualMode = true;
  currentRelaySelection = 0;
  displayRelayMenu();
}

void handleManualMode(int buttonValue) {
  if (buttonValue < buttonValues[1]) { // Up button pressed
    if (currentRelaySelection > 0) {
      currentRelaySelection--;
      displayRelayMenu();
    }
  } else if (buttonValue < buttonValues[2]) { // Down button pressed
    if (currentRelaySelection < 3) { // 3 is the "Back" option
      currentRelaySelection++;
      displayRelayMenu();
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed
    buttonPressed = true;

    if (currentRelaySelection == 3) { // "Back" option selected
      inManualMode = false;
      displayMenu();
    } else {
      toggleRelay(currentRelaySelection); // Toggle the selected relay
    }
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false;
  }
}

void displayRelayMenu() {
  lcd.clear();
  for (int i = 0; i < 2; i++) {
    lcd.setCursor(1, i);
    if (currentRelaySelection + i < 4) {
      lcd.print(relayItems[currentRelaySelection + i]);
    }
  }
  lcd.setCursor(0, 0);
  lcd.write('>');
}

void toggleRelay(int relay) {
  switch (relay) {
    case 0:
      digitalWrite(RELAY1_PIN, !digitalRead(RELAY1_PIN));
      lcd.clear();
      lcd.print(digitalRead(RELAY1_PIN) ? "R1 Started" : "R1 Stopped");
      break;
    case 1:
      digitalWrite(RELAY2_PIN, !digitalRead(RELAY2_PIN));
      lcd.clear();
      lcd.print(digitalRead(RELAY2_PIN) ? "R2 Started" : "R2 Stopped");
      break;
    case 2:
      digitalWrite(RELAY3_PIN, !digitalRead(RELAY3_PIN));
      lcd.clear();
      lcd.print(digitalRead(RELAY3_PIN) ? "R3 Started" : "R3 Stopped");
      break;
  }
  delay(1000); // Wait before returning to the relay menu
  displayRelayMenu();
}

void enterSettingsMode() {
  inSettingsMode = true;
  currentSettingSelection = 0; // Start with "Set On Time" selected
  displaySettingsMenu();
}

void handleSettingsMode(int buttonValue) {
  if (buttonValue < buttonValues[1]) { // Up button pressed
    if (currentSettingSelection > 0) {
      currentSettingSelection--;
      displaySettingsMenu();
    }
  } else if (buttonValue < buttonValues[2]) { // Down button pressed
    if (currentSettingSelection < 2) { // Includes "Back" option at index 2
      currentSettingSelection++;
      displaySettingsMenu();
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed
    buttonPressed = true;

    if (currentSettingSelection == 2) { // "Back" option selected
      inSettingsMode = false;
      displayMenu();
    } else {
      adjustOnTime = (currentSettingSelection == 0);
      adjustTimeMode = true;
      displayAdjustTime();
    }
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false;
  }
}

void displaySettingsMenu() {
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

void displayAdjustTime() {
  lcd.clear();
  lcd.print(adjustOnTime ? "Set On Time:" : "Set Off Time:");
  lcd.setCursor(0, 1);
  lcd.print(adjustOnTime ? onTime : offTime);
}

void handleAdjustTime(int buttonValue) {
  if (buttonValue < buttonValues[1]) { // Up button pressed
    if (adjustOnTime) {
      onTime++;
      EEPROM.write(0, onTime);
    } else {
      offTime++;
      EEPROM.write(1, offTime);
    }
    displayAdjustTime();
  } else if (buttonValue < buttonValues[2]) { // Down button pressed
    if (adjustOnTime) {
      if (onTime > 0) onTime--;
      EEPROM.write(0, onTime);
    } else {
      if (offTime > 0) offTime--;
      EEPROM.write(1, offTime);
    }
    displayAdjustTime();
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed
    buttonPressed = true;
    adjustTimeMode = false;
    displaySettingsMenu();
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false;
  }
}

void displayTime(unsigned long remainingTime) {
  unsigned long minutes = remainingTime / 60;
  unsigned long seconds = remainingTime % 60;
  lcd.print(minutes);
  lcd.print(" M ");
  lcd.print(seconds);
  lcd.print(" S ");
}
