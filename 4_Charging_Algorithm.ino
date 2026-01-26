// ============================================
// MPPT A NABÍJACIE ALGORITMY
// ============================================

// ============================================
// KONŠTANTY PRE NABÍJANIE
// ============================================

// Typy batérií
enum BatteryType {
  BATTERY_AGM = 0,
  BATTERY_LIFEPO4 = 1,
  BATTERY_LEAD_ACID = 2,
  BATTERY_CUSTOM = 3
};

// Aktuálne nastavený typ batérie
BatteryType selectedBatteryType = BATTERY_AGM;

// Nastavenia pre AGM batériu (štandard pre testovanie)
struct BatteryProfile {
  float bulkVoltage;          // Napätie hromadného nabíjania
  float absorptionVoltage;    // Napätie absorpčného nabíjania
  float floatVoltage;         // Plávajúce napätie
  float equalizationVoltage;  // Napätie vyrovnávacieho nabíjania
  float minVoltage;           // Minimálne povolené napätie
  float maxChargeCurrent;     // Maximálny nabíjací prúd
  bool useEqualization;       // Použiť vyrovnávacie nabíjanie
  int absorptionTime;         // Dĺžka absorpčnej fázy (minúty)
  String batteryName;         // Názov typu batérie
};

// Profily pre rôzne typy batérií
BatteryProfile batteryProfiles[] = {
  // AGM batéria (štandardné testovacie nastavenie)
  {14.4f, 14.5f, 13.8f, 14.8f, 12.0f, 5.0f, false, 120, "AGM"},
  
  // LiFePO4 batéria
  {14.2f, 14.6f, 13.8f, 14.8f, 10.0f, 20.0f, false, 60, "LiFePO4"},
  
  // Kyslý olovený akumulátor
  {14.4f, 14.7f, 13.5f, 15.5f, 11.5f, 10.0f, true, 180, "OLOVO"},
  
  // Vlastný profil
  {14.0f, 14.0f, 13.5f, 14.5f, 12.0f, 5.0f, false, 120, "CUSTOM"}
};

// Aktuálny profil
BatteryProfile* currentProfile = &batteryProfiles[0];

// Fázy nabíjania
enum ChargePhase {
  PHASE_OFF,              // Vypnuté
  PHASE_BULK,             // Hromadné nabíjanie (CC)
  PHASE_ABSORPTION,       // Absorpčné nabíjanie (CV)
  PHASE_FLOAT,            // Plávajúce nabíjanie
  PHASE_EQUALIZATION,     // Vyrovnávacie nabíjanie
  PHASE_MAINTENANCE       // Udržiavací režim
};

ChargePhase currentChargePhase = PHASE_OFF;

// MPPT stav
enum MPPTState {
  MPPT_SCANNING,          // Prehľadávanie MPP
  MPPT_TRACKING,          // Sledovanie MPP
  MPPT_HOLDING,           // Držanie v MPP
  MPPT_LOW_POWER          // Nízky výkon - šetrný režim
};

MPPTState mpptState = MPPT_SCANNING;

// Premenné pre algoritmus
float pwmDutyCycle = 0.0f;        // Aktuálna PWM střída (0-100%)
float mpptStepSize = 0.01f;       // Veľkosť kroku MPPT
float prevPower = 0.0f;           // Predchádzajúci výkon
float prevDuty = 0.0f;            // Predchádzajúca střída
int mpptDirection = 1;            // Smer zmeny (-1 alebo 1)

// Časové premenné
unsigned long absorptionStartTime = 0;
unsigned long floatStartTime = 0;
unsigned long lastMPPTUpdate = 0;
unsigned long lastPhaseChange = 0;

// Šetrný režim
bool lowPowerMode = false;        // Režim s nízkou spotrebou
unsigned long lastPanelActivity = 0;

// ============================================
// INICIALIZÁCIA NABÍJACIEHO SYSTÉMU
// ============================================

void initChargingSystem() {
  Serial.println("Inicializácia nabíjacieho systému...");
  
  // Nastavenie AGM ako štandard
  setBatteryType(BATTERY_AGM);
  
  // Resetovanie PWM
  pwmDutyCycle = 0.0f;
  
  // Inicializácia časovačov
  absorptionStartTime = 0;
  lastMPPTUpdate = millis();
  
  Serial.println("Nabíjací systém pripravený");
  Serial.print("Typ batérie: ");
  Serial.println(currentProfile->batteryName);
}

// ============================================
// NASTAVENIE TYPU BATÉRIE
// ============================================

void setBatteryType(BatteryType type) {
  selectedBatteryType = type;
  currentProfile = &batteryProfiles[type];
  
  Serial.print("Nastavená batéria: ");
  Serial.println(currentProfile->batteryName);
  
  // Výpis parametrov
  Serial.println("Parametre batérie:");
  Serial.print("Bulk: "); Serial.print(currentProfile->bulkVoltage, 1); Serial.println("V");
  Serial.print("Absorpcia: "); Serial.print(currentProfile->absorptionVoltage, 1); Serial.println("V");
  Serial.print("Float: "); Serial.print(currentProfile->floatVoltage, 1); Serial.println("V");
  Serial.print("Max prúd: "); Serial.print(currentProfile->maxChargeCurrent, 1); Serial.println("A");
}

// ============================================
// DETEKCIA SOLÁRNEJ AKTIVITY
// ============================================

bool checkSolarActivity() {
  static float prevPanelVoltage = 0.0f;
  static int lowPowerCounter = 0;
  
  // Ak je panelové napätie veľmi nízke
  if (panelVoltage < 5.0f) {
    lowPowerCounter++;
    
    // Ak dlhšie ako 10 sekúnd žiadne napätie
    if (lowPowerCounter > 100) {  // 100 cyklov po 100ms = 10 sekúnd
      // Aktivácia šetrného režimu
      if (!lowPowerMode) {
        enterLowPowerMode();
      }
      return false;
    }
  } else {
    lowPowerCounter = 0;
    
    // Ak bol v šetrnom režime, opustiť ho
    if (lowPowerMode) {
      exitLowPowerMode();
    }
  }
  
  prevPanelVoltage = panelVoltage;
  return (panelVoltage > 5.0f);
}

// ============================================
// ŠETRNÝ REŽIM (pre noc a slabé osvetlenie)
// ============================================

void enterLowPowerMode() {
  Serial.println("Aktivácia šetrného režimu...");
  lowPowerMode = true;
  
  // 1. Zastavenie PWM
  setPWM(0.0f);
  
  // 2. Zníženie frekvencie meraní
  setMeasurementInterval(5000);  // Meranie každých 5 sekúnd
  
  // 3. Zníženie jasu displeja
  setDisplayBrightness(10);  // 10% jas
  
  // 4. Vypnutie nepotrebných periférií
  disableNonEssentialPeripherals();
  
  // 5. Odpojenie od batérie (ak je HW možnosť)
  disconnectFromBattery();
  
  Serial.println("Šetrný režim aktívny");
}

void exitLowPowerMode() {
  Serial.println("Ukončenie šetrného režimu...");
  lowPowerMode = false;
  
  // 1. Obnovenie meraní
  setMeasurementInterval(100);  // Normálnych 100ms
  
  // 2. Obnovenie jasu displeja
  setDisplayBrightness(100);  // 100% jas
  
  // 3. Zapnutie periférií
  enablePeripherals();
  
  // 4. Pripojenie k batérii
  connectToBattery();
  
  Serial.println("Šetrný režim deaktivovaný");
}

// ============================================
// MPPT ALGORITMUS (Perturb & Observe)
// ============================================

void runMPPTAlgorithm() {
  // Iba ak máme dostatočné panelové napätie
  if (panelVoltage < batteryVoltage + 1.0f) {
    mpptState = MPPT_SCANNING;
    return;
  }
  
  // Iba v hromadnej fáze použijeme MPPT
  if (currentChargePhase != PHASE_BULK) {
    return;
  }
  
  unsigned long currentTime = millis();
  if (currentTime - lastMPPTUpdate < 100) {  // Iba každých 100ms
    return;
  }
  
  lastMPPTUpdate = currentTime;
  
  // Výpočet aktuálneho výkonu
  float currentPower = panelVoltage * panelCurrent;
  
  // Adaptívny krok MPPT
  updateMPPTStepSize(currentPower);
  
  // Perturb and Observe algoritmus
  float deltaPower = currentPower - prevPower;
  float deltaDuty = pwmDutyCycle - prevDuty;
  
  if (deltaPower > 0) {
    // Výkon stúpol - pokračovať v rovnakom smere
    mpptDirection = (deltaDuty > 0) ? 1 : -1;
  } else {
    // Výkon klesol - zmeniť smer
    mpptDirection = (deltaDuty > 0) ? -1 : 1;
  }
  
  // Aplikovanie zmeny
  pwmDutyCycle += mpptDirection * mpptStepSize;
  
  // Obmedzenie rozsahu
  pwmDutyCycle = constrain(pwmDutyCycle, 0.0f, 95.0f);  // Max 95% pre bezpečnosť
  
  // Uloženie hodnôt pre ďalšiu iteráciu
  prevPower = currentPower;
  prevDuty = pwmDutyCycle;
  
  // Aktualizácia stavu MPPT
  if (abs(deltaPower) < 0.1f) {  // Malá zmena výkonu
    mpptState = MPPT_HOLDING;
  } else {
    mpptState = MPPT_TRACKING;
  }
  
  // Aplikovanie PWM
  applyPWM(pwmDutyCycle);
}

// ============================================
// ADAPTÍVNY KROK MPPT
// ============================================

void updateMPPTStepSize(float currentPower) {
  // Základný krok
  mpptStepSize = 0.01f;
  
  // Prispôsobenie podľa podmienok
  if (panelVoltage < 10.0f) {
    // Veľmi slabé svetlo - väčší krok
    mpptStepSize = 0.05f;
  } else if (currentPower > 50.0f) {
    // Vysoký výkon - menší krok pre presnosť
    mpptStepSize = 0.005f;
  } else if (currentPower < 5.0f) {
    // Veľmi nízky výkon - väčší krok
    mpptStepSize = 0.02f;
  }
  
  // Ak sme blízko maxima, zmenšiť krok
  if (abs(pwmDutyCycle - prevDuty) < 0.01f && abs(currentPower - prevPower) < 0.5f) {
    mpptStepSize *= 0.5f;
  }
}

// ============================================
// ROZHODOVANIE O FÁZE NABÍJANIA
// ============================================

void determineChargePhase() {
  // Kontrola podmienok pre zmenu fázy
  
  // 1. OFF -> BULK
  if (currentChargePhase == PHASE_OFF) {
    if (panelVoltage > batteryVoltage + 2.0f &&  // Panel má dostatok napätia
        batteryVoltage < currentProfile->absorptionVoltage &&  // Batéria nie je plná
        batteryVoltage > currentProfile->minVoltage &&  // Batéria nie je prázdna
        checkSolarActivity()) {  // Slnko svieti
      
      changeChargePhase(PHASE_BULK);
    }
    return;
  }
  
  // 2. BULK -> ABSORPTION
  if (currentChargePhase == PHASE_BULK) {
    if (batteryVoltage >= currentProfile->absorptionVoltage) {
      absorptionStartTime = millis();
      changeChargePhase(PHASE_ABSORPTION);
    }
    return;
  }
  
  // 3. ABSORPTION -> FLOAT
  if (currentChargePhase == PHASE_ABSORPTION) {
    unsigned long absorptionElapsed = (millis() - absorptionStartTime) / 60000;  // Minúty
    
    // Podmienka 1: Uplynulý čas
    bool timeCondition = absorptionElapsed >= currentProfile->absorptionTime;
    
    // Podmienka 2: Prúd klesol pod prah
    bool currentCondition = batteryCurrent <= (currentProfile->maxChargeCurrent * ABSORPTION_CURRENT_THRESHOLD);
    
    if (timeCondition || currentCondition) {
      floatStartTime = millis();
      changeChargePhase(PHASE_FLOAT);
    }
    return;
  }
  
  // 4. FLOAT -> MAINTENANCE alebo OFF
  if (currentChargePhase == PHASE_FLOAT) {
    unsigned long floatElapsed = (millis() - floatStartTime) / 60000;  // Minúty
    
    // Ak po dlhšom čase v float fáze (24h) a batéria je stabilná
    if (floatElapsed >= 1440) {  // 24 hodín
      changeChargePhase(PHASE_MAINTENANCE);
    }
    
    // Ak nemáme solárny výkon, prejsť do OFF
    if (!checkSolarActivity() && floatElapsed > 10) {  // Po 10 minútach bez slnka
      changeChargePhase(PHASE_OFF);
    }
    return;
  }
  
  // 5. MAINTENANCE -> BULK (ak batéria klesla)
  if (currentChargePhase == PHASE_MAINTENANCE) {
    if (batteryVoltage < currentProfile->floatVoltage - 0.5f && 
        checkSolarActivity()) {
      changeChargePhase(PHASE_BULK);
    }
    return;
  }
}

// ============================================
// ZMENA FÁZY NABÍJANIA
// ============================================

void changeChargePhase(ChargePhase newPhase) {
  if (currentChargePhase == newPhase) return;
  
  Serial.print("Zmena fázy: ");
  Serial.print(getPhaseName(currentChargePhase));
  Serial.print(" -> ");
  Serial.println(getPhaseName(newPhase));
  
  // Exit akcie pre starú fázu
  onPhaseExit(currentChargePhase);
  
  // Update fázy
  currentChargePhase = newPhase;
  lastPhaseChange = millis();
  
  // Entry akcie pre novú fázu
  onPhaseEntry(newPhase);
}

String getPhaseName(ChargePhase phase) {
  switch(phase) {
    case PHASE_OFF: return "VYPNUTE";
    case PHASE_BULK: return "HROMADNE";
    case PHASE_ABSORPTION: return "ABSORPCIA";
    case PHASE_FLOAT: return "FLOAT";
    case PHASE_EQUALIZATION: return "VYROVNAVANIE";
    case PHASE_MAINTENANCE: return "UDRIZBA";
    default: return "NEZNÁMY";
  }
}

// ============================================
// AKCIE PRI VSTUPE/VÝSTUPE Z FÁZY
// ============================================

void onPhaseEntry(ChargePhase phase) {
  switch(phase) {
    case PHASE_BULK:
      // Resetovanie MPPT
      prevPower = 0.0f;
      prevDuty = 0.0f;
      mpptState = MPPT_SCANNING;
      Serial.println("Začiatok hromadného nabíjania");
      break;
      
    case PHASE_ABSORPTION:
      // Prechod na CV režim
      Serial.println("Začiatok absorpčného nabíjania");
      break;
      
    case PHASE_FLOAT:
      // Nastavenie plávajúceho napätia
      Serial.println("Prechod do float fázy");
      break;
      
    case PHASE_MAINTENANCE:
      // Minimalizácia spotreby
      Serial.println("Aktivácia udržiavacieho režimu");
      break;
      
    case PHASE_OFF:
      // Úplné zastavenie
      setPWM(0.0f);
      Serial.println("Nabíjanie zastavené");
      break;
  }
}

void onPhaseExit(ChargePhase phase) {
  // Uloženie štatistík pri ukončení fázy
  savePhaseStatistics(phase);
}

// ============================================
// RIADENIE PWM PODĽA FÁZY
// ============================================

void controlChargingByPhase() {
  switch(currentChargePhase) {
    case PHASE_BULK:
      // MPPT algoritmus riadi PWM
      runMPPTAlgorithm();
      // Obmedzenie prúdu podľa profilu
      limitChargeCurrent();
      break;
      
    case PHASE_ABSORPTION:
      // CV režim - udržiavať konštantné napätie
      controlConstantVoltage(currentProfile->absorptionVoltage);
      break;
      
    case PHASE_FLOAT:
      // Udržiavať plávajúce napätie
      controlConstantVoltage(currentProfile->floatVoltage);
      break;
      
    case PHASE_MAINTENANCE:
      // Periodické dobíjanie
      maintenanceCharging();
      break;
      
    case PHASE_OFF:
      // Žiadne nabíjanie
      setPWM(0.0f);
      break;
  }
}

// ============================================
// KONŠTANTNÉ NAPÄTIE (CV) RIADENIE
// ============================================

void controlConstantVoltage(float targetVoltage) {
  // Jednoduchý PI regulátor pre napätie
  static float integral = 0.0f;
  float error = targetVoltage - batteryVoltage;
  
  // P člen
  float pTerm = error * 2.0f;  // P zosilnenie
  
  // I člen (s anti-windup)
  integral += error * 0.01f;  // I zosilnenie
  integral = constrain(integral, -10.0f, 10.0f);  // Anti-windup
  
  // Výpočet novej PWM
  float newDuty = pwmDutyCycle + pTerm + integral;
  
  // Obmedzenie rozsahu
  newDuty = constrain(newDuty, 0.0f, 95.0f);
  
  // Plynulá zmena (rate limiting)
  float maxChange = 0.5f;  // Max 0.5% za cyklus
  if (abs(newDuty - pwmDutyCycle) > maxChange) {
    newDuty = pwmDutyCycle + (newDuty > pwmDutyCycle ? maxChange : -maxChange);
  }
  
  // Aplikovanie
  pwmDutyCycle = newDuty;
  applyPWM(pwmDutyCycle);
}

// ============================================
// OBMEDZENIE NABÍJACIEHO PRÚDU
// ============================================

void limitChargeCurrent() {
  // Ak prúd prekročí maximum, znížiť PWM
  if (batteryCurrent > currentProfile->maxChargeCurrent) {
    float reduction = (batteryCurrent - currentProfile->maxChargeCurrent) * 2.0f;
    pwmDutyCycle -= reduction;
    pwmDutyCycle = max(pwmDutyCycle, 0.0f);
    applyPWM(pwmDutyCycle);
  }
}

// ============================================
// UDRŽIAVACIE NABÍJANIE
// ============================================

void maintenanceCharging() {
  static unsigned long lastMaintenanceCharge = 0;
  unsigned long currentTime = millis();
  
  // Nabíjame každých 24 hodín na 30 minút
  if (currentTime - lastMaintenanceCharge > 24 * 60 * 60 * 1000UL) {
    // 30 minútové nabíjanie
    if (currentTime - lastMaintenanceCharge < 30 * 60 * 1000UL) {
      // Aktívne nabíjanie
      if (batteryVoltage < currentProfile->floatVoltage) {
        controlConstantVoltage(currentProfile->floatVoltage);
      } else {
        setPWM(0.0f);
      }
    } else {
      // Koniec nabíjania
      lastMaintenanceCharge = currentTime;
      setPWM(0.0f);
    }
  } else {
    // Medzi nabíjaním - úplne vypnuté
    setPWM(0.0f);
  }
}

// ============================================
// ODPOJENIE OD BATÉRIE (pre noc)
// ============================================

void disconnectFromBattery() {
  // HW odpojenie (ak je relé alebo MOSFET)
  digitalWrite(BATTERY_DISCONNECT_PIN, LOW);
  
  // SW odpojenie
  setPWM(0.0f);
  currentChargePhase = PHASE_OFF;
  
  Serial.println("Odpojené od batérie");
}

void connectToBattery() {
  // HW pripojenie
  digitalWrite(BATTERY_DISCONNECT_PIN, HIGH);
  
  // Reset fázy
  currentChargePhase = PHASE_OFF;
  
  Serial.println("Pripojené k batérii");
}

// ============================================
// HLAVNÁ FUNKCIA NABÍJACIEHO ALGORITMU
// ============================================

void runChargingAlgorithm() {
  // 1. Kontrola solárnej aktivity
  if (!checkSolarActivity()) {
    // Žiadne slnko - šetrný režim
    if (!lowPowerMode) {
      enterLowPowerMode();
    }
    return;
  }
  
  // 2. Určenie fázy nabíjania
  determineChargePhase();
  
  // 3. Riadanie PWM podľa fázy
  controlChargingByPhase();
  
  // 4. Logovanie stavu
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 5000) {  // Každých 5 sekúnd
    logChargingStatus();
    lastLog = millis();
  }
}

// ============================================
// LOGOVANIE STAVU NABÍJANIA
// ============================================

void logChargingStatus() {
  Serial.println("=== STATUS NABÍJANIA ===");
  Serial.print("Fáza: ");
  Serial.println(getPhaseName(currentChargePhase));
  
  Serial.print("Stav MPPT: ");
  switch(mpptState) {
    case MPPT_SCANNING: Serial.println("SKENOVANIE"); break;
    case MPPT_TRACKING: Serial.println("SLEDOVANIE"); break;
    case MPPT_HOLDING: Serial.println("DRŽANIE"); break;
    case MPPT_LOW_POWER: Serial.println("NÍZKY VÝKON"); break;
  }
  
  Serial.print("PWM: "); Serial.print(pwmDutyCycle, 1); Serial.println("%");
  Serial.print("Panel: "); Serial.print(panelPower, 1); Serial.println("W");
  Serial.print("Batéria: "); Serial.print(batteryPower, 1); Serial.println("W");
  Serial.print("Účinnosť: "); Serial.print(efficiency, 1); Serial.println("%");
  
  if (lowPowerMode) {
    Serial.println("ŠETRNÝ REŽIM AKTÍVNY");
  }
  
  Serial.println("=======================");
}

// ============================================
// MANUÁLNE OVLÁDANIE (pre testovanie)
// ============================================

void setManualPWM(float dutyPercent) {
  // Manuálne nastavenie PWM (pre testovanie)
  pwmDutyCycle = constrain(dutyPercent, 0.0f, 95.0f);
  applyPWM(pwmDutyCycle);
  
  Serial.print("Manuálne PWM: ");
  Serial.print(pwmDutyCycle, 1);
  Serial.println("%");
}

void setChargeCurrent(float currentAmps) {
  // Manuálne nastavenie prúdu (obmedzené profilom)
  float maxCurrent = min(currentAmps, currentProfile->maxChargeCurrent);
  
  // Vypočítať potrebnú PWM pre daný prúd
  // (zjednodušené - v praxi by to chcelo kalibračnú krivku)
  float targetDuty = (maxCurrent / currentProfile->maxChargeCurrent) * 50.0f;
  setManualPWM(targetDuty);
  
  Serial.print("Manuálny prúd: ");
  Serial.print(maxCurrent, 1);
  Serial.println("A");
}

// ============================================
// POMOCNÉ FUNKCIE PRE PWM
// ============================================

void applyPWM(float dutyPercent) {
  // Konverzia percent na hodnotu pre ESP32 PWM
  int pwmValue = (int)(dutyPercent * 10.23f);  // 10-bit rozlíšenie (0-1023)
  pwmValue = constrain(pwmValue, 0, 1023);
  
  ledcWrite(PWM_CHANNEL, pwmValue);
}

void setPWM(float dutyPercent) {
  pwmDutyCycle = dutyPercent;
  applyPWM(dutyPercent);
}