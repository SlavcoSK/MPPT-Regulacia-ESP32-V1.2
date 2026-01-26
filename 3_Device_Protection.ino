// ---------------------------------------------------------
// RIADENIE SPÄTNÉHO TOKU PRÚDU (ochrana proti spätnému prúdu)
// ---------------------------------------------------------
void backflowControl(){                                                 // Funkcia riadi MOSFET proti spätnému toku prúdu z batérie do panelu

  if(output_Mode == 0){                                                  // PSU režim (napájací zdroj)
    bypassEnable = 1;                                                    // V PSU režime je MOSFET vždy zapnutý
  }
  else{                                                                  // Nabíjací režim (CHARGER)
    if(voltageInput > voltageOutput + voltageDropout){                  // Ak je vstupné napätie vyššie než výstupné + rezerva
      bypassEnable = 1;                                                  // Povoliť tok energie zo soláru
    }
    else{
      bypassEnable = 0;                                                  // Zakázať MOSFET – ochrana proti spätnému toku (batéria → panel)
    }
  }

  digitalWrite(backflow_MOSFET, bypassEnable);                           // Nastavenie GPIO pinu MOSFET-u podľa výpočtu
}

// ---------------------------------------------------------
// HLAVNÁ FUNKCIA OCHRÁN ZARIADENIA
// ---------------------------------------------------------
void Device_Protection(){

  // -------- RESET POČÍTADLA CHÝB V ČASE --------
  currentRoutineMillis = millis();                                       // Aktuálny čas v milisekundách

  if(currentErrorMillis - prevErrorMillis >= errorTimeLimit){            // Ak uplynul definovaný časový interval
    prevErrorMillis = currentErrorMillis;                                 // Uloženie času poslednej kontroly

    if(errorCount < errorCountLimit){                                     // Ak počet chýb neprekročil limit
      errorCount = 0;                                                     // Vynuluj počítadlo chýb
    }
    else{
      // TODO: tu je priestor na uspatie systému alebo pauzu nabíjania
    }
  }

  // -------- DETEKCIA PORÚCH --------
  ERR = 0;                                                               // Lokálny počet chýb v tomto cykle

  backflowControl();                                                      // Spustenie ochrany proti spätnému prúdu

  // --- TEPLOTNÁ OCHRANA ---
  if(temperature > temperatureMax){                                      // Ak teplota prekročí maximum
    OTE = 1;                                                             // Nastav príznak prehriatia
    ERR++;                                                               // Zvýš lokálny počet chýb
    errorCount++;                                                        // Zvýš globálny počet chýb
  }
  else{ OTE = 0; }                                                       // Inak je teplota v poriadku

  // --- VSTUPNÝ NADPRÚD ---
  if(currentInput > currentInAbsolute){                                  // Ak vstupný prúd prekročí absolútny limit
    IOC = 1; ERR++; errorCount++;                                        // Nastav chybu nadprúdu
  }
  else{ IOC = 0; }

  // --- VÝSTUPNÝ NADPRÚD ---
  if(currentOutput > currentOutAbsolute){                                // Ak výstupný prúd prekročí limit
    OOC = 1; ERR++; errorCount++;                                        // Aktivuj ochranu
  }
  else{ OOC = 0; }

  // --- PREPÄTIE NA BATÉRII ---
  if(voltageOutput > voltageBatteryMax + voltageBatteryThresh){          // Ak je napätie batérie príliš vysoké
    OOV = 1; ERR++; errorCount++;                                        // Prepäťová ochrana
  }
  else{ OOV = 0; }

  // --- FATÁLNE NÍZKE NAPÄTIE ---
  if(voltageInput < vInSystemMin && voltageOutput < vInSystemMin){       // Ak vstup aj výstup sú pod minimom
    FLV = 1; ERR++; errorCount++;                                        // Kritický stav – systém nemôže fungovať
  }
  else{ FLV = 0; }

  // -------- OCHRANY ŠPECIFICKÉ PRE REŽIM --------
  if(output_Mode == 0){                                                  // PSU MODE
    REC = 0;                                                             // Reset príznaku obnovy
    BNC = 0;                                                             // Batéria nie je relevantná v PSU režime

    if(voltageInput < voltageBatteryMax + voltageDropout){               // Ak vstupné napätie klesne pod úroveň batérie
      IUV = 1; ERR++; errorCount++;                                      // Podpäťová ochrana vstupu
    }
    else{ IUV = 0; }
  }
  else{                                                                  // CHARGER MODE
    backflowControl();                                                    // Opätovná kontrola spätného toku

    if(voltageOutput < vInSystemMin){                                    // Ak nie je detekované napätie batérie
      BNC = 1; ERR++;                                                    // Batéria nie je pripojená
    }
    else{ BNC = 0; }

    if(voltageInput < voltageBatteryMax + voltageDropout){               // Ak solár nedokáže nabiť batériu
      IUV = 1;                                                           // Podpäťová ochrana
      ERR++;
      REC = 1;                                                           // Povolenie obnovy po zlepšení podmienok
    }
    else{ IUV = 0; }
  }
}
