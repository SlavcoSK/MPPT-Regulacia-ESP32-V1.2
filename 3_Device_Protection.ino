// ============================================
// OCHRANNÉ FUNKCIE A BEZPEČNOSTNÉ LIMITY - MODUL 3
// ============================================

#include "config.h"
#include "globals.h"

// ============================================
// LOKÁLNE PREMENNÉ MODULU OCHRÁN
// ============================================

// Časové značky pre oneskorené reakcie na poruchy
unsigned long overVoltageStartTime = 0;   // Začiatok prekročenia napätia
unsigned long overTempStartTime = 0;      // Začiatok prehriatia
unsigned long underVoltageStartTime = 0;  // Začiatok podkročenia napätia

// Historické hodnoty pre detekciu rýchlych zmien
float previousBatteryCurrent = 0.0f;      // Predchádzajúca hodnota prúdu batérie
unsigned long previousCurrentTime = 0;    // Čas predchádzajúceho merania

// ============================================
// ZÁKLADNÉ OCHRANNÉ KONTROLY - HLAVNÁ FUNKCIA
// ============================================

void checkBasicProtections() {
  // Táto funkcia kontroluje základné bezpečnostné limity
  // Volá sa pravidelne v hlavnej slučke programu
  
  // 1. KONTROLA PREKROČENIA NAPÄTIA BATÉRIE
  if (batteryVoltage >= BATTERY_VOLTAGE_CRITICAL_HIGH) {
    overVoltageFlag = true;  // Nastavíme vlajku prekročenia napätia
    
    // Ak je to prvé prekročenie, zaznamenáme čas
    if (overVoltageStartTime == 0) {
      overVoltageStartTime = millis();
      Serial.println("VAROVANIE: Batéria dosiahla kritické napätie!");
    }
    
    // Okamžitá reakcia pri veľmi vysokom napätí
    if (batteryVoltage >= BATTERY_VOLTAGE_CRITICAL_HIGH + 1.0f) {
      emergencyShutdown("KRITICKÉ PREKROČENIE NAPÄTIA BATÉRIE");
      return;  // Ukončíme funkciu - systém je vypnutý
    }
  } else {
    // Ak napätie kleslo pod kritickú hranicu, resetujeme časovač
    overVoltageStartTime = 0;
    overVoltageFlag = false;
  }
  
  // 2. KONTROLA PRÍLIŠ NÍZKEHO NAPÄTIA BATÉRIE
  if (batteryVoltage <= BATTERY_VOLTAGE_CRITICAL_LOW) {
    underVoltageFlag = true;  // Nastavíme vlajku podkročenia napätia
    
    // Ak je to prvé podkročenie, zaznamenáme čas
    if (underVoltageStartTime == 0) {
      underVoltageStartTime = millis();
      Serial.println("VAROVANIE: Batéria má príliš nízke napätie!");
    }
  } else {
    // Ak napätie stúplo nad kritickú hranicu, resetujeme časovač
    underVoltageStartTime = 0;
    underVoltageFlag = false;
  }
  
  // 3. KONTROLA PRETOČENIA TEPLOTY
  if (temperature >= TEMPERATURE_MAX) {
    overTempFlag = true;  // Nastavíme vlajku prehriatia
    
    // Ak je to prvé prehriatie, zaznamenáme čas
    if (overTempStartTime == 0) {
      overTempStartTime = millis();
      Serial.println("VAROVANIE: Systém dosiahol maximálnu teplotu!");
    }
  } else {
    // Ak teplota klesla pod maximálnu hranicu, resetujeme časovač
    overTempStartTime = 0;
    overTempFlag = false;
  }
  
  // 4. KONTROLA MAXIMÁLNEHO NABÍJACIEHO PRÚDU
  if (batteryCurrent > MAX_CHARGE_CURRENT) {
    Serial.println("VAROVANIE: Prekročený maximálny nabíjací prúd!");
    reduceChargeCurrent();  // Voláme funkciu na obmedzenie prúdu
  }
  
  // 5. KONTROLA NAPÄTIA SOLÁRNEHO PANELU
  if (panelVoltage > PANEL_VOLTAGE_MAX) {
    Serial.println("VAROVANIE: Panelové napätie príliš vysoké!");
    disableChargingTemporarily(5000);  // 5 sekúnd pauza
  }
}

// ============================================
// POKROČILÉ OCHRANY (di/dt, ZKRAT) - FUNKCIA
// ============================================

bool checkAdvancedProtections() {
  // Táto funkcia kontroluje pokročilé ochrany ako zkrat a reverzný prúd
  
  unsigned long currentTime = millis();
  float timeDiff = (currentTime - previousCurrentTime) / 1000.0f;  // v sekundách
  
  // Potrebujeme aspoň jedno predchádzajúce meranie
  if (timeDiff > 0 && previousCurrentTime != 0) {
    // VÝPOČET RÝCHLOSTI ZMENY PRÚDU (di/dt)
    float currentRiseRate = abs(batteryCurrent - previousBatteryCurrent) / timeDiff;
    
    // 1. DETEKCIA ZKRATU PODĽA RÝCHLOSTI NÁBEHU PRÚDU
    if (currentRiseRate > MAX_CURRENT_RISE_RATE) {
      Serial.println("CHYBA: Zkrat detekovaný - príliš rýchly nábeh prúdu!");
      emergencyShutdown("ZKART PRÚDOVÝ - RÝCHLY NÁBEH");
      return false;  // Funkcia sa už nevykoná ďalej
    }
    
    // 2. DETEKCIA REVERZNÉHO PRÚDU (panel vybíja batériu)
    if (batteryCurrent < -0.5f && panelVoltage < batteryVoltage) {
      Serial.println("VAROVANIE: Detekovaný reverzný prúd - panel vybíja batériu!");
      disableCharging();  // Okamžité zastavenie nabíjania
      return false;
    }
  }
  
  // ULOŽENIE AKTUÁLNYCH HODNÔT PRE ĎALŠIU ITERÁCIU
  previousBatteryCurrent = batteryCurrent;
  previousCurrentTime = currentTime;
  
  return true;  // Všetky kontroly prešli úspešne
}

// ============================================
// TEČUOVÁ OCHRANA BATÉRIE - FUNKCIA
// ============================================

void checkBatteryTemperatureProtection() {
  // Táto funkcia chráni batériu pred teplotnými extrémami
  
  // 1. KONTROLA ČI JE BATÉRIA PRÍLIŠ STUDENÁ NA NABÍJANIE
  if (temperature < BATTERY_TEMP_MIN) {
    Serial.println("VAROVANIE: Batéria je príliš studená na nabíjanie!");
    setChargeCurrentLimit(0.1f);  // Nastavíme veľmi malý prúd
    return;
  }
  
  // 2. KONTROLA ČI JE BATÉRIA PRÍLIŠ HORÚCA
  if (temperature > BATTERY_TEMP_MAX) {
    Serial.println("VAROVANIE: Batéria je príliš horúca!");
    
    // Postupné redukovanie prúdu so stúpajúcou teplotou
    float reductionFactor = 1.0f - ((temperature - BATTERY_TEMP_MAX) / 20.0f);
    reductionFactor = constrain(reductionFactor, 0.0f, 1.0f);
    
    float newLimit = MAX_CHARGE_CURRENT * reductionFactor;
    setChargeCurrentLimit(newLimit);
    
    Serial.print("Redukovaný prúd na: ");
    Serial.print(newLimit, 1);
    Serial.println(" A");
  }
}

// ============================================
// OCHRANA PROTI PREPLATENIU - FUNKCIA
// ============================================

void checkOverchargeProtection() {
  // Táto funkcia zabraňuje preplateniu batérie
  
  static bool absorptionPhase = false;           // Indikátor absorpčnej fázy
  static unsigned long absorptionStartTime = 0;  // Začiatok absorpčnej fázy
  
  // AK BATÉRIA DOSIAHLA ABSORPČNÉ NAPÄTIE
  if (batteryVoltage >= BATTERY_VOLTAGE_ABSORPTION && !absorptionPhase) {
    absorptionPhase = true;
    absorptionStartTime = millis();
    Serial.println("INFORMÁCIA: Začiatok absorpčnej fázy nabíjania");
  }
  
  // POČAS ABSORPČNEJ FAZY KONTROLUJEME PODMIENKY UKONČENIA
  if (absorptionPhase) {
    unsigned long absorptionTime = millis() - absorptionStartTime;
    
    // PODMIENKA 1: ČASOVÝ LIMIT - maximálne 2 hodiny
    if (absorptionTime > 120 * 60 * 1000UL) {  // 120 minút v milisekundách
      absorptionPhase = false;
      Serial.println("INFORMÁCIA: Ukončená absorpčná fáza (časový limit)");
      switchToFloatMode();  // Prechod do plávajúceho režimu
    }
    
    // PODMIENKA 2: PRÚDOVÝ LIMIT - keď prúd klesne pod 5% maxima
    if (batteryCurrent < MAX_CHARGE_CURRENT * 0.05f) {
      absorptionPhase = false;
      Serial.println("INFORMÁCIA: Ukončená absorpčná fáza (nízky prúd)");
      switchToFloatMode();  // Prechod do plávajúceho režimu
    }
  }
}

// ============================================
// RIADENIE OCHRAN PODĽA STAVU - FUNKCIA
// ============================================

ProtectionState evaluateProtectionState() {
  // Táto funkcia vyhodnocuje celkový stav ochrán
  
  // KRITICKÉ STAVY - OKAMŽITÁ AKCIA
  if (batteryVoltage >= BATTERY_VOLTAGE_CRITICAL_HIGH) {
    return PROTECTION_EMERGENCY;  // Najvyššia úroveň nebezpečenstva
  }
  
  if (batteryVoltage <= BATTERY_VOLTAGE_CRITICAL_LOW) {
    return PROTECTION_FAULT;  // Zastaviť nabíjanie
  }
  
  if (temperature >= TEMPERATURE_MAX) {
    return PROTECTION_FAULT;  // Zastaviť z dôvodu prehriatia
  }
  
  if (batteryCurrent >= CURRENT_SHORT_CIRCUIT_THRESHOLD) {
    return PROTECTION_EMERGENCY;  // Zkrat - okamžité vypnutie
  }
  
  // VAROVNÉ STAVY - REDUKCIA VÝKONU
  if (batteryVoltage >= BATTERY_VOLTAGE_ABSORPTION ||
      temperature >= TEMPERATURE_WARNING ||
      batteryCurrent >= MAX_CHARGE_CURRENT * 0.8f) {
    return PROTECTION_WARNING;  // Znížiť výkon ale pokračovať
  }
  
  // NORMÁLNY STAV - PLNÝ VÝKON POVOLENÝ
  return PROTECTION_NORMAL;
}

// ============================================
// APLIKÁCIA OCHRANNÝCH OPATRENÍ - FUNKCIA
// ============================================

void applyProtectionActions(ProtectionState state) {
  // Táto funkcia aplikuje konkrétne opatrenia podľa úrovne ochrany
  
  switch(state) {
    case PROTECTION_NORMAL:
      // PLNÝ VÝKON POVOLENÝ - žiadne obmedzenia
      setPWMlimit(100.0f);                  // 100% PWM
      setChargeCurrentLimit(MAX_CHARGE_CURRENT); // Maximálny prúd
      break;
      
    case PROTECTION_WARNING:
      // REDUKOVANÝ VÝKON - preventívne opatrenia
      setPWMlimit(70.0f);                   // 70% PWM
      setChargeCurrentLimit(MAX_CHARGE_CURRENT * 0.7f); // 70% prúdu
      Serial.println("INFORMÁCIA: Výkon redukovaný z dôvodu varovného stavu");
      break;
      
    case PROTECTION_FAULT:
      // ZASTAVENIE NABÍJANIA - niečo nie je v poriadku
      disableCharging();  // Úplne zastaviť
      Serial.println("VAROVANIE: Nabíjanie zastavené z dôvodu chybového stavu");
      break;
      
    case PROTECTION_EMERGENCY:
      // ÚPLNÉ VYPNUTIE SYSTÉMU - kritická situácia
      emergencyShutdown("EMERGENCY STOP - KRITICKÝ STAV");
      break;
  }
}

// ============================================
// EMERGENCY SHUTDOWN PROCEDÚRA - FUNKCIA
// ============================================

void emergencyShutdown(const char* reason) {
  // Táto funkcia vykoná úplné núdzové vypnutie systému
  
  Serial.print("NÚDZOVÉ VYPNUTIE: ");
  Serial.println(reason);
  
  // KROK 1: Okamžité zastavenie PWM výstupu
  ledcWrite(PWM_CHANNEL, 0);
  
  // KROK 2: Aktivácia hardvérovej výpojky (ak existuje)
  digitalWrite(SHUTDOWN_PIN, LOW);
  
  // KROK 3: Uloženie dôvodu vypnutia do EEPROM
  saveErrorToEEPROM(reason);
  
  // KROK 4: Blikajúca LED indikácia chyby
  while (true) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    
    // MOŽNOSŤ MANUÁLNEHO RESETU DLHÝM STLAČENÍM TLAČIDLA
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      delay(3000);  // Čakanie 3 sekundy
      if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        Serial.println("MANUÁLNY RESET - reštartujem systém...");
        ESP.restart();  // Reset ESP32 mikrokontroléra
      }
    }
  }
}

// ============================================
// LOGOVANIE UDALOSTÍ A CHÝB - FUNKCIE
// ============================================

void logEvent(const char* message) {
  // Funkcia pre logovanie normálnych udalostí
  Serial.print("[UDALOSŤ] ");
  Serial.println(message);
  addToEventLog(message);  // Uloženie do záznamu udalostí
}

void logWarning(const char* message) {
  // Funkcia pre logovanie varovaní
  Serial.print("[VAROVANIE] ");
  Serial.println(message);
  updateDisplayWarning(message);  // Aktualizácia displeja
}

void logError(const char* message) {
  // Funkcia pre logovanie chýb
  Serial.print("[CHYBA] ");
  Serial.println(message);
  saveErrorToLog(message);  // Uloženie do error logu
}

// ============================================
## **📁 6. 4_Charging_Algorithm.ino** (OPRAVENÝ)

```cpp
// ============================================
// MPPT A NABÍJACIE ALGORITMY - MODUL 4
// ============================================

#include "config.h"
#include "globals.h"

// ============================================
// LOKÁLNE PREMENNÉ MODULU NABÍJANIA
// ============================================

// Premenné pre MPPT algoritmus
float prevPower = 0.0f;       // Predchádzajúci výkon panelu
float prevDuty = 0.0f;        // Predchádzajúca střída PWM
int mpptDirection = 1;        // Smer zmeny střídy (-1 alebo 1)

// Časové premenné pre fázové riadenie
unsigned long lastPhaseChange = 0;  // Čas poslednej zmeny fázy

// ============================================
// INICIALIZÁCIA NABÍJACIEHO SYSTÉMU - FUNKCIA
// ============================================

void initChargingSystem() {
  Serial.println("Inicializácia nabíjacieho systému...");
  
  // Nastavenie AGM batérie ako štandard pre testovanie
  setBatteryType(BATTERY_AGM);
  
  // Resetovanie PWM střídy na 0%
  pwmDutyCycle = 0.0f;
  
  // Inicializácia časovačov
  absorptionStartTime = 0;
  lastMPPTUpdate = millis();
  
  Serial.println("Nabíjací systém úspešne inicializovaný");
  Serial.print("Typ batérie: ");
  Serial.println(currentProfile->batteryName);
  
  // Výpis parametrov batérie pre kontrolu
  Serial.println("Parametre batérie:");
  Serial.print("  Hromadné nabíjanie: ");
  Serial.print(currentProfile->bulkVoltage, 1);
  Serial.println(" V");
  
  Serial.print("  Absorpčné nabíjanie: ");
  Serial.print(currentProfile->absorptionVoltage, 1);
  Serial.println(" V");
  
  Serial.print("  Plávajúce nabíjanie: ");
  Serial.print(currentProfile->floatVoltage, 1);
  Serial.println(" V");
  
  Serial.print("  Maximálny prúd: ");
  Serial.print(currentProfile->maxChargeCurrent, 1);
  Serial.println(" A");
}

// ============================================
// NASTAVENIE TYPU BATÉRIE - FUNKCIA
// ============================================

void setBatteryType(BatteryType type) {
  // Táto funkcia mení typ batérie a jej parametre
  
  selectedBatteryType = type;
  currentProfile = &batteryProfiles[type];
  
  Serial.print("Nastavený typ batérie: ");
  Serial.println(currentProfile->batteryName);
  
  // Podrobný výpis parametrov pre kontrolu
  Serial.println("Detaily batérie:");
  Serial.print("  Hromadné V: ");
  Serial.print(currentProfile->bulkVoltage, 1);
  Serial.print(" V | Absorpčné V: ");
  Serial.print(currentProfile->absorptionVoltage, 1);
  Serial.print(" V | Float V: ");
  Serial.print(currentProfile->floatVoltage, 1);
  Serial.println(" V");
  
  Serial.print("  Min V: ");
  Serial.print(currentProfile->minVoltage, 1);
  Serial.print(" V | Max I: ");
  Serial.print(currentProfile->maxChargeCurrent, 1);
  Serial.println(" A");
  
  if (currentProfile->useEqualization) {
    Serial.print("  Vyrovnávacie V: ");
    Serial.print(currentProfile->equalizationVoltage, 1);
    Serial.println(" V");
  }
}

// ============================================
// DETEKCIA SOLÁRNEJ AKTIVITY - FUNKCIA
// ============================================

bool checkSolarActivity() {
  // Táto funkcia kontroluje, či solárny panel produkuje energiu
  
  static float prevPanelVoltage = 0.0f;  // Predchádzajúce napätie
  static int lowPowerCounter = 0;        // Počítadlo nízkého výkonu
  
  // AK JE PANELOVÉ NAPÄTIE VEĽMI NÍZKE (menej ako 5V)
  if (panelVoltage < 5.0f) {
    lowPowerCounter++;  // Zvýšime počítadlo
    
    // AK DLHŠIE AKO 10 SEKÚND ŽIADNE NAPÄTIE
    if (lowPowerCounter > 100) {  // 100 cyklov po 100ms = 10 sekúnd
      // AKTIVÁCIA ŠETRNÉHO REŽIMU
      if (!lowPowerMode) {
        enterLowPowerMode();
      }
      return false;  // Nie je dostatok slnečného svetla
    }
  } else {
    // AK NAPÄTIE JE DOSTATOČNÉ, RESETUJEME POČÍTADLO
    lowPowerCounter = 0;
    
    // AK BOL AKTÍVNY ŠETRNÝ REŽIM, OPUSTIŤ HO
    if (lowPowerMode) {
      exitLowPowerMode();
    }
  }
  
  prevPanelVoltage = panelVoltage;
  return (panelVoltage > 5.0f);  // Vráti TRUE ak je napätie > 5V
}

// ============================================
// ŠETRNÝ REŽIM PRE NOC A SLABÉ SVETLO - FUNKCIE
// ============================================

void enterLowPowerMode() {
  // Táto funkcia prepne systém do režimu s nízkou spotrebou
  
  Serial.println("Aktivácia šetrného režimu...");
  lowPowerMode = true;
  
  // KROK 1: Zastavenie PWM výstupu
  setPWM(0.0f);
  
  // KROK 2: Zníženie frekvencie meraní (šetríme energiu)
  setMeasurementInterval(5000);  // Meranie iba každých 5 sekúnd
  
  // KROK 3: Zníženie jasu displeja na minimum
  setDisplayBrightness(10);  // 10% z maximálneho jasu
  
  // KROK 4: Vypnutie nepotrebných periférií
  disableNonEssentialPeripherals();
  
  // KROK 5: Odpojenie od batérie (ak hardvér umožňuje)
  disconnectFromBattery();
  
  Serial.println("Šetrný režim úspešne aktivovaný");
}

void exitLowPowerMode() {
  // Táto funkcia ukončí šetrný režim a obnoví normálnu činnosť
  
  Serial.println("Ukončenie šetrného režimu...");
  lowPowerMode = false;
  
  // KROK 1: Obnovenie normálnej frekvencie meraní
  setMeasurementInterval(100);  // Normálnych 100ms
  
  // KROK 2: Obnovenie plného jasu displeja
  setDisplayBrightness(100);  // 100% jas
  
  // KROK 3: Zapnutie všetkých periférií
  enablePeripherals();
  
  // KROK 4: Znovupripojenie k batérii
  connectToBattery();
  
  Serial.println("Šetrný režim deaktivovaný, normálna činnosť obnovená");
}

// ============================================
// MPPT ALGORITMUS (Perturb & Observe) - FUNKCIA
// ============================================

void runMPPTAlgorithm() {
  // Táto funkcia implementuje MPPT algoritmus Perturb & Observe
  
  // IBA AK MÁME DOSTATOČNÉ PANELOVÉ NAPÄTIE
  if (panelVoltage < batteryVoltage + 1.0f) {
    mpptState = MPPT_SCANNING;  // Prepnúť na prehľadávanie
    return;
  }
  
  // IBA VO FÁZE HROMADNÉHO NABÍJANIA POUŽÍVAME MPPT
  if (currentChargePhase != PHASE_BULK) {
    return;
  }
  
  // IBA KAŽDÝCH 100ms VYKONÁVAME MPPT AKTUALIZÁCIU
  unsigned long currentTime = millis();
  if (currentTime - lastMPPTUpdate < 100) {
    return;
  }
  
  lastMPPTUpdate = currentTime;
  
  // VÝPOČET AKTUÁLNEHO VÝKONU PANELU
  float currentPower = panelVoltage * panelCurrent;
  
  // ADAPTÍVNY KROK MPPT PODĽA PODMIENOK
  updateMPPTStepSize(currentPower);
  
  // PERTURB AND OBSERVE ALGORITMUS
  float deltaPower = currentPower - prevPower;  // Zmena výkonu
  float deltaDuty = pwmDutyCycle - prevDuty;    // Zmena střídy
  
  // LOGIKA PERTURB & OBSERVE
  if (deltaPower > 0) {
    // VÝKON STÚPOL - POKRAČOVAŤ V ROVNAKOM SMERE
    mpptDirection = (deltaDuty > 0) ? 1 : -1;
  } else {
    // VÝKON KLESOL - ZMENIŤ SMER
    mpptDirection = (deltaDuty > 0) ? -1 : 1;
  }
  
  // APLIKOVANIE ZMENY STŘÍDY
  pwmDutyCycle += mpptDirection * mpptStepSize;
  
  // OBMEDZENIE ROZSAHU STŘÍDY NA 0-95% (5% rezerva pre bezpečnosť)
  pwmDutyCycle = constrain(pwmDutyCycle, 0.0f, 95.0f);
  
  // ULOŽENIE HODNÔT PRE ĎALŠIU ITERÁCIU
  prevPower = currentPower;
  prevDuty = pwmDutyCycle;
  
  // AKTUALIZÁCIA STAVU MPPT
  if (abs(deltaPower) < 0.1f) {  // Malá zmena výkonu
    mpptState = MPPT_HOLDING;    // Držíme sa v optimálnom bode
  } else {
    mpptState = MPPT_TRACKING;   // Sledujeme maximum
  }
  
  // APLIKOVANIE NOVEJ PWM STŘÍDY
  applyPWM(pwmDutyCycle);
}

// ============================================
## **📁 7. 5_System_Processes.ino** (OPRAVENÝ)

```cpp
// ============================================
// SYSTÉMOVÉ PROCESY A STAVOVÝ AUTOMAT - MODUL 5
// ============================================

#include "config.h"
#include "globals.h"

// ============================================
// LOKÁLNE PREMENNÉ MODULU SYSTÉMOVÝCH PROCESOV
// ============================================

// Štatistiky systému
struct SystemStats {
  unsigned long totalUptime = 0;      // Celkový čas behu (ms)
  unsigned long chargingTime = 0;     // Celkový čas nabíjania (ms)
  float totalEnergyCharged = 0.0f;    // Celková nahromadená energia (Wh)
  int errorCount = 0;                 // Počet zaznamenaných chýb
  int stateChangeCount = 0;           // Počet zmien stavu systému
} systemStats;

// Indikátor ukončenia konfigurácie
bool configCompleted = false;

// ============================================
// INICIALIZÁCIA SYSTÉMOVÝCH PROCESOV - FUNKCIA
// ============================================

void initSystemProcesses() {
  Serial.println("Inicializácia systémových procesov...");
  
  // Nastavenie počiatočného stavu systému na INICIALIZÁCIA
  changeSystemState(STATE_INIT);
  
  // Načítanie štatistík z EEPROM (ak existujú)
  loadSystemStats();
  
  Serial.println("Systémové procesy úspešne inicializované");
}

// ============================================
// ZMENA STAVU SYSTÉMU - HLAVNÁ FUNKCIA
// ============================================

void changeSystemState(SystemState newState) {
  // Táto funkcia riadi prechody medzi stavmi systému
  
  // AK SA STAV NEMENÍ, NEKONAJEME
  if (currentSystemState == newState) return;
  
  // LOGOVANIE ZMENY STAVU DO SÉRIOVÉHO PORTU
  Serial.print("Zmena stavu systému: ");
  Serial.print(getStateName(currentSystemState));
  Serial.print(" -> ");
  Serial.println(getStateName(newState));
  
  // EXIT AKCIE PRE STARÝ STAV
  onStateExit(currentSystemState);
  
  // AKTUALIZÁCIA STAVOVÝCH PREMENNÝCH
  previousSystemState = currentSystemState;
  currentSystemState = newState;
  stateEntryTime = millis();  // Zaznamenanie času vstupu do nového stavu
  
  // ŠTATISTIKA: Zvýšenie počtu zmien stavu
  systemStats.stateChangeCount++;
  
  // ENTRY AKCIE PRE NOVÝ STAV
  onStateEntry(newState);
  
  // AKTUALIZÁCIA DISPLEJA PODĽA NOVÉHO STAVU
  updateDisplayState();
}

// ============================================
// PREKLAD STAVU NA ČITATEĽNÝ REŤAZEC - FUNKCIA
// ============================================

String getStateName(SystemState state) {
  // Táto funkcia prekladá číselný stav na textový reťazec
  
  switch(state) {
    case STATE_INIT:      return "INICIALIZÁCIA";
    case STATE_SELFTEST:  return "SAMOTESTOVANIE";
    case STATE_IDLE:      return "NECINNOSŤ";
    case STATE_CHARGING:  return "NABÍJANIE";
    case STATE_FAULT:     return "CHYBA";
    case STATE_SLEEP:     return "SPÁNOK";
    case STATE_CONFIG:    return "KONFIGURÁCIA";
    default:              return "NEZNÁMY STAV";
  }
}

// ============================================
// ENTRY AKCIE PRE JEDNOTLIVÉ STAVY - FUNKCIA
// ============================================

void onStateEntry(SystemState state) {
  // Táto funkcia vykonáva akcie pri vstupe do nového stavu
  
  switch(state) {
    case STATE_INIT:
      // Spustenie inicializačných sekvencií
      startInitializationSequence();
      Serial.println("Začiatok inicializácie systémových komponentov");
      break;
      
    case STATE_SELFTEST:
      // Spustenie samotestu všetkých komponentov
      startSelfTest();
      Serial.println("Spustenie samodiagnostiky a testov");
      break;
      
    case STATE_IDLE:
      // Nastavenie do nečinnosti - čakanie na podmienky
      enterIdleMode();
      Serial.println("Prechod do režimu nečinnosti");
      break;
      
    case STATE_CHARGING:
      // Pripraviť systém pre aktívne nabíjanie
      prepareCharging();
      Serial.println("Začiatok aktívneho nabíjania batérie");
      break;
      
    case STATE_FAULT:
      // Spracovanie chybového stavu
      handleFaultEntry();
      Serial.println("Vstup do chybového stavu - analýza problému");
      break;
      
    case STATE_SLEEP:
      // Prechod do spánkového režimu pre šetrenie energie
      enterSleepMode();
      Serial.println("Aktivácia spánkového režimu");
      break;
      
    case STATE_CONFIG:
      // Vstup do konfiguračného režimu
      enterConfigurationMode();
      Serial.println("Vstup do konfiguračného menu");
      break;
  }
}

// ============================================
// EXIT AKCIE PRE JEDNOTLIVÉ STAVY - FUNKCIA
// ============================================

void onStateExit(SystemState state) {
  // Táto funkcia vykonáva akcie pri opustení stavu
  
  switch(state) {
    case STATE_INIT:
      // Čistenie po inicializácii
      cleanupAfterInit();
      Serial.println("Inicializácia dokončená, čistenie dočasných údajov");
      break;
      
    case STATE_SELFTEST:
      // Vyhodnotenie výsledkov testov
      evaluateSelfTest();
      Serial.println("Vyhodnotenie výsledkov samotestov");
      break;
      
    case STATE_SLEEP:
      // Prebudenie zo spánkového režimu
      wakeFromSleep();
      Serial.println("Prebudenie zo spánkového režimu");
      break;
      
    default:
      // Pre ostatné stavy nerobíme špeciálne akcie
      break;
  }
}

// ============================================
// HLAVNÁ SMYČKA SYSTÉMOVÝCH PROCESOV - FUNKCIA
// ============================================

void runSystemProcesses() {
  // Táto funkcia spúšťa aktuálny stav systému
  
  // SPUSTENIE AKTUÁLNEHO STAVU
  switch(currentSystemState) {
    case STATE_INIT:
      runInitState();
      break;
      
    case STATE_SELFTEST:
      runSelfTestState();
      break;
      
    case STATE_IDLE:
      runIdleState();
      break;
      
    case STATE_CHARGING:
      runChargingState();
      break;
      
    case STATE_FAULT:
      runFaultState();
      break;
      
    case STATE_SLEEP:
      runSleepState();
      break;
      
    case STATE_CONFIG:
      runConfigState();
      break;
  }
  
  // SPOLOČNÉ ÚLOHY PRE VŠETKY STAVY
  runCommonTasks();
  
  // AUTOMATICKÉ PRECHODY MEDZI STAVMI
  checkStateTransitions();
  
  // AKTUALIZÁCIA ŠTATISTÍK SYSTÉMU
  updateSystemStats();
}

// ============================================
// IMPLEMENTÁCIA STAVU INICIALIZÁCIE - FUNKCIA
// ============================================

void runInitState() {
  // Táto funkcia riadi stav inicializácie
  
  static int initStep = 0;              // Aktuálny krok inicializácie
  static unsigned long initDelay = 0;   // Časovač pre oneskorenia
  
  switch(initStep) {
    case 0:
      // KROK 1: Inicializácia hardvérových komponentov
      initHardware();
      Serial.println("Krok 1: Hardvér inicializovaný");
      initStep++;
      initDelay = millis();  // Uloženie aktuálneho času
      break;
      
    case 1:
      // ČAKANIE 500ms PRED ĎALŠÍM KROKOM
      if (millis() - initDelay > 500) {
        // KROK 2: Inicializácia senzorov
        initSensors();
        Serial.println("Krok 2: Senzory inicializované");
        initStep++;
        initDelay = millis();
      }
      break;
      
    case 2:
      // ČAKANIE 500ms PRED ĎALŠÍM KROKOM
      if (millis() - initDelay > 500) {
        // KROK 3: Inicializácia komunikačných rozhraní
        initCommunication();
        Serial.println("Krok 3: Komunikácia inicializovaná");
        initStep++;
      }
      break;
      
    case 3:
      // DOKONČENIE INICIALIZÁCIE A PREJAZD NA ĎALŠÍ STAV
      Serial.println("Inicializácia úspešne dokončená");
      changeSystemState(STATE_SELFTEST);
      break;
  }
}

// ============================================
// IMPLEMENTÁCIA STAVU SAMOTESTOVANIA - FUNKCIA
// ============================================

void runSelfTestState() {
  // Táto funkcia riadi samotestovací stav
  
  static bool testsCompleted = false;  // Indikátor dokončenia testov
  
  if (!testsCompleted) {
    // SPUSTENIE VŠETKÝCH TESTOV
    bool testResult = performAllTests();
    
    if (testResult) {
      // VŠETKY TESTY PREBEHLI ÚSPEŠNE
      Serial.println("Všetky samotesty prebehli úspešne");
      testsCompleted = true;
      
      // KRÁTKA PAUZA PRED PREJAZDOM
      delay(1000);
      changeSystemState(STATE_IDLE);
    } else {
      // NIEKTORÝ TEST ZLYHAL
      Serial.println("Samotesty zlyhali! Prechod do chybového stavu");
      changeSystemState(STATE_FAULT);
    }
  }
}

// ============================================
// IMPLEMENTÁCIA STAVU NECINNOSTI - FUNKCIA
// ============================================

void runIdleState() {
  // Táto funkcia riadi stav nečinnosti
  
  // KONTROLA PODMIENOK PRE ZAČIATOK NABÍJANIA
  bool chargingConditions = 
    panelVoltage > batteryVoltage + 2.0f &&        // Panel má vyššie napätie
    batteryVoltage > currentProfile->minVoltage && // Batéria nie je prázdna
    batteryVoltage < currentProfile->absorptionVoltage && // Nie je plná
    !lowPowerMode;                                 // Nie sme v šetrnom režime
  
  if (chargingConditions) {
    changeSystemState(STATE_CHARGING);
    return;
  }
  
  // KONTROLA NECINNOSTI - PREJAZD DO SPÁNKU
  unsigned long idleTime = millis() - lastActivityTime;
  if (idleTime > 300000) {  // 5 minút nečinnosti
    changeSystemState(STATE_SLEEP);
  }
  
  // MINIMÁLNA AKTIVITA V IDLE STAVE
  static unsigned long lastIdleUpdate = 0;
  if (millis() - lastIdleUpdate > 1000) {
    // AKTUALIZÁCIA DISPLEJA S AKTUÁLNYMI ÚDAJMI
    updateDisplayIdle();
    lastIdleUpdate = millis();
  }
}

// ============================================
// IMPLEMENTÁCIA STAVU NABÍJANIA - FUNKCIA
// ============================================

void runChargingState() {
  // Táto funkcia riadi stav aktívneho nabíjania
  
  // SPUSTENIE NABÍJACIEHO ALGORITMU
  runChargingAlgorithm();
  
  // KONTROLA PODMIENOK PRE UKONČENIE NABÍJANIA
  bool stopChargingConditions = 
    batteryVoltage >= currentProfile->absorptionVoltage && 
    batteryCurrent < 0.1f;  // Príliš nízky prúd
  
  if (stopChargingConditions) {
    changeSystemState(STATE_IDLE);
    return;
  }
  
  // KONTROLA KRITICKÝCH CHÝB POČAS NABÍJANIA
  if (checkCriticalErrors()) {
    changeSystemState(STATE_FAULT);
    return;
  }
  
  // AKTUALIZÁCIA DISPLEJA POČAS NABÍJANIA
  static unsigned long lastChargingUpdate = 0;
  if (millis() - lastChargingUpdate > 1000) {
    updateDisplayCharging();
    lastChargingUpdate = millis();
  }
}

// ============================================
## **Pokračujem s ďalšími súbormi...**

Chcete, aby som pokračoval s kompletnymi opravenými súbormi `6_Onboard_Telemetry.ino`, `7_Wireless_Telemetry.ino`, `8_LCDMenu.ino` a finálnym hlavným súborom? Všetky budú obsahovať detailné slovenské komentáre a oznámenia.