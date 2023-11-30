#ifndef TRANS

Menu BigMenu[] = 
    {   LNG_SET_JAR_TYPE_TARRAS,
        // "1 - Set Jar Type Tarras",     
        LNG_SET_JAR_TYPE_TARRAS_LD,
        //"Set Your Tarra Values For Jar Types Used",     
        LNG_ALGORITHM, "List?", 10, 1000, 250, 1, JARARRAY, SET_TARRA,
        LNG_ALGORITHM, "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA,
        LNG_ALGORITHM, "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
        LNG_ALGORITHM, "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
        LNG_ALGORITHM, "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
        LNG_ALGORITHM, "List?", 10, 1000, 250, 0, JARARRAY, SET_TARRA, 
//        "Exit", 
        1,   // columns

       LNG_CALIBRATE_SCALE,
       // "2 - Calibrate Scale",
       LNG_CALIBRATE_SCALE_LD,
       // "Use This For Calibration Of The Scale With A Known Weight",
       LNG_ALGORITHM,  "", 0, 100, 50, 1, NOT_USED, SET_INTEGER,
       LNG_ALGORITHM, "", 0, 100, 50, 1, NOT_USED, SET_INTEGER,
       LNG_ALGORITHM, "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
       LNG_ALGORITHM,  "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
       LNG_ALGORITHM,  "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
       LNG_ALGORITHM,  "", 0, 100, 50, 1, NOT_USED, SET_INTEGER, 
//        "", 
        0,   // columns

      
       LNG_NET_WEIGHT_PRODUCTS,
       // "3 - Net Weight Products",
       LNG_NET_WEIGHT_PRODUCTS_LD,
       // "Adjust The Net Weight & Jar Type For Various Products",
        LNG_ALGORITHM, "netto & jartype", 0, 4500, 100, 1, JARARRAY, SET_JAR_PRESET, // also deals with JAR1_TYPE
        LNG_ALGORITHM, "netto & jartype", 0, 4500, 250, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR2_TYPE
        LNG_ALGORITHM, "netto & jartype", 0, 4500, 250, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR3_TYPE 
        LNG_ALGORITHM, "netto & jartype", 0, 4500, 500, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR4_TYPE
        LNG_ALGORITHM, "netto & jartype", 0, 4500, 500, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR5_TYPE 
        LNG_ALGORITHM, "netto & jartype", 0, 4500, 500, 0, JARARRAY, SET_JAR_PRESET, // also deals with JAR6_TYPE
//        "Exit", 
        2,   // columns

        LNG_AUTOMATIC_SETTINGS,
        // "4 - Automatic Settings",
        LNG_AUTOMATIC_SETTINGS_LD,
        // "Set The Parameters For Automatic Filling",
        LNG_AUTO_START, "Filling Auto Start", 0, 1, 1, 0, AUTOSTART,  SET_ON_OFF,
        LNG_JAR_TOLERANCE, "Jar Tolerance", 0, 100, 20, 0, AUTO_JAR_TOLERANCE, SET_GRAM_TOLERANCE, 
        LNG_CORRECTION, "Correction", -100, 0, -20, 0, CORRECTION, SET_GRAMS,
        LNG_AUTO_CORRECTION, "Auto Correction", 0, 1, 0, 0, AUTO_CORRECTION, SET_ON_OFF,
        LNG_AUTO_COULANCE, "Coulance", 0, 100, 7, 0, COULANCE, SET_GRAMS,
        LNG_SLOW_AT_PERCENT, "Slow Down Filling At", 25, 75, 50, 0, SLOWDOWNPERCENT, SET_PERCENT,
//        "Exit", 
         1,   // columns

        LNG_SERVO_ADJUSTMENT,
        //"5 - Servo Adjustment",
        LNG_SERVO_ADJUSTMENT_LD,
        //"Sets The Limits For Your Servo Valve",
        LNG_SERVO_LIVE_SETUP, "Live Setup", 0, 1, 1, 1, LIVESETUP, SET_ON_OFF,
        LNG_SERVO_MINIMUM, "Minimum", 0, 30, 5, 1, SERVOMIN, SET_DEGREES,
        LNG_SERVO_MAXIMUM, "Maximum", 60, 180, 120, 1, SERVOFASTDOS, SET_DEGREES,
        LNG_SERVO_SLOW_DOSAGE, "Slow Dosage", 10, 150, 75, 1, SERVOSLOWDOS, SET_DEGREES,
        LNG_SERVO_MAX_CURRENT, "DC Max Current", 50, 1000, 500, 1, SERVOMAXCURRENT, SET_MILLIAMPSMAX,
        LNG_NOT_USED, "", 0, 100, 50, 1, NOT_USED, SET_DEGREES,
//        "Exit", 
        1,   // columns

        LNG_SYSTEM_PARAMETERS,
        //"6 - System Parameters",
        LNG_SYSTEM_PARAMETERS_LD,
        //"Sets The General Parameters And Options",
        LNG_BUZZER, "Buzzer", 0, 1, 1, 0, BUZZER, SET_ON_OFF,
        LNG_LED_INDICATOR, "Led Indicator", 0, 1, 1, 0, LED, SET_ON_OFF,
        LNG_DEFAULT_PRODUCT, "Default Product", 0, 5, 0, 0, CHOSENPRODUCT, SET_CHOSEN,
        LNG_LANGUAGE, "Language", 0, 5, 0, 0, LANGUAGE, SET_LANGUAGE,
        LNG_NOT_USED, "", 0, 100, 50, 0, NOT_USED, SET_INTEGER,
        LNG_NOT_USED, "", 0, 100, 50, 0, NOT_USED, SET_INTEGER,
//        "Exit", 
        1,   //  columns
       
        LNG_RESET_COUNTERS,
        //"7 - Reset Counters",
        LNG_RESET_COUNTERS_LD,
        // "Check And Reset Counters For Various Jars Filled",
        LNG_ALGORITHM, "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        LNG_ALGORITHM, "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        LNG_ALGORITHM, "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        LNG_ALGORITHM, "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        LNG_ALGORITHM, "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
        LNG_ALGORITHM, "List?", 0, 100, 0, 1, JARARRAY, SET_TO_ZERO,
//        "Exit", 
        1,
       
        LNG_DEFAULTS_RESET,
        //"8 - Default Reset",
        LNG_DEFAULTS_RESET_LD,
        //"Resets All Parameters To Standard Values",
        LNG_FACTORY_RESET, "Factory Reset", 0, 0, 0, 1, RESETPREFS, RESETPREFS,
        LNG_EEPROM_FORMAT, "Format Eeprom", 0, 0, 0, 1, RESETEEPROM, RESETEEPROM,
        LNG_NOT_USED, "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
        LNG_NOT_USED, "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
        LNG_NOT_USED, "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
        LNG_NOT_USED, "", 0, 100, 50, 1, NOT_USED, SET_YES_NO,
//        "Exit", 
        1   // columns

        
};

#else
extern Menu BigMenu[];
#endif
