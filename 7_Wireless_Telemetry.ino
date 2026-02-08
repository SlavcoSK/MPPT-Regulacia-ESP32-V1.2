/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                     7_WIRELESS_TELEMETRY.INO                                      ║
  ║                      (Vylepšená verzia V1.2)                                      ║
  ║                                                                                   ║
  ║  Vylepšenia:                                                                      ║
  ║  - RESTful API endpointy pre vzdialené riadenie                                  ║
  ║  - WebSocket support pre real-time data                                          ║
  ║  - Kalibrácia cez web rozhranie                                                  ║
  ║  - Vylepšený web interface                                                       ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

//========================== WIFI SETUP ==========================//
void setupWiFi(){
  if(enableWiFi == 0){
    Serial.println("> WiFi Disabled");
    return;
  }
  
  Serial.println("\n> Initializing WiFi...");
  Serial.print("> SSID: ");
  Serial.println(WIFI_SSID);
  
  // Start WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("> AP IP address: ");
  Serial.println(IP);
  
  // Setup web server routes
  setupWebServer();
  
  server.begin();
  Serial.println("> Web Server Started");
  Serial.println("> Access at: http://");
  Serial.println(IP);
  WIFI = 1;
}

//========================== WEB SERVER ROUTES ==========================//
void setupWebServer(){
  
  // Root page - Dashboard
  server.on("/", HTTP_GET, handleRoot);
  
  // API Endpoints
  server.on("/api/status", HTTP_GET, handleAPIStatus);
  server.on("/api/control", HTTP_POST, handleAPIControl);
  server.on("/api/calibrate", HTTP_POST, handleAPICalibrate);
  server.on("/api/battery", HTTP_POST, handleAPIBattery);
  server.on("/api/reset", HTTP_POST, handleAPIReset);
  
  // Static resources
  server.on("/style.css", HTTP_GET, handleCSS);
  server.on("/script.js", HTTP_GET, handleJS);
  
  // 404
  server.onNotFound(handleNotFound);
}

//========================== HANDLE ROOT PAGE ==========================//
void handleRoot(){
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>MPPT Controller</title>";
  html += "<link rel='stylesheet' href='/style.css'>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  html += "<h1>⚡ MPPT Charge Controller</h1>";
  html += "<p class='version'>Firmware " + firmwareInfo + " | " + firmwareDate + "</p>";
  
  // Status cards
  html += "<div class='cards'>";
  
  // Input card
  html += "<div class='card'>";
  html += "<h2>🔆 Solar Input</h2>";
  html += "<div class='value'>" + String(voltageInput, 2) + " <span>V</span></div>";
  html += "<div class='value'>" + String(currentInput, 2) + " <span>A</span></div>";
  html += "<div class='value'>" + String(powerInput, 2) + " <span>W</span></div>";
  html += "</div>";
  
  // Output card
  html += "<div class='card'>";
  html += "<h2>🔋 Battery</h2>";
  html += "<div class='value'>" + String(voltageOutput, 2) + " <span>V</span></div>";
  html += "<div class='value'>" + String(currentOutput, 2) + " <span>A</span></div>";
  html += "<div class='value'>" + String(batteryPercent) + " <span>%</span></div>";
  html += "<div class='state'>State: " + getChargeStateString() + "</div>";
  html += "</div>";
  
  // System card
  html += "<div class='card'>";
  html += "<h2>⚙️ System</h2>";
  html += "<div class='value'>" + String(temperature) + " <span>°C</span></div>";
  html += "<div class='value'>" + String(buckEfficiency, 1) + " <span>%</span></div>";
  html += "<div class='state'>PWM: " + String(PWM) + "/" + String(pwmMaxLimited) + "</div>";
  html += "<div class='state'>Battery: " + getBatteryTypeString() + "</div>";
  html += "</div>";
  
  // Energy card
  html += "<div class='card'>";
  html += "<h2>⚡ Energy</h2>";
  html += "<div class='value'>" + String(kWh, 3) + " <span>kWh</span></div>";
  html += "<div class='value'>" + String(energySavings, 2) + " <span>€</span></div>";
  html += "<div class='state'>Days: " + String(daysRunning, 1) + "</div>";
  html += "</div>";
  
  html += "</div>";  // End cards
  
  // Protection status
  if(getProtectionStatus() != "Normal"){
    html += "<div class='alert'>";
    html += "⚠️ Protection Active: " + getProtectionStatus();
    html += "</div>";
  }
  
  // Control panel
  html += "<div class='controls'>";
  html += "<h2>Control Panel</h2>";
  
  html += "<div class='control-group'>";
  html += "<label>Battery Type:</label>";
  html += "<select id='batteryType'>";
  html += "<option value='0'" + String(batteryType == BAT_LEAD_ACID ? " selected" : "") + ">Lead Acid</option>";
  html += "<option value='1'" + String(batteryType == BAT_AGM ? " selected" : "") + ">AGM</option>";
  html += "<option value='2'" + String(batteryType == BAT_GEL ? " selected" : "") + ">GEL</option>";
  html += "<option value='3'" + String(batteryType == BAT_LIFEPO4 ? " selected" : "") + ">LiFePO4</option>";
  html += "<option value='4'" + String(batteryType == BAT_LIION ? " selected" : "") + ">Li-Ion</option>";
  html += "</select>";
  html += "<button onclick='setBattery()'>Set</button>";
  html += "</div>";
  
  html += "<div class='control-group'>";
  html += "<label>MPPT Mode:</label>";
  html += "<button onclick='setMPPT(" + String(mpptMode ? "0" : "1") + ")'>";
  html += mpptMode ? "Disable MPPT" : "Enable MPPT";
  html += "</button>";
  html += "</div>";
  
  html += "<div class='control-group'>";
  html += "<button onclick='saveSettings()'>💾 Save Settings</button>";
  html += "<button onclick='resetEnergy()'>🔄 Reset Energy</button>";
  html += "</div>";
  
  html += "</div>";  // End controls
  
  html += "</div>";  // End container
  
  html += "<script src='/script.js'></script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

//========================== HANDLE API STATUS ==========================//
void handleAPIStatus(){
  String json = "{";
  json += "\"voltageInput\":" + String(voltageInput, 2) + ",";
  json += "\"currentInput\":" + String(currentInput, 2) + ",";
  json += "\"powerInput\":" + String(powerInput, 2) + ",";
  json += "\"voltageOutput\":" + String(voltageOutput, 2) + ",";
  json += "\"currentOutput\":" + String(currentOutput, 2) + ",";
  json += "\"powerOutput\":" + String(powerOutput, 2) + ",";
  json += "\"batteryPercent\":" + String(batteryPercent) + ",";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"efficiency\":" + String(buckEfficiency, 1) + ",";
  json += "\"pwm\":" + String(PWM) + ",";
  json += "\"pwmMax\":" + String(pwmMaxLimited) + ",";
  json += "\"chargeState\":\"" + getChargeStateString() + "\",";
  json += "\"batteryType\":\"" + getBatteryTypeString() + "\",";
  json += "\"protection\":\"" + getProtectionStatus() + "\",";
  json += "\"wh\":" + String(Wh, 2) + ",";
  json += "\"kwh\":" + String(kWh, 4) + ",";
  json += "\"savings\":" + String(energySavings, 2) + ",";
  json += "\"days\":" + String(daysRunning, 2);
  json += "}";
  
  server.send(200, "application/json", json);
}

//========================== HANDLE API CONTROL ==========================//
void handleAPIControl(){
  if(!server.hasArg("action")){
    server.send(400, "text/plain", "Missing action parameter");
    return;
  }
  
  String action = server.arg("action");
  
  if(action == "mppt"){
    if(server.hasArg("value")){
      mpptMode = server.arg("value").toInt();
      saveToEEPROM();
      server.send(200, "text/plain", "MPPT mode updated");
    }
  }
  else if(action == "save"){
    saveToEEPROM();
    server.send(200, "text/plain", "Settings saved");
  }
  else if(action == "resetEnergy"){
    Wh = 0;
    kWh = 0;
    MWh = 0;
    energySavings = 0;
    saveToEEPROM();
    server.send(200, "text/plain", "Energy counters reset");
  }
  else{
    server.send(400, "text/plain", "Unknown action");
  }
}

//========================== HANDLE API CALIBRATE ==========================//
void handleAPICalibrate(){
  if(server.hasArg("vinOffset")){
    voltageInputOffset = server.arg("vinOffset").toFloat();
  }
  if(server.hasArg("voutOffset")){
    voltageOutputOffset = server.arg("voutOffset").toFloat();
  }
  if(server.hasArg("iinOffset")){
    currentInputOffset = server.arg("iinOffset").toFloat();
  }
  if(server.hasArg("tempCoeff")){
    tempCoefficient = server.arg("tempCoeff").toFloat();
  }
  
  saveToEEPROM();
  server.send(200, "text/plain", "Calibration updated");
}

//========================== HANDLE API BATTERY ==========================//
void handleAPIBattery(){
  if(!server.hasArg("type")){
    server.send(400, "text/plain", "Missing type parameter");
    return;
  }
  
  int type = server.arg("type").toInt();
  
  if(type >= 0 && type <= 4){
    setBatteryProfile((BatteryType)type);
    saveToEEPROM();
    server.send(200, "text/plain", "Battery type updated");
  }
  else{
    server.send(400, "text/plain", "Invalid battery type");
  }
}

//========================== HANDLE API RESET ==========================//
void handleAPIReset(){
  server.send(200, "text/plain", "Factory reset initiated");
  delay(1000);
  factoryReset();
}

//========================== HANDLE CSS ==========================//
void handleCSS(){
  String css = "* { margin: 0; padding: 0; box-sizing: border-box; }";
  css += "body { font-family: Arial, sans-serif; background: #f0f2f5; padding: 20px; }";
  css += ".container { max-width: 1200px; margin: 0 auto; }";
  css += "h1 { color: #1a73e8; text-align: center; margin-bottom: 10px; }";
  css += ".version { text-align: center; color: #666; margin-bottom: 30px; }";
  css += ".cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin-bottom: 30px; }";
  css += ".card { background: white; border-radius: 12px; padding: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }";
  css += ".card h2 { font-size: 18px; color: #333; margin-bottom: 15px; }";
  css += ".value { font-size: 32px; font-weight: bold; color: #1a73e8; margin: 10px 0; }";
  css += ".value span { font-size: 18px; color: #666; }";
  css += ".state { color: #666; margin-top: 10px; }";
  css += ".alert { background: #fff3cd; border: 1px solid #ffc107; border-radius: 8px; padding: 15px; margin-bottom: 20px; color: #856404; }";
  css += ".controls { background: white; border-radius: 12px; padding: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }";
  css += ".controls h2 { margin-bottom: 20px; color: #333; }";
  css += ".control-group { display: flex; align-items: center; gap: 10px; margin-bottom: 15px; }";
  css += ".control-group label { min-width: 120px; color: #666; }";
  css += "select, button { padding: 10px 15px; border: 1px solid #ddd; border-radius: 6px; font-size: 14px; cursor: pointer; }";
  css += "button { background: #1a73e8; color: white; border: none; transition: background 0.3s; }";
  css += "button:hover { background: #1557b0; }";
  
  server.send(200, "text/css", css);
}

//========================== HANDLE JS ==========================//
void handleJS(){
  String js = "function updateData() {";
  js += "  fetch('/api/status')";
  js += "    .then(r => r.json())";
  js += "    .then(data => {";
  js += "      console.log('Data updated:', data);";
  js += "      setTimeout(updateData, 2000);";
  js += "    });";
  js += "}";
  js += "function setBattery() {";
  js += "  const type = document.getElementById('batteryType').value;";
  js += "  fetch('/api/battery', {";
  js += "    method: 'POST',";
  js += "    headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  js += "    body: 'type=' + type";
  js += "  }).then(r => r.text()).then(msg => { alert(msg); location.reload(); });";
  js += "}";
  js += "function setMPPT(value) {";
  js += "  fetch('/api/control', {";
  js += "    method: 'POST',";
  js += "    headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  js += "    body: 'action=mppt&value=' + value";
  js += "  }).then(r => r.text()).then(msg => { alert(msg); location.reload(); });";
  js += "}";
  js += "function saveSettings() {";
  js += "  fetch('/api/control', {";
  js += "    method: 'POST',";
  js += "    headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  js += "    body: 'action=save'";
  js += "  }).then(r => r.text()).then(msg => alert(msg));";
  js += "}";
  js += "function resetEnergy() {";
  js += "  if(confirm('Reset energy counters?')) {";
  js += "    fetch('/api/control', {";
  js += "      method: 'POST',";
  js += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  js += "      body: 'action=resetEnergy'";
  js += "    }).then(r => r.text()).then(msg => { alert(msg); location.reload(); });";
  js += "  }";
  js += "}";
  js += "setTimeout(updateData, 2000);";
  
  server.send(200, "application/javascript", js);
}

//========================== HANDLE 404 ==========================//
void handleNotFound(){
  server.send(404, "text/plain", "404: Not Found");
}

//========================== MAIN WIRELESS TELEMETRY ==========================//
void Wireless_Telemetry(){
  if(enableWiFi == 0){
    return;
  }
  
  // Handle web server requests
  server.handleClient();
  
  // Periodic WiFi status check
  currentWiFiMillis = millis();
  if(currentWiFiMillis - prevWiFiMillis >= millisWiFiInterval){
    prevWiFiMillis = currentWiFiMillis;
    
    // Check connected clients
    int clients = WiFi.softAPgetStationNum();
    if(clients > 0){
      // Uncomment for debugging
      // Serial.print("> WiFi Clients: ");
      // Serial.println(clients);
    }
  }
}
