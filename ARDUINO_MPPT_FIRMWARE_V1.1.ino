// ============================================
// Hlavný kód MPPT regulátora
// Verzia 1.1 - Kompletný systém
// ============================================

// ============================================
// KNIŽNICE
// ============================================

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <EEPROM.h>

// ============================================
// KONFIGURÁCIA PINOV
// ============================================

// PWM výstup
#define PWM_PIN 27
#define PWM_CHANNEL 0
#define PWM_FREQUENCY 20000
#define PWM_RESOLUTION 10  // 10-bit (0-1023)

// Indikačné LED
#define LED_PIN 2
#define ERROR_LED_PIN 4

// Relé/MOSFET pre odpojenie
#define BATTERY_DISCONNECT_PIN 14
#define PANEL_DISCONNECT_PIN 12

// Teplotný senzor
#define TEMP_PIN 34

// Tlačidlá (už definované v LCDMenu.ino)
// #define BUTTON_UP_PIN 32
// #define BUTTON_DOWN_PIN 33
// #define BUTTON_ENTER_PIN 25
// #define BUTTON_BACK_PIN 26

// ============================================
// GLOBÁLNE PREMENNÉ
// ============================================

// Senzorické hodnoty
float panelVoltage = 0.0f;
float panelCurrent = 0.0f;
float panelPower = 0.0f;
float batteryVoltage = 0.0f;
float batteryCurrent = 0.0f;
float batteryPower = 0.0f;
float efficiency = 0.0f;
float temperature = 25.0f;

// Stavové premenné
bool overVoltageFlag = false;
bool underVoltageFlag = false;
bool overTempFlag = false;
bool shortCircuitFlag = false;
bool lowPowerMode = false;
bool faultCleared = false;

// ============================================
// EXTERNÉ DEKLARÁCIE
// ============================================

// Funkcie z jednotlivých súborov
extern void initSensors();
extern void readAllSensors();
extern void calibrateSensors();

extern void runProtectionChecks();
extern void emergencyShutdown(const char* reason);
extern void logWarning(const char* message);

extern void initChargingSystem();
extern void runChargingAlgorithm();
extern void setBatteryType(BatteryType type);
extern void setManualPWM(float dutyPercent);

extern void initSystemProcesses();
extern void runSystemProcesses();
extern void changeSystemState(SystemState newState);

extern void initTelemetry();
extern void runTelemetryTasks();
extern void generateDailyReport();

extern void initWirelessTelemetry();
extern void runWirelessTelemetry();
extern void handleWebServer();

extern void initLCDMenu();
extern void runLCDMenuSystem();
extern void updateDisplay();

// ============================================
// SETUP FUNKCIA
// ============================================

void setup() {
  // 1. Inicializácia sériovej komunikácie
  Serial.begin(115200);
  Serial.println("\n\n=================================");
  Serial.println("   MPPT REGULATOR - START");
  Serial.println("=================================");
  
  // 2. Inicializácia EEPROM
  EEPROM.begin(512);
  Serial.println("EEPROM inicializovaná");
  
  // 3. Inicializácia GPIO pinov
  initGPIO();
  
  // 4. Inicializácia PWM
  initPWM();
  
  // 5. Inicializácia senzorov
  initSensors();
  
  // 6. Inicializácia systémových procesov
  initSystemProcesses();
  
  // 7. Inicializácia nabíjacieho systému
  initChargingSystem();
  
  // 8. Inicializácia telemetrie
  initTelemetry();
  
  // 9. Inicializácia displeja
  initLCDMenu();
  
  // 10. Inicializácia bezdrôtovej komunikácie
  initWirelessTelemetry();
  
  // 11. Úvodné testy
  performStartupTests();
  
  Serial.println("=================================");
  Serial.println("   SYSTEM PRIPRAVENY");
  Serial.println("=================================\n");
  
  // Zobrazenie uvítacej obrazovky
  displayWelcomeMessage();
}

void initGPIO() {
  Serial.println("Inicializácia GPIO pinov...");
  
  // Nastavenie pinov ako výstupy
  pinMode(LED_PIN, OUTPUT);
  pinMode(ERROR_LED_PIN, OUTPUT);
  pinMode(BATTERY_DISCONNECT_PIN, OUTPUT);
  pinMode(PANEL_DISCONNECT_PIN, OUTPUT);
  
  // Počiatočné stavy
  digitalWrite(LED_PIN, LOW);
  digitalWrite(ERROR_LED_PIN, LOW);
  digitalWrite(BATTERY_DISCONNECT_PIN, HIGH);  // Pripojené
  digitalWrite(PANEL_DISCONNECT_PIN, HIGH);    // Pripojené
  
  Serial.println("GPIO inicializované");
}

void initPWM() {
  Serial.println("Inicializácia PWM...");
  
  // Nastavenie PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);  // Počiatočná hodnota 0
  
  Serial.print("PWM inicializované: ");
  Serial.print(PWM_FREQUENCY);
  Serial.print("Hz, ");
  Serial.print(PWM_RESOLUTION);
  Serial.println("bit");
}

void performStartupTests() {
  Serial.println("Spúšťam štartové testy...");
  
  // Rýchly test LED
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(200);
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  
  // Test PWM (postupne zvyšovať a znižovať)
  for (int i = 0; i <= 100; i += 10) {
    ledcWrite(PWM_CHANNEL, i * 10.23);
    delay(50);
  }
  for (int i = 100; i >= 0; i -= 10) {
    ledcWrite(PWM_CHANNEL, i * 10.23);
    delay(50);
  }
  ledcWrite(PWM_CHANNEL, 0);
  
  Serial.println("Štartové testy dokončené");
}

void displayWelcomeMessage() {
  Serial.println("\n=== MPPT REGULATOR v1.1 ===");
  Serial.println("Funkcie:");
  Serial.println("- MPPT algoritmus s adaptívnym krokom");
  Serial.println("- 4-fázové nabíjanie (Bulk/Absorpcia/Float/Udržba)");
  Serial.println("- Podpora AGM, LiFePO4, olovených batérií");
  Serial.println("- Bezdrôtová telemetria (WiFi + Blynk)");
  Serial.println("- OLED displej s menu systémom");
  Serial.println("- Komplexné ochrany a diagnostika");
  Serial.println("- Šetrný režim pre noc");
  Serial.println("===========================\n");
}

// ============================================
// HLAVNÁ SMYČKA (LOOP)
// ============================================

void loop() {
  // Časovač pre hlavnú slučku
  static unsigned long lastLoopTime = 0;
  unsigned long currentTime = millis();
  unsigned long loopInterval = 100;  // 100ms základný cyklus
  
  // Iba ak uplynul dostatočný čas
  if (currentTime - lastLoopTime < loopInterval) {
    return;
  }
  
  lastLoopTime = currentTime;
  
  // === 1. ČÍTANIE SENZOROV (10Hz) ===
  readAllSensors();
  
  // === 2. BEZPEČNOSTNÉ KONTROLY (10Hz) ===
  runProtectionChecks();
  
  // === 3. NABÍJACÍ ALGORITMUS (10Hz) ===
  runChargingAlgorithm();
  
  // === 4. SYSTÉMOVÉ PROCESY (10Hz) ===
  runSystemProcesses();
  
  // === 5. TELEMETRIA (1Hz) ===
  static unsigned long lastTelemetryTime = 0;
  if (currentTime - lastTelemetryTime >= 1000) {
    runTelemetryTasks();
    lastTelemetryTime = currentTime;
  }
  
  // === 6. BEZDRÔTOVÁ KOMUNIKÁCIA ===
  runWirelessTelemetry();
  handleWebServer();
  
  // === 7. DISPLEJ A MENU ===
  runLCDMenuSystem();
  
  // === 8. LED INDIKÁCIA ===
  updateLEDIndication();
  
  // === 9. SÉRIOVÁ KOMUNIKÁCIA ===
  processSerialCommands();
  
  // === 10. WATCHDOG ÚDRŽBA ===
  feedWatchdog();
  
  // === 11. DIAGNOSTIKA VÝKONU ===
  monitorPerformance();
}

// ============================================
// POMOCNÉ FUNKCIE
// ============================================

void updateLEDIndication() {
  static unsigned long lastLEDUpdate = 0;
  static bool ledState = false;
  
  unsigned long currentTime = millis();
  
  // Rýchlosť blikania podľa stavu
  unsigned long blinkInterval = 1000;  // Štandardne 1Hz
  
  if (currentSystemState == STATE_CHARGING) {
    // Rýchlejšie blikanie pri nabíjaní
    blinkInterval = 500;
  } else if (currentSystemState == STATE_FAULT) {
    // Veľmi rýchle blikanie pri chybe
    blinkInterval = 200;
  } else if (lowPowerMode) {
    // Pomalé blikanie v šetrnom režime
    blinkInterval = 2000;
  }
  
  // Blikanie LED
  if (currentTime - lastLEDUpdate >= blinkInterval) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    lastLEDUpdate = currentTime;
  }
  
  // Chybová LED
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
  // Rozdelenie príkazu na časti
  int spaceIndex = command.indexOf(' ');
  String cmd = command;
  String param = "";
  
  if (spaceIndex != -1) {
    cmd = command.substring(0, spaceIndex);
    param = command.substring(spaceIndex + 1);
  }
  
  cmd.toLowerCase();
  
  // Spracovanie príkazov
  if (cmd == "help" || cmd == "?") {
    printHelp();
  } else if (cmd == "status") {
    printSystemStatus();
  } else if (cmd == "sensors") {
    printSensorValues();
  } else if (cmd == "protections") {
    printProtectionStatus();
  } else if (cmd == "charging") {
    printChargingStatus();
  } else if (cmd == "stats") {
    generateDailyReport();
  } else if (cmd == "telemetry") {
    processTelemetryCommand(param);
  } else if (cmd == "battery") {
    setBatteryCommand(param);
  } else if (cmd == "pwm") {
    setPWMCommand(param);
  } else if (cmd == "reboot") {
    Serial.println("Reboot systému...");
    delay(1000);
    ESP.restart();
  } else if (cmd == "clear") {
    faultCleared = true;
    Serial.println("Chyby vyčistené");
  } else if (cmd == "test") {
    runTestCommand(param);
  } else {
    Serial.print("Neznámy príkaz: ");
    Serial.println(command);
    Serial.println("Napíšte 'help' pre zoznam príkazov");
  }
}

void printHelp() {
  Serial.println("\n=== DOSTUPNÉ PRÍKAZY ===");
  Serial.println("help / ?          - Táto pomoc");
  Serial.println("status            - Stav systému");
  Serial.println("sensors           - Hodnoty senzorov");
  Serial.println("protections       - Stav ochrán");
  Serial.println("charging          - Stav nabíjania");
  Serial.println("stats             - Denné štatistiky");
  Serial.println("telemetry [cmd]   - Telemetria (status/report/export)");
  Serial.println("battery [type]    - Nastavenie batérie (0=AGM,1=LiFePO4,2=Pb,3=Custom)");
  Serial.println("pwm [value]       - Manuálne PWM (0-100%)");
  Serial.println("reboot            - Reboot systému");
  Serial.println("clear             - Vyčistenie chýb");
  Serial.println("test [test]       - Testovacie funkcie");
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
    } else {
      Serial.println("Chyba: PWM musí byť 0-100");
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
  } else if (param == "sensors") {
    testSensors();
  } else if (param == "adc") {
    testADC();
  } else {
    Serial.println("Dostupné testy: pwm, sensors, adc");
  }
}

void testPWMSequence() {
  Serial.println("Test PWM sekvencie...");
  for (int i = 0; i <= 100; i += 10) {
    setManualPWM(i);
    Serial.print("PWM: ");
    Serial.print(i);
    Serial.println("%");
    delay(500);
  }
  setManualPWM(0);
  Serial.println("Test PWM dokončený");
}

// ============================================
// DIAGNOSTICKÉ FUNKCIE
// ============================================

void monitorPerformance() {
  static unsigned long lastPerformanceCheck = 0;
  static unsigned long loopCounter = 0;
  
  loopCounter++;
  
  // Každých 100 cyklov (10 sekúnd)
  if (loopCounter >= 100) {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastPerformanceCheck;
    
    // Výpočet zaťaženia
    float loadPercent = (100.0f * loopCounter * 100.0f) / elapsed;
    
    // Výpis iba ak je zaťaženie vysoké
    if (loadPercent > 80.0f) {
      Serial.print("VAROVANIE: Vysoké zaťaženie CPU: ");
      Serial.print(loadPercent, 1);
      Serial.println("%");
    }
    
    // Reset
    lastPerformanceCheck = currentTime;
    loopCounter = 0;
  }
}

void feedWatchdog() {
  // Simulácia watchdog timera
  static unsigned long lastWatchdog = 0;
  
  if (millis() - lastWatchdog > 1000) {
    // Resetovanie watchdog (v HW by bol watchdog timer)
    lastWatchdog = millis();
  }
  
  // Kontrola zaseknutia
  if (millis() - lastWatchdog > 5000) {
    Serial.println("WATCHDOG: Zaseknutie detekované!");
    emergencyShutdown("WATCHDOG_TIMEOUT");
  }
}

// ============================================
// EMERGENCY HANDLING
// ============================================

void emergencyStop() {
  // Okamžité zastavenie všetkého
  ledcWrite(PWM_CHANNEL, 0);
  digitalWrite(BATTERY_DISCONNECT_PIN, LOW);
  digitalWrite(PANEL_DISCONNECT_PIN, LOW);
  
  // Blikanie error LED
  while (true) {
    digitalWrite(ERROR_LED_PIN, HIGH);
    delay(100);
    digitalWrite(ERROR_LED_PIN, LOW);
    delay(100);
  }
}

// ============================================
// VÝPIS STAVU SYSTÉMU
// ============================================

void printSystemStatus() {
  Serial.println("\n=== STATUS SYSTÉMU ===");
  Serial.print("Stav: ");
  Serial.println(getStateName(currentSystemState));
  Serial.print("Fáza nabíjania: ");
  Serial.println(getPhaseName(currentChargePhase));
  Serial.print("Typ batérie: ");
  Serial.println(currentProfile->batteryName);
  Serial.print("Režim: ");
  Serial.println(lowPowerMode ? "ŠETRNÝ" : "NORMÁLNY");
  Serial.print("WiFi: ");
  Serial.println(wifiState == WIFI_CONNECTED ? "PRIPOJENÉ" : "ODPOJENÉ");
  Serial.print("Uptime: ");
  Serial.print(millis() / 3600000.0f, 1);
  Serial.println(" hodín");
  Serial.println("=====================\n");
}

void printSensorValues() {
  Serial.println("\n=== HODNOTY SENZOROV ===");
  Serial.print("Panel: ");
  Serial.print(panelVoltage, 2);
  Serial.print("V, ");
  Serial.print(panelCurrent, 3);
  Serial.print("A, ");
  Serial.print(panelPower, 1);
  Serial.println("W");
  
  Serial.print("Batéria: ");
  Serial.print(batteryVoltage, 2);
  Serial.print("V, ");
  Serial.print(batteryCurrent, 3);
  Serial.print("A, ");
  Serial.print(batteryPower, 1);
  Serial.println("W");
  
  Serial.print("Účinnosť: ");
  Serial.print(efficiency, 1);
  Serial.println("%");
  
  Serial.print("Teplota: ");
  Serial.print(temperature, 1);
  Serial.println("°C");
  
  Serial.print("PWM: ");
  Serial.print(pwmDutyCycle, 1);
  Serial.println("%");
  Serial.println("=======================\n");
}

// ============================================
// KONIEC HLAVNÉHO KÓDU
// ============================================