// ============================================
// ČÍTANIE A FILTRÁCIA SENZOROV - MODUL 2
// ============================================

#include "config.h"
#include "globals.h"
#include <Adafruit_ADS1X15.h>

// ============================================
// LOKÁLNE PREMENNÉ MODULU (iba tu, nie v globals)
// ============================================

// POZOR: ads objekt je už definovaný v hlavnom súbore!
// Použijeme "extern" aby sme odkazovali na existujúci objekt
extern Adafruit_ADS1115 ads;  // SPRÁVNE - odkaz na existujúci objekt

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
  
  // POZOR: ads.begin() voláme na už existujúcom objekte
  if (!ads.begin()) {
    Serial.println("CHYBA: ADS1115 nenájdený! Skontrolujte zapojenie.");
    Serial.println("Postup riešenia:");
    Serial.println("1. Skontrolujte I2C zapojenie (SDA=21, SCL=22 pre ESP32)");
    Serial.println("2. Skontrolujte napájanie ADS1115 (3.3V a GND)");
    Serial.println("3. Skontrolujte adresu I2C (0x48 štandardne)");
    
    // Indikácia chyby blikaním LED
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
    
    // Pokračujeme aj s chybou, ale senzory nebudú fungovať
    Serial.println("VAROVANIE: Pokračujem bez ADS1115, hodnoty budú 0");
    return;
  }
  
  // Nastavenie zosilnenia ADS1115 na rozsah ±4.096V
  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 úspešne inicializovaný");
  
  // Automatická kalibrácia senzorov pri štarte
  calibrateSensors();
  
  Serial.println("Senzory pripravené na prácu");
}

// ... (zvyšok kódu zostáva rovnaký, je správny) ...