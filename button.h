#pragma once
#include <Arduino.h>

// Key codes
enum { K_NONE=0, K1=1, K2=2, K3=3, K4=4, K5=5 };

// Tunables (只保留一份在這裡)
static const int TH1 =  262;
static const int TH2 =  878;
static const int TH3 = 1915;
static const int TH4 = 2420;
static const int IDLE_MIN   = 3800;
static const int ADC_PIN    = 7;
static const int HYS        = 10;
static const int DEBOUNCE_MS= 30;
static int levelToKey[6] = {0, 1, 2, 3, 4, 5};

// State
static int lastStableLvl = 0;
static int lastReadLvl   = 0;
static unsigned long lastChangeMs = 0;

// Helpers
inline int readADCavg(uint8_t n=3){
  long s=0; for(uint8_t i=0;i<n;i++) s+=analogRead(ADC_PIN);
  return (int)(s/n);
}
inline int rawToLevel(int v){
  if(v < TH1) return 1;
  if(v < TH2) return 2;
  if(v < TH3) return 3;
  if(v < TH4) return 4;
  if(v < IDLE_MIN) return 5;
  return 0; // idle
}
inline int levelWithHys(int lvl, int prev){
  if(lvl==prev) return lvl;
  if(lvl==0 || prev==0) return lvl;
  if (abs(lvl - prev) >= 1) return lvl;
  return prev;
}
inline void buttonsBegin(int adcPin=ADC_PIN){ pinMode(adcPin, INPUT); }
inline void updateButtons(){
  int v   = readADCavg();
  int lvl = rawToLevel(v);
  lastReadLvl = levelWithHys(lvl, lastReadLvl);
}
inline int buttonJustPressed(){
  static int prev = 0;
  static unsigned long lastMs = 0;
  unsigned long now = millis();
  int cur = lastReadLvl, key = 0;
  if (cur != prev && (now - lastMs) > DEBOUNCE_MS){
    if (prev == 0 && cur > 0) key = levelToKey[cur];
    prev = cur; lastMs = now;
  }
  return key;
}
