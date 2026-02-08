/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                      6_ONBOARD_TELEMETRY.INO                                      ║
  ║                      (Vylepšená verzia V1.2)                                      ║
  ║                                                                                   ║
  ║  Vylepšenia:                                                                      ║
  ║  - Rôzne formáty výstupu (human readable, CSV, JSON)                             ║
  ║  - Príkazy cez sériový port                                                      ║
  ║  - Vylepšené logovanie                                                           ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

//========================== TELEMETRY MODES ==========================//
// 0 = OFF
// 1 = Human Readable (štandardný)
// 2 = CSV (pre Excel/logging)
// 3 = JSON (pre webové aplikácie)

//========================== PRINT TELEMETRY DATA ==========================//
void printTelemetryData(){
  
  switch(serialTelemMode){
    
    case 0:  // OFF - žiadny výstup
      break;
      
    case 1:  // HUMAN READABLE
      Serial.println("\n╔════════════════════════════════════════════╗");
      Serial.println("║         MPPT CHARGE CONTROLLER            ║");
      Serial.println("╚════════════════════════════════════════════╝");
      
      Serial.println("\n┌─── INPUT (Solar) ───────────────────────┐");
      Serial.print("│ Voltage:    ");
      Serial.print(voltageInput, 2);
      Serial.println(" V");
      Serial.print("│ Current:    ");
      Serial.print(currentInput, 2);
      Serial.println(" A");
      Serial.print("│ Power:      ");
      Serial.print(powerInput, 2);
      Serial.println(" W");
      Serial.println("└─────────────────────────────────────────────┘");
      
      Serial.println("\n┌─── OUTPUT (Battery) ────────────────────┐");
      Serial.print("│ Voltage:    ");
      Serial.print(voltageOutput, 2);
      Serial.println(" V");
      Serial.print("│ Current:    ");
      Serial.print(currentOutput, 2);
      Serial.println(" A");
      Serial.print("│ Power:      ");
      Serial.print(powerOutput, 2);
      Serial.println(" W");
      Serial.print("│ SOC:        ");
      Serial.print(batteryPercent);
      Serial.println(" %");
      Serial.println("└─────────────────────────────────────────────┘");
      
      Serial.println("\n┌─── BATTERY STATUS ──────────────────────┐");
      Serial.print("│ Type:       ");
      Serial.println(getBatteryTypeString());
      Serial.print("│ State:      ");
      Serial.println(getChargeStateString());
      Serial.print("│ Protection: ");
      Serial.println(getProtectionStatus());
      Serial.println("└─────────────────────────────────────────────┘");
      
      Serial.println("\n┌─── SYSTEM ──────────────────────────────┐");
      Serial.print("│ Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");
      Serial.print("│ Fan:         ");
      Serial.println(fanStatus ? "ON" : "OFF");
      Serial.print("│ PWM:         ");
      Serial.print(PWM);
      Serial.print(" / ");
      Serial.print(pwmMaxLimited);
      Serial.print(" (");
      Serial.print((PWM * 100.0) / pwmMaxLimited, 1);
      Serial.println("%)");
      Serial.print("│ Efficiency:  ");
      Serial.print(buckEfficiency, 1);
      Serial.println(" %");
      Serial.println("└─────────────────────────────────────────────┘");
      
      Serial.println("\n┌─── ENERGY ──────────────────────────────┐");
      Serial.print("│ Harvested:   ");
      if(kWh >= 1.0){
        Serial.print(kWh, 3);
        Serial.println(" kWh");
      }
      else{
        Serial.print(Wh, 2);
        Serial.println(" Wh");
      }
      Serial.print("│ Savings:     ");
      Serial.print(energySavings, 2);
      Serial.println(" €");
      Serial.print("│ Days:        ");
      Serial.print(daysRunning, 2);
      Serial.println(" days");
      Serial.println("└─────────────────────────────────────────────┘\n");
      break;
      
    case 2:  // CSV FORMAT
      // Header (print iba pri prvom volaní)
      static bool csvHeaderPrinted = false;
      if(!csvHeaderPrinted){
        Serial.println("Timestamp,Vin,Iin,Pin,Vout,Iout,Pout,SOC,Temp,PWM,Efficiency,ChargeState,Wh,kWh");
        csvHeaderPrinted = true;
      }
      
      // Data
      Serial.print(millis());
      Serial.print(",");
      Serial.print(voltageInput, 2);
      Serial.print(",");
      Serial.print(currentInput, 2);
      Serial.print(",");
      Serial.print(powerInput, 2);
      Serial.print(",");
      Serial.print(voltageOutput, 2);
      Serial.print(",");
      Serial.print(currentOutput, 2);
      Serial.print(",");
      Serial.print(powerOutput, 2);
      Serial.print(",");
      Serial.print(batteryPercent);
      Serial.print(",");
      Serial.print(temperature);
      Serial.print(",");
      Serial.print(PWM);
      Serial.print(",");
      Serial.print(buckEfficiency, 1);
      Serial.print(",");
      Serial.print(getChargeStateString());
      Serial.print(",");
      Serial.print(Wh, 2);
      Serial.print(",");
      Serial.println(kWh, 4);
      break;
      
    case 3:  // JSON FORMAT
      Serial.println("{");
      Serial.print("  \"timestamp\": ");
      Serial.print(millis());
      Serial.println(",");
      
      Serial.println("  \"input\": {");
      Serial.print("    \"voltage\": ");
      Serial.print(voltageInput, 2);
      Serial.println(",");
      Serial.print("    \"current\": ");
      Serial.print(currentInput, 2);
      Serial.println(",");
      Serial.print("    \"power\": ");
      Serial.println(powerInput, 2);
      Serial.println("  },");
      
      Serial.println("  \"output\": {");
      Serial.print("    \"voltage\": ");
      Serial.print(voltageOutput, 2);
      Serial.println(",");
      Serial.print("    \"current\": ");
      Serial.print(currentOutput, 2);
      Serial.println(",");
      Serial.print("    \"power\": ");
      Serial.print(powerOutput, 2);
      Serial.println(",");
      Serial.print("    \"soc\": ");
      Serial.println(batteryPercent);
      Serial.println("  },");
      
      Serial.println("  \"battery\": {");
      Serial.print("    \"type\": \"");
      Serial.print(getBatteryTypeString());
      Serial.println("\",");
      Serial.print("    \"chargeState\": \"");
      Serial.print(getChargeStateString());
      Serial.println("\"");
      Serial.println("  },");
      
      Serial.println("  \"system\": {");
      Serial.print("    \"temperature\": ");
      Serial.print(temperature);
      Serial.println(",");
      Serial.print("    \"fan\": ");
      Serial.print(fanStatus ? "true" : "false");
      Serial.println(",");
      Serial.print("    \"pwm\": ");
      Serial.print(PWM);
      Serial.println(",");
      Serial.print("    \"pwmMax\": ");
      Serial.print(pwmMaxLimited);
      Serial.println(",");
      Serial.print("    \"efficiency\": ");
      Serial.println(buckEfficiency, 1);
      Serial.println("  },");
      
      Serial.println("  \"energy\": {");
      Serial.print("    \"wh\": ");
      Serial.print(Wh, 2);
      Serial.println(",");
      Serial.print("    \"kwh\": ");
      Serial.print(kWh, 4);
      Serial.println(",");
      Serial.print("    \"savings\": ");
      Serial.println(energySavings, 2);
      Serial.println("  },");
      
      Serial.println("  \"protection\": {");
      Serial.print("    \"status\": \"");
      Serial.print(getProtectionStatus());
      Serial.println("\",");
      Serial.print("    \"oov\": ");
      Serial.print(OOV);
      Serial.println(",");
      Serial.print("    \"ouv\": ");
      Serial.print(OUV);
      Serial.println(",");
      Serial.print("    \"ooc\": ");
      Serial.print(OOC);
      Serial.println(",");
      Serial.print("    \"iov\": ");
      Serial.print(IOV);
      Serial.println(",");
      Serial.print("    \"iuv\": ");
      Serial.print(IUV);
      Serial.println(",");
      Serial.print("    \"ote\": ");
      Serial.println(OTE);
      Serial.println("  }");
      Serial.println("}");
      break;
  }
}

//========================== PROCESS SERIAL COMMANDS ==========================//
void processSerialCommand(String command){
  command.trim();
  command.toLowerCase();
  
  if(command == "help" || command == "?"){
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║         AVAILABLE COMMANDS                ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.println("  help         - Show this help");
    Serial.println("  status       - Show detailed system status");
    Serial.println("  diag         - Run diagnostics");
    Serial.println("  save         - Save settings to EEPROM");
    Serial.println("  load         - Load settings from EEPROM");
    Serial.println("  reset        - Factory reset");
    Serial.println("  telem <0-3>  - Set telemetry mode");
    Serial.println("                 0=OFF, 1=Human, 2=CSV, 3=JSON");
    Serial.println("  battery <type> - Set battery type");
    Serial.println("                   lead/agm/gel/lifepo4/liion");
    Serial.println("  mppt <0/1>   - Enable/disable MPPT");
    Serial.println("  fan <0/1>    - Enable/disable fan");
    Serial.println("  calib        - Show calibration values");
    Serial.println("───────────────────────────────────────────────\n");
  }
  else if(command == "status"){
    printSystemDiagnostics();
  }
  else if(command == "diag"){
    printSystemDiagnostics();
  }
  else if(command == "save"){
    saveToEEPROM();
  }
  else if(command == "load"){
    loadFromEEPROM();
  }
  else if(command == "reset"){
    Serial.println("> Factory reset requested");
    Serial.println("> Type 'yes' to confirm...");
    
    unsigned long timeout = millis() + 5000;
    while(millis() < timeout){
      if(Serial.available()){
        String confirm = Serial.readStringUntil('\n');
        confirm.trim();
        confirm.toLowerCase();
        if(confirm == "yes"){
          factoryReset();
        }
        else{
          Serial.println("> Reset cancelled");
        }
        return;
      }
    }
    Serial.println("> Reset timeout - cancelled");
  }
  else if(command.startsWith("telem")){
    int mode = command.substring(6).toInt();
    if(mode >= 0 && mode <= 3){
      serialTelemMode = mode;
      Serial.print("> Telemetry mode set to: ");
      Serial.println(mode);
      saveToEEPROM();
    }
    else{
      Serial.println("> ERROR: Invalid mode (0-3)");
    }
  }
  else if(command.startsWith("battery")){
    String type = command.substring(8);
    type.trim();
    
    if(type == "lead"){
      setBatteryProfile(BAT_LEAD_ACID);
      Serial.println("> Battery type set to Lead Acid");
      saveToEEPROM();
    }
    else if(type == "agm"){
      setBatteryProfile(BAT_AGM);
      Serial.println("> Battery type set to AGM");
      saveToEEPROM();
    }
    else if(type == "gel"){
      setBatteryProfile(BAT_GEL);
      Serial.println("> Battery type set to GEL");
      saveToEEPROM();
    }
    else if(type == "lifepo4"){
      setBatteryProfile(BAT_LIFEPO4);
      Serial.println("> Battery type set to LiFePO4");
      saveToEEPROM();
    }
    else if(type == "liion"){
      setBatteryProfile(BAT_LIION);
      Serial.println("> Battery type set to Li-Ion");
      saveToEEPROM();
    }
    else{
      Serial.println("> ERROR: Invalid battery type");
      Serial.println(">   Options: lead, agm, gel, lifepo4, liion");
    }
  }
  else if(command.startsWith("mppt")){
    int mode = command.substring(5).toInt();
    if(mode == 0 || mode == 1){
      mpptMode = mode;
      Serial.print("> MPPT mode set to: ");
      Serial.println(mode ? "ENABLED" : "DISABLED");
      saveToEEPROM();
    }
    else{
      Serial.println("> ERROR: Invalid mode (0 or 1)");
    }
  }
  else if(command.startsWith("fan")){
    int mode = command.substring(4).toInt();
    if(mode == 0 || mode == 1){
      enableFan = mode;
      Serial.print("> Fan set to: ");
      Serial.println(mode ? "ENABLED" : "DISABLED");
      saveToEEPROM();
    }
    else{
      Serial.println("> ERROR: Invalid mode (0 or 1)");
    }
  }
  else if(command == "calib"){
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║         CALIBRATION VALUES                ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.print("  Voltage Input Offset:  ");
    Serial.print(voltageInputOffset, 4);
    Serial.println(" V");
    Serial.print("  Voltage Output Offset: ");
    Serial.print(voltageOutputOffset, 4);
    Serial.println(" V");
    Serial.print("  Current Input Offset:  ");
    Serial.print(currentInputOffset, 4);
    Serial.println(" A");
    Serial.print("  Current Midpoint:      ");
    Serial.print(currentMidPoint, 4);
    Serial.println(" V");
    Serial.print("  Temp Coefficient:      ");
    Serial.print(tempCoefficient, 4);
    Serial.println(" V/°C");
    Serial.println("───────────────────────────────────────────────\n");
  }
  else{
    Serial.print("> Unknown command: ");
    Serial.println(command);
    Serial.println("> Type 'help' for available commands");
  }
}

//========================== MAIN TELEMETRY FUNCTION ==========================//
void Onboard_Telemetry(){
  
  // Check for serial commands
  if(Serial.available()){
    String command = Serial.readStringUntil('\n');
    processSerialCommand(command);
  }
  
  // Print telemetry data periodically
  currentSerialMillis = millis();
  if(currentSerialMillis - prevSerialMillis >= millisSerialInterval){
    prevSerialMillis = currentSerialMillis;
    
    if(serialTelemMode > 0){
      printTelemetryData();
    }
  }
}
