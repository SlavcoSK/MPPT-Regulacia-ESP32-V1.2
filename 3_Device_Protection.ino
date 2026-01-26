// Pridajte tieto funkcie na KONIEC súboru:

// ============================================
// LOGOVANIE A ULOŽENIE CHÝB - FUNKCIE
// ============================================

void addToEventLog(const char* message) {
  // Pridanie udalosti do záznamu udalostí
  
  Serial.print("[EVENT_LOG] ");
  Serial.println(message);
  
  // Tu by sa udalosť ukladala do EEPROM alebo SD karty
  // Napríklad: EEPROM.write(eventLogAddress++, message);
}

void saveErrorToLog(const char* message) {
  // Uloženie chyby do chybového logu
  
  Serial.print("[ERROR_LOG] ");
  Serial.println(message);
  
  // Implementácia ukladania chýb do EEPROM
  static int errorLogIndex = 0;
  const int MAX_ERRORS = 50;
  
  // Tu by sa chyba ukladala do EEPROM
  // EEPROM.put(ERROR_LOG_START + errorLogIndex * sizeof(ErrorEntry), error);
  errorLogIndex = (errorLogIndex + 1) % MAX_ERRORS;
}

void saveErrorToEEPROM(const char* reason) {
  // Uloženie dôvodu núdzového vypnutia do EEPROM
  
  Serial.print("[EEPROM_ERROR] Ukladám chybu: ");
  Serial.println(reason);
  
  // Uloženie časovej značky
  unsigned long errorTime = millis();
  EEPROM.put(0, errorTime);
  
  // Uloženie dôvodu (ako text)
  int address = 4;  // Za 4 bajtami pre čas
  for (int i = 0; i < 50 && reason[i] != '\0'; i++) {
    EEPROM.write(address + i, reason[i]);
  }
  EEPROM.write(address + 50, '\0');  // Ukončenie reťazca
  
  EEPROM.commit();
  Serial.println("Chyba úspešne uložená do EEPROM");
}

void updateDisplayWarning(const char* message) {
  // Aktualizácia displeja s varovaním
  
  Serial.print("[DISPLAY_WARNING] ");
  Serial.println(message);
  
  // Zobrazenie varovania na OLED displeji
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // Nadpis
  u8g2.drawStr(0, 10, "VAROVANIE");
  
  // Správa (max 20 znakov na riadok)
  char displayMsg[21];
  strncpy(displayMsg, message, 20);
  displayMsg[20] = '\0';
  u8g2.drawStr(0, 25, displayMsg);
  
  // Ak je správa dlhšia, zobraziť druhý riadok
  if (strlen(message) > 20) {
    strncpy(displayMsg, message + 20, 20);
    displayMsg[20] = '\0';
    u8g2.drawStr(0, 40, displayMsg);
  }
  
  u8g2.drawStr(0, 55, "Stlačte OK pre pokr.");
  
  u8g2.sendBuffer();
}

// ============================================
// POMOCNÉ OCHRANNÉ FUNKCIE
// ============================================

void checkForReset() {
  // Kontrola podmienok pre reset systému
  
  static unsigned long lastButtonCheck = 0;
  
  if (millis() - lastButtonCheck > 100) {
    // Skontrolovať tlačidlo reset
    if (digitalRead(BUTTON_ENTER_PIN) == LOW) {
      static unsigned long buttonPressTime = 0;
      
      if (buttonPressTime == 0) {
        buttonPressTime = millis();
      } else if (millis() - buttonPressTime > 3000) {
        // Tlačidlo držané 3 sekundy - vykonať reset
        Serial.println("MANUÁLNY RESET - reštartujem systém...");
        delay(100);
        ESP.restart();
      }
    } else {
      buttonPressTime = 0;
    }
    
    lastButtonCheck = millis();
  }
}

void displayFaultMessage() {
  // Zobrazenie chybového hlásenia na displeji
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  u8g2.drawStr(0, 10, "CHYBOVY STAV!");
  u8g2.drawStr(0, 25, "Skontrolujte:");
  u8g2.drawStr(0, 35, "- Napätie batérie");
  u8g2.drawStr(0, 45, "- Teplotu systému");
  u8g2.drawStr(0, 55, "- Spojenia");
  
  u8g2.sendBuffer();
}