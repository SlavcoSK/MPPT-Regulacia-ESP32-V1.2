/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                      4_CHARGING_ALGORITHM.INO                                     ║
  ║                      (Vylepšená verzia V1.2 - FIXED)                             ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

//========================== BATTERY PROFILES ==========================//
struct BatteryProfile {
  float bulkVoltage;
  float absorptionVoltage;
  float floatVoltage;
  float maxCurrent;
  float minCurrent;
  bool useFloat;
  int absorptionTime;
  bool useTempComp;
};

// Databáza profilov batérií
BatteryProfile batProfiles[] = {
  // BAT_LEAD_ACID
  {14.4, 14.4, 13.6, 20.0, 0.5, true, 120, true},
  // BAT_AGM
  {14.6, 14.4, 13.5, 20.0, 0.3, true, 180, true},
  // BAT_GEL
  {14.1, 14.1, 13.8, 15.0, 0.3, true, 180, true},
  // BAT_LIFEPO4
  {14.6, 14.4, 13.6, 50.0, 0.5, true, 60, false},
  // BAT_LIION
  {12.6, 12.6, 12.6, 30.0, 0.3, false, 30, false}
};

BatteryProfile activeBat = batProfiles[BAT_AGM];

//========================== MPPT ALGORITHM PARAMETERS ==========================//
int mpptStepSize = 5;
int mpptAdaptiveMax = 20;
int mpptAdaptiveMin = 1;
float mpptEfficiencyThreshold = 95.0;
float mpptDelta = 0.05;

//========================== TEMPERATURE COMPENSATION ==========================//
float calculateTempCompensatedVoltage(float baseVoltage){
  if(!activeBat.useTempComp){
    return baseVoltage;
  }
  
  float tempDiff = temperature - referenceTemp;
  float compensation = tempCoefficient * tempDiff;
  float compensatedVoltage = baseVoltage + compensation;
  
  static float lastLoggedComp = 0;
  if(abs(compensation - lastLoggedComp) > 0.05){
    Serial.print("> Temp Comp: ");
    Serial.print(compensation, 3);
    Serial.print("V @ ");
    Serial.print(temperature);
    Serial.print("C (Target: ");
    Serial.print(compensatedVoltage, 2);
    Serial.println("V)");
    lastLoggedComp = compensation;
  }
  
  return compensatedVoltage;
}

//========================== SET BATTERY PROFILE ==========================//
void setBatteryProfile(BatteryType type){
  batteryType = type;
  activeBat = batProfiles[type];
  voltageBatteryMax = activeBat.bulkVoltage;  // Now OK because it's a variable
  
  Serial.print("> Battery Profile: ");
  switch(type){
    case BAT_LEAD_ACID:
      Serial.println("Lead Acid");
      break;
    case BAT_AGM:
      Serial.println("AGM");
      break;
    case BAT_GEL:
      Serial.println("GEL");
      break;
    case BAT_LIFEPO4:
      Serial.println("LiFePO4");
      break;
    case BAT_LIION:
      Serial.println("Li-Ion");
      break;
  }
  
  Serial.print("  Bulk: ");
  Serial.print(activeBat.bulkVoltage, 2);
  Serial.println("V");
}

//========================== CHARGING STATE MACHINE ==========================//
void updateChargeState(){
  static unsigned long lastStateChange = 0;
  static ChargeState prevState = CHARGE_OFF;
  
  // Calculate temp compensated targets OUTSIDE switch
  float bulkTarget = calculateTempCompensatedVoltage(activeBat.bulkVoltage);
  float absorptionTarget = calculateTempCompensatedVoltage(activeBat.absorptionVoltage);
  float floatTarget = calculateTempCompensatedVoltage(activeBat.floatVoltage);
  
  ChargeState newState = chargeState;
  
  switch(chargeState){
    case CHARGE_OFF:
      {
        if(isSafeToCharge() && voltageInput > voltageOutput + 1.0){
          newState = CHARGE_BULK;
        }
      }
      break;
      
    case CHARGE_BULK:
      {
        if(voltageOutput >= bulkTarget - 0.1){
          newState = CHARGE_ABSORPTION;
          absorptionStartTime = millis();
        }
        if(!isSafeToCharge()){
          newState = CHARGE_OFF;
        }
      }
      break;
      
    case CHARGE_ABSORPTION:
      {
        unsigned long absorptionDuration = (millis() - absorptionStartTime) / 60000;
        
        if(currentOutput < activeBat.minCurrent || 
           absorptionDuration >= activeBat.absorptionTime){
          if(activeBat.useFloat){
            newState = CHARGE_FLOAT;
          }
          else{
            newState = CHARGE_OFF;
          }
        }
        
        if(voltageOutput < bulkTarget - 0.5){
          newState = CHARGE_BULK;
        }
        
        if(!isSafeToCharge()){
          newState = CHARGE_OFF;
        }
      }
      break;
      
    case CHARGE_FLOAT:
      {
        if(voltageOutput < floatTarget - 0.5){
          newState = CHARGE_BULK;
        }
        if(!isSafeToCharge()){
          newState = CHARGE_OFF;
        }
      }
      break;
      
    case CHARGE_EQUALIZATION:
      {
        // Not implemented
      }
      break;
  }
  
  if(newState != prevState){
    Serial.print("> Charge: ");
    switch(prevState){
      case CHARGE_OFF: Serial.print("OFF"); break;
      case CHARGE_BULK: Serial.print("BULK"); break;
      case CHARGE_ABSORPTION: Serial.print("ABS"); break;
      case CHARGE_FLOAT: Serial.print("FLOAT"); break;
      case CHARGE_EQUALIZATION: Serial.print("EQ"); break;
    }
    Serial.print(" -> ");
    switch(newState){
      case CHARGE_OFF: Serial.println("OFF"); break;
      case CHARGE_BULK: Serial.println("BULK"); break;
      case CHARGE_ABSORPTION: Serial.println("ABS"); break;
      case CHARGE_FLOAT: Serial.println("FLOAT"); break;
      case CHARGE_EQUALIZATION: Serial.println("EQ"); break;
    }
    lastStateChange = millis();
    prevState = newState;
  }
  
  chargeState = newState;
}

//========================== MPPT ALGORITHM ==========================//
void MPPT_Algorithm(){
  if(mpptMode == 0 || !buckEnable){
    return;
  }
  
  if(powerInput < 1.0 || voltageInput < voltageOutput + 1.0){
    return;
  }
  
  float powerDelta = powerInput - powerInputPrev;
  float voltageDelta = voltageInput - voltageInputPrev;
  
  int adaptiveStep = mpptStepSize;
  if(abs(powerDelta) > 5.0){
    adaptiveStep = min(mpptAdaptiveMax, mpptStepSize * 3);
  }
  else if(abs(powerDelta) < 0.5){
    adaptiveStep = mpptAdaptiveMin;
  }
  
  if(powerDelta > mpptDelta){
    if(voltageDelta > 0){
      PWM = max(0, PWM - adaptiveStep);
    }
    else{
      PWM = min(pwmMaxLimited, PWM + adaptiveStep);
    }
  }
  else if(powerDelta < -mpptDelta){
    if(voltageDelta > 0){
      PWM = min(pwmMaxLimited, PWM + adaptiveStep);
    }
    else{
      PWM = max(0, PWM - adaptiveStep);
    }
  }
  
  powerInputPrev = powerInput;
  voltageInputPrev = voltageInput;
  
  ledcWrite(buck_IN, PWM);
  
  mpptCycles++;
  if(mpptCycles >= 100){
    Serial.print("> MPPT: P=");
    Serial.print(powerInput, 1);
    Serial.print("W V=");
    Serial.print(voltageInput, 1);
    Serial.print("V PWM=");
    Serial.println(PWM);
    mpptCycles = 0;
  }
}

//========================== CONSTANT VOLTAGE CHARGING ==========================//
void constantVoltageCharge(float targetVoltage){
  static float integral = 0;
  const float Kp = 10.0;
  const float Ki = 0.1;
  
  float error = targetVoltage - voltageOutput;
  integral += error;
  integral = constrain(integral, -100, 100);
  
  float adjustment = (Kp * error) + (Ki * integral);
  
  PWM = PWM - (int)adjustment;
  PWM = constrain(PWM, 0, pwmMaxLimited);
  
  ledcWrite(buck_IN, PWM);
}

//========================== MAIN CHARGING ALGORITHM ==========================//
void Charging_Algorithm(){
  updateChargeState();
  
  // Calculate temp comp targets OUTSIDE switch
  float absorptionTarget = calculateTempCompensatedVoltage(activeBat.absorptionVoltage);
  float floatTarget = calculateTempCompensatedVoltage(activeBat.floatVoltage);
  
  switch(chargeState){
    case CHARGE_OFF:
      {
        if(buckEnable){
          buck_Disable();
        }
        PWM = 0;
        ledcWrite(buck_IN, PWM);
      }
      break;
      
    case CHARGE_BULK:
      {
        if(!buckEnable){
          buck_Enable();
          initiateSoftStart();
        }
        
        MPPT_Algorithm();
        
        if(currentOutput > activeBat.maxCurrent){
          PWM = max(0, PWM - 2);
          ledcWrite(buck_IN, PWM);
        }
      }
      break;
      
    case CHARGE_ABSORPTION:
      {
        if(!buckEnable){
          buck_Enable();
          initiateSoftStart();
        }
        
        constantVoltageCharge(absorptionTarget);
      }
      break;
      
    case CHARGE_FLOAT:
      {
        if(!buckEnable){
          buck_Enable();
          initiateSoftStart();
        }
        
        constantVoltageCharge(floatTarget);
      }
      break;
      
    case CHARGE_EQUALIZATION:
      {
        // Not implemented
      }
      break;
  }
  
  PWM = constrain(PWM, 0, pwmMaxLimited);
}

//========================== HELPER FUNCTIONS ==========================//
String getChargeStateString(){
  switch(chargeState){
    case CHARGE_OFF: return "OFF";
    case CHARGE_BULK: return "BULK";
    case CHARGE_ABSORPTION: return "ABS";
    case CHARGE_FLOAT: return "FLOAT";
    case CHARGE_EQUALIZATION: return "EQ";
    default: return "???";
  }
}

String getBatteryTypeString(){
  switch(batteryType){
    case BAT_LEAD_ACID: return "Lead Acid";
    case BAT_AGM: return "AGM";
    case BAT_GEL: return "GEL";
    case BAT_LIFEPO4: return "LiFePO4";
    case BAT_LIION: return "Li-Ion";
    default: return "Unknown";
  }
}
