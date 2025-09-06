#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

struct GameState {
  uint32_t points = 0;
  uint32_t training_count = 0;
  uint32_t pomo_completed = 0;

  String token = "123456";     // simple admin token
  String sta_ssid = "";        // optional station mode creds
  String sta_pass = "";
};

// Access the singleton state in memory
GameState &gs();

// Load from SPIFFS (creates default if missing)
bool loadGameState();

// Save to SPIFFS
bool saveGameState();

// Helpers
void addPoints(uint32_t p);
void addTraining(uint32_t c);
void incPomodoro();

// Serialize state to JSON string
String stateToJson();
