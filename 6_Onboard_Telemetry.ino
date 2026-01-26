// ============================================
// PALUBNÁ TELEMETRIA A ULOŽENIE DÁT - MODUL 6
// ============================================

#include "config.h"
#include "globals.h"

// ============================================
// LOKÁLNE PREMENNÉ MODULU TELEMETRIE
// ============================================

// Časové značky pre periodické úlohy
unsigned long lastFastTelemetry = 0;    // Čas poslednej rýchlej telemetrie
unsigned long lastSlowTelemetry = 0;    // Čas poslednej pomaly telemetrie
unsigned long lastStatsTelemetry = 0;   // Čas posledných štatistík
unsigned long lastHistorySave = 0;      // Čas posledného uloženia histórie

// Indikátory zmien údajov
bool dataChanged = false;               // Zmenili sa telemetrické údaje
bool statsChanged = false;              // Zmenili sa štatistiky

// Mesačné štatistiky (31 dní)
DailyStats monthlyStats[31];

// ============================================
// INICIALIZÁCIA TELEMETRIE - FUNKCIA
// ============================================

void initTelemetry() {
  Serial.println("Inicializácia palubnej telemetrie...");
  
  // Vyčistenie kruhového buffera pre históriu
  clearTelemetryHistory();
  
  // Načítanie denných štatistík z EEPROM
  loadDailyStats();
  
  // Načítanie mesačných štatistík z EEPROM
  loadMonthlyStats();
  
  // Resetovanie denných štatistík pre nový deň
  resetDailyStats();
  
  Serial.println("Telemetria úspešne inicializovaná");
  Serial.print("Veľkosť histórie: ");
  Serial.print(HISTORY_SIZE);
  Serial.println(" záznamov");
}

// ============================================
// ZBER TELEMETRICKÝCH DÁT - HLAVNÁ FUNKCIA
// ============================================

void collectTelemetryData() {
  // Táto funkcia zhromažďuje všetky telemetrické údaje do jedného záznamu
  
  // VYTVORENIE NOVÉHO TELEMETRICKÉHO ZÁZNAMU
  TelemetryData data;
  
  // ZÁKLADNÉ SENZORICKÉ ÚDAJE
  data.timestamp = millis();               // Aktuálny čas v milisekundách
  data.panelVoltage = panelVoltage;        // Napätie solárneho panelu (V)
  data.panelCurrent = panelCurrent;        // Prúd zo solárneho panelu (A)
  data.panelPower = panelPower;            // Výkon solárneho panelu (W)
  data.batteryVoltage = batteryVoltage;    // Napätie batérie (V)
  data.batteryCurrent = batteryCurrent;    // Prúd do batérie (A)
  data.batteryPower = batteryPower;        // Výkon do batérie (W)
  data.efficiency = efficiency;            // Účinnosť MPPT regulátora (%)
  data.temperature = temperature;          // Teplota systému (°C)
  data.pwmDuty = pwmDutyCycle;             // Aktuálna střída PWM (%)
  
  // SYSTÉMOVÉ STAVY
  data.chargePhase = currentChargePhase;   // Aktuálna fáza nabíjania
  data.systemState = currentSystemState;   // Stav systému
  
  // CHYBOVÉ VLAJKY (bitové pole pre úsporu miesta)
  data.errorFlags = 0;
  if (overVoltageFlag) data.errorFlags |= 0x01;     // Bit 0: Prekročenie napätia
  if (underVoltageFlag) data.errorFlags |= 0x02;    // Bit 1: Podkročenie napätia
  if (overTempFlag) data.errorFlags |= 0x04;        // Bit 2: Prehriatie
  if (shortCircuitFlag) data.errorFlags |= 0x08;    // Bit 3: Zkrat
  
  // ULOŽENIE ZÁZNAMU DO HISTÓRIE
  addToHistory(data);
  
  // AKTUALIZÁCIA DENNÝCH ŠTATISTÍK
  updateDailyStats(data);
  
  // NASTAVENIE INDIKÁTORA ZMIEN
  dataChanged = true;
  
  // DEBUG: Vypísanie záznamu (voliteľné)
  /*
  Serial.print("Telemetria: ");
  Serial.print(data.panelPower, 1);
  Serial.print("W -> ");
  Serial.print(data.batteryPower, 1);
  Serial.print("W (");
  Serial.print(data.efficiency, 1);
  Serial.println("%)");
  */
}

// ============================================
// SPRÁVA HISTÓRIE DÁT - FUNKCIE
// ============================================

void addToHistory(TelemetryData data) {
  // Táto funkcia ukladá telemetrické údaje do kruhového buffera
  
  // ULOŽENIE DÁT NA AKTUÁLNU POZÍCIU
  telemetryHistory[historyIndex] = data;
  
  // POSUN INDEXU V KRUHOVOM BUFERI
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  
  // AKTUALIZÁCIA POČITADLA ZÁZNAMOV
  if (historyCount < HISTORY_SIZE) {
    historyCount++;
  }
  
  // DEBUG: Vypísanie stavu histórie (voliteľné)
  /*
  static int lastReport = 0;
  if (historyIndex % 100 == 0 && historyIndex != lastReport) {
    Serial.print("História: ");
    Serial.print(historyCount);
    Serial.print("/");
    Serial.print(HISTORY_SIZE);
    Serial.println(" záznamov");
    lastReport = historyIndex;
  }
  */
}

void clearTelemetryHistory() {
  // Táto funkcia úplne vyčistí históriu telemetrie
  
  historyIndex = 0;
  historyCount = 0;
  Serial.println("História telemetrie bola vyčistená");
}

TelemetryData* getHistoryEntry(int index) {
  // Táto funkcia vráti konkrétny záznam z histórie
  
  // KONTROLA PLATNÉHO INDEXU
  if (index < 0 || index >= historyCount) {
    Serial.print("CHYBA: Neplatný index histórie: ");
    Serial.println(index);
    return nullptr;
  }
  
  // VÝPOČET SKUTOČNÉHO INDEXU V KRUHOVOM BUFERI
  int actualIndex = (historyIndex - historyCount + index + HISTORY_SIZE) % HISTORY_SIZE;
  return &telemetryHistory[actualIndex];
}

int getHistoryCount() {
  // Vráti aktuálny počet záznamov v histórii
  return historyCount;
}

// ============================================
// AKTUALIZÁCIA DENNÝCH ŠTATISTÍK - FUNKCIA
// ============================================

void updateDailyStats(TelemetryData data) {
  // Táto funkcia aktualizuje denné štatistiky
  
  // ZÍSKANIE AKTUÁLNEHO DÁTU (RRRRMMDD)
  unsigned long today = getTodayDate();
  
  // AK JE NOVÝ DEŇ, UKLADAJ STARÉ A ZAČNI NOVÉ ŠTATISTIKY
  if (dailyStats.date != today) {
    Serial.println("Nový deň - ukladám štatistiky a začínam nové");
    saveDailyStats();      // Uloženie starých štatistík
    resetDailyStats();     // Resetovanie pre nový deň
    dailyStats.date = today; // Nastavenie nového dátumu
  }
  
  // AKTUALIZÁCIA MAXIMÁLNEHO VÝKONU
  if (data.panelPower > dailyStats.maxPower) {
    dailyStats.maxPower = data.panelPower;
  }
  
  // VÝPOČET ENERGIE (Wh za sekundu)
  float energyThisSecond = data.batteryPower / 3600.0f;
  dailyStats.totalEnergy += energyThisSecond;
  
  // VÝPOČET PRIEMERNEJ ÚČINNOSTI (vážený priemer)
  if (data.panelPower > 0.1f) {
    int count = dailyStats.chargeCycles + 1;
    dailyStats.avgEfficiency = (dailyStats.avgEfficiency * dailyStats.chargeCycles + data.efficiency) / count;
  }
  
  // AKTUALIZÁCIA NAPÄTIA BATÉRIE
  if (data.batteryVoltage < dailyStats.minBatteryVoltage) {
    dailyStats.minBatteryVoltage = data.batteryVoltage;
  }
  if (data.batteryVoltage > dailyStats.maxBatteryVoltage) {
    dailyStats.maxBatteryVoltage = data.batteryVoltage;
  }
  
  // POČÍTANIE NABÍJACÍCH CYKLOV
  static ChargePhase lastPhase = PHASE_OFF;
  if (lastPhase == PHASE_OFF && data.chargePhase == PHASE_BULK) {
    dailyStats.chargeCycles++;
    Serial.print("Nový nabíjací cyklus: ");
    Serial.println(dailyStats.chargeCycles);
  }
  lastPhase = data.chargePhase;
  
  // NASTAVENIE INDIKÁTORA ZMIEN
  statsChanged = true;
}

// ============================================
// SPRACOVANIE TELEMETRIE - HLAVNÁ FUNKCIA
// ============================================

void processTelemetry() {
  // Táto funkcia riadi všetky telemetrické procesy
  
  unsigned long currentTime = millis();
  
  // 1. RÝCHLA TELEMETRIA (každú sekundu)
  if (currentTime - lastFastTelemetry >= TELEMETRY_INTERVAL_FAST) {
    collectTelemetryData();   // Zber údajov
    sendFastTelemetry();      // Odoslanie rýchlych údajov
    lastFastTelemetry = currentTime;
  }
  
  // 2. POMALÁ TELEMETRIA (každých 5 sekúnd)
  if (currentTime - lastSlowTelemetry >= TELEMETRY_INTERVAL_SLOW) {
    sendSlowTelemetry();      // Odoslanie pomalých údajov
    lastSlowTelemetry = currentTime;
  }
  
  // 3. ŠTATISTIKY (každú minútu)
  if (currentTime - lastStatsTelemetry >= TELEMETRY_INTERVAL_STATS) {
    sendStatistics();         // Odoslanie štatistík
    lastStatsTelemetry = currentTime;
  }
  
  // 4. PERIODICKÉ UKLADANIE (každých 30 sekúnd)
  if (currentTime - lastHistorySave >= 30000) {
    if (dataChanged) {
      saveTelemetryHistory();  // Uloženie histórie
      dataChanged = false;
    }
    if (statsChanged) {
      saveDailyStats();        // Uloženie štatistik
      statsChanged = false;
    }
    lastHistorySave = currentTime;
  }
}

// ============================================
// POMOCNÉ TELEMETRICKÉ FUNKCIE
// ============================================

unsigned long getTodayDate() {
  // Získanie dnešného dátumu vo formáte RRRRMMDD
  
  // Poznámka: Toto je zjednodušená implementácia
  // V reálnom projekte by sa použil RTC modul
  
  unsigned long seconds = millis() / 1000;
  unsigned long days = seconds / 86400;  // Počet dní od štartu
  
  // Začneme od 1. januára 2024
  return 20240101 + days;
}

void saveTelemetryHistory() {
  // Uloženie histórie telemetrie do EEPROM
  
  Serial.print("Ukladám históriu telemetrie... ");
  
  int startAddress = TELEMETRY_HISTORY_START;
  int entrySize = sizeof(TelemetryData);
  int maxEntries = (DAILY_STATS_ADDRESS - TELEMETRY_HISTORY_START) / entrySize;
  
  int entriesToSave = min(historyCount, maxEntries);
  
  for (int i = 0; i < entriesToSave; i++) {
    TelemetryData* data = getHistoryEntry(i);
    if (data != nullptr) {
      EEPROM.put(startAddress + i * entrySize, *data);
    }
  }
  
  EEPROM.commit();
  Serial.print(entriesToSave);
  Serial.println(" záznamov uložených");
}

void loadTelemetryHistory() {
  // Načítanie histórie telemetrie z EEPROM
  
  Serial.print("Načítavam históriu telemetrie... ");
  
  int startAddress = TELEMETRY_HISTORY_START;
  int entrySize = sizeof(TelemetryData);
  
  TelemetryData data;
  int i = 0;
  
  while (i < 1000) {  // Maximálny počet záznamov
    EEPROM.get(startAddress + i * entrySize, data);
    
    // Kontrola platného záznamu (timestamp > 0)
    if (data.timestamp == 0) {
      break;
    }
    
    addToHistory(data);
    i++;
  }
  
  Serial.print(i);
  Serial.println(" záznamov načítaných");
}

void saveDailyStats() {
  // Uloženie denných štatistík do EEPROM
  
  EEPROM.put(DAILY_STATS_ADDRESS, dailyStats);
  EEPROM.commit();
  
  Serial.print("Denné štatistiky uložené (");
  Serial.print(dailyStats.date);
  Serial.println(")");
}

void loadDailyStats() {
  // Načítanie denných štatistík z EEPROM
  
  EEPROM.get(DAILY_STATS_ADDRESS, dailyStats);
  
  if (dailyStats.date == 0) {
    // Žiadne uložené štatistiky - resetovať
    resetDailyStats();
    Serial.println("Žiadne denné štatistiky - resetujem");
  } else {
    Serial.print("Denné štatistiky načítané pre dátum: ");
    Serial.println(dailyStats.date);
  }
}

void resetDailyStats() {
  // Resetovanie denných štatistík
  
  dailyStats.date = getTodayDate();
  dailyStats.maxPower = 0.0f;
  dailyStats.totalEnergy = 0.0f;
  dailyStats.avgEfficiency = 0.0f;
  dailyStats.chargeCycles = 0;
  dailyStats.minBatteryVoltage = 100.0f;  // Veľmi vysoká hodnota
  dailyStats.maxBatteryVoltage = 0.0f;    // Veľmi nízka hodnota
  
  Serial.println("Denné štatistiky resetované");
}

void saveMonthlyStats() {
  // Uloženie mesačných štatistík do EEPROM
  
  int address = MONTHLY_STATS_ADDRESS;
  for (int i = 0; i < 31; i++) {
    EEPROM.put(address + i * sizeof(DailyStats), monthlyStats[i]);
  }
  EEPROM.commit();
  
  Serial.println("Mesačné štatistiky uložené");
}

void loadMonthlyStats() {
  // Načítanie mesačných štatistík z EEPROM
  
  int address = MONTHLY_STATS_ADDRESS;
  for (int i = 0; i < 31; i++) {
    EEPROM.get(address + i * sizeof(DailyStats), monthlyStats[i]);
  }
  
  Serial.println("Mesačné štatistiky načítané");
}

void checkMemoryUsage() {
  // Kontrola využitia pamäte
  
  int freeHeap = ESP.getFreeHeap();
  int minFreeHeap = ESP.getMinFreeHeap();
  
  Serial.print("Voľná pamäť: ");
  Serial.print(freeHeap);
  Serial.print(" B, Min voľná: ");
  Serial.print(minFreeHeap);
  Serial.println(" B");
  
  if (freeHeap < 10000) {  // Menej ako 10KB
    Serial.println("VAROVANIE: Málo voľnej pamäte!");
    cleanupTelemetry();
  }
}

void cleanupTelemetry() {
  // Vyčistenie starých telemetrických dát
  
  Serial.println("Čistím staré telemetrické dáta...");
  
  // Ponechať iba posledných 500 záznamov
  int entriesToKeep = 500;
  if (historyCount > entriesToKeep) {
    // Presunúť novšie záznamy na začiatok
    for (int i = 0; i < entriesToKeep; i++) {
      TelemetryData* data = getHistoryEntry(historyCount - entriesToKeep + i);
      if (data != nullptr) {
        telemetryHistory[i] = *data;
      }
    }
    
    historyCount = entriesToKeep;
    historyIndex = historyCount % 1000;
    
    Serial.print("História vyčistená, ponechaných ");
    Serial.print(historyCount);
    Serial.println(" záznamov");
  }
}

void archiveOldData() {
  // Archivácia dát starších ako 7 dní
  
  Serial.println("Kontrolujem staré dáta na archiváciu...");
  
  unsigned long sevenDaysAgo = getTodayDate() - 7;
  int archived = 0;
  
  for (int i = 0; i < historyCount; i++) {
    TelemetryData* data = getHistoryEntry(i);
    if (data != nullptr) {
      // Zjednodušené: dátum z timestampu
      unsigned long recordDate = data->timestamp / 86400000;
      if (recordDate < sevenDaysAgo) {
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

// ============================================
// KOMUNIKAČNÉ FUNKCIE
// ============================================

void sendFastTelemetry() {
  // Odošle rýchle telemetrické údaje cez sériový port
  
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
  // Odošle pomalé telemetrické údaje
  
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
  // Odošle štatistické údaje
  
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
// DIAGNOSTICKÉ A REPORT FUNKCIE
// ============================================

void generateDailyReport() {
  // Generovanie denného reportu
  
  Serial.println("=== DENNÝ REPORT ===");
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
  // Generovanie mesačného reportu
  
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
  
  Serial.println("=== MESAČNÝ REPORT ===");
  Serial.print("Celková energia: "); Serial.print(totalEnergy, 1); Serial.println(" Wh");
  Serial.print("Maximálny výkon: "); Serial.print(maxPower, 1); Serial.println(" W");
  Serial.print("Celkový počet cyklov: "); Serial.println(totalCycles);
  Serial.println("=====================");
}

// ============================================
// KOMUNIKAČNÉ PRÍKAZY
// ============================================

void processTelemetryCommand(String command) {
  // Spracovanie príkazov pre telemetriu
  
  command.trim();
  
  if (command == "stav") {
    printTelemetryStatus();
  } else if (command == "report") {
    generateDailyReport();
  } else if (command == "mesacny") {
    generateMonthlyReport();
  } else if (command == "export") {
    exportTelemetryData();
  } else if (command == "vyčisti") {
    clearTelemetryHistory();
    resetDailyStats();
    Serial.println("Telemetria vyčistená");
  } else if (command.startsWith("get ")) {
    String param = command.substring(4);
    if (param == "historia") {
      Serial.print("Počet záznamov: ");
      Serial.println(historyCount);
    } else if (param == "statistiky") {
      printDailyStats();
    }
  } else {
    Serial.println("Neznámy telemetrický príkaz");
    Serial.println("Dostupné: stav, report, mesacny, export, vyčisti, get historia, get statistiky");
  }
}

void printTelemetryStatus() {
  // Výpis stavu telemetrie
  
  Serial.println("=== STATUS TELEMETRIE ===");
  Serial.print("Počet záznamov v histórii: "); Serial.println(historyCount);
  Serial.print("Index: "); Serial.println(historyIndex);
  Serial.print("Denné štatistiky aktualizované: ");
  Serial.println(statsChanged ? "ÁNO" : "NIE");
  Serial.print("Dáta zmenené: ");
  Serial.println(dataChanged ? "ÁNO" : "NIE");
  Serial.println("=========================");
}

void printDailyStats() {
  // Výpis denných štatistík
  
  Serial.print("Denné štatistiky pre ");
  Serial.println(dailyStats.date);
  Serial.print("Max výkon: ");
  Serial.print(dailyStats.maxPower, 1);
  Serial.println(" W");
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
// HLAVNÁ TELEMETRICKÁ FUNKCIA
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