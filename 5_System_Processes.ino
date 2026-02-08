/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                      5_SYSTEM_PROCESSES.INO                                       ║
  ║                      (Vylepšená verzia V1.2)                                      ║
  ║                                                                                   ║
  ║  Vylepšenia:                                                                      ║
  ║  - Vylepšená správa EEPROM pamäte                                                 ║
  ║  - Periodicke automatické ukladanie dát                                          ║
  ║  - Watchdog funkcie pre stabilitu                                                ║
  ║  - Diagnostické funkcie                                                          ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

//========================== EEPROM MEMORY MAP ==========================//
// Mapa pamäťových adries pre rôzne premenné
#define ADDR_BATTERY_TYPE           0
#define ADDR_VOLTAGE_INPUT_OFFSET   4
#define ADDR_VOLTAGE_OUTPUT_OFFSET  8
#define ADDR_CURRENT_INPUT_OFFSET   12
#define ADDR_TEMP_COEFFICIENT       16
#define ADDR_WH_ACCUMULATOR         20
#define ADDR_DAYS_RUNNING           24
#define ADDR_MPPT_MODE              28
#define ADDR_ENABLE_WIFI            32
#define ADDR_ENABLE_LCD             36
#define ADDR_ENABLE_FAN             40
#define ADDR_PWM_FREQUENCY          44
#define ADDR_CHECKSUM               48

//========================== FLASH MEMORY FUNCTIONS ==========================//

// Vypočítaj checksum pre overenie integrity dát
uint32_t calculateChecksum(){
  uint32_t checksum = 0;
  checksum += (uint32_t)(batteryType);
  checksum += (uint32_t)(voltageInputOffset * 1000);
  checksum += (uint32_t)(voltageOutputOffset * 1000);
  checksum += (uint32_t)(currentInputOffset * 1000);
  checksum += (uint32_t)(Wh * 100);
  return checksum;
}

// Ulož dáta do EEPROM
void saveToEEPROM(){
  Serial.println("> Saving data to EEPROM...");
  
  EEPROM.put(ADDR_BATTERY_TYPE, (int)batteryType);
  EEPROM.put(ADDR_VOLTAGE_INPUT_OFFSET, voltageInputOffset);
  EEPROM.put(ADDR_VOLTAGE_OUTPUT_OFFSET, voltageOutputOffset);
  EEPROM.put(ADDR_CURRENT_INPUT_OFFSET, currentInputOffset);
  EEPROM.put(ADDR_TEMP_COEFFICIENT, tempCoefficient);
  EEPROM.put(ADDR_WH_ACCUMULATOR, Wh);
  EEPROM.put(ADDR_DAYS_RUNNING, daysRunning);
  EEPROM.put(ADDR_MPPT_MODE, mpptMode);
  EEPROM.put(ADDR_ENABLE_WIFI, (int)enableWiFi);
  EEPROM.put(ADDR_ENABLE_LCD, (int)enableLCD);
  EEPROM.put(ADDR_ENABLE_FAN, (int)enableFan);
  EEPROM.put(ADDR_PWM_FREQUENCY, pwmFrequency);
  
  // Ulož checksum
  uint32_t checksum = calculateChecksum();
  EEPROM.put(ADDR_CHECKSUM, checksum);
  
  EEPROM.commit();
  Serial.println("> Data saved successfully");
}

// Načítaj dáta z EEPROM
void loadFromEEPROM(){
  Serial.println("> Loading data from EEPROM...");
  
  // Načítaj checksum
  uint32_t storedChecksum;
  EEPROM.get(ADDR_CHECKSUM, storedChecksum);
  
  // Načítaj dáta
  int tempBatteryType;
  EEPROM.get(ADDR_BATTERY_TYPE, tempBatteryType);
  EEPROM.get(ADDR_VOLTAGE_INPUT_OFFSET, voltageInputOffset);
  EEPROM.get(ADDR_VOLTAGE_OUTPUT_OFFSET, voltageOutputOffset);
  EEPROM.get(ADDR_CURRENT_INPUT_OFFSET, currentInputOffset);
  EEPROM.get(ADDR_TEMP_COEFFICIENT, tempCoefficient);
  EEPROM.get(ADDR_WH_ACCUMULATOR, Wh);
  EEPROM.get(ADDR_DAYS_RUNNING, daysRunning);
  EEPROM.get(ADDR_MPPT_MODE, mpptMode);
  
  int tempWiFi, tempLCD, tempFan;
  EEPROM.get(ADDR_ENABLE_WIFI, tempWiFi);
  EEPROM.get(ADDR_ENABLE_LCD, tempLCD);
  EEPROM.get(ADDR_ENABLE_FAN, tempFan);
  EEPROM.get(ADDR_PWM_FREQUENCY, pwmFrequency);
  
  // Over checksum
  uint32_t calculatedChecksum = calculateChecksum();
  
  if(storedChecksum == calculatedChecksum){
    // Checksum je OK, použi načítané dáta
    batteryType = (BatteryType)tempBatteryType;
    setBatteryProfile(batteryType);
    enableWiFi = (tempWiFi == 1);
    enableLCD = (tempLCD == 1);
    enableFan = (tempFan == 1);
    
    kWh = Wh / 1000.0;
    MWh = Wh / 1000000.0;
    
    Serial.println("> Data loaded and verified successfully");
    Serial.print("  Battery Type: ");
    Serial.println(getBatteryTypeString());
    Serial.print("  Energy Harvested: ");
    Serial.print(Wh, 2);
    Serial.println(" Wh");
  }
  else{
    Serial.println("> WARNING: EEPROM checksum mismatch!");
    Serial.println("> Using default values");
    
    // Nastav default hodnoty
    batteryType = BAT_AGM;
    setBatteryProfile(batteryType);
    voltageInputOffset = 0.0;
    voltageOutputOffset = 0.0;
    currentInputOffset = 0.0;
    Wh = 0.0;
    daysRunning = 0.0;
  }
}

// Inicializuj autoload z flash pamäte
void initializeFlashAutoload(){
  if(disableFlashAutoLoad == 0){
    loadFromEEPROM();
  }
  else{
    Serial.println("> Flash autoload disabled");
  }
}

//========================== PERIODIC SAVE ==========================//
unsigned long lastSaveTime = 0;
const unsigned long saveInterval = 3600000;  // Ulož každú hodinu (3600s)

void periodicSave(){
  if(millis() - lastSaveTime >= saveInterval){
    saveToEEPROM();
    lastSaveTime = millis();
  }
}

//========================== LED INDICATOR ==========================//
void updateLEDIndicator(){
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  unsigned long blinkRate = 1000;  // Default 1s
  
  // Rôzne rýchlosti blikania podľa stavu
  switch(chargeState){
    case CHARGE_OFF:
      blinkRate = 2000;  // Pomalé blikanie
      break;
    case CHARGE_BULK:
      blinkRate = 500;   // Rýchle blikanie
      break;
    case CHARGE_ABSORPTION:
      blinkRate = 1000;  // Stredné blikanie
      break;
    case CHARGE_FLOAT:
      digitalWrite(LED, HIGH);  // Stále svietenie
      return;
    case CHARGE_EQUALIZATION:
      blinkRate = 250;   // Veľmi rýchle blikanie
      break;
  }
  
  // Ak je chyba, blikaj veľmi rýchlo
  if(OOV || OUV || OOC || IOV || OTE){
    blinkRate = 100;
  }
  
  // Blikaj LED
  if(millis() - lastBlink >= blinkRate){
    ledState = !ledState;
    digitalWrite(LED, ledState);
    lastBlink = millis();
  }
}

//========================== BACKFLOW MOSFET CONTROL ==========================//
void updateBackflowMOSFET(){
  // Zapni backflow MOSFET len ak batéria poskytuje energiu systému
  if(inputSource == 2){  // Battery is power source
    digitalWrite(backflow_MOSFET, LOW);  // Disable backflow prevention
  }
  else{
    digitalWrite(backflow_MOSFET, HIGH);  // Enable backflow prevention
  }
}

//========================== SYSTEM DIAGNOSTICS ==========================//
void printSystemDiagnostics(){
  Serial.println("\n========== SYSTEM DIAGNOSTICS ==========");
  Serial.print("Firmware: ");
  Serial.print(firmwareInfo);
  Serial.print(" (");
  Serial.print(firmwareDate);
  Serial.println(")");
  
  Serial.println("\n--- POWER ---");
  Serial.print("Input: ");
  Serial.print(voltageInput, 2);
  Serial.print("V, ");
  Serial.print(currentInput, 2);
  Serial.print("A, ");
  Serial.print(powerInput, 2);
  Serial.println("W");
  
  Serial.print("Output: ");
  Serial.print(voltageOutput, 2);
  Serial.print("V, ");
  Serial.print(currentOutput, 2);
  Serial.print("A, ");
  Serial.print(powerOutput, 2);
  Serial.println("W");
  
  Serial.println("\n--- BATTERY ---");
  Serial.print("Type: ");
  Serial.println(getBatteryTypeString());
  Serial.print("Charge State: ");
  Serial.println(getChargeStateString());
  Serial.print("SOC: ");
  Serial.print(batteryPercent);
  Serial.println("%");
  
  Serial.println("\n--- MPPT ---");
  Serial.print("PWM: ");
  Serial.print(PWM);
  Serial.print("/");
  Serial.print(pwmMaxLimited);
  Serial.print(" (");
  Serial.print((PWM * 100.0) / pwmMaxLimited, 1);
  Serial.println("%)");
  Serial.print("Efficiency: ");
  Serial.print(buckEfficiency, 1);
  Serial.println("%");
  
  Serial.println("\n--- ENVIRONMENT ---");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("Fan: ");
  Serial.println(fanStatus ? "ON" : "OFF");
  
  Serial.println("\n--- ENERGY ---");
  Serial.print("Harvested: ");
  if(Wh < 1000){
    Serial.print(Wh, 2);
    Serial.println(" Wh");
  }
  else if(kWh < 1000){
    Serial.print(kWh, 3);
    Serial.println(" kWh");
  }
  else{
    Serial.print(MWh, 3);
    Serial.println(" MWh");
  }
  Serial.print("Savings: ");
  Serial.print(energySavings, 2);
  Serial.println(" €");
  Serial.print("Days Running: ");
  Serial.println(daysRunning, 2);
  
  Serial.println("\n--- PROTECTION ---");
  Serial.print("Status: ");
  Serial.println(getProtectionStatus());
  Serial.print("Errors: OOV=");
  Serial.print(OOV);
  Serial.print(" OUV=");
  Serial.print(OUV);
  Serial.print(" OOC=");
  Serial.print(OOC);
  Serial.print(" IOV=");
  Serial.print(IOV);
  Serial.print(" IUV=");
  Serial.print(IUV);
  Serial.print(" OTE=");
  Serial.println(OTE);
  
  Serial.println("========================================\n");
}

//========================== WATCHDOG FUNCTIONS ==========================//
unsigned long lastWatchdogReset = 0;
const unsigned long watchdogTimeout = 60000;  // 60s watchdog timeout

void resetWatchdog(){
  lastWatchdogReset = millis();
}

void checkWatchdog(){
  // Ak hlavná slučka neresetuje watchdog, môže byť problém
  if(millis() - lastWatchdogReset > watchdogTimeout){
    Serial.println("> WARNING: Watchdog timeout! System may be hung.");
    Serial.println("> Attempting recovery...");
    
    // Pokus o obnovu
    buck_Disable();
    PWM = 0;
    ledcWrite(buck_IN, PWM);
    
    // Reset watchdog
    resetWatchdog();
  }
}

//========================== MAIN SYSTEM PROCESSES ==========================//
void System_Processes(){
  // Update LED indicator
  updateLEDIndicator();
  
  // Update backflow MOSFET
  updateBackflowMOSFET();
  
  // Periodic save to EEPROM
  periodicSave();
  
  // Reset watchdog
  resetWatchdog();
  
  // Check watchdog (kontrola stability systému)
  checkWatchdog();
  
  // Periodická diagnostika (každých 60 sekúnd)
  static unsigned long lastDiagnostics = 0;
  if(millis() - lastDiagnostics >= 60000){
    printSystemDiagnostics();
    lastDiagnostics = millis();
  }
}

//========================== EMERGENCY SHUTDOWN ==========================//
void emergencyShutdown(String reason){
  Serial.println("\n!!! EMERGENCY SHUTDOWN !!!");
  Serial.print("Reason: ");
  Serial.println(reason);
  
  // Vypni všetko
  buck_Disable();
  PWM = 0;
  ledcWrite(buck_IN, PWM);
  digitalWrite(FAN, HIGH);  // Zapni fan
  
  // Ulož dáta
  saveToEEPROM();
  
  // Blikaj LED rýchlo
  while(1){
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
  }
}

//========================== FACTORY RESET ==========================//
void factoryReset(){
  Serial.println("> Performing factory reset...");
  
  // Reset všetkých premenných
  batteryType = BAT_AGM;
  setBatteryProfile(batteryType);
  voltageInputOffset = 0.0;
  voltageOutputOffset = 0.0;
  currentInputOffset = 0.0;
  tempCoefficient = -0.018;
  Wh = 0.0;
  kWh = 0.0;
  MWh = 0.0;
  daysRunning = 0.0;
  energySavings = 0.0;
  mpptMode = 1;
  enableWiFi = 1;
  enableLCD = 1;
  enableFan = 1;
  
  // Ulož do EEPROM
  saveToEEPROM();
  
  Serial.println("> Factory reset complete");
  Serial.println("> System will restart in 3 seconds...");
  delay(3000);
  
  // Restart ESP32
  ESP.restart();
}
