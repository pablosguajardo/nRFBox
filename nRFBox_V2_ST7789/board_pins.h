#pragma once

// -----------------------------------------------------------------------------
// Pines de placa (ESP32/ESP32-S3) - TODO en un solo lugar
//
// Idea:
// - Editás SOLO este archivo cuando cambiás el cableado.
// - El resto del proyecto consume estos defines (botones, TFT, NeoPixel, etc.).
//
// Nota ESP32-S3 (común en muchos módulos):
// - GPIO26..GPIO32 (y a veces otros) pueden estar usados por Flash/PSRAM.
// - Evitá usarlos para botones / periféricos si tenés resets raros (INT_WDT).
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Display TFT ST7789 (TFT_eSPI)
// Ajustar según tu cableado real
// -----------------------------------------------------------------------------
#ifndef NRFBOX_TFT_WIDTH
#define NRFBOX_TFT_WIDTH 240
#endif

#ifndef NRFBOX_TFT_HEIGHT
#define NRFBOX_TFT_HEIGHT 240
#endif

// Backlight
#ifndef NRFBOX_TFT_BL_PIN
#define NRFBOX_TFT_BL_PIN 12
#endif

#ifndef NRFBOX_TFT_BACKLIGHT_ON
#define NRFBOX_TFT_BACKLIGHT_ON HIGH
#endif

// SPI / control
#ifndef NRFBOX_TFT_MISO_PIN
#define NRFBOX_TFT_MISO_PIN -1
#endif

#ifndef NRFBOX_TFT_MOSI_PIN
#define NRFBOX_TFT_MOSI_PIN 35   // SDA
#endif

#ifndef NRFBOX_TFT_SCLK_PIN
#define NRFBOX_TFT_SCLK_PIN 19   // CLK
#endif

#ifndef NRFBOX_TFT_CS_PIN
#define NRFBOX_TFT_CS_PIN 15
#endif

#ifndef NRFBOX_TFT_DC_PIN
#define NRFBOX_TFT_DC_PIN 13
#endif

#ifndef NRFBOX_TFT_RST_PIN
#define NRFBOX_TFT_RST_PIN 14
#endif

// -----------------------------------------------------------------------------
// Botones
// -----------------------------------------------------------------------------
#ifndef NRFBOX_BTN_UP_PIN
#define NRFBOX_BTN_UP_PIN 4
#endif

#ifndef NRFBOX_BTN_SELECT_PIN
#define NRFBOX_BTN_SELECT_PIN 16
#endif

#ifndef NRFBOX_BTN_DOWN_PIN
#define NRFBOX_BTN_DOWN_PIN 18
#endif

// Botón BACK opcional (si lo cableás). Si no existe físicamente, al usar
// INPUT_PULLUP quedará siempre en HIGH y no molestará.
#ifndef NRFBOX_BTN_BACK_PIN
#define NRFBOX_BTN_BACK_PIN 36
#endif

// -----------------------------------------------------------------------------
// NeoPixel (DATA)
// IMPORTANTE: no usar un pin que esté asignado al TFT (por ejemplo RST/CS/DC),
// porque genera conflictos.
// -----------------------------------------------------------------------------
#ifndef NRFBOX_NEOPIXEL_PIN
#define NRFBOX_NEOPIXEL_PIN 21
#endif
