#pragma once

// -----------------------------------------------------------------------------
// Configuración unificada de UI / Display
//
// IMPORTANTE:
// - TFT_WIDTH/TFT_HEIGHT (en tft_setup.h) = tamaño FÍSICO del display.
// - NRFBOX_UI_W/NRFBOX_UI_H = tamaño LÓGICO del "canvas" que el firmware dibuja
//   como si fuera un OLED U8g2 128x64.
//
// Hoy el proyecto está portado con un "adaptador" (display_compat.h) que emula
// un canvas 128x64 sobre el TFT 172x320. Si querés que TODO use 172x320,
// eso implica re-diseñar layouts y/o escalar coordenadas (trabajo extra).
// -----------------------------------------------------------------------------

#include "tft_setup.h"

// Tamaño "de diseño" original del proyecto (U8g2 OLED 128x64)
#ifndef NRFBOX_UI_DESIGN_W
#define NRFBOX_UI_DESIGN_W 172
#endif

#ifndef NRFBOX_UI_DESIGN_H
#define NRFBOX_UI_DESIGN_H 320
#endif

// Canvas lógico estilo U8g2 (lo que la mayoría del código asume)
// Por defecto = tamaño de diseño (128x64).
//
// Nota: si querés usar TODO el TFT (172x320) como canvas lógico, podrías poner:
//   #define NRFBOX_UI_W TFT_WIDTH
//   #define NRFBOX_UI_H TFT_HEIGHT
// pero eso requiere re-maquetar pantallas/bitmaps (no escala automático).
#ifndef NRFBOX_UI_W
#define NRFBOX_UI_W NRFBOX_UI_DESIGN_W
#endif

#ifndef NRFBOX_UI_H
#define NRFBOX_UI_H NRFBOX_UI_DESIGN_H
#endif

// Helpers (tamaño físico)
#ifndef NRFBOX_TFT_W
#define NRFBOX_TFT_W TFT_WIDTH
#endif

#ifndef NRFBOX_TFT_H
#define NRFBOX_TFT_H TFT_HEIGHT
#endif
