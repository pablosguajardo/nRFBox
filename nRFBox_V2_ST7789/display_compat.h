#pragma once

/*
  Adaptador mínimo para reemplazar U8g2lib (OLED 128x64 mono) por TFT_eSPI.

  Importante (tamaño de firmware):
  - Esta versión NO usa TFT_eSprite para ahorrar Flash (código).
  - clearBuffer() limpia el área 128x64 en el TFT y sendBuffer() es no-op.

  Ventajas:
  - Reduce bastante el tamaño del binario (evita linkear TFT_eSprite).
  - Mantiene la mayoría de llamadas existentes u8g2.* sin tocar el resto del proyecto.

  Notas:
  - setContrast()/setBitmapMode() quedan como no-ops.
  - Coordenadas siguen siendo las del “lienzo” 128x64, centrado en el TFT.
  - drawXBMP usa drawXBitmap (bitmaps 1-bit tipo XBM).
*/

#include <Arduino.h>

/*
  Debe ir ANTES de TFT_eSPI.h para que USER_SETUP_LOADED (y el resto del setup local)
  aplique correctamente. Esto suele reducir MUCHO el tamaño (fuentes/feature flags).
*/
#include "ui_config.h"

#include <SPI.h>
#include <TFT_eSPI.h>

// --- Compatibilidad con constantes usadas por U8g2 ---
#ifndef U8G2_R0
#define U8G2_R0 0
#endif

#ifndef U8X8_PIN_NONE
#define U8X8_PIN_NONE -1
#endif

// --- IDs de fuentes (mapeo aproximado) ---
enum U8G2_CompatFont : uint8_t {
  U8G2_FONT_SMALL = 2, // 1: similar a 5x8 / 6x10
  U8G2_FONT_MED   = 3, // 2: ~16px alto
};

// U8g2 font symbols usados en el proyecto (mapeados a IDs anteriores)
#ifndef u8g2_font_5x8_tr
#define u8g2_font_5x8_tr U8G2_FONT_SMALL
#endif
#ifndef u8g2_font_6x10_tr
#define u8g2_font_6x10_tr U8G2_FONT_SMALL
#endif
#ifndef u8g2_font_6x10_tf
#define u8g2_font_6x10_tf U8G2_FONT_SMALL
#endif
#ifndef u8g2_font_ncenB08_tr
#define u8g2_font_ncenB08_tr U8G2_FONT_SMALL
#endif
#ifndef u8g2_font_ncenB14_tr
#define u8g2_font_ncenB14_tr U8G2_FONT_MED
#endif
#ifndef u8g2_font_profont11_tf
#define u8g2_font_profont11_tf U8G2_FONT_SMALL
#endif
#ifndef u8g_font_7x14
#define u8g_font_7x14 U8G2_FONT_MED
#endif
#ifndef u8g_font_7x14B
#define u8g_font_7x14B U8G2_FONT_MED
#endif

class U8G2_SSD1306_128X64_NONAME_F_HW_I2C : public Print {
public:
  // Firma compatible con el constructor usado en el .ino
  explicit U8G2_SSD1306_128X64_NONAME_F_HW_I2C(int /*rotation*/, int /*reset*/ = U8X8_PIN_NONE)
      : _tft() {}

  void begin() {
#if defined(TFT_BL)
    // Muchos módulos ST7789 requieren habilitar el backlight por GPIO
    pinMode(TFT_BL, OUTPUT);
  #if defined(TFT_BACKLIGHT_ON)
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  #else
    digitalWrite(TFT_BL, HIGH);
  #endif
#endif

#if defined(TFT_SCLK) && defined(TFT_MOSI) && defined(TFT_CS)
    // Forzar SPI con los pines del TFT (evita quedar en pines por defecto si otro módulo llamó SPI.begin())
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
#endif

    _tft.init();
    _tft.setRotation(0);

    // Limpiar TODO el TFT para evitar que quede "basura" (fondo blanco) fuera del
    // canvas lógico. Lo hacemos en tiras + yield() para evitar WDT.
    clearFullTft();

    // Centrar el "lienzo" lógico (NRFBOX_UI_W/NRFBOX_UI_H) en el TFT
    _originX = (_tft.width()  - SCREEN_W) / 2;
    _originY = (_tft.height() - SCREEN_H) / 2;
    if (_originX < 0) _originX = 0;
    if (_originY < 0) _originY = 0;

    setFont(U8G2_FONT_SMALL);
    setTextColor(TFT_WHITE, TFT_BLACK);
    clearBuffer();
    sendBuffer();
  }

  // U8g2 API (subset)

  void clearBuffer() {
    // Limpia SOLO el área 128x64 (la ventana emulada).
    //
    // Nota: un fillRect grande puede disparar INT_WDT en algunos ESP32/ESP32-S3
    // si el driver mantiene secciones críticas durante transfers largos.
    // Por eso limpiamos en "tiras" y cedemos CPU entre cada una.
    static constexpr int16_t STRIP_H = 8;

    for (int16_t y = 0; y < SCREEN_H; y += STRIP_H) {
      const int16_t h = (y + STRIP_H <= SCREEN_H) ? STRIP_H : (SCREEN_H - y);
      _tft.fillRect(_originX, _originY + y, SCREEN_W, h, _bg);
      yield();
    }

    _tft.setTextColor(_fg, _bg);
  }

  void sendBuffer() {
    // No-op en modo “direct draw”
  }

  void setFont(uint8_t fontId) {
    _font = fontId;
    // Con tft_setup.h cargamos sólo LOAD_GLCD (Font 1) para ahorrar Flash.
    // Por lo tanto siempre usamos la fuente interna 1.
    _tft.setTextFont(1);
  }

  void setCursor(int16_t x, int16_t y) {
    // En U8g2, 'y' suele ser baseline. En TFT_eSPI es "top" (aprox).
    int16_t topY = y - currentFontHeight();
    if (topY < 0) topY = 0;
    _tft.setCursor(_originX + x, _originY + topY);
  }

  void drawStr(int16_t x, int16_t y, const char *s) {
    setCursor(x, y);
    _tft.print(s);
  }

  int16_t getUTF8Width(const char *s) {
    // Implementación "barata" (aproximada) para NO linkear utilidades de medición
    // de texto de TFT_eSPI (reduce tamaño de programa).
    //
    // En este proyecto se usa principalmente para centrar textos del splash/menu.
    if (!s) return 0;

    // Nota: esto cuenta bytes (ASCII). Para UTF-8 real sería aproximado igualmente.
    const size_t len = strlen(s);

    // Aproximación de ancho por carácter para fuentes internas 1 y 2.
    // Font 1 ~ 6px por char (5x7 + espacio), Font 2 ~ 12px por char.
    const uint8_t cw = 6;
    return (int16_t)(len * cw);
  }

  void drawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap) {
    // Bitmaps 1-bit tipo XBM.
    //
    // Importante: la implementación de TFT_eSPI::drawXBitmap() puede ser lenta
    // para bitmaps grandes (por ejemplo 128x64 del logo) y, en algunos setups,
    // terminar disparando INT_WDT al no ceder CPU.
    //
    // Esta versión dibuja por "runs" horizontales (sólo los bits en 1) y hace
    // yield() periódicamente para evitar WDT.
    if (!bitmap || w <= 0 || h <= 0) return;

    const int16_t ax = _originX + x;
    const int16_t ay = _originY + y;
    const int bytesPerRow = (w + 7) >> 3;

    // Si no es transparente, primero pintamos el fondo del rectángulo.
    if (!_bitmapTransparent) {
      _tft.fillRect(ax, ay, w, h, _bg);
    }

    for (int16_t row = 0; row < h; ++row) {
      const uint8_t *rowPtr = bitmap + (row * bytesPerRow);

      int16_t col = 0;
      while (col < w) {
        const uint8_t b = rowPtr[col >> 3];
        const bool on = (b & (1U << (col & 7))) != 0;

        if (!on) {
          ++col;
          continue;
        }

        const int16_t start = col;
        do {
          ++col;
          if (col >= w) break;
        } while ((rowPtr[col >> 3] & (1U << (col & 7))) != 0);

        _tft.drawFastHLine(ax + start, ay + row, col - start, _fg);
      }

      // Ceder CPU cada algunas filas para evitar WDT
      if ((row & 7) == 7) yield();
    }
  }

  void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) {
    _tft.fillRect(_originX + x, _originY + y, w, h, _fg);
  }

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    _tft.drawLine(_originX + x0, _originY + y0, _originX + x1, _originY + y1, _fg);
  }

  void drawVLine(int16_t x, int16_t y, int16_t h) {
    _tft.drawFastVLine(_originX + x, _originY + y, h, _fg);
  }

  void drawPixel(int16_t x, int16_t y) {
    _tft.drawPixel(_originX + x, _originY + y, _fg);
  }

  // APIs usadas en el proyecto pero no aplican directamente en TFT:
  void setContrast(uint8_t /*contrast*/) {
    // No-op (el brillo suele manejarse por pin BL / PWM fuera de TFT_eSPI)
  }

  void setBitmapMode(uint8_t mode) {
    // U8g2:
    // - mode 0: normal (0 bits se interpretan como background)
    // - mode 1: transparente (0 bits NO dibujan)
    _bitmapTransparent = (mode == 1);
  }

  // Opcional: permitir cambiar colores si en el futuro se requiere
  void setTextColor(uint16_t fg, uint16_t bg) {
    _fg = fg;
    _bg = bg;
    _tft.setTextColor(_fg, _bg);
  }

  // Print interface: delegamos a TFT_eSPI (que ya implementa Print)
  size_t write(uint8_t c) override {
    return _tft.write(c);
  }

private:
  // Canvas lógico U8g2 (unificado en ui_config.h)
  static constexpr int16_t SCREEN_W = NRFBOX_UI_W;
  static constexpr int16_t SCREEN_H = NRFBOX_UI_H;

  void clearFullTft() {
    // Algunos drivers hacen secciones críticas largas en fillScreen/flood.
    // Esto reduce la chance de INT_WDT.
    static constexpr int16_t STRIP_H = 16;
    const int16_t w = _tft.width();
    const int16_t h = _tft.height();

    for (int16_t y = 0; y < h; y += STRIP_H) {
      const int16_t hh = (y + STRIP_H <= h) ? STRIP_H : (h - y);
      _tft.fillRect(0, y, w, hh, _bg);
      yield();
    }
  }

  int16_t currentFontHeight() const {
    // Con la fuente interna 1 (GLCD) la altura efectiva es ~8px
    return 8;
  }

  TFT_eSPI _tft;

  uint16_t _fg = TFT_WHITE;
  uint16_t _bg = TFT_BLACK;

  bool _bitmapTransparent = true; // modo por defecto: transparente (como setBitmapMode(1))

  uint8_t _font = U8G2_FONT_SMALL;

  int16_t _originX = 0;
  int16_t _originY = 0;
};
