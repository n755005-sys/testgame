#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include "Display.h"
#include "Pomodoro.h"

static WebServer server(80);
static char ssid_ap[32];
static const char* pass_ap = "12345678";

// ===== HTML（raw string，避免引號問題）=====
static const char HTML_UP[] PROGMEM = R"=====( 
<!doctype html><meta charset="utf-8">
<title>ESP32 SPIFFS Uploader</title>
<body style="font-family:sans-serif;max-width:760px">
<h2>ESP32 SPIFFS 多檔上傳</h2>

<form method="POST" action="/upload" enctype="multipart/form-data" onsubmit="saveForm()">
  <fieldset style="border:1px solid #ccc;padding:10px">
    <legend>儲存目標</legend>

    <label><input type="radio" name="mode" value="monster" checked onclick="syncMode()"> 用 <b>Monster 代號</b>（例：monster_001）</label><br>
    &nbsp;&nbsp;Monster：<input id="monster" name="monster" value="monster_001" size="20" oninput="syncMode()">
    → 會存到 <code id="monPath">/m/monster_001/</code><br><br>

    <label><input type="radio" name="mode" value="dir" onclick="syncMode()"> 指定 <b>目標資料夾</b></label><br>
    &nbsp;&nbsp;資料夾：<input id="dir" name="dir" value="/m/monster_001" size="30" oninput="syncMode()"><br><br>

    <label><input type="radio" name="mode" value="auto" onclick="syncMode()"> <b>AUTO</b> 依檔名分流</label>
    <span style="opacity:.75">（檔名形如 <code>monster_007_idle_0.bmp</code> → 自動進 <code>/m/monster_007/</code>）</span>
  </fieldset>

  <p>檔案（可多選）：<input type="file" name="upload" multiple></p>
  <p><button type="submit">上傳</button></p>
</form>

<hr>
<ul>
<li><a href="/list">檔案列表</a></li>
<li>查檔：
  <form style="display:inline" method="GET" action="/exists">
    <input name="path" value="/m/monster_001/idle_0.bmp" size="32">
    <button type="submit">exists?</button>
  </form>
</li>
<li>
  <form method="POST" action="/format" onsubmit="return confirm('確定要清空 SPIFFS？');">
    <button type="submit" style="color:red">⚠ 清空 SPIFFS</button>
  </form>
</li>
</ul>

<script>
function syncMode(){
  const m = document.querySelector('input[name=mode]:checked').value;
  const monster = document.getElementById('monster').value.trim();
  const dir = document.getElementById('dir');
  const monPath = document.getElementById('monPath');
  if(m==='monster'){
    if(monster){ monPath.textContent = '/m/' + monster + '/'; dir.value = '/m/' + monster; }
  }
}
function saveForm(){
  localStorage.setItem('monster', document.getElementById('monster').value);
  localStorage.setItem('dir', document.getElementById('dir').value);
  const m = document.querySelector('input[name=mode]:checked').value;
  localStorage.setItem('mode', m);
}
(function(){
  const m = localStorage.getItem('monster'); if(m) document.getElementById('monster').value = m;
  const d = localStorage.getItem('dir');     if(d) document.getElementById('dir').value = d;
  const mode = localStorage.getItem('mode');
  if(mode){
    const r = document.querySelector('input[name=mode][value="'+mode+'"]');
    if(r){ r.checked = true; }
  }
  syncMode();
})();
</script>
</body>
)=====";

// ====== 基本頁面與工具端點 ======
static void handleRoot(){ server.send_P(200,"text/html",HTML_UP); }

static void handleList(){
  String out = "== SPIFFS files ==\n";
  File root = SPIFFS.open("/");
  if(!root){ server.send(500,"text/plain","open / fail"); return; }
  File f = root.openNextFile();
  if(!f) out += "(empty)\n";
  while(f){
    out += String(f.isDirectory() ? "[D] " : "") + String(f.name()) + "  (" + String((uint32_t)f.size()) + " bytes)\n";
    f = root.openNextFile();
  }
  server.send(200,"text/plain",out);
}

static void handleExists(){
  if(!server.hasArg("path")){ server.send(400,"text/plain","need ?path="); return; }
  String p = server.arg("path");
  if(!p.startsWith("/")) p = "/"+p;
  server.send(200,"text/plain", String(SPIFFS.exists(p)?"YES ":"NO ") + p);
}

static void handleFormat(){ SPIFFS.format(); server.sendHeader("Location","/"); server.send(303); }

// ====== 上傳處理 ======
static File upF;

static void handleUploadData(){
  HTTPUpload& up = server.upload();
  if(up.status==UPLOAD_FILE_START){
    // --- 讀取表單選項 ---
    String mode = server.hasArg("mode") ? server.arg("mode") : "monster";
    String monster = server.hasArg("monster") ? server.arg("monster") : "";
    String dir = server.hasArg("dir") ? server.arg("dir") : "";

    monster.replace("\\","/"); dir.replace("\\","/");
    if(dir=="" || dir==" ") dir = "/";

    // --- 計算最終目標資料夾 ---
    String targetDir = "/";
    if(mode=="monster"){
      if(monster=="") monster="monster_001";
      targetDir = "/m/" + monster + "/";
    }else if(mode=="dir"){
      targetDir = dir; if(!targetDir.startsWith("/")) targetDir = "/"+targetDir;
      if(!targetDir.endsWith("/")) targetDir += "/";
    }else{ // AUTO 模式：解析檔名 monster_xxx_...
      targetDir = "/m/auto/"; // 先暫存，下面會依檔名修正
    }

    // 原始檔名
    String fname = up.filename; fname.replace("\\","/");

    // AUTO：從檔名前綴取 monster 代號
    if(mode=="auto"){
      int us = fname.indexOf('_');
      String mon = (us>0)? fname.substring(0,us) : "misc";
      targetDir = "/m/" + mon + "/";
    }

    String path = targetDir + fname;
    SPIFFS.remove(path);
    upF = SPIFFS.open(path, FILE_WRITE);
    Serial.printf("[UPLOAD] start -> %s\n", path.c_str());
  }
  else if(up.status==UPLOAD_FILE_WRITE){
    if(upF) upF.write(up.buf, up.currentSize);
  }
  else if(up.status==UPLOAD_FILE_END){
    if(upF) upF.close();
    Serial.printf("[UPLOAD] done  -> (%u bytes)\n", up.totalSize);
  }
}

static void handleUploadDone(){ server.sendHeader("Location","/"); server.send(303); }

// ====== 啟動 AP 與 HTTP 伺服器 ======
inline void uploaderBegin(){
  uint32_t tail = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF);
  snprintf(ssid_ap,sizeof(ssid_ap),"ESP32-UP-%06X", tail);

  WiFi.mode(WIFI_OFF); delay(80);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap, pass_ap, 1, 0, 4);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/list", HTTP_GET, handleList);
  server.on("/exists", HTTP_GET, handleExists);
  server.on("/format", HTTP_POST, handleFormat);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUploadData);
  server.begin();

  auto &tft = getTFT();
  tft.fillScreen(CLR_BG);
  tft.setTextColor(CLR_FG, CLR_BG);
  tft.setCursor(6,6);
  tft.print("AP: "); tft.println(ssid_ap);
  tft.print("PW: "); tft.println(pass_ap);
  tft.print("IP: "); tft.println(WiFi.softAPIP().toString());
  tft.println("Open / in browser");
  // 在 Uploader.h 的 uploaderBegin() 內， server.begin() 之前加入
// ===== Web Handlers for Pomodoro =====
  server.on("/pomodoro", HTTP_GET, [](){
    String html = R"=====(
    <!doctype html><meta charset="utf-8">
    <title>Pomodoro Settings</title>
    <body style="font-family:sans-serif;max-width:760px">
    <h2>番茄鐘設定</h2>
    <form method="POST" action="/pomodoro/set">
      <p>
        工作時間 (分鐘): <input type="number" name="work" value=")=====";
    html += String(pomodoroWorkMinutes);
    html += R"=====(">
      </p>
      <p>
        休息時間 (分鐘): <input type="number" name="rest" value=")=====";
    html += String(pomodoroRestMinutes);
    html += R"=====(">
      </p>
      <p>
        <button type="submit" name="action" value="start">開始工作</button>
        <button type="submit" name="action" value="stop">停止計時</button>
      </p>
    </form>
    <hr>
    <p><a href="/">返回檔案上傳</a></p>
    <p><a href="/setMode?mode=monster">切換至怪獸動畫</a></p>
    </body>
    )=====";
    server.send(200, "text/html", html);
});

server.on("/pomodoro/set", HTTP_POST, [](){
  if (server.hasArg("work") && server.hasArg("rest")) {
    pomodoroWorkMinutes = server.arg("work").toInt();
    pomodoroRestMinutes = server.arg("rest").toInt();
    Serial.printf("[Pomodoro] Settings updated: Work=%d, Rest=%d\n", pomodoroWorkMinutes, pomodoroRestMinutes);
  }

  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "start") {
      startPomodoroWork();
    } else if (action == "stop") {
      stopPomodoro();
    }
  }
  
  // 處理完後跳轉回設定頁
  server.sendHeader("Location", "/pomodoro");
  server.send(303);
});
// ===== END of Pomodoro Handlers =====
}

inline void uploaderLoop(){ server.handleClient(); }
