/*
 * LEDStripCtrl.h
 *
 *  Created on: 19.05.2017
 *      Author: sezep_000
 */

#ifndef LEDSTRIPCTRL_H_
#define LEDSTRIPCTRL_H_

#include <Arduino.h>

#define LED_STRIP_LENGTH 32	//Number of WS2812 Led's in the strip

void initStrip();
void setStripColor(uint32_t color);
uint32_t hsv(float h, const float s, const float v);
uint32_t rgb(const float r, const float g, const float b);

#endif /* LEDSTRIPCTRL_H_ */
