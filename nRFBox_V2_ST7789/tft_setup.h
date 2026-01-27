// Config local de TFT_eSPI para nRFBox_V2 (recortada para reducir Flash)
// Este archivo, si existe, tiene prioridad sobre User_Setup_Select.h
#pragma once

// Centralizamos pines en un solo archivo
#include "board_pins.h"

// Evita que TFT_eSPI cargue otros setups
#define USER_SETUP_LOADED

// Driver del display
#define ST7789_DRIVER

// Orden de color
#define TFT_RGB_ORDER TFT_BGR

// Tamaño del display
#define TFT_WIDTH  NRFBOX_TFT_WIDTH
#define TFT_HEIGHT NRFBOX_TFT_HEIGHT

// Backlight
#define TFT_BL           NRFBOX_TFT_BL_PIN
#define TFT_BACKLIGHT_ON NRFBOX_TFT_BACKLIGHT_ON

// Pines SPI / control (según tu cableado)
#define TFT_MISO NRFBOX_TFT_MISO_PIN
#define TFT_MOSI NRFBOX_TFT_MOSI_PIN   // SDA
#define TFT_SCLK NRFBOX_TFT_SCLK_PIN   // CLK
#define TFT_CS   NRFBOX_TFT_CS_PIN
#define TFT_DC   NRFBOX_TFT_DC_PIN
#define TFT_RST  NRFBOX_TFT_RST_PIN

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

// Frecuencia SPI (bajar un poco ayuda a evitar problemas de señal / WDT)
#define SPI_FREQUENCY 20000000
