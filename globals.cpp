// ============================================
// DEFINÍCIE GLOBÁLNYCH PREMENNÝCH
// Tento súbor obsahuje SKUTOČNÉ definície premenných
// ============================================

#include "globals.h"

// ============================================
// SENZORICKÉ HODNOTY - DEFINÍCIE
// ============================================

// Napätie a prúd zo solárneho panelu
float panelVoltage = 0.0f;        // Počiatočná hodnota 0V
float panelCurrent = 0.0f;        // Počiatočná hodnota 0A
float panelPower = 0.0f;          // Počiatočný výkon 0W

// Napätie a prúd batérie
float batteryVoltage = 0.0f;      // Počiatočná hodnota 0V
float batteryCurrent = 0.0f;      // Počiatočná hodnota 0A
float batteryPower = 0.0f;        // Počiatočný výkon 0W

// Účinnosť a teplota systému
float efficiency = 0.0f;          // Počiatočná účinnosť 0%
float temperature = 25.0f;        // Predpokladaná izbová teplota 25°C

// ============================================
// STAVOVÉ INDIKÁTORY - DEFINÍCIE
// ============================================

bool overVoltageFlag = false;     // Vlajka prekročenia napätia - NEPRAVDA
bool underVoltageFlag = false;    // Vlajka podkročenia napätia - NEPRAVDA
bool overTempFlag = false;        // Vlajka prehriatia - NEPRAVDA
bool shortCircuitFlag = false;    // Vlajka zkratu - NEPRAVDA
bool lowPowerMode = false;        // Režim s nízkou spotrebou - VYPNUTÝ
bool faultCleared = false;        // Stav vyčistenia chýb - NEPRAVDA

// ============================================
// MPPT ALGORITMUS - DEFINÍCIE
// ============================================

float pwmDutyCycle = 0.0f;        // Počiatočná střída PWM 0%
float mpptStepSize = 0.01f;       // Základná veľkosť kroku MPPT 1%
MPPTState mpptState = MPPT_SCANNING; // Počiatočný stav - prehľadávanie

// ============================================
// BATÉRIA A NABÍJANIE - DEFINÍCIE
// ============================================

// Pole profilov pre rôzne typy batérií
BatteryProfile batteryProfiles[4] = {
  // AGM batéria - štandardné testovacie nastavenie
  {14.4f, 14.5f, 13.8f, 14.8f, 12.0f, 5.0f, false, 120, "AGM"},
  
  // LiFePO4 batéria - vysokovýkonná lithium
  {14.2f, 14.6f, 13.8f, 14.8f, 10.0f, 20.0f, false, 60, "LiFePO4"},
  
  // Kyslá olovená batéria - tradičná technológia
  {14.4f, 14.7f, 13.5f, 15.5f, 11.5f, 10.0f, true, 180, "OLOVO"},
  
  // Vlastný profil - pre špeciálne aplikácie
  {14.0f, 14.0f, 13.5f, 14.5f, 12.0f, 5.0f, false, 120, "CUSTOM"}
};

BatteryProfile* currentProfile = &batteryProfiles[0]; // Štandardne AGM
BatteryType selectedBatteryType = BATTERY_AGM;        // Vybraný typ AGM
ChargePhase currentChargePhase = PHASE_OFF;           // Počiatočný stav - VYPNUTÉ

// ============================================
// SYSTÉMOVÝ STAV - DEFINÍCIE
// ============================================

SystemState currentSystemState = STATE_INIT;      // Začíname s inicializáciou
SystemState previousSystemState = STATE_INIT;     // Predchádzajúci stav tiež inicializácia

// ============================================
// DISPLEJOVÉ MENU - DEFINÍCIE
// ============================================

MenuState currentMenu = MENU_STATUS;              // Začíname so stavovou obrazovkou
MenuState previousMenu = MENU_STATUS;             // Predchádzajúce menu tiež stav
int menuCursor = 0;                               // Kurzor na prvej polohe
int menuScroll = 0;                               // Bez posunu v zozname

// ============================================
// BEZDRÔTOVÁ KOMUNIKÁCIA - DEFINÍCIE
// ============================================

WiFiState wifiState = WIFI_DISCONNECTED;          // Na začiatku nie sme pripojení

// ============================================
// OCHRANNÝ SYSTÉM - DEFINÍCIE
// ============================================

ProtectionState currentProtection = PROTECTION_NORMAL; // Normálny režim bez chýb

// ============================================
// ČASOVAČE A ČASOVÉ ZNAČKY - DEFINÍCIE
// ============================================

unsigned long lastMPPTUpdate = 0;                 // Čas poslednej aktualizácie MPPT
unsigned long stateEntryTime = 0;                 // Čas vstupu do aktuálneho stavu
unsigned long lastActivityTime = 0;               // Čas poslednej aktivity
unsigned long absorptionStartTime = 0;            // Začiatok absorpčnej fázy
unsigned long floatStartTime = 0;                 // Začiatok plávajúcej fázy

// ============================================
// TELEMETRIA A HISTÓRIE - DEFINÍCIE
// ============================================

TelemetryData telemetryHistory[1000]; // Kruhový buffer pre dáta (1000 záznamov)
int historyIndex = 0;      // Aktuálny index v histórii
int historyCount = 0;      // Počet uložených záznamov

// Počiatočné denné štatistiky
DailyStats dailyStats = {
  0,        // Dátum - bude nastavený pri prvom meraní
  0.0f,     // Maximálny výkon - začiatok od 0W
  0.0f,     // Celková energia - začiatok od 0Wh
  0.0f,     // Priemerná účinnosť - začiatok od 0%
  0,        // Počet nabíjacích cyklov - začiatok od 0
  100.0f,   // Minimálne napätie batérie - vysoká hodnota pre detekciu minima
  0.0f      // Maximálne napätie batérie - nízka hodnota pre detekciu maxima
};