// Config local de TFT_eSPI para nRFBox_V2 (recortada para reducir Flash)
// Este archivo, si existe, tiene prioridad sobre User_Setup_Select.h
#pragma once

// Evita que TFT_eSPI cargue otros setups
#define USER_SETUP_LOADED

// Driver del display
#define ST7789_DRIVER

// Orden de color
#define TFT_RGB_ORDER TFT_BGR

// Tamaño del display (según tu setup actual)
#define TFT_WIDTH  170
#define TFT_HEIGHT 320

// Backlight
#define TFT_BL           12
#define TFT_BACKLIGHT_ON HIGH

// Pines SPI / control (según Setup72_ESP32_ST7789_172x320.h)
#define TFT_MISO -1
#define TFT_MOSI 35
#define TFT_SCLK 19
#define TFT_CS   15
#define TFT_DC   13
#define TFT_RST  14

// ---- Fuentes: MINIMO indispensable para ahorrar Flash ----
// Sólo Font 1 (GLCD). Font2/4/6/7/8 + FreeFonts + Smooth fonts desactivados.
#define LOAD_GLCD

// Asegurar que NO se activen por otro lado
#ifdef LOAD_FONT2
  #undef LOAD_FONT2
#endif
#ifdef LOAD_FONT4
  #undef LOAD_FONT4
#endif
#ifdef LOAD_FONT6
  #undef LOAD_FONT6
#endif
#ifdef LOAD_FONT7
  #undef LOAD_FONT7
#endif
#ifdef LOAD_FONT8
  #undef LOAD_FONT8
#endif
#ifdef LOAD_FONT8N
  #undef LOAD_FONT8N
#endif
#ifdef LOAD_GFXFF
  #undef LOAD_GFXFF
#endif
#ifdef SMOOTH_FONT
  #undef SMOOTH_FONT
#endif

// Frecuencia SPI
#define SPI_FREQUENCY 27000000
