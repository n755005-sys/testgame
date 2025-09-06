#pragma once
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPIFFS.h>
#include <FS.h>

// ==== 你的接腳 ====
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  17
#define TFT_SCK  14
#define TFT_MOSI 13
#define TFT_MISO 12   // 面板可不接 MISO，有定義也無妨

static Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// 先宣告新的常數名稱（避免跟庫內 #define 衝突）
static const uint16_t CLR_BG = ILI9341_BLACK;   // 背景
static const uint16_t CLR_FG = ILI9341_WHITE;   // 文字顏色

// 相容別名（給還在用舊名的檔案，例如 Uploader.h）
static const uint16_t BG_COLOR = CLR_BG;
static const uint16_t FG_COLOR = CLR_FG;

inline Adafruit_ILI9341& getTFT(){ return tft; }

// portraitUp=true → 直立正常（接頭在下、字不倒）；false → 另一直立方向
inline void displayInitPortrait(bool portraitUp=true){
  SPI.end(); SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
  tft.begin();
  tft.setRotation(portraitUp ? 0 : 2);
  tft.fillScreen(CLR_BG);
  tft.setTextColor(CLR_FG, CLR_BG);
  tft.setTextSize(2);
  tft.setCursor(6,6);
  tft.println("Display OK");
}
