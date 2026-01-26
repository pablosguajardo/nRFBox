/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */

#include <EEPROM.h>
#include "display_compat.h"

#include "setting.h"
#include "board_pins.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern Adafruit_NeoPixel pixels;

#define BUTTON_UP NRFBOX_BTN_UP_PIN
#define BUTTON_DOWN NRFBOX_BTN_DOWN_PIN
#define BUTTON_SELECT NRFBOX_BTN_SELECT_PIN

#define EEPROM_ADDRESS_NEOPIXEL 0
#define EEPROM_ADDRESS_BRIGHTNESS 1

int currentOption = 0;
int totalOptions = 2;
bool neoPixelActive = false;
uint8_t oledBrightness = 100;

bool buttonUpPressed = false;
bool buttonDownPressed = false;
bool buttonSelectPressed = false;

void toggleOption(int option) {
  if (option == 0) { 
    neoPixelActive = !neoPixelActive;
    EEPROM.write(EEPROM_ADDRESS_NEOPIXEL, neoPixelActive ? 1 : 0);
    EEPROM.commit();
    Serial.print("NeoPixel is now ");
    Serial.println(neoPixelActive ? "Enabled" : "Disabled");
  } else if (option == 1) { 
    uint8_t brightnessPercent = map(oledBrightness, 0, 255, 0, 100); // Map to 0-100
    brightnessPercent += 10; // Increment brightness by 10%
    if (brightnessPercent > 100) brightnessPercent = 0; // Wrap around to 0
    oledBrightness = map(brightnessPercent, 0, 100, 0, 255); // Map back to 0-255

    u8g2.setContrast(oledBrightness); // Apply the brightness
    EEPROM.write(EEPROM_ADDRESS_BRIGHTNESS, oledBrightness);
    EEPROM.commit();

    Serial.print("Brightness set to: ");
    Serial.print(brightnessPercent);
    Serial.println("%");
  }
}

bool handleButtons() {
  bool changed = false;

  if (!digitalRead(BUTTON_UP)) {
    if (!buttonUpPressed) {
      buttonUpPressed = true;
      currentOption = (currentOption - 1 + totalOptions) % totalOptions;
      changed = true;
    }
  } else {
    buttonUpPressed = false;
  }

  if (!digitalRead(BUTTON_DOWN)) {
    if (!buttonDownPressed) {
      buttonDownPressed = true;
      currentOption = (currentOption + 1) % totalOptions;
      changed = true;
    }
  } else {
    buttonDownPressed = false;
  }

  if (!digitalRead(BUTTON_SELECT)) {
    if (!buttonSelectPressed) {
      buttonSelectPressed = true;
      toggleOption(currentOption);
      changed = true;
    }
  } else {
    buttonSelectPressed = false;
  }

  return changed;
}

void displayMenu() {
  u8g2.clearBuffer();

  // Draw menu header
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Settings Menu");

  // Draw menu options
  if (currentOption == 0) {
    u8g2.drawStr(0, 25, "> NeoPixel: ");
  } else {
    u8g2.drawStr(0, 25, "  NeoPixel: ");
  }

  if (currentOption == 1) {
    u8g2.drawStr(0, 40, "> Brightness: ");
  } else {
    u8g2.drawStr(0, 40, "  Brightness: ");
  }

  // Show current settings
  u8g2.setCursor(80, 25);
  u8g2.print(neoPixelActive ? "Enabled" : "Disabled");

  u8g2.setCursor(80, 40);
  uint8_t brightnessPercent = map(oledBrightness, 0, 255, 0, 100);
  u8g2.print(brightnessPercent);
  u8g2.print("%");

  u8g2.sendBuffer();
}

void settingSetup() {
  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(512);

  // Load settings from EEPROM
  uint8_t neoRaw = EEPROM.read(EEPROM_ADDRESS_NEOPIXEL);
  neoPixelActive = (neoRaw == 1); // sólo 1 => enabled, cualquier otro => disabled

  oledBrightness = EEPROM.read(EEPROM_ADDRESS_BRIGHTNESS);
  if (oledBrightness == 0xFF) oledBrightness = 128; // EEPROM virgen -> default razonable

  u8g2.setContrast(oledBrightness);

  // Initialize buttons
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
}

void settingLoop() {
  static bool first = true;
  static int lastOption = -1;
  static bool lastNeo = false;
  static uint8_t lastBright = 0;

  bool changed = handleButtons();

  if (first || changed || currentOption != lastOption || neoPixelActive != lastNeo || oledBrightness != lastBright) {
    displayMenu();
    lastOption = currentOption;
    lastNeo = neoPixelActive;
    lastBright = oledBrightness;
    first = false;
  }

  // Evita que el loop dibuje “a máxima velocidad” (reduce flicker y carga SPI)
  delay(10);
}
