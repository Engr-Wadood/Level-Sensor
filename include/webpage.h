#pragma once

#include <Arduino.h>

namespace webpage {

struct Reading {
  float distanceCm = -1; // -1 means no echo / invalid
  bool belowMinimum = false;
};

struct Config {
  uint16_t httpPort = 80;
  uint16_t wsPort = 81;
  const char *deviceName = "Level Sensor";
};

// Start the HTTP server + WebSocket server. Call after Wi-Fi is connected.
void begin(const Config &cfg = Config{});

// Update the reading shown on the webpage (call whenever you have a new value).
void updateReading(const Reading &r);

// Call frequently in loop() to service clients.
void loop();

} // namespace webpage

