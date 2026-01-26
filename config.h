// ============================================
// KONFIGURAČNÝ SÚBOR PRE MPPT REGULÁTOR
// ============================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// HARDVÉROVÉ NASTAVENIA - GPIO PINTY
// ============================================

#define PWM_PIN 27                 // PWM výstupný pin pre MOSFET
#define PWM_CHANNEL 0              // Číslo PWM kanála ESP32
#define PWM_FREQUENCY 20000        // Frekvencia PWM 20kHz (bez zvuku)
#define PWM_RESOLUTION 10          // Rozlíšenie PWM 10-bit (0-1023)

#define LED_PIN 2                  // Stavová LED dióda
#define ERROR_LED_PIN 4            // LED pre indikáciu chýb
#define BATTERY_DISCONNECT_PIN 14  // Pin pre odpojenie batérie (relé/MOSFET)
#define PANEL_DISCONNECT_PIN 12    // Pin pre odpojenie panelu (relé/MOSFET)
#define TEMP_PIN 34                // Analógový vstup pre teplotný senzor

#define BUTTON_UP_PIN 32           // Tlačidlo HORE pre menu
#define BUTTON_DOWN_PIN 33         // Tlačidlo DOLE pre menu
#define BUTTON_ENTER_PIN 25        // Tlačidlo POTVRDIŤ pre menu
#define BUTTON_BACK_PIN 26         // Tlačidlo SPÄŤ pre menu

// ============================================
// ANALÓGOVÉ SENZORY - NASTAVENIA ADC
// ============================================

#define ADC_SCALE_FACTOR 0.125f    // Konverzný faktor ADS1115 (mV na digit)
#define SHUNT_RESISTOR 0.01f       // Hodnota shunt odporu pre meranie prúdu (Ω)
#define VOLTAGE_DIVIDER_RATIO 5.11f // Pomer deliča napätia (R1=39k, R2=10k)

// ============================================
// BEZPEČNOSTNÉ LIMITY - OCHRANNÉ PARAMETRE
// ============================================

// Batéria - napäťové limity pre AGM batériu 12V
#define BATTERY_VOLTAGE_CRITICAL_HIGH 15.0f  // Absolútne maximum! (V)
#define BATTERY_VOLTAGE_ABSORPTION 14.6f     // Absorpčné napätie (V)
#define BATTERY_VOLTAGE_FLOAT 13.8f          // Plávajúce napätie (V)
#define BATTERY_VOLTAGE_LOW 11.0f            // Varovanie pri nízkom napätí (V)
#define BATTERY_VOLTAGE_CRITICAL_LOW 10.0f   // Kritické minimum (V)
#define BATTERY_VOLTAGE_HYSTERESIS 0.2f      // Hysteréza pre prepínanie stavov (V)

// Prúdové limity - ochrana pred preťažením
#define MAX_CHARGE_CURRENT 5.0f              // Maximálny nabíjací prúd pre AGM (A)
#define MAX_DISCHARGE_CURRENT 30.0f          // Maximálny vybíjací prúd (A)
#define CURRENT_SHORT_CIRCUIT_THRESHOLD 50.0f // Prah pre detekciu zkratu (A)
#define MAX_CURRENT_RISE_RATE 100.0f         // Maximálna povolená rýchlosť nábehu prúdu (A/ms)

// Teplotné limity - ochrana pred prehriatím
#define TEMPERATURE_MAX 70.0f                // Maximálna teplota MOSFETu (°C)
#define TEMPERATURE_WARNING 60.0f            // Varovanie pri vysokej teplote (°C)
#define BATTERY_TEMP_MAX 50.0f               // Maximálna teplota batérie pre nabíjanie (°C)
#define BATTERY_TEMP_MIN -10.0f              // Minimálna teplota batérie pre nabíjanie (°C)

// Panelové napätie - vstupné parametre
#define PANEL_VOLTAGE_MAX 60.0f              // Maximálne vstupné napätie (V)
#define PANEL_VOLTAGE_MIN 10.0f              // Minimálne napätie pre fungovanie (V)

// ============================================
// FILTRE SIGNÁLOV - NASTAVENIA FILTRÁCIE
// ============================================

#define ALPHA_VOLTAGE 0.05f    // Koeficient filtra pre napätie (pomalá odozva)
#define ALPHA_CURRENT 0.1f     // Koeficient filtra pre prúd (stredná odozva)
#define ALPHA_TEMP 0.02f       // Koeficient filtra pre teplotu (veľmi pomalá odozva)

// ============================================
## **📁 4. 2_Read_Sensors.ino** (OPRAVENÝ)
```cpp
// ============================================
// ČÍTANIE A FILTRÁCIA SENZOROV - MODUL 2
// ============================================

#include "config.h"
#include "globals.h"
#include <Adafruit_ADS1X15.h>

// ============================================
// LOKÁLNE PREMENNÉ MODULU
// ============================================

Adafruit_ADS1115 ads;  // Objekt pre ADC prevodník ADS1115

// Kalibračné offsety pre senzory
float currentOffset = 0.0f;      // Offset pre prúdový senzor
float voltageOffset1 = 0.0f;     // Offset pre napätie panelu (kanál 0)
float voltageOffset2 = 0.0f;     // Offset pre napätie batérie (kanál 2)

// Filtrované hodnoty senzorov
float filteredPanelVoltage = 0.0f;   // Filtrované napätie panelu
float filteredPanelCurrent = 0.0f;   // Filtrovaný prúd panelu
float filteredBatteryVoltage = 0.0f; // Filtrované napätie batérie
float filteredBatteryCurrent = 0.0f; // Filtrovaný prúd batérie
float filteredTemperature = 0.0f;    // Filtrovaná teplota

// ============================================
// INICIALIZÁCIA SENZOROV - FUNKCIA
// ============================================

void initSensors() {
  Serial.println("Inicializácia senzorov...");
  
  // Pokus o inicializáciu ADS1115 ADC prevodníka
  if (!ads.begin()) {
    Serial.println("CHYBA: ADS1115 nenájdený! Skontrolujte zapojenie.");
    while (1) { // Zastavenie programu - kritická chyba
      delay(1000);
    }
  }
  
  // Nastavenie zosilnenia ADS1115 na rozsah ±4.096V
  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 úspešne inicializovaný");
  
  // Automatická kalibrácia senzorov pri štarte
  calibrateSensors();
  
  Serial.println("Senzory pripravené na prácu");
}

// ============================================
// KALIBRÁCIA SENZOROV - FUNKCIA
// ============================================

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

// ============================================
// ČÍTANIE SUROVÝCH HODNÔT - POMOCNÉ FUNKCIE
// ============================================

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
  
  // Poznámka: Pre NTC termistor by bola potrebná konverzná tabuľka
  return temperature;
}

// ============================================
## **Pokračujem s ďalšími súbormi...**

Chcete, aby som pokračoval s kompletne opravenými súbormi s vysvetľujúcimi komentármi v slovenčine? Mám pripravené všetkých 8 súborov plus globals.h, globals.cpp a config.h.