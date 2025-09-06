#pragma once
#include <Arduino.h>
#include "Display.h"     // getTFT(), CLR_BG/CLR_FG
#include "Anim.h"        // animLoopStep() for center monster
#include "Pokemon.h"     // pokemonMgr feed/play
#include "Pomodoro.h"    // pomodoro state/time

#ifndef CHROMA565
#define CHROMA565 0xF81F  // magenta as transparent key if you need one elsewhere
#endif
// ---- Forward declarations (因為下方會先用到) ----
static inline void drawTopHUD();
static inline void drawBottomHUD();
static inline void drawMainStage();
static inline void uiRedrawAll();
static inline void uiPomodoroDrawStatic();

// =========================
//         UI STATE
// =========================

enum UiState {
  UI_HOME = 0,
  UI_MENU,
  UI_STATUS,
  UI_STUDY,
  UI_BATTLE,
  UI_NORMAL
};

static UiState gUiState = UI_NORMAL; // start with normal so title shows selected menu
static int     gMenuIndex = 0;       // bottom menu cursor 0..4 (5 items)

// =========================
//      LAYOUT HELPERS
// =========================

static inline int kHeaderH(){ return 26; }
static inline int kBottomH(){ return 56; }

struct StripLayout { int y0; int h; int gap; int x0; int count; };

static inline StripLayout bottomStripLayout(){
  auto &t = getTFT();
  const int W = t.width();
  const int H = t.height();
  const int h = kBottomH();    // bottom bar height
  const int y0 = H - h;        // top of the bar
  const int count = 5;         // 5 menu items now (Feed/Train/Play/Battle/Pomo)
  const int gap = W / (count + 1); // equal spacing with margins
  const int x0 = gap;          // first center x
  return { y0, h, gap, x0, count };
}

static inline void clearMainArea(){
  auto &t = getTFT();
  const int y0 = kHeaderH();
  const int h  = t.height() - kHeaderH() - kBottomH();
  t.fillRect(0, y0, t.width(), h, CLR_BG);
}

static inline void cuteCardFrame(int x, int y, bool selected){
  auto &t = getTFT();
  uint16_t bg = 0x0000;          // black bar bg
  uint16_t frame = selected ? 0xFFE0 /*yellow*/ : 0x4208 /*dark gray*/;
  // soft background
  t.fillRoundRect(x, y, 32, 32, 6, bg);
  t.drawRoundRect(x, y, 32, 32, 6, frame);
}

static inline void drawHeaderBar(const String &title){
  auto &t = getTFT();
  const int W = t.width();
  const int barH = kHeaderH();
  t.fillRect(0,0,W,barH, CLR_BG);
  t.drawFastHLine(0,barH-1,W, CLR_FG);
  t.setTextColor(CLR_FG, CLR_BG);
  t.setCursor(6,5);
  t.setTextSize(2);
  t.print(title);
}

static inline void drawBottomBarFrame(){
  auto &t = getTFT();
  auto L = bottomStripLayout();
  t.fillRect(0, L.y0, t.width(), L.h, CLR_BG);
  t.drawFastHLine(0, L.y0, t.width(), CLR_FG);
}

// ---- Tiny vector icons (no external icon arrays required) ----
static inline void iconFeed(int x, int y, bool sel){
  auto &t = getTFT();
  cuteCardFrame(x,y,sel);
  // plate
  t.fillCircle(x+16, y+20, 10, 0xFFFF);
  t.drawCircle(x+16, y+20, 10, 0x4208);
  // apple
  t.fillCircle(x+16, y+12, 5, 0xF800);
  // leaf
  t.fillTriangle(x+18,y+8, x+22,y+10, x+18,y+12, 0x07E0);
  // stem
  t.drawLine(x+16,y+7, x+16,y+10, 0x4208);
}

static inline void iconTrain(int x, int y, bool sel){
  auto &t = getTFT();
  cuteCardFrame(x,y,sel);
  // dumbbell: two weights + bar
  t.fillRoundRect(x+6, y+12, 6, 10, 2, 0x7BEF);
  t.fillRoundRect(x+20, y+12, 6, 10, 2, 0x7BEF);
  t.fillRect(x+12, y+16, 8, 2, 0x4208);
}

static inline void iconPlay(int x, int y, bool sel){
  auto &t = getTFT();
  cuteCardFrame(x,y,sel);
  // toy ball with star
  t.fillCircle(x+16, y+16, 11, 0x5AD6);
  t.drawCircle(x+16, y+16, 11, 0x4208);
  // star (simple)
  t.fillTriangle(x+16,y+6,  x+20,y+17, x+12,y+17, 0xFFE0);
  t.fillTriangle(x+16,y+26, x+20,y+15, x+12,y+15, 0xFFE0);
}

static inline void iconBattle(int x, int y, bool sel){
  auto &t = getTFT();
  cuteCardFrame(x,y,sel);
  // crossed swords (simple lines)
  t.drawLine(x+10,y+22, x+22,y+10, 0xFFFF);
  t.drawLine(x+10,y+10, x+22,y+22, 0xFFFF);
  // handles
  t.drawLine(x+9,y+23, x+7,y+25, 0xF81F);
  t.drawLine(x+23,y+23, x+25,y+25, 0xF81F);
}

static inline void iconPomo(int x, int y, bool sel){
  auto &t = getTFT();
  cuteCardFrame(x,y,sel);
  // tomato body
  t.fillCircle(x+16, y+18, 9, 0xF800);    // red
  t.drawCircle(x+16, y+18, 9, 0x4208);
  // leaf + stem
  t.fillTriangle(x+14,y+8, x+19,y+10, x+16,y+12, 0x07E0);
  t.drawLine(x+16,y+7, x+16,y+11, 0x4208);
}

static inline void drawMenuIcons(){
  auto L = bottomStripLayout();
  for (int i=0;i<L.count;i++){
    const int cx = L.x0 + i * L.gap;  // evenly spaced centers
    const int x  = cx - 16;           // top-left for 32×32
    const int y  = L.y0 + 8;
    const bool sel = (i==gMenuIndex);
    switch(i){
      case 0: iconFeed  (x,y,sel); break;
      case 1: iconTrain (x,y,sel); break;
      case 2: iconPlay  (x,y,sel); break;
      case 3: iconBattle(x,y,sel); break;
      case 4: iconPomo  (x,y,sel); break;  // NEW: Pomodoro
    }
  }
}

// =========================
//       POMODORO UI (no flicker)
// =========================

// Pause support
static bool     gPomoPaused = false;
static uint32_t gPomoRemainSec = 0;
static PomodoroState gPomoPausedState = STATE_IDLE;


// Only redraw the time stamp area, not the whole screen
static inline void uiPomodoroDrawTime(){
  auto &t = getTFT();
  int tx = t.width()/2 - 36;   // text pos
  int ty = t.height()/2 - 8;
  t.setTextSize(2);
  t.setTextColor(CLR_FG, CLR_BG);

  String s;
  if (gPomoPaused){
    s = "Paused";
  } else if (pomodoroCurrentState == STATE_IDLE){
    s = "Idle";
  } else {
    uint32_t now = millis();
    uint32_t rem = 0;
    if (now < pomodoroEndTime) rem = (pomodoroEndTime - now)/1000;
    uint32_t mm = rem / 60, ss = rem % 60;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)mm, (unsigned long)ss);
    s = buf;
  }
  // clear only the time area (lightweight)
  t.fillRect(tx-6, ty-2, 100, 20, CLR_BG);
  t.setCursor(tx, ty);
  t.print(s);
}

static inline void uiPomodoroDrawStatic(){
  clearMainArea();
  auto &t = getTFT();
  // optional frame around time
  int cx = t.width()/2 - 60, cy = t.height()/2 - 16;
  t.drawRoundRect(cx, cy, 120, 32, 6, CLR_FG);
  uiPomodoroDrawTime();
}

static inline void uiPomodoroEnter(){
  gPomoPaused = false;
  gUiState = UI_STUDY;
  drawTopHUD();
  uiPomodoroDrawStatic();
}

static inline void uiPomodoroTick(){
  static uint32_t lastSec = 0;
  if (!gPomoPaused) pomodoroLoopStep();  // keep state machine running when not paused
  uint32_t nowSec = millis()/1000;
  if (nowSec != lastSec){
    lastSec = nowSec;
    uiPomodoroDrawTime();
  }
}

// =========================
//          PUBLIC
// =========================

static inline void drawTopHUD(){
  // ASCII only to avoid mojibake
  static const char* kMenuNames[5] = { "Feed", "Train", "Play", "Battle", "Pomodoro" };
  String title;
  switch(gUiState){
    case UI_STUDY:  title = "Pomodoro"; break;
    case UI_BATTLE: title = "Battle";   break;
    case UI_STATUS: title = "Status";   break;
    case UI_MENU:   title = kMenuNames[gMenuIndex]; break;
    case UI_HOME:
    case UI_NORMAL:
    default:        title = kMenuNames[gMenuIndex]; break;
  }
  drawHeaderBar(title);
}

static inline void drawBottomHUD(){
  drawBottomBarFrame();
  drawMenuIcons();
}

static inline void drawMainStage(){
  if (gUiState == UI_STUDY){
    uiPomodoroDrawStatic();
  } else {
    clearMainArea();
  }
}

static inline void uiRedrawAll(){
  drawTopHUD();
  drawMainStage();
  drawBottomHUD();
}

// Back-compat overloads (某些舊呼叫有帶 bool)
static inline void drawTopHUD(bool /*force*/){ drawTopHUD(); }
static inline void drawBottomHUD(bool /*force*/){ drawBottomHUD(); }

static inline void drawStudyHUD(){ uiPomodoroEnter(); }

static inline void doMenuAction(int idx){
  switch(idx){
    case 0: pokemonMgr.feedPokemon(); break;         // FEED
    case 1: pokemonMgr.playWithPokemon(); break;     // TRAIN (暫以 play 動作代替)
    case 2: pokemonMgr.playWithPokemon(); break;     // PLAY
    case 3: gUiState = UI_BATTLE; break;             // BATTLE
    case 4: uiPomodoroEnter(); return;               // POMODORO via menu
  }
  if (gUiState != UI_BATTLE && gUiState != UI_STUDY) gUiState = UI_NORMAL;
  uiRedrawAll();
}

// k: 1..5 from your keypad mapper
static inline void uiOnKey(int k){
  if (k<=0) return;

  // === STUDY PAGE ===
  if (gUiState == UI_STUDY){
    // K1: exit, K2: +5m, K3: -5m, K4: pause, K5: start/resume
    if (k==1){
      gUiState = UI_HOME;         // exit only (timer keeps running if not paused)
      uiRedrawAll();
    } else if (k==2){             // +5m (cap 120)
      if (pomodoroWorkMinutes < 120) pomodoroWorkMinutes += 5;
      uiPomodoroDrawStatic();
    } else if (k==3){             // -5m (floor 5)
      if (pomodoroWorkMinutes >= 10) pomodoroWorkMinutes -= 5; else pomodoroWorkMinutes = 5;
      uiPomodoroDrawStatic();
    } else if (k==4){             // pause
      if (!gPomoPaused && (pomodoroCurrentState == STATE_WORK || pomodoroCurrentState == STATE_REST)){
        uint32_t now = millis();
        uint32_t remMs = (pomodoroEndTime > now) ? (pomodoroEndTime - now) : 0;
        gPomoRemainSec  = remMs / 1000;
        gPomoPausedState= pomodoroCurrentState;
        gPomoPaused     = true;
        stopPomodoro();
        uiPomodoroDrawStatic();
      }
    } else if (k==5){             // start / resume
      if (gPomoPaused){
        pomodoroCurrentState = gPomoPausedState;      // resume previous state
        pomodoroEndTime = millis() + gPomoRemainSec * 1000UL;
        gPomoPaused = false;
        uiPomodoroDrawStatic();
      } else {
        stopPomodoro();
        startPomodoroWork();
        uiPomodoroDrawStatic();
      }
    }
    return;
  }

  // === ALWAYS: 左右可移動選單，K3 執行，K1 回到 HOME ===
  if (k==2){               // left
    if (--gMenuIndex < 0) gMenuIndex = 4;
    drawBottomHUD();
    drawTopHUD();
    return;
  }
  if (k==4){               // right
    if (++gMenuIndex > 4) gMenuIndex = 0;
    drawBottomHUD();
    drawTopHUD();
    return;
  }
  if (k==3){               // select action directly (includes Pomodoro)
    doMenuAction(gMenuIndex);
    return;
  }
  if (k==1){               // back/home
    gUiState = UI_HOME;
    uiRedrawAll();
    return;
  }
}
