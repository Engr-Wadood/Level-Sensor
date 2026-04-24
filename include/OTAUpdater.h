#pragma once

#include <Arduino.h>

namespace OTAUpdater {

struct Config {
  const char *hostname = "waterlevel-esp32";
  const char *password = "";   // optional; leave empty for none
  uint16_t port = 3232;        // ArduinoOTA default port
  bool rebootOnSuccess = true; // default ArduinoOTA behavior
};

// Call after Wi-Fi is connected (STA or AP), before serving.
void begin(const Config &cfg = Config{});

// Call frequently in loop().
void loop();

} // namespace OTAUpdater

#pragma once
#include <Arduino.h>

namespace OTAUpdater {
  void begin(const char* hostname = "esp32-ota", const char* password = nullptr, uint16_t port = 3232);
  void handle();
}