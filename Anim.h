#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <math.h>
#include "Display.h"

// ===== 可調參數 =====
static const uint32_t FRAME_MS = 150;   // 幀間隔(ms)
static const int      BOB_PIX  = 2;     // 上下起伏幅度(px)
static const float    BOB_W    = 0.25f; // 起伏角速度
static const int      STEP_X   = 3;     // 每幀水平位移(px)
static const int      SCALE_DEFAULT = 2;// 64x64*2=128x128
static const uint16_t CHROMA_KEY = 0xF81F; // #FF00FF

// ===== 動畫內部狀態 =====
static String   g_mon   = "monster_001";  // 資料夾名（/m/monster_001）
static String   g_state = "frame";        // 幀前綴（frame0.bmp..）
static int      g_frameCount = 0;         // 幀數
static int      g_cur   = 0;              // 當前幀
static int      g_scale = SCALE_DEFAULT;  
static uint16_t g_chroma565 = CHROMA_KEY;

static uint32_t g_last = 0;               // 上次換幀時間
static int      g_x=0, g_y=0;             // 螢幕座標
static int      g_w=0, g_h=0;             // 精靈顯示尺寸
static int      g_prevX=-10000, g_prevY=-10000;// 上一幀位置（反閃爍用）
static int      g_vx = STEP_X;            // 水平速度，<0 表示向左

// ===== BMP 讀取 =====
static bool readBmpHeader(fs::File &f, int &w, int &h, uint16_t &bpp, uint32_t &comp, uint32_t &off, bool &flip){
  uint8_t H[54]; if (f.read(H,54) != 54) return false;
  if (*(uint16_t*)&H[0] != 0x4D42) return false; // 'BM'
  off = *(uint32_t*)&H[10];
  w   = *(int32_t*)&H[18];
  h   = *(int32_t*)&H[22];
  bpp = *(uint16_t*)&H[28];
  comp= *(uint32_t*)&H[30];
  bool neg = (h < 0);
  if (neg) h = -h;
  flip = !neg; // 正常 BMP 自底向上，flip=true 時我們要反著讀
  return true;
}

static bool readBmpRow565(fs::File &f, uint32_t off, int w, int row, uint16_t bpp, uint32_t comp, uint16_t* out){
  uint32_t rowSize = ((bpp * w + 31) / 32) * 4;
  f.seek(off + (uint32_t)row * rowSize);
  if (bpp == 24) {
    for (int x=0; x<w; ++x) {
      uint8_t B=f.read(), G=f.read(), R=f.read();
      out[x] = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
    }
    return true;
  }
  if (bpp == 16 && (comp==0 || comp==3)) {
    for (int x=0; x<w; ++x) {
      uint16_t px; f.read((uint8_t*)&px, 2);
      out[x] = px; // 已是 565
    }
    return true;
  }
  return false;
}

// 把一張 BMP 放大寫到 canvas；mirrorX=true 代表水平鏡像；chroma565=透明色（-1=關閉透明）
static bool compositeFrameToCanvas(const char* path, uint16_t* canvas, int cw, int ch,
                                   int cx, int cy, int scale, int chroma565, bool mirrorX,
                                   int &outW, int &outH){
  fs::File f = SPIFFS.open(path, "r"); if (!f) return false;
  int w,h; uint16_t bpp; uint32_t comp, off; bool flip;
  if (!readBmpHeader(f, w, h, bpp, comp, off, flip)) { f.close(); return false; }
  if (!((bpp==24 && comp==0) || (bpp==16 && (comp==0 || comp==3)))) { f.close(); return false; }

  uint16_t* line = (uint16_t*)malloc(w*2); if (!line){ f.close(); return false; }
  outW = w * scale; outH = h * scale;

  static uint16_t* temp = nullptr; static int cap = 0;
  if (outW > cap) { if (temp) free(temp); temp = (uint16_t*)malloc(outW*2); cap = outW; }

  for (int sy=0; sy<h; ++sy){
    int srcRow = flip ? (h-1-sy) : sy;
    if (!readBmpRow565(f, off, w, srcRow, bpp, comp, line)) { free(line); f.close(); return false; }

    // 水平放大 → temp[]
    int dx=0;
    for (int i=0; i<w; ++i){
      uint16_t c = line[i];
      uint16_t o = (chroma565 >= 0 && c == (uint16_t)chroma565) ? 0 : c;
      for (int k=0; k<scale; ++k) temp[dx++] = o;
    }

    // 寫入 canvas（支援水平鏡像；值 0 視為透明、不覆蓋）
    for (int k=0; k<scale; ++k){
      int yy = cy + sy*scale + k; if (yy < 0 || yy >= ch) continue;
      uint16_t* row = &canvas[yy*cw];
      for (int i=0; i<outW; ++i){
        int xx = cx + (mirrorX ? (outW-1-i) : i);
        if (xx < 0 || xx >= cw) continue;
        uint16_t c = temp[i]; if (c) row[xx] = c;
      }
    }
  }
  free(line); f.close(); return true;
}

// 掃描幀數：支援 frame0.bmp.. 或 frame_0.bmp.. 連號
static int scanFrames(const String& folder, const String& state){
  int n=0;
  for (int i=0; i<32; ++i){
    String pA = folder + "/" + state + String(i) + ".bmp";
    String pB = folder + "/" + state + "_" + String(i) + ".bmp";
    if (SPIFFS.exists(pA) || SPIFFS.exists(pB)) n++;
    else break;
  }
  return n;
}

// 設定怪物與狀態（例如 monster_001 + "frame"），會自動掃幀數、置中、重置計時
inline void animSetMonster(const String& monsterId, const String& state, int scale=SCALE_DEFAULT, uint16_t chroma565=CHROMA_KEY){
  g_mon = monsterId; g_state = state; g_scale = scale; g_chroma565 = chroma565;

  String folder = "/m/" + g_mon;
  g_frameCount = scanFrames(folder, g_state);
  if (g_frameCount <= 0) { Serial.println("[ANIM] no frames, fallback 1"); g_frameCount = 1; }
  g_cur = 0; g_last = millis();

  auto &t = getTFT();
  t.fillScreen(CLR_BG);

  // 先用預設尺寸，第一幀載入後會更新
  g_w = 64 * g_scale; g_h = 64 * g_scale;
  g_x = (t.width()  - g_w) / 2;
  g_y = (t.height() - g_h) / 2+20;
  g_prevX = -10000; g_prevY = -10000;
  g_vx = STEP_X;
}

// 反閃爍：先畫「新幀」，再用一張「空白畫布（背景色）」覆蓋舊位置
inline void animLoopStep(){
  uint32_t now = millis();
  if (now - g_last < FRAME_MS) return;
  g_last = now;

  auto &t = getTFT();

  // 先算下一幀位置（含左右反彈）
  int nextX = g_x + g_vx;
  int nextY = g_y;
  if (nextX <= 0) { nextX = 0; g_vx = +STEP_X; }
  if (nextX + g_w >= t.width()) { nextX = t.width() - g_w; g_vx = -STEP_X; }
  bool mirror = (g_vx > 0);

  // 垂直起伏
  static float ph = 0; ph += BOB_W;
  int bob = (int)(sinf(ph) * BOB_PIX);

  // 聯集矩形（舊位置 + 新位置）
  int ox = (g_prevX > -9999) ? g_prevX : g_x;
  int oy = (g_prevY > -9999) ? g_prevY : g_y;
  int ux = min(ox, nextX);
  int uy = min(oy, nextY);
  int uw = max(ox + g_w, nextX + g_w) - ux;
  int uh = max(oy + g_h, nextY + g_h) - uy;
  // 邊界裁切
  if (ux < 0) ux = 0;
  if (uy < 0) uy = 0;
  if (ux + uw > t.width())  uw = t.width()  - ux;
  if (uy + uh > t.height()) uh = t.height() - uy;
  if (uw <= 0 || uh <= 0) return;

  // 組一塊「聯集矩形」的離屏緩衝
  static uint16_t* buf = nullptr; static int bw = 0, bh = 0;
  if (bw != uw || bh != uh){
    if (buf) free(buf);
    buf = (uint16_t*)malloc(uw * uh * 2);
    bw = uw; bh = uh;
  }
  if (!buf) return;

  // 先鋪背景
  for (int i=0; i<uw*uh; ++i) buf[i] = CLR_BG;

  // 準備當前幀路徑（支援 frame0.bmp / frame_0.bmp）
  String base = "/m/" + g_mon + "/";
  String pA   = base + g_state + String(g_cur) + ".bmp";
  String pB   = base + g_state + "_" + String(g_cur) + ".bmp";
  String path = SPIFFS.exists(pA) ? pA : pB;

  // 把「新幀」合成到聯集緩衝的對應位置
  int outW=0, outH=0;
  compositeFrameToCanvas(path.c_str(),
                         buf, uw, uh,
                         nextX - ux, nextY - uy + bob,
                         g_scale, g_chroma565, mirror,
                         outW, outH);
  if (outW>0 && outH>0) { g_w = outW; g_h = outH; }

  // 一次性丟上螢幕 → 沒有中間清空/覆蓋的「閃一下」
  t.drawRGBBitmap(ux, uy, buf, uw, uh);

  // 更新座標與上一幀位置
  g_prevX = g_x = nextX;
  g_prevY = g_y = nextY;

  // 下一幀
  g_cur = (g_cur + 1) % g_frameCount;
}

