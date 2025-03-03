#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

//OPTIONAL: IF NDEF OLED SETUP
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C // This might change to 0x3D depending on the OLED 0.96'' module.

#endif