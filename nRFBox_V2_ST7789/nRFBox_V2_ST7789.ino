/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */

#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#include <SPI.h>
#include "display_compat.h"

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
#define TFT_BL           12    // LED back-light control pin
#define TFT_MISO -1      // -1 es no usado.
#define TFT_MOSI 35      // Mosi SDA
#define TFT_SCLK 19      // Reloj
#define TFT_CS   15       // Chip Select
#define TFT_DC    13      // Data Comand.
#define TFT_RST   14      // Reset.

*/
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

#define BUTTON_UP_PIN 26
// Unificar con el resto del proyecto (setting/jammer/etc.): el SELECT es GPIO27.
// Si quedaba en 32 (pin flotante o no conectado) el firmware entraba en los while()
// esperando “soltar el botón” y terminaba reseteando por WDT.
#define BUTTON_SELECT_PIN 27
#define BUTTON_DOWN_PIN 33

//RF24 RadioA(CE_PIN_A, CSN_PIN_A);
//RF24 RadioB(CE_PIN_B, CSN_PIN_B);
//RF24 RadioC(CE_PIN_C, CSN_PIN_C);

//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0); // [full framebuffer, size = 1024 bytes]
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_NeoPixel pixels(1, 14, NEO_GRB + NEO_KHZ800);

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

  u8g2.drawXBMP(0, 22, 128, 21, bitmap_item_sel_outline);

  u8g2.setFont(u8g_font_7x14);
  u8g2.drawStr(25, 15, menu_items[item_sel_previous]);
  u8g2.drawXBMP(4, 2, 16, 16, bitmap_icons[item_sel_previous]);

  u8g2.setFont(u8g_font_7x14B);
  u8g2.drawStr(25, 15 + 20 + 2, menu_items[item_selected]);
  u8g2.drawXBMP(4, 24, 16, 16, bitmap_icons[item_selected]);

  u8g2.setFont(u8g_font_7x14);
  u8g2.drawStr(25, 15 + 20 + 20 + 2 + 2, menu_items[item_sel_next]);
  u8g2.drawXBMP(4, 46, 16, 16, bitmap_icons[item_sel_next]);

  u8g2.drawXBMP(128 - 8, 0, 8, 64, bitmap_scrollbar_background);
  u8g2.drawBox(125, 64 / NUM_ITEMS * item_selected, 3, 64 / NUM_ITEMS);

  u8g2.sendBuffer();
}

void setup() {

  neopixelSetup();

  //configureNrf(RadioA);
  //configureNrf(RadioB);
  //configureNrf(RadioC);

  EEPROM.begin(512);
  oledBrightness = EEPROM.read(1);

  u8g2.begin();
  u8g2.setContrast(oledBrightness);
  u8g2.setBitmapMode(1);

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB14_tr);
  int16_t nameWidth = u8g2.getUTF8Width("nRF-BOX");
  int16_t nameX = (128 - nameWidth) / 2;
  u8g2.setCursor(nameX, 25);
  u8g2.print("nRF-BOX");

  u8g2.setFont(u8g2_font_ncenB08_tr);
  int16_t creditWidth = u8g2.getUTF8Width("by CiferTech");
  int16_t creditX = (106 - creditWidth) / 2;
  u8g2.setCursor(creditX, 40);
  u8g2.print("by CiferTech");

  u8g2.setFont(u8g2_font_6x10_tf);
  int16_t versionWidth = u8g2.getUTF8Width("v2.5.0");
  int16_t versionX = (128 - versionWidth) / 2;
  u8g2.setCursor(versionX, 60);
  u8g2.print("v2.5.0");

  u8g2.sendBuffer();
  delay(3000);

  u8g2.clearBuffer();

  u8g2.drawXBMP(0, 0, 128, 64, logo_cifer);

  u8g2.sendBuffer();
  delay(250);

  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);

  activeScreen = SCREEN_MENU;
  screenNeedsSetup = false;
  drawMenu();
}

void loop() {
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
