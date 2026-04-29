#include "webpage.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

namespace webpage {

static Config g_cfg{};
static WebServer g_http(80);
static WebSocketsServer g_ws(81);

static Reading g_reading{};
static uint32_t g_lastPushMs = 0;

static const char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Level Sensor</title>
  <style>
    :root{
      --bg:#0b1220; --card:#121b2e; --muted:#93a4c7; --text:#e9f0ff;
      --good:#22c55e; --warn:#f59e0b; --bad:#ef4444; --line:rgba(255,255,255,.08);
    }
    *{box-sizing:border-box}
    body{
      margin:0; font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial;
      background: radial-gradient(1200px 600px at 20% 10%, rgba(59,130,246,.25), transparent 50%),
                  radial-gradient(900px 500px at 90% 20%, rgba(34,197,94,.18), transparent 55%),
                  var(--bg);
      color:var(--text);
    }
    .wrap{max-width:980px; margin:0 auto; padding:24px}
    header{display:flex; align-items:center; justify-content:space-between; gap:12px; margin-bottom:18px}
    .title{display:flex; flex-direction:column; gap:6px}
    h1{font-size:20px; margin:0}
    .sub{color:var(--muted); font-size:13px}
    .grid{display:grid; grid-template-columns: 1.3fr .7fr; gap:14px}
    @media (max-width:860px){ .grid{grid-template-columns:1fr} }
    .card{
      background: linear-gradient(180deg, rgba(255,255,255,.06), rgba(255,255,255,.03));
      border:1px solid var(--line);
      border-radius:16px;
      padding:16px;
      box-shadow: 0 20px 55px rgba(0,0,0,.35);
      backdrop-filter: blur(6px);
    }
    .value{font-size:56px; font-weight:800; letter-spacing:-1px; margin:6px 0 0}
    .unit{font-size:16px; color:var(--muted); font-weight:600}
    .row{display:flex; align-items:center; justify-content:space-between; gap:10px}
    .badge{
      display:inline-flex; align-items:center; gap:8px;
      padding:8px 10px; border-radius:999px; font-size:12px; font-weight:700;
      border:1px solid var(--line); background:rgba(0,0,0,.18);
    }
    .dot{width:10px; height:10px; border-radius:999px; background:var(--warn)}
    .dot.good{background:var(--good)} .dot.bad{background:var(--bad)}
    .muted{color:var(--muted)}
    .note{margin-top:10px; padding:12px; border-radius:12px; border:1px dashed rgba(245,158,11,.55); background:rgba(245,158,11,.08)}
    .note.bad{border-color:rgba(239,68,68,.55); background:rgba(239,68,68,.08)}
    .note p{margin:0; font-size:13px; line-height:1.45}
    .kvs{display:grid; grid-template-columns:1fr; gap:10px; margin-top:8px}
    .kv{display:flex; justify-content:space-between; gap:10px; padding:10px 12px; border-radius:12px; border:1px solid var(--line); background:rgba(0,0,0,.14)}
    .kv b{font-size:13px} .kv span{font-size:13px; color:var(--muted)}
    footer{margin-top:14px; color:var(--muted); font-size:12px}
    a{color:#93c5fd; text-decoration:none} a:hover{text-decoration:underline}
  </style>
</head>
<body>
  <div class="wrap">
    <header>
      <div class="title">
        <h1 id="device">Level Sensor</h1>
        <div class="sub">Live distance reading (AJ‑SR04M, valid range: 20–450cm)</div>
      </div>
      <div class="badge" id="status"><span class="dot" id="dot"></span><span id="st">Connecting…</span></div>
    </header>

    <div class="grid">
      <section class="card">
        <div class="row">
          <div class="muted">Distance to surface</div>
          <div class="muted" id="updated">—</div>
        </div>
        <div class="value"><span id="dist">—</span> <span class="unit">cm</span></div>

        <div class="note">
          <p><b>Installation note:</b> The sensor measures reliably from <b>20cm to 450cm</b>. Please place the sensor <b>at least 20cm above the maximum water level</b> of the tank.</p>
        </div>

        <div class="note bad" id="minNote" style="display:none">
          <p><b>Distance is less than minimum required.</b> Please fix the probe distance (keep it ≥ 20cm).</p>
        </div>
      </section>

      <aside class="card">
        <div class="muted" style="margin-bottom:8px">Device</div>
        <div class="kvs">
          <div class="kv"><b>Wi‑Fi</b><span id="wifi">—</span></div>
          <div class="kv"><b>IP</b><span id="ip">—</span></div>
          <div class="kv"><b>Range</b><span>20–450cm</span></div>
          <div class="kv"><b>API</b><span><a href="/api/status">/api/status</a></span></div>
        </div>
        <footer>
          Tip: keep this tab open on your phone for live updates.
        </footer>
      </aside>
    </div>
  </div>

  <script>
    const $ = (id) => document.getElementById(id);
    let ws;
    let lastOk = 0;

    function setConn(ok, msg){
      $("st").textContent = msg;
      $("dot").className = "dot " + (ok ? "good" : "bad");
      $("status").style.borderColor = ok ? "rgba(34,197,94,.45)" : "rgba(239,68,68,.55)";
      lastOk = ok ? Date.now() : lastOk;
    }

    function fmtTs(ms){
      const d = new Date(ms);
      return d.toLocaleTimeString([], {hour:"2-digit", minute:"2-digit", second:"2-digit"});
    }

    function applyStatus(s){
      if (!s) return;
      $("device").textContent = s.deviceName || "Level Sensor";
      $("wifi").textContent = s.wifiSsid || "—";
      $("ip").textContent = s.ip || "—";

      const d = (typeof s.distanceCm === "number") ? s.distanceCm : null;
      if (d === null || d < 0){
        $("dist").textContent = "—";
      } else {
        $("dist").textContent = d.toFixed(1);
      }

      $("minNote").style.display = s.belowMinimum ? "block" : "none";
      if (s.belowMinimum) {
        // distance is clamped to 20cm in firmware, so show the warning note.
      }

      if (s.updatedAtMs) $("updated").textContent = "Updated: " + fmtTs(s.updatedAtMs);
    }

    async function fetchFallback(){
      try{
        const r = await fetch("/api/status", {cache:"no-store"});
        const s = await r.json();
        applyStatus(s);
        setConn(true, "Connected (polling)");
      }catch(e){
        setConn(false, "Disconnected");
      }
    }

    function connectWs(){
      const url = `ws://${location.hostname}:81/`;
      ws = new WebSocket(url);
      ws.onopen = () => setConn(true, "Connected");
      ws.onclose = () => { setConn(false, "Disconnected"); setTimeout(connectWs, 1200); };
      ws.onerror = () => { /* will trigger close */ };
      ws.onmessage = (ev) => {
        try { applyStatus(JSON.parse(ev.data)); }
        catch(e){}
      };
    }

    // Start WS and also do a periodic fallback poll.
    connectWs();
    fetchFallback();
    setInterval(() => {
      if (!ws || ws.readyState !== 1) return fetchFallback();
      if (Date.now() - lastOk > 5000) return fetchFallback();
    }, 1500);
  </script>
</body>
</html>
)HTML";

static void sendStatusJson() {
  JsonDocument doc;
  doc["deviceName"] = g_cfg.deviceName;
  doc["distanceCm"] = g_reading.distanceCm;
  doc["belowMinimum"] = g_reading.belowMinimum;
  doc["updatedAtMs"] = millis();
  doc["wifiSsid"] = (WiFi.SSID().length() ? WiFi.SSID().c_str() : "");
  doc["ip"] = WiFi.localIP().toString();

  String out;
  serializeJson(doc, out);
  g_http.send(200, "application/json", out);
}

static void pushStatusWs() {
  JsonDocument doc;
  doc["deviceName"] = g_cfg.deviceName;
  doc["distanceCm"] = g_reading.distanceCm;
  doc["belowMinimum"] = g_reading.belowMinimum;
  doc["updatedAtMs"] = millis();
  doc["wifiSsid"] = (WiFi.SSID().length() ? WiFi.SSID().c_str() : "");
  doc["ip"] = WiFi.localIP().toString();

  char buf[256];
  const size_t n = serializeJson(doc, buf, sizeof(buf));
  g_ws.broadcastTXT(buf, n);
}

void begin(const Config &cfg) {
  g_cfg = cfg;

  g_http.on("/", HTTP_GET, []() {
    g_http.send_P(200, "text/html", kIndexHtml);
  });
  g_http.on("/api/status", HTTP_GET, []() { sendStatusJson(); });
  g_http.onNotFound([]() {
    g_http.send(404, "text/plain", "Not found");
  });

  g_ws.begin();
  g_ws.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    (void)num;
    (void)payload;
    (void)length;
    if (type == WStype_CONNECTED) {
      pushStatusWs();
    }
  });

  g_http.begin();
  Serial.printf("[WEB] HTTP on :%u, WS on :%u\n", 80u, 81u);
}

void updateReading(const Reading &r) { g_reading = r; }

void loop() {
  g_http.handleClient();
  g_ws.loop();

  const uint32_t now = millis();
  if (now - g_lastPushMs >= 250) {
    g_lastPushMs = now;
    pushStatusWs();
  }
}

} // namespace webpage

