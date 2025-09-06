
# ESP32-S3 怪獸養成機（TFT + 按鍵 + Wi‑Fi 上傳 + 番茄鐘）

> 這份 `README.md` 是根據你目前的專案檔案（`game.ino`, `Display.h`, `Anim.h`, `Pokemon.h`, `Pomodoro.h`, `Uploader.h`, `button.h`, `pixel_icons_rgb565.h`, `pomodoro_icons.h`, `png2bmp_esp32.py`）與你先前的接線/需求整理而成。

---

## 1) 硬體清單（目前使用）
- **主控板**：ESP32‑S3 DevKitC **N16R8**（16MB Flash / 8MB PSRAM）  
- **螢幕**：ILI9341 **240×320 SPI**（使用 Adafruit_ILI9341）  
- **按鍵**：5 鍵電阻梯（單一 ADC 腳讀值分壓）  
- **（選配）SD 卡**：MicroSD（SPI 模式，獨立 SPI 匯流排；目前程式尚未啟用）

> 備註：當前韌體以 **SPIFFS** 儲存動畫與資料；SD 卡為未來擴充。

---

## 2) 接線對應（Pin Mapping）

### 2.1 ILI9341 (SPI 顯示器)
依 `Display.h`：

| 功能 | 腳位（ESP32‑S3） |
| --- | --- |
| `TFT_CS`  | **GPIO 5** |
| `TFT_DC`  | **GPIO 4** |
| `TFT_RST` | **GPIO 17** |
| `TFT_SCK` | **GPIO 14** |
| `TFT_MOSI`| **GPIO 13** |
| `TFT_MISO`| **GPIO 12**（可不接，僅寫入時不需） |
| VCC / BL | 3V3（或 BL 接可 PWM 的腳位做背光調光） |
| GND | GND |

> 旋轉：`displayInitPortrait(true)` 採 **直立** 正向。

---

### 2.2 按鍵（ADC 電阻梯）
依 `game.ino` / `button.h`：

- **ADC 腳**：**GPIO 7**
- 解析度：`analogReadResolution(12)` → 0..4095
- 去抖：15~30ms
- **電壓分級（閾值）**（可視實機微調）：  
  - `TH1 = 262`  
  - `TH2 = 878`  
  - `TH3 = 1915`  
  - `TH4 = 2420`  
  - `IDLE_MIN = 3800`（> 這值以上視為未按）

**ADC 等級 → 鍵值對應（依你的實機）**：
```
Level 1 -> K3
Level 2 -> K2
Level 3 -> K1
Level 4 -> K4
Level 5 -> K5
```
> 程式內部使用 `g_levelToKey = {0, 3, 2, 1, 4, 5}` 完成對應。

---

### 2.3 （預留）MicroSD（獨立 SPI 匯流排）
> 目前程式未啟用 SD；此為你現場接法備忘。

| 功能 | 腳位（ESP32‑S3） |
| --- | --- |
| `SD_CS`   | **GPIO 15** |
| `SD_SCK`  | **GPIO 10** |
| `SD_MOSI` | **GPIO 11** |
| `SD_MISO` | **GPIO 9**  |

- **格式**：建議 **FAT / FAT32**。  
- 若未來啟用程式，建議使用 Arduino `SD` 或 `SdFat`，與 `SPIClass` 指定第二組 SPI（避免與 TFT 共用）。

---

## 3) 專案結構與主要模組

```
game.ino                 // 主流程：顯示初始化、鍵盤掃描、UI 狀態、呼叫各模組
Display.h                // Adafruit_ILI9341 封裝（腳位、旋轉、文字顏色）
Anim.h                   // 怪獸動畫：讀 SPIFFS 下 BMP，Magenta 去背(#F81F)，晃動/位移
Pokemon.h                // 怪獸屬性、JSON 存檔/讀檔、定時衰減、自動存檔與番茄鐘獎勵
Pomodoro.h               // 番茄鐘（工作/休息/閒置；預設 25/5；每秒更新顯示）
Uploader.h               // Wi‑Fi AP + Web 上傳，亦提供 /pomodoro 設定頁
button.h                 // 單一 ADC 五鍵讀值、去抖、Level->Key 對應
pixel_icons_rgb565.h     // 32×32 向量繪圖形式的圖示（含主題色、選取外框）
pomodoro_icons.h         // 32×32 的番茄鐘相關圖示（RGB565 陣列，F81F = 透明）
png2bmp_esp32.py         // 素材轉檔：PNG → 24-bit BMP（透明鋪成 #FF00FF）
```

**動畫檔案路徑（SPIFFS）**：
```
/m/<monster_id>/frame0.bmp
/m/<monster_id>/frame1.bmp
...
或：/m/<monster_id>/frame_0.bmp  /frame_1.bmp ...
```
> `Anim.h` 會自動掃描連號幀，並以 `CHROMA_KEY = 0xF81F` 作為透明色。

---

## 4) 操作方式（UI & 鍵位）

> 實作以 `ui.h`/`game.ino` 為準，以下為目前邏輯整理。

- **首頁 (UI_HOME)**  
  - `K5`：打開主選單

- **主選單 (UI_MENU)**（底部 7 個圖示：餵食、訓練、互動、戰鬥、狀態、學習、設定）  
  - `K1`：上一個圖示  
  - `K4`：下一個圖示  
  - `K5`：進入（依當前項目）  
  - `K3`：關閉選單/返回

- **狀態頁 (UI_STATUS)**  
  - 顯示攻擊/防禦/速度/特攻四列圖形化長條  
  - `K3`：返回首頁  
  - `K5`：打開主選單

- **學習/番茄鐘 (UI_STUDY)**  
  - `Pomodoro` 預設：**工作 25 分、休息 5 分**（可在 `/pomodoro` 網頁調整）  
  - **快捷鍵**（在你的 `game.ino` 註解規劃）：  
    - `K1`：開始/停止  
    - `K2`：+5 分鐘（工作時段）  
  - 每秒刷新顯示；閒置時顯示 `MM:00`；避免閃爍

- **戰鬥/一般畫面**  
  - `animLoopStep()` 只在 `UI_NORMAL` 與 `UI_BATTLE` 運作（狀態/學習頁不重畫怪獸以省資源）

---

## 5) Wi‑Fi 上傳與番茄鐘控制（網頁）

- 啟動後會自動建立 AP：  
  - **SSID**：`ESP32-UP-xxxxxx`（MAC 尾碼）  
  - **密碼**：`12345678`  
- 以手機/電腦連線後，打開瀏覽器進入首頁（顯示於 TFT 上）。  
- **檔案上傳**：可選「怪獸模式」自動放到 `/m/<id>/`，或「指定資料夾」。  
- **檔案列表 / 存在查詢 / 清空 SPIFFS** 功能已提供。  
- **番茄鐘設定頁**：`/pomodoro`  
  - 可調整工作/休息分鐘數，並提供 **Start/Stop** 按鈕。

---

## 6) 素材轉檔流程（PNG → BMP → 上傳）

1. 準備像素圖（透明背景 PNG）。  
2. 使用 `png2bmp_esp32.py` 轉成 24‑bit BMP，透明會以 **Magenta (#FF00FF)** 鋪底供裝置去背：

```bash
# 單檔
python png2bmp_esp32.py --in ./sprite.png --out ./frame0.bmp --size 64 64

# 整個資料夾（保持子資料夾結構）
python png2bmp_esp32.py --in ./pngs --out ./bmps --size 64 64
```

3. 透過裝置 AP 的網頁 `/` 選擇「怪獸模式」或指定資料夾，**上傳到 SPIFFS**：  
   - 例如：`/m/monster_001/frame0.bmp`, `/m/monster_001/frame1.bmp` ...  
4. 在 `game.ino` 以 `animSetMonster("monster_001", "frame", 2 /*scale*/)` 指定要播的怪獸。

---

## 7) 編譯與燒錄建議

- Arduino IDE → **ESP32 by Espressif** 套件（選：**ESP32S3 Dev Module**）  
- 建議設定：240MHz、USB CDC 開啟、Partition 依素材量選擇：  
  - N16R8 可用 **Huge APP + 大 SPIFFS**（或自訂分割）  
- 需要函式庫：`Adafruit_GFX`, `Adafruit_ILI9341`, `ArduinoJson`, `WiFi`, `WebServer`  
- 檔案系統：**SPIFFS**（由程式在第一次啟動時格式化/掛載；亦可於網頁一鍵清空）

---

## 8) 目前已完成的功能

- [x] ILI9341 顯示初始化（直立）與文字顯示色彩設定  
- [x] **五鍵 ADC** 輸入（分壓閾值、去抖、Level→Key 映射）  
- [x] **主選單/狀態列/HUD**（乾淨底部列、不再鋪白底造成閃爍）  
- [x] **怪獸動畫引擎**：SPIFFS BMP 幀讀取、Magenta 去背、縮放、左右鏡像、區域重繪  
- [x] **番茄鐘**（UI 與 `/pomodoro` 網頁雙控制；每秒更新；完成後可給怪獸獎勵）  
- [x] **Wi‑Fi AP + Web 上傳器**（清單/查詢/清空 SPIFFS、路徑捷徑）  
- [x] **Pokemon 管理**：JSON 檔案儲存、定時衰減、自動存檔、`earnReward()` 支援

---

## 9) 未來功能追加（Roadmap）

- [ ] **SD 卡**：啟用獨立 SPI 的 SD（FAT/FAT32），支援從 SD 播放大容量素材  
- [ ] **聲音**：I²S 擴音（如 MAX98357A），戰鬥/提示音效  
- [ ] **戰鬥系統**：回合制數值、連擊/能量、屬性相剋 UI（已在 `ui.h` 留 hooks）  
- [ ] **學習模式擴充**：完成番茄鐘一次自動觸發 `pokemonMgr.earnReward()`（量值可在設定頁調）  
- [ ] **資產管線**：加入「從 GIF 自動拆幀 → PNG → BMP → 壓縮上傳」腳本  
- [ ] **OTA 更新**：從 AP 或 STA 模式進行 OTA 升級  
- [ ] **多語與字型**：中文字型壓縮/分段載入，UI 多語切換  
- [ ] **耗電最佳化**：閒置亮度下降、深度睡眠喚醒（長按某鍵喚醒）

---

## 10) 疑難排解（FAQ）

- **螢幕有條紋/殘影**：繪圖時改用區塊 `fillRect` 寫入、避免每像素多次覆蓋；目前 `blitIcon()` 已處理。  
- **時間顯示重疊或閃爍**：番茄鐘視圖只 **每秒** 重繪一次、且背景先清除。  
- **按鍵誤觸**：微調 `TH1..TH4` 與 `IDLE_MIN`，並確認分壓電阻布局與接地。  
- **SD 無法掛載**：檢查 `CS/SCK/MOSI/MISO` 接線、頻率與卡片格式（**FAT/FAT32**）。目前程式未使用 SD，先使用 SPIFFS 與 AP 上傳。  
- **無法上傳檔案**：確認已連上裝置 AP，並使用首頁提供的表單上傳（瀏覽器勿自動重整）。

---

## 11) 版權 & 致謝
- 部分圖示與程式碼為專案內自製/客製；RGB565 與 Magenta 去背方式為常見嵌入式作法。  
- 使用到的開源函式庫版權分別歸原專案所有。

---

若需要我再把此 `README.md` 直接放進你的專案資料夾，或補上接線示意圖（PNG/SVG），跟我說一聲就加上！
