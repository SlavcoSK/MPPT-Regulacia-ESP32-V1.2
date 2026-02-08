/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                          2_READ_SENSORS.INO                                       ║
  ║                      (Vylepšená verzia V1.2)                                      ║
  ║                                                                                   ║
  ║  Vylepšenia:                                                                      ║
  ║  - Exponenciálny filter namiesto jednoduchého priemeru                           ║
  ║  - Detekcia anomálií v čítaní senzorov                                           ║
  ║  - Kalibračné offsety pre presnejšie meranie                                     ║
  ║  - Vylepšená automatická kalibrácia prúdového senzora                            ║
  ║  - Robustnejšie spracovanie chybných hodnôt                                      ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

//========================== ADC GAIN CONFIGURATION ==========================//
void ADC_SetGain(){
  if(ADS1015_Mode == true){                                                  // ADS1015 12-BIT ADC MODEL
    if(ADC_GainSelect == 0){
      ads.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 3.0000;                                                  // Gain: 2/3x  Range: +/- 6.144V  
    }
    else if(ADC_GainSelect == 1){
      ads.setGain(GAIN_ONE);
      ADC_BitReso = 2.0000;                                                  // Gain: 1x    Range: +/- 4.096V    
    }
    else if(ADC_GainSelect == 2){
      ads.setGain(GAIN_TWO);
      ADC_BitReso = 1.0000;                                                  // Gain: 2x    Range: +/- 2.048V       
    }
  }
  else{                                                                      // ADS1115 16-BIT ADC MODEL
    if(ADC_GainSelect == 0){
      ads.setGain(GAIN_TWOTHIRDS);
      ADC_BitReso = 0.1875;                                                  // Gain: 2/3x  Range: +/- 6.144V
    }
    else if(ADC_GainSelect == 1){
      ads.setGain(GAIN_ONE);
      ADC_BitReso = 0.125;                                                   // Gain: 1x    Range: +/- 4.096V  
    }
    else if(ADC_GainSelect == 2){
      ads.setGain(GAIN_TWO);
      ADC_BitReso = 0.0625;                                                  // Gain: 2x    Range: +/- 2.048V    
    }
  }
}

//========================== RESET SYSTEM VARIABLES ==========================//
void resetVariables(){
  secondsElapsed = 0;
  energySavings  = 0; 
  daysRunning    = 0; 
  timeOn         = 0;
  Wh             = 0.0000;
  kWh            = 0.0000;
  MWh            = 0.0000;
  
  Serial.println("> System Variables Reset");
}

//========================== ANOMALY DETECTION ==========================//
// Funkcia detekuje abnormálne hodnoty zo senzorov
bool detectAnomaly(float value, float min, float max, String sensorName){
  if(value < min || value > max || isnan(value) || isinf(value)){
    Serial.print("> ANOMALY DETECTED: ");
    Serial.print(sensorName);
    Serial.print(" = ");
    Serial.print(value);
    Serial.print(" (Range: ");
    Serial.print(min);
    Serial.print(" - ");
    Serial.print(max);
    Serial.println(")");
    return true;
  }
  return false;
}

//========================== EXPONENTIAL FILTER ==========================//
// Exponenciálny filter pre vyhladenie merania senzorov
float exponentialFilter(float newValue, float prevValue, float alpha){
  return (alpha * newValue) + ((1.0 - alpha) * prevValue);
}

//========================== MAIN SENSOR READING FUNCTION ==========================//
void Read_Sensors(){
  
  //==================== TEMPERATURE SENSOR ====================//
  if(sampleStoreTS <= avgCountTS){
    TS = TS + analogRead(TempSensor);
    sampleStoreTS++;   
  }
  else{
    TS = TS / sampleStoreTS;
    
    // NTC thermistor temperature calculation
    if(TS > 0){  // Prevent log(0) error
      TSlog = log(ntcResistance * (4095.00 / TS - 1.00));
      temperature = (1.0 / (1.009249522e-03 + 2.378405444e-04 * TSlog + 2.019202697e-07 * TSlog * TSlog * TSlog)) - 273.15;
      
      // Anomaly detection for temperature
      if(detectAnomaly(temperature, -20.0, 100.0, "Temperature")){
        temperature = 25.0;  // Default to room temperature if anomaly
      }
    }
    else{
      temperature = 25.0;  // Default value
    }
    
    sampleStoreTS = 0;
    TS = 0;
  }
  
  //==================== VOLTAGE & CURRENT SENSORS ====================//
  VSI = 0.0000;      // Clear previous input voltage 
  VSO = 0.0000;      // Clear previous output voltage  
  CSI = 0.0000;      // Clear previous current  
  
  // VOLTAGE SENSOR - Instantaneous averaging with anomaly detection
  for(int i = 0; i < avgCountVS; i++){
    float tempVSI = ads.computeVolts(ads.readADC_SingleEnded(3));
    float tempVSO = ads.computeVolts(ads.readADC_SingleEnded(1));
    
    // Check for valid ADC readings
    if(!isnan(tempVSI) && !isinf(tempVSI) && tempVSI >= 0){
      VSI = VSI + tempVSI;
    }
    if(!isnan(tempVSO) && !isinf(tempVSO) && tempVSO >= 0){
      VSO = VSO + tempVSO;
    }
  }
  
  // Calculate voltages with calibration offsets
  float rawVoltageInput = (VSI / avgCountVS) * inVoltageDivRatio + voltageInputOffset;
  float rawVoltageOutput = (VSO / avgCountVS) * outVoltageDivRatio + voltageOutputOffset;
  
  // Anomaly detection for voltages
  if(detectAnomaly(rawVoltageInput, 0.0, inputVoltageMax, "Input Voltage")){
    rawVoltageInput = voltageInput;  // Keep previous value if anomaly
  }
  if(detectAnomaly(rawVoltageOutput, 0.0, outputVoltageMax, "Output Voltage")){
    rawVoltageOutput = voltageOutput;  // Keep previous value if anomaly
  }
  
  // Apply exponential filter for smooth readings
  voltageInput = exponentialFilter(rawVoltageInput, prevVin, filterAlpha);
  voltageOutput = exponentialFilter(rawVoltageOutput, prevVout, filterAlpha);
  
  // Store for next iteration
  prevVin = voltageInput;
  prevVout = voltageOutput;
  
  //==================== CURRENT SENSOR ====================//
  for(int i = 0; i < avgCountCS; i++){
    float tempCSI = ads.computeVolts(ads.readADC_SingleEnded(2));
    
    // Check for valid ADC readings
    if(!isnan(tempCSI) && !isinf(tempCSI) && tempCSI >= 0){
      CSI = CSI + tempCSI;
    }
  }
  
  CSI_converted = (CSI / avgCountCS) * 1.3300;
  
  // Calculate input current with calibration
  float rawCurrentInput = ((CSI_converted - currentMidPoint) * -1) / currentSensV + currentInputOffset;
  
  // Prevent negative current readings
  if(rawCurrentInput < 0){
    rawCurrentInput = 0.0000;
  }
  
  // Anomaly detection for current
  if(detectAnomaly(rawCurrentInput, 0.0, outputCurrentMax, "Input Current")){
    rawCurrentInput = currentInput;  // Keep previous value if anomaly
  }
  
  // Apply exponential filter
  currentInput = exponentialFilter(rawCurrentInput, prevIin, filterAlpha);
  prevIin = currentInput;
  
  // Calculate output current based on power conservation
  if(voltageOutput <= vOutSystemMin){
    currentOutput = 0.0000;
  }
  else{
    float rawCurrentOutput = (voltageInput * currentInput) / voltageOutput;
    
    // Anomaly detection
    if(detectAnomaly(rawCurrentOutput, 0.0, outputCurrentMax, "Output Current")){
      rawCurrentOutput = currentOutput;
    }
    
    // Apply exponential filter
    currentOutput = exponentialFilter(rawCurrentOutput, prevIout, filterAlpha);
    prevIout = currentOutput;
  }
  
  //==================== POWER SOURCE DETECTION ====================//
  if(voltageInput <= 3 && voltageOutput <= 3){
    inputSource = 0;  // System powered by USB only
  }
  else if(voltageInput > voltageOutput + 0.5){
    inputSource = 1;  // System running on solar
  }
  else if(voltageInput < voltageOutput - 0.5){
    inputSource = 2;  // System running on battery
  }
  // else keep previous state (hysteresis)
  
  //==================== AUTOMATIC CURRENT SENSOR CALIBRATION ====================//
  // Calibrate when buck is disabled and no faults
  if(buckEnable == 0 && FLV == 0 && OOV == 0 && inputSource == 1){
    static int calibrationCount = 0;
    static float calibrationSum = 0.0;
    
    calibrationSum += ((CSI / avgCountCS) * 1.3300);
    calibrationCount++;
    
    // Average over 10 samples for stable calibration
    if(calibrationCount >= 10){
      currentMidPoint = (calibrationSum / calibrationCount) - 0.003;
      calibrationCount = 0;
      calibrationSum = 0.0;
      Serial.print("> Current Sensor Auto-Calibrated: ");
      Serial.println(currentMidPoint, 4);
    }
  }
  
  //==================== POWER COMPUTATION ====================//
  powerInput = voltageInput * currentInput;
  powerOutput = voltageInput * currentInput * efficiencyRate;
  
  // Calculate actual efficiency if possible
  if(powerInput > 0.1){  // Avoid division by very small numbers
    buckEfficiency = (voltageOutput * currentOutput) / powerInput * 100.0;
    buckEfficiency = constrain(buckEfficiency, 0.0, 100.0);
  }
  
  outputDeviation = (voltageOutput / voltageBatteryMax) * 100.000;
  
  //==================== STATE OF CHARGE - Battery Percentage ====================//
  batteryPercent = ((voltageOutput - voltageBatteryMin) / (voltageBatteryMax - voltageBatteryMin)) * 100;
  batteryPercent = constrain(batteryPercent, 0, 100);
  
  //==================== TIME DEPENDENT SENSOR DATA COMPUTATION ====================//
  currentRoutineMillis = millis();
  
  if(currentRoutineMillis - prevRoutineMillis >= millisRoutineInterval){
    prevRoutineMillis = currentRoutineMillis;
    
    // Accumulate energy harvested
    float energyInterval = powerInput / (3600.000 * (1000.000 / millisRoutineInterval));
    
    // Only accumulate when actually charging
    if(buckEnable == 1 && powerInput > 0.1){
      Wh = Wh + energyInterval;
      kWh = Wh / 1000.000;
      MWh = Wh / 1000000.000;
    }
    
    // Compute days running
    daysRunning = timeOn / (86400.000 * (1000.000 / millisRoutineInterval));
    timeOn++;
  } 
  
  //==================== OTHER DATA ====================//
  secondsElapsed = millis() / 1000;
  energySavings = electricalPrice * (Wh / 1000.0000);
  
  //==================== EFFICIENCY TRACKING ====================//
  // Track efficiency over time for diagnostics
  static float efficiencySum = 0.0;
  static int efficiencyCount = 0;
  
  if(buckEnable == 1 && powerInput > 1.0){
    efficiencySum += buckEfficiency;
    efficiencyCount++;
    
    // Report average efficiency every 100 samples
    if(efficiencyCount >= 100){
      float avgEfficiency = efficiencySum / efficiencyCount;
      Serial.print("> Average Efficiency: ");
      Serial.print(avgEfficiency, 2);
      Serial.println(" %");
      efficiencySum = 0.0;
      efficiencyCount = 0;
    }
  }
}
