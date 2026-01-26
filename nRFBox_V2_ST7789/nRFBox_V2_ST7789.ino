/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */

#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#include <esp_system.h>

#include <SPI.h>
#include "display_compat.h"
#include "ui_config.h"
#include "board_pins.h"

#include "icon.h"
#include "neopixel.h"
#include "setting.h"

#include "scanner.h"
#include "analyzer.h"
#include "jammer.h"
#include "blejammer.h"
#include "spoofer.h"
#include "sourapple.h"
#include "blescan.h"
#include "wifiscan.h"
#include "blackout.h"

/*
  RF24 globales deshabilitados

  Motivo:
  - Los módulos (jammer/blejammer/blackout/etc.) ya crean y usan sus propios RF24.
  - Estos RF24 globales inicializan pines al arranque y pueden chocar con el TFT.
    Ej: CE_PIN_C=15 pisa TFT_CS=15 => pantalla en negro / no inicializa.
*/
//#define CE_PIN_A  5
//#define CSN_PIN_A 20
//
//#define CE_PIN_B  16
//#define CSN_PIN_B 4
//
//#define CE_PIN_C  15
//#define CSN_PIN_C 2

#define BUTTON_UP_PIN NRFBOX_BTN_UP_PIN
// Pines unificados en board_pins.h (evita GPIO conflictivos en ESP32-S3)
#define BUTTON_SELECT_PIN NRFBOX_BTN_SELECT_PIN
#define BUTTON_DOWN_PIN NRFBOX_BTN_DOWN_PIN

// Diagnóstico
// - NRFBOX_DIAG_HOLD_AFTER_RESET: congela después de mostrar el motivo de reset.
// - NRFBOX_SKIP_SPLASH: salta pantallas de splash/logo para aislar INT_WDT.
#define NRFBOX_DIAG_HOLD_AFTER_RESET 0
#define NRFBOX_SKIP_SPLASH 0

// Diagnóstico UI: deshabilita bitmaps del menú (XBMP) para ver si el INT_WDT
// viene de las rutinas de dibujo (muy probable en algunos ST7789/S3).
#define NRFBOX_UI_NO_BITMAPS 0

// Diagnóstico: traza por etapas en pantalla (muy liviano) para ubicar EXACTAMENTE
// en qué punto ocurre el INT_WDT.
#define NRFBOX_DIAG_STAGE_TRACE 0

// Diagnóstico HW (ESP32-S3):
// En muchos módulos ESP32-S3, GPIO26..GPIO32 están usados por la Flash/PSRAM.
// Si configuramos GPIO26/27 como botones, podemos romper el fetch de código y caer en INT_WDT.
// Con esto deshabilitamos botones para confirmar si el WDT viene por ahí.
#define NRFBOX_SKIP_BUTTONS 0

// Diagnóstico: si está en 1, se congela inmediatamente después de dibujar el menú.
// (lo dejamos en 0 para que pueda avanzar y ver hasta dónde llega)
#define NRFBOX_DIAG_HOLD_AFTER_MENU_DRAW 0

//RF24 RadioA(CE_PIN_A, CSN_PIN_A);
//RF24 RadioB(CE_PIN_B, CSN_PIN_B);
//RF24 RadioC(CE_PIN_C, CSN_PIN_C);

//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0); // [full framebuffer, size = 1024 bytes]
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_NeoPixel pixels(1, NRFBOX_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

extern uint8_t oledBrightness;

const unsigned char* bitmap_icons[11] = {
  bitmap_icon_scanner,
  bitmap_icon_analyzer,
  bitmap_icon_jammer,
  bitmap_icon_kill,
  bitmap_icon_ble_jammer,
  bitmap_icon_spoofer,
  bitmap_icon_apple,
  bitmap_icon_ble,
  bitmap_icon_wifi,
  bitmap_icon_about,
  bitmap_icon_setting
};

const int NUM_ITEMS = 11;
const int MAX_ITEM_LENGTH = 20;

char menu_items[NUM_ITEMS][MAX_ITEM_LENGTH] = {
  { "Scanner" },
  { "Analyzer" },
  { "WLAN Jammer" },
  { "Proto Kill" },
  { "BLE Jammer" },
  { "BLE Spoofer" },
  { "Sour Apple" },
  { "BLE Scan" },
  { "WiFi Scan" },
  { "About" },
  { "Setting" }
};

int button_up_clicked = 0;
int button_select_clicked = 0;
int button_down_clicked = 0;

int item_selected = 0;

int item_sel_previous;
int item_sel_next;

// -----------------------------------------------------------------------------
// Máquina de estados (NO bloqueante)
// Motivo: los while() bloqueantes podían disparar WDT y resetear si el botón
// SELECT quedaba en LOW/flotante al arranque.
// -----------------------------------------------------------------------------
enum ScreenId : uint8_t {
  SCREEN_MENU = 0,
  SCREEN_SCANNER,
  SCREEN_ANALYZER,
  SCREEN_JAMMER,
  SCREEN_BLACKOUT,
  SCREEN_BLE_JAMMER,
  SCREEN_SPOOFER,
  SCREEN_SOURAPPLE,
  SCREEN_BLESCAN,
  SCREEN_WIFISCAN,
  SCREEN_ABOUT,
  SCREEN_SETTING,
};

static ScreenId activeScreen = SCREEN_MENU;
static bool screenNeedsSetup = false;

static const char* resetReasonToStr(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN:   return "UNKNOWN";
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXT";
    case ESP_RST_SW:        return "SW";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "OTHER";
  }
}

static ScreenId screenFromMenuItem(int item) {
  switch (item) {
    case 0: return SCREEN_SCANNER;
    case 1: return SCREEN_ANALYZER;
    case 2: return SCREEN_JAMMER;
    case 3: return SCREEN_BLACKOUT;
    case 4: return SCREEN_BLE_JAMMER;
    case 5: return SCREEN_SPOOFER;
    case 6: return SCREEN_SOURAPPLE;
    case 7: return SCREEN_BLESCAN;
    case 8: return SCREEN_WIFISCAN;
    case 9: return SCREEN_ABOUT;
    case 10: return SCREEN_SETTING;
    default: return SCREEN_MENU;
  }
}

void about() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 15, "CiferTech@gmail.com");
  u8g2.drawStr(21, 35, "GitHub/cifertech");
  u8g2.drawStr(0, 55, "instagram/cifertech");
  u8g2.sendBuffer();
}

void configureNrf(RF24 &radio) {
  radio.begin();
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setDataRate(RF24_2MBPS);
  radio.setCRCLength(RF24_CRC_DISABLED);
}

static void drawMenu() {
  item_sel_previous = item_selected - 1;
  if (item_sel_previous < 0) {
    item_sel_previous = NUM_ITEMS - 1;
  }
  item_sel_next = item_selected + 1;
  if (item_sel_next >= NUM_ITEMS) {
    item_sel_next = 0;
  }

  u8g2.clearBuffer();

  // UI original estaba diseñada para un “canvas” 128x64.
  // Si NRFBOX_UI_W/H cambia, centramos ese layout dentro del canvas actual.
  const int16_t baseX = (NRFBOX_UI_W - NRFBOX_UI_DESIGN_W) / 2;
  const int16_t baseY = (NRFBOX_UI_H - NRFBOX_UI_DESIGN_H) / 2;

#if NRFBOX_UI_NO_BITMAPS
  // Menú minimalista (solo texto) para evitar WDT por dibujo de XBM
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(baseX + 0, baseY + 12, menu_items[item_sel_previous]);

  u8g2.drawStr(baseX + 0, baseY + 32, ">");
  u8g2.drawStr(baseX + 12, baseY + 32, menu_items[item_selected]);

  u8g2.drawStr(baseX + 0, baseY + 52, menu_items[item_sel_next]);

  u8g2.sendBuffer();
  return;
#endif

  // En TFT, algunos bitmaps del menú (outline/scrollbar) se ven con artefactos
  // (puntitos, “flechitas” pequeñas). En lugar de XBM, dibujamos la selección y
  // el scrollbar con primitivas: queda más limpio y legible.
  const int16_t selX = baseX + 0;
  const int16_t selY = baseY + 22;
  const int16_t selW = NRFBOX_UI_DESIGN_W - 8; // dejamos el área del scrollbar
  const int16_t selH = 21;

  // Fila anterior (normal)
  u8g2.setTextColor(TFT_WHITE, TFT_BLACK);
  u8g2.setFont(u8g_font_7x14);
  u8g2.drawStr(baseX + 25, baseY + 15, menu_items[item_sel_previous]);
  u8g2.drawXBMP(baseX + 4, baseY + 2, 16, 16, bitmap_icons[item_sel_previous]);

  // Fila seleccionada: fondo blanco + texto/icono negros
  u8g2.setTextColor(TFT_WHITE, TFT_BLACK);
  u8g2.drawBox(selX, selY, selW, selH);

  u8g2.setTextColor(TFT_BLACK, TFT_WHITE);
  u8g2.setFont(u8g_font_7x14B);
  u8g2.drawStr(baseX + 25, baseY + 15 + 20 + 2, menu_items[item_selected]);
  u8g2.drawXBMP(baseX + 4, baseY + 24, 16, 16, bitmap_icons[item_selected]);

  // Fila siguiente (normal)
  u8g2.setTextColor(TFT_WHITE, TFT_BLACK);
  u8g2.setFont(u8g_font_7x14);
  u8g2.drawStr(baseX + 25, baseY + 15 + 20 + 20 + 2 + 2, menu_items[item_sel_next]);
  u8g2.drawXBMP(baseX + 4, baseY + 46, 16, 16, bitmap_icons[item_sel_next]);

  // Scrollbar simple (sin bitmap)
  const int16_t sbX = baseX + (NRFBOX_UI_DESIGN_W - 3);
  u8g2.drawVLine(sbX + 1, baseY + 0, NRFBOX_UI_DESIGN_H);

  int16_t knobH = (NRFBOX_UI_DESIGN_H / NUM_ITEMS);
  if (knobH < 4) knobH = 4;
  int16_t knobY = baseY + knobH * item_selected;
  u8g2.drawBox(sbX, knobY, 3, knobH);

  u8g2.sendBuffer();
}

void setup() {

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.print("Reset reason: ");
  Serial.println(resetReasonToStr(esp_reset_reason()));

  neopixelSetup();

  //configureNrf(RadioA);
  //configureNrf(RadioB);
  //configureNrf(RadioC);

  EEPROM.begin(512);
  oledBrightness = EEPROM.read(1);

  u8g2.begin();
  u8g2.setContrast(oledBrightness);
  u8g2.setBitmapMode(1);

  // Diagnóstico visible para detectar si es brownout/WDT/panic/etc.
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 12, "Reset:");
  u8g2.setCursor(0, 28);
  u8g2.print(resetReasonToStr(esp_reset_reason()));
  u8g2.sendBuffer();
  delay(800);

#if NRFBOX_DIAG_STAGE_TRACE
  // Marca "A": justo después del delay de Reset
  u8g2.setCursor(0, 60);
  u8g2.print("A");
  delay(150);
#endif

#if NRFBOX_DIAG_HOLD_AFTER_RESET
  while (true) {
    delay(1000);
  }
#endif

#if !NRFBOX_SKIP_SPLASH
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB14_tr);
  int16_t nameWidth = u8g2.getUTF8Width("nRF-BOX");
  int16_t nameX = (NRFBOX_UI_W - nameWidth) / 2;
  u8g2.setCursor(nameX, 25);
  u8g2.print("nRF-BOX");

  u8g2.setFont(u8g2_font_ncenB08_tr);
  int16_t creditWidth = u8g2.getUTF8Width("by CiferTech");
  int16_t creditX = (NRFBOX_UI_W - creditWidth) / 2;
  u8g2.setCursor(creditX, 40);
  u8g2.print("by CiferTech");

  u8g2.setFont(u8g2_font_6x10_tf);
  int16_t versionWidth = u8g2.getUTF8Width("v2.5.0");
  int16_t versionX = (NRFBOX_UI_W - versionWidth) / 2;
  u8g2.setCursor(versionX, 60);
  u8g2.print("v2.5.0");

  u8g2.sendBuffer();
  delay(3000);

  u8g2.clearBuffer();

  u8g2.drawXBMP(0, 0, NRFBOX_UI_DESIGN_W, NRFBOX_UI_DESIGN_H, logo_cifer);

  u8g2.sendBuffer();
  delay(250);
#endif

#if !NRFBOX_SKIP_BUTTONS
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
#endif

#if NRFBOX_DIAG_STAGE_TRACE
  // Marca "B": después de (intentar) configurar botones
  u8g2.setCursor(10, 60);
  u8g2.print("B");
  delay(150);
#endif

  activeScreen = SCREEN_MENU;
  screenNeedsSetup = false;

#if NRFBOX_DIAG_STAGE_TRACE
  // Marca "C": justo antes de dibujar menú
  u8g2.setCursor(20, 60);
  u8g2.print("C");
  delay(150);
#endif

  drawMenu();

#if NRFBOX_DIAG_STAGE_TRACE
  // Marca "D": si ves esta letra, drawMenu() terminó sin colgarse
  u8g2.setCursor(30, 60);
  u8g2.print("D");
  delay(150);
#endif

#if NRFBOX_DIAG_HOLD_AFTER_MENU_DRAW
  while (true) {
    delay(1000);
  }
#endif
}

void loop() {

#if NRFBOX_SKIP_BUTTONS
  // Si deshabilitamos botones, no leemos esos GPIO (evita tocar pines conflictivos).
  // Solo mantenemos vivo el sistema.
  delay(50);
  return;
#endif
  // Ignorar botones al arranque: evita entrar solo a pantallas por ruido o
  // porque el botón está presionado durante el boot.
  static uint32_t bootIgnoreUntilMs = 0;
  if (bootIgnoreUntilMs == 0) bootIgnoreUntilMs = millis() + 1500;
  if (millis() < bootIgnoreUntilMs) {
    button_up_clicked = (digitalRead(BUTTON_UP_PIN) == LOW) ? 1 : 0;
    button_down_clicked = (digitalRead(BUTTON_DOWN_PIN) == LOW) ? 1 : 0;
    button_select_clicked = (digitalRead(BUTTON_SELECT_PIN) == LOW) ? 1 : 0;
    delay(10);
    return;
  }

  // ------------------------------------------------------------
  // Lectura de botones con “edge detect” (click)
  // ------------------------------------------------------------
  bool upPressedEvent = false;
  bool downPressedEvent = false;

  if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0)) {
    button_up_clicked = 1;
    upPressedEvent = true;
  } else if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1)) {
    button_up_clicked = 0;
  }

  if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0)) {
    button_down_clicked = 1;
    downPressedEvent = true;
  } else if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1)) {
    button_down_clicked = 0;
  }

  // SELECT: click + long-press para "volver" (para no chocar con Setting u otros)
  static uint32_t selectPressStartMs = 0;
  static bool selectLongSent = false;

  bool selectPressedEvent = false;
  bool selectLongPressEvent = false;

  bool selectDown = (digitalRead(BUTTON_SELECT_PIN) == LOW);

  if (selectDown && (button_select_clicked == 0)) {
    button_select_clicked = 1;
    selectPressedEvent = true;
    selectPressStartMs = millis();
    selectLongSent = false;
  } else if (!selectDown && (button_select_clicked == 1)) {
    button_select_clicked = 0;
    selectPressStartMs = 0;
    selectLongSent = false;
  }

  // Long press solo cuando NO estás en el menú (back global)
  if (activeScreen != SCREEN_MENU && selectDown && (button_select_clicked == 1) && !selectLongSent) {
    if (millis() - selectPressStartMs > 900) {
      selectLongPressEvent = true;
      selectLongSent = true;
    }
  }

  // ------------------------------------------------------------
  // Menú
  // ------------------------------------------------------------
  if (activeScreen == SCREEN_MENU) {
    if (upPressedEvent) {
      item_selected = item_selected - 1;
      if (item_selected < 0) {
        item_selected = NUM_ITEMS - 1;
      }
      drawMenu();
    } else if (downPressedEvent) {
      item_selected = item_selected + 1;
      if (item_selected >= NUM_ITEMS) {
        item_selected = 0;
      }
      drawMenu();
    }

    if (selectPressedEvent) {
      activeScreen = screenFromMenuItem(item_selected);
      screenNeedsSetup = true;
    }

    // Mantener el watchdog satisfecho
    delay(1);
    return;
  }

  // ------------------------------------------------------------
  // Screens / Módulos
  // ------------------------------------------------------------
  if (screenNeedsSetup) {
    switch (activeScreen) {
      case SCREEN_SCANNER: scannerSetup(); break;
      case SCREEN_ANALYZER: analyzerSetup(); break;
      case SCREEN_JAMMER: jammerSetup(); break;
      case SCREEN_BLACKOUT: blackoutSetup(); break;
      case SCREEN_BLE_JAMMER: blejammerSetup(); break;
      case SCREEN_SPOOFER: spooferSetup(); break;
      case SCREEN_SOURAPPLE: sourappleSetup(); break;
      case SCREEN_BLESCAN: blescanSetup(); break;
      case SCREEN_WIFISCAN: wifiscanSetup(); break;
      case SCREEN_SETTING: settingSetup(); break;
      case SCREEN_ABOUT: about(); break;
      default: break;
    }
    screenNeedsSetup = false;
  }

  switch (activeScreen) {
    case SCREEN_SCANNER: scannerLoop(); break;
    case SCREEN_ANALYZER: analyzerLoop(); break;
    case SCREEN_JAMMER: jammerLoop(); break;
    case SCREEN_BLACKOUT: blackoutLoop(); break;
    case SCREEN_BLE_JAMMER: blejammerLoop(); break;
    case SCREEN_SPOOFER: spooferLoop(); break;
    case SCREEN_SOURAPPLE: sourappleLoop(); break;
    case SCREEN_BLESCAN: blescanLoop(); break;
    case SCREEN_WIFISCAN: wifiscanLoop(); break;
    case SCREEN_SETTING: settingLoop(); break;
    case SCREEN_ABOUT: /* ya dibujado en setup */ break;
    default: break;
  }

  // Salida:
  // - About: click corto vuelve
  // - Otros: long-press de SELECT vuelve (no interfiere con Setting)
  if (activeScreen == SCREEN_ABOUT) {
    if (selectPressedEvent) {
      activeScreen = SCREEN_MENU;
      screenNeedsSetup = false;
      drawMenu();
    }
  } else {
    if (selectLongPressEvent) {
      activeScreen = SCREEN_MENU;
      screenNeedsSetup = false;
      drawMenu();
    }
  }

  // Evitar WDT/reset por loop “demasiado apretado”
  delay(1);
}
