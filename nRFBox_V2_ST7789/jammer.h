/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */
   
#ifndef jammer_H
#define jammer_H

#include <SPI.h>
#include "display_compat.h"
#include <RF24.h>
#include "esp_bt.h"
#include "esp_wifi.h"
#include "neopixel.h"

void jammerSetup();
void jammerLoop();

#endif
