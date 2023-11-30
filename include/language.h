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
#define LNG_SET_JAR_TYPE_TARRAS_LD 11
#define LNG_CALIBRATE_SCALE 20
#define LNG_CALIBRATE_SCALE_LD 21
#define LNG_NET_WEIGHT_PRODUCTS 30
#define LNG_NET_WEIGHT_PRODUCTS_LD 31
#define LNG_AUTOMATIC_SETTINGS 40
#define LNG_AUTOMATIC_SETTINGS_LD 41
#define LNG_AUTO_START 42
#define LNG_JAR_TOLERANCE 43
#define LNG_CORRECTION 44
#define LNG_AUTO_CORRECTION 45
#define LNG_AUTO_COULANCE 46
#define LNG_SLOW_AT_PERCENT 47
#define LNG_SERVO_ADJUSTMENT 50
#define LNG_SERVO_ADJUSTMENT_LD 51
#define LNG_SERVO_LIVE_SETUP 52
#define LNG_SERVO_MINIMUM 53
#define LNG_SERVO_MAXIMUM 54
#define LNG_SERVO_SLOW_DOSAGE 55
#define LNG_SERVO_MAX_CURRENT 56
#define LNG_SYSTEM_PARAMETERS 60
#define LNG_SYSTEM_PARAMETERS_LD 61
#define LNG_BUZZER 62
#define LNG_LED_INDICATOR 63
#define LNG_DEFAULT_PRODUCT 64
#define LNG_RESET_COUNTERS 70
#define LNG_RESET_COUNTERS_LD 71
#define LNG_DEFAULTS_RESET 80
#define LNG_DEFAULTS_RESET_LD 81
#define LNG_FACTORY_RESET 82
#define LNG_EEPROM_FORMAT 83
 
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

#define LNG_SEMI_AUTO_STOPPED 118
#define LNG_SEMI_AUTO_PAUSED  119
#define LNG_SEMI_AUTO_FILLING 120
#define LNG_FULL_AUTO_STOPPED 121
#define LNG_FULL_AUTO_PAUSED  122
#define LNG_FULL_AUTO_FILLING 123

#define LNG_BAD_TARRA_VALUE 124
#define LNG_PLEASE_SET_TARRA_VALUE 125

#define LNG_PLACE_EMPTY_JAR 126
#define LNG_JAR_IS_PLACED 127
#define LNG_PRESS_START_TO_FILL 128
#define LNG_PRESS_START_TO_RESUME 129
#define LNG_WILL_START_FILLING 130
#define LNG_WILL_RESUME_FILLING 131
#define LNG_FAST_FILLING 132
#define LNG_SLOW_FILLING 133
#define LNG_PAUSED_FILL 134
#define LNG_JAR_IS_READY 135






#define LNG_NOT_USED 148
#define LNG_ALGORITHM 149 // no direct translation, swapped in software
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

  LNG_SET_JAR_TYPE_TARRAS, "Set Jar Type Tarras", "Tarra Fur Gläser",    "Glaswerk Tarra's",        "Ensemble De Pots Tarras", "Conjunto Tarro Tipo Frasco", "Impostare Il Tipo Di Vaso Tarras", 
  LNG_CALIBRATE_SCALE,     "Calibrate Scale",     "Waage Kalibrieren",   "Weegschaal Calibreren",   "Calibrer La Balance",     "Calibrar Báscula",           "Calibrare La Bilancia", 
  LNG_NET_WEIGHT_PRODUCTS, "Net Weight Products", "Fullmenge",           "Netto Gewicht Producten", "Produits de poids net",   "Productos Peso Neto",        "Prodotti Peso Netto", 
  LNG_AUTOMATIC_SETTINGS,  "Automatic Settings",  "Automatik",           "Automatisch Bedrijf",     "Paramètres Automatiques", "Configuración Automática",   "Impostazioni Automatiche", 
  LNG_SERVO_ADJUSTMENT,    "Servo Adjustment",    "Servo Einstellung",   "Servo Instellingen",      "Réglage Des Servos",      "Ajuste Del Servo",           "Regolazione Del Servo", 
  LNG_SYSTEM_PARAMETERS,   "System Parameters",   "System Parameter",    "Systeem Parameters",      "Paramètres Système",      "Parámetros Sistema",         "Parametri Sistema", 
  LNG_RESET_COUNTERS,      "Reset Counters",      "Zähler Zurucksetzen", "Tellers Resetten",        "Réinitialiser Compteurs", "Restablecer Contadores",     "Reimposta Contatori", 
  LNG_DEFAULTS_RESET,      "Reset Defaults",      "Clear Prefs",         "Reset Instellingen",      "Réinitialise Défaut ",    "Restablecer Estándar",       "Reimposta Standard", 
 
  LNG_SET_JAR_TYPE_TARRAS_LD, "Set Your Tarra Values For Jar Types Used", 
                              "Setze Die Tara Werte Fur Ihre Gläser",    
                              "Stel De Tarra Waarden In Voor Al Uw Types Potten",
                              "Définissez Vos Valeurs Tarra Pour Les Types De Pots Utilisés", 
                              "Establezca Sus Valores De Tarra Para Los Tipos De Tarros Utilizados", 
                              "Imposta I Valori Tarra Per I Tipi Di Barattoli Utilizzati", 
  LNG_CALIBRATE_SCALE_LD,     "Use This For Calibration Of The Scale With A Known Weight",     
                              "Hier Können Sie Ihre Waage Kalibrieren Mit Ein Bekantes Gewicht",   
                              "Hier Kunt U De Weegschaal Calibreren Met Een IJkgewicht",   
                              "Utilisez-Le Pour L'Etalonnage De La Balance Avec Un Poids Connu", 
                              "Utilice Esto Para Calibrar La Báscula Con Un Peso Conocido", 
                              "Utilizzare Questo Per La Calibrazione Della Bilancia Con Un Peso Noto", 
  LNG_NET_WEIGHT_PRODUCTS_LD, "Adjust The Net Weight & Jar Type For Various Products", 
                              "Justiere Die Fullmenge Und Glas Typ Für Ihre Produkten",           
                              "Stel Netto Gewicht En Type Pot Voor Uw Producten In", 
                              "Ajustez Le Poids Net Et Le Type De Pot Pour Divers Produits", 
                              "Ajuste El Peso Neto Y El Tipo De Tarro Para Varios Productos", 
                              "Regola Il Peso Netto E Il Tipo Di Barattolo Per Vari Prodotti", 
  LNG_AUTOMATIC_SETTINGS_LD,  "Set The Parameters For Automatic Filling",  
                              "Parameter Fur Automatik Betrieb Andern",
                              "Parameters Wijzigen Voor Automatisch Bedrijf",     
                              "Définir Les Paramètres Pour Le Remplissage Automatique", 
                              "Establecer Los Parámetros Para El Llenado Automático", 
                              "Imposta I Parametri Per Il Riempimento Automatico", 
  LNG_SERVO_ADJUSTMENT_LD,    "Sets The Limits For Your Servo Valve",    
                              "Einstellung Von Servo Limieten Fur Minimum Und Maximum Winkel",   
                              "Servo Limiet Instellingen Voor Minimaal En Maximaal",      
                              "Définit Les Limites De Votre Servovalve", 
                              "Establece Los Límites Para Su Servoválvula", 
                              "Imposta I Limiti Per La Tua Servovalvola", 
  LNG_SYSTEM_PARAMETERS_LD,   "Sets The General Parameters And Options",   
                              "Algemeine System Parameter Und Optionale Einstellungen",
                              "Algemene Systeem Parameter En Optionele Instellingen",      
                              "Définit Les Paramètres Généraux Et Les Options", 
                              "Establece Los Parámetros Y Opciones Generales", 
                              "Imposta I Parametri E Le Opzioni Generali", 
  LNG_RESET_COUNTERS_LD,      "Set Or Reset Counters For Various Jars Filled",      
                              "Zähler Fur Fertige Produkten Setzen Oder Zurucksetzen", 
                              "Tellers Voor Gereed Produkten Aanpassen Of Resetten",        
                              "Définir Ou Réinitialiser Les Compteurs Pour Divers Pots Remplis", 
                              "Establecer O Restablecer Contadores Para Varios Frascos Llenos", 
                              "Imposta O Reimposta I Contatori Per Vari Vasetti Riempiti", 
  LNG_DEFAULTS_RESET_LD,      "Resets All Parameters To Standard Values",      
                              "Alle Parameter Zuricksetzen Nach Original Werte",         
                              "Reset Alle Parameters Naar Standaard Waarden",
                              "Réinitialise Tous Les Paramètres Aux Valeurs Standard", 
                              "Restablece Todos Los Parámetros A Los Valores Estándar", 
                              "Reimposta Tutti I Parametri Sui Valori Standard", 
 

  LNG_AUTO_START,        "Filling Auto Start",   "Automatik Start",     "Automatisch Starten",    "Démarrage Automatique",     "Autoencendido",            "Avvio automatico", 
  LNG_JAR_TOLERANCE,     "Jar Tolerance",        "Glastoleranz",        "Tolerantie Pot",         "Tolérance Pot",             "Tolerancia Frasco",        "Tolleranza Barattolo", 
  LNG_CORRECTION,        "Correction",           "Korrektur",           "Correctie",              "Correction",                "Corrección",               "Correzione", 
  LNG_AUTO_CORRECTION,   "Auto Correction",      "Auto Korrektur",      "Auto Correctie",         "Auto Correction",           "Auto Corrección",          "Auto Correzione", 
  LNG_AUTO_COULANCE,     "Coulance",             "Coulance",            "Coulance",               "Coulance",                  "Coulance",                 "Coulance", 
  LNG_SLOW_AT_PERCENT,   "Slow Down Filling At", "Feindosieren Bei",    "Langzaam Doseren Bij",   "Ralentissez Remplissage À", "Llenado Lento En",         "Riempimento Lento A", 
  LNG_SERVO_LIVE_SETUP,  "Live Setup",           "Live-Setup",          "Live Setup",             "Configuration Direct",      "Configuración Vivo",       "Configurazione Vivo", 
  LNG_SERVO_MINIMUM,     "Minimum",              "Minimum",             "Minimum",                "Minimum",                   "Mínimo",                   "Minimo", 
  LNG_SERVO_MAXIMUM,     "Maximum",              "Maximum",             "Maximum",                "Maximum",                   "Máximo",                   "Massimo", 
  LNG_SERVO_SLOW_DOSAGE, "Slow Dosage",          "Feindosierung",       "Langzaam Doseren",       "Dosage lent",               "Dosis Lenta",              "Dosaggio Lento", 
  LNG_SERVO_MAX_CURRENT, "DC Max Current",       "Maximale Strom",      "Maximale Stroom",        "Courant Maximum",           "Corriente Maxima",         "Corrente Massima", 
  LNG_BUZZER,            "Buzzer",               "Piepser",             "Zoemer",                 "Sondeur",                   "Sonda",                    "Sirena", 
  LNG_LED_INDICATOR,     "Led Indicator",        "LED-Anzeige",         "Led Indicator",          "Indicateur LED",            "Indicador LED",            "Indicatore LED", 
  LNG_DEFAULT_PRODUCT,   "Default Product",      "Standardprodukt",     "Standaard Produkt",      "Produit Par Défaut",        "Producto Standard",        "Prodotto Standard", 
  LNG_FACTORY_RESET,     "Factory Reset",        "Alles Zurück Setzten", "Fabrieks Instellingen", "Paramètres d'Usine",        "Restablecimiento Fábrica", "Impostazioni Fabbrica", 
  LNG_EEPROM_FORMAT,     "Format Eeprom",        "Formattieren Eeprom", "Formateer Eeprom",       "Formater l'Eeprom",         "Formatear EEPROM",         "Formato Eeprom", 

  LNG_SAVED,             "Saved!",               "Gespeichert!",         "Opgeslagen!",           "Enregistré!",     "Salvado!", "Salvato!", 
  LNG_THANKS,            "Thank You!",           "Danke!",               "Dankjewel!",            "Merci Beaucoup!", "Gracias!", "Grazie!", 

  LNG_INA219_INIT,         "INA219 Initialized!",   "INA219 chip gefunden!",       "INA219 Stroomsensor OK!",       "INA219 Initialisé",         "INA219 Inicializado",         "INA219 Inizializzato", 
  LNG_INA219_MISSING,      "INA219 Not Installed!", "INA219 chip nicht gefunden!", "INA219 Niet Aanwezig!",         "INA219 Pas Installé",       "INA219 No Instalado",         "INA210 Non Installato", 
  LNG_SCALE_NOTCALIBRATED, "Scale Not Calibrated!", "Waage Nicht Kalibriert!",     "Weegschaal Niet Gecalibreerd!", "Balance Non Calibrée",      "Báscula No Calibrada",        "Bilancia Non Calibrata", 
  LNG_SCALE_INIT,          "Scale Initialized!",    "Waage Initialisiert!",        "Weegschaal Geïnitialiseerd!",   "Balance Non Initialisé",    "Báscula No Inicializado",     "Bilancia Non Inizializzato", 
  LNG_SCALE_MISSING,       "Scale Not Connected!",  "Keine Waage!",                "Weegschaal Ontbreekt!",         "balance Non Connectée",     "Báscula No Conectada",        "Bilancia Non Collegata", 
  LNG_SCALE_TARRED,        "Scale Tared",           "Waage Tarriert",              "Weegschaal Getarreerd",         "Balance Tarée",             "Balanza Tarada",              "Bilancia Tarata", 
  LNG_ANY_BUTTON_TO_SKIP,  "Any Button To Skip",    "Jeder Taste Geht Weiter",     "Tik Een Toets",                 "Appuyez Bouton Pour Sauter", "Presione Botón Para Saltar", "Premere Pulsante Per Saltare", 
  LNG_PLEASE_EMPTY_SCALE,  "Please Empty Scale!",   "Bitte Waage Leeren!",         "Weegschaal Leeg Maken!",        "Claire Balance!",            "Clare Balanza!", "Chiare Bilancia!", 
  LNG_GRAM,                "Gram",                  "Gramm",                       "Gram",                          "Gramme",                     "Gramo", "Grammo",
  LNG_TARRA,               "Tarra",                 "Tara",                        "Tarra",                         "Tarra",                      "Tarra", "Tarra", 
  LNG_JAR,                 "Jar",                   "Glas",                        "Pot",                           "Pot",                        "Frasco", "Vaso", 
  LNG_NETWEIGHT,           "Net Weight",            "Nettogewicht",                "Netto Gewicht",                 "Poids Net",                  "Peso Neto", "Peso Netto",
  LNG_CONTINUE_WHEN_DONE,  "Continue When Done",    "Weiter Gehen",                "Volgende Stap",                 "Continuer",                  "Continuar", "Continua", 
  LNG_PLACE_KNOWN_WEIGHT,  "Place A Known Weight",  "Kalibriergewicht Aufstellen", "Plaats IJkgewicht",             "Placer Poids Connu",         "Coloque Peso Conocido", "Posiziona Peso Noto", 
  LNG_CALIBRATE_AS,        "Calibrate As",          "Kalibrieren Als",             "Calibreren Als",                "Calibrer Comme",             "Calibrar Como", "Calibra Come", 


  LNG_SEMI_AUTO_STOPPED,  "SEMI-AUTO STOPPED", "SEMI-AUTO GESTOPT", "SEMI-AUTO GESTOPT", "SEMI-AUTO ARRÊTÉ", "SEMI-AUTO INTERRUMPIDO", "SEMI-AUTO FERMATO", 
  LNG_SEMI_AUTO_PAUSED,   "SEMI-AUTO PAUSED",  "SEMI-AUTO PAUSE",   "SEMI-AUTO PAUZE",   "SEMI-AUTO PAUSE",  "SEMI-AUTO PAUSA ",       "SEMI-AUTO PAUSA", 
  LNG_SEMI_AUTO_FILLING,  "SEMI-AUTO FILLING", "SEMI-AUTO DOSIERT", "SEMI-AUTO DOSEERT", "SEMI-AUTO DOSAGE", "SEMI-AUTO DOSIFICAR",    "SEMI-AUTO DOSSAGGIO", 
  LNG_FULL_AUTO_STOPPED,  "AUTO STOPPED",      "AUTO GESTOPT",      "AUTO GESTOPT",      "AUTO ARRÊTÉ",      "AUTO INTERRUMPIDO",      "AUTO FERMATO", 
  LNG_FULL_AUTO_PAUSED,   "AUTO PAUSED",       "AUTO PAUSE",        "AUTO PAUZE",        "AUTO PAUSE",       "AUTO PAUSA",             "AUTO PAUSA", 
  LNG_FULL_AUTO_FILLING,  "AUTO FILLING",      "AUTO DOSIERT",      "AUTO DOSEERT",      "AUTO DOSAGE",      "AUTO DOSIFICAR",         "AUTO DOSSAGGIO", 

  LNG_BAD_TARRA_VALUE,        "Bad Tarra Value",                   "Falsche Tara Werte",          "Foute Tarra Waarde",       "Valeur Tare Erronée",        "Valor Tara Incorrecto",          "Valore Tara Errato", 
  LNG_PLEASE_SET_TARRA_VALUE, "Please Configure Valid Tarra For", "Bitte Tara Configurieren Für", "AUB Tarra Instellen Voor", "Veuillez Définir Tare Pour", "Por Favor Establezca Tara Para", "Prega Impostare Tara Per", 

  LNG_PLACE_EMPTY_JAR,       "Place Empty Jar %d",    "Stelle Leeres Glas %d Auf",    "Plaats Lege Pot %d",         "Placer Pot %d Vide",       "Coloque Frasco %d Vacío",       "Posiziona Barattolo %d Vuoto", 
  LNG_JAR_IS_PLACED,         "Jar Is Placed",         "Glas Ist Aufgestellt",         "Pot Is Geplaatst",           "Pot Est Placé",            "Se Coloca Frasco",              "Barattolo È Posizionato", 
  LNG_PRESS_START_TO_FILL,   "Press Start To Fill",   "Druck Start Zum Abfüllen",     "Druk Start Om Te Vullen",    "Démarrer Pour Remplir",    "Presione Iniciar Para Llenar",  "Premere Avvia Per Riempire", 
  LNG_PRESS_START_TO_RESUME, "Press Start To Resume", "Druck Start Fur Weiter",       "Druk Start Om Te Hervatten", "Démarrer Pour Reprendre",  "Presione Inicio Para Reanudar", "Premere Avvia Per Riprendere", 
  LNG_WILL_START_FILLING,    "Will Start Filling",    "Beginnt Mit Dem Füllen",       "Begint Met Vullen",          "Va Commencer À Remplir",   "Comenzará A Llenarse",          "Inizierà A Riempirsi", 
  LNG_WILL_RESUME_FILLING,   "Will Resume Filling",   "Wird Mit Befüllen Fortfahren", "Zal Vullen Hervatten",       "Reprendre Le Remplissage", "Reanudará El Llenado",          "Riprenderà Il Riempimento", 
  LNG_FAST_FILLING,          "Fast Filling",          "Schnelles Befüllen",           "Snel Vullen",                "Remplissage Rapide",       "Llenado Rápido",                "Riempimento Veloce", 
  LNG_SLOW_FILLING,          "Slow Filling",          "Langsam Abfüllen",             "Langzaam Vullen",            "Remplissage Lent",         "Llenado Lento",                 "Riempimento Lento", 
  LNG_PAUSED_FILL,           "Paused Fill",           "Füllvorgang Angehalten",       "Vullen Gepauseerd",          "Remplissage En Pause",     "Llenado En Pausa",              "Riempimento in pausa", 
  LNG_JAR_IS_READY,          "Jar %d Is Ready!",      "Glas %d Ist Fertig!",          "Pot %d Is Klaar!",           "Pot %d Est Prêt",          "Frasco %d Está Listo",          "Barattolo %d È Pronto", 


  LNG_LAST,   "?E", "?DE", "?NL", "?FR", "?SP", "?IT", 

};


#else
extern Translation Trans[];
extern char* GetTrans(int idx);
#endif


// English - German - Dutch - French - Spanish - Italian
//Trans Words[] = 
//{ LNG_SET_TARA_VAL,    "Set Tarra Values", "Tara Werte",        "Stel Tarra's In",       "Définir Les Tares", "", "",
//  LNG_CALIBRATE_SCALE, "Calibrate Scale",  "Waage Kalibrieren", "Weegschaal Calibreren", "Calibrer La Balance", "", "",
//  LNG_WEIGTH_PRESET, "Net Weight Presets",  "Fuell Menge", "Netto Vulgewichten",         "Poids De Remplissage", "", "",
//  LNG_SAVE_AND_EXIT,   "Save & Exit",      "Speichern",         "Opslaan",               "Sauvegarder", "", ""
//};