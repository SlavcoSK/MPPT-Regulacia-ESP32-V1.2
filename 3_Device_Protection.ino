// ============================================
// OCHRANNÉ FUNKCIE A BEZPEČNOSTNÉ LIMITY
// ============================================

// ============================================
// KONŠTANTY PRE BEZPEČNOSTNÉ LIMITY
// ============================================

// Batéria LiFePO4 12V (4S)
#define BATTERY_VOLTAGE_CRITICAL_HIGH 15.0f    // Absolútne maximum!
#define BATTERY_VOLTAGE_ABSORPTION 14.6f       // Absorpčné napätie
#define BATTERY_VOLTAGE_FLOAT 13.8f            // Plávajúce napätie
#define BATTERY_VOLTAGE_LOW 11.0f              // Nízke napätie varovanie
#define BATTERY_VOLTAGE_CRITICAL_LOW 10.0f     // Kriticky nízke napätie
#define BATTERY_VOLTAGE_HYSTERESIS 0.2f        // Hysteréza pre prepínanie

// Prúdové limity
#define MAX_CHARGE_CURRENT 20.0f               // Maximálny nabíjací prúd
#define MAX_DISCHARGE_CURRENT 30.0f            // Maximálny vybíjací prúd
#define CURRENT_SHORT_CIRCUIT_THRESHOLD 50.0f  // Prah pre zkrat

// Teplotné limity
#define TEMPERATURE_MAX 70.0f                  // Maximálna teplota MOSFETu
#define TEMPERATURE_WARNING 60.0f              // Varovanie pri vysokej teplote
#define BATTERY_TEMP_MAX 50.0f                 // Maximálna teplota batérie
#define BATTERY_TEMP_MIN -10.0f                // Minimálna teplota nabíjania

// Panelové napätie
#define PANEL_VOLTAGE_MAX 60.0f                // Maximálne vstupné napätie
#define PANEL_VOLTAGE_MIN 10.0f                // Minimálne napätie pre fungovanie

// Rýchlosť zmeny prúdu (di/dt ochrana)
#define MAX_CURRENT_RISE_RATE 100.0f           // A/ms

// ============================================
// PREMENNÉ PRE OCHRANNÝ SYSTÉM
// ============================================

enum ProtectionState {
  PROTECTION_NORMAL,        // Všetko v poriadku
  PROTECTION_WARNING,       // Varovanie - redukovaný výkon
  PROTECTION_FAULT,         // Chyba - zastavené nabíjanie
  PROTECTION_EMERGENCY      // Kritická chyba - úplné vypnutie
};

ProtectionState currentProtection = PROTECTION_NORMAL;

// Časové značky pre oneskorené reakcie
unsigned long overVoltageStartTime = 0;
unsigned long overTempStartTime = 0;
unsigned long underVoltageStartTime = 0;

// Stavové premenne pre ochrany
bool overVoltageFlag = false;
bool underVoltageFlag = false;
bool overTempFlag = false;
bool shortCircuitFlag = false;

// Historické hodnoty pre detekciu rýchlych zmien
float previousBatteryCurrent = 0.0f;
unsigned long previousCurrentTime = 0;

// ============================================
// ZÁKLADNÉ OCHRANNÉ KONTROLY
// ============================================

void checkBasicProtections() {
  // Resetovať vlajky pred kontrolou
  overVoltageFlag = false;
  underVoltageFlag = false;
  overTempFlag = false;
  
  // 1. KONTROLA PREKROČENIA NAPÄTIA BATÉRIE
  if (batteryVoltage >= BATTERY_VOLTAGE_CRITICAL_HIGH) {
    overVoltageFlag = true;
    if (overVoltageStartTime == 0) {
      overVoltageStartTime = millis();  // Začiatok časovača
    }
    
    // Okamžitá akcia pri kritickom napätí
    if (batteryVoltage >= BATTERY_VOLTAGE_CRITICAL_HIGH + 1.0f) {
      emergencyShutdown("KRIT_NAPIATIE");
      return;
    }
  } else {
    overVoltageStartTime = 0;  // Resetovať časovač
  }
  
  // 2. KONTROLA PRÍLIŠ NÍZKEHO NAPÄTIA BATÉRIE
  if (batteryVoltage <= BATTERY_VOLTAGE_CRITICAL_LOW) {
    underVoltageFlag = true;
    if (underVoltageStartTime == 0) {
      underVoltageStartTime = millis();
    }
  } else {
    underVoltageStartTime = 0;
  }
  
  // 3. KONTROLA PRETOČENIA TEPLOTY
  if (temperature >= TEMPERATURE_MAX) {
    overTempFlag = true;
    if (overTempStartTime == 0) {
      overTempStartTime = millis();
    }
  } else {
    overTempStartTime = 0;
  }
  
  // 4. KONTROLA MAXIMÁLNEHO PRÚDU
  if (batteryCurrent > MAX_CHARGE_CURRENT) {
    reduceChargeCurrent();  // Postupné obmedzenie
  }
  
  // 5. KONTROLA NAPÄTIA PANELU
  if (panelVoltage > PANEL_VOLTAGE_MAX) {
    logWarning("Panelové napätie príliš vysoké");
    disableChargingTemporarily(5000);  // 5 sekúnd pauza
  }
}

// ============================================
// POKROČILÉ OCHRANY (di/dt, zkrat, atď.)
// ============================================

bool checkAdvancedProtections() {
  unsigned long currentTime = millis();
  float timeDiff = (currentTime - previousCurrentTime) / 1000.0f;  // v sekundách
  
  if (timeDiff > 0 && previousCurrentTime != 0) {
    // Výpočet rýchlosti zmeny prúdu (di/dt)
    float currentRiseRate = abs(batteryCurrent - previousBatteryCurrent) / timeDiff;
    
    // 1. DETEKCIA ZKRATU PODĽA di/dt
    if (currentRiseRate > MAX_CURRENT_RISE_RATE) {
      emergencyShutdown("ZKART_PRUD");
      return false;
    }
    
    // 2. DETEKCIA REVERZNÉHO PRÚDU (panel vybíja batériu)
    if (batteryCurrent < -0.5f && panelVoltage < batteryVoltage) {
      logWarning("Reversný prúd detekovaný");
      disableCharging();
      return false;
    }
  }
  
  // Uloženie aktuálnych hodnôt pre ďalšiu iteráciu
  previousBatteryCurrent = batteryCurrent;
  previousCurrentTime = currentTime;
  
  return true;
}

// ============================================
// TEČUOVÁ OCHRANA BATÉRIE
// ============================================

void checkBatteryTemperatureProtection() {
  // 1. Kontrola či je batéria príliš studená na nabíjanie
  if (temperature < BATTERY_TEMP_MIN) {
    logWarning("Batéria príliš studená na nabíjanie");
    setChargeCurrentLimit(0.1f);  // Veľmi malý prúd
    return;
  }
  
  // 2. Kontrola či je batéria príliš horúca
  if (temperature > BATTERY_TEMP_MAX) {
    logWarning("Batéria príliš horúca");
    
    // Postupné redukovanie prúdu so stúpajúcou teplotou
    float reductionFactor = 1.0f - ((temperature - BATTERY_TEMP_MAX) / 20.0f);
    reductionFactor = constrain(reductionFactor, 0.0f, 1.0f);
    
    float newLimit = MAX_CHARGE_CURRENT * reductionFactor;
    setChargeCurrentLimit(newLimit);
  }
}

// ============================================
// FUNKCIE PRE OCHRANU PROTI PREPLATENIU
// ============================================

void checkOverchargeProtection() {
  static bool absorptionPhase = false;
  static unsigned long absorptionStartTime = 0;
  
  // Ak batéria dosiahla absorpčné napätie
  if (batteryVoltage >= BATTERY_VOLTAGE_ABSORPTION && !absorptionPhase) {
    absorptionPhase = true;
    absorptionStartTime = millis();
    logEvent("Začiatok absorpčnej fázy");
  }
  
  // Počas absorpčnej fázy
  if (absorptionPhase) {
    unsigned long absorptionTime = millis() - absorptionStartTime;
    
    // Absorpčná fáza trvá max 2 hodiny
    if (absorptionTime > 120 * 60 * 1000UL) {  // 120 minút v ms
      absorptionPhase = false;
      logEvent("Koniec absorpčnej fázy (čas)");
      switchToFloatMode();
    }
    
    // Alebo keď prúd klesne pod 5% max prúdu
    if (batteryCurrent < MAX_CHARGE_CURRENT * 0.05f) {
      absorptionPhase = false;
      logEvent("Koniec absorpčnej fázy (prúd)");
      switchToFloatMode();
    }
  }
}

// ============================================
// RIADENIE OCHRAN PODĽA STAVU
// ============================================

ProtectionState evaluateProtectionState() {
  // Kritické stavy - okamžitá akcia
  if (batteryVoltage >= BATTERY_VOLTAGE_CRITICAL_HIGH) {
    return PROTECTION_EMERGENCY;
  }
  
  if (batteryVoltage <= BATTERY_VOLTAGE_CRITICAL_LOW) {
    return PROTECTION_FAULT;
  }
  
  if (temperature >= TEMPERATURE_MAX) {
    return PROTECTION_FAULT;
  }
  
  if (batteryCurrent >= CURRENT_SHORT_CIRCUIT_THRESHOLD) {
    return PROTECTION_EMERGENCY;
  }
  
  // Varovné stavy - redukcia výkonu
  if (batteryVoltage >= BATTERY_VOLTAGE_ABSORPTION ||
      temperature >= TEMPERATURE_WARNING ||
      batteryCurrent >= MAX_CHARGE_CURRENT * 0.8f) {
    return PROTECTION_WARNING;
  }
  
  // Normálny stav
  return PROTECTION_NORMAL;
}

void applyProtectionActions(ProtectionState state) {
  switch(state) {
    case PROTECTION_NORMAL:
      // Plný výkon povolený
      setPWMlimit(100.0f);
      setChargeCurrentLimit(MAX_CHARGE_CURRENT);
      break;
      
    case PROTECTION_WARNING:
      // Redukovaný výkon
      setPWMlimit(70.0f);  // 70% max výkonu
      setChargeCurrentLimit(MAX_CHARGE_CURRENT * 0.7f);
      logWarning("Výkon redukovaný - varovný stav");
      break;
      
    case PROTECTION_FAULT:
      // Zastavenie nabíjania
      disableCharging();
      logError("Nabíjanie zastavené - chybový stav");
      break;
      
    case PROTECTION_EMERGENCY:
      // Úplné vypnutie
      emergencyShutdown("EMERGENCY_STOP");
      break;
  }
}

// ============================================
// EMERGENCY SHUTDOWN PROCEDÚRA
// ============================================

void emergencyShutdown(const char* reason) {
  Serial.print("EMERGENCY SHUTDOWN: ");
  Serial.println(reason);
  
  // 1. Okamžité zastavenie PWM
  ledcWrite(PWM_CHANNEL, 0);
  
  // 2. Aktivácia HW výpojky (ak existuje)
  digitalWrite(SHUTDOWN_PIN, LOW);
  
  // 3. Uloženie chyby do EEPROM
  saveErrorToEEPROM(reason);
  
  // 4. Blikajúca LED indikácia
  while (true) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    
    // Možnosť resetu dlhým stlačením tlačidla
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      delay(3000);  // Čakanie 3 sekundy
      if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        ESP.restart();  // Reset ESP32
      }
    }
  }
}

// ============================================
// LOGOVANIE UDALOSTÍ A CHÝB
// ============================================

void logEvent(const char* message) {
  Serial.print("[EVENT] ");
  Serial.println(message);
  
  // Uloženie do záznamu udalostí
  addToEventLog(message);
}

void logWarning(const char* message) {
  Serial.print("[WARNING] ");
  Serial.println(message);
  
  // Aktualizácia displeja
  updateDisplayWarning(message);
}

void logError(const char* message) {
  Serial.print("[ERROR] ");
  Serial.println(message);
  
  // Uloženie do error logu
  saveErrorToLog(message);
}

// ============================================
// HLAVNÁ FUNKCIA OCHRAN
// ============================================

void runProtectionChecks() {
  // 1. Základné kontroly
  checkBasicProtections();
  
  // 2. Pokročilé ochrany
  if (!checkAdvancedProtections()) {
    return;  // Ak bola aktivovaná emergency ochrana
  }
  
  // 3. Teplotná ochrana batérie
  checkBatteryTemperatureProtection();
  
  // 4. Ochrana proti preplateniu
  checkOverchargeProtection();
  
  // 5. Vyhodnotenie celkového stavu
  ProtectionState newState = evaluateProtectionState();
  
  // 6. Aplikácia opatrení
  if (newState != currentProtection) {
    applyProtectionActions(newState);
    currentProtection = newState;
  }
}

// ============================================
// DIAGNOSTICKÉ FUNKCIE
// ============================================

void printProtectionStatus() {
  Serial.println("=== STATUS OCHRAN ===");
  Serial.print("Stav: ");
  switch(currentProtection) {
    case PROTECTION_NORMAL: Serial.println("NORMALNY"); break;
    case PROTECTION_WARNING: Serial.println("VAROVANIE"); break;
    case PROTECTION_FAULT: Serial.println("CHYBA"); break;
    case PROTECTION_EMERGENCY: Serial.println("KRITICKY"); break;
  }
  
  Serial.print("Over Voltage: "); Serial.println(overVoltageFlag ? "ANO" : "NIE");
  Serial.print("Under Voltage: "); Serial.println(underVoltageFlag ? "ANO" : "NIE");
  Serial.print("Over Temp: "); Serial.println(overTempFlag ? "ANO" : "NIE");
  Serial.print("Short Circuit: "); Serial.println(shortCircuitFlag ? "ANO" : "NIE");
  Serial.println("====================");
}