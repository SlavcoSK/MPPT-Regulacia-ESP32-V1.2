// ============================================
// BEZDRÔTOVÁ TELEMETRIA A KOMUNIKÁCIA
// ============================================

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>  // Knižnica pre Blynk

// ============================================
// KONFIGURÁCIA SIETE
// ============================================

// WiFi nastavenia
char wifiSSID[] = "Vase_WiFi_SSID";
char wifiPassword[] = "Vase_WiFi_Heslo";

// Blynk nastavenia
char blynkAuth[] = "Vas_Blynk_Auth_Token";
char blynkServer[] = "blynk.cloud";
uint16_t blynkPort = 8080;

// Stav WiFi
enum WiFiState {
  WIFI_DISCONNECTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_ERROR
};

WiFiState wifiState = WIFI_DISCONNECTED;

// Časové premenné
unsigned long lastWiFiAttempt = 0;
unsigned long lastBlynkSync = 0;
unsigned long wifiReconnectInterval = 30000;  // 30 sekúnd

// ============================================
// BLYNK VIRTUÁLNE PINTY
// ============================================

// Virtuálne piny pre dátové streamy
#define BLYNK_VPANEL_VOLTAGE    V0
#define BLYNK_VPANEL_CURRENT    V1
#define BLYNK_VPANEL_POWER      V2
#define BLYNK_VBATTERY_VOLTAGE  V3
#define BLYNK_VBATTERY_CURRENT  V4
#define BLYNK_VBATTERY_POWER    V5
#define BLYNK_VEFFICIENCY       V6
#define BLYNK_VTEMPERATURE      V7
#define BLYNK_VPWM_DUTY         V8
#define BLYNK_VCHARGE_PHASE     V9
#define BLYNK_VSYSTEM_STATE     V10
#define BLYNK_VERROR_FLAGS      V11
#define BLYNK_VDAILY_ENERGY     V12
#define BLYNK_VMAX_POWER        V13
#define BLYNK_VBATTERY_TYPE     V14
#define BLYNK_VLOW_POWER_MODE   V15

// Piny pre ovládanie
#define BLYNK_VSET_BATTERY_TYPE V20
#define BLYNK_VSET_MAX_CURRENT  V21
#define BLYNK_VSET_PWM_MANUAL   V22
#define BLYNK_VRESET_STATS      V23
#define BLYNK_VCLEAR_ERRORS     V24
#define BLYNK_VREBOOT           V25

// ============================================
// INICIALIZÁCIA BEZDRÔTOVEJ KOMUNIKÁCIE
// ============================================

void initWirelessTelemetry() {
  Serial.println("Inicializácia bezdrôtovej telemetrie...");
  
  // Nastavenie WiFi módu
  WiFi.mode(WIFI_STA);
  
  // Pokus o pripojenie k WiFi
  connectToWiFi();
  
  // Inicializácia Blynk (ak je WiFi pripojené)
  if (wifiState == WIFI_CONNECTED) {
    initBlynk();
  }
  
  Serial.println("Bezdrôtová telemetria pripravená");
}

// ============================================
// SPRÁVA WIFI PRIPOJENIA
// ============================================

void connectToWiFi() {
  Serial.print("Pripojenie k WiFi: ");
  Serial.println(wifiSSID);
  
  wifiState = WIFI_CONNECTING;
  lastWiFiAttempt = millis();
  
  WiFi.begin(wifiSSID, wifiPassword);
  
  // Čakanie na pripojenie (s timeoutom)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiState = WIFI_CONNECTED;
    Serial.println("\nWiFi pripojené!");
    Serial.print("IP adresa: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    wifiState = WIFI_ERROR;
    Serial.println("\nCHYBA: Nepodarilo sa pripojiť k WiFi!");
  }
}

void checkWiFiConnection() {
  if (wifiState != WIFI_CONNECTED && 
      millis() - lastWiFiAttempt > wifiReconnectInterval) {
    Serial.println("Pokus o opätovné pripojenie k WiFi...");
    connectToWiFi();
  }
  
  // Ak sme stratili spojenie
  if (wifiState == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi spojenie stratene!");
    wifiState = WIFI_DISCONNECTED;
  }
}

// ============================================
// BLYNK KOMUNIKÁCIA
// ============================================

void initBlynk() {
  Serial.println("Inicializácia Blynk...");
  
  Blynk.config(blynkAuth, blynkServer, blynkPort);
  
  // Test spojenia
  if (Blynk.connect()) {
    Serial.println("Blynk pripojený!");
    
    // Odoslanie počiatočných hodnôt
    sendInitialBlynkData();
    
    // Nastavenie callbackov
    setupBlynkCallbacks();
  } else {
    Serial.println("CHYBA: Nepodarilo sa pripojiť k Blynk!");
  }
}

void sendInitialBlynkData() {
  // Odoslanie počiatočných hodnôt po pripojení
  
  // Nastavenia batérie
  Blynk.virtualWrite(BLYNK_VBATTERY_TYPE, currentProfile->batteryName);
  Blynk.virtualWrite(BLYNK_VSET_MAX_CURRENT, currentProfile->maxChargeCurrent);
  
  // Aktuálny stav
  updateBlynkDisplay();
  
  // Denné štatistiky
  Blynk.virtualWrite(BLYNK_VDAILY_ENERGY, dailyStats.totalEnergy);
  Blynk.virtualWrite(BLYNK_VMAX_POWER, dailyStats.maxPower);
  
  Serial.println("Počiatočné dáta odoslané do Blynk");
}

// ============================================
// AKTUALIZÁCIA BLYNK DISPLEJA
// ============================================

void updateBlynkDisplay() {
  // Iba ak sme pripojený
  if (wifiState != WIFI_CONNECTED || !Blynk.connected()) {
    return;
  }
  
  // Rýchle dáta (každú sekundu)
  Blynk.virtualWrite(BLYNK_VPANEL_VOLTAGE, panelVoltage);
  Blynk.virtualWrite(BLYNK_VPANEL_CURRENT, panelCurrent);
  Blynk.virtualWrite(BLYNK_VPANEL_POWER, panelPower);
  Blynk.virtualWrite(BLYNK_VBATTERY_VOLTAGE, batteryVoltage);
  Blynk.virtualWrite(BLYNK_VBATTERY_CURRENT, batteryCurrent);
  Blynk.virtualWrite(BLYNK_VBATTERY_POWER, batteryPower);
  Blynk.virtualWrite(BLYNK_VEFFICIENCY, efficiency);
  Blynk.virtualWrite(BLYNK_VTEMPERATURE, temperature);
  Blynk.virtualWrite(BLYNK_VPWM_DUTY, pwmDutyCycle);
  
  // Stavové informácie
  Blynk.virtualWrite(BLYNK_VCHARGE_PHASE, getPhaseName(currentChargePhase));
  Blynk.virtualWrite(BLYNK_VSYSTEM_STATE, getStateName(currentSystemState));
  Blynk.virtualWrite(BLYNK_VLOW_POWER_MODE, lowPowerMode ? "ANO" : "NIE");
  
  // Chybové vlajky
  String errorString = "";
  if (overVoltageFlag) errorString += "VYSOKE_NAP ";
  if (underVoltageFlag) errorString += "NIZKE_NAP ";
  if (overTempFlag) errorString += "VYS_TEP ";
  if (shortCircuitFlag) errorString += "ZKART ";
  Blynk.virtualWrite(BLYNK_VERROR_FLAGS, errorString);
}

void sendBlynkStatistics() {
  // Odoslanie štatistík (menej často)
  Blynk.virtualWrite(BLYNK_VDAILY_ENERGY, dailyStats.totalEnergy);
  Blynk.virtualWrite(BLYNK_VMAX_POWER, dailyStats.maxPower);
  
  // Informácia o batérii
  Blynk.virtualWrite(BLYNK_VBATTERY_TYPE, currentProfile->batteryName);
}

// ============================================
// BLYNK CALLBACK FUNKCIE (OVLÁDANIE)
// ============================================

void setupBlynkCallbacks() {
  // Nastavenie callbackov pre virtuálne piny
  
  // Zmena typu batérie
  Blynk.virtualWrite(BLYNK_VSET_BATTERY_TYPE, selectedBatteryType);
  BLYNK_WRITE(BLYNK_VSET_BATTERY_TYPE) {
    int batteryType = param.asInt();
    if (batteryType >= 0 && batteryType <= 3) {
      setBatteryType((BatteryType)batteryType);
      Blynk.virtualWrite(BLYNK_VBATTERY_TYPE, currentProfile->batteryName);
    }
  }
  
  // Zmena maximálneho prúdu
  Blynk.virtualWrite(BLYNK_VSET_MAX_CURRENT, currentProfile->maxChargeCurrent);
  BLYNK_WRITE(BLYNK_VSET_MAX_CURRENT) {
    float maxCurrent = param.asFloat();
    if (maxCurrent > 0 && maxCurrent <= 30.0f) {
      currentProfile->maxChargeCurrent = maxCurrent;
      Serial.print("Max prúd nastavený na: ");
      Serial.print(maxCurrent);
      Serial.println("A (cez Blynk)");
    }
  }
  
  // Manuálne ovládanie PWM
  BLYNK_WRITE(BLYNK_VSET_PWM_MANUAL) {
    float manualPWM = param.asFloat();
    if (manualPWM >= 0 && manualPWM <= 100) {
      setManualPWM(manualPWM);
    }
  }
  
  // Reset štatistík
  BLYNK_WRITE(BLYNK_VRESET_STATS) {
    if (param.asInt() == 1) {
      resetDailyStats();
      Blynk.virtualWrite(BLYNK_VDAILY_ENERGY, 0);
      Blynk.virtualWrite(BLYNK_VMAX_POWER, 0);
      Serial.println("Štatistiky resetované (cez Blynk)");
    }
  }
  
  // Vyčistenie chýb
  BLYNK_WRITE(BLYNK_VCLEAR_ERRORS) {
    if (param.asInt() == 1) {
      clearFault();
      Serial.println("Chyby vyčistené (cez Blynk)");
    }
  }
  
  // Reboot systému
  BLYNK_WRITE(BLYNK_VREBOOT) {
    if (param.asInt() == 1) {
      Serial.println("Reboot požiadavka z Blynk...");
      delay(1000);
      ESP.restart();
    }
  }
}

// ============================================
// NOTIFIKÁCIE A UPOZORNENIA
// ============================================

void sendBlynkNotification(String message) {
  // Odoslanie notifikácie do Blynk aplikácie
  if (Blynk.connected()) {
    Blynk.notify(message);
    Serial.print("Notifikácia odoslaná: ");
    Serial.println(message);
  }
}

void sendCriticalAlert(String alert) {
  // Kritické upozornenie
  sendBlynkNotification("⚠️ " + alert);
  
  // Opakované notifikácie pre kritické stavy
  static unsigned long lastAlert = 0;
  if (millis() - lastAlert > 60000) {  // Každú minútu
    sendBlynkNotification("⛔ STALE TRVA: " + alert);
    lastAlert = millis();
  }
}

// ============================================
// BEZDRÔTOVÁ TELEMETRIA - HLAVNÁ FUNKCIA
// ============================================

void runWirelessTelemetry() {
  // 1. Kontrola WiFi spojenia
  checkWiFiConnection();
  
  // 2. Spracovanie Blynk komunikácie
  if (wifiState == WIFI_CONNECTED && Blynk.connected()) {
    Blynk.run();
  }
  
  // 3. Aktualizácia dát na Blynk
  static unsigned long lastBlynkUpdate = 0;
  if (millis() - lastBlynkUpdate > 1000) {  // Každú sekundu
    updateBlynkDisplay();
    lastBlynkUpdate = millis();
  }
  
  // 4. Odosielanie štatistík (každých 30 sekúnd)
  static unsigned long lastStatsUpdate = 0;
  if (millis() - lastStatsUpdate > 30000) {
    sendBlynkStatistics();
    lastStatsUpdate = millis();
  }
  
  // 5. Kontrola notifikácií
  checkAndSendNotifications();
}

// ============================================
// SPRACOVANIE NOTIFIKÁCIÍ
// ============================================

void checkAndSendNotifications() {
  static bool lastLowPowerState = false;
  static bool lastErrorState = false;
  
  // Notifikácia pre vstup do šetrného režimu
  if (lowPowerMode && !lastLowPowerState) {
    sendBlynkNotification("🌙 Šetrný režim aktivovaný");
  }
  lastLowPowerState = lowPowerMode;
  
  // Notifikácia pre chybové stavy
  bool hasError = (overVoltageFlag || underVoltageFlag || 
                   overTempFlag || shortCircuitFlag);
  if (hasError && !lastErrorState) {
    String errorMsg = "🚨 Chyba: ";
    if (overVoltageFlag) errorMsg += "Vysoké napätie ";
    if (underVoltageFlag) errorMsg += "Nízke napätie ";
    if (overTempFlag) errorMsg += "Prehriatie ";
    if (shortCircuitFlag) errorMsg += "Zkrat";
    sendCriticalAlert(errorMsg);
  }
  lastErrorState = hasError;
  
  // Notifikácia pre dokončenie nabíjania
  static ChargePhase lastChargePhase = PHASE_OFF;
  if (lastChargePhase == PHASE_ABSORPTION && currentChargePhase == PHASE_FLOAT) {
    sendBlynkNotification("✅ Batéria plne nabitá");
  }
  lastChargePhase = currentChargePhase;
}

// ============================================
// WEBOVÝ SERVER PRE LOKÁLNY PRÍSTUP
// ============================================

#include <WebServer.h>
WebServer webServer(80);

void initWebServer() {
  Serial.println("Inicializácia webového servera...");
  
  // Webové endpointy
  webServer.on("/", handleRoot);
  webServer.on("/data", handleData);
  webServer.on("/stats", handleStats);
  webServer.on("/control", handleControl);
  webServer.on("/settings", handleSettings);
  webServer.on("/reboot", handleReboot);
  
  // Štart servera
  webServer.begin();
  Serial.println("Webový server spustený na porte 80");
}

void handleWebServer() {
  webServer.handleClient();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>MPPT Regulátor - Status</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += ".card { border: 1px solid #ddd; padding: 15px; margin: 10px 0; border-radius: 5px; }";
  html += ".value { font-weight: bold; color: #2c3e50; }";
  html += ".error { color: #e74c3c; }";
  html += ".warning { color: #f39c12; }";
  html += ".success { color: #27ae60; }";
  html += "</style></head><body>";
  
  html += "<h1>🌞 MPPT Solárny Regulátor</h1>";
  
  // Základné informácie
  html += "<div class='card'>";
  html += "<h2>Stav systému: <span class='value'>" + getStateName(currentSystemState) + "</span></h2>";
  html += "<p>Fáza nabíjania: <span class='value'>" + getPhaseName(currentChargePhase) + "</span></p>";
  html += "<p>Typ batérie: <span class='value'>" + currentProfile->batteryName + "</span></p>";
  html += "<p>Teplota: <span class='value'>" + String(temperature, 1) + " °C</span></p>";
  html += "</div>";
  
  // Panelové údaje
  html += "<div class='card'>";
  html += "<h2>Solárny Panel</h2>";
  html += "<p>Napätie: <span class='value'>" + String(panelVoltage, 2) + " V</span></p>";
  html += "<p>Prúd: <span class='value'>" + String(panelCurrent, 3) + " A</span></p>";
  html += "<p>Výkon: <span class='value'>" + String(panelPower, 1) + " W</span></p>";
  html += "</div>";
  
  // Batéria
  html += "<div class='card'>";
  html += "<h2>Batéria</h2>";
  html += "<p>Napätie: <span class='value'>" + String(batteryVoltage, 2) + " V</span></p>";
  html += "<p>Prúd: <span class='value'>" + String(batteryCurrent, 3) + " A</span></p>";
  html += "<p>Výkon: <span class='value'>" + String(batteryPower, 1) + " W</span></p>";
  html += "<p>Účinnosť: <span class='value'>" + String(efficiency, 1) + " %</span></p>";
  html += "</div>";
  
  // Kontrolné prvky
  html += "<div class='card'>";
  html += "<h2>Ovládanie</h2>";
  html += "<p><a href='/control?cmd=reboot'>🔄 Reboot systému</a></p>";
  html += "<p><a href='/control?cmd=clear'>🧹 Vyčistiť chyby</a></p>";
  html += "<p><a href='/stats'>📊 Zobraziť štatistiky</a></p>";
  html += "<p><a href='/settings'>⚙️ Nastavenia</a></p>";
  html += "</div>";
  
  // Footer
  html += "<hr><p><small>MPPT Regulátor v1.0 | ";
  html += "Uptime: " + String(millis() / 3600000.0, 1) + " hodín</small></p>";
  html += "</body></html>";
  
  webServer.send(200, "text/html; charset=UTF-8", html);
}

void handleData() {
  // JSON odpoveď s dátami
  String json = "{";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"panel\":{\"voltage\":" + String(panelVoltage, 2) + ",";
  json += "\"current\":" + String(panelCurrent, 3) + ",";
  json += "\"power\":" + String(panelPower, 1) + "},";
  json += "\"battery\":{\"voltage\":" + String(batteryVoltage, 2) + ",";
  json += "\"current\":" + String(batteryCurrent, 3) + ",";
  json += "\"power\":" + String(batteryPower, 1) + "},";
  json += "\"efficiency\":" + String(efficiency, 1) + ",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"pwm\":" + String(pwmDutyCycle, 1) + ",";
  json += "\"phase\":\"" + getPhaseName(currentChargePhase) + "\",";
  json += "\"state\":\"" + getStateName(currentSystemState) + "\"";
  json += "}";
  
  webServer.send(200, "application/json", json);
}

// ============================================
// DIAGNOSTIKA BEZDRÔTOVEJ KOMUNIKÁCIE
// ============================================

void printWirelessStatus() {
  Serial.println("=== STATUS BEZDRÔTOVEJ KOMUNIKÁCIE ===");
  
  Serial.print("WiFi stav: ");
  switch(wifiState) {
    case WIFI_DISCONNECTED: Serial.println("ODPOJENÉ"); break;
    case WIFI_CONNECTING: Serial.println("PRIPOJOVANIE"); break;
    case WIFI_CONNECTED: Serial.println("PRIPOJENÉ"); break;
    case WIFI_ERROR: Serial.println("CHYBA"); break;
  }
  
  if (wifiState == WIFI_CONNECTED) {
    Serial.print("SSID: "); Serial.println(WiFi.SSID());
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    Serial.print("RSSI: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
    Serial.print("Blynk: "); Serial.println(Blynk.connected() ? "PRIPOJENÝ" : "ODPOJENÝ");
  }
  
  Serial.println("=====================================");
}

void testWirelessConnection() {
  // Test WiFi spojenia
  Serial.println("Test WiFi spojenia...");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi OK - ping test");
    
    // Jednoduchý ping test
    if (WiFi.ping("8.8.8.8") >= 0) {
      Serial.println("Internet dostupný");
    } else {
      Serial.println("Internet nedostupný");
    }
  }
  
  // Test Blynk spojenia
  if (Blynk.connected()) {
    Serial.println("Blynk spojenie OK");
  } else {
    Serial.println("Blynk nie je pripojený");
  }
}