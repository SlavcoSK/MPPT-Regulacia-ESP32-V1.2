// ============================================
// GLOBÁLNE PREMENNÉ A DEKLARÁCIE
// ============================================

#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Wire.h>

// ============================================
// ENUMERÁCIE PRE STAVY SYSTÉMU
// ============================================

// Typy batérií podporované systémom
enum BatteryType {
  BATTERY_AGM = 0,      // AGM batéria (štandard pre testovanie)
  BATTERY_LIFEPO4 = 1,  // LiFePO4 batéria
  BATTERY_LEAD_ACID = 2,// Kyslá olovená batéria
  BATTERY_CUSTOM = 3    // Vlastné nastavenia
};

// Fázy nabíjania batérie
enum ChargePhase {
  PHASE_OFF,           // Vypnuté - žiadne nabíjanie
  PHASE_BULK,          // Hromadné nabíjanie (konštantný prúd)
  PHASE_ABSORPTION,    // Absorpčné nabíjanie (konštantné napätie)
  PHASE_FLOAT,         // Plávajúce nabíjanie (udržiavacie)
  PHASE_EQUALIZATION,  // Vyrovnávacie nabíjanie
  PHASE_MAINTENANCE    // Udržiavací režim
};

// Stav MPPT algoritmu
enum MPPTState {
  MPPT_SCANNING,       // Prehľadávanie maximálneho bodu
  MPPT_TRACKING,       // Sledovanie maximálneho bodu
  MPPT_HOLDING,        // Držanie v maximálnom bode
  MPPT_LOW_POWER       // Nízky výkon - optimalizácia pre slabé svetlo
};

// Celkový stav systému
enum SystemState {
  STATE_INIT,          // Inicializácia systémových komponentov
  STATE_SELFTEST,      // Samodiagnostika a testy
  STATE_IDLE,          // Nečinnosť - čakanie na podmienky
  STATE_CHARGING,      // Aktívne nabíjanie batérie
  STATE_FAULT,         // Chybový stav - niečo nie je v poriadku
  STATE_SLEEP,         // Spánkový režim pre šetrenie energie
  STATE_CONFIG         // Konfiguračný režim
};

// Stav menu na displeji
enum MenuState {
  MENU_MAIN,           // Hlavné menu
  MENU_STATUS,         // Stavová obrazovka
  MENU_CHARGING,       // Detaily nabíjania
  MENU_STATS,          // Štatistické údaje
  MENU_SETTINGS,       // Nastavenia systému
  MENU_BATTERY_SETUP,  // Nastavenie parametrov batérie
  MENU_SYSTEM_INFO,    // Informácie o systéme
  MENU_DIAGNOSTICS,    // Diagnostické nástroje
  MENU_CALIBRATION     // Kalibrácia senzorov
};

// Stav WiFi pripojenia
enum WiFiState {
  WIFI_DISCONNECTED,   // Bezdrôtové pripojenie neaktívne
  WIFI_CONNECTING,     // Pokus o pripojenie k sieti
  WIFI_CONNECTED,      // Úspešne pripojené k WiFi
  WIFI_ERROR           // Chyba pri pripájaní
};

// Úroveň ochrany systému
enum ProtectionState {
  PROTECTION_NORMAL,   // Normálny režim - všetky parametre OK
  PROTECTION_WARNING,  // Varovanie - niektoré parametre na hranici
  PROTECTION_FAULT,    // Chyba - nabíjanie zastavené
  PROTECTION_EMERGENCY // Kritická chyba - okamžité vypnutie
};

// ============================================
// ŠTRUKTÚRY PRE ÚDAJE
// ============================================

// Profil batérie - nastavenia pre konkrétny typ
struct BatteryProfile {
  float bulkVoltage;          // Napätie pre hromadné nabíjanie (V)
  float absorptionVoltage;    // Absorpčné napätie (V)
  float floatVoltage;         // Plávajúce napätie (V)
  float equalizationVoltage;  // Vyrovnávacie napätie (V) - ak je potrebné
  float minVoltage;           // Minimálne povolené napätie (V)
  float maxChargeCurrent;     // Maximálny nabíjací prúd (A)
  bool useEqualization;       // Použiť vyrovnávacie nabíjanie
  int absorptionTime;         // Dĺžka absorpčnej fázy (minúty)
  String batteryName;         // Názov typu batérie
};

// Záznam telemetrických údajov
struct TelemetryData {
  unsigned long timestamp;    // Časová značka merania (ms)
  float panelVoltage;         // Napätie solárneho panelu (V)
  float panelCurrent;         // Prúd z panelu (A)
  float panelPower;           // Výkon panelu (W)
  float batteryVoltage;       // Napätie batérie (V)
  float batteryCurrent;       // Prúd do batérie (A)
  float batteryPower;         // Výkon do batérie (W)
  float efficiency;           // Účinnosť MPPT (%)
  float temperature;          // Teplota (°C)
  float pwmDuty;              // Střída PWM (%)
  ChargePhase chargePhase;    // Aktuálna fáza nabíjania
  SystemState systemState;    // Stav systému
  byte errorFlags;            // Chybové vlajky (bitové pole)
};

// Denné štatistiky výkonu
struct DailyStats {
  unsigned long date;         // Dátum vo formáte RRRRMMDD
  float maxPower;             // Maximálny dosiahnutý výkon (W)
  float totalEnergy;          // Celková nahromadená energia (Wh)
  float avgEfficiency;        // Priemerná účinnosť (%)
  int chargeCycles;           // Počet nabíjacích cyklov
  float minBatteryVoltage;    // Minimálne napätie batérie počas dňa (V)
  float maxBatteryVoltage;    // Maximálne napätie batérie počas dňa (V)
};

// ============================================
// DEKLARÁCIE GLOBÁLNYCH PREMENNÝCH
// ============================================

// Senzorické údaje - merané hodnoty
extern float panelVoltage;
extern float panelCurrent;
extern float panelPower;
extern float batteryVoltage;
extern float batteryCurrent;
extern float batteryPower;
extern float efficiency;
extern float temperature;

// Stavové indikátory - vlajky
extern bool overVoltageFlag;
extern bool underVoltageFlag;
extern bool overTempFlag;
extern bool shortCircuitFlag;
extern bool lowPowerMode;
extern bool faultCleared;

// MPPT algoritmus - premenné
extern float pwmDutyCycle;
extern float mpptStepSize;
extern MPPTState mpptState;

// Batéria a nabíjanie - profily
extern BatteryProfile batteryProfiles[];
extern BatteryProfile* currentProfile;
extern BatteryType selectedBatteryType;
extern ChargePhase currentChargePhase;

// Systémový stav - riadenie
extern SystemState currentSystemState;
extern SystemState previousSystemState;

// Displejové menu - ovládanie
extern MenuState currentMenu;
extern MenuState previousMenu;
extern int menuCursor;
extern int menuScroll;

// Bezdrôtová komunikácia - sieť
extern WiFiState wifiState;

// Ochranný systém - bezpečnosť
extern ProtectionState currentProtection;

// Časovače a časové značky
extern unsigned long lastMPPTUpdate;
extern unsigned long stateEntryTime;
extern unsigned long lastActivityTime;
extern unsigned long absorptionStartTime;
extern unsigned long floatStartTime;

// Telemetria a histórie - ukladanie
extern TelemetryData telemetryHistory[];
extern int historyIndex;
extern int historyCount;
extern DailyStats dailyStats;

#endif // GLOBALS_H