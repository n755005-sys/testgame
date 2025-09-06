// ===== game.ino — works with pixel_icons_rgb565.h (16x16), fast keys, clean bottom menu (no white fill),
// ===== custom Pomodoro (K1 start/stop, K2 +5min, idle shows MM:00, no flicker) =====
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>

#include "Display.h"    // getTFT(), displayInitPortrait(), CLR_BG/CLR_FG
#include "Anim.h"       // animSetMonster(), animLoopStep(), CHROMA_KEY
#include "Uploader.h"   // uploaderBegin(), uploaderLoop()
#include "Pomodoro.h"   // startPomodoroWork(), stopPomodoro(), pomodoro* vars
#include "Pokemon.h"    // pokemonMgr
#include "AdminServer.h"  // WiFi + Admin backend
#include "pixel_icons_rgb565.h"  // 你的圖示 header（預設 16x16）
#include "pomodoro_icons.h"      // 新增：番茄鐘專用圖示
#include "ui.h"      // <-- 要在用到 CHROMA565 之前 include


// ★真正的全域定義（只能有一份）
PokemonManager pokemonMgr;
static uint32_t gPomoLastRedraw = 0; 

// ========================= Icon 尺寸與繪圖（避免「條紋」） =========================
// 若改用 32x32 的新圖，只要把這裡改為 32 即可
static const int ICON_W = 32;
static const int ICON_H = 32;

static inline void blitIcon(int x, int y, const uint16_t* data, uint16_t w, uint16_t h, bool center=false, uint8_t scale=1){
  auto &t = getTFT(); 
  if(center){ x -= (w*scale)/2; y -= (h*scale)/2;
  }
  for(int j=0;j<h;j++){
    const uint16_t* row = data + j*w;
    for(int i=0;i<w;i++){
      uint16_t c = row[i];
      if (c == CHROMA565) continue;
      if (scale==1) t.drawPixel(x+i, y+j, c);
      else t.fillRect(x+i*scale, y+j*scale, scale, scale, c);
    }
  }
}


// ========================= KEYPAD (ADC) — 更靈敏 =========================
enum { K_NONE=0, K1=1, K2=2, K3=3, K4=4, K5=5 };
// 你的實機對應：1->K3, 2->K2, 3->K1, 4->K4, 5->K5
static int g_levelToKey[6] = {0, 3, 2, 1, 4, 5};
static const int BTN_ADC_PIN   = 7;
static const int BTN_TH1       =  262;
static const int BTN_TH2       =  878;
static const int BTN_TH3       = 1915;
static const int BTN_TH4       = 2420;
static const int BTN_IDLE_MIN  = 3800;
static const int BTN_DEBOUNCE  = 15;   // 更快

static int g_lastReadLvl = 0;           // 立即層級
static int g_lastStableLvl = 0;
// 去彈跳層級
static unsigned long g_lastChangeMs = 0;

static inline void buttonsBegin(){
  pinMode(BTN_ADC_PIN, INPUT);
  analogReadResolution(12);                         // 0..4095
  analogSetPinAttenuation(BTN_ADC_PIN, ADC_11db);
// ~3.3V
}
static inline int readADCavg(uint8_t n=3){
  long s=0;
  for(uint8_t i=0;i<n;i++) s += analogRead(BTN_ADC_PIN);
  return (int)(s/n);
}
static inline int rawToLevel(int v){
  if(v < BTN_TH1) return 1;
  if(v < BTN_TH2) return 2;
  if(v < BTN_TH3) return 3;
  if(v < BTN_TH4) return 4;
  if(v < BTN_IDLE_MIN) return 5;
  return 0; // idle
}
static int g_lastStableKey = K_NONE;
static int buttonJustPressed(){
  int currentKey = g_lastStableKey;
  if(currentKey == K_NONE) return K_NONE;
  int key = g_levelToKey[currentKey];
  g_lastStableKey = K_NONE;
  return key;
}
static void updateButtons(){
  int v = readADCavg();
  int lvl = rawToLevel(v);
  if(lvl != g_lastReadLvl){
    g_lastReadLvl = lvl;
    g_lastChangeMs = millis();
  }
  if(millis() - g_lastChangeMs > BTN_DEBOUNCE && lvl != g_lastStableLvl){
    g_lastStableLvl = lvl;
    if(lvl != 0){
      g_lastStableKey = lvl;
    }
  }
}
// ========================= UI (from ui.h) =========================


// ========================= SETUP / LOOP =========================
void setup(){
  Serial.begin(115200);
  delay(100);
  SPIFFS.begin(true);

  // 你先前說螢幕有翻轉，所以這裡用 false（直向 0 度）
  displayInitPortrait(false);
  getTFT().fillScreen(CLR_BG);

  uploaderBegin();
  pokemonMgr.begin();
  adminBegin();

  // 指定怪獸動畫（依你的檔案調整）
  animSetMonster("monster_001", "frame", 2, CHROMA_KEY);

  buttonsBegin();

  drawTopHUD(true);
  drawBottomHUD(true);
  drawTopHUD();
  drawBottomHUD();
  clearMainArea();   // 這行需要你已套用我給你的新版 ui.h
}

void loop() {
  updateButtons();
  int k = buttonJustPressed();
  if (k) uiOnKey(k);

  uploaderLoop();
  pokemonMgr.update();
  adminLoop();

  if (gUiState == UI_STUDY) {
    uiPomodoroTick();        // ← 每秒只更新時間，不閃爍
  } else {
    animLoopStep();          // 中央怪獸動畫
  }
}
