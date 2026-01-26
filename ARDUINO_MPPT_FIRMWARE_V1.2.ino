// ============================================
// HLAVNÝ KÓD MPPT REGULÁTORA - KOMPLETNÝ SYSTÉM SK
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
// GLOBÁLNE OBJEKTY KNIŽNÍC - SKUTOČNÉ DEFINÍCIE
// ============================================

Adafruit_ADS1115 ads;  // SKUTOČNÁ definícia ADS1115 objektu
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
void logEvent(const char* message);
void logError(const char* message);
void printProtectionStatus();

// MODUL 4: Nabíjanie
void initChargingSystem();
void runChargingAlgorithm();
void setBatteryType(BatteryType type);
void setManualPWM(float dutyPercent);
String getPhaseName(ChargePhase phase);
String getStateName(SystemState state);
void printChargingStatus();
void applyPWM(float dutyPercent);
void setPWM(float dutyPercent);

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

// Nové pomocné funkcie
void addToEventLog(const char* message);
void saveErrorToLog(const char* message);
void saveErrorToEEPROM(const char* reason);
void updateDisplayWarning(const char* message);

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

// ZOSTÁVAJÚCA ČASŤ KÓDU JE SPRÁVNA...
// ... (zvyšok kódu zostáva rovnaký)