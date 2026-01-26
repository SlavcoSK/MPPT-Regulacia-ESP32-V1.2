/*  PROJECT FUGU FIRMWARE V1.10 (DIY 1kW MPPT solárny regulátor nabíjania s otvoreným zdrojovým kódom)
* Autor: TechBuilder (Angelo Casimiro)
* STAV FIRMWARE: Overená stabilná verzia zostavy
* (Kontaktujte ma pre experimentálne beta verzie)
 *  -----------------------------------------------------------------------------------------------------------
 *  DATE CREATED:  02/07/2021 
 *  DATE MODIFIED: 30/08/2021
 *  -----------------------------------------------------------------------------------------------------------
 *  CONTACTS:
 *  GitHub - www.github.com/AngeloCasi (New firmware releases will only be available on GitHub Link)
 *  Email - casithebuilder@gmail.com
 *  YouTube - www.youtube.com/TechBuilder
 *  Facebook - www.facebook.com/AngeloCasii
 *  -----------------------------------------------------------------------------------------------------------
 *  VLASTNOSTI PROGRAMU:
* - MPPT algoritmus s poruchami a CC-CV
* - Telemetria aplikácie Blynk Phone cez WiFi a Bluetooth
* - Voliteľný režim nabíjačky/zdroja (môže fungovať ako programovateľný buck prevodník)
* - Odomknutý dvojjadrový ESP32 (pomocou xTaskCreatePinnedToCore(); )
* - Presné sledovanie ADC, automatická detekcia ADS1115/ADS1015 (16-bitový/12-bitový I2C ADC)
* - Automatická kalibrácia prúdového senzora ACS712-30A
* - Vybavený protokolom ochrany proti odpojeniu batérie a obnoveniu odpojenia vstupu
* - LCD menu (s nastaveniami a 4 rozloženiami displeja)
* - Flash pamäť (funkcia uloženia nastavení bez nutnosti výpadku energie)
* - Nastaviteľné rozlíšenie PWM (8-16-bitový)
* - Nastaviteľná frekvencia prepínania PWM (1,2 kHz - 312 kHz)
 *  -----------------------------------------------------------------------------------------------------------
 *  POKYNY K PROGRAMU:
* 1.) Pred použitím si pozrite video tutoriál na YouTube.
* 2.) Nainštalujte potrebné knižnice Arduino pre integrované obvody.
* 3.) Vyberte Nástroje > Vývojová doska ESP32.
* 4.) Neupravujte kód, pokiaľ neviete, čo robíte.
* 5.) Topológia synchrónneho buck meniča MPPT je závislá od kódu a môže narušiť algoritmus.
* a protokoly bezpečnostnej ochrany môžu byť mimoriadne nebezpečné, najmä pri práci s HVDC.
* 6.) Nainštalujte Blynk Legacy pre prístup k funkcii telemetrie aplikácie telefónu.
* 7.) Do tohto programu zadajte autentifikačný token Blynk, ktorý vám Blynk poslal na váš e-mail po registrácii.
* 8.) Do tohto programu zadajte SSID WiFi a heslo.
* 9.) Pri použití režimu iba WiFi zmeňte „disableFlashAutoLoad = 0“ na = 1 (LCD a tlačidlá nie sú nainštalované).
* toto zabráni jednotke MPPT načítať uložené nastavenia z flash pamäte a načíta premennú Arduino.
* deklarácie uvedené nižšie.
 *  -----------------------------------------------------------------------------------------------------------
 *  GOOGLE DRIVE PROJECT LINK: coming soon
 *  INSTRUCTABLE TUTORIAL LINK: coming soon
 *  YOUTUBE TUTORIAL LINK: www.youtube.com/watch?v=ShXNJM6uHLM
 *  GITHUB UPDATED FUGU FIRMWARE LINK: github.com/AngeloCasi/FUGU-ARDUINO-MPPT-FIRMWARE
 *  -----------------------------------------------------------------------------------------------------------
 *  ACTIVE CHIPS USED IN FIRMWARE:
 *  - ESP32 WROOM32
 *  - ADS1115/ADS1015 I2C ADC
 *  - ACS712-30A Current Sensor IC
 *  - IR2104 MOSFET Driver
 *  - CH340C USB TO UART IC
 *  - 16X2 I2C Character LCD

 *  OTHER CHIPS USED IN PROJECT:
 *  - XL7005A 80V 0.4A Buck Regulator (2x)
 *  - AMS1115-3.3 LDO Linear Regulator 
 *  - AMS1115-5.0 LDO Linear Regulator  
 *  - CSD19505 N-ch MOSFETS (3x)
 *  - B1212 DC-DC Isolated Converter
 *  - SS310 Diodes
 */
//================================ MPPT FIRMWARE LCD MENU INFO =====================================//
// Riadky nižšie slúžia pre informácie o verzii firmvéru zobrazené na LCD rozhraní ponuky MPPT.    //
//==================================================================================================//
String 
firmwareInfo      = "V1.10   ",
firmwareDate      = "30/08/21",
firmwareContactR1 = "www.youtube.com/",  
firmwareContactR2 = "TechBuilder     ";        
           
//====================== ARDUINO LIBRARIES (ESP32 Compatible Libraries) ============================//
// Na naprogramovanie MPPT jednotky si budete musieť stiahnuť a nainštalovať nasledujúce knižnice. //
// Návod na používanie MPPT nájdete na YouTube kanáli TechBuilder.                         //
//============================================================================================= ====//
#include <EEPROM.h>                 //SYSTEM PARAMETER  - EEPROM Library (By: Arduino)
#include <Wire.h>                   //SYSTEM PARAMETER  - WIRE Library (By: Arduino)
#include <SPI.h>                    //SYSTEM PARAMETER  - SPI Library (By: Arduino)
#include <WiFi.h>                   //SYSTEM PARAMETER  - WiFi Library (By: Arduino)
#include <WiFiClient.h>             //SYSTEM PARAMETER  - WiFi Library (By: Arduino)
#include <BlynkSimpleEsp32.h>       //SYSTEM PARAMETER  - Blynk WiFi Library For Phone App 
#include <LiquidCrystal_I2C.h>      //SYSTEM PARAMETER  - ESP32 LCD Compatible Library (By: Robojax)
#include <Adafruit_ADS1X15.h>       //SYSTEM PARAMETER  - ADS1115/ADS1015 ADC Library (By: Adafruit)
LiquidCrystal_I2C lcd(0x27,16,2);   //SYSTEM PARAMETER  - Configure LCD RowCol Size and I2C Address
TaskHandle_t Core2;                 //SYSTEM PARAMETER  - Used for the ESP32 dual core operation
//Adafruit_ADS1015 ads;               //SYSTEM PARAMETER  - ADS1015 ADC Library (By: Adafruit) Kindly delete this line if you are using ADS1115
Adafruit_ADS1115 ads;             //SYSTEM PARAMETER  - ADS1115 ADC Library (By: Adafruit) Kindly uncomment this if you are using ADS1115

//====================================== USER PARAMETERS ===========================================//
// Nižšie uvedené parametre sú predvolené parametre používané, keď nastavenia MPPT nabíjačky neboli //
// nastavené alebo uložené prostredníctvom rozhrania ponuky LCD alebo aplikácie WiFi mobilného telefónu. Niektoré parametre tu //
// by vám umožnili prepísať alebo odomknúť funkcie pre pokročilých používateľov (nastavenia nie sú v ponuke LCD) //
//==================================================================================================//
#define backflow_MOSFET 27          //SYSTEM PARAMETER - Spätný MOSFET
#define buck_IN         33          //SYSTEM PARAMETER - PWM ovládač Buck MOSFETu
#define buck_EN         32          //SYSTEM PARAMETER - Pin aktivácie ovládača Buck MOSFET
#define LED             2           //SYSTEM PARAMETER - LED indikátor GPIO pin
#define FAN             16          //SYSTEM PARAMETER - GPIO pin ventilátora
#define ADC_ALERT       34          //SYSTEM PARAMETER - GPIO pin ventilátora
#define TempSensor      35          //SYSTEM PARAMETER - TGPIO pin teplotného senzora
#define buttonLeft      18          //SYSTEM PARAMETER - 
#define buttonRight     17          //SYSTEM PARAMETER -
#define buttonBack      19          //SYSTEM PARAMETER - 
#define buttonSelect    23          //SYSTEM PARAMETER -

//========================================= WiFi SSID ==============================================//
// Tento MPPT firmvér používa telefónnu aplikáciu Blynk a knižnicu Arduino na ovládanie a telemetriu údajov //
// Vyplňte svoje WiFi SSID a heslo. Po registrácii na platforme Blynk budete musieť získať aj vlastný autentifikačný token //
// z e-mailu.            //
//==================================================================================================//
char 
auth[] = "Blynk token",   //   POUŽÍVATEĽSKÝ PARAMETER - Zadajte autentifikačný token Blynk (z e-mailu po registrácii)
ssid[] = "Wifi ssid",                   //   USER PARAMETER - Enter Your WiFi SSID
pass[] = " heslo";               //   USER PARAMETER - Enter Your WiFi Password

//====================================== USER PARAMETERS ==========================================//
// Nižšie uvedené parametre sú predvolené parametre používané, keď nastavenia MPPT nabíjačky neboli //
// nastavené alebo uložené prostredníctvom rozhrania ponuky LCD alebo aplikácie WiFi mobilného telefónu. Niektoré parametre tu //
// vám umožnia prepísať alebo odomknúť funkcie pre pokročilých používateľov (nastavenia nie sú v ponuke LCD)//
//=================================================================================================//
bool                                  
MPPT_Mode               = 1,           //   USER PARAMETER - Povoliť algoritmus MPPT, keď je vypnutý, nabíjačka používa algoritmus CC-CV 
output_Mode             = 1,           //   USER PARAMETER - 0 = REŽIM NAPÁJACIEHO ZÁSOBNÍKA, 1 = Režim nabíjačky 
disableFlashAutoLoad    = 0,           //   USER PARAMETER - Núti MPPT, aby nepoužíval nastavenia uložené v pamäti, povolenie tejto hodnoty „1“ predvolene nastaví naprogramované nastavenia firmvéru.
enablePPWM              = 1,           //   USER PARAMETER - Umožňuje prediktívnu PWM, čo zrýchľuje reguláciu (platí len pre nabíjanie batérií).
enableWiFi              = 1,           //   USER PARAMETER - Povoliť pripojenie Wi-Fi
enableFan               = 1,           //   USER PARAMETER - Povoliť chladiaci ventilátor
enableBluetooth         = 1,           //   USER PARAMETER - Povoliť pripojenie Bluetooth
enableLCD               = 1,           //   USER PARAMETER - Povoliť LCD displej
enableLCDBacklight      = 1,           //   USER PARAMETER - Povoliť podsvietenie LCD displeja
overrideFan             = 0,           //   USER PARAMETER - Ventilátor je stále zapnutý
enableDynamicCooling    = 0;           //   USER PARAMETER - Povoliť PWM riadenie chladenia 
int
serialTelemMode         = 1,           //  USER PARAMETER - Vyberá sériový telemetrický prenos údajov (0 – Zakázať sériový port, 1 – Zobraziť všetky údaje, 2 – Zobraziť základné údaje, 3 – Iba čísla)
pwmResolution           = 11,          //  USER PARAMETER - PWM Bit Resolution 
pwmFrequency            = 39000,       //  USER PARAMETER - PWM Switching Frequency - Hz (For Buck)
temperatureFan          = 60,          //  USER PARAMETER - Temperature threshold for fan to turn on
temperatureMax          = 90,          //  USER PARAMETER - Overtemperature, System Shudown When Exceeded (deg C)
telemCounterReset       = 0,           //  USER PARAMETER - Reset Telem Data Every (0 = Never, 1 = Day, 2 = Week, 3 = Month, 4 = Year) 
errorTimeLimit          = 1000,        //  USER PARAMETER - Time interval for reseting error counter (milliseconds)  
errorCountLimit         = 5,           //  USER PARAMETER - Maximum number of errors  
millisRoutineInterval   = 250,         //  USER PARAMETER - Time Interval Refresh Rate For Routine Functions (ms)
millisSerialInterval    = 1,           //  USER PARAMETER - Time Interval Refresh Rate For USB Serial Datafeed (ms)
millisLCDInterval       = 1000,        //  USER PARAMETER - Time Interval Refresh Rate For LCD Display (ms)
millisWiFiInterval      = 2000,        //  USER PARAMETER - Time Interval Refresh Rate For WiFi Telemetry (ms)
millisLCDBackLInterval  = 2000,        //  USER PARAMETER - Time Interval Refresh Rate For WiFi Telemetry (ms)
backlightSleepMode      = 0,           //  USER PARAMETER - 0 = Never, 1 = 10secs, 2 = 5mins, 3 = 1hr, 4 = 6 hrs, 5 = 12hrs, 6 = 1 day, 7 = 3 days, 8 = 1wk, 9 = 1month
baudRate                = 500000;      //  USER PARAMETER - USB Serial Baud Rate (bps)

float 
voltageBatteryMax       = 14.3000,     //   USER PARAMETER - Maximálne nabíjacie napätie batérie (výstupné V)
voltageBatteryMin       = 12.1000,     //   USER PARAMETER - Minimálne nabíjacie napätie batérie (výstupné V)
currentCharging         = 5.0000,     //   USER PARAMETER - Maximálny nabíjací prúd (A - výstup)
electricalPrice         = 0.15415;      //   USER PARAMETER - Vstupná elektrická cena za kWh (dolár/kWh,euro/kWh,peso/kWh)


//================================== CALIBRATION PARAMETERS =======================================//
// Nižšie uvedené parametre je možné upraviť pre návrh vlastných MPPT regulátorov nabíjania. Upravte //
// hodnoty nižšie iba v prípade, že viete, čo robíte. Nižšie uvedené hodnoty boli predkalibrované pre //
// MPPT regulátory nabíjania navrhnuté spoločnosťou TechBuilder (Angelo S. Casimiro)                            //
//=================================================================================================//
bool
ADS1115_Mode            = 0;          //  KALIBRÁČNY PARAMETER - Pre model ADS1015 ADC použite 1, pre model ADS1115 ADC použite 0
int
ADC_GainSelect          = 2,          //  CALIB PARAMETER - ADC Gain Selection (0→±6.144V 3mV/bit, 1→±4.096V 2mV/bit, 2→±2.048V 1mV/bit)
avgCountVS              = 3,          //  CALIB PARAMETER - Voltage Sensor Average Sampling Count (Recommended: 3)
avgCountCS              = 4,          //  CALIB PARAMETER - Current Sensor Average Sampling Count (Recommended: 4)
avgCountTS              = 500;        //  CALIB PARAMETER - Temperature Sensor Average Sampling Count
float
inVoltageDivRatio       = 40.2156,    //  CALIB PARAMETER - Input voltage divider sensor ratio (change this value to calibrate voltage sensor)
outVoltageDivRatio      = 24.5000,    //  CALIB PARAMETER - Output voltage divider sensor ratio (change this value to calibrate voltage sensor)
vOutSystemMax           = 50.0000,    //  CALIB PARAMETER - 
cOutSystemMax           = 50.0000,    //  CALIB PARAMETER - 
ntcResistance           = 9000.00,   //  CALIB PARAMETER - NTC temp sensor's resistance. Change to 10000.00 if you are using a 10k NTC
voltageDropout          = 1.0000,     //  CALIB PARAMETER - Buck regulator's dropout voltage (DOV is present due to Max Duty Cycle Limit)
voltageBatteryThresh    = 1.5000,     //  CALIB PARAMETER - Power cuts-off when this voltage is reached (Output V)
currentInAbsolute       = 31.0000,    //  CALIB PARAMETER - Maximum Input Current The System Can Handle (A - Input)
currentOutAbsolute      = 50.0000,    //  CALIB PARAMETER - Maximum Output Current The System Can Handle (A - Input)
PPWM_margin             = 99.5000,    //  CALIB PARAMETER - Minimum Operating Duty Cycle for Predictive PWM (%)
PWM_MaxDC               = 97.0000,    //  CALIB PARAMETER - Maximum Operating Duty Cycle (%) 90%-97% is good
efficiencyRate          = 1.0000,     //  CALIB PARAMETER - Theroretical Buck Efficiency (% decimal)
currentMidPoint         = 2.5250,     //  CALIB PARAMETER - Current Sensor Midpoint (V)
currentSens             = 0.0000,     //  CALIB PARAMETER - Current Sensor Sensitivity (V/A)
currentSensV            = 0.0660,     //  CALIB PARAMETER - Current Sensor Sensitivity (mV/A)
vInSystemMin            = 10.000;     //  CALIB PARAMETER - 

//===================================== SYSTEM PARAMETERS =========================================//
// V tejto sekcii nemeňte hodnoty parametrov. Nižšie uvedené hodnoty sú premenné používané systémovými //
// procesmi. Zmena hodnôt môže poškodiť hardvér MPPT. Prosím, nechajte to tak, ako je! //
// K týmto premenným však môžete pristupovať na získanie údajov potrebných pre vaše modifikácie.                           //
//=================================================================================================//
bool
buckEnable            = 0,           // SYSTEM PARAMETER - Buck Enable Status
fanStatus             = 0,           // SYSTEM PARAMETER - Fan activity status (1 = On, 0 = Off)
bypassEnable          = 0,           // SYSTEM PARAMETER - 
chargingPause         = 0,           // SYSTEM PARAMETER - 
lowPowerMode          = 0,           // SYSTEM PARAMETER - 
buttonRightStatus     = 0,           // SYSTEM PARAMETER -
buttonLeftStatus      = 0,           // SYSTEM PARAMETER - 
buttonBackStatus      = 0,           // SYSTEM PARAMETER - 
buttonSelectStatus    = 0,           // SYSTEM PARAMETER -
buttonRightCommand    = 0,           // SYSTEM PARAMETER - 
buttonLeftCommand     = 0,           // SYSTEM PARAMETER - 
buttonBackCommand     = 0,           // SYSTEM PARAMETER - 
buttonSelectCommand   = 0,           // SYSTEM PARAMETER -
settingMode           = 0,           // SYSTEM PARAMETER -
setMenuPage           = 0,           // SYSTEM PARAMETER -
boolTemp              = 0,           // SYSTEM PARAMETER -
flashMemLoad          = 0,           // SYSTEM PARAMETER -  
confirmationMenu      = 0,           // SYSTEM PARAMETER -      
WIFI                  = 0,           // SYSTEM PARAMETER - Indikátor povolenia WiFi
BNC                   = 1,           // SYSTEM PARAMETER - Indikátor nepripojenej batérie 
REC                   = 0,           // SYSTEM PARAMETER - Indikátor zotavenia z poruchy
FLV                   = 1,           // SYSTEM PARAMETER - Fatálne nízke napätie systému (nemožnosť obnovenia prevádzky)
IUV                   = 1,           // SYSTEM PARAMETER - Indikátor podpätia vstupu
IOV                   = 1,           // SYSTEM PARAMETER - Indikátor prepätia vstupu
IOC                   = 0,           // SYSTEM PARAMETER - 
OUV                   = 0,           // SYSTEM PARAMETER - 
OOV                   = 1,           // SYSTEM PARAMETER - Indikátor prepätia na výstupe
OOC                   = 1,           // SYSTEM PARAMETER - Indikátor nadprúdu výstupu
OTE                   = 1;           // SYSTEM PARAMETER - Indikátor nadmernej teploty
int
inputSource           = 0,           // SYSTEM PARAMETER - 0 = MPPT nemá zdroj napájania, 1 = MPPT používa solárnu energiu ako zdroj napájania, 2 = MPPT používa batériu ako zdroj napájania
avgStoreTS            = 0,           // SYSTEM PARAMETER - Teplotný senzor používa neinvazívne priemerovanie, na priemerovanie priemerných hodnôt sa používa akumulátor
temperature           = 0,           // SYSTEM PARAMETER -
sampleStoreTS         = 0,           // SYSTEM PARAMETER - TS AVG nth Sample
pwmMax                = 0,           // SYSTEM PARAMETER -
pwmMaxLimited         = 0,           // SYSTEM PARAMETER -
PWM                   = 0,           // SYSTEM PARAMETER - PWM vstreknuté do IR2104 (desatinný ekvivalent)
PPWM                  = 0,           // SYSTEM PARAMETER - Prípustný limit spodnej hranice PWM
pwmChannel            = 0,           // SYSTEM PARAMETER -
batteryPercent        = 0,           // SYSTEM PARAMETER -
errorCount            = 0,           // SYSTEM PARAMETER -
menuPage              = 0,           // SYSTEM PARAMETER -
subMenuPage           = 0,           // SYSTEM PARAMETER -
ERR                   = 0,           // SYSTEM PARAMETER - Počet prítomných chýb
conv1                 = 0,           // SYSTEM PARAMETER -
conv2                 = 0,           // SYSTEM PARAMETER -
intTemp               = 0;           // SYSTEM PARAMETER -
float
VSI                   = 0.0000,      // SYSTEM PARAMETER - Napätie ADC snímača vstupného napätia
VSO                   = 0.0000,      // SYSTEM PARAMETER - Raw output voltage sensor ADC voltage
CSI                   = 0.0000,      // SYSTEM PARAMETER - Raw current sensor ADC voltage
CSI_converted         = 0.0000,      // SYSTEM PARAMETER - Actual current sensor ADC voltage 
TS                    = 0.0000,      // SYSTEM PARAMETER - Raw temperature sensor ADC value
powerInput            = 0.0000,      // SYSTEM PARAMETER - Input power (solar power) in Watts
powerInputPrev        = 0.0000,      // SYSTEM PARAMETER - Previously stored input power variable for MPPT algorithm (Watts)
powerOutput           = 0.0000,      // SYSTEM PARAMETER - Output power (battery or charing power in Watts)
energySavings         = 0.0000,      // SYSTEM PARAMETER - Energy savings in fiat currency (Peso, USD, Euros or etc...)
voltageInput          = 0.0000,      // SYSTEM PARAMETER - Input voltage (solar voltage)
voltageInputPrev      = 0.0000,      // SYSTEM PARAMETER - Previously stored input voltage variable for MPPT algorithm
voltageOutput         = 0.0000,      // SYSTEM PARAMETER - Input voltage (battery voltage)
currentInput          = 0.0000,      // SYSTEM PARAMETER - Output power (battery or charing voltage)
currentOutput         = 0.0000,      // SYSTEM PARAMETER - Output current (battery or charing current in Amperes)
TSlog                 = 0.0000,      // SYSTEM PARAMETER - Part of NTC thermistor thermal sensing code
ADC_BitReso           = 0.0000,      // SYSTEM PARAMETER - Systém detekuje vhodný faktor rozlíšenia bitov pre ADC ADS1015/ADS1115
daysRunning           = 0.0000,      // SYSTEM PARAMETER - Stores the total number of days the MPPT device has been running since last powered
Wh                    = 0.0000,      // SYSTEM PARAMETER - Stores the accumulated energy harvested (Watt-Hours)
kWh                   = 0.0000,      // SYSTEM PARAMETER - Stores the accumulated energy harvested (Kiliowatt-Hours)
MWh                   = 0.0000,      // SYSTEM PARAMETER - Stores the accumulated energy harvested (Megawatt-Hours)
loopTime              = 0.0000,      // SYSTEM PARAMETER -
outputDeviation       = 0.0000,      // SYSTEM PARAMETER - Output Voltage Deviation (%)
buckEfficiency        = 0.0000,      // SYSTEM PARAMETER - Measure buck converter power conversion efficiency (only applicable to my dual current sensor version)
floatTemp             = 0.0000,
vOutSystemMin         = 0.0000;     //  CALIB PARAMETER - 
unsigned long 
currentErrorMillis    = 0,           //SYSTEM PARAMETER -
currentButtonMillis   = 0,           //SYSTEM PARAMETER -
currentSerialMillis   = 0,           //SYSTEM PARAMETER -
currentRoutineMillis  = 0,           //SYSTEM PARAMETER -
currentLCDMillis      = 0,           //SYSTEM PARAMETER - 
currentLCDBackLMillis = 0,           //SYSTEM PARAMETER - 
currentWiFiMillis     = 0,           //SYSTEM PARAMETER - 
currentMenuSetMillis  = 0,           //SYSTEM PARAMETER - 
prevButtonMillis      = 0,           //SYSTEM PARAMETER -
prevSerialMillis      = 0,           //SYSTEM PARAMETER -
prevRoutineMillis     = 0,           //SYSTEM PARAMETER -
prevErrorMillis       = 0,           //SYSTEM PARAMETER -
prevWiFiMillis        = 0,           //SYSTEM PARAMETER -
prevLCDMillis         = 0,           //SYSTEM PARAMETER -
prevLCDBackLMillis    = 0,           //SYSTEM PARAMETER -
timeOn                = 0,           //SYSTEM PARAMETER -
loopTimeStart         = 0,           //SYSTEM PARAMETER - Used for the loop cycle stop watch, records the loop start time
loopTimeEnd           = 0,           //SYSTEM PARAMETER - Used for the loop cycle stop watch, records the loop end time
secondsElapsed        = 0;           //SYSTEM PARAMETER - 

//====================================== MAIN PROGRAM =============================================//
// TNižšie uvedené kódy obsahujú všetky systémové procesy pre firmvér MPPT. Väčšina z nich sa nazýva //
// z 8 kariet .ino. Kódy sú príliš dlhé, karty Arduino mi veľmi pomohli s ich organizáciou. //
// Firmvér beží na dvoch jadrách Arduina ESP32, ako je vidieť na dvoch samostatných pároch nastavení a slučiek void //
//. Funkcia xTaskCreatePinnedToCore() vo freeRTOS vám umožňuje prístup k //
// nepoužívanému jadru ESP32 cez Arduino. Áno, vykonáva viacjadrové procesy súčasne!             // 
//=================================================================================================//

//================= CORE0: SETUP (DUAL CORE MODE) =====================//
void coreTwo(void * pvParameters){
 setupWiFi();                                              //TAB#7 - WiFi Initialization
//================= CORE0: LOOP (DUAL CORE MODE) ======================//
  while(1){
    Wireless_Telemetry();                                   //TAB#7 - Wireless telemetry (WiFi & Bluetooth)
    
}}
//================== CORE1: SETUP (DUAL CORE MODE) ====================//
void setup() { 
  
  //SERIAL INITIALIZATION          
  Serial.begin(baudRate);                                   //Set serial baud rate
  Serial.println("> Serial Initialized");                   //Startup message
  
  //GPIO PIN INITIALIZATION
  pinMode(backflow_MOSFET,OUTPUT);                          
  pinMode(buck_EN,OUTPUT);
  pinMode(LED,OUTPUT); 
  pinMode(FAN,OUTPUT);
  pinMode(TS,INPUT); 
  pinMode(ADC_ALERT,INPUT);
  pinMode(buttonLeft,INPUT); 
  pinMode(buttonRight,INPUT); 
  pinMode(buttonBack,INPUT); 
  pinMode(buttonSelect,INPUT); 
  
  //PWM INITIALIZATION
  ledcSetup(pwmChannel,pwmFrequency,pwmResolution);          //Set PWM Parameters
  ledcAttachPin(buck_IN, pwmChannel);                        //Set pin as PWM
  ledcWrite(pwmChannel,PWM);                                 //Write PWM value at startup (duty = 0)
  pwmMax = pow(2,pwmResolution)-1;                           //Get PWM Max Bit Ceiling
  pwmMaxLimited = (PWM_MaxDC*pwmMax)/100.000;                //Get maximum PWM Duty Cycle (pwm limiting protection)
  
  //ADC INITIALIZATION
  ADC_SetGain();                                             //Sets ADC Gain & Range
  ads.begin();                                               //Initialize ADC

  //GPIO INITIALIZATION                          
  buck_Disable();

  //ENABLE DUAL CORE MULTITASKING
  xTaskCreatePinnedToCore(coreTwo,"coreTwo",10000,NULL,0,&Core2,0);
  
  //INITIALIZE AND LIOAD FLASH MEMORY DATA
  EEPROM.begin(512);
  Serial.println("> FLASH MEMORY: STORAGE INITIALIZED");  //Startup message 
  initializeFlashAutoload();                              //Load stored settings from flash memory       
  Serial.println("> FLASH MEMORY: SAVED DATA LOADED");    //Startup message 

  //LCD INITIALIZATION
  if(enableLCD==1){
    lcd.begin();
    lcd.setBacklight(HIGH);
    lcd.setCursor(0,0);
    lcd.print("MPPT INITIALIZED");
    lcd.setCursor(0,1);
    lcd.print("FIRMWARE ");
    lcd.print(firmwareInfo);    
    delay(1500);
    lcd.clear();
  }

  //SETUP FINISHED
  Serial.println("> MPPT HAS INITIALIZED");                //Startup message

}
//================== CORE1: LOOP (DUAL CORE MODE) ======================//
void loop() {
  Read_Sensors();         //TAB#2 - Sensor data measurement and computation
  Device_Protection();    //TAB#3 - Fault detection algorithm  
  System_Processes();     //TAB#4 - Routine system processes 
  Charging_Algorithm();   //TAB#5 - Battery Charging Algorithm                    
  Onboard_Telemetry();    //TAB#6 - Onboard telemetry (USB & Serial Telemetry)
  LCD_Menu();             //TAB#8 - Low Power Algorithm
}
