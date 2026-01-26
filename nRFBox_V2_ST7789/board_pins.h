#pragma once

// -----------------------------------------------------------------------------
// Pines de placa (ESP32-S3) - unificados
//
// Motivo:
// En tu ESP32-S3, usar GPIO26/27/33 para botones provocaba resets ESP_RST_INT_WDT
// (probable conflicto con bus de Flash/PSRAM en tu módulo).
//
// Estos son los GPIO que confirmaste como disponibles para recablear botones:
//   UP     -> GPIO4
//   SELECT -> GPIO16
//   DOWN   -> GPIO18
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

// Botón BACK opcional (si lo cableás). Si no existe físicamente, al usar INPUT_PULLUP
// quedará siempre en HIGH y no molestará.
#ifndef NRFBOX_BTN_BACK_PIN
#define NRFBOX_BTN_BACK_PIN 25
#endif

// NeoPixel (DATA)
//
// IMPORTANTE: NO usar un pin que esté asignado al TFT (por ejemplo TFT_RST=14 en tu setup),
// porque genera conflictos.
//
// Ajustalo a tu cableado real.
#ifndef NRFBOX_NEOPIXEL_PIN
#define NRFBOX_NEOPIXEL_PIN 21
#endif
