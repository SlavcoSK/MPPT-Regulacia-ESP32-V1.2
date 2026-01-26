// ============================================
// ČÍTANIE A FILTRÁCIA SENZOROV - MODUL 2
// ============================================

#include "config.h"
#include "globals.h"
#include <Adafruit_ADS1X15.h>

// ============================================
// LOKÁLNE PREMENNÉ MODULU (iba tu, nie v globals)
// ============================================

Adafruit_ADS1115 ads;  // Objekt pre ADC prevodník ADS1115

// Kalibračné offsety pre senzory (iba lokálne)
float currentOffset = 0.0f;      // Offset pre prúdový senzor
float voltageOffset1 = 0.0f;     // Offset pre napätie panelu (kanál 0)
float voltageOffset2 = 0.0f;     // Offset pre napätie batérie (kanál 2)

// Filtrované hodnoty senzorov (iba lokálne)
float filteredPanelVoltage = 0.0f;   // Filtrované napätie panelu
float filteredPanelCurrent = 0.0f;   // Filtrovaný prúd panelu
float filteredBatteryVoltage = 0.0f; // Filtrované napätie batérie
float filteredBatteryCurrent = 0.0f; // Filtrovaný prúd batérie
float filteredTemperature = 0.0f;    // Filtrovaná teplota

// ============================================
// FUNKCIE PRE NAČÍTANIE A FILTRÁCIU HODNÔT
// ============================================

void initSensors() {
  Serial.println("Inicializácia senzorov...");
  
  // Pokus o inicializáciu ADS1115 ADC prevodníka
  if (!ads.begin()) {
    Serial.println("CHYBA: ADS1115 nenájdený! Skontrolujte zapojenie.");
    Serial.println("Postup riešenia:");
    Serial.println("1. Skontrolujte I2C zapojenie (SDA=21, SCL=22 pre ESP32)");
    Serial.println("2. Skontrolujte napájanie ADS1115 (3.3V a GND)");
    Serial.println("3. Skontrolujte adresu I2C (0x48 štandardne)");
    while (1) { // Zastavenie programu - kritická chyba
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      delay(500);
    }
  }
  
  // Nastavenie zosilnenia ADS1115 na rozsah ±4.096V
  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 úspešne inicializovaný");
  
  // Automatická kalibrácia senzorov pri štarte
  calibrateSensors();
  
  Serial.println("Senzory pripravené na prácu");
}

void calibrateSensors() {
  Serial.println("Začínam kalibráciu senzorov...");
  Serial.println("POZOR: Počas kalibrácie odpojte všetky vstupy!");
  
  float sumCurrent = 0.0f;        // Súčet hodnôt pre prúdový senzor
  float sumPanelVoltage = 0.0f;   // Súčet hodnôt pre napätie panelu
  float sumBatteryVoltage = 0.0f; // Súčet hodnôt pre napätie batérie
  
  int calibrationSamples = 100;   // Počet vzoriek pre presnú kalibráciu
  
  // Zber kalibračných vzoriek
  for (int i = 0; i < calibrationSamples; i++) {
    // Čítanie surových hodnôt pri vypnutom výkone
    sumCurrent += ads.readADC_SingleEnded(1);        // Prúdový kanál
    sumPanelVoltage += ads.readADC_SingleEnded(0);   // Napätie panelu
    sumBatteryVoltage += ads.readADC_SingleEnded(2); // Napätie batérie
    
    delay(10);  // Krátky oneskorenie medzi meraniami
    if (i % 20 == 0) Serial.print("."); // Indikácia priebehu
  }
  
  // Výpočet priemerných offsetov
  currentOffset = sumCurrent / calibrationSamples;
  voltageOffset1 = sumPanelVoltage / calibrationSamples;
  voltageOffset2 = sumBatteryVoltage / calibrationSamples;
  
  Serial.println("\nKalibrácia úspešne dokončená");
  Serial.print("Offset prúdu: "); Serial.println(currentOffset, 2);
  Serial.print("Offset panelu: "); Serial.println(voltageOffset1, 2);
  Serial.print("Offset batérie: "); Serial.println(voltageOffset2, 2);
}

float readRawPanelVoltage() {
  // Čítanie surového napätia panelu z ADC kanálu 0
  int16_t rawValue = ads.readADC_SingleEnded(0);
  
  // Konverzia ADC hodnoty na napätie v Voltách
  float voltage = (rawValue - voltageOffset1) * ADC_SCALE_FACTOR / 1000.0f;
  
  // Aplikovanie pomeru deliča napätia
  voltage *= VOLTAGE_DIVIDER_RATIO;
  
  return voltage;
}

float readRawPanelCurrent() {
  // Čítanie surového prúdu panelu z ADC kanálu 1
  int16_t rawValue = ads.readADC_SingleEnded(1);
  
  // Konverzia ADC hodnoty na napätie na shunt odpore
  float voltageDrop = (rawValue - currentOffset) * ADC_SCALE_FACTOR / 1000.0f;
  
  // Výpočet prúdu podľa Ohmovho zákona: I = V / R
  float current = voltageDrop / SHUNT_RESISTOR;
  
  return current;
}

float readRawBatteryVoltage() {
  // Čítanie surového napätia batérie z ADC kanálu 2
  int16_t rawValue = ads.readADC_SingleEnded(2);
  
  // Konverzia ADC hodnoty na napätie v Voltách
  float voltage = (rawValue - voltageOffset2) * ADC_SCALE_FACTOR / 1000.0f;
  
  // Aplikovanie pomeru deliča napätia
  voltage *= VOLTAGE_DIVIDER_RATIO;
  
  return voltage;
}

float readRawBatteryCurrent() {
  // Pre nabíjanie používame rovnaký shunt odpor
  // V tomto zapojení sa prúd meria rovnakým spôsobom
  return readRawPanelCurrent();
}

float readRawTemperature() {
  // Čítanie teplotného senzora (LM35 alebo NTC)
  int rawValue = analogRead(TEMP_PIN);
  
  // Konverzia 12-bit ADC hodnoty na napätie (ESP32 má 3.3V referenciu)
  float voltage = rawValue * (3.3 / 4095.0);
  
  // Pre LM35 senzor: 10mV/°C, 0V = 0°C
  float temperature = voltage * 100.0f;
  
  return temperature;
}

// ============================================
// FILTRÁCIA A SPRAcovanie HODNÔT
// ============================================

float applyIIRFilter(float newValue, float oldValue, float alpha) {
  // IIR filter prvého rádu: y[n] = alpha * x[n] + (1-alpha) * y[n-1]
  return alpha * newValue + (1.0f - alpha) * oldValue;
}

void readAllSensors() {
  // KROK 1: Čítanie surových nefiltrovaných hodnôt
  float rawPanelV = readRawPanelVoltage();
  float rawPanelI = readRawPanelCurrent();
  float rawBatteryV = readRawBatteryVoltage();
  float rawBatteryI = readRawBatteryCurrent();
  float rawTemp = readRawTemperature();
  
  // KROK 2: Aplikovanie IIR filtrov na vyhladenie signálu
  filteredPanelVoltage = applyIIRFilter(rawPanelV, filteredPanelVoltage, ALPHA_VOLTAGE);
  filteredPanelCurrent = applyIIRFilter(rawPanelI, filteredPanelCurrent, ALPHA_CURRENT);
  filteredBatteryVoltage = applyIIRFilter(rawBatteryV, filteredBatteryVoltage, ALPHA_VOLTAGE);
  filteredBatteryCurrent = applyIIRFilter(rawBatteryI, filteredBatteryCurrent, ALPHA_CURRENT);
  filteredTemperature = applyIIRFilter(rawTemp, filteredTemperature, ALPHA_TEMP);
  
  // KROK 3: Ochrana proti záporným hodnotám prúdu
  if (filteredPanelCurrent < 0.0f) filteredPanelCurrent = 0.0f;
  if (filteredBatteryCurrent < 0.0f) filteredBatteryCurrent = 0.0f;
  
  // KROK 4: Aktualizácia globálnych premenných filtrovanými hodnotami
  panelVoltage = filteredPanelVoltage;
  panelCurrent = filteredPanelCurrent;
  batteryVoltage = filteredBatteryVoltage;
  batteryCurrent = filteredBatteryCurrent;
  temperature = filteredTemperature;
  
  // KROK 5: Výpočet výkonov z napätia a prúdu
  panelPower = panelVoltage * panelCurrent;
  batteryPower = batteryVoltage * batteryCurrent;
  
  // KROK 6: Výpočet účinnosti MPPT regulátora
  if (panelPower > 0.1f) {  // Iba ak máme zmysluplný výkon z panelu (>0.1W)
    efficiency = (batteryPower / panelPower) * 100.0f;
    efficiency = constrain(efficiency, 0.0f, 100.0f);  // Obmedzenie na rozsah 0-100%
  } else {
    efficiency = 0.0f;  // Pri nulovom výkone panelu je účinnosť 0%
  }
}

// ============================================
// DIAGNOSTICKÉ A POMOCNÉ FUNKCIE
// ============================================

void printSensorValues() {
  Serial.println("\n=== HODNOTY SENZOROV ===");
  Serial.print("Napätie panelu: "); Serial.print(panelVoltage, 2); Serial.println(" V");
  Serial.print("Prúd panelu: "); Serial.print(panelCurrent, 3); Serial.println(" A");
  Serial.print("Výkon panelu: "); Serial.print(panelPower, 2); Serial.println(" W");
  Serial.print("Napätie batérie: "); Serial.print(batteryVoltage, 2); Serial.println(" V");
  Serial.print("Prúd batérie: "); Serial.print(batteryCurrent, 3); Serial.println(" A");
  Serial.print("Výkon batérie: "); Serial.print(batteryPower, 2); Serial.println(" W");
  Serial.print("Teplota systému: "); Serial.print(temperature, 1); Serial.println(" °C");
  Serial.print("Účinnosť MPPT: "); Serial.print(efficiency, 1); Serial.println(" %");
  Serial.println("=======================\n");
}

void testSensors() {
  Serial.println("=== TEST SENZOROV ===");
  Serial.println("Testujem všetky senzory po dobu 5 sekúnd...");
  
  for (int i = 0; i < 5; i++) {
    readAllSensors();
    printSensorValues();
    delay(1000);  // Čakanie 1 sekunda medzi meraniami
  }
  
  Serial.println("Test senzorov dokončený");
  Serial.println("=====================");
}

void testADC() {
  Serial.println("=== TEST ADC PREVODNÍKA ===");
  
  for (int i = 0; i < 4; i++) {
    int16_t value = ads.readADC_SingleEnded(i);
    Serial.print("ADC kanál ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(value);
    Serial.print(" (");
    Serial.print(value * ADC_SCALE_FACTOR, 2);
    Serial.println(" mV)");
  }
  
  Serial.println("=====================");
}