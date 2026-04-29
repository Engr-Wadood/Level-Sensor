#include <Arduino.h>  // ← ADD THIS LINE
#include <WiFi.h>

#include "OTAUPDATER.h"
#include "webpage.h"

#define TRIG_PIN  5
#define ECHO_PIN  18

// --- Wi‑Fi STA credentials ---
// Fill these in to connect your ESP32 to your router.
static const char *WIFI_SSID = "Khan Home";
static const char *WIFI_PASS = "Khanboys@232";

// --- OTA password (must match PlatformIO upload_flags --auth="...") ---
static const char *OTA_PASSWORD = "12345678";

// Sensor valid measuring range (AJ‑SR04M): 20cm – 450cm
static constexpr float kMinDistanceCm = 20.0f;
static constexpr float kMaxDistanceCm = 450.0f;

static void connectWifiSta() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.printf("Connecting to Wi‑Fi SSID: %s\n", WIFI_SSID);
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi‑Fi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi‑Fi not connected (check SSID/password).");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("AJ-SR04M Ready");

  connectWifiSta();

  if (WiFi.status() == WL_CONNECTED) {
    webpage::Config webCfg;
    webCfg.httpPort = 80;
    webCfg.wsPort = 81;
    webCfg.deviceName = "Level Sensor";
    webpage::begin(webCfg);

    OTAUPDATER::Config otaCfg;
    otaCfg.hostname = "level-sensor-esp32";
    otaCfg.password = OTA_PASSWORD;
    otaCfg.port = 3232;
    OTAUPDATER::begin(otaCfg);
  }
}

float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  return (duration * 0.0343) / 2.0;
}

void loop() {
  // IMPORTANT: keep loop non-blocking so OTA doesn't time out during upload.
  if (WiFi.status() == WL_CONNECTED) {
    webpage::loop();
    OTAUPDATER::loop();
  }

  static uint32_t lastSampleMs = 0;
  const uint32_t now = millis();
  if (now - lastSampleMs < 200) return;
  lastSampleMs = now;

  const float raw = getDistanceCm();

  webpage::Reading r;
  r.distanceCm = raw;
  r.belowMinimum = false;

  // Clamp behavior:
  // - If raw < 20cm, the sensor can output rubbish. We force a constant 20cm and show a warning note.
  // - If raw > 450cm, cap at 450cm (outside range).
  if (raw >= 0) {
    if (raw < kMinDistanceCm) {
      r.distanceCm = kMinDistanceCm;
      r.belowMinimum = true;
    } else if (raw > kMaxDistanceCm) {
      r.distanceCm = kMaxDistanceCm;
    }
  }

  if (raw < 0) {
    Serial.println("Out of range or no echo");
  } else {
    Serial.print("Distance: ");
    Serial.print(r.distanceCm, 1);
    Serial.println(" cm");
    if (r.belowMinimum) {
      Serial.println("NOTE: Distance is less than minimum required (20cm). Please fix the probe distance.");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    webpage::updateReading(r);
  }
}