/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                       3_DEVICE_PROTECTION.INO                                     ║
  ║                      (Vylepšená verzia V1.2)                                      ║
  ║                                                                                   ║
  ║  Vylepšenia:                                                                      ║
  ║  - Hysteréza pre všetky ochranné funkcie (zabráni oscilácii)                    ║
  ║  - Postupné znižovanie PWM namiesto okamžitého vypnutia                          ║
  ║  - Lepšie logovanie chýb a stavov                                                ║
  ║  - Ochrana pred rapid cycling                                                    ║
  ║  - Soft start po obnovení z chyby                                                ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

//========================== PROTECTION HYSTERESIS VALUES ==========================//
// Hysteréza zabráni častému zapínaniu/vypínaniu pri kolísavých hodnotách
#define VOLTAGE_HYSTERESIS      0.3      // Hysteréza napätia (V)
#define CURRENT_HYSTERESIS      0.5      // Hysteréza prúdu (A)
#define TEMP_HYSTERESIS         5.0      // Hysteréza teploty (°C)

//========================== PROTECTION STATE TRACKING ==========================//
static unsigned long lastProtectionTrigger = 0;
static unsigned long protectionCooldown = 5000;  // 5s cooldown medzi resetmi
static int consecutiveErrors = 0;
static int maxConsecutiveErrors = 3;

//========================== SOFT START AFTER PROTECTION ==========================//
static bool softStartActive = false;
static int softStartPWM = 0;
static unsigned long softStartTime = 0;
static const int softStartDuration = 2000;  // 2s soft start

//========================== ENABLE/DISABLE BUCK CONVERTER ==========================//
void buck_Enable(){
  digitalWrite(buck_EN, HIGH);
  buckEnable = 1;
  Serial.println("> Buck Converter ENABLED");
}

void buck_Disable(){
  digitalWrite(buck_EN, LOW);
  ledcWrite(buck_IN, 0);
  PWM = 0;
  buckEnable = 0;
  Serial.println("> Buck Converter DISABLED");
}

//========================== SOFT START FUNCTION ==========================//
void initiateSoftStart(){
  softStartActive = true;
  softStartPWM = 0;
  softStartTime = millis();
  Serial.println("> Soft Start Initiated");
}

void processSoftStart(){
  if(!softStartActive) return;
  
  unsigned long elapsed = millis() - softStartTime;
  
  if(elapsed >= softStartDuration){
    softStartActive = false;
    Serial.println("> Soft Start Complete");
    return;
  }
  
  // Postupne zvyšuj PWM počas soft startu
  int targetPWM = (PWM * elapsed) / softStartDuration;
  ledcWrite(buck_IN, targetPWM);
}

//========================== MAIN PROTECTION FUNCTION ==========================//
void Device_Protection(){
  
  bool protectionTriggered = false;
  String protectionReason = "";
  
  //==================== BATTERY OVERVOLTAGE PROTECTION ====================//
  // Ochrana pred prepätím batérie
  static bool batteryOVActive = false;
  
  if(voltageOutput > outputVoltageMax){
    if(!batteryOVActive){
      OOV = 1;
      batteryOVActive = true;
      protectionTriggered = true;
      protectionReason = "Battery Overvoltage";
      
      // Okamžite zastav nabíjanie
      PWM = 0;
      ledcWrite(buck_IN, PWM);
      
      Serial.print("> PROTECTION: Battery Overvoltage! Vout=");
      Serial.print(voltageOutput, 2);
      Serial.print("V (Max=");
      Serial.print(outputVoltageMax, 2);
      Serial.println("V)");
    }
  }
  else if(voltageOutput < (outputVoltageMax - VOLTAGE_HYSTERESIS)){
    if(batteryOVActive){
      OOV = 0;
      batteryOVActive = false;
      Serial.println("> Battery Overvoltage Cleared");
    }
  }
  
  //==================== BATTERY UNDERVOLTAGE PROTECTION ====================//
  // Detekcia odpojenej alebo veľmi slabej batérie
  static bool batteryUVActive = false;
  
  if(voltageOutput < vOutSystemMin && inputSource != 0){
    if(!batteryUVActive){
      OUV = 1;
      BNC = 1;  // Battery not connected
      batteryUVActive = true;
      protectionTriggered = true;
      protectionReason = "Battery Undervoltage/Disconnected";
      
      PWM = 0;
      ledcWrite(buck_IN, PWM);
      
      Serial.print("> PROTECTION: Battery Undervoltage! Vout=");
      Serial.print(voltageOutput, 2);
      Serial.println("V");
    }
  }
  else if(voltageOutput > (vOutSystemMin + VOLTAGE_HYSTERESIS)){
    if(batteryUVActive){
      OUV = 0;
      BNC = 0;
      batteryUVActive = false;
      Serial.println("> Battery Undervoltage Cleared");
    }
  }
  
  //==================== OUTPUT OVERCURRENT PROTECTION ====================//
  // Ochrana pred nadprúdom na výstupe
  static bool outputOCActive = false;
  
  if(currentOutput > outputCurrentMax){
    if(!outputOCActive){
      OOC = 1;
      outputOCActive = true;
      protectionTriggered = true;
      protectionReason = "Output Overcurrent";
      
      // Postupne zniž PWM namiesto okamžitého vypnutia
      if(PWM > 10){
        PWM -= 5;
        ledcWrite(buck_IN, PWM);
      }
      else{
        PWM = 0;
        ledcWrite(buck_IN, PWM);
      }
      
      Serial.print("> PROTECTION: Output Overcurrent! Iout=");
      Serial.print(currentOutput, 2);
      Serial.print("A (Max=");
      Serial.print(outputCurrentMax, 2);
      Serial.println("A)");
    }
    else{
      // Pokračuj v znižovaní PWM ak problém pretrváva
      if(PWM > 0){
        PWM = max(0, PWM - 2);
        ledcWrite(buck_IN, PWM);
      }
    }
  }
  else if(currentOutput < (outputCurrentMax - CURRENT_HYSTERESIS)){
    if(outputOCActive){
      OOC = 0;
      outputOCActive = false;
      Serial.println("> Output Overcurrent Cleared");
    }
  }
  
  //==================== INPUT OVERVOLTAGE PROTECTION ====================//
  // Ochrana pred prepätím na vstupe (solárne panely)
  static bool inputOVActive = false;
  
  if(voltageInput > inputVoltageMax){
    if(!inputOVActive){
      IOV = 1;
      inputOVActive = true;
      protectionTriggered = true;
      protectionReason = "Input Overvoltage";
      
      PWM = 0;
      ledcWrite(buck_IN, PWM);
      
      Serial.print("> PROTECTION: Input Overvoltage! Vin=");
      Serial.print(voltageInput, 2);
      Serial.print("V (Max=");
      Serial.print(inputVoltageMax, 2);
      Serial.println("V)");
    }
  }
  else if(voltageInput < (inputVoltageMax - VOLTAGE_HYSTERESIS)){
    if(inputOVActive){
      IOV = 0;
      inputOVActive = false;
      Serial.println("> Input Overvoltage Cleared");
    }
  }
  
  //==================== INPUT UNDERVOLTAGE PROTECTION ====================//
  // PV dropout - solárne napätie je príliš nízke
  static bool inputUVActive = false;
  float minInputVoltage = voltageOutput + 1.0;  // Vin musí byť aspoň 1V nad Vout
  
  if(voltageInput < minInputVoltage && inputSource == 1){
    if(!inputUVActive){
      IUV = 1;
      inputUVActive = true;
      
      // Postupne zniž PWM
      if(PWM > 5){
        PWM -= 3;
        ledcWrite(buck_IN, PWM);
      }
      else{
        PWM = 0;
        ledcWrite(buck_IN, PWM);
      }
      
      Serial.print("> WARNING: Input Undervoltage! Vin=");
      Serial.print(voltageInput, 2);
      Serial.print("V, Vout=");
      Serial.print(voltageOutput, 2);
      Serial.println("V");
    }
  }
  else if(voltageInput > (minInputVoltage + VOLTAGE_HYSTERESIS)){
    if(inputUVActive){
      IUV = 0;
      inputUVActive = false;
      Serial.println("> Input Undervoltage Cleared");
    }
  }
  
  //==================== OVERTEMPERATURE PROTECTION ====================//
  // Ochrana pred prehriatím
  static bool overTempActive = false;
  
  if(temperature > temperatureMax){
    if(!overTempActive){
      OTE = 1;
      overTempActive = true;
      protectionTriggered = true;
      protectionReason = "Overtemperature";
      
      PWM = 0;
      ledcWrite(buck_IN, PWM);
      buck_Disable();
      
      Serial.print("> PROTECTION: Overtemperature! Temp=");
      Serial.print(temperature);
      Serial.print("°C (Max=");
      Serial.print(temperatureMax);
      Serial.println("°C)");
    }
  }
  else if(temperature < (temperatureMax - TEMP_HYSTERESIS)){
    if(overTempActive){
      OTE = 0;
      overTempActive = false;
      Serial.println("> Overtemperature Cleared");
      
      // Po vychladnutí znovu zapni buck converter
      if(!batteryOVActive && !batteryUVActive && !outputOCActive && !inputOVActive){
        buck_Enable();
        initiateSoftStart();
      }
    }
  }
  
  //==================== FAN CONTROL WITH HYSTERESIS ====================//
  static bool fanActive = false;
  
  if(enableFan == 1 || overrideFan == 1){
    if(temperature > temperatureFan && !fanActive){
      digitalWrite(FAN, HIGH);
      fanStatus = 1;
      fanActive = true;
      Serial.print("> Fan ENABLED (Temp=");
      Serial.print(temperature);
      Serial.println("°C)");
    }
    else if(temperature < (temperatureFan - TEMP_HYSTERESIS) && fanActive){
      digitalWrite(FAN, LOW);
      fanStatus = 0;
      fanActive = false;
      Serial.print("> Fan DISABLED (Temp=");
      Serial.print(temperature);
      Serial.println("°C)");
    }
  }
  else{
    digitalWrite(FAN, LOW);
    fanStatus = 0;
    fanActive = false;
  }
  
  //==================== PROTECTION COOLDOWN & ERROR TRACKING ====================//
  if(protectionTriggered){
    lastProtectionTrigger = millis();
    consecutiveErrors++;
    
    Serial.print("> Protection Triggered: ");
    Serial.print(protectionReason);
    Serial.print(" (Error #");
    Serial.print(consecutiveErrors);
    Serial.println(")");
    
    // Ak je príliš veľa chýb za sebou, predĺž cooldown
    if(consecutiveErrors >= maxConsecutiveErrors){
      protectionCooldown = 30000;  // 30s cooldown po opakovaných chybách
      Serial.println("> WARNING: Multiple consecutive errors detected!");
      Serial.println("> Extended cooldown activated (30s)");
    }
  }
  
  // Reset error counter ak prešiel dostatok času bez chyby
  if(millis() - lastProtectionTrigger > protectionCooldown){
    if(consecutiveErrors > 0){
      consecutiveErrors = 0;
      protectionCooldown = 5000;  // Reset na normálny cooldown
      Serial.println("> Error counter reset");
    }
  }
  
  //==================== SOFT START PROCESSING ====================//
  processSoftStart();
  
  //==================== PWM LIMITS ====================//
  // Zaisti že PWM je v povolených medziach
  PWM = constrain(PWM, 0, pwmMaxLimited);
}

//==================== CHECK IF SYSTEM IS SAFE TO CHARGE ====================//
bool isSafeToCharge(){
  // Vráti true ak nie sú aktívne žiadne ochranné mechanizmy
  return (OOV == 0 && OUV == 0 && OOC == 0 && IOV == 0 && OTE == 0);
}

//==================== GET PROTECTION STATUS STRING ====================//
String getProtectionStatus(){
  if(OOV == 1) return "Battery Overvoltage";
  if(OUV == 1) return "Battery Undervoltage";
  if(OOC == 1) return "Output Overcurrent";
  if(IOV == 1) return "Input Overvoltage";
  if(IUV == 1) return "Input Undervoltage";
  if(OTE == 1) return "Overtemperature";
  if(BNC == 1) return "Battery Not Connected";
  return "Normal";
}
