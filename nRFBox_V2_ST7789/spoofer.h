/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */
   
#ifndef spoofer_H
#define spoofer_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "display_compat.h"
#include <Adafruit_NeoPixel.h>
#include "neopixel.h"

void spooferSetup();
void spooferLoop();

#endif
