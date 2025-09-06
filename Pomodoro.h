// ===== Pomodoro.h =====
#pragma once
#include <Arduino.h>
#include "Display.h" // 引用螢幕相關功能 getTFT()

// 番茄鐘狀態
enum PomodoroState {
  STATE_IDLE,    // 閒置
  STATE_WORK,    // 工作中
  STATE_REST     // 休息中
};

// 全域變數
static PomodoroState pomodoroCurrentState = STATE_IDLE;
static uint32_t pomodoroWorkMinutes  = 25; // 預設工作25分鐘
static uint32_t pomodoroRestMinutes  = 5;  // 預設休息5分鐘
static uint32_t pomodoroEndTime = 0;      // 當前階段的結束時間戳 (ms)

// 在螢幕上繪製番茄鐘介面
void pomodoroDrawDisplay() {
  auto& tft = getTFT();
  
  // 計算剩餘時間
  long remainingSeconds = 0;
  if (pomodoroCurrentState != STATE_IDLE) {
    long now = millis();
    if (now < pomodoroEndTime) {
      remainingSeconds = (pomodoroEndTime - now) / 1000;
    }
  }

  int minutes = remainingSeconds / 60;
  int seconds = remainingSeconds % 60;

  // 準備要顯示的文字
  String statusText;
  switch(pomodoroCurrentState) {
    case STATE_WORK:
      statusText = "";
      break;
    case STATE_REST:
      statusText = "";
      break;
    default:
      statusText = "";
      break;
  }
  
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", minutes, seconds);

  // 清除舊畫面並繪製新畫面
  tft.fillScreen(CLR_BG);
  tft.setTextSize(3);
  tft.setCursor(tft.width() / 2 - 50, 50);
  tft.print(statusText);

  tft.setTextSize(5);
  tft.setCursor(tft.width() / 2 - 80, 120);
  tft.print(timeStr);
  
  tft.setTextSize(2);
  tft.setCursor(10, tft.height() - 30);
  tft.print("Set Timer on WiFi");
}


// 番茄鐘主循環
void pomodoroLoopStep() {
  static uint32_t lastDisplayUpdate = 0;
  uint32_t now = millis();

  // 如果在計時中，檢查是否時間已到
  if (pomodoroCurrentState != STATE_IDLE && now >= pomodoroEndTime) {
    if (pomodoroCurrentState == STATE_WORK) {
      // 工作結束，開始休息
      pomodoroCurrentState = STATE_REST;
      pomodoroEndTime = now + pomodoroRestMinutes * 60 * 1000;
      Serial.println("[Pomodoro] Work finished. Starting rest.");
    } else {
      // 休息結束，回到閒置
      pomodoroCurrentState = STATE_IDLE;
      Serial.println("[Pomodoro] Rest finished. Back to idle.");
    }
    pomodoroDrawDisplay(); // 狀態改變，立即重繪
    lastDisplayUpdate = now;
  }

  // 每秒更新一次螢幕即可
  if (now - lastDisplayUpdate > 1000) {
    pomodoroDrawDisplay();
    lastDisplayUpdate = now;
  }
}

// 啟動工作計時
void startPomodoroWork() {
  if (pomodoroWorkMinutes > 0) {
    pomodoroCurrentState = STATE_WORK;
    pomodoroEndTime = millis() + pomodoroWorkMinutes * 60 * 1000;
    Serial.printf("[Pomodoro] Starting work for %d minutes.\n", pomodoroWorkMinutes);
    pomodoroDrawDisplay();
  }
}

// 停止並重置
void stopPomodoro() {
  pomodoroCurrentState = STATE_IDLE;
  pomodoroEndTime = 0;
  Serial.println("[Pomodoro] Timer stopped and reset.");
  pomodoroDrawDisplay();
}