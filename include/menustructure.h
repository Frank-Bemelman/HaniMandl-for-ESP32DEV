
Menu BigMenu[] = 
    { 
        "1 - Set Jar Type Tarras",     
        "Set Your Tarra Values For Jar Types Used",     
        "List?", 10, 1000, 250, 1, JARARRAY, SET_TARRA,
        "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA,
        "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
        "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
        "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
        "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
        "Exit", 1,   // exit mode & columns

            
       "2 - Calibrate Scale",
       "Use This For Calibration Of The Scale With A Known Weight",
        "", 0, 100, 50, 1, NOT_USED, SET_INTEGER,
        "", 0, 100, 50, 1, NOT_USED, SET_INTEGER,
        "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
        "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
        "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
        "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
        "Bottom", 0,   // exit mode & columns

      
       "3 - Net Weight Products",
       "Adjust The Net Weight & Jar Type For Various Products",
        "netto & jartype", 0, 4500, 100, 1, JARARRAY, SET_JAR_PRESET, // also deals with JAR1_TYPE
        "netto & jartype", 0, 4500, 250, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR2_TYPE
        "netto & jartype", 0, 4500, 250, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR3_TYPE 
        "netto & jartype", 0, 4500, 500, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR4_TYPE
        "netto & jartype", 0, 4500, 500, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR5_TYPE 
        "netto & jartype", 0, 4500, 500, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR6_TYPE
        "Exit", 2,   // exit mode & columns

        "4 - Automatic Settings",
        "Set The Parameters For Automatic Filling",
        "Full Auto Start", 0, 1, 1, 0, AUTOSTART,  SET_ON_OFF,
        "Jar Tolerance", 0, 100, 20, 0, AUTO_JAR_TOLERANCE, SET_GRAM_TOLERANCE, 
        "Correction", -100, 0, -20, 0, CORRECTION, SET_GRAMS,
        "Auto Correction", 0, 1, 0, 0, AUTO_CORRECTION, SET_ON_OFF,
        "Coulance", 0, 100, 7, 0, COULANCE, SET_GRAMS,
        "Slow Down Filling At ", 25, 75, 50, 0, SLOWDOWNPERCENT, SET_PERCENT,
        "Exit", 1,   // exit mode & columns

        "5 - Servo Adjustment",
        "Sets The Limits For Your Servo Valve",
        "Live Setup", 0, 1, 1, 1, LIVESETUP, SET_ON_OFF,
        "Minimum", 0, 30, 5, 1, SERVOMIN, SET_DEGREES,
        "Maximum", 60, 180, 120, 1, SERVOMAXDOS, SET_DEGREES,
        "Fine Dosage", 10, 150, 75, 1, SERVOFINEDOS, SET_DEGREES,
        "DC Max Current", 50, 1000, 500, 1, SERVOMAXCURRENT, SET_MILLIAMPSMAX,
        "", 0, 100, 50, 1, NOT_USED, SET_DEGREES,
        "Exit", 1,   // exit mode & columns

        "6 - System Parameters",
        "Sets The General Parameters And Options",
        "Buzzer", 0, 1, 1, 0, BUZZER, SET_ON_OFF,
        "Led Indicator", 0, 1, 1, 0, LED, SET_ON_OFF,
        "Default Product", 0, 5, 0, 0, CHOSENPRODUCT, SET_CHOSEN,
        "", 0, 100, 50, 0, NOT_USED, SET_INTEGER,
        "", 0, 100, 50, 0, NOT_USED, SET_INTEGER,
        "", 0, 100, 50, 0, NOT_USED, SET_INTEGER,
        "Exit", 1,   // exit mode & columns
       
        "7 - Reset Counters",
        "Check And Reset Counters For Various Jars Filled",
        "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        "Exit", 1,
       
        "8 - Counter Presets",
        "Set The Number Of Jars To Fill",
        "A", 0, 100, 10, 1, JARARRAY, SET_TRIPCOUNT,
        "A", 0, 100, 10, 1, JARARRAY, SET_TRIPCOUNT,
        "A", 0, 100, 10, 1, JARARRAY, SET_TRIPCOUNT,
        "A", 0, 100, 10, 1, JARARRAY, SET_TRIPCOUNT,
        "A", 0, 100, 10, 1, JARARRAY, SET_TRIPCOUNT,
        "A", 0, 100, 10, 1, JARARRAY, SET_TRIPCOUNT,
        "Exit", 1,   // exit mode & columns
        
        "9 - Default Reset",
        "Resets All Parameters To Standard Values",
        "Factory Reset", 0, 0, 0, 1, RESETPREFS, RESETPREFS,
        "Format Eeprom", 0, 0, 0, 1, RESETEEPROM, RESETEEPROM,
        "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
        "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
        "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
        "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
        "Exit", 1,   // exit mode & columns

        "10 - Set Language",
        "Choose One Of The Supported Languages",
        "ENGLISH", 0, 0, 0, 1, LANGUAGE, SET_LANGUAGE,
        "GERMAN",  1, 1, 1, 1, LANGUAGE, SET_LANGUAGE,
        "DUTCH",   2, 2, 2, 1, LANGUAGE, SET_LANGUAGE,
        "FRENCH",  3, 3, 3, 1, LANGUAGE, SET_LANGUAGE,
        "SPANISH", 4, 4, 4, 1, LANGUAGE, SET_LANGUAGE,
        "ITALIAN", 5, 5, 5, 1, LANGUAGE, SET_LANGUAGE,
        "Exit", 1   // exit mode & columns
};



/* setupTara();              
   setupCalibration();       // Kalibrieren 
   setupFuellmenge();        // Füllmenge 
   setupAutomatik();         // Autostart/Autokorrektur konfigurieren 
   setupServoWinkel();       // Servostellungen Minimum, Maximum und Feindosierung
   setupParameter();         // Sonstige Einstellungen
   setupCounter();           // Zählwerk
   setupTripCounter();       // Zählwerk Trip
   setupINA219();            // INA219 Setup
   setupClearPrefs();  
*/   


// English - German - Dutch - French - Spanish - Italian
Trans Words[] = 
{ LNG_SET_TARA_VAL,    "Set Tarra Values", "Tara Werte",        "Stel Tarra's In",       "Définir Les Tares", "", "",
  LNG_CALIBRATE_SCALE, "Calibrate Scale",  "Waage Kalibrieren", "Weegschaal Calibreren", "Calibrer La Balance", "", "",
  LNG_WEIGTH_PRESET, "Net Weight Presets",  "Fuell Menge", "Netto Vulgewichten",         "Poids De Remplissage", "", "",
  LNG_SAVE_AND_EXIT,   "Save & Exit",      "Speichern",         "Opslaan",               "Sauvegarder", "", ""
};
