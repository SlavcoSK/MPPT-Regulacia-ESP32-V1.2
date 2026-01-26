// ============================================
// SYSTÉMOVÉ PROCESY A STAVOVÝ AUTOMAT
// ============================================

// ============================================
// STAVY SYSTÉMU
// ============================================

enum SystemState {
  STATE_INIT,           // Inicializácia
  STATE_SELFTEST,       // Samodiagnostika
  STATE_IDLE,           // Nečinnosť
  STATE_CHARGING,       // Aktívne nabíjanie
  STATE_FAULT,          // Chybový stav
  STATE_SLEEP,          // Spánkový režim
  STATE_CONFIG          // Konfigurácia
};

SystemState currentSystemState = STATE_INIT;
SystemState previousSystemState = STATE_INIT;

// Časové premenné
unsigned long stateEntryTime = 0;
unsigned long lastActivityTime = 0;
unsigned long sleepStartTime = 0;

// Štatistiky systému
struct SystemStats {
  unsigned long totalUptime = 0;
  unsigned long chargingTime = 0;
  float totalEnergyCharged = 0.0f;  // Wh
  int errorCount = 0;
  int stateChangeCount = 0;
} systemStats;

// ============================================
// INICIALIZÁCIA SYSTÉMU
// ============================================

void initSystemProcesses() {
  Serial.println("Inicializácia systémových procesov...");
  
  // Nastavenie počiatočného stavu
  changeSystemState(STATE_INIT);
  
  // Inicializácia štatistík
  loadSystemStats();
  
  Serial.println("Systémové procesy pripravené");
}

// ============================================
// ZMENA STAVU SYSTÉMU
// ============================================

void changeSystemState(SystemState newState) {
  if (currentSystemState == newState) return;
  
  // Logovanie zmeny stavu
  Serial.print("Zmena stavu systému: ");
  Serial.print(getStateName(currentSystemState));
  Serial.print(" -> ");
  Serial.println(getStateName(newState));
  
  // Exit akcie pre starý stav
  onStateExit(currentSystemState);
  
  // Update stavu
  previousSystemState = currentSystemState;
  currentSystemState = newState;
  stateEntryTime = millis();
  
  // Štatistika
  systemStats.stateChangeCount++;
  
  // Entry akcie pre nový stav
  onStateEntry(newState);
  
  // Aktualizácia displeja
  updateDisplayState();
}

String getStateName(SystemState state) {
  switch(state) {
    case STATE_INIT: return "INICIALIZÁCIA";
    case STATE_SELFTEST: return "SAMOTESTOVANIE";
    case STATE_IDLE: return "NECINNOSŤ";
    case STATE_CHARGING: return "NABÍJANIE";
    case STATE_FAULT: return "CHYBA";
    case STATE_SLEEP: return "SPÁNOK";
    case STATE_CONFIG: return "KONFIGURÁCIA";
    default: return "NEZNÁMY";
  }
}

// ============================================
// ENTRY AKCIE PRE STAVY
// ============================================

void onStateEntry(SystemState state) {
  switch(state) {
    case STATE_INIT:
      // Spustenie inicializačných sekvencií
      startInitializationSequence();
      break;
      
    case STATE_SELFTEST:
      // Spustenie samotestu
      startSelfTest();
      break;
      
    case STATE_IDLE:
      // Nastavenie do nečinnosti
      enterIdleMode();
      break;
      
    case STATE_CHARGING:
      // Pripraviť nabíjanie
      prepareCharging();
      break;
      
    case STATE_FAULT:
      // Spracovanie chyby
      handleFaultEntry();
      break;
      
    case STATE_SLEEP:
      // Prechod do spánku
      enterSleepMode();
      break;
      
    case STATE_CONFIG:
      // Vstup do konfigurácie
      enterConfigurationMode();
      break;
  }
}

// ============================================
// EXIT AKCIE PRE STAVY
// ============================================

void onStateExit(SystemState state) {
  switch(state) {
    case STATE_INIT:
      // Čistenie po inicializácii
      cleanupAfterInit();
      break;
      
    case STATE_SELFTEST:
      // Vyhodnotenie testov
      evaluateSelfTest();
      break;
      
    case STATE_SLEEP:
      // Prebudenie zo spánku
      wakeFromSleep();
      break;
  }
}

// ============================================
// HLAVNÁ SMYČKA SYSTÉMOVÝCH PROCESOV
// ============================================

void runSystemProcesses() {
  // Spustenie aktuálneho stavu
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
  
  // Spoločné úlohy pre všetky stavy
  runCommonTasks();
  
  // Automatické prechody medzi stavmi
  checkStateTransitions();
  
  // Aktualizácia štatistík
  updateSystemStats();
}

// ============================================
// IMPLEMENTÁCIA JEDNOTLIVÝCH STAVOV
// ============================================

void runInitState() {
  static int initStep = 0;
  static unsigned long initDelay = 0;
  
  switch(initStep) {
    case 0:
      // Krok 1: Inicializácia HW
      initHardware();
      initStep++;
      initDelay = millis();
      break;
      
    case 1:
      // Čakanie 500ms
      if (millis() - initDelay > 500) {
        // Krok 2: Inicializácia senzorov
        initSensors();
        initStep++;
        initDelay = millis();
      }
      break;
      
    case 2:
      // Čakanie 500ms
      if (millis() - initDelay > 500) {
        // Krok 3: Inicializácia komunikácie
        initCommunication();
        initStep++;
      }
      break;
      
    case 3:
      // Dokončenie inicializácie
      Serial.println("Inicializácia dokončená");
      changeSystemState(STATE_SELFTEST);
      break;
  }
}

void runSelfTestState() {
  static bool testsCompleted = false;
  
  if (!testsCompleted) {
    // Spustenie testov
    bool testResult = performAllTests();
    
    if (testResult) {
      Serial.println("Všetky testy prebehli úspešne");
      testsCompleted = true;
      
      // Krátka pauza pred prechodom
      delay(1000);
      changeSystemState(STATE_IDLE);
    } else {
      Serial.println("Testy zlyhali!");
      changeSystemState(STATE_FAULT);
    }
  }
}

void runIdleState() {
  // Kontrola podmienok pre začiatok nabíjania
  bool chargingConditions = 
    panelVoltage > batteryVoltage + 2.0f &&  // Panel má vyššie napätie
    batteryVoltage > currentProfile->minVoltage &&  // Batéria nie je prázdna
    !lowPowerMode;  // Nie sme v šetrnom režime
  
  if (chargingConditions) {
    changeSystemState(STATE_CHARGING);
    return;
  }
  
  // Kontrola nečinnosti - prechod do spánku
  unsigned long idleTime = millis() - lastActivityTime;
  if (idleTime > 300000) {  // 5 minút nečinnosti
    changeSystemState(STATE_SLEEP);
  }
  
  // Minimálna aktivita v idle stave
  static unsigned long lastIdleUpdate = 0;
  if (millis() - lastIdleUpdate > 1000) {
    // Aktualizácia displeja
    updateDisplayIdle();
    lastIdleUpdate = millis();
  }
}

void runChargingState() {
  // Spustenie nabíjacieho algoritmu
  runChargingAlgorithm();
  
  // Kontrola podmienok pre ukončenie nabíjania
  bool stopChargingConditions = 
    batteryVoltage >= currentProfile->absorptionVoltage && 
    batteryCurrent < 0.1f;  // Príliš nízky prúd
  
  if (stopChargingConditions) {
    changeSystemState(STATE_IDLE);
    return;
  }
  
  // Kontrola chýb počas nabíjania
  if (checkCriticalErrors()) {
    changeSystemState(STATE_FAULT);
    return;
  }
  
  // Aktualizácia displeja
  static unsigned long lastChargingUpdate = 0;
  if (millis() - lastChargingUpdate > 1000) {
    updateDisplayCharging();
    lastChargingUpdate = millis();
  }
}

void runFaultState() {
  // Zastavenie všetkých procesov
  setPWM(0.0f);
  
  // Zobrazenie chyby na displeji
  displayFaultMessage();
  
  // Blikanie LED indikátora
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
  
  // Možnosť resetu
  checkForReset();
}

void runSleepState() {
  // Minimálna aktivita v spánku
  static unsigned long lastSleepCheck = 0;
  
  if (millis() - lastSleepCheck > 5000) {  // Každých 5 sekúnd
    // Kontrola solárnej aktivity
    if (panelVoltage > 10.0f) {
      // Slnko svieti - prebudiť sa
      changeSystemState(STATE_IDLE);
    }
    
    lastSleepCheck = millis();
  }
  
  // Deep sleep možnosť (ak je veľmi dlho tma)
  unsigned long sleepDuration = millis() - sleepStartTime;
  if (sleepDuration > 3600000) {  // Po 1 hodine
    enterDeepSleep();
  }
}

void runConfigState() {
  // Spracovanie konfigurácie
  handleConfiguration();
  
  // Kontrola ukončenia konfigurácie
  if (configCompleted) {
    changeSystemState(STATE_IDLE);
  }
}

// ============================================
// KONTROLA AUTOMATICKÝCH PREJAZDOV
// ============================================

void checkStateTransitions() {
  // Automatické prechody podľa podmienok
  
  // Z FAULT do IDLE po resetovaní chyby
  if (currentSystemState == STATE_FAULT) {
    if (faultCleared && millis() - stateEntryTime > 5000) {
      faultCleared = false;
      changeSystemState(STATE_IDLE);
    }
  }
  
  // Z IDLE do SLEEP pri dlhej nečinnosti
  if (currentSystemState == STATE_IDLE) {
    unsigned long idleTime = millis() - lastActivityTime;
    if (idleTime > 300000 && panelVoltage < 5.0f) {  // 5 minút bez slnka
      changeSystemState(STATE_SLEEP);
    }
  }
}

// ============================================
// SPOLOČNÉ ÚLOHY PRE VŠETKY STAVY
// ============================================

void runCommonTasks() {
  unsigned long currentTime = millis();
  
  // 1. Časovač watchdog
  static unsigned long lastWatchdog = 0;
  if (currentTime - lastWatchdog > 1000) {
    feedWatchdog();
    lastWatchdog = currentTime;
  }
  
  // 2. Kontrola tlačidiel
  static unsigned long lastButtonCheck = 0;
  if (currentTime - lastButtonCheck > 50) {
    checkButtons();
    lastButtonCheck = currentTime;
  }
  
  // 3. Spracovanie sériovej komunikácie
  static unsigned long lastSerialCheck = 0;
  if (currentTime - lastSerialCheck > 100) {
    processSerialInput();
    lastSerialCheck = currentTime;
  }
  
  // 4. Aktualizácia času nečinnosti
  if (isActivityDetected()) {
    lastActivityTime = currentTime;
  }
}

// ============================================
// SAMOTESTOVACIE FUNKCIE
// ============================================

bool performAllTests() {
  Serial.println("Spúšťam samotestovanie...");
  
  bool allTestsPassed = true;
  
  // Test 1: ADC senzory
  Serial.print("Test ADC senzorov... ");
  if (testADCSensors()) {
    Serial.println("OK");
  } else {
    Serial.println("CHYBA");
    allTestsPassed = false;
  }
  
  // Test 2: PWM výstup
  Serial.print("Test PWM výstupu... ");
  if (testPWMOutput()) {
    Serial.println("OK");
  } else {
    Serial.println("CHYBA");
    allTestsPassed = false;
  }
  
  // Test 3: Teplotný senzor
  Serial.print("Test teplotného senzora... ");
  if (testTemperatureSensor()) {
    Serial.println("OK");
  } else {
    Serial.println("CHYBA");
    allTestsPassed = false;
  }
  
  // Test 4: Pamäť EEPROM
  Serial.print("Test EEPROM... ");
  if (testEEPROM()) {
    Serial.println("OK");
  } else {
    Serial.println("CHYBA");
    allTestsPassed = false;
  }
  
  // Test 5: Komunikácia
  Serial.print("Test komunikácie... ");
  if (testCommunication()) {
    Serial.println("OK");
  } else {
    Serial.println("CHYBA");
    allTestsPassed = false;
  }
  
  return allTestsPassed;
}

bool testADCSensors() {
  // Test ADC čítaním všetkých kanálov
  for (int i = 0; i < 4; i++) {
    int16_t value = ads.readADC_SingleEnded(i);
    if (value < -1000 || value > 1000) {  // Rozumný rozsah
      return false;
    }
    delay(10);
  }
  return true;
}

bool testPWMOutput() {
  // Test PWM postupným zvyšovaním
  for (int i = 0; i <= 100; i += 10) {
    setPWM(i);
    delay(50);
  }
  setPWM(0);
  return true;
}

// ============================================
// ŠETRNÝ/SPÁNKOVÝ REŽIM
// ============================================

void enterSleepMode() {
  Serial.println("Prechod do spánkového režimu...");
  
  // Zaznamenanie času vstupu
  sleepStartTime = millis();
  
  // 1. Zastavenie PWM
  setPWM(0.0f);
  
  // 2. Vypnutie displeja
  turnOffDisplay();
  
  // 3. Zníženie frekvencie CPU
  setCpuFrequencyMhz(80);
  
  // 4. Vypnutie nepotrebných periférií
  disablePeripherals();
  
  // 5. Nastavenie interruptu pre prebudenie
  setupWakeupInterrupt();
  
  Serial.println("Spánkový režim aktívny");
}

void enterDeepSleep() {
  Serial.println("Prechod do hlbokého spánku...");
  
  // Uloženie stavu do EEPROM
  saveSystemState();
  
  // Nastavenie prebudenia po 30 minútach alebo pri zmene na vstupe
  esp_sleep_enable_timer_wakeup(30 * 60 * 1000000);  // 30 minút
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);  // Prebudenie pri zmene na pine 33
  
  // Vstup do deep sleep
  esp_deep_sleep_start();
}

void wakeFromSleep() {
  Serial.println("Prebudenie zo spánku...");
  
  // Obnovenie frekvencie CPU
  setCpuFrequencyMhz(240);
  
  // Zapnutie periférií
  enablePeripherals();
  
  // Zapnutie displeja
  turnOnDisplay();
  
  // Načítanie uloženého stavu
  loadSystemState();
  
  Serial.println("Systém prebudený");
}

// ============================================
// SPRACOVANIE CHÝB
// ============================================

bool checkCriticalErrors() {
  // Kontrola kritických chýb
  
  // 1. Prekročenie napätia batérie
  if (batteryVoltage > currentProfile->absorptionVoltage + 1.0f) {
    logError("KRITICKÉ: Prekročené napätie batérie");
    return true;
  }
  
  // 2. Príliš nízke napätie batérie
  if (batteryVoltage < currentProfile->minVoltage - 1.0f) {
    logError("KRITICKÉ: Príliš nízke napätie batérie");
    return true;
  }
  
  // 3. Prehriatie
  if (temperature > 80.0f) {
    logError("KRITICKÉ: Prehriatie systému");
    return true;
  }
  
  // 4. Zkrat
  if (batteryCurrent > 30.0f) {
    logError("KRITICKÉ: Prúdový zkrat");
    return true;
  }
  
  return false;
}

void handleFaultEntry() {
  // Zastavenie všetkých aktívnych procesov
  emergencyStop();
  
  // Uloženie chyby do logu
  saveFaultToLog();
  
  // Notifikácia cez komunikáciu
  sendFaultNotification();
  
  // Resetovanie chybových stavov
  faultCleared = false;
}

void clearFault() {
  // Resetovanie chybových stavov
  faultCleared = true;
  
  // Resetovanie chybových indikátorov
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Chyba vyčistená");
}

// ============================================
// ŠTATISTIKY SYSTÉMU
// ============================================

void updateSystemStats() {
  unsigned long currentTime = millis();
  
  // Celkový čas behu
  systemStats.totalUptime = currentTime;
  
  // Čas nabíjania
  if (currentSystemState == STATE_CHARGING) {
    systemStats.chargingTime += 100;  // Približne (volané každých 100ms)
  }
  
  // Nahromadená energia (Wh)
  static unsigned long lastEnergyUpdate = 0;
  if (currentTime - lastEnergyUpdate > 1000) {  // Každú sekundu
    float energyThisSecond = batteryPower / 3600.0f;  // Wh za sekundu
    systemStats.totalEnergyCharged += energyThisSecond;
    lastEnergyUpdate = currentTime;
  }
}

void printSystemStats() {
  Serial.println("=== ŠTATISTIKA SYSTÉMU ===");
  Serial.print("Celkový čas behu: ");
  Serial.print(systemStats.totalUptime / 3600000.0f, 1);
  Serial.println(" hodín");
  
  Serial.print("Čas nabíjania: ");
  Serial.print(systemStats.chargingTime / 3600000.0f, 1);
  Serial.println(" hodín");
  
  Serial.print("Celková energia: ");
  Serial.print(systemStats.totalEnergyCharged, 1);
  Serial.println(" Wh");
  
  Serial.print("Počet chýb: ");
  Serial.println(systemStats.errorCount);
  
  Serial.print("Počet zmien stavu: ");
  Serial.println(systemStats.stateChangeCount);
  
  Serial.print("Aktuálny stav: ");
  Serial.println(getStateName(currentSystemState));
  
  Serial.print("Doba v stave: ");
  Serial.print((millis() - stateEntryTime) / 1000.0f, 0);
  Serial.println(" sekúnd");
  
  Serial.println("========================");
}

void saveSystemStats() {
  // Uloženie štatistík do EEPROM
  EEPROM.put(STATS_STORAGE_ADDRESS, systemStats);
  EEPROM.commit();
}

void loadSystemStats() {
  // Načítanie štatistík z EEPROM
  EEPROM.get(STATS_STORAGE_ADDRESS, systemStats);
}

// ============================================
// KONFIGURÁCIA SYSTÉMU
// ============================================

void enterConfigurationMode() {
  Serial.println("Vstup do konfiguračného režimu");
  
  // Zastavenie aktívnych procesov
  setPWM(0.0f);
  
  // Zobrazenie konfiguračného menu
  displayConfigMenu();
}

void handleConfiguration() {
  // Spracovanie konfiguračných príkazov
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    processConfigCommand(command);
  }
  
  // Spracovanie tlačidiel
  handleConfigButtons();
}

void processConfigCommand(String command) {
  command.trim();
  
  if (command == "exit") {
    configCompleted = true;
    Serial.println("Ukončenie konfigurácie");
  } else if (command.startsWith("battery")) {
    // Zmena typu batérie
    int type = command.substring(8).toInt();
    if (type >= 0 && type <= 3) {
      setBatteryType((BatteryType)type);
      Serial.println("Typ batérie zmenený");
    }
  } else if (command.startsWith("current")) {
    // Zmena max prúdu
    float current = command.substring(8).toFloat();
    currentProfile->maxChargeCurrent = current;
    Serial.print("Max prúd nastavený na: ");
    Serial.print(current);
    Serial.println("A");
  }
  // Ďalšie príkazy...
}

// ============================================
// DISPLEJOVÉ FUNKCIE
// ============================================

void updateDisplayState() {
  // Aktualizácia displeja podľa stavu
  switch(currentSystemState) {
    case STATE_INIT:
      displayInitScreen();
      break;
    case STATE_SELFTEST:
      displayTestScreen();
      break;
    case STATE_IDLE:
      updateDisplayIdle();
      break;
    case STATE_CHARGING:
      updateDisplayCharging();
      break;
    case STATE_FAULT:
      displayFaultScreen();
      break;
    case STATE_SLEEP:
      displaySleepScreen();
      break;
    case STATE_CONFIG:
      displayConfigScreen();
      break;
  }
}

void updateDisplayIdle() {
  // Zobrazenie základných informácií v idle režime
  char buffer[64];
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // Nadpis
  u8g2.drawStr(0, 10, "MPPT REGULATOR");
  u8g2.drawStr(0, 20, "Stav: NECINNY");
  
  // Údaje
  sprintf(buffer, "Bat: %.1fV", batteryVoltage);
  u8g2.drawStr(0, 35, buffer);
  
  sprintf(buffer, "Panel: %.1fV", panelVoltage);
  u8g2.drawStr(0, 45, buffer);
  
  sprintf(buffer, "Teplota: %.1fC", temperature);
  u8g2.drawStr(0, 55, buffer);
  
  u8g2.sendBuffer();
}

void updateDisplayCharging() {
  // Zobrazenie nabíjania
  char buffer[64];
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // Nadpis
  u8g2.drawStr(0, 10, "NABIJANIE");
  
  // Fáza nabíjania
  sprintf(buffer, "Faza: %s", getPhaseName(currentChargePhase).c_str());
  u8g2.drawStr(0, 20, buffer);
  
  // Údaje
  sprintf(buffer, "Bat: %.1fV %.1fA", batteryVoltage, batteryCurrent);
  u8g2.drawStr(0, 35, buffer);
  
  sprintf(buffer, "Panel: %.1fV %.1fW", panelVoltage, panelPower);
  u8g2.drawStr(0, 45, buffer);
  
  sprintf(buffer, "Ucinnost: %.1f%%", efficiency);
  u8g2.drawStr(0, 55, buffer);
  
  sprintf(buffer, "PWM: %.1f%%", pwmDutyCycle);
  u8g2.drawStr(0, 65, buffer);
  
  u8g2.sendBuffer();
}

void displayFaultMessage() {
  // Zobrazenie chybového hlásenia
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  u8g2.drawStr(0, 10, "CHYBOVY STAV!");
  u8g2.drawStr(0, 25, "Skontrolujte:");
  u8g2.drawStr(0, 35, "- Napätie batérie");
  u8g2.drawStr(0, 45, "- Teplotu");
  u8g2.drawStr(0, 55, "- Spojenie");
  u8g2.drawStr(0, 65, "Reset: 3s stlac");
  
  u8g2.sendBuffer();
}