// ============================================
// HLAVNÝ KÓD MPPT REGULÁTORA - KOMPLETNÝ SYSTÉM
// Verzia 1.1 - Všetky moduly integrované
// ============================================

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <EEPROM.h>
#include "config.h"
#include "globals.h"

// ============================================
// GLOBÁLNE OBJEKTY KNIŽNÍC
// ============================================

Adafruit_ADS1115 ads;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ============================================
// DEKLARÁCIE FUNKCIÍ Z MODULOV
// ============================================

// MODUL 2: Senzory
void initSensors();
void readAllSensors();
void calibrateSensors();
void printSensorValues();
void testSensors();
void testADC();

// MODUL 3: Ochranný systém
void runProtectionChecks();
void emergencyShutdown(const char* reason);
void logWarning(const char* message);
void printProtectionStatus();

// MODUL 4: Nabíjanie
void initChargingSystem();
void runChargingAlgorithm();
void setBatteryType(BatteryType type);
void setManualPWM(float dutyPercent);
String getPhaseName(ChargePhase phase);
String getStateName(SystemState state);
void printChargingStatus();

// MODUL 5: Systémové procesy
void initSystemProcesses();
void runSystemProcesses();
void changeSystemState(SystemState newState);

// MODUL 6: Telemetria
void initTelemetry();
void runTelemetryTasks();
void generateDailyReport();
void processTelemetryCommand(String command);

// MODUL 7: Bezdrôtová komunikácia
void initWirelessTelemetry();
void runWirelessTelemetry();
void handleWebServer();

// MODUL 8: Displej
void initLCDMenu();
void runLCDMenuSystem();
void updateDisplay();
String getMenuName(MenuState menu);

// Testovacie funkcie
void testPWMSequence();

// ============================================
// SETUP FUNKCIA
// ============================================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n========================================");
  Serial.println("   MPPT SOLÁRNY REGULÁTOR - ŠTART");
  Serial.println("========================================");
  
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("EEPROM inicializovaná");
  
  initGPIO();
  initPWM();
  initSensors();
  initSystemProcesses();
  initChargingSystem();
  initTelemetry();
  initLCDMenu();
  initWirelessTelemetry();
  performStartupTests();
  
  Serial.println("========================================");
  Serial.println("   SYSTÉM PRIPRAVENÝ");
  Serial.println("========================================\n");
  
  displayWelcomeMessage();
}

// ============================================
// POMOCNÉ SETUP FUNKCIE
// ============================================

void initGPIO() {
  Serial.println("Inicializácia GPIO pinov...");
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(ERROR_LED_PIN, OUTPUT);
  pinMode(BATTERY_DISCONNECT_PIN, OUTPUT);
  pinMode(PANEL_DISCONNECT_PIN, OUTPUT);
  
  digitalWrite(LED_PIN, LOW);
  digitalWrite(ERROR_LED_PIN, LOW);
  digitalWrite(BATTERY_DISCONNECT_PIN, HIGH);
  digitalWrite(PANEL_DISCONNECT_PIN, HIGH);
  
  Serial.println("GPIO inicializované");
}

void initPWM() {
  Serial.println("Inicializácia PWM...");
  
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);
  
  Serial.print("PWM: ");
  Serial.print(PWM_FREQUENCY);
  Serial.print("Hz, ");
  Serial.print(PWM_RESOLUTION);
  Serial.println("bit");
}

void performStartupTests() {
  Serial.println("Štartové testy...");
  
  // Test LED
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  
  // Test PWM
  testPWMSequence();
  
  Serial.println("Štartové testy dokončené");
}

void displayWelcomeMessage() {
  Serial.println("\n🌞 MPPT SOLÁRNY REGULÁTOR v1.1 🌞");
  Serial.println("========================================");
  Serial.println("Napíšte 'pomoc' pre zoznam príkazov");
}

// ============================================
// HLAVNÁ SMYČKA
// ============================================

void loop() {
  static unsigned long lastLoopTime = 0;
  unsigned long currentTime = millis();
  unsigned long loopInterval = 100;
  
  if (currentTime - lastLoopTime < loopInterval) {
    return;
  }
  
  lastLoopTime = currentTime;
  
  // 1. Senzory
  readAllSensors();
  
  // 2. Ochrany
  runProtectionChecks();
  
  // 3. Nabíjanie
  runChargingAlgorithm();
  
  // 4. Systémové procesy
  runSystemProcesses();
  
  // 5. Telemetria (každú sekundu)
  static unsigned long lastTelemetryTime = 0;
  if (currentTime - lastTelemetryTime >= 1000) {
    runTelemetryTasks();
    lastTelemetryTime = currentTime;
  }
  
  // 6. Bezdrôtová komunikácia
  runWirelessTelemetry();
  handleWebServer();
  
  // 7. Displej
  runLCDMenuSystem();
  
  // 8. LED indikácia
  updateLEDIndication();
  
  // 9. Sériová komunikácia
  processSerialCommands();
  
  // 10. Watchdog
  feedWatchdog();
  
  // 11. Diagnostika
  monitorPerformance();
}

// ============================================
// POMOCNÉ FUNKCIE PRE HLAVNÚ SMYČKU
// ============================================

void updateLEDIndication() {
  static unsigned long lastLEDUpdate = 0;
  static bool ledState = false;
  
  unsigned long currentTime = millis();
  unsigned long blinkInterval = 1000;
  
  if (currentSystemState == STATE_CHARGING) {
    blinkInterval = 500;
  } else if (currentSystemState == STATE_FAULT) {
    blinkInterval = 200;
  } else if (lowPowerMode) {
    blinkInterval = 2000;
  }
  
  if (currentTime - lastLEDUpdate >= blinkInterval) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    lastLEDUpdate = currentTime;
  }
  
  digitalWrite(ERROR_LED_PIN, 
    (overVoltageFlag || underVoltageFlag || overTempFlag || shortCircuitFlag));
}

void processSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      processCommand(command);
    }
  }
}

void processCommand(String command) {
  int spaceIndex = command.indexOf(' ');
  String cmd = command;
  String param = "";
  
  if (spaceIndex != -1) {
    cmd = command.substring(0, spaceIndex);
    param = command.substring(spaceIndex + 1);
  }
  
  cmd.toLowerCase();
  
  if (cmd == "pomoc" || cmd == "?") {
    printHelp();
  } else if (cmd == "stav") {
    printSystemStatus();
  } else if (cmd == "senzory") {
    printSensorValues();
  } else if (cmd == "ochrany") {
    printProtectionStatus();
  } else if (cmd == "nabijanie") {
    printChargingStatus();
  } else if (cmd == "statistiky") {
    generateDailyReport();
  } else if (cmd == "telemetria") {
    processTelemetryCommand(param);
  } else if (cmd == "bateria") {
    setBatteryCommand(param);
  } else if (cmd == "pwm") {
    setPWMCommand(param);
  } else if (cmd == "reštart") {
    Serial.println("Reštart systému...");
    delay(1000);
    ESP.restart();
  } else if (cmd == "vyčisti") {
    faultCleared = true;
    Serial.println("Chyby vyčistené");
  } else if (cmd == "test") {
    runTestCommand(param);
  } else {
    Serial.print("Neznámy príkaz: ");
    Serial.println(command);
    Serial.println("Napíšte 'pomoc' pre zoznam príkazov");
  }
}

// ============================================
// CHYBIAJUCE FUNKCIE - DOPLNIŤ NA KONIEC SÚBORU
// ============================================

void printHelp() {
  Serial.println("\n=== DOSTUPNÉ PRÍKAZY ===");
  Serial.println("pomoc / ?          - Táto pomoc");
  Serial.println("stav               - Stav systému");
  Serial.println("senzory            - Hodnoty senzorov");
  Serial.println("ochrany            - Stav ochrán");
  Serial.println("nabijanie          - Stav nabíjania");
  Serial.println("statistiky         - Denné štatistiky");
  Serial.println("telemetria [cmd]   - Telemetria (stav/report/export)");
  Serial.println("bateria [typ]      - Nastavenie batérie (0=AGM,1=LiFePO4,2=Pb,3=Vlastny)");
  Serial.println("pwm [hodnota]      - Manuálne PWM (0-100%)");
  Serial.println("reštart            - Reštart systému");
  Serial.println("vyčisti            - Vyčistenie chýb");
  Serial.println("test [test]        - Testovacie funkcie");
  Serial.println("=======================\n");
}

void printSystemStatus() {
  Serial.println("\n=== STATUS SYSTÉMU ===");
  Serial.print("Stav systému: ");
  Serial.println(getStateName(currentSystemState));
  Serial.print("Fáza nabíjania: ");
  Serial.println(getPhaseName(currentChargePhase));
  Serial.print("Typ batérie: ");
  Serial.println(currentProfile->batteryName);
  Serial.print("Režim: ");
  Serial.println(lowPowerMode ? "ŠETRNÝ" : "NORMÁLNY");
  Serial.print("WiFi: ");
  switch(wifiState) {
    case WIFI_DISCONNECTED: Serial.println("ODPOJENÉ"); break;
    case WIFI_CONNECTING: Serial.println("PRIPOJOVANIE"); break;
    case WIFI_CONNECTED: Serial.println("PRIPOJENÉ"); break;
    case WIFI_ERROR: Serial.println("CHYBA"); break;
  }
  Serial.print("Čas behu: ");
  Serial.print(millis() / 3600000.0f, 1);
  Serial.println(" hodín");
  Serial.println("=====================\n");
}

void printProtectionStatus() {
  Serial.println("\n=== STATUS OCHRÁN ===");
  Serial.print("Celkový stav ochrán: ");
  switch(currentProtection) {
    case PROTECTION_NORMAL: Serial.println("NORMÁLNY"); break;
    case PROTECTION_WARNING: Serial.println("VAROVANIE"); break;
    case PROTECTION_FAULT: Serial.println("CHYBA"); break;
    case PROTECTION_EMERGENCY: Serial.println("KRITICKÝ"); break;
  }
  
  Serial.print("Prekročenie napätia: ");
  Serial.println(overVoltageFlag ? "ÁNO" : "NIE");
  Serial.print("Podkročenie napätia: ");
  Serial.println(underVoltageFlag ? "ÁNO" : "NIE");
  Serial.print("Prehriatie: ");
  Serial.println(overTempFlag ? "ÁNO" : "NIE");
  Serial.print("Zkrat: ");
  Serial.println(shortCircuitFlag ? "ÁNO" : "NIE");
  Serial.println("=====================\n");
}

void printChargingStatus() {
  Serial.println("\n=== STATUS NABÍJANIA ===");
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
  Serial.print("Krok MPPT: "); Serial.print(mpptStepSize, 4); Serial.println("%");
  Serial.print("Panel: "); Serial.print(panelPower, 1); Serial.println("W");
  Serial.print("Batéria: "); Serial.print(batteryPower, 1); Serial.println("W");
  Serial.print("Účinnosť: "); Serial.print(efficiency, 1); Serial.println("%");
  
  Serial.println("=======================\n");
}

void setBatteryCommand(String param) {
  if (param.length() > 0) {
    int type = param.toInt();
    if (type >= 0 && type <= 3) {
      setBatteryType((BatteryType)type);
      Serial.print("Typ batérie nastavený na: ");
      Serial.println(currentProfile->batteryName);
    } else {
      Serial.println("Chyba: Typ batérie musí byť 0-3");
      Serial.println("0 = AGM, 1 = LiFePO4, 2 = Olovo, 3 = Vlastný");
    }
  } else {
    Serial.print("Aktuálny typ batérie: ");
    Serial.println(currentProfile->batteryName);
  }
}

void setPWMCommand(String param) {
  if (param.length() > 0) {
    float pwmValue = param.toFloat();
    if (pwmValue >= 0 && pwmValue <= 100) {
      setManualPWM(pwmValue);
      Serial.print("PWM manuálne nastavené na: ");
      Serial.print(pwmValue, 1);
      Serial.println("%");
    } else {
      Serial.println("Chyba: PWM musí byť v rozsahu 0-100%");
    }
  } else {
    Serial.print("Aktuálne PWM: ");
    Serial.print(pwmDutyCycle, 1);
    Serial.println("%");
  }
}

void runTestCommand(String param) {
  if (param == "pwm") {
    testPWMSequence();
  } else if (param == "senzory") {
    testSensors();
  } else if (param == "adc") {
    testADC();
  } else {
    Serial.println("Dostupné testy:");
    Serial.println("  pwm     - Test PWM sekvencie");
    Serial.println("  senzory - Test všetkých senzorov");
    Serial.println("  adc     - Test ADC prevodníka");
  }
}

void testPWMSequence() {
  Serial.println("=== TEST PWM SEKVENCIE ===");
  Serial.println("Postupné zvyšovanie PWM 0-100%...");
  
  for (int i = 0; i <= 100; i += 10) {
    setManualPWM(i);
    Serial.print("PWM: ");
    Serial.print(i);
    Serial.println("%");
    delay(500);
  }
  
  Serial.println("Postupné znižovanie PWM 100-0%...");
  for (int i = 100; i >= 0; i -= 10) {
    setManualPWM(i);
    Serial.print("PWM: ");
    Serial.print(i);
    Serial.println("%");
    delay(500);
  }
  
  setManualPWM(0);
  Serial.println("Test PWM dokončený");
  Serial.println("=====================");
}

void feedWatchdog() {
  // Simulácia watchdog timera
  static unsigned long lastWatchdog = 0;
  
  if (millis() - lastWatchdog > 1000) {
    lastWatchdog = millis();
  }
  
  if (millis() - lastWatchdog > 5000) {
    Serial.println("WATCHDOG: Zaseknutie detekované!");
    emergencyShutdown("WATCHDOG_TIMEOUT");
  }
}

void monitorPerformance() {
  static unsigned long lastPerformanceCheck = 0;
  static unsigned long loopCounter = 0;
  
  loopCounter++;
  
  if (loopCounter >= 100) {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastPerformanceCheck;
    
    float loadPercent = (100.0f * loopCounter * 100.0f) / elapsed;
    
    if (loadPercent > 80.0f) {
      Serial.print("VAROVANIE: Vysoké zaťaženie CPU: ");
      Serial.print(loadPercent, 1);
      Serial.println("%");
    }
    
    lastPerformanceCheck = currentTime;
    loopCounter = 0;
  }
}

// ============================================
// FUNKCIE PRE PREKLAD STAVOV
// ============================================

String getPhaseName(ChargePhase phase) {
  switch(phase) {
    case PHASE_OFF: return "VYPNUTÉ";
    case PHASE_BULK: return "HROMADNÉ";
    case PHASE_ABSORPTION: return "ABSORPCIA";
    case PHASE_FLOAT: return "FLOAT";
    case PHASE_EQUALIZATION: return "VYROVNAVANIE";
    case PHASE_MAINTENANCE: return "UDRIZBA";
    default: return "NEZNÁMY";
  }
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

String getMenuName(MenuState menu) {
  switch(menu) {
    case MENU_MAIN: return "HLAVNÉ MENU";
    case MENU_STATUS: return "STAV";
    case MENU_CHARGING: return "NABÍJANIE";
    case MENU_STATS: return "ŠTATISTIKY";
    case MENU_SETTINGS: return "NASTAVENIA";
    case MENU_BATTERY_SETUP: return "BATÉRIA";
    case MENU_SYSTEM_INFO: return "SYSTÉM";
    case MENU_DIAGNOSTICS: return "DIAGNOSTIKA";
    case MENU_CALIBRATION: return "KALIBRÁCIA";
    default: return "NEZNÁMY";
  }
}