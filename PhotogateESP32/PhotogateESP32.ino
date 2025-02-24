// #define USE_INTRANET

// Debug only
#define LOCAL_SSID "XXX"
#define LOCAL_PASS "XXX"

#define AP_SSID "PhotoGate01"
#define AP_PASS "ccaufscar01"

// #include <WiFi.h>
// #include <WebServer.h>
#include "Photogate.h"

// Photogate pinout.
//must change s_gateSize due to static allocation on module changes!!!!
//const unsigned int gate[6] = {34, 35, 32, 33, 25, 26}; // ESP-WROOM-32 GPIO PINOUT. If gate size changed, must change s_gateSize due to static allocation.
const unsigned int gate[4] = {0, 1, 2, 3}; // ESP32-C3 SuperMini. *CHIP HAS A FAILURE THAT A5 (GPIO5) CAN'T BE USED WITH OTHERS ANALOG PORTS!
Photogate Gate(gate);

// Add this to Web class later.
// char XML[2048];
// char buf[32];
// 
// IPAddress ActualIP;
// IPAddress PageIP(192, 168, 1, 1);
// IPAddress Gateway(192, 168, 1, 1);
// IPAddress Subnet(255, 255, 255, 0);
// IPAddress Ip;
// 
// WebServer Server(80);

void setup() 
{
  Serial.begin(115200);
  Gate.PinSetGate();
}
  
void loop() 
{
  // If start button pressed, calls Gate.InitPhotogate();
  // then,
  // Photogate time loop:
  if(Gate.IsRunning())
  {
    Gate.OnUpdate();
  }
}


