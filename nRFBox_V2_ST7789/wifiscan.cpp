/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */

#include <Arduino.h> 
#include "wifiscan.h"
#include "board_pins.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

#define BTN_PIN_UP NRFBOX_BTN_UP_PIN
#define BTN_PIN_DOWN NRFBOX_BTN_DOWN_PIN
#define BTN_PIN_SELECT NRFBOX_BTN_SELECT_PIN
#define BTN_PIN_BACK NRFBOX_BTN_BACK_PIN

int currentIndex = 0;
int listStartIndex = 0;
bool isDetailView = false;
unsigned long scan_StartTime = 0;
const unsigned long scanTimeout = 5000;
bool isScanComplete = false;

unsigned long lastButtonPress = 0; // (legacy, ya no se usa; se puede eliminar)
unsigned long debounceTime = 200;  // (legacy, ya no se usa; se puede eliminar)

void wifiscanSetup() {
  Serial.begin(115200);
  u8g2.setFont(u8g2_font_6x10_tr);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  pinMode(BTN_PIN_UP, INPUT_PULLUP);
  pinMode(BTN_PIN_DOWN, INPUT_PULLUP);
  pinMode(BTN_PIN_SELECT, INPUT_PULLUP);
  pinMode(BTN_PIN_BACK, INPUT_PULLUP);
  
  for (int cycle = 0; cycle < 3; cycle++) { 
    for (int i = 0; i < 3; i++) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, "Scanning WiFi");

      String dots = "";
      for (int j = 0; j <= i; j++) {
        dots += " .";
        setNeoPixelColour("white");
      }
      setNeoPixelColour("0");
      
      u8g2.drawStr(80, 10, dots.c_str()); 

      u8g2.sendBuffer();
      delay(300); 
    }
  }
  
  scan_StartTime = millis();
  isScanComplete = false;
}

void wifiscanLoop() {
  unsigned long currentMillis = millis();
  bool needRedraw = false;

  // --- Scan (una vez) ---
  if (!isScanComplete && currentMillis - scan_StartTime < scanTimeout) {
    int foundNetworks = WiFi.scanNetworks(); // (en ESP32 suele ser bloqueante)
    if (foundNetworks >= 0) {
      isScanComplete = true;
      needRedraw = true;
    }
  }

  // --- Botones: click DEBOUNCEADO (estable) ---
  // Motivo: con INPUT_PULLUP, al SOLTAR el botón puede haber rebote y generar
  // un “click fantasma” (hace que el detalle aparezca 1s y vuelva al listado).
  //
  // Implementación:
  // - leemos el estado raw (LOW = presionado)
  // - esperamos que se mantenga estable X ms
  // - generamos "click" sólo en transición estable a PRESIONADO
  static bool upStableDown = false, upLastRaw = false;
  static bool downStableDown = false, downLastRaw = false;
  static bool selStableDown = false, selLastRaw = false;
  static bool backStableDown = false, backLastRaw = false;

  static uint32_t upLastChangeMs = 0;
  static uint32_t downLastChangeMs = 0;
  static uint32_t selLastChangeMs = 0;
  static uint32_t backLastChangeMs = 0;

  const uint32_t DEBOUNCE_MS = 35;

  const bool upRawDown = (digitalRead(BTN_PIN_UP) == LOW);
  const bool downRawDown = (digitalRead(BTN_PIN_DOWN) == LOW);
  const bool selRawDown = (digitalRead(BTN_PIN_SELECT) == LOW);
  const bool backRawDown = (digitalRead(BTN_PIN_BACK) == LOW);

  if (upRawDown != upLastRaw) { upLastRaw = upRawDown; upLastChangeMs = currentMillis; }
  if (downRawDown != downLastRaw) { downLastRaw = downRawDown; downLastChangeMs = currentMillis; }
  if (selRawDown != selLastRaw) { selLastRaw = selRawDown; selLastChangeMs = currentMillis; }
  if (backRawDown != backLastRaw) { backLastRaw = backRawDown; backLastChangeMs = currentMillis; }

  bool upClick = false, downClick = false, selClick = false, backClick = false;

  if ((currentMillis - upLastChangeMs) >= DEBOUNCE_MS && upStableDown != upLastRaw) {
    upStableDown = upLastRaw;
    if (upStableDown) upClick = true;
  }
  if ((currentMillis - downLastChangeMs) >= DEBOUNCE_MS && downStableDown != downLastRaw) {
    downStableDown = downLastRaw;
    if (downStableDown) downClick = true;
  }
  if ((currentMillis - selLastChangeMs) >= DEBOUNCE_MS && selStableDown != selLastRaw) {
    selStableDown = selLastRaw;
    if (selStableDown) selClick = true;
  }
  if ((currentMillis - backLastChangeMs) >= DEBOUNCE_MS && backStableDown != backLastRaw) {
    backStableDown = backLastRaw;
    if (backStableDown) backClick = true;
  }

  // Acciones (mismo comportamiento: 1 click entra a detalle, 1 click sale)
  if (upClick && !isDetailView) {
    if (currentIndex > 0) {
      currentIndex--;
      if (currentIndex < listStartIndex) listStartIndex--;
      needRedraw = true;
    }
  } else if (downClick && !isDetailView) {
    if (currentIndex < WiFi.scanComplete() - 1) {
      currentIndex++;
      if (currentIndex >= listStartIndex + 5) listStartIndex++;
      needRedraw = true;
    }
  } else if (selClick) {
    if (!isDetailView) {
      if (isScanComplete && WiFi.scanComplete() > 0) {
        isDetailView = true;
        needRedraw = true;
      }
    } else {
      isDetailView = false;
      needRedraw = true;
    }
  } else if (backClick) {
    if (isDetailView) {
      isDetailView = false;
      needRedraw = true;
    }
  }

  // --- Redibujar sólo si cambió estado/selección (evita flicker en ST7789) ---
  static bool lastDetailView = false;
  static bool lastScanComplete = false;
  static int lastIndex = -1;
  static int lastStart = -1;

  if (isDetailView != lastDetailView ||
      isScanComplete != lastScanComplete ||
      currentIndex != lastIndex ||
      listStartIndex != lastStart) {
    needRedraw = true;
  }

  if (needRedraw) {
    if (!isDetailView && isScanComplete) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(0, 10, "Wi-Fi Networks:");

      int networkCount = WiFi.scanComplete();
      for (int i = 0; i < 5; i++) {
        int currentNetworkIndex = i + listStartIndex;
        if (currentNetworkIndex >= networkCount) break;

        String networkName = WiFi.SSID(currentNetworkIndex);
        int rssi = WiFi.RSSI(currentNetworkIndex);

        String networkInfo = networkName.substring(0, 7);
        String networkrssi = " | RSSI " + String(rssi);

        if (currentNetworkIndex == currentIndex) {
          u8g2.drawStr(0, 20 + i * 10, ">");
        }
        u8g2.drawStr(10, 20 + i * 10, networkInfo.c_str());
        u8g2.drawStr(50, 20 + i * 10, networkrssi.c_str());
      }
      u8g2.sendBuffer();
    }

    if (isDetailView) {
      String networkName = WiFi.SSID(currentIndex);
      String networkBSSID = WiFi.BSSIDstr(currentIndex);
      int rssi = WiFi.RSSI(currentIndex);
      int channel = WiFi.channel(currentIndex);

      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(0, 10, "Network Details:");

      u8g2.setFont(u8g2_font_5x8_tr);
      String name = "SSID: " + networkName;
      String bssid = "BSSID: " + networkBSSID;
      String signal = "RSSI: " + String(rssi);
      String ch = "Channel: " + String(channel);

      u8g2.drawStr(0, 20, name.c_str());
      u8g2.drawStr(0, 30, bssid.c_str());
      u8g2.drawStr(0, 40, signal.c_str());
      u8g2.drawStr(0, 50, ch.c_str());
      u8g2.drawStr(0, 60, "Press SELECT to go back");
      u8g2.sendBuffer();
    }

    lastDetailView = isDetailView;
    lastScanComplete = isScanComplete;
    lastIndex = currentIndex;
    lastStart = listStartIndex;
  }

  // Evita loop ultra-rápido (reduce flicker y carga SPI)
  delay(10);
}
