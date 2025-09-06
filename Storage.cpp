#include "Storage.h"
#include "Pokemon.h"

static const char* kStatePath = "/game_state.json";
static GameState g_state;

GameState &gs(){ return g_state; }

static bool ensureSPIFFS(){
  if (!SPIFFS.begin(true)){
    Serial.println("[Storage] SPIFFS mount failed");
    return false;
  }
  return true;
}

bool loadGameState(){
  if (!ensureSPIFFS()) return false;
  if (!SPIFFS.exists(kStatePath)){
    // Save defaults
    return saveGameState();
  }
  File f = SPIFFS.open(kStatePath, FILE_READ);
  if (!f){
    Serial.println("[Storage] open state for read failed");
    return false;
  }
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err){
    Serial.printf("[Storage] JSON parse error: %s\n", err.c_str());
    return false;
  }
  g_state.points         = doc["points"]        | g_state.points;
  g_state.training_count = doc["training_count"]| g_state.training_count;
  g_state.pomo_completed = doc["pomo_completed"]| g_state.pomo_completed;
  g_state.token          = String((const char*)(doc["token"] | g_state.token.c_str()));
  g_state.sta_ssid       = String((const char*)(doc["sta_ssid"] | g_state.sta_ssid.c_str()));
  g_state.sta_pass       = String((const char*)(doc["sta_pass"] | g_state.sta_pass.c_str()));
  return true;
}

bool saveGameState(){
  if (!ensureSPIFFS()) return false;
  File f = SPIFFS.open(kStatePath, FILE_WRITE);
  if (!f){
    Serial.println("[Storage] open state for write failed");
    return false;
  }
  StaticJsonDocument<256> doc;
  doc["points"]         = g_state.points;
  doc["training_count"] = g_state.training_count;
  doc["pomo_completed"] = g_state.pomo_completed;
  doc["token"]          = g_state.token;
  doc["sta_ssid"]       = g_state.sta_ssid;
  doc["sta_pass"]       = g_state.sta_pass;
  bool ok = (serializeJson(doc, f) > 0);
  f.close();
  if (!ok) Serial.println("[Storage] JSON write failed");
  return ok;
}

void addPoints(uint32_t p){
  g_state.points += p;
  // map points to a small reward for the active pokemon as feedback
  uint32_t h = p / 2; // happiness gain
  uint32_t e = max(1UL, p / 4); // energy gain (min 1)
  pokemonMgr.earnReward((int)h, (int)e);
  saveGameState();
}

void addTraining(uint32_t c){
  g_state.training_count += c;
  // lightweight effect: playing increases happiness a bit
  for(uint32_t i=0;i<c;i++) pokemonMgr.playWithPokemon();
  saveGameState();
}

void incPomodoro(){
  g_state.pomo_completed += 1;
  saveGameState();
}

String stateToJson(){
  StaticJsonDocument<256> doc;
  doc["points"]         = g_state.points;
  doc["training_count"] = g_state.training_count;
  doc["pomo_completed"] = g_state.pomo_completed;
  if (pokemonMgr.getActivePokemon()){
    doc["active_pokemon"] = pokemonMgr.getActivePokemon()->name;
  }
  String out;
  serializeJson(doc, out);
  return out;
}
