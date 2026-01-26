// ============================================
// PALUBNÁ TELEMETRIA A ULOŽENIE DÁT
// ============================================

// ============================================
// KONFIGURÁCIA TELEMETRIE
// ============================================

// Intervaly odosielania dát
#define TELEMETRY_INTERVAL_FAST   1000   // Rýchla telemetria (ms)
#define TELEMETRY_INTERVAL_SLOW   5000   // Pomalá telemetria (ms)
#define TELEMETRY_INTERVAL_STATS  60000  // Štatistiky (ms)

// Veľkosť histórie
#define HISTORY_SIZE 1000  // Počet záznamov v histórii

// Štruktúra pre telemetrický záznam
struct TelemetryData {
  unsigned long timestamp;     // Časová značka
  float panelVoltage;          // Napätie panelu (V)
  float panelCurrent;          // Prúd panelu (A)
  float panelPower;           // Výkon panelu (W)
  float batteryVoltage;       // Napätie batérie (V)
  float batteryCurrent;       // Prúd batérie (A)
  float batteryPower;         // Výkon batérie (W)
  float efficiency;           // Účinnosť (%)
  float temperature;          // Teplota (°C)
  float pwmDuty;              // PWM střída (%)
  ChargePhase chargePhase;    // Fáza nabíjania
  SystemState systemState;    // Stav systému
  byte errorFlags;            // Chybové vlajky
};

// Štruktúra pre denné štatistiky
struct DailyStats {
  unsigned long date;          // Dátum (YYYYMMDD)
  float maxPower;              // Maximálny výkon (W)
  float totalEnergy;           // Celková energia (Wh)
  float avgEfficiency;         // Priemerná účinnosť (%)
  int chargeCycles;            // Počet nabíjacích cyklov
  float minBatteryVoltage;     // Minimálne napätie batérie
  float maxBatteryVoltage;     // Maximálne napätie batérie
};

// ============================================
// PREMENNÉ TELEMETRIE
// ============================================

// Kruhový buffer pre históriu
TelemetryData telemetryHistory[HISTORY_SIZE];
int historyIndex = 0;
int historyCount = 0;

// Denné štatistiky
DailyStats dailyStats;
DailyStats monthlyStats[31];  // Štatistiky za 31 dní

// Časové premenné
unsigned long lastFastTelemetry = 0;
unsigned long lastSlowTelemetry = 0;
unsigned long lastStatsTelemetry = 0;
unsigned long lastHistorySave = 0;

// Indikátory zmien
bool dataChanged = false;
bool statsChanged = false;

// ============================================
// INICIALIZÁCIA TELEMETRIE
// ============================================

void initTelemetry() {
  Serial.println("Inicializácia telemetrie...");
  
  // Vyčistenie histórie
  clearTelemetryHistory();
  
  // Načítanie uložených štatistík
  loadDailyStats();
  loadMonthlyStats();
  
  // Nastavenie počiatočných hodnôt
  resetDailyStats();
  
  Serial.println("Telemetria inicializovaná");
}

// ============================================
// ZBER TELEMETRICKÝCH DÁT
// ============================================

void collectTelemetryData() {
  // Vytvorenie nového záznamu
  TelemetryData data;
  
  // Základné údaje
  data.timestamp = millis();
  data.panelVoltage = panelVoltage;
  data.panelCurrent = panelCurrent;
  data.panelPower = panelPower;
  data.batteryVoltage = batteryVoltage;
  data.batteryCurrent = batteryCurrent;
  data.batteryPower = batteryPower;
  data.efficiency = efficiency;
  data.temperature = temperature;
  data.pwmDuty = pwmDutyCycle;
  data.chargePhase = currentChargePhase;
  data.systemState = currentSystemState;
  
  // Chybové vlajky
  data.errorFlags = 0;
  if (overVoltageFlag) data.errorFlags |= 0x01;
  if (underVoltageFlag) data.errorFlags |= 0x02;
  if (overTempFlag) data.errorFlags |= 0x04;
  if (shortCircuitFlag) data.errorFlags |= 0x08;
  
  // Uloženie do histórie
  addToHistory(data);
  
  // Aktualizácia štatistík
  updateDailyStats(data);
  
  // Nastavenie indikátora zmien
  dataChanged = true;
}

// ============================================
// SPRÁVA HISTÓRIE DÁT
// ============================================

void addToHistory(TelemetryData data) {
  // Uloženie dát do kruhového buffera
  telemetryHistory[historyIndex] = data;
  
  // Posun indexu
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  
  // Počítadlo záznamov
  if (historyCount < HISTORY_SIZE) {
    historyCount++;
  }
}

void clearTelemetryHistory() {
  historyIndex = 0;
  historyCount = 0;
  Serial.println("História telemetrie vyčistená");
}

TelemetryData* getHistoryEntry(int index) {
  if (index < 0 || index >= historyCount) {
    return nullptr;
  }
  
  // Výpočet skutočného indexu v kruhovom bufferi
  int actualIndex = (historyIndex - historyCount + index + HISTORY_SIZE) % HISTORY_SIZE;
  return &telemetryHistory[actualIndex];
}

int getHistoryCount() {
  return historyCount;
}

// ============================================
// AKTUALIZÁCIA DENNÝCH ŠTATISTÍK
// ============================================

void updateDailyStats(TelemetryData data) {
  // Dátum (YYYYMMDD)
  unsigned long today = getTodayDate();
  
  // Ak je nový deň, uložiť staré a začať nové
  if (dailyStats.date != today) {
    saveDailyStats();
    resetDailyStats();
    dailyStats.date = today;
  }
  
  // Aktualizácia štatistík
  if (data.panelPower > dailyStats.maxPower) {
    dailyStats.maxPower = data.panelPower;
  }
  
  // Výpočet energie (Wh za sekundu)
  float energyThisSecond = data.batteryPower / 3600.0f;
  dailyStats.totalEnergy += energyThisSecond;
  
  // Priemerná účinnosť
  if (data.panelPower > 0.1f) {
    int count = dailyStats.chargeCycles + 1;
    dailyStats.avgEfficiency = (dailyStats.avgEfficiency * dailyStats.chargeCycles + data.efficiency) / count;
  }
  
  // Napätie batérie
  if (data.batteryVoltage < dailyStats.minBatteryVoltage) {
    dailyStats.minBatteryVoltage = data.batteryVoltage;
  }
  if (data.batteryVoltage > dailyStats.maxBatteryVoltage) {
    dailyStats.maxBatteryVoltage = data.batteryVoltage;
  }
  
  // Počet nabíjacích cyklov
  static ChargePhase lastPhase = PHASE_OFF;
  if (lastPhase == PHASE_OFF && data.chargePhase == PHASE_BULK) {
    dailyStats.chargeCycles++;
  }
  lastPhase = data.chargePhase;
  
  statsChanged = true;
}

unsigned long getTodayDate() {
  // Zjednodušené získanie dátumu (YYYYMMDD)
  // V reálnom projekte by sa použil RTC modul
  unsigned long seconds = millis() / 1000;
  unsigned long days = seconds / 86400;
  return 20240101 + days;  // Začiatok od 1.1.2024
}

void resetDailyStats() {
  dailyStats.date = getTodayDate();
  dailyStats.maxPower = 0.0f;
  dailyStats.totalEnergy = 0.0f;
  dailyStats.avgEfficiency = 0.0f;
  dailyStats.chargeCycles = 0;
  dailyStats.minBatteryVoltage = 100.0f;  // Veľmi vysoké
  dailyStats.maxBatteryVoltage = 0.0f;    // Veľmi nízke
}

// ============================================
// SPRACOVANIE TELEMETRIE
// ============================================

void processTelemetry() {
  unsigned long currentTime = millis();
  
  // 1. Rýchla telemetria (každú sekundu)
  if (currentTime - lastFastTelemetry >= TELEMETRY_INTERVAL_FAST) {
    collectTelemetryData();
    sendFastTelemetry();
    lastFastTelemetry = currentTime;
  }
  
  // 2. Pomalá telemetria (každých 5 sekúnd)
  if (currentTime - lastSlowTelemetry >= TELEMETRY_INTERVAL_SLOW) {
    sendSlowTelemetry();
    lastSlowTelemetry = currentTime;
  }
  
  // 3. Štatistiky (každú minútu)
  if (currentTime - lastStatsTelemetry >= TELEMETRY_INTERVAL_STATS) {
    sendStatistics();
    lastStatsTelemetry = currentTime;
  }
  
  // 4. Periodické ukladanie (každých 30 sekúnd)
  if (currentTime - lastHistorySave >= 30000) {
    if (dataChanged) {
      saveTelemetryHistory();
      dataChanged = false;
    }
    if (statsChanged) {
      saveDailyStats();
      statsChanged = false;
    }
    lastHistorySave = currentTime;
  }
}

// ============================================
// ODOSIELANIE TELEMETRICKÝCH DÁT
// ============================================

void sendFastTelemetry() {
  // Základné údaje pre rýchly monitoring
  Serial.print("FAST,");
  Serial.print(millis()); Serial.print(",");
  Serial.print(panelVoltage, 2); Serial.print(",");
  Serial.print(panelCurrent, 3); Serial.print(",");
  Serial.print(panelPower, 2); Serial.print(",");
  Serial.print(batteryVoltage, 2); Serial.print(",");
  Serial.print(batteryCurrent, 3); Serial.print(",");
  Serial.print(efficiency, 1); Serial.print(",");
  Serial.print(pwmDutyCycle, 1); Serial.print(",");
  Serial.println(getPhaseName(currentChargePhase));
}

void sendSlowTelemetry() {
  // Podrobné údaje
  Serial.print("SLOW,");
  Serial.print(millis()); Serial.print(",");
  Serial.print(temperature, 1); Serial.print(",");
  Serial.print(getStateName(currentSystemState)); Serial.print(",");
  
  // Chybové stavy
  Serial.print(overVoltageFlag ? "1" : "0"); Serial.print(",");
  Serial.print(underVoltageFlag ? "1" : "0"); Serial.print(",");
  Serial.print(overTempFlag ? "1" : "0"); Serial.print(",");
  Serial.print(shortCircuitFlag ? "1" : "0"); Serial.print(",");
  
  // Nastavenia
  Serial.print(currentProfile->batteryName); Serial.print(",");
  Serial.print(currentProfile->maxChargeCurrent, 1); Serial.print(",");
  Serial.println(lowPowerMode ? "SOPORN" : "NORMAL");
}

void sendStatistics() {
  // Denné štatistiky
  Serial.print("STATS,");
  Serial.print(dailyStats.date); Serial.print(",");
  Serial.print(dailyStats.maxPower, 1); Serial.print(",");
  Serial.print(dailyStats.totalEnergy, 1); Serial.print(",");
  Serial.print(dailyStats.avgEfficiency, 1); Serial.print(",");
  Serial.print(dailyStats.chargeCycles); Serial.print(",");
  Serial.print(dailyStats.minBatteryVoltage, 2); Serial.print(",");
  Serial.println(dailyStats.maxBatteryVoltage, 2);
}

// ============================================
// ULOŽENIE A NAČÍTANIE DÁT
// ============================================

void saveTelemetryHistory() {
  // Uloženie histórie do EEPROM (ak je miesto)
  int startAddress = TELEMETRY_HISTORY_START;
  int entrySize = sizeof(TelemetryData);
  int maxEntries = EEPROM_SIZE / entrySize;
  
  int entriesToSave = min(historyCount, maxEntries);
  
  for (int i = 0; i < entriesToSave; i++) {
    TelemetryData* data = getHistoryEntry(i);
    if (data != nullptr) {
      EEPROM.put(startAddress + i * entrySize, *data);
    }
  }
  
  EEPROM.commit();
  Serial.print("Uložená história: ");
  Serial.print(entriesToSave);
  Serial.println(" záznamov");
}

void loadTelemetryHistory() {
  // Načítanie histórie z EEPROM
  int startAddress = TELEMETRY_HISTORY_START;
  int entrySize = sizeof(TelemetryData);
  
  TelemetryData data;
  int i = 0;
  
  while (EEPROM.get(startAddress + i * entrySize, data).timestamp != 0) {
    addToHistory(data);
    i++;
  }
  
  Serial.print("Načítaná história: ");
  Serial.print(i);
  Serial.println(" záznamov");
}

void saveDailyStats() {
  // Uloženie denných štatistík
  int address = DAILY_STATS_ADDRESS;
  EEPROM.put(address, dailyStats);
  EEPROM.commit();
  
  // Aktualizácia mesačných štatistík
  updateMonthlyStats();
}

void loadDailyStats() {
  // Načítanie denných štatistík
  int address = DAILY_STATS_ADDRESS;
  EEPROM.get(address, dailyStats);
  
  // Ak sú dáta prázdne, resetovať
  if (dailyStats.date == 0) {
    resetDailyStats();
  }
}

void updateMonthlyStats() {
  // Aktualizácia štatistík pre aktuálny mesiac
  int dayOfMonth = dailyStats.date % 100;
  if (dayOfMonth >= 1 && dayOfMonth <= 31) {
    monthlyStats[dayOfMonth - 1] = dailyStats;
    saveMonthlyStats();
  }
}

void saveMonthlyStats() {
  // Uloženie mesačných štatistík
  int address = MONTHLY_STATS_ADDRESS;
  for (int i = 0; i < 31; i++) {
    EEPROM.put(address + i * sizeof(DailyStats), monthlyStats[i]);
  }
  EEPROM.commit();
}

void loadMonthlyStats() {
  // Načítanie mesačných štatistík
  int address = MONTHLY_STATS_ADDRESS;
  for (int i = 0; i < 31; i++) {
    EEPROM.get(address + i * sizeof(DailyStats), monthlyStats[i]);
  }
}

// ============================================
// GENEROVANIE REPORTU
// ============================================

void generateDailyReport() {
  Serial.println("=== DENNY REPORT ===");
  Serial.print("Dátum: "); Serial.println(dailyStats.date);
  Serial.print("Maximálny výkon: "); Serial.print(dailyStats.maxPower, 1); Serial.println(" W");
  Serial.print("Celková energia: "); Serial.print(dailyStats.totalEnergy, 1); Serial.println(" Wh");
  Serial.print("Priemerná účinnosť: "); Serial.print(dailyStats.avgEfficiency, 1); Serial.println(" %");
  Serial.print("Počet cyklov: "); Serial.println(dailyStats.chargeCycles);
  Serial.print("Min napätie batérie: "); Serial.print(dailyStats.minBatteryVoltage, 2); Serial.println(" V");
  Serial.print("Max napätie batérie: "); Serial.print(dailyStats.maxBatteryVoltage, 2); Serial.println(" V");
  Serial.println("===================");
}

void generateMonthlyReport() {
  float totalEnergy = 0.0f;
  float maxPower = 0.0f;
  int totalCycles = 0;
  
  for (int i = 0; i < 31; i++) {
    if (monthlyStats[i].date != 0) {
      totalEnergy += monthlyStats[i].totalEnergy;
      if (monthlyStats[i].maxPower > maxPower) {
        maxPower = monthlyStats[i].maxPower;
      }
      totalCycles += monthlyStats[i].chargeCycles;
    }
  }
  
  Serial.println("=== MESACNY REPORT ===");
  Serial.print("Celková energia: "); Serial.print(totalEnergy, 1); Serial.println(" Wh");
  Serial.print("Maximálny výkon: "); Serial.print(maxPower, 1); Serial.println(" W");
  Serial.print("Celkový počet cyklov: "); Serial.println(totalCycles);
  Serial.println("=====================");
}

// ============================================
// DIAGNOSTICKÉ FUNKCIE
// ============================================

void printTelemetryStatus() {
  Serial.println("=== STATUS TELEMETRIE ===");
  Serial.print("Počet záznamov v histórii: "); Serial.println(historyCount);
  Serial.print("Index: "); Serial.println(historyIndex);
  Serial.print("Denné štatistiky aktualizované: ");
  Serial.println(statsChanged ? "ANO" : "NIE");
  Serial.print("Dáta zmenené: ");
  Serial.println(dataChanged ? "ANO" : "NIE");
  Serial.println("=========================");
}

void exportTelemetryData() {
  // Export všetkých dát do CSV formátu
  Serial.println("EXPORT_START");
  Serial.println("timestamp,panelV,panelI,panelP,batV,batI,batP,eff,temp,pwm,phase,state,errors");
  
  for (int i = 0; i < historyCount; i++) {
    TelemetryData* data = getHistoryEntry(i);
    if (data != nullptr) {
      Serial.print(data->timestamp); Serial.print(",");
      Serial.print(data->panelVoltage, 2); Serial.print(",");
      Serial.print(data->panelCurrent, 3); Serial.print(",");
      Serial.print(data->panelPower, 2); Serial.print(",");
      Serial.print(data->batteryVoltage, 2); Serial.print(",");
      Serial.print(data->batteryCurrent, 3); Serial.print(",");
      Serial.print(data->batteryPower, 2); Serial.print(",");
      Serial.print(data->efficiency, 1); Serial.print(",");
      Serial.print(data->temperature, 1); Serial.print(",");
      Serial.print(data->pwmDuty, 1); Serial.print(",");
      Serial.print(getPhaseName(data->chargePhase)); Serial.print(",");
      Serial.print(getStateName(data->systemState)); Serial.print(",");
      Serial.println(data->errorFlags, BIN);
    }
  }
  
  Serial.println("EXPORT_END");
}

// ============================================
// KOMUNIKAČNÉ FUNKCIE
// ============================================

void processTelemetryCommand(String command) {
  command.trim();
  
  if (command == "status") {
    printTelemetryStatus();
  } else if (command == "report") {
    generateDailyReport();
  } else if (command == "monthly") {
    generateMonthlyReport();
  } else if (command == "export") {
    exportTelemetryData();
  } else if (command == "clear") {
    clearTelemetryHistory();
    resetDailyStats();
    Serial.println("Telemetria vyčistená");
  } else if (command.startsWith("get ")) {
    // Získanie konkrétnych dát
    String param = command.substring(4);
    if (param == "history") {
      Serial.print("Počet záznamov: ");
      Serial.println(historyCount);
    } else if (param == "stats") {
      printDailyStats();
    }
  }
}

void printDailyStats() {
  Serial.print("Denné štatistiky pre ");
  Serial.println(dailyStats.date);
  Serial.print("Max výkon: ");
  Serial.print(dailyStats.maxPower, 1);
  Serial.println("W");
}

// ============================================
// AUTOMATICKÉ ÚLOHY TELEMETRIE
// ============================================

void runTelemetryTasks() {
  // Hlavná funkcia telemetrie
  processTelemetry();
  
  // Kontrola pamäte
  static unsigned long lastMemoryCheck = 0;
  if (millis() - lastMemoryCheck > 60000) {  // Každú minútu
    checkMemoryUsage();
    lastMemoryCheck = millis();
  }
  
  // Archivácia starých dát
  static unsigned long lastArchive = 0;
  if (millis() - lastArchive > 3600000) {  // Každú hodinu
    archiveOldData();
    lastArchive = millis();
  }
}

void checkMemoryUsage() {
  // Kontrola využitia pamäte
  int freeHeap = ESP.getFreeHeap();
  int minFreeHeap = ESP.getMinFreeHeap();
  
  if (freeHeap < 10000) {  // Menej ako 10KB voľnej pamäte
    Serial.println("VAROVANIE: Málo voľnej pamäte!");
    Serial.print("Voľná pamäť: ");
    Serial.print(freeHeap);
    Serial.println(" bajtov");
    
    // Vyčistenie nepotrebných dát
    cleanupTelemetry();
  }
}

void cleanupTelemetry() {
  // Vyčistenie starých dát
  int entriesToKeep = HISTORY_SIZE / 2;  // Ponechať polovicu
  if (historyCount > entriesToKeep) {
    // Presunúť novšiu polovicu na začiatok
    for (int i = 0; i < entriesToKeep; i++) {
      TelemetryData* data = getHistoryEntry(historyCount - entriesToKeep + i);
      if (data != nullptr) {
        telemetryHistory[i] = *data;
      }
    }
    
    historyCount = entriesToKeep;
    historyIndex = historyCount % HISTORY_SIZE;
    
    Serial.print("Telemetria vyčistená, ponechaných ");
    Serial.print(historyCount);
    Serial.println(" záznamov");
  }
}

void archiveOldData() {
  // Archivácia dát starších ako 7 dní
  unsigned long sevenDaysAgo = getTodayDate() - 7;
  int archived = 0;
  
  for (int i = 0; i < historyCount; i++) {
    TelemetryData* data = getHistoryEntry(i);
    if (data != nullptr) {
      unsigned long recordDate = data->timestamp / 86400000;  // Konverzia na dni
      if (recordDate < sevenDaysAgo) {
        // Označiť na vymazanie
        // V reálnom projekte by sa presunuli do sekundárneho úložiska
        archived++;
      }
    }
  }
  
  if (archived > 0) {
    Serial.print("Na archiváciu označených ");
    Serial.print(archived);
    Serial.println(" starých záznamov");
  }
}