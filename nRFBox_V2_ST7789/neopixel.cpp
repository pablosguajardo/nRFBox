/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */
   
#include "neopixel.h"
#include "setting.h"
#include <EEPROM.h>

extern Adafruit_NeoPixel pixels;

void neopixelSetup() {
  EEPROM.begin(512);
  uint8_t neoRaw = EEPROM.read(0);
  neoPixelActive = (neoRaw == 1); // sólo 1 => enabled, 0xFF (virgen) => disabled
  
 if (neoPixelActive) {
  pixels.begin();
  pixels.clear();
  //pixels.show();
  }
}

void neopixelLoop();

void setNeoPixelColour(const std::string& colour) {
  // IMPORTANTE: si está deshabilitado, no tocar el pin (GPIO14).
  // Esto evita interferir con TFT_RST cuando comparten pin.
  if (!neoPixelActive) return;

  uint32_t colorValue = 0;

  if (colour == "red") {
    colorValue = pixels.Color(5, 0, 0);
  } else if (colour == "green") {
    colorValue = pixels.Color(0, 5, 0);
  } else if (colour == "blue") {
    colorValue = pixels.Color(0, 0, 5);
  } else if (colour == "yellow") {
    colorValue = pixels.Color(5, 5, 0);
  } else if (colour == "purple") {
    colorValue = pixels.Color(5, 0, 5);
  } else if (colour == "cyan") {
    colorValue = pixels.Color(0, 5, 5);
  } else if (colour == "white") {
    colorValue = pixels.Color(5, 5, 5);
  } else if (colour == "null") {
    colorValue = pixels.Color(0, 0, 0);
  } else {
    // Cualquier valor desconocido => apagado
    colorValue = pixels.Color(0, 0, 0);
  }

  pixels.setPixelColor(0, colorValue);
  pixels.show();
}

void flash(int numberOfFlashes, const std::vector<std::string>& colors, const std::string& finalColour) {
  if (!neoPixelActive) return;

  if (numberOfFlashes <= 0 || colors.empty()) {
    Serial.println("Invalid parameters for flash: Check numberOfFlashes or colors vector.");
    return;
  }

  for (int i = 0; i < numberOfFlashes; ++i) {
    for (const auto& color : colors) {
      setNeoPixelColour(color);
      delay(500);
    }
  }
  setNeoPixelColour(finalColour);
}
