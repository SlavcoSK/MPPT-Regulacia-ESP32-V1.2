// ============================================
// ČÍTANIE A FILTRÁCIA SENZOROV
// ============================================

#include <Adafruit_ADS1X15.h>  // Knižnica pre ADC prevodník

Adafruit_ADS1115 ads;  // Vytvorenie objektu ADS1115

// ============================================
// KONFIGURÁCIA SENZOROV
// ============================================

// Kalibračné konštanty pre ADC
#define ADC_SCALE_FACTOR 0.125f  // Faktor pre ADS1115 (mV na digit)
#define SHUNT_RESISTOR 0.01f     // Hodnota shunt odporu pre prúd (Ohm)
#define VOLTAGE_DIVIDER_RATIO 5.11f  // Delič napätia (R1=39k, R2=10k)

// Kalibračné offsety
float currentOffset = 0.0f;      // Offset pre prúdový senzor
float voltageOffset1 = 0.0f;     // Offset pre napätie panelu
float voltageOffset2 = 0.0f;     // Offset pre napätie batérie

// Filtrované hodnoty
float filteredPanelVoltage = 0.0f;
float filteredPanelCurrent = 0.0f;
float filteredBatteryVoltage = 0.0f;
float filteredBatteryCurrent = 0.0f;
float filteredTemperature = 0.0f;

// Koeficienty IIR filtrov
const float ALPHA_VOLTAGE = 0.05f;   // Pomaly filter pre napätie
const float ALPHA_CURRENT = 0.1f;    // Rýchlejší filter pre prúd
const float ALPHA_TEMP = 0.02f;      // Veľmi pomaly filter pre teplotu

// ============================================
// INICIALIZÁCIA SENZOROV
// ============================================

void initSensors() {
  Serial.println("Inicializácia senzorov...");
  
  // Inicializácia ADS1115
  if (!ads.begin()) {
    Serial.println("CHYBA: ADS1115 nenájdený!");
    while (1);  // Zastavenie programu pri chybe
  }
  
  // Nastavenie zosilnenia ADS1115
  ads.setGain(GAIN_ONE);  // ±4.096V rozsah
  
  Serial.println("ADS1115 inicializovaný");
  
  // Kalibrácia offsetov
  calibrateSensors();
}

// ============================================
// KALIBRÁCIA SENZOROV
// ============================================

void calibrateSensors() {
  Serial.println("Kalibrácia senzorov...");
  
  float sumCurrent = 0.0f;
  float sumPanelVoltage = 0.0f;
  float sumBatteryVoltage = 0.0f;
  
  int calibrationSamples = 100;  // Počet vzoriek pre kalibráciu
  
  for (int i = 0; i < calibrationSamples; i++) {
    // Čítanie surových hodnôt pri vypnutom výkone
    sumCurrent += ads.readADC_SingleEnded(1);        // Prúdový kanál
    sumPanelVoltage += ads.readADC_SingleEnded(0);   // Panel napätie
    sumBatteryVoltage += ads.readADC_SingleEnded(2); // Batéria napätie
    
    delay(10);  // Krátky oneskorenie medzi meraniami
  }
  
  // Výpočet priemerných offsetov
  currentOffset = sumCurrent / calibrationSamples;
  voltageOffset1 = sumPanelVoltage / calibrationSamples;
  voltageOffset2 = sumBatteryVoltage / calibrationSamples;
  
  Serial.println("Kalibrácia dokončená");
  Serial.print("Offset prúdu: "); Serial.println(currentOffset);
  Serial.print("Offset panelu: "); Serial.println(voltageOffset1);
  Serial.print("Offset batérie: "); Serial.println(voltageOffset2);
}

// ============================================
// ČÍTANIE SUROVÝCH HODNÔT
// ============================================

float readRawPanelVoltage() {
  int16_t rawValue = ads.readADC_SingleEnded(0);
  float voltage = (rawValue - voltageOffset1) * ADC_SCALE_FACTOR / 1000.0f;  // Konverzia na Volty
  voltage *= VOLTAGE_DIVIDER_RATIO;  // Aplikovanie deliča napätia
  return voltage;
}

float readRawPanelCurrent() {
  int16_t rawValue = ads.readADC_SingleEnded(1);
  float voltageDrop = (rawValue - currentOffset) * ADC_SCALE_FACTOR / 1000.0f;  // mV na V
  float current = voltageDrop / SHUNT_RESISTOR;  // Ohmov zákon: I = V/R
  return current;
}

float readRawBatteryVoltage() {
  int16_t rawValue = ads.readADC_SingleEnded(2);
  float voltage = (rawValue - voltageOffset2) * ADC_SCALE_FACTOR / 1000.0f;
  voltage *= VOLTAGE_DIVIDER_RATIO;
  return voltage;
}

float readRawBatteryCurrent() {
  // Pri nabíjaní sa prúd meria rovnakým spôsobom
  return readRawPanelCurrent();  // Použije sa ten istý shunt
}

float readRawTemperature() {
  // Pre teplotný senzor LM35 alebo NTC
  int rawValue = analogRead(TEMP_PIN);
  float voltage = rawValue * (3.3 / 4095.0);  // ESP32 má 12-bit ADC
  
  // Pre LM35: 10mV/°C
  float temperature = voltage * 100.0f;
  
  // Pre NTC by bola potrebná konverzná tabuľka
  return temperature;
}

// ============================================
// IIR FILTER (EXPOENENCIÁLNY PRIEBEH)
// ============================================

float applyIIRFilter(float newValue, float oldValue, float alpha) {
  // IIR filter: y[n] = alpha * x[n] + (1-alpha) * y[n-1]
  return alpha * newValue + (1.0f - alpha) * oldValue;
}

// ============================================
// HLAVNÁ FUNKCIA PRE ČÍTANIE SENZOROV
// ============================================

void readAllSensors() {
  // Čítanie surových hodnôt
  float rawPanelV = readRawPanelVoltage();
  float rawPanelI = readRawPanelCurrent();
  float rawBatteryV = readRawBatteryVoltage();
  float rawBatteryI = readRawBatteryCurrent();
  float rawTemp = readRawTemperature();
  
  // Aplikovanie IIR filtrov
  filteredPanelVoltage = applyIIRFilter(rawPanelV, filteredPanelVoltage, ALPHA_VOLTAGE);
  filteredPanelCurrent = applyIIRFilter(rawPanelI, filteredPanelCurrent, ALPHA_CURRENT);
  filteredBatteryVoltage = applyIIRFilter(rawBatteryV, filteredBatteryVoltage, ALPHA_VOLTAGE);
  filteredBatteryCurrent = applyIIRFilter(rawBatteryI, filteredBatteryCurrent, ALPHA_CURRENT);
  filteredTemperature = applyIIRFilter(rawTemp, filteredTemperature, ALPHA_TEMP);
  
  // Ochrana proti záporným hodnotám prúdu (ak by nastala kalibračná chyba)
  if (filteredPanelCurrent < 0.0f) filteredPanelCurrent = 0.0f;
  if (filteredBatteryCurrent < 0.0f) filteredBatteryCurrent = 0.0f;
  
  // Výpočet výkonov
  panelPower = filteredPanelVoltage * filteredPanelCurrent;
  batteryPower = filteredBatteryVoltage * filteredBatteryCurrent;
  
  // Výpočet účinnosti (ak je výkon panelu > 0)
  if (panelPower > 0.1f) {  // Hranica 0.1W pre presnosť
    efficiency = (batteryPower / panelPower) * 100.0f;
    efficiency = constrain(efficiency, 0.0f, 100.0f);  // Obmedzenie na 0-100%
  } else {
    efficiency = 0.0f;
  }
}

// ============================================
// DIAGNOSTICKÉ FUNKCIE
// ============================================

void printSensorDiagnostics() {
  Serial.println("=== DIAGNOSTIKA SENZOROV ===");
  Serial.print("Panel napätie: "); Serial.print(filteredPanelVoltage, 2); Serial.println(" V");
  Serial.print("Panel prúd: "); Serial.print(filteredPanelCurrent, 3); Serial.println(" A");
  Serial.print("Panel výkon: "); Serial.print(panelPower, 2); Serial.println(" W");
  Serial.print("Batéria napätie: "); Serial.print(filteredBatteryVoltage, 2); Serial.println(" V");
  Serial.print("Batéria prúd: "); Serial.print(filteredBatteryCurrent, 3); Serial.println(" A");
  Serial.print("Batéria výkon: "); Serial.print(batteryPower, 2); Serial.println(" W");
  Serial.print("Teplota: "); Serial.print(filteredTemperature, 1); Serial.println(" °C");
  Serial.print("Účinnosť: "); Serial.print(efficiency, 1); Serial.println(" %");
  Serial.println("============================");
}

bool validateSensorReadings() {
  // Kontrola platnosti nameraných hodnôt
  bool isValid = true;
  
  if (filteredPanelVoltage > 60.0f || filteredPanelVoltage < 0.0f) {
    Serial.println("VAROVANIE: Panelové napätie mimo rozsahu!");
    isValid = false;
  }
  
  if (filteredBatteryVoltage > 20.0f || filteredBatteryVoltage < 0.0f) {
    Serial.println("VAROVANIE: Batériové napätie mimo rozsahu!");
    isValid = false;
  }
  
  if (filteredTemperature > 100.0f || filteredTemperature < -20.0f) {
    Serial.println("VAROVANIE: Teplota mimo rozsahu!");
    isValid = false;
  }
  
  return isValid;
}

// ============================================
// GETTER FUNKCIE PRE EXTERNÝ PRÍSTUP
// ============================================

float getPanelVoltage() { return filteredPanelVoltage; }
float getPanelCurrent() { return filteredPanelCurrent; }
float getBatteryVoltage() { return filteredBatteryVoltage; }
float getBatteryCurrent() { return filteredBatteryCurrent; }
float getTemperature() { return filteredTemperature; }
float getPanelPower() { return panelPower; }
float getBatteryPower() { return batteryPower; }
float getEfficiency() { return efficiency; }