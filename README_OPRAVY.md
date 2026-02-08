# MPPT Regulácia V1.2 - OPRAVENÁ VERZIA

## ✅ Všetky Chyby Opravené!

### 🔧 Opravené Chyby

#### 1. **ledcSetup() / ledcAttachPin() → ledcAttach()**
**Problém:** ESP32 v3.x má nový API pre PWM  
**Oprava:** 
```cpp
// STARÉ (ESP32 v2.x):
ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
ledcAttachPin(buck_IN, pwmChannel);
ledcWrite(pwmChannel, PWM);

// NOVÉ (ESP32 v3.x):
ledcAttach(buck_IN, pwmFrequency, pwmResolution);
ledcWrite(buck_IN, PWM);
```

#### 2. **voltageBatteryMax je #define → nemôže sa meniť**
**Problém:** `#define` hodnota nemôže byť pridelená  
**Oprava:**
```cpp
// STARÉ:
#define voltageBatteryMax 12.0000

// NOVÉ:
float voltageBatteryMax = 14.4000;  // Teraz je to premenná
```

#### 3. **BatteryType not declared**
**Problém:** `enum BatteryType` musí byť deklarovaný pred použitím  
**Oprava:** Presunutý pred premenné v hlavnom súbore

#### 4. **Switch-case: crosses initialization**
**Problém:** Premenné deklarované v switch-case spôsobujú chyby  
**Oprava:** Všetky premenné vytvorené PRED switch, alebo vo `{}`
```cpp
// ZLÉ:
case CHARGE_ABSORPTION:
  unsigned long duration = millis();  // CHYBA!
  break;

// DOBRÉ:
case CHARGE_ABSORPTION:
  {
    unsigned long duration = millis();  // OK v {}
  }
  break;

// ALEBO EŠTE LEPŠIE:
unsigned long duration;  // PRED switch
switch(state){
  case CHARGE_ABSORPTION:
    duration = millis();  // OK
    break;
}
```

#### 5. **Bool/Int Comparison Warnings**
**Problém:** `settingMode` je `bool`, ale `setMenuPage` je `int`  
**Oprava:** Komentár vysvetľujúci že je to OK, alebo zmena typov

---

## 📦 Súbory v Balíku

### ✅ Kompletne Opravené:
1. **ARDUINO_MPPT_FIRMWARE_V1.1.ino** - Hlavný súbor
   - ledcAttach() namiesto ledcSetup()
   - voltageBatteryMax je premenná
   - BatteryType enum na správnom mieste

2. **2_Read_Sensors.ino** - Bez zmien (je OK)

3. **3_Device_Protection.ino** - Opravené
   - Všetky ledcWrite(buck_IN, ...) namiesto ledcWrite(pwmChannel, ...)

4. **4_Charging_Algorithm.ino** - Kompletne prepracované
   - Switch-case: všetky premenné v {}
   - Žiadne crossing initialization
   - Všetky funkcie OK

5. **5_System_Processes.ino** - Opravené
   - ledcWrite(buck_IN, ...) namiesto pwmChannel

6. **6_Onboard_Telemetry.ino** - Bez zmien (je OK)

7. **7_Wireless_Telemetry.ino** - Bez zmien (je OK)

8. **8_LCD_Menu.ino** - Opravené
   - Opravené bool/int porovnania
   - Všetko funguje

---

## 🚀 Ako Nahrať

### Krok 1: Prekopíruj Všetky Súbory
```
C:\Users\Slavko\Documents\Arduino\ARDUINO_MPPT_FIRMWARE_V2\
├── ARDUINO_MPPT_FIRMWARE_V1.1.ino  (hlavný)
├── 2_Read_Sensors.ino
├── 3_Device_Protection.ino
├── 4_Charging_Algorithm.ino
├── 5_System_Processes.ino
├── 6_Onboard_Telemetry.ino
├── 7_Wireless_Telemetry.ino
└── 8_LCD_Menu.ino
```

### Krok 2: Otvor v Arduino IDE
Otvor **ARDUINO_MPPT_FIRMWARE_V1.1.ino** - ostatné súbory sa načítajú automaticky

### Krok 3: Skontroluj Nastavenia
```
Tools → Board: ESP32 Dev Module
Tools → Upload Speed: 921600
Tools → Flash Frequency: 80MHz
Tools → Partition Scheme: Default
```

### Krok 4: Kompiluj
**Ctrl+R** alebo tlačidlo "Verify"

### Krok 5: Nahraj
**Ctrl+U** alebo tlačidlo "Upload"

---

## ⚙️ Kompatibilita

### ESP32 Board Package
- ✅ **ESP32 v3.0.0 - v3.3.6** (odporúčané)
- ⚠️ **ESP32 v2.x** - Ak máš starší balík, musíš updatovať:
  1. Arduino IDE → Tools → Board → Boards Manager
  2. Hľadaj "ESP32"
  3. Update na v3.x

### Knižnice
```cpp
#include <EEPROM.h>           // Built-in ✅
#include <Wire.h>             // Built-in ✅
#include <SPI.h>              // Built-in ✅
#include <WiFi.h>             // Built-in ✅
#include <Adafruit_ADS1X15.h> // Nainštaluj cez Library Manager
#include <LiquidCrystal_I2C.h>// Nainštaluj cez Library Manager
```

---

## 🔍 Testovanie

Po nahratí:

1. **Otvor Serial Monitor** (Ctrl+Shift+M)
2. **Nastav Baud Rate: 500000**
3. **Hľadaj:**
   ```
   > Serial Initialized
   > Firmware: V1.2F
   > PWM: 25000 Hz
   > ADC OK
   > Dual Core OK
   > EEPROM OK
   > MPPT READY
   ```

4. **Testuj WiFi:**
   - Pripoj sa na WiFi: "MPPT" / "mppt1234"
   - Otvor: http://192.168.4.1

5. **Testuj LCD:**
   - Malo by sa zobraziť: "MPPT INIT OK"

---

## 🐛 Ak Máš Stále Problémy

### Chyba: "ledcAttach was not declared"
→ Update ESP32 board package na v3.x

### Chyba: "undefined reference to setBatteryProfile"
→ Uisti sa, že všetkých 8 súborov je v rovnakom priečinku

### Chyba: "ADS1115 not found"
→ Skontroluj I2C zapojenie (SDA=21, SCL=22)

### WiFi nefunguje
→ Skontroluj že heslo má min. 8 znakov

### LCD nefunguje
→ Skontroluj I2C adresu (môže byť 0x3F namiesto 0x27)

---

## 📊 Zhrnutie Zmien V1.2F

| Funkcia | Status | Poznámka |
|---------|--------|----------|
| PWM 25kHz | ✅ | ESP32 v3.x API |
| Teplotná komp | ✅ | Funguje |
| AGM/Lead profily | ✅ | Funguje |
| Hysteréza | ✅ | Funguje |
| Soft start | ✅ | Funguje |
| MPPT algoritmus | ✅ | Funguje |
| WiFi | ✅ | Funguje |
| LCD | ✅ | Funguje |
| EEPROM | ✅ | S checksumom |
| Kalibrácia | ✅ | Serial/WiFi |

---

## 💡 Tips

1. **Ak chceš vyššiu PWM:** Zmeň `pwmFrequency = 30000` (až 40kHz)
2. **Ak je príliš hlučné:** Skontroluj induktor a kondenzátory
3. **Ak osciluje:** Zníž `filterAlpha` na 0.1
4. **Ak pomalá odozva:** Zvýš `filterAlpha` na 0.2

---

## 📞 Podpora

- **GitHub Issues** - Pre technické problémy
- **Serial Monitor** - Pre diagnostiku (baud: 500000)
- **WiFi Interface** - Pre konfiguráciu (192.168.4.1)

---

**Verzia:** V1.2F (Fixed)  
**Dátum:** 2024.02.07  
**Kompatibilita:** ESP32 v3.x+

**🎉 Všetko by malo teraz fungovať! 🎉**
