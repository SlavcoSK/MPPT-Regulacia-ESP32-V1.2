// ============================================
// DISPLEJOVÝ SYSTÉM A MENU OVLÁDANIE
// ============================================

#include <U8g2lib.h>  // Knižnica pre OLED displej

// ============================================
// KONFIGURÁCIA DISPLEJA
// ============================================

// Inicializácia displeja (SSD1306 128x64)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Menu stav
enum MenuState {
  MENU_MAIN,           // Hlavné menu
  MENU_STATUS,         // Stavový displej
  MENU_CHARGING,       // Detaily nabíjania
  MENU_STATS,          // Štatistiky
  MENU_SETTINGS,       // Nastavenia
  MENU_BATTERY_SETUP,  // Nastavenie batérie
  MENU_SYSTEM_INFO,    // Informácie o systéme
  MENU_DIAGNOSTICS,    // Diagnostika
  MENU_CALIBRATION     // Kalibrácia
};

MenuState currentMenu = MENU_STATUS;
MenuState previousMenu = MENU_STATUS;

// Navigácia v menu
int menuCursor = 0;
int menuScroll = 0;
const int MAX_MENU_ITEMS = 8;

// Tlačidlá
#define BUTTON_UP_PIN     32
#define BUTTON_DOWN_PIN   33
#define BUTTON_ENTER_PIN  25
#define BUTTON_BACK_PIN   26

// Stav tlačidiel
bool buttonUpPressed = false;
bool buttonDownPressed = false;
bool buttonEnterPressed = false;
bool buttonBackPressed = false;

// Časové premenné pre debouncing
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ============================================
// INICIALIZÁCIA DISPLEJA A TLAČIDIEL
// ============================================

void initLCDMenu() {
  Serial.println("Inicializácia displeja a menu...");
  
  // Inicializácia displeja
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);  // Štandardné písmo
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  
  // Inicializácia tlačidiel
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ENTER_PIN, INPUT_PULLUP);
  pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);
  
  // Úvodná obrazovka
  showSplashScreen();
  
  Serial.println("Displej a menu inicializované");
}

void showSplashScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_tf);
  
  // Nadpis
  u8g2.drawStr(10, 10, "MPPT");
  u8g2.drawStr(10, 30, "REGULATOR");
  
  // Verzia
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(30, 50, "Verzia 1.0");
  
  u8g2.sendBuffer();
  delay(2000);
}

// ============================================
// SPRACOVANIE TLAČIDIEL
// ============================================

void readButtons() {
  unsigned long currentTime = millis();
  
  // Debouncing kontrola
  if (currentTime - lastButtonPress < DEBOUNCE_DELAY) {
    return;
  }
  
  // Čítanie tlačidiel
  bool upPressed = (digitalRead(BUTTON_UP_PIN) == LOW);
  bool downPressed = (digitalRead(BUTTON_DOWN_PIN) == LOW);
  bool enterPressed = (digitalRead(BUTTON_ENTER_PIN) == LOW);
  bool backPressed = (digitalRead(BUTTON_BACK_PIN) == LOW);
  
  // Detekcia stlačenia
  if (upPressed && !buttonUpPressed) {
    handleButtonUp();
    lastButtonPress = currentTime;
  }
  
  if (downPressed && !buttonDownPressed) {
    handleButtonDown();
    lastButtonPress = currentTime;
  }
  
  if (enterPressed && !buttonEnterPressed) {
    handleButtonEnter();
    lastButtonPress = currentTime;
  }
  
  if (backPressed && !buttonBackPressed) {
    handleButtonBack();
    lastButtonPress = currentTime;
  }
  
  // Uloženie stavu
  buttonUpPressed = upPressed;
  buttonDownPressed = downPressed;
  buttonEnterPressed = enterPressed;
  buttonBackPressed = backPressed;
}

void handleButtonUp() {
  switch(currentMenu) {
    case MENU_MAIN:
    case MENU_SETTINGS:
    case MENU_BATTERY_SETUP:
      // Posun kurzora hore
      menuCursor--;
      if (menuCursor < 0) {
        menuCursor = getMenuItemsCount() - 1;
      }
      break;
      
    case MENU_STATS:
      // Scroll hore
      menuScroll = max(menuScroll - 1, 0);
      break;
  }
  
  updateDisplay();
}

void handleButtonDown() {
  switch(currentMenu) {
    case MENU_MAIN:
    case MENU_SETTINGS:
    case MENU_BATTERY_SETUP:
      // Posun kurzora dole
      menuCursor++;
      if (menuCursor >= getMenuItemsCount()) {
        menuCursor = 0;
      }
      break;
      
    case MENU_STATS:
      // Scroll dole
      menuScroll++;
      break;
  }
  
  updateDisplay();
}

void handleButtonEnter() {
  switch(currentMenu) {
    case MENU_MAIN:
      // Vstup do vybraného menu
      enterSelectedMenu();
      break;
      
    case MENU_SETTINGS:
    case MENU_BATTERY_SETUP:
      // Spracovanie výberu v nastaveniach
      processMenuSelection();
      break;
      
    case MENU_STATUS:
    case MENU_CHARGING:
      // Prepnúť na hlavné menu
      changeMenu(MENU_MAIN);
      break;
  }
  
  updateDisplay();
}

void handleButtonBack() {
  switch(currentMenu) {
    case MENU_MAIN:
      // Naspäť na stavový displej
      changeMenu(MENU_STATUS);
      break;
      
    case MENU_SETTINGS:
    case MENU_BATTERY_SETUP:
    case MENU_STATS:
    case MENU_SYSTEM_INFO:
    case MENU_DIAGNOSTICS:
    case MENU_CALIBRATION:
      // Naspäť do hlavného menu
      changeMenu(MENU_MAIN);
      break;
      
    case MENU_STATUS:
    case MENU_CHARGING:
      // Nič (už sme v hlavnom zobrazení)
      break;
  }
  
  updateDisplay();
}

// ============================================
// RIADENIE MENU
// ============================================

void changeMenu(MenuState newMenu) {
  if (currentMenu == newMenu) return;
  
  previousMenu = currentMenu;
  currentMenu = newMenu;
  
  // Resetovanie kurzora a scrollu
  menuCursor = 0;
  menuScroll = 0;
  
  Serial.print("Zmena menu: ");
  Serial.println(getMenuName(currentMenu));
}

String getMenuName(MenuState menu) {
  switch(menu) {
    case MENU_MAIN: return "HLAVNE_MENU";
    case MENU_STATUS: return "STAV";
    case MENU_CHARGING: return "NABIJANIE";
    case MENU_STATS: return "STATISTIKY";
    case MENU_SETTINGS: return "NASTAVENIA";
    case MENU_BATTERY_SETUP: return "BATERIA";
    case MENU_SYSTEM_INFO: return "SYSTEM";
    case MENU_DIAGNOSTICS: return "DIAGNOSTIKA";
    case MENU_CALIBRATION: return "KALIBRACIA";
    default: return "NEZNAME";
  }
}

int getMenuItemsCount() {
  switch(currentMenu) {
    case MENU_MAIN: return 6;
    case MENU_SETTINGS: return 5;
    case MENU_BATTERY_SETUP: return 4;
    default: return 0;
  }
}

void enterSelectedMenu() {
  switch(menuCursor) {
    case 0: changeMenu(MENU_STATUS); break;
    case 1: changeMenu(MENU_CHARGING); break;
    case 2: changeMenu(MENU_STATS); break;
    case 3: changeMenu(MENU_SETTINGS); break;
    case 4: changeMenu(MENU_SYSTEM_INFO); break;
    case 5: changeMenu(MENU_DIAGNOSTICS); break;
  }
}

void processMenuSelection() {
  // Spracovanie výberu v nastaveniach
  if (currentMenu == MENU_SETTINGS) {
    switch(menuCursor) {
      case 0: changeMenu(MENU_BATTERY_SETUP); break;
      case 1: toggleLowPowerMode(); break;
      case 2: calibrateSensors(); break;
      case 3: resetStatistics(); break;
      case 4: rebootSystem(); break;
    }
  }
  
  if (currentMenu == MENU_BATTERY_SETUP) {
    setBatteryType((BatteryType)menuCursor);
    changeMenu(MENU_SETTINGS);
  }
}

// ============================================
// ZOBRAZOVACIE FUNKCIE
// ============================================

void updateDisplay() {
  // Aktualizácia displeja podľa aktuálneho menu
  switch(currentMenu) {
    case MENU_STATUS:
      displayStatusScreen();
      break;
      
    case MENU_CHARGING:
      displayChargingScreen();
      break;
      
    case MENU_STATS:
      displayStatsScreen();
      break;
      
    case MENU_MAIN:
      displayMainMenu();
      break;
      
    case MENU_SETTINGS:
      displaySettingsMenu();
      break;
      
    case MENU_BATTERY_SETUP:
      displayBatteryMenu();
      break;
      
    case MENU_SYSTEM_INFO:
      displaySystemInfo();
      break;
      
    case MENU_DIAGNOSTICS:
      displayDiagnostics();
      break;
      
    case MENU_CALIBRATION:
      displayCalibration();
      break;
  }
}

void displayStatusScreen() {
  u8g2.clearBuffer();
  
  // Hlavička
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "MPPT REGULATOR");
  
  // Stav systému
  u8g2.drawStr(0, 12, "Stav:");
  u8g2.drawStr(40, 12, getStateName(currentSystemState).c_str());
  
  // Fáza nabíjania
  u8g2.drawStr(0, 24, "Faza:");
  u8g2.drawStr(40, 24, getPhaseName(currentChargePhase).c_str());
  
  // Batéria
  char buffer[32];
  sprintf(buffer, "Bat: %.1fV %.1fA", batteryVoltage, batteryCurrent);
  u8g2.drawStr(0, 36, buffer);
  
  // Panel
  sprintf(buffer, "Panel: %.1fV %.1fW", panelVoltage, panelPower);
  u8g2.drawStr(0, 48, buffer);
  
  // Účinnosť a teplota
  sprintf(buffer, "Ucin: %.1f%%  %.1fC", efficiency, temperature);
  u8g2.drawStr(0, 60, buffer);
  
  // Indikátor menu
  u8g2.drawStr(110, 60, "MENU");
  
  u8g2.sendBuffer();
}

void displayChargingScreen() {
  u8g2.clearBuffer();
  
  // Nadpis
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "DETAIL NABIJANIA");
  
  // PWM a MPPT
  char buffer[32];
  sprintf(buffer, "PWM: %.1f%%", pwmDutyCycle);
  u8g2.drawStr(0, 12, buffer);
  
  sprintf(buffer, "MPPT: %s", 
    mpptState == MPPT_TRACKING ? "SLEDUJE" : 
    mpptState == MPPT_SCANNING ? "SKENUJE" : "DRZI");
  u8g2.drawStr(70, 12, buffer);
  
  // Prúdy
  sprintf(buffer, "Ipanel: %.2f A", panelCurrent);
  u8g2.drawStr(0, 24, buffer);
  
  sprintf(buffer, "Ibat: %.2f A", batteryCurrent);
  u8g2.drawStr(0, 36, buffer);
  
  // Napätia
  sprintf(buffer, "Upanel: %.1f V", panelVoltage);
  u8g2.drawStr(0, 48, buffer);
  
  sprintf(buffer, "Ubat: %.1f V", batteryVoltage);
  u8g2.drawStr(0, 60, buffer);
  
  u8g2.sendBuffer();
}

void displayStatsScreen() {
  u8g2.clearBuffer();
  
  // Nadpis
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "STATISTIKY");
  
  char buffer[32];
  int line = 12;
  
  // Denné štatistiky
  sprintf(buffer, "Energia: %.0f Wh", dailyStats.totalEnergy);
  u8g2.drawStr(0, line); line += 12;
  
  sprintf(buffer, "Max vykon: %.0f W", dailyStats.maxPower);
  u8g2.drawStr(0, line); line += 12;
  
  sprintf(buffer, "Cyklov: %d", dailyStats.chargeCycles);
  u8g2.drawStr(0, line); line += 12;
  
  sprintf(buffer, "Ucin: %.1f %%", dailyStats.avgEfficiency);
  u8g2.drawStr(0, line); line += 12;
  
  // Batéria
  sprintf(buffer, "Ubat min: %.1f V", dailyStats.minBatteryVoltage);
  u8g2.drawStr(0, line); line += 12;
  
  sprintf(buffer, "Ubat max: %.1f V", dailyStats.maxBatteryVoltage);
  u8g2.drawStr(0, line);
  
  // Indikátor scrollu
  if (menuScroll > 0) {
    u8g2.drawStr(110, 60, "^");
  }
  
  u8g2.sendBuffer();
}

void displayMainMenu() {
  u8g2.clearBuffer();
  
  // Nadpis
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "HLAVNE MENU");
  
  // Položky menu
  const char* menuItems[] = {
    "Stav systemu",
    "Detaily nabijania",
    "Statistiky",
    "Nastavenia",
    "Info o systeme",
    "Diagnostika"
  };
  
  // Zobrazenie položiek
  for (int i = 0; i < 6; i++) {
    int yPos = 12 + i * 10;
    
    // Kurzor
    if (i == menuCursor) {
      u8g2.drawStr(0, yPos, ">");
    }
    
    // Text položky
    u8g2.drawStr(8, yPos, menuItems[i]);
  }
  
  // Návod
  u8g2.drawStr(0, 60, "ENTER-vyber  BACK-spat");
  
  u8g2.sendBuffer();
}

void displaySettingsMenu() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "NASTAVENIA");
  
  const char* menuItems[] = {
    "Typ baterie",
    "Setrny rezim",
    "Kalibracia",
    "Reset statistik",
    "Reboot systemu"
  };
  
  // Zobrazenie položiek
  for (int i = 0; i < 5; i++) {
    int yPos = 12 + i * 10;
    
    // Kurzor
    if (i == menuCursor) {
      u8g2.drawStr(0, yPos, ">");
    }
    
    // Text položky
    u8g2.drawStr(8, yPos, menuItems[i]);
    
    // Aktuálne hodnoty
    if (i == 0) {
      u8g2.drawStr(70, yPos, currentProfile->batteryName.c_str());
    } else if (i == 1) {
      u8g2.drawStr(70, yPos, lowPowerMode ? "ZAPNUTY" : "VYPNUTY");
    }
  }
  
  u8g2.drawStr(0, 60, "ENTER-vyber  BACK-spat");
  
  u8g2.sendBuffer();
}

void displayBatteryMenu() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "TYP BATERIE");
  
  const char* batteryTypes[] = {
    "AGM (test)",
    "LiFePO4",
    "Olova kysla",
    "Vlastny"
  };
  
  // Zobrazenie typov
  for (int i = 0; i < 4; i++) {
    int yPos = 12 + i * 12;
    
    // Kurzor
    if (i == menuCursor) {
      u8g2.drawStr(0, yPos, ">");
    }
    
    // Typ batérie
    u8g2.drawStr(8, yPos, batteryTypes[i]);
    
    // Zvýraznenie aktuálneho typu
    if (i == (int)selectedBatteryType) {
      u8g2.drawStr(100, yPos, "[AKT]");
    }
  }
  
  // Parametre vybraného typu
  int yInfo = 60;
  u8g2.drawStr(0, yInfo, "Max prud:");
  char buffer[16];
  sprintf(buffer, "%.1fA", currentProfile->maxChargeCurrent);
  u8g2.drawStr(60, yInfo, buffer);
  
  u8g2.sendBuffer();
}

void displaySystemInfo() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "INFO O SYSTEME");
  
  char buffer[32];
  int line = 12;
  
  // Verzia
  u8g2.drawStr(0, line, "Verzia: 1.0"); line += 10;
  
  // Čas behu
  float uptimeHours = millis() / 3600000.0;
  sprintf(buffer, "Uptime: %.1f h", uptimeHours);
  u8g2.drawStr(0, line); line += 10;
  
  // WiFi stav
  u8g2.drawStr(0, line, "WiFi:");
  u8g2.drawStr(30, line, wifiState == WIFI_CONNECTED ? "Pripojene" : "Nepripojene");
  line += 10;
  
  // Pamäť
  sprintf(buffer, "Pamät: %d B", ESP.getFreeHeap());
  u8g2.drawStr(0, line); line += 10;
  
  // Batéria
  sprintf(buffer, "Typ: %s", currentProfile->batteryName.c_str());
  u8g2.drawStr(0, line); line += 10;
  
  // CPU frekvencia
  sprintf(buffer, "CPU: %d MHz", getCpuFrequencyMhz());
  u8g2.drawStr(0, line);
  
  u8g2.sendBuffer();
}

// ============================================
// AKCIE MENU
// ============================================

void toggleLowPowerMode() {
  if (lowPowerMode) {
    exitLowPowerMode();
  } else {
    enterLowPowerMode();
  }
  
  // Vrátiť sa do menu
  updateDisplay();
}

void resetStatistics() {
  clearTelemetryHistory();
  resetDailyStats();
  
  // Potvrdenie
  u8g2.clearBuffer();
  u8g2.drawStr(0, 20, "Statistiky");
  u8g2.drawStr(0, 30, "vycistene!");
  u8g2.sendBuffer();
  delay(1000);
  
  changeMenu(MENU_SETTINGS);
}

void rebootSystem() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 20, "Reboot systemu...");
  u8g2.sendBuffer();
  delay(1000);
  
  ESP.restart();
}

// ============================================
// DIAGNOSTICKÉ ZOBRAZENIE
// ============================================

void displayDiagnostics() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "DIAGNOSTIKA");
  
  char buffer[32];
  int line = 12;
  
  // ADC hodnoty
  sprintf(buffer, "ADC0: %d", ads.readADC_SingleEnded(0));
  u8g2.drawStr(0, line); line += 10;
  
  sprintf(buffer, "ADC1: %d", ads.readADC_SingleEnded(1));
  u8g2.drawStr(0, line); line += 10;
  
  sprintf(buffer, "ADC2: %d", ads.readADC_SingleEnded(2));
  u8g2.drawStr(0, line); line += 10;
  
  // Chybové vlajky
  u8g2.drawStr(0, line, "Chyby:");
  line += 10;
  
  if (overVoltageFlag) {
    u8g2.drawStr(0, line, "Vysoke napatie"); line += 10;
  }
  if (underVoltageFlag) {
    u8g2.drawStr(0, line, "Nizke napatie"); line += 10;
  }
  if (overTempFlag) {
    u8g2.drawStr(0, line, "Vysoka teplota"); line += 10;
  }
  
  // Stav MPPT
  sprintf(buffer, "MPPT: %d", mpptState);
  u8g2.drawStr(0, line);
  
  u8g2.sendBuffer();
}

void displayCalibration() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 0, "KALIBRACIA");
  
  u8g2.drawStr(0, 15, "Odpojte vsetky");
  u8g2.drawStr(0, 25, "vstupy a vystupy!");
  u8g2.drawStr(0, 40, "ENTER - zacat");
  u8g2.drawStr(0, 50, "BACK - zrusit");
  
  u8g2.sendBuffer();
}

// ============================================
// AUTOMATICKÁ AKTUALIZÁCIA DISPLEJA
// ============================================

void updateDisplayPeriodically() {
  static unsigned long lastDisplayUpdate = 0;
  unsigned long currentTime = millis();
  
  // Aktualizovať displej každých 500ms
  if (currentTime - lastDisplayUpdate >= 500) {
    // Iba ak nie sme v menu s kurzorom
    if (currentMenu == MENU_STATUS || currentMenu == MENU_CHARGING) {
      updateDisplay();
    }
    lastDisplayUpdate = currentTime;
  }
}

// ============================================
// HLAVNÁ FUNKCIA DISPLEJOVÉHO SYSTÉMU
// ============================================

void runLCDMenuSystem() {
  // 1. Čítanie tlačidiel
  readButtons();
  
  // 2. Periodická aktualizácia
  updateDisplayPeriodically();
  
  // 3. Automatický návrat do stavového displeja
  static unsigned long lastInteraction = 0;
  if (currentMenu != MENU_STATUS) {
    if (millis() - lastInteraction > 30000) {  // 30 sekúnd nečinnosti
      changeMenu(MENU_STATUS);
    }
  } else {
    lastInteraction = millis();
  }
  
  // 4. Špeciálne efekty pre chybové stavy
  if (currentSystemState == STATE_FAULT && currentMenu == MENU_STATUS) {
    displayFaultBlink();
  }
}

void displayFaultBlink() {
  // Blikanie pri chybovom stave
  static unsigned long lastBlink = 0;
  static bool blinkState = false;
  
  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    
    if (blinkState) {
      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 0, 128, 64);
      u8g2.setDrawColor(1);
      u8g2.drawStr(10, 20, "CHYBOVY STAV!");
      u8g2.drawStr(10, 35, "Skontrolujte");
      u8g2.drawStr(10, 45, "pripojenia");
      u8g2.sendBuffer();
    } else {
      updateDisplay();
    }
    
    lastBlink = millis();
  }
}