#include "AdminServer.h"
#include "Storage.h"
#include <WiFi.h>
#include <WebServer.h>

static WebServer server(80);
static bool wifiReady = false;

static String chipIdStr(){
  uint64_t mac = ESP.getEfuseMac();
  char buf[9];
  snprintf(buf, sizeof(buf), "%08X", (uint32_t)(mac & 0xFFFFFFFF));
  return String(buf);
}

static String getTokenFromReq(){
  // Prefer query param
  if (server.hasArg("token")) return server.arg("token");
  // Fallback: header
  if (server.hasHeader("X-Token")) return server.header("X-Token");
  return String();
}

static bool checkAuth(){
  String tok = getTokenFromReq();
  if (tok.length() == 0) return false;
  return tok == gs().token;
}

static void handleStatus(){
  String s = stateToJson();
  server.send(200, "application/json", s);
}

static void handleReward(){
  if (!checkAuth()) { server.send(401, "text/plain", "Unauthorized"); return; }
  int p = 0;
  if (server.hasArg("points")) p = server.arg("points").toInt();
  else if (server.method() == HTTP_POST && server.hasArg("plain")){
    // naive parse for {"points":N}
    StaticJsonDocument<128> doc; DeserializationError e = deserializeJson(doc, server.arg("plain"));
    if (!e) p = doc["points"] | 0;
  }
  if (p <= 0){ server.send(400, "text/plain", "points>0 required"); return; }
  addPoints((uint32_t)p);
  server.send(200, "application/json", stateToJson());
}

static void handleTrain(){
  if (!checkAuth()) { server.send(401, "text/plain", "Unauthorized"); return; }
  int c = 0;
  if (server.hasArg("count")) c = server.arg("count").toInt();
  else if (server.method() == HTTP_POST && server.hasArg("plain")){
    StaticJsonDocument<128> doc; DeserializationError e = deserializeJson(doc, server.arg("plain"));
    if (!e) c = doc["count"] | 0;
  }
  if (c <= 0){ server.send(400, "text/plain", "count>0 required"); return; }
  addTraining((uint32_t)c);
  server.send(200, "application/json", stateToJson());
}

static const char kAdminPage[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>PomoMonster Admin</title>
<style>body{font-family:system-ui,Arial;margin:16px}fieldset{margin:12px 0}input[type=number]{width:8ch}</style>
</head><body>
<h1>PomoMonster Admin</h1>
<div id=st>Loading...</div>
<fieldset><legend>Reward Points</legend>
<input id=p type=number min=1 value=10> <button onclick="sendReward()">Give</button>
</fieldset>
<fieldset><legend>Training</legend>
<input id=c type=number min=1 value=1> <button onclick="sendTrain()">Add</button>
</fieldset>
<script>
const TOKEN = new URL(location).searchParams.get('token') || '';
async function getStatus(){
  const r = await fetch('/api/status');
  const j = await r.json();
  st.textContent = JSON.stringify(j);
}
async function sendReward(){
  const p = parseInt(document.getElementById('p').value||'0');
  const r = await fetch('/api/reward?token='+encodeURIComponent(TOKEN)+'&points='+p,{method:'POST'});
  st.textContent = await r.text();
}
async function sendTrain(){
  const c = parseInt(document.getElementById('c').value||'0');
  const r = await fetch('/api/train?token='+encodeURIComponent(TOKEN)+'&count='+c,{method:'POST'});
  st.textContent = await r.text();
}
getStatus();
</script>
</body></html>
)HTML";

static void handleAdmin(){
  server.send_P(200, "text/html", kAdminPage);
}

static void startWiFi(){
  // Try STA if creds available
  if (gs().sta_ssid.length()){ 
    WiFi.mode(WIFI_STA);
    WiFi.begin(gs().sta_ssid.c_str(), gs().sta_pass.c_str());
    Serial.printf("[WiFi] Connecting to %s...\n", gs().sta_ssid.c_str());
    unsigned long t0 = millis();
    while (WiFi.status()!=WL_CONNECTED && millis()-t0<8000){ delay(200); Serial.print('.'); }
    Serial.println();
    if (WiFi.status()==WL_CONNECTED){
      Serial.printf("[WiFi] STA IP: %s\n", WiFi.localIP().toString().c_str());
      wifiReady = true; return;
    }
  }
  // Fallback AP
  String ssid = String("PomoMonster-") + chipIdStr();
  String pass = "pomomonster";
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(ssid.c_str(), pass.c_str());
  if (ok){
    Serial.printf("[WiFi] AP %s started, pass=%s, IP=%s\n", ssid.c_str(), pass.c_str(), WiFi.softAPIP().toString().c_str());
    wifiReady = true;
  } else {
    Serial.println("[WiFi] AP start failed");
  }
}

void adminBegin(){
  loadGameState();
  startWiFi();
  server.on("/", HTTP_GET, handleAdmin);
  server.on("/admin", HTTP_GET, handleAdmin);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/reward", HTTP_POST, handleReward);
  server.on("/api/train", HTTP_POST, handleTrain);
  server.begin();
  Serial.println("[HTTP] Server started on :80");
}

void adminLoop(){
  server.handleClient();
}

// Event hooks
void backendOnPomodoroCompleted(){
  // Count complete and auto-give small base reward
  incPomodoro();
  addPoints(5);
  Serial.println("[Backend] Pomodoro completed: +1 count, +5 points");
}

