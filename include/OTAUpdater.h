#pragma once

#include <Arduino.h>

namespace OTAUPDATER {

struct Config {
  const char *hostname = "waterlevel-esp32";
  const char *password = ""; // optional; empty = no auth
  uint16_t port = 3232;      // default ArduinoOTA port
};

// Call after Wi‑Fi is connected.
void begin(const Config &cfg = Config{});

// Call frequently in loop().
void loop();

} // namespace OTAUPDATER

