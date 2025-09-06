#pragma once
#include <Arduino.h>

// Start WiFi (STA then fallback to AP) and HTTP server
void adminBegin();

// Pump HTTP server
void adminLoop();

// For event hook
void backendOnPomodoroCompleted();

