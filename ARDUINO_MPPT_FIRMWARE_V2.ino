/*
  ╔═══════════════════════════════════════════════════════════════════════════════════╗
  ║                    MPPT-Regulacia-ESP32-V1.2                                      ║
  ║                        (Vylepšená verzia V1.2 - FIXED)                            ║
  ╚═══════════════════════════════════════════════════════════════════════════════════╝
*/

//================================ FIRMWARE INFO =========================================//
String 
firmwareInfo    = "V1.2F",
firmwareDate    = "2024.02.07";

//================================== USER PARAMETERS =====================================//
#define backflow_MOSFET       27
#define buck_IN               33
#define buck_EN               32
#define LED                   2
#define FAN                   16
#define ADC_ALERT             35
#define TempSensor            34
#define buttonLeft            18
#define buttonRight           17
#define buttonBack            19
#define buttonSelect          23

//================================== MPPT PARAMETERS =====================================//
float voltageBatteryMax       = 14.4000;  // CHANGED to variable (not #define)
float voltageBatteryMin       = 10.0000;
float currentCharging         = 30.0000;
float electricalPrice         = 9.5000;

//=================================== WIFI SSID AND PASSWORD ==============================//
#define WIFI_SSID "MPPT"
#define WIFI_PASSWORD "mppt1234"

//============================ ARDUINO LIBRARIES (ESP32) =================================//
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>

Adafruit_ADS1115 ads;
LiquidCrystal_I2C lcd(0x27,20,4);
TaskHandle_t Core2;

//========================== BATTERY TYPE ENUM ==========================================//
enum BatteryType {
  BAT_LEAD_ACID,
  BAT_AGM,
  BAT_GEL,
  BAT_LIFEPO4,
  BAT_LIION
};

BatteryType batteryType = BAT_AGM;

//========================== CHARGING STATES ENUM ========================================//
enum ChargeState {
  CHARGE_OFF,
  CHARGE_BULK,
  CHARGE_ABSORPTION,
  CHARGE_FLOAT,
  CHARGE_EQUALIZATION
};

ChargeState chargeState = CHARGE_OFF;
unsigned long absorptionStartTime = 0;

//======================= CALIBRATION PARAMETERS ==============================//
float inVoltageDivRatio   = 40.2156;
float outVoltageDivRatio  = 24.5000;
float vOutSystemMin       = 0.5000;
float currentSensV        = 0.0660;
float currentMidPoint     = 2.5000;
float tempCoefficient     = -0.018;
float referenceTemp       = 25.0;
float voltageInputOffset  = 0.0000;
float voltageOutputOffset = 0.0000;
float currentInputOffset  = 0.0000;

//========================== CALIBRATION PARAMETERS (Fixed) ==============================//
bool ADS1015_Mode         = 0;
int avgCountVS            = 3;
int avgCountCS            = 4;
int avgCountTS            = 500;
float pwmFrequency        = 25000;
int pwmResolution         = 11;
int pwmMax                = 2047;
int ADC_GainSelect        = 2;
float ADC_BitReso         = 0.0625;
float ntcResistance       = 10000.00;

//============================= SYSTEM PARAMETERS ================================//
bool MPPT_Mode            = 1;
int pwmChannel            = 0;
int PWM_MaxDC             = 97;
float efficiencyRate      = 1.0000;
float delta_IIN           = 0.0050;
float delta_VIN           = 0.0100;
int mpptCycles            = 0;
int dutyCycleStep         = 5;
int mpptMode              = 1;
float temperatureMax      = 80.0000;
float temperatureFan      = 60.0000;
float inputVoltageMin     = 10.0000;
float inputVoltageMax     = 80.0000;
float outputVoltageMax    = 15.5000;
float outputCurrentMax    = 50.0000;
float filterAlpha         = 0.15;
float prevVin             = 0.0;
float prevVout            = 0.0;
float prevIin             = 0.0;
float prevIout            = 0.0;

// TIMING PARAMETERS
unsigned long baudRate              = 500000;
unsigned long millisRoutineInterval = 250;
unsigned long millisSerialInterval  = 1000;
unsigned long millisLCDInterval     = 1000;
unsigned long millisWiFiInterval    = 2000;
bool enableWiFi                     = 1;
bool disableFlashAutoLoad           = 0;
bool enableFan                      = 1;
bool enableBluetooth                = 1;
bool enableLCD                      = 1;
bool enableLCDBacklight             = 1;
bool overrideFan                    = 0;
int serialTelemMode                 = 1;

//===================================== SYSTEM VARIABLES =================================//
bool  
buckEnable            = 0,
fanStatus             = 0,
chargingPause         = 0,
bypassEnable          = 0,
buttonRightStatus     = 0,
buttonLeftStatus      = 0,
buttonBackStatus      = 0,
buttonSelectStatus    = 0,
buttonRightCommand    = 0,
buttonLeftCommand     = 0,
buttonBackCommand     = 0,
buttonSelectCommand   = 0,
settingMode           = 0,
boolTemp              = 0,
flashMemLoad          = 0,
confirmationMenu      = 0,
WIFI                  = 0,
BNC                   = 0,
REC                   = 0,
FLV                   = 0,
IUV                   = 0,
IOV                   = 0,
IOC                   = 0,
OUV                   = 0,
OOV                   = 0,
OOC                   = 0,
OTE                   = 0;

int
inputSource           = 0,
avgStoreTS            = 0,
temperature           = 0,
sampleStoreTS         = 0,
pwmMaxLimited         = 0,
PWM                   = 0,
PPWM                  = 0,
batteryPercent        = 0,
errorCount            = 0,
menuPage              = 0,
subMenuPage           = 0,
ERR                   = 0,
conv1                 = 0,
conv2                 = 0,
intTemp               = 0,
setMenuPage           = 0;

float
VSI                   = 0.0000,
VSO                   = 0.0000,
CSI                   = 0.0000,
CSI_converted         = 0.0000,
TS                    = 0.0000,
powerInput            = 0.0000,
powerInputPrev        = 0.0000,
powerOutput           = 0.0000,
energySavings         = 0.0000,
voltageInput          = 0.0000,
voltageInputPrev      = 0.0000,
voltageOutput         = 0.0000,
currentInput          = 0.0000,
currentOutput         = 0.0000,
TSlog                 = 0.0000,
daysRunning           = 0.0000,
Wh                    = 0.0000,
kWh                   = 0.0000,
MWh                   = 0.0000,
loopTime              = 0.0000,
outputDeviation       = 0.0000,
buckEfficiency        = 0.0000,
floatTemp             = 0.0000,
voltageOutputTemp     = 0.0000;

unsigned long 
currentErrorMillis    = 0,
currentButtonMillis   = 0,
currentSerialMillis   = 0,
currentRoutineMillis  = 0,
currentLCDMillis      = 0,
currentLCDBackLMillis = 0,
currentWiFiMillis     = 0,
currentMenuSetMillis  = 0,
prevButtonMillis      = 0,
prevSerialMillis      = 0,
prevRoutineMillis     = 0,
prevErrorMillis       = 0,
prevWiFiMillis        = 0,
prevLCDMillis         = 0,
prevLCDBackLMillis    = 0,
timeOn                = 0,
loopTimeStart         = 0,
loopTimeEnd           = 0,
secondsElapsed        = 0;

//====================================== MAIN PROGRAM ====================================//

//================= CORE0: SETUP (DUAL CORE MODE) ================//
void coreTwo(void * pvParameters){
  setupWiFi();
  while(1){
    Wireless_Telemetry();
  }
}

//================== CORE1: SETUP (DUAL CORE MODE) ===============//
void setup() { 
  Serial.begin(baudRate);
  Serial.println("> Serial Initialized");
  Serial.println("> Firmware: " + firmwareInfo);
  
  pinMode(backflow_MOSFET, OUTPUT);
  pinMode(buck_EN, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(TempSensor, INPUT);
  pinMode(ADC_ALERT, INPUT);
  pinMode(buttonLeft, INPUT);
  pinMode(buttonRight, INPUT);
  pinMode(buttonBack, INPUT);
  pinMode(buttonSelect, INPUT);
  
  // PWM INIT - ESP32 v3.x API
  ledcAttach(buck_IN, pwmFrequency, pwmResolution);
  ledcWrite(buck_IN, PWM);
  pwmMax = pow(2, pwmResolution) - 1;
  pwmMaxLimited = (PWM_MaxDC * pwmMax) / 100.000;
  Serial.print("> PWM: ");
  Serial.print(pwmFrequency);
  Serial.println(" Hz");
  
  ADC_SetGain();
  ads.begin();
  Serial.println("> ADC OK");
  
  buck_Disable();
  
  xTaskCreatePinnedToCore(coreTwo, "coreTwo", 10000, NULL, 0, &Core2, 0);
  Serial.println("> Dual Core OK");
  
  EEPROM.begin(512);
  initializeFlashAutoload();
  Serial.println("> EEPROM OK");
  
  if(enableLCD == 1){
    lcd.begin();
    lcd.setBacklight(HIGH);
    lcd.setCursor(0, 0);
    lcd.print("MPPT INIT OK");
    lcd.setCursor(0, 1);
    lcd.print("FW: ");
    lcd.print(firmwareInfo);
    delay(2000);
    lcd.clear();
  }
  
  Serial.println("> MPPT READY");
}

//================== CORE1: LOOP (DUAL CORE MODE) ================//
void loop() {
  Read_Sensors();
  Device_Protection();
  System_Processes();
  Charging_Algorithm();
  Onboard_Telemetry();
  LCD_Menu();
  delay(10);
}
