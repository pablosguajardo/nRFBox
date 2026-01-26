/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */
   
#ifndef blackout_H
#define blackout_H

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "display_compat.h"
#include "neopixel.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "neopixel.h"

void blackoutSetup();
void blackoutLoop();

#endif
