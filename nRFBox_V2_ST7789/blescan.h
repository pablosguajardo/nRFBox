/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */
   
#ifndef blescan_H
#define blescan_H

#include <BLEDevice.h>
#include "display_compat.h"
#include "neopixel.h"

void blescanSetup();
void blescanLoop();

#endif
