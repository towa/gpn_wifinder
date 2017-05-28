/*
 * LEDStripCtrl.cpp
 *
 *  Created on: 19.05.2017
 *      Author: sezep_000
 */

#include "LEDStripCtrl.h"

#include <NeoPixelBus.h>

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> ledOutput(LED_STRIP_LENGTH);

/**
 * Initialize the WS2812 strip
 */
void initStrip() {
	ledOutput.Begin();
}

/**
 * Set the color of the entire strip
 */
void setStripColor(uint32_t color) {
	uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >>  8) & 0xFF;
    uint8_t b = (color >>  0) & 0xFF;

	for (int i = 0; i < LED_STRIP_LENGTH; i++)
		ledOutput.SetPixelColor(i, RgbColor(r, g, b));
	ledOutput.Show();
}

/**
 * Build a color from the given hsv values
 * @param h hue
 * @param s saturation
 * @param v value
 * @return the hsv color
 */
uint32_t hsv(float h, const float s, const float v) {
  h -= ((int) (h / 360)) * 360.0f;

  int   i =   floor(h / 60.0f);
  float f  = (h / 60.0f) - i;
  float p = v * (1 - s);
  float q = v * (1 - s * f);
  float t = v * (1 - s * (1 - f));

  switch (i) {
    case 0  : return rgb(v, t, p); break;
    case 1  : return rgb(q, v, p); break;
    case 2  : return rgb(p, v, t); break;
    case 3  : return rgb(p, q, v); break;
    case 4  : return rgb(t, p, v); break;
    case 5  : return rgb(v, p, q); break;
    default : return rgb(v, t, p); break;
  }

}

/**
 * Build a color from the given rgb values
 * @param r red
 * @param g green
 * @param b blue
 * @return the rgb color
 */
uint32_t rgb(const float r, const float g, const float b) {
  return 0x00 << 24 | ((int) (r * 255)) << 16
                    | ((int) (g * 255)) << 8
                    | ((int) (b * 255));
}
