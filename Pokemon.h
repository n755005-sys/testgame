#pragma once
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <FS.h>

// ===== 怪獸數據結構 =====
struct Pokemon {
  String id;           // monster_001
  String name;         // 皮卡丘
  
  // 當前能力值
  int hp, maxHp;       
  int attack;          
  int defense;         
  int speed;           
  int special;         // 特殊攻擊
  
  // 能力值上限（每隻怪獸不同）
  int maxAttack;       
  int maxDefense;      
  int maxSpeed;        
  int maxSpecial;      
  
  // 心情與狀態
  int happiness;       // 心情度 0-100
  int hunger;          // 飽食度 0-100
  int energy;          // 精力 0-100
  
  String type1, type2; // 屬性（火、水、草等）
  
  // 時間相關
  unsigned long lastFed;     // 上次餵食時間
  unsigned long lastPlayed;  // 上次遊戲時間
  unsigned long lastSaved;   // 上次存檔時間
  
  // 建構函數
  Pokemon() {
    id = "";
    name = "";
    hp = maxHp = 100;
    attack = defense = speed = special = 50;
    maxAttack = maxDefense = maxSpeed = maxSpecial = 100;
    happiness = hunger = energy = 100;
    type1 = "Normal";
    type2 = "";
    lastFed = lastPlayed = lastSaved = millis();
  }
};

// ===== 怪獸管理類 =====
class PokemonManager {
private:
  static const int MAX_POKEMON = 6;  // 最多6隻怪獸
  Pokemon pokemons[MAX_POKEMON];
  int pokemonCount;
  int activePokemon;  // 當前選中的怪獸索引
  
  // 自動存檔間隔（毫秒）
  static const unsigned long AUTO_SAVE_INTERVAL = 30000; // 30秒
  unsigned long lastAutoSave;
  
  // 時間衰減間隔
  static const unsigned long TIME_DECAY_INTERVAL = 60000; // 1分鐘
  unsigned long lastTimeDecay;
  
public:
  PokemonManager() : pokemonCount(0), activePokemon(-1) {
    lastAutoSave = lastTimeDecay = millis();
  }
  void autoSave(){
  for(int i=0;i<pokemonCount;i++){
    savePokemon(i);   // 逐隻存檔
  }
}

// 給 UI 用的基本動作（先做簡單數值變動即可）
  void feedPokemon(){
    if (activePokemon < 0) return;
    Pokemon &p = pokemons[activePokemon];
    p.hunger    = min(100, p.hunger + 20);
    p.happiness = min(100, p.happiness + 5);
    p.lastFed   = millis();
    savePokemon(activePokemon);
  }

  void playWithPokemon(){
    if (activePokemon < 0) return;
    Pokemon &p = pokemons[activePokemon];
    p.happiness = min(100, p.happiness + 10);
    p.energy    = max(0,   p.energy - 5);
    p.lastPlayed= millis();
    savePokemon(activePokemon);
  }

  // 新增：完成番茄鐘工作後的獎勵
  void earnReward(int happinessGain, int energyGain) {
    if (activePokemon == -1) return;
    Pokemon& poke = pokemons[activePokemon];
    poke.happiness = min(100, poke.happiness + happinessGain);
    poke.energy    = min(100, poke.energy + energyGain);
    Serial.printf("[PokemonMgr] %s earned reward. Happiness: %d, Energy: %d\n", poke.name.c_str(), poke.happiness, poke.energy);
    // 立即存檔，確保獎勵不會丟失
    savePokemon(activePokemon);
  }
  
  // 其餘程式碼保持不變...
  bool begin() {
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    loadAllPokemon();
    return true;
  }
  void addPokemon(const String& monsterId, const String& name) { /*... */ }
  Pokemon* getActivePokemon() { return (activePokemon != -1) ? &pokemons[activePokemon] : nullptr; }
  bool savePokemon(int index) { /*... */ }
  void loadPokemon(int index) { /*... */ }
  void loadAllPokemon() { /*... */ }
  void deletePokemon(int index) { /*... */ }
  bool checkFileExists(const String& filename) { /*... */ }
  void timeDecay() { /*... */ }
  void update() {
    // 時間衰減
    unsigned long now = millis();
    if (now - lastTimeDecay > TIME_DECAY_INTERVAL) {
        timeDecay();
        lastTimeDecay = now;
    }
    // 自動存檔
    if (now - lastAutoSave > AUTO_SAVE_INTERVAL) {
      if (pokemonCount > 0) autoSave();
      lastAutoSave = now;
    }
  }
};
extern PokemonManager pokemonMgr; // 讓其他檔案可以引用