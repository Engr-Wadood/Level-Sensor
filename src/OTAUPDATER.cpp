#include "OTAUPDATER.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

namespace OTAUPDATER {

static Config g_cfg{};

void begin(const Config &cfg) {
  g_cfg = cfg;

  ArduinoOTA.setHostname(g_cfg.hostname);
  ArduinoOTA.setPort(g_cfg.port);

  if (g_cfg.password && g_cfg.password[0] != '\0') {
    ArduinoOTA.setPassword(g_cfg.password);
  }

  ArduinoOTA
      .onStart([]() {
        Serial.println("[OTA] Start");
      })
      .onEnd([]() {
        Serial.println("[OTA] End");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        static uint32_t lastMs = 0;
        const uint32_t now = millis();
        if (now - lastMs < 500) return;
        lastMs = now;
        const uint32_t pct = (total == 0) ? 0 : (progress * 100U) / total;
        Serial.printf("[OTA] Progress: %lu%%\n", static_cast<unsigned long>(pct));
      })
      .onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]\n", error);
        if (error == OTA_AUTH_ERROR) Serial.println("[OTA] Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("[OTA] Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("[OTA] Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("[OTA] Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("[OTA] End Failed");
      });

  ArduinoOTA.begin();

  Serial.printf("[OTA] Ready. IP: %s  Port: %u  Hostname: %s\n",
                WiFi.localIP().toString().c_str(), g_cfg.port, g_cfg.hostname);
}

void loop() { ArduinoOTA.handle(); }

} // namespace OTAUPDATER

