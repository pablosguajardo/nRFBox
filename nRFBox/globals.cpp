/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */

#include "config.h"

// Definiciones únicas (deben existir en UN solo .cpp en todo el proyecto)

// OLED (U8g2)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// NeoPixel
Adafruit_NeoPixel pixels(1, 14, NEO_GRB + NEO_KHZ800);

// Settings globales
bool neoPixelActive = false;
uint8_t oledBrightness = 100;
