/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                          8_LCD_MENU.INO                                           ║
  ║                      (Vylepšená verzia V1.2 - FIXED)                             ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

//========================== LCD MENU VARIABLES ==========================//
int lcdBacklightLevel = 255;
unsigned long lastButtonPress = 0;
const unsigned long backlightTimeout = 60000;

//========================== BUTTON READING ==========================//
void readButtons(){
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;
  
  currentButtonMillis = millis();
  
  if(currentButtonMillis - lastDebounceTime > debounceDelay){
    
    bool leftPressed = (digitalRead(buttonLeft) == LOW);
    bool rightPressed = (digitalRead(buttonRight) == LOW);
    bool backPressed = (digitalRead(buttonBack) == LOW);
    bool selectPressed = (digitalRead(buttonSelect) == LOW);
    
    if(leftPressed && !buttonLeftStatus){
      buttonLeftCommand = 1;
      lastButtonPress = millis();
    }
    if(rightPressed && !buttonRightStatus){
      buttonRightCommand = 1;
      lastButtonPress = millis();
    }
    if(backPressed && !buttonBackStatus){
      buttonBackCommand = 1;
      lastButtonPress = millis();
    }
    if(selectPressed && !buttonSelectStatus){
      buttonSelectCommand = 1;
      lastButtonPress = millis();
    }
    
    buttonLeftStatus = leftPressed;
    buttonRightStatus = rightPressed;
    buttonBackStatus = backPressed;
    buttonSelectStatus = selectPressed;
    
    lastDebounceTime = currentButtonMillis;
  }
}

//========================== PROGRESS BAR ==========================//
void drawProgressBar(int row, int value, int maxValue){
  lcd.setCursor(0, row);
  lcd.print("[");
  
  int barWidth = 18;
  int filledBars = (value * barWidth) / maxValue;
  
  for(int i = 0; i < barWidth; i++){
    if(i < filledBars){
      lcd.print("=");
    }
    else{
      lcd.print(" ");
    }
  }
  
  lcd.print("]");
}

//========================== LCD DISPLAY FUNCTIONS ==========================//
void displayMainScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MPPT Charge Ctrl");
  
  lcd.setCursor(0, 1);
  lcd.print("In:");
  lcd.print(voltageInput, 1);
  lcd.print("V ");
  lcd.print(currentInput, 1);
  lcd.print("A");
  
  lcd.setCursor(0, 2);
  lcd.print("Out:");
  lcd.print(voltageOutput, 1);
  lcd.print("V ");
  lcd.print(batteryPercent);
  lcd.print("%");
  
  lcd.setCursor(0, 3);
  lcd.print(getChargeStateString());
  lcd.print(" ");
  lcd.print(temperature);
  lcd.print("C");
}

void displayPowerScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("=== POWER ===");
  
  lcd.setCursor(0, 1);
  lcd.print("Pin: ");
  lcd.print(powerInput, 1);
  lcd.print("W");
  
  lcd.setCursor(0, 2);
  lcd.print("Pout: ");
  lcd.print(powerOutput, 1);
  lcd.print("W");
  
  lcd.setCursor(0, 3);
  lcd.print("Eff: ");
  lcd.print(buckEfficiency, 1);
  lcd.print("%  PWM:");
  lcd.print((PWM * 100) / pwmMaxLimited);
  lcd.print("%");
}

void displayBatteryScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("=== BATTERY ===");
  
  lcd.setCursor(0, 1);
  lcd.print("Type: ");
  lcd.print(getBatteryTypeString());
  
  lcd.setCursor(0, 2);
  lcd.print("State: ");
  lcd.print(getChargeStateString());
  
  lcd.setCursor(0, 3);
  drawProgressBar(3, batteryPercent, 100);
}

void displayEnergyScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("=== ENERGY ===");
  
  lcd.setCursor(0, 1);
  if(kWh >= 1.0){
    lcd.print("Total: ");
    lcd.print(kWh, 2);
    lcd.print(" kWh");
  }
  else{
    lcd.print("Total: ");
    lcd.print(Wh, 0);
    lcd.print(" Wh");
  }
  
  lcd.setCursor(0, 2);
  lcd.print("Savings: ");
  lcd.print(energySavings, 2);
  lcd.print(" E");
  
  lcd.setCursor(0, 3);
  lcd.print("Days: ");
  lcd.print(daysRunning, 1);
}

void displaySystemScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("=== SYSTEM ===");
  
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C  Fan:");
  lcd.print(fanStatus ? "ON " : "OFF");
  
  lcd.setCursor(0, 2);
  lcd.print("MPPT: ");
  lcd.print(mpptMode ? "ON " : "OFF");
  lcd.print(" WiFi:");
  lcd.print(WIFI ? "ON" : "OFF");
  
  lcd.setCursor(0, 3);
  lcd.print("FW: ");
  lcd.print(firmwareInfo);
}

void displaySettingsScreen(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("=== SETTINGS ===");
  
  lcd.setCursor(0, 1);
  lcd.print("< Battery Type >");
  
  lcd.setCursor(0, 2);
  lcd.print("< MPPT Mode >");
  
  lcd.setCursor(0, 3);
  lcd.print("< Save & Exit >");
}

void displayBatteryTypeSelect(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("= SELECT BAT =");
  
  lcd.setCursor(0, 1);
  lcd.print("< Lead Acid >");
  
  lcd.setCursor(0, 2);
  lcd.print("< AGM >");
  
  lcd.setCursor(0, 3);
  lcd.print("< GEL >");
}

void displayMPPTModeSelect(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("=== MPPT ===");
  
  lcd.setCursor(0, 1);
  lcd.print("Current: ");
  lcd.print(mpptMode ? "ON" : "OFF");
  
  lcd.setCursor(0, 2);
  lcd.print("< Toggle >");
  
  lcd.setCursor(0, 3);
  lcd.print("< Back >");
}

//========================== LCD MENU NAVIGATION ==========================//
void handleMenuNavigation(){
  
  if(buttonLeftCommand){
    buttonLeftCommand = 0;
    menuPage--;
    if(menuPage < 0) menuPage = 5;
  }
  
  if(buttonRightCommand){
    buttonRightCommand = 0;
    menuPage++;
    if(menuPage > 5) menuPage = 0;
  }
  
  if(buttonBackCommand){
    buttonBackCommand = 0;
    if(settingMode){
      settingMode = 0;
      menuPage = 0;
    }
    else{
      menuPage = 0;
    }
  }
  
  if(buttonSelectCommand){
    buttonSelectCommand = 0;
    
    if(menuPage == 5 && !settingMode){
      settingMode = 1;
      setMenuPage = 0;
    }
    else if(settingMode){
      handleSettingsSelection();
    }
  }
}

//========================== HANDLE SETTINGS SELECTION ==========================//
void handleSettingsSelection(){
  // FIXED: Use proper int comparisons for setMenuPage
  if(setMenuPage == 0){
    subMenuPage = 0;
    setMenuPage = 1;
  }
  else if(setMenuPage == 1){
    if(subMenuPage == 0){
      setBatteryProfile(BAT_LEAD_ACID);
    }
    else if(subMenuPage == 1){
      setBatteryProfile(BAT_AGM);
    }
    else if(subMenuPage == 2){
      setBatteryProfile(BAT_GEL);
    }
    saveToEEPROM();
    settingMode = 0;
    menuPage = 0;
  }
  else if(setMenuPage == 2){  // This is OK now - both are ints
    mpptMode = !mpptMode;
    saveToEEPROM();
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("MPPT: ");
    lcd.print(mpptMode ? "ON" : "OFF");
    delay(1000);
    settingMode = 0;
    menuPage = 0;
  }
  else if(setMenuPage == 3){  // This is OK now - both are ints
    saveToEEPROM();
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Saved!");
    delay(1000);
    settingMode = 0;
    menuPage = 0;
  }
}

//========================== LCD BACKLIGHT MANAGEMENT ==========================//
void manageLCDBacklight(){
  if(!enableLCDBacklight){
    lcd.setBacklight(LOW);
    return;
  }
  
  if(millis() - lastButtonPress > backlightTimeout){
    lcdBacklightLevel = 50;
  }
  else{
    lcdBacklightLevel = 255;
  }
  
  if(lcdBacklightLevel > 128){
    lcd.setBacklight(HIGH);
  }
  else{
    lcd.setBacklight(LOW);
  }
}

//========================== MAIN LCD MENU FUNCTION ==========================//
void LCD_Menu(){
  
  if(!enableLCD){
    return;
  }
  
  readButtons();
  handleMenuNavigation();
  manageLCDBacklight();
  
  currentLCDMillis = millis();
  if(currentLCDMillis - prevLCDMillis >= millisLCDInterval){
    prevLCDMillis = currentLCDMillis;
    
    if(settingMode){
      if(setMenuPage == 0){
        displaySettingsScreen();
      }
      else if(setMenuPage == 1){
        displayBatteryTypeSelect();
      }
      else if(setMenuPage == 2){  // OK now
        displayMPPTModeSelect();
      }
    }
    else{
      switch(menuPage){
        case 0:
          displayMainScreen();
          break;
        case 1:
          displayPowerScreen();
          break;
        case 2:
          displayBatteryScreen();
          break;
        case 3:
          displayEnergyScreen();
          break;
        case 4:
          displaySystemScreen();
          break;
        case 5:
          displaySettingsScreen();
          break;
      }
    }
  }
}
