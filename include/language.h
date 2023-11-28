// language stuff, lot todo

#define LNG_FIRST 0
#define LNG_LANGUAGE 0
#define LNG_ENGLISH 1
#define LNG_GERMAN 2
#define LNG_DUTCH 3
#define LNG_FRENCH 4
#define LNG_SPANISH 5
#define LNG_ITALIAN 6
#define LNG_ON 7
#define LNG_OFF 8
#define LNG_EXIT 9

// menu's
#define LNG_SET_JAR_TYPE_TARRAS 10 
#define LNG_CALIBRATE_SCALE 20
#define LNG_NET_WEIGHT_PRODUCTS 30
#define LNG_AUTOMATIC_SETTINGS 40
#define LNG_SERVO_ADJUSTMENT 50
#define LNG_SYSTEM_PARAMETERS 60
#define LNG_RESET_COUNTERS 70
#define LNG_DEFAULTS_RESET 80

#define LNG_SET_JAR_TYPE_TARRAS_LD 90
#define LNG_CALIBRATE_SCALE_LD 91
#define LNG_NET_WEIGHT_PRODUCTS_LD 92
#define LNG_AUTOMATIC_SETTINGS_LD 93
#define LNG_SERVO_ADJUSTMENT_LD 94
#define LNG_SYSTEM_PARAMETERS_LD 95
#define LNG_RESET_COUNTERS_LD 96
#define LNG_DEFAULTS_RESET_LD 97
 

#define LNG_YES 100
#define LNG_NO 101
#define LNG_SAVED 102
#define LNG_THANKS 103

#define LNG_INA219_INIT 104
#define LNG_INA219_MISSING 105
#define LNG_SCALE_NOTCALIBRATED 106
#define LNG_SCALE_INIT 107
#define LNG_SCALE_MISSING 107
#define LNG_SCALE_TARRED 108
#define LNG_ANY_BUTTON_TO_SKIP 109
#define LNG_PLEASE_EMPTY_SCALE 110
#define LNG_GRAM 111
#define LNG_TARRA 112
#define LNG_JAR 113
#define LNG_NETWEIGHT 114
#define LNG_CONTINUE_WHEN_DONE 115
#define LNG_PLACE_KNOWN_WEIGHT 116
#define LNG_CALIBRATE_AS 117



#define LNG_LAST 150

struct Translation
{ int  index; // a define value such as LNG_SET_TARA_VAL
  char word[6][80]; // 6 translations of one text
};

#ifndef TRANS


Translation Trans[] = 
{ //                    English      German            Nederlands    Frans           Spaans        Italiaans
  LNG_LANGUAGE,        "Language",  "Sprache",        "Taal",       "Langue",        "Idioma",    "Lingua",
  LNG_ENGLISH,         "Englisch",  "Engels",         "Engels",     "Anglais",       "Ingles",    "Inglese", 
  LNG_GERMAN,          "German",    "Deutch",         "Duits",      "Allemand",      "Aleman",    "Tedesca", 
  LNG_DUTCH,           "Dutch",     "Niederlandisch", "Nederlands", "Néerlandais",   "Holandés",  "Olandese", 
  LNG_FRENCH,          "French",    "Französisch",    "Frans",      "Francais",      "Francés",   "Francese", 
  LNG_SPANISH,         "Spanish",   "Spanisch",       "Spaans",     "Espagnol",      "Española",  "Spagnola", 
  LNG_ITALIAN,         "Italian",   "Italiener",      "Italiaans",  "Italien",       "Italiano",  "Italiana", 
  LNG_ON,              "On",        "Ein",            "Aan",        "Allumee",       "Encendida", "Accesa",
  LNG_OFF,             "Off",       "Aus",            "Uit",        "Desactive",     "Apagada",   "Spenta",
  LNG_YES,              "Yes",      "Ja",             "Ja",         "Oui",           "Si",        "Si",
  LNG_NO,               "No",       "Nein",           "Nee",        "Non",           "No",        "No",
  LNG_EXIT,             "Exit",     "Fertig",        "Klaar",     "Sortie",        "Salida",     "Uscita",

  LNG_SET_JAR_TYPE_TARRAS, "Set Jar Type Tarras", "Tarra Fur Gläser",    "Glaswerk Tarra's",        "???", "???", "???", 
  LNG_CALIBRATE_SCALE,     "Calibrate Scale",     "Waage Kalibrieren",   "Weegschaal Calibreren",   "???", "???", "???", 
  LNG_NET_WEIGHT_PRODUCTS, "Net Weight Products", "Fullmenge",           "Netto Gewicht Producten", "???", "???", "???", 
  LNG_AUTOMATIC_SETTINGS,  "Automatic Settings",  "Automatik",           "Automatisch Bedrijf",     "???", "???", "???", 
  LNG_SERVO_ADJUSTMENT,    "Servo Adjustment",    "Servo Einstellung",   "Servo Instellingen",      "???", "???", "???", 
  LNG_SYSTEM_PARAMETERS,   "System Parameters",   "System Parameter",    "Systeem Parameters",      "???", "???", "???", 
  LNG_RESET_COUNTERS,      "Reset Counters",      "Zähler Zurucksetzen", "Tellers Resetten",        "???", "???", "???", 
  LNG_DEFAULTS_RESET,      "Reset Defaults",      "Clear Prefs",         "Reset Instellingen",             "???", "???", "???", 
 
  LNG_SET_JAR_TYPE_TARRAS_LD, "Set Your Tarra Values For Jar Types Used", 
                              "Setze Die Tara Werte Fur Ihre Gläser",    
                              "Stel De Tarra Waarden In Voor Al Uw Types Potten",
                              "???", "???", "???", 
  LNG_CALIBRATE_SCALE_LD,     "Use This For Calibration Of The Scale With A Known Weight",     
                              "Hier Können Sie Ihre Waage Kalibrieren Mit Ein Bekantes Gewicht",   
                              "Hier Kunt U Uw Weegschaal Calibreren Met Een IJKgewicht",   
                              "???", "???", "???", 
  LNG_NET_WEIGHT_PRODUCTS_LD, "Adjust The Net Weight & Jar Type For Various Products", 
                              "Justiere Die Fullmenge Und Glas Typ Für Ihre Produkten",           
                              "Stel Netto Gewicht En Type Pot Voor Uw Producten In", 
                              "???", "???", "???", 
  LNG_AUTOMATIC_SETTINGS_LD,  "Set The Parameters For Automatic Filling",  
                              "Parameter Fur Automatik Betrieb Andern",
                              "Parameters Wijzigen Voor Automatisch Bedrijf",     
                              "???", "???", "???", 
  LNG_SERVO_ADJUSTMENT_LD,    "Sets The Limits For Your Servo Valve",    
                              "Einstellung Von Servo Limieten Fur Minimum Und Maximum Winkel",   
                              "Servo Limiet Instellingen Voor Minimaal En Maximaal",      
                              "???", "???", "???", 
  LNG_SYSTEM_PARAMETERS_LD,   "Sets The General Parameters And Options",   
                              "Algemeine System Parameter Und Optionale Einstellungen",
                              "Algemene Systeem Parameter En Optionele Instellingen",      
                              "???", "???", "???", 
  LNG_RESET_COUNTERS_LD,      "Check And Reset Counters For Various Jars Filled",      
                              "Zähler Fur Fertige Produkten Setzen Order Zurucksetzen", 
                              "Tellers Voor Gereed Produkten Aanpassen Of Resetten",        
                              "???", "???", "???", 
  LNG_DEFAULTS_RESET_LD,      "Resets All Parameters To Standard Values",      
                              "Alle Parameter Zuricksetzen Nach Original Werte",         
                              "Reset Alle Parameters Naar Standaard Waarden",
                               "???", "???", "???", 
 


  LNG_SAVED,               "Saved!",              "Gespeichert!",        "Opgeslagen!",             "Enregistré!",     "Salvado!", "Salvato!", 
  LNG_THANKS,              "Thank You!",          "Danke!",              "Dankjewel!",              "Merci Beaucoup!", "Gracias!", "Grazie!", 

  LNG_INA219_INIT,         "INA219 Initialized!",   "INA219 chip gefunden!",       "INA219 Stroomsensor OK!", "?", "?", "?", 
  LNG_INA219_MISSING,      "INA219 Not Installed!", "INA219 chip nicht gefunden!", "INA219 Niet Aanwezig!", "?", "?", "?", 
  LNG_SCALE_NOTCALIBRATED, "Scale Not Calibrated!", "Waage Nicht Kalibriert!",     "Weegschaal Niet Gecalibreerd!", "?", "?", "?", 
  LNG_SCALE_INIT,          "Scale Initialized!",    "Waage Initialisiert!",        "Weegschaal Geïnitialiseerd!", "?", "?", "?", 
  LNG_SCALE_MISSING,       "Scale Not Connected!",  "Keine Waage!",                "Weegschaal Ontbreekt!", "?", "?", "?", 
  LNG_SCALE_TARRED,        "Scale Tared",           "Waage Tarriert",              "Weegschaal Getarreerd", "?", "?", "?", 
  LNG_ANY_BUTTON_TO_SKIP,  "Any Button To Skip",    "Jeder Taste Geht Weiter",     "Tik Een Toets", "?", "?", "?", 
  LNG_PLEASE_EMPTY_SCALE,  "Please Empty Scale!",   "Bitte Waage Leeren!",         "Weegschaal Leeg Maken!", "?", "?", "?", 
  LNG_GRAM,                "Gram",                  "Gramm",                       "Gram",               "Gramme",  "Gramo", "Grammo",
  LNG_TARRA,               "Tarra",                 "Tara",                        "Tarra",              "Tarra", "Tarra", "Tarra", 
  LNG_JAR,                 "Jar",                   "Glas",                        "Pot",                "Pot", "Frasco", "Vaso", 
  LNG_NETWEIGHT,           "Net Weight",            "Nettogewicht",                "Netto Gewicht",      "Poids Net", "Peso Neto", "Peso Netto",
  LNG_CONTINUE_WHEN_DONE,  "Continue When Done",    "Weiter Gehen",                "Volgende Stap",      "?", "?", "?", 
  LNG_PLACE_KNOWN_WEIGHT,  "Place A Known Weight",  "Kalibriergewicht Aufstellen", "Plaats IJkgewicht",  "?", "?", "?", 
  LNG_CALIBRATE_AS,        "Calibrate As",         "Kalibrieren Als",             "Calibreren Als",     "?", "?", "?", 

  LNG_LAST,   "?", "?", "?", "?", "?", "?", 

};


#else
extern Translation Trans[];
#endif


// English - German - Dutch - French - Spanish - Italian
//Trans Words[] = 
//{ LNG_SET_TARA_VAL,    "Set Tarra Values", "Tara Werte",        "Stel Tarra's In",       "Définir Les Tares", "", "",
//  LNG_CALIBRATE_SCALE, "Calibrate Scale",  "Waage Kalibrieren", "Weegschaal Calibreren", "Calibrer La Balance", "", "",
//  LNG_WEIGTH_PRESET, "Net Weight Presets",  "Fuell Menge", "Netto Vulgewichten",         "Poids De Remplissage", "", "",
//  LNG_SAVE_AND_EXIT,   "Save & Exit",      "Speichern",         "Opslaan",               "Sauvegarder", "", ""
//};