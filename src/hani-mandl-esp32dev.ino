// HaniMandl rewrite by Frank
// For readabilty, I removed and changed as I saw fit
// Supports only 320x240 color tft display ST7789
// Code provided as is, use as you please

#include <Arduino.h>
#include <Wire.h>
#include <HX711.h>                  /* aus dem Bibliotheksverwalter */
#include <ESP32Servo.h>             /* aus dem Bibliotheksverwalter */
#include <Adafruit_INA219.h>        /* aus dem Bibliotheksverwalter */
#include <Preferences.h>            /* aus dem BSP von expressif, wird verfügbar wenn das richtige Board ausgewählt ist */
#include <nvs_flash.h>              /* aus dem BSP von expressif, wird verfügbar wenn das richtige Board ausgewählt ist */
#include <Arduino_GFX_Library.h>    /* aus dem Bibliotheksverwalter */

// Version
String version = "V1.0 by F.";

//
// Hier den Code auf die verwendete Hardware einstellen
//

#define SCALE_TYP 2               // 1 = 2kg Wägezelle
                                  // 2 = 5kg Wägezelle
#define SERVO_ERWEITERT           // definieren, falls die Hardware mit dem alten Programmcode mit Poti aufgebaut wurde oder der Servo zu wenig fährt
                                  // Sonst bleibt der Servo in Stop-Position einige Grad offen! Nach dem Update erst prüfen!
#define ROTARY_SCALE 2            // number of steps for each click of your rotary encoder used

//#define FEHLERKORREKTUR_WAAGE   // falls Gewichtssprünge auftreten, können diese hier abgefangen werden
                                  // Achtung, kann den Wägeprozess verlangsamen. Vorher Hardware prüfen.
//#define QUETSCHHAHN_LINKS       // Servo invertieren, falls der Quetschhahn von links geöffnet wird. Mindestens ein Exemplar bekannt
//
// Ende Benutzereinstellungen!
// 

//
// Ab hier nur verstellen wenn Du genau weisst, was Du tust!
//
//#define isDebug 4             // serielle debug-Ausgabe aktivieren. Mit > 3 wird jeder Messdurchlauf ausgegeben
                                // mit 4 zusätzlich u.a. Durchlaufzeiten
                                // mit 5 zusätzlich rotary debug-Infos
                                // ACHTUNG: zu viel Serieller Output kann einen ISR-Watchdog Reset auslösen!

#if SCALE_TYP == 1
  #define MAXIMALGEWICHT 1500     // Maximales Gewicht für 2kg Wägezelle
#elif SCALE_TYP == 2
  #define MAXIMALGEWICHT 4500     // Maximales Gewicht für 5kg Wägezelle
#else
  #error Keine Wägezelle definiert
#endif

// Ansteuerung der Waage
#define SCALE_READS 2           // Parameter für hx711 Library. Messwert wird aus der Anzahl gemittelt
#define SCALE_GETUNITS(n)       round(scale.get_units(n))

// Ansteuerung Servo
#ifdef QUETSCHHAHN_LINKS
  #define SERVO_WRITE(n)     servo.write(180-n)
#else
  #define SERVO_WRITE(n)     servo.write(n)
#endif


// Betriebsmodi 
#define MODE_SETUP       0
#define MODE_AUTOMATIK   1
#define MODE_HANDBETRIEB 2

// Buzzer Sounds
#define BUZZER_SHORT   1
#define BUZZER_LONG    2
#define BUZZER_SUCCESS 3
#define BUZZER_ERROR   4

// INA 219
Adafruit_INA219 ina219;

// Display
Arduino_DataBus *bus = new Arduino_HWSPI(13 /* DC */, 5 /* CS */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 14 /* RST */, 3 /* rotation */);

// Fonts
#include "./Fonts/Punk_Mono_Bold_120_075.h"           //10 x 7
#include "./Fonts/Punk_Mono_Bold_160_100.h"           //13 x 9
#include "./Fonts/Punk_Mono_Bold_200_125.h"           //16 x 12
#include "./Fonts/Punk_Mono_Bold_240_150.h"           //19 x 14
//#include "./Fonts/Punk_Mono_Bold_280_175.h"          //22 x 16
#include "./Fonts/Punk_Mono_Bold_320_200.h"           //25 x 18
#include "./Fonts/Punk_Mono_Bold_600_375.h"           //48 x 36
#include "./Fonts/Punk_Mono_Thin_120_075.h"           //10 x 7
#include "./Fonts/Punk_Mono_Thin_160_100.h"           //13 x 9
#include "./Fonts/Punk_Mono_Thin_240_150.h"           //19 x 14
#include "./Fonts/Icons_Start_Stop.h"                 //A=Start, B=Stop, M=Rahmen
#include "./Fonts/Checkbox.h"                         //A=OK, B=nOK

//Logos
#include "./Logos/LogoBieneTFT.h"


// Pin Definitions
// ESP32-DEVKIT 30 Pins Connector

// Rotary Encoder
#define EncoderA 33
#define EncoderB 26
#define EncoderButton 32

//const int outputA  = 33;
//const int outputB  = 26;
//const int outputSW = 32;
// Servo
const int servo_pin = 2;
// 3x Schalter Ein 1 - Aus - Ein 2, VCC pins wurden direkt auf VCC gelegt
const int switch_betrieb_pin = 15;
const int switch_setup_pin   = 4;
// Taster, VCC pins wurden direkt auf VCC gelegt
const int button_start_pin     = 12;
const int button_stop_pin      = 27;
// Wägezelle-IC 
const int hx711_sck_pin = 17;
const int hx711_dt_pin  = 35;
// Buzzer and LED
static int buzzer_pin = 25;
int led_pin = 0;

Servo servo;
HX711 scale;
Preferences preferences;

// Denstrukturen für Rotary Encoder
struct rotary {                        
  int Value;
  int Minimum;
  int Maximum;
  int Step;
};
#define SW_WINKEL    0
#define SW_KORREKTUR 1
#define SW_MENU      2
struct rotary rotaries[3];         // Werden in setup() initialisiert
int rotary_select = SW_WINKEL;
static boolean rotating = false;   // debounce management für Rotary Encoder

// Füllmengen für 5 verschiedene Gläser
struct glas { 
  int Gewicht;
  int GlasTyp;
  int Tara;
  int TripCount;
  int Count;
};

const char *GlasTypArray[3] = {"DIB", "TOF", "DEE"};
struct glas glaeser[5] = { 
                            {125, 0, -9999, 0, 0},
                            {250, 1, -9999, 0, 0},
                            {250, 2, -9999, 0, 0},
                            {500, 1, -9999, 0, 0},
                            {500, 0, -9999, 0, 0} 
                          };

// Allgemeine Variablen
int i;                              // allgemeine Zählvariable
int pos;                            // aktuelle Position des Poti bzw. Rotary 
int gewicht;                        // aktuelles Gewicht
int tara;                           // Tara für das ausgewählte Glas, für Automatikmodus
int tara_glas;                      // Tara für das aktuelle Glas, falls Glasgewicht abweicht
long gewicht_leer;                  // Gewicht der leeren Waage
float faktor;                       // Skalierungsfaktor für Werte der Waage
int fmenge;                         // ausgewählte Füllmenge
int fmenge_index;                   // Index in gläser[]
int korrektur;                      // Korrekturwert für Abfüllmenge
int autostart;                      // Vollautomatik ein/aus
int autokorrektur;                  // Autokorrektur ein/aus
int kulanz_gr;                      // gewollte Überfüllung im Autokorrekturmodus in Gramm
int winkel;                         // aktueller Servo-Winkel
int winkel_hard_min = 0;            // Hard-Limit für Servo
int winkel_hard_max = 180;          // Hard-Limit für Servo
int winkel_min = 0;                 // konfigurierbar im Setup
int winkel_max = 85;                // konfigurierbar im Setup
int winkel_fein = 35;               // konfigurierbar im Setup
float fein_dosier_gewicht = 60;     // float wegen Berechnung des Schliesswinkels
int servo_aktiv = 0;                // Servo aktivieren ja/nein
int kali_gewicht = 500;             // frei wählbares Gewicht zum kalibrieren
char ausgabe[30];                   // Fontsize 12 = 13 Zeichen maximal in einer Zeile
int modus = -1;                     // Bei Modus-Wechsel den Servo auf Minimum fahren
int auto_aktiv = 0;                 // Für Automatikmodus - System ein/aus?
int waage_vorhanden = 0;            // HX711 nicht ansprechen, wenn keine Waage angeschlossen ist, sonst Crash
long preferences_chksum;            // Checksumme, damit wir nicht sinnlos Prefs schreiben
int buzzermode = 0;                 // 0 = aus, 1 = ein.
int ledmode = 0;                    // 0 = aus, 1 = ein.
bool gezaehlt = true;               // Kud Zähl-Flag
int glastoleranz = 20;              // Gewicht für autostart darf um +-20g schwanken, insgesamt also 40g!
int MenuepunkteAnzahl;              // Anzahl Menüpunkte im Setupmenü
int lastpos = 0;                    // Letzte position im Setupmenü
int progressbar = 0;                // Variable für den Progressbar
int showlogo = 1;                   // 0 = aus, 1 = ein
int showcredits = 1;                // 0 = aus, 1 = ein
int ina219_installed = 0;           // 0 = kein INA219 instaliert, 1 = INA219 ist instaliert
int current_servo = 0;              // 0 = INA219 wird ignoriert, 1-1000 = erlaubter Maximalstrom vom Servo in mA
int current_mA;                     // Strom welcher der Servo zieht
int updatetime_ina219 = 500;        // Zeit in ms in welchem der INA219 gelesen wird (500 -> alle 0,5 sek wird eine Strommessung vorgenommen)
int last_ina219_measurement = 0;    // Letzte Zeit in welcher der Strom gemessen wurde
int overcurrenttime = 1500;         // Zeit in welcher der Servo mehr Strom ziehen darf als einfestellt in ms
int last_overcurrenttime = 0;       // Letzte Zeit in welcher keinen Ueberstrom gemessen wurde
int alarm_overcurrent = 0;          // Alarmflag wen der Servo zuwiel Strom zieht
int show_current = 0;               // 0 = aus, 1 = ein / Zeige den Strom an auch wenn der INA ignoriert wird
int inawatchdog = 1;                // 0 = aus, 1 = ein / wird benötigt um INA messung auszusetzen
int offset_winkel = 0;              // Offset in Grad vom Schlieswinkel wen der Servo Übersrom hatte (max +3Grad vom eingestelten Winkel min)
int color_scheme = 0;               // 0 = dunkel, 1 = hell / Wechsel vom color scheme für den TFT Display
int color_marker = 2;               // Farbe für den Marker für das TFT Display

//Variablen für TFT update
bool no_ina;
int gewicht_alt;
int winkel_min_alt;
int pos_alt;
int winkel_ist_alt;
int tara_alt;
int current_mA_alt;
int servo_aktiv_alt;
int auto_aktiv_alt;
int glas_alt;
int korr_alt;
int rotary_select_alt;
int autokorr_gr_alt;

//Color Scheme für den TFT Display
unsigned long  BACKGROUND;
unsigned long  TEXT;
unsigned long  MENU_POS1;
unsigned long  MENU_POS2;
unsigned long  MENU_POS3;
unsigned long  MENU_POS4;
unsigned long  MARKER;
//#define MARKER 0x1AF6

void tft_colors() {
  if (color_scheme == 0) {          //dunkel
    BACKGROUND  = 0x0000;
    TEXT        = 0xFFFF;
    MENU_POS1   = 0x73AF;
    MENU_POS2   = 0x5AEC;
    MENU_POS3   = 0x39C8;
    MENU_POS4   = 0x20E6;
  }
  else {                            //hell
    BACKGROUND  = 0xFFFF;
    TEXT        = 0x0000;
    MENU_POS1   = 0x1082;
    MENU_POS2   = 0x2104;
    MENU_POS3   = 0x3166;
    MENU_POS4   = 0x4208;
  }
}

void tft_marker() {                
  if (color_marker == 0)                              {MARKER = 0x05ff;}
  else if (color_marker == 1)                         {MARKER = 0x021f;}
  else if (color_marker == 2)                         {MARKER = 0x001f;}
  else if (color_marker == 3)                         {MARKER = 0x4851;}
  else if (color_marker == 4)                         {MARKER = 0xc050;}
  else if (color_marker == 5)                         {MARKER = 0xf800;}
  else if (color_marker == 6)                         {MARKER = 0xfbc0;}
  else if (color_marker == 7)                         {MARKER = 0xfde0;}
  else if (color_marker == 8)                         {MARKER = 0xd6e1;}
  else if (color_marker == 9)                         {MARKER = 0xffe0;}
  else if (color_marker == 10)                        {MARKER = 0x2724;}
  else if (color_marker == 11)                        {MARKER = 0x0da1;}
}

// Rotary Taster. Der Interrupt kommt nur im Automatikmodus zum Tragen und nur wenn der Servo inaktiv ist.
// Der Taster schaltet in einen von drei Modi, in denen unterschiedliche Werte gezählt werden.
void IRAM_ATTR isr1() {
  static unsigned long last_interrupt_time = 0; 
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 300) {      // If interrupts come faster than 300ms, assume it's a bounce and ignore
    if ( modus == MODE_AUTOMATIK && servo_aktiv == 0 ) { // nur im Automatik-Modus interessiert uns der Click
      rotary_select = (rotary_select + 1) % 3;
      if (rotary_select == SW_KORREKTUR or rotary_select - 1 == SW_KORREKTUR) {
        rotary_select_alt = SW_KORREKTUR;
        korr_alt = -99999;
      }
      if (rotary_select == SW_MENU or rotary_select + 2 == SW_MENU) {
        glas_alt = -1;
      }
      #ifdef isDebug
        Serial.print("Rotary Button changed to ");
        Serial.println(rotary_select);
      #endif 
    }
    last_interrupt_time = interrupt_time;
  }
}

// Rotary Encoder. Counts in one of three rotary structures
// SW_WINKEL    = value set for servo angle
// SW_KORREKTUR = correction factor for filling weight
// SW_MENU      = counter for menu selection or parameter setting
void IRAM_ATTR isr2() {
  static int aState;
  static int bState;
  static int aLastState; 
  aState = digitalRead(EncoderA); // Reads the "current" state of the encoder
  bState = digitalRead(EncoderB); // Reads the "current" state of the encoder
  if (aState != aLastState) {     
    // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
    if (aState != bState) {
      rotaries[rotary_select].Value -= rotaries[rotary_select].Step;
    }
    else {    // counter-clockwise
      rotaries[rotary_select].Value += rotaries[rotary_select].Step;
    }
    rotaries[rotary_select].Value = constrain( rotaries[rotary_select].Value, rotaries[rotary_select].Minimum, rotaries[rotary_select].Maximum);
    rotating = false;
  }
  aLastState = aState; 
}

//
// Skalierung des Rotaries für verschiedene Rotary Encoder
int getRotariesValue( int rotary_mode ) {
  return ((rotaries[rotary_mode].Value - (rotaries[rotary_mode].Value % (rotaries[rotary_mode].Step * ROTARY_SCALE))) / ROTARY_SCALE );
}

void setRotariesValue( int rotary_mode, int rotary_value ) {
  rotaries[rotary_mode].Value = rotary_value * ROTARY_SCALE;
}

void initRotaries( int rotary_mode, int rotary_value, int rotary_min, int rotary_max, int rotary_step ) {
  rotaries[rotary_mode].Value     = rotary_value * ROTARY_SCALE;
  rotaries[rotary_mode].Minimum   = rotary_min   * ROTARY_SCALE;
  rotaries[rotary_mode].Maximum   = rotary_max   * ROTARY_SCALE;
  rotaries[rotary_mode].Step      = rotary_step;
  #ifdef isDebug
    Serial.print("initRotaries..."); 
    Serial.print(" Rotary Mode: ");  Serial.print(rotary_mode);
    Serial.print(" rotary_value: "); Serial.print(rotary_value);
    Serial.print(" Value: ");        Serial.print(rotaries[rotary_mode].Value);
    Serial.print(" Min: ");          Serial.print(rotaries[rotary_mode].Minimum);
    Serial.print(" Max: ");          Serial.print(rotaries[rotary_mode].Maximum);
    Serial.print(" Step: ");         Serial.print(rotaries[rotary_mode].Step);
    Serial.print(" Scale: ");        Serial.println(ROTARY_SCALE);
  #endif
}
// Ende Funktionen für den Rotary Encoder
//
boolean EncoderButtonTapped(void)
{ if (digitalRead(EncoderButton) == LOW) {
    while(digitalRead(EncoderButton) == LOW) {
    delay(1);
    }
    return true;
  }  
  else return false;
}    

boolean EncoderButtonPressed(void)
{ if (digitalRead(EncoderButton) == LOW) {
    return true;
  }
  else return false;
}  



void getPreferences(void) {
  preferences.begin("EEPROM", false);                     // Parameter aus dem EEPROM lesen
  faktor          = preferences.getFloat("faktor", 0.0);  // falls das nicht gesetzt ist -> Waage ist nicht kalibriert
  pos             = preferences.getUInt("pos", 0);
  gewicht_leer    = preferences.getUInt("gewicht_leer", 0); 
  korrektur       = preferences.getUInt("korrektur", 0);
  autostart       = preferences.getUInt("autostart", 0);
  autokorrektur   = preferences.getUInt("autokorrektur", 0);
  kulanz_gr       = preferences.getUInt("kulanz_gr", 5);
  fmenge_index    = preferences.getUInt("fmenge_index", 3);
  winkel_min      = preferences.getUInt("winkel_min", winkel_min);
  winkel_max      = preferences.getUInt("winkel_max", winkel_max);
  winkel_fein     = preferences.getUInt("winkel_fein", winkel_fein);
  buzzermode      = preferences.getUInt("buzzermode", buzzermode);
  ledmode         = preferences.getUInt("ledmode", ledmode);
  showlogo        = preferences.getUInt("showlogo", showlogo);
  showcredits     = preferences.getUInt("showcredits", showcredits);
  kali_gewicht    = preferences.getUInt("kali_gewicht", kali_gewicht);
  glastoleranz    = preferences.getUInt("glastoleranz", glastoleranz);
  current_servo   = preferences.getUInt("current_servo", current_servo);
  show_current    = preferences.getUInt("show_current", show_current);
  color_scheme    = preferences.getUInt("color_scheme", color_scheme);
  color_marker    = preferences.getUInt("color_marker", color_marker);
  preferences_chksum = faktor + pos + gewicht_leer + korrektur + autostart + autokorrektur + kulanz_gr + fmenge_index +
                       winkel_min + winkel_max + winkel_fein + buzzermode + ledmode + showlogo + showcredits + 
                       kali_gewicht + current_servo + glastoleranz + show_current + color_scheme + color_marker;
  i = 0;
  int ResetGewichte[] = {125,250,250,500,500,};
  int ResetGlasTyp[] = {0,1,2,1,0,};
  while( i < 5) {
    sprintf(ausgabe, "Gewicht%d", i);
    glaeser[i].Gewicht = preferences.getInt(ausgabe, ResetGewichte[i]);
    preferences_chksum += glaeser[i].Gewicht;
    sprintf(ausgabe, "GlasTyp%d", i);
    glaeser[i].GlasTyp = preferences.getInt(ausgabe, ResetGlasTyp[i]);
    preferences_chksum += glaeser[i].GlasTyp;
    sprintf(ausgabe, "Tara%d", i);
    glaeser[i].Tara= preferences.getInt(ausgabe, -9999);
    preferences_chksum += glaeser[i].Tara;
    sprintf(ausgabe, "TripCount%d", i);
    glaeser[i].TripCount = preferences.getInt(ausgabe, 0);
    preferences_chksum += glaeser[i].TripCount;
    sprintf(ausgabe, "Count%d", i);
    glaeser[i].Count = preferences.getInt(ausgabe, 0);
    preferences_chksum += glaeser[i].Count;
    i++;
  }
  preferences.end();
  #ifdef isDebug
    Serial.println("get Preferences:");
    Serial.print("pos = ");             Serial.println(pos);
    Serial.print("faktor = ");          Serial.println(faktor);
    Serial.print("gewicht_leer = ");    Serial.println(gewicht_leer);
    Serial.print("korrektur = ");       Serial.println(korrektur);
    Serial.print("autostart = ");       Serial.println(autostart);
    Serial.print("autokorrektur = ");   Serial.println(autokorrektur);
    Serial.print("kulanz_gr = ");       Serial.println(kulanz_gr);
    Serial.print("fmenge_index = ");    Serial.println(fmenge_index);
    Serial.print("winkel_min = ");      Serial.println(winkel_min);
    Serial.print("winkel_max = ");      Serial.println(winkel_max);
    Serial.print("winkel_fein = ");     Serial.println(winkel_fein);
    Serial.print("buzzermode = ");      Serial.println(buzzermode);
    Serial.print("ledmode = ");         Serial.println(ledmode);
    Serial.print("showlogo = ");        Serial.println(showlogo);
    Serial.print("showcredits = ");     Serial.println(showcredits);
    Serial.print("current_servo = ");   Serial.println(current_servo);
    Serial.print("color_scheme = ");    Serial.println(color_scheme);
    Serial.print("color_marker = ");    Serial.println(color_marker);
    Serial.print("kali_gewicht = ");    Serial.println(kali_gewicht);
    Serial.print("glastoleranz = ");    Serial.println(glastoleranz);
    Serial.print("show_current = ");    Serial.println(show_current);
    i = 0;
    while( i < 5 ) {
      sprintf(ausgabe, "Gewicht%d = ", i);
      Serial.print(ausgabe);         
      Serial.println(glaeser[i].Gewicht);
      sprintf(ausgabe, "GlasTyp%d = ", i);
      Serial.print(ausgabe);         
      Serial.println(GlasTypArray[glaeser[i].GlasTyp]);
      sprintf(ausgabe, "Tara%d = ", i);
      Serial.print(ausgabe);         
      Serial.println(glaeser[i].Tara);
      i++;
    }
    Serial.print("Checksumme:");    
    Serial.println(preferences_chksum);    
  #endif
}

void setPreferences(void) {
  long preferences_newchksum;
  int winkel = getRotariesValue(SW_WINKEL);
  int i;
  preferences.begin("EEPROM", false);
  // Winkel-Einstellung separat behandeln, ändert sich häufig
  if ( winkel != preferences.getUInt("pos", 0) ) {
    preferences.putUInt("pos", winkel);
    #ifdef isDebug
      Serial.print("winkel gespeichert: ");
      Serial.println(winkel);
    #endif
  }
  // Counter separat behandeln, ändert sich häufig
  for ( i=0 ; i < 5; i++ ) {
    sprintf(ausgabe, "TripCount%d", i);
    if (glaeser[i].TripCount != preferences.getInt(ausgabe, 0)) {
      preferences.putInt(ausgabe, glaeser[i].TripCount);
    }
    sprintf(ausgabe, "Count%d", i);
    if (glaeser[i].Count != preferences.getInt(ausgabe, 0)) {
      preferences.putInt(ausgabe, glaeser[i].Count);
    }
    #ifdef isDebug
      Serial.print("Counter gespeichert: Index ");
      Serial.print(i);
      Serial.print(" Trip ");
      Serial.print(glaeser[fmenge_index].TripCount);
      Serial.print(" Gesamt ");
      Serial.println(glaeser[fmenge_index].Count);      
    #endif
  }
  // Den Rest machen wir gesammelt, das ist eher statisch
  preferences_newchksum = faktor + gewicht_leer + korrektur + autostart + autokorrektur +
                          fmenge_index + winkel_min + winkel_max + winkel_fein + kulanz_gr +
                          buzzermode + ledmode + showlogo + showcredits + current_servo + kali_gewicht + 
                          glastoleranz + show_current + color_scheme + color_marker;
  i = 0;
  while( i < 5 ) {
    preferences_newchksum += glaeser[i].Gewicht;
    preferences_newchksum += glaeser[i].GlasTyp;
    preferences_newchksum += glaeser[i].Tara;
    i++;
  }
  if( preferences_newchksum == preferences_chksum ) {
    #ifdef isDebug
      Serial.println("Preferences unverändert");
    #endif
    return;
  }
  preferences_chksum = preferences_newchksum;
  preferences.putFloat("faktor", faktor);
  preferences.putUInt("gewicht_leer", gewicht_leer);
  preferences.putUInt("korrektur", korrektur);
  preferences.putUInt("autostart", autostart);
  preferences.putUInt("autokorrektur", autokorrektur);
  preferences.putUInt("kulanz_gr", kulanz_gr);
  preferences.putUInt("winkel_min", winkel_min);
  preferences.putUInt("winkel_max", winkel_max);
  preferences.putUInt("winkel_fein", winkel_fein);
  preferences.putUInt("fmenge_index", fmenge_index);
  preferences.putUInt("buzzermode", buzzermode);
  preferences.putUInt("ledmode", ledmode);
  preferences.putUInt("showlogo", showlogo);
  preferences.putUInt("showcredits", showcredits);
  preferences.putUInt("kali_gewicht", kali_gewicht);
  preferences.putUInt("glastoleranz", glastoleranz);
  preferences.putUInt("current_servo", current_servo);
  preferences.putUInt("show_current", show_current);
  preferences.putUInt("color_scheme", color_scheme);
  preferences.putUInt("color_marker", color_marker);
  i = 0;
  while( i < 5 ) {
    sprintf(ausgabe, "Gewicht%d", i);
    preferences.putInt(ausgabe, glaeser[i].Gewicht);
    sprintf(ausgabe, "GlasTyp%d", i);
    preferences.putInt(ausgabe, glaeser[i].GlasTyp);  
    sprintf(ausgabe, "Tara%d", i);
    preferences.putInt(ausgabe, glaeser[i].Tara);
    i++;
  }
  preferences.end();
  #ifdef isDebug
    Serial.println("Set Preferences:");
    Serial.print("pos = ");             Serial.println(winkel);
    Serial.print("faktor = ");          Serial.println(faktor);
    Serial.print("gewicht_leer = ");    Serial.println(gewicht_leer);
    Serial.print("korrektur = ");       Serial.println(korrektur);
    Serial.print("autostart = ");       Serial.println(autostart);
    Serial.print("autokorrektur = ");   Serial.println(autokorrektur);
    Serial.print("kulanz_gr = ");       Serial.println(kulanz_gr);
    Serial.print("fmenge_index = ");    Serial.println(fmenge_index);
    Serial.print("winkel_min = ");      Serial.println(winkel_min);
    Serial.print("winkel_max = ");      Serial.println(winkel_max);
    Serial.print("winkel_fein = ");     Serial.println(winkel_fein);
    Serial.print("buzzermode = ");      Serial.println(buzzermode);
    Serial.print("ledmode = ");         Serial.println(ledmode);
    Serial.print("showlogo = ");        Serial.println(showlogo);
    Serial.print("showcredits = ");     Serial.println(showcredits);
    Serial.print("current_servo = ");   Serial.println(current_servo);
    Serial.print("kali_gewicht = ");    Serial.println(kali_gewicht);
    Serial.print("glastoleranz = ");    Serial.println(glastoleranz);
    Serial.print("show_current = ");    Serial.println(show_current);
    Serial.print("color_scheme = ");    Serial.println(color_scheme);
    Serial.print("color_marker = ");    Serial.println(color_marker);
    i = 0;
    while( i < 5 ) {
      sprintf(ausgabe, "Gewicht%d = ", i);
      Serial.print(ausgabe);         Serial.println(glaeser[i].Gewicht);
      sprintf(ausgabe, "GlasTyp%d = ", i);
      Serial.print(ausgabe);         Serial.println(GlasTypArray[glaeser[i].GlasTyp]);
      sprintf(ausgabe, "Tara%d = ", i);
      Serial.print(ausgabe);         Serial.println(glaeser[i].Tara);
      i++;
    }
  #endif
}

void setupTripCounter(void) {
  int j;
  i = 1;
  float TripAbfuellgewicht = 0;
  int y_offset_tft = 28;
  const char title[] = "Zählwerk Trip";
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  while (i > 0) { //Erster Screen: Anzahl pro Glasgröße
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    j = 0;
    while ( j < 5 ) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", glaeser[j].Gewicht,GlasTypArray[glaeser[j].GlasTyp]);
      gfx->print(ausgabe);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%11d St.", glaeser[j].TripCount);
      gfx->print(ausgabe);
      j++;
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Zweiter Screen: Gewicht pro Glasgröße
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    j = 0;
    while ( j < 5 ) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", glaeser[j].Gewicht,GlasTypArray[glaeser[j].GlasTyp]);
      gfx->print(ausgabe);
      gfx->setCursor(150, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%10.3fkg", glaeser[j].Gewicht * glaeser[j].TripCount / 1000.0);
      gfx->print(ausgabe);
      j++;
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Dritter Screen: Gesamtgewicht
    TripAbfuellgewicht = 0;
    j = 0;
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    while (j < 5) {
      TripAbfuellgewicht += glaeser[j].Gewicht * glaeser[j].TripCount / 1000.0;
      j++;
    }
    gfx->setFont(Punk_Mono_Bold_600_375);
    sprintf(ausgabe, "Summe");
    x_pos = CenterPosX(ausgabe, 36, 320);
    gfx->setCursor(x_pos, 124);
    gfx->print(ausgabe);
    sprintf(ausgabe, "%.1fkg", TripAbfuellgewicht);
    x_pos = CenterPosX(ausgabe, 36, 320);
    gfx->setCursor(x_pos, 184);
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Bold_240_150);
    if (digitalRead(EncoderButton) == LOW) {
      //verlasse Screen
      while(digitalRead(EncoderButton) == LOW) {
        delay(1);
      }
      i = 0;
      gfx->fillRect(0, 31, 320, 209, BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Vierter Screen: Zurücksetzen
    initRotaries(SW_MENU, 1, 0, 1, -1);
    rotaries[SW_MENU].Value = 1;
    i = 1;
    while (i > 0) {
      if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
        while (digitalRead(button_stop_pin) == HIGH) {
          delay(1);
        }
        modus = -1;
        return;
      }
      pos = getRotariesValue(SW_MENU);
      gfx->setCursor(5, 30+(1 * y_offset_tft));
      if (pos == 0) {gfx->setTextColor(MARKER);}
      else {gfx->setTextColor(TEXT);}
      gfx->print("Reset");
      gfx->setCursor(5, 30+(2 * y_offset_tft));
      if (pos == 1) {gfx->setTextColor(MARKER);}
      else {gfx->setTextColor(TEXT);}
      gfx->print("Abbrechen");
      if (digitalRead(EncoderButton) == LOW) {
        gfx->setTextColor(MARKER);
        gfx->setCursor(283, 30+((pos+1) * y_offset_tft));
        gfx->print("OK");
        if ( pos == 0) {
          j = 0;
          while ( j < 5  ) {
            glaeser[j].TripCount = 0;
            j++;
          }
          setPreferences();
        }
        delay(1000);
        modus = -1;
        i = 0;
      }
    }
  }
}

void setupCounter(void) {
  int j;
  i = 1;
  float Abfuellgewicht = 0;
  int y_offset_tft = 28;
  const char title[] = "Zählwerk";
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  while (i > 0) { //Erster Screen: Anzahl pro Glasgröße
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    j = 0;
    while ( j < 5 ) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", glaeser[j].Gewicht,GlasTypArray[glaeser[j].GlasTyp]);
      gfx->print(ausgabe);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%11d St.", glaeser[j].Count);
      gfx->print(ausgabe);
      j++;
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Zweiter Screen: Gewicht pro Glasgröße
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    j = 0;
    while ( j < 5 ) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", glaeser[j].Gewicht,GlasTypArray[glaeser[j].GlasTyp]);
      gfx->print(ausgabe);
      gfx->setCursor(150, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%10.3fkg", glaeser[j].Gewicht * glaeser[j].Count / 1000.0);
      gfx->print(ausgabe);
      j++;
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Dritter Screen: Gesamtgewicht
    Abfuellgewicht = 0;
    j = 0;
    if (digitalRead(button_stop_pin) == HIGH  or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    while ( j < 5  ) {
      Abfuellgewicht += glaeser[j].Gewicht * glaeser[j].Count / 1000.0;
      j++;
    }
    gfx->setFont(Punk_Mono_Bold_600_375);
    sprintf(ausgabe, "Summe");
    x_pos = CenterPosX(ausgabe, 36, 320);
    gfx->setCursor(x_pos, 124);
    gfx->print(ausgabe);
    sprintf(ausgabe, "%.1fkg", Abfuellgewicht);
    x_pos = CenterPosX(ausgabe, 36, 320);
    gfx->setCursor(x_pos, 184);
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Bold_240_150);
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Vierter Screen: Zurücksetzen
    initRotaries(SW_MENU, 1, 0, 1, -1);
    rotaries[SW_MENU].Value = 1;
    i = 1;
    while (i > 0) {
      if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
        while (digitalRead(button_stop_pin) == HIGH) {
          delay(1);
        }
        modus = -1;
        return;
      }
      pos = getRotariesValue(SW_MENU);
      gfx->setCursor(5, 30+(1 * y_offset_tft));
      if (pos == 0) {gfx->setTextColor(MARKER);}
      else {gfx->setTextColor(TEXT);}
      gfx->print("Reset");
      gfx->setCursor(5, 30+(2 * y_offset_tft));
      if (pos == 1) {gfx->setTextColor(MARKER);}
      else {gfx->setTextColor(TEXT);}
      gfx->print("Abbrechen");
      if (EncoderButtonPressed()) {
        gfx->setTextColor(MARKER);
        gfx->setCursor(283, 30+((pos+1) * y_offset_tft));
        gfx->print("OK");
        if ( pos == 0) {
          j = 0;
          while ( j < 5  ) {
            glaeser[j].Count = 0;
            glaeser[j].TripCount = 0;
            j++;
          }
          setPreferences();
        }
        delay(1000);
        modus = -1;
        i = 0;
      }
    }
  }
}

void setupTara(void) {
  int j;
  int x_pos;
  tara = 0;
  initRotaries(SW_MENU, 0, 0, 4, -1);   // Set Encoder to Menu Mode, four Selections, inverted count
  i = 0;
  gfx->fillScreen(BACKGROUND);
  bool change = false;
  const char menu[] = "Tarawerte Gläser";
  while (i == 0)  {
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    else if (EncoderButtonPressed()) {
      tara = int(SCALE_GETUNITS(10));
      if (tara > 20) {                  // Gläser müssen mindestens 20g haben
         glaeser[getRotariesValue(SW_MENU)].Tara = tara;
      }
      change = true;
      i++;
    }
    if (change) {
      gfx->fillScreen(BACKGROUND);
    }
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Bold_240_150);
    x_pos = CenterPosX(menu, 14, 320);
    gfx->setCursor(x_pos, 25);
    gfx->println(menu);
    gfx->drawLine(0, 30, 320, 30, TEXT);
    j = 0;
    while(j < 5) {
      if (j == getRotariesValue(SW_MENU)) {
        gfx->setTextColor(MARKER);
      }
      else {
        gfx->setTextColor(TEXT);
      }
      gfx->setCursor(5, 60+(j*30));
      if (glaeser[j].Gewicht < 1000) {
        sprintf(ausgabe, " %3dg - %3s", glaeser[j].Gewicht, GlasTypArray[glaeser[j].GlasTyp]); 
      } 
      else {
        sprintf(ausgabe, "%.1fkg - %3s", float(glaeser[j].Gewicht) / 1000, GlasTypArray[glaeser[j].GlasTyp]); 
      }
      gfx->println(ausgabe);
      gfx->setCursor(215, 60+(j*30));
      if (glaeser[j].Tara > 0) { 
        sprintf(ausgabe, "  %4dg", glaeser[j].Tara); 
        gfx->print(ausgabe);
      }
      else {
        gfx->print("  fehlt");
      }
      j++;
    }
  }
  modus = -1;
  delay(2000);
}

void setupCalibration(void) {
  float gewicht_raw;
  gfx->fillScreen(BACKGROUND);
  int kali_gewicht_old = -1;
  const char menu[] = "Kalibrieren";
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(menu, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(menu);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  sprintf(ausgabe, "Bitte Waage leeren");
  x_pos = CenterPosX(ausgabe, 14, 320);
  gfx->setCursor(x_pos, 100); gfx->print(ausgabe);
  sprintf(ausgabe, "und mit OK");
  x_pos = CenterPosX(ausgabe, 14, 320);
  gfx->setCursor(x_pos, 128); gfx->print(ausgabe);
  sprintf(ausgabe, "bestätigen");
  x_pos = CenterPosX(ausgabe, 14, 320);
  gfx->setCursor(x_pos, 156); gfx->print(ausgabe);
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    else if (EncoderButtonPressed()) {
      scale.set_scale();
      scale.tare(10);
      delay(500);
      i = 0;
    }
  }
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  x_pos = CenterPosX(menu, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(menu);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  initRotaries(SW_MENU, kali_gewicht, 100, 9999, 1); 
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    kali_gewicht = getRotariesValue(SW_MENU);
    if (kali_gewicht != kali_gewicht_old) {
      gfx->setTextColor(BACKGROUND);
      if ((kali_gewicht >= 1000 and kali_gewicht_old <= 999) or (kali_gewicht_old >= 1000 and kali_gewicht <= 999)) {
        sprintf(ausgabe, "Bitte %dg", kali_gewicht_old);
      }
      else {
        sprintf(ausgabe, "      %dg", kali_gewicht_old);
      }
      x_pos = CenterPosX(ausgabe, 14, 320);
      gfx->setCursor(x_pos, 100); gfx->print(ausgabe);
      gfx->setTextColor(TEXT);
      sprintf(ausgabe, "      %dg", kali_gewicht);
      x_pos = CenterPosX(ausgabe, 14, 320);
      gfx->setCursor(x_pos, 100); gfx->print("Bitte ");
      gfx->setTextColor(MARKER);
      gfx->setCursor(x_pos, 100); gfx->print(ausgabe);
      gfx->setTextColor(TEXT);
      sprintf(ausgabe, "aufstellen");
      x_pos = CenterPosX(ausgabe, 14, 320);
      gfx->setCursor(x_pos, 128); gfx->print(ausgabe);
      sprintf(ausgabe, "und mit OK");
      x_pos = CenterPosX(ausgabe, 14, 320);
      gfx->setCursor(x_pos, 156); gfx->print(ausgabe);
      sprintf(ausgabe, "bestätigen");
      x_pos = CenterPosX(ausgabe, 14, 320);
      gfx->setCursor(x_pos, 184); gfx->print(ausgabe);

      kali_gewicht_old = kali_gewicht;
    }
    if (EncoderButtonPressed()) {
      gewicht_raw  = scale.get_units(10);
      faktor       = gewicht_raw / kali_gewicht;
      scale.set_scale(faktor);
      gewicht_leer = scale.get_offset();    // Leergewicht der Waage speichern
      #ifdef isDebug
        Serial.print("kalibrier_gewicht = ");
        Serial.println(kali_gewicht);
        Serial.print("gewicht_leer = ");
        Serial.println(gewicht_leer);
        Serial.print("gewicht_raw = ");
        Serial.println(gewicht_raw);
        Serial.print(" Faktor = ");
        Serial.println(faktor);
      #endif        
      delay(1000);
      modus = -1;
      i = 0;        
    }
  }
}

void setupServoWinkel(void) {
  int menuitem;
  int menuitem_used = 4;
  int lastmin  = winkel_min;
  int lastfein = winkel_fein;
  int lastmax  = winkel_max;
  int wert_alt;
  bool wert_aendern = false;
  bool servo_live = false;
  initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
  int y_offset_tft = 28;
  bool change = false;
  int wert_old = -1;
  const char title[] = "Servoeinstellungen";
  int MenuepunkteAnzahl = 5;
  const char *menuepunkte[MenuepunkteAnzahl] = {"Livesetup", "Minimum", "Feindos.", "Maximum", "Speichern"};
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      winkel_min  = lastmin;
      winkel_fein = lastfein;
      winkel_max  = lastmax;
      if (servo_live == true) {
        SERVO_WRITE(winkel_min);
      }
      modus = -1;
      return;
    }
    if (wert_aendern == false) {
      menuitem = getRotariesValue(SW_MENU);
      pos = menuitem;
      if (menuitem == menuitem_used) {
        menuitem = 6;
      }
    }
    else {
      switch (menuitem) {
        case 0: servo_live  = getRotariesValue(SW_MENU);
                break;
        case 1: winkel_min  = getRotariesValue(SW_MENU);
                if (servo_live == true) SERVO_WRITE(winkel_min);
                break;
        case 2: winkel_fein = getRotariesValue(SW_MENU);
                if (servo_live == true) SERVO_WRITE(winkel_fein);
                break;
        case 3: winkel_max  = getRotariesValue(SW_MENU);
                if (servo_live == true) SERVO_WRITE(winkel_max);
                break;
      }
    }
    int j = 0;
    while(j < MenuepunkteAnzahl) {
      gfx->setTextColor(TEXT);
      if (j == pos and wert_aendern == false) {
        gfx->setTextColor(MARKER);
      }
      if (j < MenuepunkteAnzahl - 1) {
        gfx->setCursor(5, 30+((j+1) * y_offset_tft));
        gfx->print(menuepunkte[j]);
        if (j == pos and wert_aendern == true) {
          sprintf(ausgabe,"(%d°)", wert_alt);
          if (j > 0) {
            gfx->setCursor(170, 30+((j+1) * y_offset_tft));
            gfx->print(ausgabe);
          }
          gfx->setTextColor(MARKER);
        }
        switch (j) {
          case 0: sprintf(ausgabe,"%s", servo_live==false?"aus":"ein");
                  if (wert_old != servo_live and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = servo_live;
                  }
                  break;
          case 1: sprintf(ausgabe,"%d°", winkel_min);
                  if (wert_old != winkel_min and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = winkel_min;
                  }
                  break;
          case 2: sprintf(ausgabe,"%d°", winkel_fein);
                  if (wert_old != winkel_fein and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = winkel_fein;
                  }
                  break;
          case 3: sprintf(ausgabe,"%d°", winkel_max);
                  if (wert_old != winkel_max and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = winkel_max;
                  }
                  break;
          }
          if (change) {
            gfx->fillRect(251, 27+((j+1) * y_offset_tft)-19, 64, 25, BACKGROUND);
            change = false;
          }
          int y = get_length(ausgabe);
          gfx->setCursor(320 - y * 14 - 5, 30+((j+1) * y_offset_tft));
          gfx->print(ausgabe);
        }
        else {
          gfx->setCursor(5, 30+(7 * y_offset_tft));
          gfx->print(menuepunkte[j]);
        }
        j++;
      }
    
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      switch (menuitem) { 
        case 0: initRotaries(SW_MENU, servo_live, 0, 1, 1);
              break;
        case 1: initRotaries(SW_MENU, winkel_min, winkel_hard_min, winkel_fein, 1);
              wert_alt = lastmin;
              break;
        case 2: initRotaries(SW_MENU, winkel_fein, winkel_min, winkel_max, 1);
              wert_alt = lastfein;
              break;
        case 3: initRotaries(SW_MENU, winkel_max, winkel_fein, winkel_hard_max, 1);
              wert_alt = lastmax;
              break;
      }
      wert_old = -1;
      wert_aendern = true;
    }
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == true) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      if (servo_live == true) {
        SERVO_WRITE(winkel_min);
      }
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
      gfx->fillRect(170, 30+y_offset_tft+3, 96, 3*y_offset_tft + 2, BACKGROUND);
    }
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      gfx->setCursor(283, 30+(7 * y_offset_tft));
      gfx->print("OK");
      modus = -1;
      delay(1000);
      i = 0;
    }
  }
}

void setupAutomatik(void) {
  int menuitem;
  int menuitem_used = 5;
  int lastautostart     = autostart;
  int lastglastoleranz  = glastoleranz;
  int lastautokorrektur = autokorrektur;
  int lastkulanz        = kulanz_gr;
  int korrektur_alt = korrektur;
  bool wert_aendern = false;
  int y_offset_tft = 28;
  bool change = false;
  int wert_old = -1;
  const char title[] = "Automatik";
  int MenuepunkteAnzahl = 6;
  const char *menuepunkte[MenuepunkteAnzahl] = {"Autostart", "Glastoleranz", "Korrektur", "Autokorrektur", "-> Kulanz", "Speichern"};
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      autostart     = lastautostart;
      autokorrektur = lastautokorrektur;
      kulanz_gr     = lastkulanz;
      glastoleranz  = lastglastoleranz;
      korrektur     = korrektur_alt;
      setRotariesValue(SW_KORREKTUR, korrektur_alt);
      rotary_select = SW_MENU;
      modus = -1;
      return;
    }
    if (wert_aendern == false) {
      menuitem = getRotariesValue(SW_MENU);
      pos = menuitem;
      if (menuitem == menuitem_used) {
        menuitem = 6;
      }
    }
    else {
      switch (menuitem) {
        case 0: autostart     = getRotariesValue(SW_MENU);
                break;
        case 1: glastoleranz  = getRotariesValue(SW_MENU);
                break;
        case 2: korrektur     = getRotariesValue(SW_KORREKTUR);
                break;
        case 3: autokorrektur = getRotariesValue(SW_MENU);
                break;
        case 4: kulanz_gr     = getRotariesValue(SW_MENU);
                break;
      }
    }
    // Menu
    int j = 0;
    while(j < MenuepunkteAnzahl) {
      gfx->setTextColor(TEXT);
      if (j == pos and wert_aendern == false) {
        gfx->setTextColor(MARKER);
      }
      if (j < MenuepunkteAnzahl - 1) {
        gfx->setCursor(5, 30+((j+1) * y_offset_tft));
        gfx->print(menuepunkte[j]);
        if (j == pos and wert_aendern == true) {
          gfx->setTextColor(MARKER);
        }
        switch (j) {
          case 0: sprintf(ausgabe,"%s", autostart==false?"aus":"ein");
                  if (wert_old != autostart and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = autostart;
                  }
                  break;
          case 1: if (glastoleranz > 0) {
                    sprintf(ausgabe,"±%dg", glastoleranz);
                  }
                  else {
                    sprintf(ausgabe,"%dg", glastoleranz);
                  }
                  if (wert_old != glastoleranz and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = glastoleranz;
                  }
                  break;
          case 2: sprintf(ausgabe,"%dg", korrektur);
                  if (wert_old != korrektur and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = korrektur;
                  }
                  break;
          case 3: sprintf(ausgabe,"%s", autokorrektur==false?"aus":"ein");
                  if (wert_old != autokorrektur and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = autokorrektur;
                  }
                  break;
          case 4: sprintf(ausgabe,"%dg", kulanz_gr);
                  if (wert_old != kulanz_gr and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = kulanz_gr;
                  }
                  break;
        }
        if (change) {
          gfx->fillRect(251, 27+((j+1) * y_offset_tft)-19, 64, 25, BACKGROUND);
          change = false;
        }
        int y = get_length(ausgabe);
        gfx->setCursor(320 - y * 14 - 5, 30+((j+1) * y_offset_tft));
        gfx->print(ausgabe);
      }
      else {
        gfx->setCursor(5, 30+(7 * y_offset_tft));
        gfx->print(menuepunkte[j]);
      }
      j++;
    }
    // Menupunkt zum Ändern ausgewählt
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) { 
      while(EncoderButtonPressed()) {
        delay(1);
      }
      switch (menuitem) { 
        case 0: rotary_select = SW_MENU;
                initRotaries(SW_MENU, autostart, 0, 1, 1);
                break;
        case 1: rotary_select = SW_MENU;
                initRotaries(SW_MENU, glastoleranz, 0, 99, 1);
                break;
        case 2: rotary_select = SW_KORREKTUR;
                break;
        case 3: rotary_select = SW_MENU;
                initRotaries(SW_MENU, autokorrektur, 0, 1, 1);
                break;
        case 4: rotary_select = SW_MENU;
                initRotaries(SW_MENU, kulanz_gr, 0, 99, 1);
                break;
      }
      wert_old = -1;
      wert_aendern = true;
    }
    // Änderung im Menupunkt übernehmen
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == true) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      rotary_select = SW_MENU;
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
    }
    // Menu verlassen 
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      gfx->setCursor(283, 30+(7 * y_offset_tft));
      gfx->print("OK");
      delay(1000);
      modus = -1;
      i = 0;
    }
  }
  rotary_select = SW_MENU;
}

void setupFuellmenge(void) {
  int y_offset_tft = 28;
  String wert_old = "";
  const char title[] = "Füllmenge Gläser";
  //int MenuepunkteAnzahl = 5;
  //const char *menuepunkte[MenuepunkteAnzahl] = {"Livesetup", "Minimum", "Feindos.", "Maximum", "Speichern"};
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  int j;
  initRotaries(SW_MENU, fmenge_index, 0, 4, -1);
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    pos = getRotariesValue(SW_MENU);
    j = 0;
    while(j < 5) {
    if (j == pos) {
        gfx->setTextColor(MARKER);
      }
      else {
        gfx->setTextColor(TEXT);
      }
      gfx->setCursor(10, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg", glaeser[j].Gewicht);
      gfx->print(ausgabe);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      if (GlasTypArray[glaeser[j].GlasTyp] == "DIB") {
        gfx->print("DE Imker Bund");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "TOF") {
        gfx->print("TwistOff");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "DEE") {
        gfx->print("DeepTwist");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "SPZ") {
        gfx->print("Spezial");
      }
      j++;
    }
    if (EncoderButtonTapped()) { // Füllmenge gewählt
      i = 0;
    }
  }
  i = 1;
  initRotaries(SW_MENU, weight2step(glaeser[pos].Gewicht) , 25, weight2step(MAXIMALGEWICHT), 1);
  while (i > 0){
    if ((digitalRead(button_stop_pin)) == HIGH  or digitalRead(switch_setup_pin) == LOW) {
      modus = -1;
      return;
    }
    glaeser[pos].Gewicht = step2weight(getRotariesValue(SW_MENU));
    j = 0;
    while(j < 5) {
      gfx->setCursor(10, 30+((j+1) * y_offset_tft));
      if (j == pos) {
        if (wert_old != String(glaeser[j].Gewicht)) {
          gfx->fillRect(0, 27+((j+1) * y_offset_tft)-19, 100, 27, BACKGROUND);
          wert_old = String(glaeser[j].Gewicht);
        }
        gfx->setTextColor(MARKER);
      }
      sprintf(ausgabe, "%4dg", glaeser[j].Gewicht);
      gfx->print(ausgabe);
      gfx->setTextColor(TEXT);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      if (GlasTypArray[glaeser[j].GlasTyp] == "DIB") {
        gfx->print("DE Imker Bund");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "TOF") {
        gfx->print("TwistOff");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "DEE") {
        gfx->print("DeepTwist");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "SPZ") {
        gfx->print("Spezial");
      }
      j++;
    }
    if (EncoderButtonTapped()) { // Gewicht bestätigt
      i = 0;
    }
  }
  i = 1;
  initRotaries(SW_MENU, glaeser[pos].GlasTyp, 0, sizeof(GlasTypArray)/sizeof(GlasTypArray[0]) - 1, 1);
  while (i > 0){ 
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    glaeser[pos].GlasTyp = getRotariesValue(SW_MENU);
    j = 0;
    while(j < 5) {
      gfx->setCursor(10, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg", glaeser[j].Gewicht);
      gfx->print(ausgabe);
      if (j == pos) {
        if (wert_old != String(GlasTypArray[glaeser[j].GlasTyp])) {
          gfx->fillRect(100, 27+((j+1) * y_offset_tft)-19, 220, 27, BACKGROUND);
          wert_old = String(GlasTypArray[glaeser[j].GlasTyp]);
        }
        gfx->setTextColor(MARKER);
      }
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      if (GlasTypArray[glaeser[j].GlasTyp] == "DIB") {
        gfx->print("DE Imker Bund");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "TOF") {
        gfx->print("TwistOff");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "DEE") {
        gfx->print("DeepTwist");
      }
      else if (GlasTypArray[glaeser[j].GlasTyp] == "SPZ") {
        gfx->print("Spezial");
      }
      gfx->setTextColor(TEXT);
      j++;
    }
    if (EncoderButtonTapped()) { //GlasTyp bestätigt
      while(EncoderButtonTapped()) {
        delay(1);
      }
      i = 0;
    }
  }
  fmenge = glaeser[pos].Gewicht;
  tara   = glaeser[pos].Tara;
  fmenge_index = pos; 
  modus = -1;
  i = 0;
}

void setupParameter(void) {
  int menuitem;
  int menuitem_used = 4;
  int lastbuzzer    = buzzermode;
  int lastled       = ledmode;
  int lastlogo      = showlogo;
  int lastcredits   = showcredits;
  int lastcolor_scheme = color_scheme;
  int lastcolor_marker = color_marker;
  bool wert_aendern = false;
  int y_offset_tft = 28;
  int wert_old = -1;
  bool change = false;
  bool change_scheme = true;
  bool change_marker = true;
  const char title[] = "Parameter";
  menuitem_used = 6;
  int MenuepunkteAnzahl = 7;
  const char *menuepunkte[MenuepunkteAnzahl] = {"Buzzer", "LED", "Show Logo", "Show Credits", "Color Scheme", "Color Marker", "Speichern"};
  initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      buzzermode = lastbuzzer;
      ledmode = lastled;
      showlogo = lastlogo;
      showcredits = lastcredits;
      color_scheme = lastcolor_scheme;
      color_marker = lastcolor_marker;
      modus = -1;
      return;
    }
    if (wert_aendern == false) {
      menuitem = getRotariesValue(SW_MENU);
      pos = menuitem;
      if (menuitem == menuitem_used) {
        menuitem = 6;
      }
    }
    else {
      switch (menuitem) {
        case 0: buzzermode    = getRotariesValue(SW_MENU);
                break;
        case 1: ledmode       = getRotariesValue(SW_MENU);
                break;
        case 2: showlogo      = getRotariesValue(SW_MENU);
                break;
        case 3: showcredits   = getRotariesValue(SW_MENU);
                break;
        case 4: color_scheme  = getRotariesValue(SW_MENU);
                break;
        case 5: color_marker  = getRotariesValue(SW_MENU);
                break;
      }
    }
    // Menu
    if(change_scheme) {
      gfx->fillScreen(BACKGROUND);
      gfx->setTextColor(TEXT);
      gfx->setFont(Punk_Mono_Bold_240_150);
      int x_pos = CenterPosX(title, 14, 320);
      gfx->setCursor(x_pos, 25);
      gfx->println(title);
      gfx->drawLine(0, 30, 320, 30, TEXT);
      change_scheme = false;
      change_marker = true;
    }
    int j = 0;
    while(j < MenuepunkteAnzahl) {
      gfx->setTextColor(TEXT);
      if (j == pos and wert_aendern == false) {
        gfx->setTextColor(MARKER);
      }
      if (j < MenuepunkteAnzahl - 1) {
        gfx->setCursor(5, 30+((j+1) * y_offset_tft));
        gfx->print(menuepunkte[j]);
        if (j == pos and wert_aendern == true) {
          gfx->setTextColor(MARKER);
        }
        switch (j) {
          case 0: sprintf(ausgabe,"%s", buzzermode==false?"aus":"ein");
                  if (wert_old != buzzermode and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = buzzermode;
                  }
                  break;
          case 1: sprintf(ausgabe,"%s", ledmode==false?"aus":"ein");
                  if (wert_old != ledmode and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = ledmode;
                  }
                  break;
          case 2: sprintf(ausgabe,"%s", showlogo==false?"aus":"ein");
                  if (wert_old != showlogo and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = showlogo;
                  }
                  break;
          case 3: sprintf(ausgabe,"%s", showcredits==false?"aus":"ein");
                  if (wert_old != showcredits and wert_aendern == true and j == pos) {
                    change = true;
                    wert_old = showcredits;
                  }
                  break;
          case 4: sprintf(ausgabe,"%s", color_scheme==false?"dunkel":"hell");
                  if (wert_old != color_scheme and wert_aendern == true and j == pos) {
                    change = true;
                    change_scheme = true;
                    wert_old = color_scheme;
                    tft_colors();
                  }
                  break;
          case 5: sprintf(ausgabe,"");
                  if (wert_old != color_marker and wert_aendern == true and j == pos) {
                    change = true;
                    change_marker = true;
                    wert_old = color_marker;
                    tft_marker();
                  }
                  break;
        }
        if (change) {
          gfx->fillRect(211, 27+((j+1) * y_offset_tft)-19, 104, 25, BACKGROUND);
          change = false;
        }
        if (change_marker) {
          gfx->fillRect(265, 27+(6 * y_offset_tft)-17, 45, 21, MARKER);
          change_marker = false;
        }
        int y = get_length(ausgabe);
        gfx->setCursor(320 - y * 14 - 5, 30+((j+1) * y_offset_tft));
        gfx->print(ausgabe);
      }
      else {
        gfx->setCursor(5, 30+(7 * y_offset_tft));
        gfx->print(menuepunkte[j]);
      }
      j++;
    }
    // Menupunkt zum Ändern ausgewählt
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) {
      while(EncoderButtonPressed()) {
        delay(1);
      } 
      switch (menuitem) { 
        case 0: initRotaries(SW_MENU, buzzermode, 0, 1, 1);
                break;
        case 1: initRotaries(SW_MENU, ledmode, 0, 1, 1);
                break;
        case 2: initRotaries(SW_MENU, showlogo, 0, 1, 1);
                break;
        case 3: initRotaries(SW_MENU, showcredits, 0, 1, 1);
                break;
        case 4: initRotaries(SW_MENU, color_scheme, 0, 1, 1);
                break;
        case 5: initRotaries(SW_MENU, color_marker, 0, 11, 1);
                break;
      }
      wert_old = -1;
      wert_aendern = true;
    }
    // Änderung im Menupunkt übernehmen
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == true) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
    }
    // Menu verlassen 
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      gfx->setCursor(283, 30+(7 * y_offset_tft));
      gfx->print("OK");
      delay(1000);
      modus = -1;
      i = 0;
    }
  }
}

void setupClearPrefs(void) {
  initRotaries(SW_MENU, 2, 0, 2, -1);
  int x_pos;
  int y_offset_tft = 28;
  gfx->fillScreen(BACKGROUND);
  const char title[] = "Clear Preferences";
  int MenuepunkteAnzahl = 3;
  const char *menuepunkte[MenuepunkteAnzahl - 1] = {"Lösche Preferences", "Lösche NVS memory", "Zurück"};
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH  or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      modus = -1;
      return;
    }
    pos = getRotariesValue(SW_MENU);
    int j = 0;
    while(j < MenuepunkteAnzahl) {
      if (j == pos) {
        gfx->setTextColor(MARKER);
      }
      else {
        gfx->setTextColor(TEXT);
      }
      if (j < MenuepunkteAnzahl - 1) {
        gfx->setCursor(10, 30+((j+1) * y_offset_tft));
        gfx->print(menuepunkte[j]);
      }
      else {
        gfx->setCursor(10, 30+(7 * y_offset_tft));
        gfx->print(menuepunkte[j]);
      }
      j++;
    }
    if (EncoderButtonTapped()) {  
      if (pos == 0) {
        preferences.begin("EEPROM", false);
        preferences.clear();
        preferences.end();
        //Da machen wir gerade einen restart
        ESP.restart();
      }
      else if (pos == 1) {
        nvs_flash_erase();    // erase the NVS partition and...
        nvs_flash_init();     // initialize the NVS partition.
        //Da machen wir gerade einen restart
        ESP.restart();
      }
      gfx->setCursor(280, 30+(7 * y_offset_tft));
      gfx->print("OK");
      delay(1000);
      modus = -1;
      i = 0;
    }
  }
}

void setupINA219(void) {                            //Funktioniert nur wenn beide Menüs die gleiche Anzahl haben. Feel free to change it :-)
  int menuitem;
  int lastcurrent             = current_servo;
  int lastwinkel_min          = winkel_min;
  int lastshow_current        = show_current;
  bool wert_aendern = false;
  int menuitem_used           = 2;
  String kalibration_status   = "Start";
  String quetschhan           = "zu";
  bool cal_done = false;
  int cal_winkel = 0;
  int j = 0;
  int k = 0;
  int y_offset_tft = 28;
  bool change = false;
  int wert_old = -1;
  const char title[] = "INA219 Setup";
  int MenuepunkteAnzahl = 3;
  const char *menuepunkte_1[MenuepunkteAnzahl] = {"Servo Strom", "Cal. Quetschhan", "Speichern"};
  const char *menuepunkte_2[MenuepunkteAnzahl] = {"Servo Strom", "Zeige Strom an", "Speichern"};
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, TEXT);
  initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
  i = 1;
  while (i > 0) {
    if (digitalRead(button_stop_pin) == HIGH  or digitalRead(switch_setup_pin) == LOW) {
      while (digitalRead(button_stop_pin) == HIGH) {
        delay(1);
      }
      current_servo = lastcurrent;
      winkel_min = lastwinkel_min;
      show_current = lastshow_current;
      modus = -1;
      return;
    }
    if (wert_aendern == false) {
      menuitem = getRotariesValue(SW_MENU);
      pos = menuitem;
      if (menuitem == menuitem_used) {
        menuitem = 6;
      }
    }
    else {
      switch (menuitem) {
        case 0: current_servo         = getRotariesValue(SW_MENU);
                break;
        case 1: if (current_servo == 0) {
                  show_current = getRotariesValue(SW_MENU);
                }
                else {
                  j                     = 1;
                  kalibration_status    = "Start";
                  wert_aendern          = false;
                  menuitem_used         = 1;
                  setRotariesValue(SW_MENU, 0);
                  initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
                }
                break;
      }
    }
    // Menu
    int a = 0;
    while(a < MenuepunkteAnzahl) {
      gfx->setTextColor(TEXT);
      if (a == pos and wert_aendern == false) {
        gfx->setTextColor(MARKER);
      }
      if (a < MenuepunkteAnzahl - 1) {
        if (current_servo == 0) {
          gfx->setCursor(5, 30+((a+1) * y_offset_tft));
          gfx->print(menuepunkte_2[a]);
        }
        else {
          gfx->setCursor(5, 30+((a+1) * y_offset_tft));
          gfx->print(menuepunkte_1[a]);
        }
        if (a == pos and wert_aendern == true) {
          gfx->setTextColor(MARKER);
        }
        switch (a) {
          case 0: sprintf(ausgabe,"%dmA", current_servo);
                  if (wert_old != current_servo and wert_aendern == true and a == pos) {
                    change = true;
                    wert_old = current_servo;
                  }
                  break;
          case 1: if (current_servo == 0) {
                    sprintf(ausgabe,"%s", show_current==false?"aus":"ein");
                    if (wert_old != show_current and wert_aendern == true and a == pos) {
                      change = true;
                      wert_old = show_current;
                    }
                  }
                  else {
                    sprintf(ausgabe,"%s", "");
                    if (wert_old != show_current and wert_aendern == true and a == pos) {
                      change = true;
                      wert_old = show_current;
                    }
                  }
                  break;
        }
        if (change) {
          if (a == 0) {
            gfx->fillRect(215, 27+((a+1) * y_offset_tft)-19, 100, 25, BACKGROUND);
            if (wert_old == 0 or wert_old == 50) {                                                //nicht ganz sauber bei den Schwellwerten. 
              gfx->fillRect(0, 27+((a+2) * y_offset_tft)-19, 320, 27, BACKGROUND);
            }
          }
          else {
            gfx->fillRect(251, 27+((a+1) * y_offset_tft)-19, 64, 25, BACKGROUND);
          }
          change = false;
        }
        int y = get_length(ausgabe);
        gfx->setCursor(320 - y * 14 - 5, 30+((a+1) * y_offset_tft));
        gfx->print(ausgabe);
      }
      else {
        gfx->setCursor(5, 30+(7 * y_offset_tft));
        gfx->print(menuepunkte_2[a]);
      }
      a++;
    }
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) {
      while(EncoderButtonPressed()) {
        delay(1);
      } 
      switch (menuitem) { 
        case 0: initRotaries(SW_MENU, current_servo, 0, 1500, 50);
                break;
        case 1: if (current_servo == 0) {initRotaries(SW_MENU, show_current, 0, 1, 1);}
                break;
      }
      wert_aendern = true;
    }
    // Änderung im Menupunkt übernehmen
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == true) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
    }
    // Menu verlassen 
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed()) {
        delay(1);
      }
      gfx->setCursor(283, 30+(7 * y_offset_tft));
      gfx->print("OK");
      delay(1000);
      modus = -1;
      i = 0;
    }
    while (j > 0) {
      if (digitalRead(switch_setup_pin) == LOW) {
        current_servo = lastcurrent;
        show_current = lastshow_current;
        modus = -1;
        return;
      }
      if (wert_aendern == false) {
        menuitem = getRotariesValue(SW_MENU);
        if (menuitem == menuitem_used) {
          menuitem = 6;
        }
      }
      menuitem_used = 1;
      if (j == 1) {
        gfx->fillScreen(BACKGROUND);
        gfx->setTextColor(TEXT);
        gfx->setFont(Punk_Mono_Bold_240_150);
        x_pos = CenterPosX(title, 14, 320);
        gfx->setCursor(x_pos, 25);
        gfx->println(title);
        gfx->drawLine(0, 30, 320, 30, TEXT);
        gfx->setCursor(5, 30+(3 * y_offset_tft));
        sprintf(ausgabe,"max. Strom:     %4imA", current_servo - 30);
        gfx->print(ausgabe);
        gfx->setCursor(5, 30+(4 * y_offset_tft));
        sprintf(ausgabe,"min. Winkel:      %3i°", winkel_min);
        gfx->print(ausgabe);
        j++;
      }
      if (wert_aendern == false && menuitem < menuitem_used && kalibration_status == "Start") {
        gfx->setTextColor(MARKER);
        gfx->setCursor(5, 30+(1 * y_offset_tft));
        gfx->print(kalibration_status);
        gfx->setTextColor(TEXT);
        gfx->setCursor(5, 30+(7 * y_offset_tft));
        gfx->print("Zurück");
      }
      else {
        gfx->setTextColor(TEXT);
        gfx->setCursor(5, 30+(1 * y_offset_tft));
        gfx->print(kalibration_status);
        gfx->setTextColor(MARKER);
        gfx->setCursor(5, 30+(7 * y_offset_tft));
        gfx->print("Zurück");
      }
      if (wert_aendern == true && menuitem < menuitem_used) {
        lastcurrent = current_servo;
        kalibration_status = "Kalibration läuft";
        quetschhan = "zu";
        cal_done = false;
        cal_winkel = 0;
        k = 1;
      }
      if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) {
        while(EncoderButtonPressed()) {
          delay(1);
        } 
        wert_aendern = true;
      }
      // Änderung im Menupunkt übernehmen
      if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == true) {
        while(EncoderButtonPressed()) {
          delay(1);
        }
        initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
        wert_aendern = false;
      }
      //verlassen
      if ((EncoderButtonPressed() && menuitem == 6) or digitalRead(button_stop_pin) == HIGH) {
        while(EncoderButtonPressed() or digitalRead(button_stop_pin) == HIGH) {
          delay(1);
        }
        gfx->fillScreen(BACKGROUND);
        x_pos = CenterPosX(title, 14, 320);
        gfx->setCursor(x_pos, 25);
        gfx->println(title);
        gfx->drawLine(0, 30, 320, 30, TEXT);
        j = 0;
        wert_aendern = false;
        menuitem_used = 2;
        initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
      }
      while (k > 0) {
        SERVO_WRITE(90);
        quetschhan = "offen";
        int scaletime = millis();
        bool measurement_run = false;
        while (!cal_done) {
          if (k == 1) {
            gfx->fillScreen(BACKGROUND);
            gfx->setTextColor(TEXT);
            gfx->setFont(Punk_Mono_Bold_240_150);
            x_pos = CenterPosX(title, 14, 320);
            gfx->setCursor(x_pos, 25);
            gfx->println(title);
            gfx->drawLine(0, 30, 320, 30, TEXT);
            gfx->setCursor(5, 30+(3 * y_offset_tft));
            change = true;
            k++;
          }
          if (change) {
            gfx->setTextColor(TEXT);
            gfx->setCursor(5, 30+(1 * y_offset_tft));
            gfx->print(kalibration_status);
            gfx->fillRect(205, 27+(3 * y_offset_tft)-19, 110, 3 * y_offset_tft, BACKGROUND);
            gfx->setCursor(5, 30+(3 * y_offset_tft));
            sprintf(ausgabe,"Strom:          %4imA", current_mA);
            gfx->print(ausgabe);
            gfx->setCursor(5, 30+(4 * y_offset_tft));
            sprintf(ausgabe,"Winkel:           %3i°", cal_winkel);
            gfx->print(ausgabe);
            gfx->setCursor(5, 30+(5 * y_offset_tft));
            sprintf(ausgabe,"Quetschhan:      %5s", quetschhan);
            gfx->print(ausgabe);
            gfx->setTextColor(MARKER);
            gfx->setCursor(5, 30+(7 * y_offset_tft));
            gfx->print("Abbrechen");
            change = false;
          }
          if (millis() - scaletime >= 800 and !measurement_run) {
            SERVO_WRITE(cal_winkel);
            quetschhan = "zu";
            measurement_run = true;
            scaletime = millis();
            change = true;
          }
          else if (millis() - scaletime >= 800 and measurement_run) {
            current_mA = GetCurrent(50);
            if (current_mA > current_servo - 30) {   //30mA unter dem max. Wert Kalibrieren
              SERVO_WRITE(90);
              quetschhan = "offen";
              cal_winkel++;
              change = true;
            }
            else {
              cal_done = true;
              kalibration_status = "Kalibration beendet";
              winkel_min = cal_winkel;
              k++;
            }
            measurement_run = false;
            scaletime = millis();
          }
          //verlassen
          if (EncoderButtonPressed() or digitalRead(button_stop_pin) == HIGH) {
            while(EncoderButtonPressed() or digitalRead(button_stop_pin) == HIGH) {
              delay(1);
            }
            gfx->fillScreen(BACKGROUND);
            gfx->setTextColor(TEXT);
            x_pos = CenterPosX(title, 14, 320);
            gfx->setCursor(x_pos, 25);
            gfx->println(title);
            gfx->drawLine(0, 30, 320, 30, TEXT);
            j = 0;
            k = 0;
            winkel_min = lastwinkel_min;
            cal_done = true;
            wert_aendern = false;
            menuitem_used = 2;
            initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
          }
        }
        while (cal_done and k > 1) {
          current_mA = GetCurrent(50);
          if (wert_old != current_mA) {
            change = true;
            wert_old = current_mA;
          }
          if (k==2) {
            gfx->setTextColor(TEXT);
            gfx->fillRect(0, 27+(1 * y_offset_tft)-19, 320, 25, BACKGROUND);
            gfx->fillRect(0, 27+(5 * y_offset_tft)-19, 320, 25, BACKGROUND);
            gfx->fillRect(0, 27+(7 * y_offset_tft)-19, 320, 25, BACKGROUND);
            gfx->setCursor(5, 30+(1 * y_offset_tft));
            gfx->print(kalibration_status);
            gfx->setTextColor(MARKER);
            gfx->setCursor(5, 30+(7 * y_offset_tft));
            gfx->print("Speichern");
            gfx->setTextColor(TEXT);
            k++;
          }
          if (change) {
            gfx->fillRect(200, 27+(3 * y_offset_tft)-19, 120, 25, BACKGROUND);
            gfx->setCursor(5, 30+(3 * y_offset_tft));
            sprintf(ausgabe,"Strom:          %4imA", current_mA);
            gfx->print(ausgabe);
            change = false;
          }
          //verlassen
          if (digitalRead(button_stop_pin) == HIGH) {
            while(digitalRead(button_stop_pin) == HIGH) {
              delay(1);
            }
            j = 0;
            k = 0;
            wert_aendern = false;
            menuitem_used = 2;
            winkel_min = lastwinkel_min;
            initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
            gfx->fillScreen(BACKGROUND);
            x_pos = CenterPosX(title, 14, 320);
            gfx->setCursor(x_pos, 25);
            gfx->println(title);
            gfx->drawLine(0, 30, 320, 30, TEXT);
          }
          if (EncoderButtonTapped()) {
            j = 0;
            k = 0;
            modus = -1;
            wert_aendern = false;
            menuitem_used = 2;
            initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
            setPreferences();
            return;
          }
        }
      }
    }
  }
}

void processSetup(void) {
  int x_pos;
  int MenuepunkteAnzahl = 9;
  int menuitem_old = -1;
  if (ina219_installed) {MenuepunkteAnzahl++;}
  int posmenu[MenuepunkteAnzahl];
  const char *menuepunkte[MenuepunkteAnzahl] = {"Tarawerte","Kalibrieren","Füllmenge","Automatik","Servoeinst.","Parameter","Zählwerk","ZählwerkTrip","Clear Prefs"};
  //MenuepunkteAnzahl = sizeof(menuepunkte)/sizeof(menuepunkte[0]);
  if (ina219_installed) {
      menuepunkte[MenuepunkteAnzahl - 1] = menuepunkte[MenuepunkteAnzahl -2];    //Clear Pref eins nach hinten schieben
      menuepunkte[MenuepunkteAnzahl - 2] = "INA219 Setup";
  }
  modus = MODE_SETUP;
  winkel = winkel_min;          // Hahn schliessen
  servo_aktiv = 0;              // Servo-Betrieb aus
  SERVO_WRITE(winkel);
  rotary_select = SW_MENU;
  initRotaries(SW_MENU, lastpos, -1, MenuepunkteAnzahl, 1);
  while (modus == MODE_SETUP and (digitalRead(switch_setup_pin)) == HIGH) {
    if (rotaries[SW_MENU].Value < 0) {
      rotaries[SW_MENU].Value = (MenuepunkteAnzahl * ROTARY_SCALE) - 1;
    }
    else if (rotaries[SW_MENU].Value > ((MenuepunkteAnzahl * ROTARY_SCALE) - 1)) {
      rotaries[SW_MENU].Value = 0;
    }
    int menuitem = getRotariesValue(SW_MENU) % MenuepunkteAnzahl;
    for (i = 0; i < MenuepunkteAnzahl; i++) {
      posmenu[i] = (menuitem + i) % MenuepunkteAnzahl;
    }
    if (menuitem != menuitem_old) {
      gfx->fillScreen(BACKGROUND);
      gfx->drawLine(0, 100, 320, 100, TEXT);
      gfx->drawLine(0, 139, 320, 139, TEXT);
      gfx->setTextColor(TEXT);
      gfx->setFont(Punk_Mono_Bold_320_200);
      x_pos = CenterPosX(menuepunkte[posmenu[0]], 18, 320);
      gfx->setCursor(x_pos, 130);
      gfx->println(menuepunkte[posmenu[0]]);
      gfx->setTextColor(MENU_POS1);
      gfx->setFont(Punk_Mono_Bold_240_150);
      x_pos = CenterPosX(menuepunkte[posmenu[MenuepunkteAnzahl-1]], 14, 320);
      gfx->setCursor(x_pos, 86);
      gfx->println(menuepunkte[posmenu[MenuepunkteAnzahl-1]]);
      x_pos = CenterPosX(menuepunkte[posmenu[1]], 14, 320);
      gfx->setCursor(x_pos, 169);
      gfx->println(menuepunkte[posmenu[1]]);
      gfx->setTextColor(MENU_POS2);
      gfx->setFont(Punk_Mono_Bold_200_125);                   //9 x 11 Pixel
      x_pos = CenterPosX(menuepunkte[posmenu[MenuepunkteAnzahl-2]], 12, 320);
      gfx->setCursor(x_pos, 60);
      gfx->println(menuepunkte[posmenu[MenuepunkteAnzahl-2]]);
      x_pos = CenterPosX(menuepunkte[posmenu[2]], 12, 320);
      gfx->setCursor(x_pos, 192);
      gfx->println(menuepunkte[posmenu[2]]);
      gfx->setTextColor(MENU_POS3);
      gfx->setFont(Punk_Mono_Bold_160_100);                   //7 x 9 Pixel
      x_pos = CenterPosX(menuepunkte[posmenu[MenuepunkteAnzahl-3]], 9, 320);
      gfx->setCursor(x_pos, 37);
      gfx->println(menuepunkte[posmenu[MenuepunkteAnzahl-3]]);    
      x_pos = CenterPosX(menuepunkte[posmenu[3]], 9, 320);
      gfx->setCursor(x_pos, 213);
      gfx->println(menuepunkte[posmenu[3]]);
      gfx->setTextColor(MENU_POS4);
      gfx->setFont(Punk_Mono_Bold_120_075);                   //6 x 7 Pixel
      x_pos = CenterPosX(menuepunkte[posmenu[MenuepunkteAnzahl-4]], 7, 320);
      gfx->setCursor(x_pos, 16);
      gfx->println(menuepunkte[posmenu[MenuepunkteAnzahl-4]]);
      x_pos = CenterPosX(menuepunkte[posmenu[4]], 7, 320);
      gfx->setCursor(x_pos, 232);
      gfx->println(menuepunkte[posmenu[4]]);
      menuitem_old = menuitem;
    }
    lastpos = menuitem;
    if (EncoderButtonTapped()) {
    #ifdef isDebug 
      Serial.print("Setup Position: ");
      Serial.println(menuitem);
    #endif
      lastpos = menuitem;
      if (menuepunkte[menuitem] == "Tarawerte")     setupTara();              // Tara 
      if (menuepunkte[menuitem] == "Kalibrieren")   setupCalibration();       // Kalibrieren 
      if (menuepunkte[menuitem] == "Füllmenge")     setupFuellmenge();        // Füllmenge 
      if (menuepunkte[menuitem] == "Automatik")     setupAutomatik();         // Autostart/Autokorrektur konfigurieren 
      if (menuepunkte[menuitem] == "Servoeinst.")   setupServoWinkel();       // Servostellungen Minimum, Maximum und Feindosierung
      if (menuepunkte[menuitem] == "Parameter")     setupParameter();         // Sonstige Einstellungen
      if (menuepunkte[menuitem] == "Zählwerk")      setupCounter();           // Zählwerk
      if (menuepunkte[menuitem] == "ZählwerkTrip")  setupTripCounter();       // Zählwerk Trip
      if (menuepunkte[menuitem] == "INA219 Setup")  setupINA219();            // INA219 Setup
      setPreferences();
      if (menuepunkte[menuitem] == "Clear Prefs")   setupClearPrefs();        // EEPROM löschen
      initRotaries(SW_MENU,lastpos, 0,255, -1);                               // Menu-Parameter könnten verstellt worden sein
    }
  }
}

void processAutomatik(void) {
  int zielgewicht;                 // Glas + Korrektur
  static int autokorrektur_gr = 0; 
  int erzwinge_servo_aktiv = 0;
  boolean voll = false;
  static int gewicht_vorher;       // Gewicht des vorher gefüllten Glases
  static int sammler_num = 5;      // Anzahl identischer Messungen für Nachtropfen
  int n;
  int y_offset = 0;
  if (modus != MODE_AUTOMATIK) {
     modus = MODE_AUTOMATIK;
     winkel = winkel_min;          // Hahn schliessen
     servo_aktiv = 0;              // Servo-Betrieb aus
     SERVO_WRITE(winkel);
     auto_aktiv = 0;               // automatische Füllung starten
     tara_glas = 0;
     rotary_select = SW_WINKEL;    // Einstellung für Winkel über Rotary
     offset_winkel = 0;            // Offset vom Winkel wird auf 0 gestellt
     initRotaries(SW_MENU, fmenge_index, 0, 4, 1);
     gewicht_vorher = glaeser[fmenge_index].Gewicht + korrektur;
     int x_pos;
    if (current_servo > 0) {
      no_ina = true;
    }
    else {
      no_ina = false;
    }
    glas_alt = -1;
    current_mA_alt = -1;
    korr_alt = -99999;
    autokorr_gr_alt = -99999;
    winkel_min_alt = -1;
    pos_alt = -1;
    winkel_ist_alt = -1;
    servo_aktiv_alt = -1;
    auto_aktiv_alt = -1;
    gewicht_alt = -9999999;
    gfx->fillScreen(BACKGROUND);
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Bold_320_200);
    sprintf(ausgabe,"Automatik");
    x_pos = CenterPosX(ausgabe, 18, 320);
    gfx->setCursor(x_pos, 27);
    gfx->print(ausgabe);
    gfx->drawLine(0, 30, 320, 30, TEXT);
    gfx->drawLine(0, 184, 320, 184, TEXT);
    gfx->drawLine(160, 184, 160, 240, TEXT);
    gfx->setCursor(2, 202);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->print("INA219");
    gfx->setFont(Checkbox);
    gfx->setCursor(140, 199);
    if (ina219_installed == 0){
      gfx->setTextColor(RED);
      gfx->print("B");
    }
    else {
      gfx->setTextColor(GREEN);
      gfx->print("A");
    }
    gfx->setTextColor(TEXT);
    gfx->setCursor(2, 219);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->print("Autostart");
    gfx->setFont(Checkbox);
    gfx->setCursor(140, 216);
    if (autostart == 0){
      gfx->setTextColor(RED);
      gfx->print("B");
    }
    else {
      gfx->setTextColor(GREEN);
      gfx->print("A");
    }
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(2, 236);
    gfx->print("Autokorrektur");
    gfx->setFont(Checkbox);
    gfx->setCursor(140, 233);
    if (autokorrektur == 0){
      gfx->setTextColor(RED);
      gfx->print("B");
    }
    else {
      gfx->setTextColor(GREEN);
      gfx->print("A");
    }
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(170, 202);
    gfx->print("Strom:");
    gfx->setCursor(170, 219);
    gfx->print("Korrektur:");
    gfx->setCursor(170, 236);
    gfx->print("Autokorr.:");
    gfx->setCursor(2, 47);
    //sprintf(ausgabe,"min:000°     max:000°      ist:000°", (winkel_min, winkel_max*pos/100));
    sprintf(ausgabe,"Min:         Max:         Ist:");
    gfx->print(ausgabe);
    gfx->drawLine(0, 50, 320, 50, TEXT);
  }
  pos = getRotariesValue(SW_WINKEL);
  // nur bis winkel_fein regeln, oder über initRotaries lösen?
  if (pos < winkel_fein * 100 / winkel_max) {                      
    pos = winkel_fein * 100 / winkel_max;
    setRotariesValue(SW_WINKEL, pos);
  }
  korrektur    = getRotariesValue(SW_KORREKTUR);
  fmenge_index = getRotariesValue(SW_MENU);
  fmenge       = glaeser[fmenge_index].Gewicht;
  tara         = glaeser[fmenge_index].Tara;
  if (tara <= 0) { 
    auto_aktiv = 0;
  }
  // wir starten nur, wenn das Tara für die Füllmenge gesetzt ist!
  // Ein erneuter Druck auf Start erzwingt die Aktivierung des Servo
  if (digitalRead(button_start_pin) == HIGH && tara > 0) {
    while(digitalRead(button_start_pin) == HIGH) {
       delay(1);
    }
    if (auto_aktiv == 1) {
      erzwinge_servo_aktiv = 1;
      #ifdef isDebug
        Serial.println("erzwinge Servo aktiv");      
      #endif
    }
    auto_aktiv    = 1;                            // automatisches Füllen aktivieren
    rotary_select = SW_WINKEL;                    // falls während der Parameter-Änderung auf Start gedrückt wurde    
    setPreferences();                             // falls Parameter über den Rotary verändert wurden
    glas_alt = -1;                              // Glas Typ Farbe zurücksetzen fals markiert ist
    korr_alt = -99999;                          // Korrektur Farbe zurücksetzen fals markiert ist
  }
  if (digitalRead(button_stop_pin) == HIGH) {
    winkel      = winkel_min + offset_winkel;
    servo_aktiv = 0;
    auto_aktiv  = 0;
    tara_glas   = 0;
  }
// Fehlerkorrektur der Waage, falls Gewicht zu sehr schwankt 
  #ifdef FEHLERKORREKTUR_WAAGE
    int Vergleichsgewicht = (int(SCALE_GETUNITS(SCALE_READS))) - tara;
    for (byte j = 0 ; j < 3; j++) { // Anzahl der Wiederholungen, wenn Abweichung zu hoch
      gewicht = int(SCALE_GETUNITS(SCALE_READS)) - tara;
      if (abs(gewicht - Vergleichsgewicht) < 50)  // Abweichung für Fehlererkennung
        break; 
      delay(100);
    }
  #else
    gewicht = int(SCALE_GETUNITS(SCALE_READS)) - tara;
  #endif 
  // Glas entfernt -> Servo schliessen
  if (gewicht < -20) {
    winkel      = winkel_min + offset_winkel;
    servo_aktiv = 0;
    tara_glas   = 0;
    if (autostart != 1) {  // Autostart nicht aktiv
      auto_aktiv  = 0;
    }
  }
  // Automatik ein, leeres Glas aufgesetzt, Servo aus -> Glas füllen
  if (auto_aktiv == 1 && abs(gewicht) <= glastoleranz && servo_aktiv == 0) {
    rotary_select = SW_WINKEL;     // falls während der Parameter-Änderung ein Glas aufgesetzt wird
    // START: nicht schön aber es funktioniert (vieleicht mit Subprogrammen arbeiten damit es besser wird)
    //PLAY ICON setzen
    gfx->fillRect(12, 74, 38, 38, BACKGROUND);
    gfx->setFont(Icons_Start_Stop);
    gfx->setCursor(5, 118);
    gfx->print("M");
    gfx->setCursor(5, 118);
    gfx->setTextColor(GREEN);
    gfx->print("A");
    //Korrektur Wert auf Textfarbe setzen
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 219);
    sprintf(ausgabe, "%5ig", korrektur + autokorrektur_gr);
    gfx->print(ausgabe);
    gfx->setTextColor(TEXT);
    //Glasgewicht auf Textfarbe Setzen
    gfx->setCursor(2, 176);
    gfx->fillRect(0, 156, 320, 27, BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_240_150);
    sprintf(ausgabe, "%dg ", (glaeser[fmenge_index].Gewicht));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_120_075);
    gfx->setCursor(2 + 14*StringLenght(ausgabe), 168);
    sprintf(ausgabe, "+%dg ", (kulanz_gr));
    gfx->print(ausgabe);
    // END:   nicht schön aber es funktioniert
    gfx->fillRect(80, 54, 240, 80, BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_600_375);
    gfx->setCursor(100, 115);
    gfx->print("START");
    // kurz warten und prüfen ob das Gewicht nicht nur eine zufällige Schwankung war 
    delay(1500);  
    gewicht = int(SCALE_GETUNITS(SCALE_READS)) - tara;
    if (abs(gewicht) <= glastoleranz) {
      tara_glas   = gewicht;
      gfx->fillRect(109, 156, 211, 27, BACKGROUND);
      gfx->setFont(Punk_Mono_Thin_240_150);
      sprintf(ausgabe, "Tara Glas: %dg ", (tara_glas));
      int sl = StringLenght(ausgabe);
      gfx->setCursor(320 - 14 * sl, 176);
      gfx->print(ausgabe);
      #ifdef isDebug 
        Serial.print("gewicht: ");            Serial.print(gewicht);
        Serial.print(" gewicht_vorher: ");    Serial.print(gewicht_vorher);
        Serial.print(" zielgewicht: ");       Serial.print(fmenge + korrektur + tara_glas + autokorrektur_gr);
        Serial.print(" kulanz: ");            Serial.print(kulanz_gr);
        Serial.print(" Autokorrektur: ");     Serial.println(autokorrektur_gr);
      #endif      
      servo_aktiv = 1;
      sammler_num = 0;
      voll = false;
      gezaehlt = false;
      gewicht_alt = -9999999;               //Damit die Gewichtsanzeige aktiviert wird
      buzzer(BUZZER_SHORT);
    }
  }
  zielgewicht = fmenge + korrektur + tara_glas + autokorrektur_gr;
  // Anpassung des Autokorrektur-Werts
  if (autokorrektur == 1) {                                                       
    if ( auto_aktiv == 1 && servo_aktiv == 0 && winkel == winkel_min + offset_winkel && gewicht >= zielgewicht && sammler_num <= 5) {     
      voll = true;                       
      if (gewicht == gewicht_vorher && sammler_num < 5) { // wir wollen 5x das identische Gewicht sehen  
        sammler_num++;
      } 
      else if (gewicht != gewicht_vorher) {               // sonst gewichtsänderung nachführen
        gewicht_vorher = gewicht;
        sammler_num = 0;
      } 
      else if (sammler_num == 5) {                        // gewicht ist 5x identisch, autokorrektur bestimmen
        autokorrektur_gr = (fmenge + kulanz_gr + tara_glas) - (gewicht - autokorrektur_gr);
        if (korrektur + autokorrektur_gr > kulanz_gr) {   // Autokorrektur darf nicht überkorrigieren, max Füllmenge plus Kulanz
          autokorrektur_gr = kulanz_gr - korrektur;
          #ifdef isDebug
            Serial.print("Autokorrektur begrenzt auf ");
            Serial.println(autokorrektur_gr);
          #endif
        }
        buzzer(BUZZER_SUCCESS);
        sammler_num++;                                      // Korrekturwert für diesen Durchlauf erreicht
      }
      if (voll == true && gezaehlt == false) {
        glaeser[fmenge_index].TripCount++;
        glaeser[fmenge_index].Count++;
        gezaehlt = true;
      }
      #ifdef isDebug
        Serial.print("Nachtropfen:");
        Serial.print(" gewicht: ");        Serial.print(gewicht);
        Serial.print(" gewicht_vorher: "); Serial.print(gewicht_vorher);
        Serial.print(" sammler_num: ");    Serial.print(sammler_num);
        Serial.print(" Korrektur: ");      Serial.println(autokorrektur_gr);
        Serial.print(" Zähler Trip: ");    Serial.print(glaeser[fmenge_index].TripCount); //Kud
        Serial.print(" Zähler: ");         Serial.println(glaeser[fmenge_index].Count); //Kud
      #endif
    }
  }
  // Glas ist teilweise gefüllt. Start wird über Start-Taster erzwungen
  if (auto_aktiv == 1 && gewicht > 5 && erzwinge_servo_aktiv == 1) {
    servo_aktiv = 1;
    voll = false;
    gezaehlt = false;
    buzzer(BUZZER_SHORT);
  }
  if (servo_aktiv == 1) {
    winkel = (winkel_max * pos / 100);
  }
  if (servo_aktiv == 1 && (zielgewicht - gewicht <= fein_dosier_gewicht)) {
    winkel = ((winkel_max*pos / 100) * ((zielgewicht-gewicht) / fein_dosier_gewicht) );
  }
  if (servo_aktiv == 1 && winkel <= winkel_fein) {
    winkel = winkel_fein;
  }
  // Glas ist voll
  if (servo_aktiv == 1 && gewicht >= zielgewicht) {
    winkel      = winkel_min + offset_winkel;
    servo_aktiv = 0;
    if (gezaehlt == false) {
      glaeser[fmenge_index].TripCount++;
      glaeser[fmenge_index].Count++;
      gezaehlt = true;
    }
    if (autostart != 1)       // autostart ist nicht aktiv, kein weiterer Start
      auto_aktiv = 0;
    if (autokorrektur == 1)   // autokorrektur, gewicht merken
      gewicht_vorher = gewicht;
    buzzer(BUZZER_SHORT);
  }
  SERVO_WRITE(winkel);
  #ifdef isDebug
    #if isDebug >= 4
      Serial.print("Automatik:");  
      Serial.print(" Gewicht: ");        Serial.print(gewicht);
      Serial.print(" Winkel: ");         Serial.print(winkel);
      Serial.print(" Dauer ");           Serial.print(millis() - scaletime);
      Serial.print(" Füllmenge: ");      Serial.print(fmenge);
      Serial.print(" Korrektur: ");      Serial.print(korrektur);
      Serial.print(" Tara_glas:");       Serial.print(tara_glas);
      Serial.print(" Autokorrektur: ");  Serial.print(autokorrektur_gr);
      Serial.print(" Zielgewicht ");     Serial.print(zielgewicht);
      Serial.print(" Erzwinge Servo: "); Serial.print(erzwinge_servo_aktiv);
      Serial.print(" servo_aktiv ");     Serial.print(servo_aktiv);
      Serial.print(" auto_aktiv ");      Serial.println(auto_aktiv);
    #endif 
  #endif
  if (ina219_installed and (current_servo > 0 or show_current == 1)) {
    y_offset = 4;
  }
  if (ina219_installed == 1 and current_mA != current_mA_alt and (current_servo > 0 or show_current == 1)) {
    gfx->fillRect(260, 187, 70, 18, BACKGROUND);
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 202);
    if (current_mA > current_servo and current_servo > 0) {
      gfx->setTextColor(RED);
    }
    sprintf(ausgabe, "%4imA", current_mA);
    gfx->print(ausgabe);
    gfx->setTextColor(TEXT);
    current_mA_alt = current_mA;
  }
  else if (ina219_installed == 1 and no_ina == 0 and show_current == 0) {
    no_ina = true;
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 202);
    gfx->print("   aus");
  }
  else if (ina219_installed == 0 and no_ina == 0) {
    no_ina = true;
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 202);
    gfx->print("    NA");
  }
  if (korr_alt != korrektur + autokorrektur_gr) {
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setTextColor(BACKGROUND);
    gfx->setCursor(265, 219);
    sprintf(ausgabe, "%5ig", korr_alt);
    gfx->print(ausgabe);
    gfx->setTextColor(TEXT);
    if (rotary_select == SW_KORREKTUR and servo_aktiv == 0) {
      gfx->setTextColor(MARKER);
    }
    gfx->setCursor(265, 219);
    sprintf(ausgabe, "%5ig", korrektur + autokorrektur_gr);
    gfx->print(ausgabe);
    gfx->setTextColor(TEXT);
    korr_alt = korrektur + autokorrektur_gr;
  }
  if (autokorr_gr_alt != autokorrektur_gr) {
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setTextColor(BACKGROUND);
    gfx->setCursor(265, 236);
    sprintf(ausgabe, "%5ig", autokorr_gr_alt);
    gfx->print(ausgabe);
    gfx->setTextColor(TEXT);
    gfx->setCursor(265, 236);
    sprintf(ausgabe, "%5ig", autokorrektur_gr);
    gfx->print(ausgabe);
    autokorr_gr_alt = autokorrektur_gr;
  }
  if ((glas_alt != fmenge_index and servo_aktiv == 0 and gewicht <= glaeser[fmenge_index].Gewicht - tara) or (glas_alt != fmenge_index and rotary_select_alt == SW_KORREKTUR)) {
    if (rotary_select == SW_MENU and servo_aktiv == 0) {
      gfx->setTextColor(MARKER);
    }
    gfx->setCursor(2, 176);
    gfx->fillRect(0, 156, 320, 27, BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_240_150);
    sprintf(ausgabe, "%dg ", (glaeser[fmenge_index].Gewicht));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_120_075);
    gfx->setCursor(2 + 14*StringLenght(ausgabe), 168);
    sprintf(ausgabe, "+%dg ", (kulanz_gr));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_240_150);
    gfx->setCursor(110, 176);
    if (GlasTypArray[glaeser[fmenge_index].GlasTyp] == "DIB") {
      gfx->print("DE Imker Bund");
    }
    else if (GlasTypArray[glaeser[fmenge_index].GlasTyp] == "TOF") {
      gfx->print("TwistOff");
    }
    else if (GlasTypArray[glaeser[fmenge_index].GlasTyp] == "DEE") {
      gfx->print("DeepTwist");
    }
    else if (GlasTypArray[glaeser[fmenge_index].GlasTyp] == "SPZ") {
      gfx->print("Spezial");
    }
    gfx->setTextColor(TEXT);
    glas_alt = fmenge_index;
  }
  if (winkel_min  + offset_winkel != winkel_min_alt) {
    gfx->setTextColor(TEXT);
    gfx->fillRect(37, 33, 40, 16, BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(38, 47);
    sprintf(ausgabe, "%3i°", winkel_min + offset_winkel);
    gfx->print(ausgabe);
    winkel_min_alt = winkel_min + offset_winkel;
  }
  if (pos != pos_alt) {
    gfx->fillRect(154, 33, 40, 16, BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(155, 47);
    sprintf(ausgabe, "%3i°", winkel_max*pos/100);
    gfx->print(ausgabe);
    pos_alt = pos;
  }
  if (winkel != winkel_ist_alt) {
    gfx->fillRect(271, 33, 40, 16, BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(272, 47);
    sprintf(ausgabe, "%3i°", winkel);
    gfx->print(ausgabe);
    winkel_ist_alt = winkel;
  }
  if (auto_aktiv != auto_aktiv_alt) {
    gfx->fillRect(12, 74, 38, 38, BACKGROUND);
    gfx->setFont(Icons_Start_Stop);
    gfx->setCursor(5, 118);
    gfx->print("M");
    gfx->setCursor(5, 118);
    if (auto_aktiv == 1) {
      gfx->setTextColor(GREEN);
      gfx->print("A");
      glas_alt = -1;
      korr_alt = -99999;
    }
    else {
      gfx->setTextColor(RED);
      gfx->print("B");
    }
    auto_aktiv_alt = auto_aktiv;
    gfx->setTextColor(TEXT);
  }
  if (tara > 0) {
    if (gewicht < -20 and gewicht != gewicht_alt) {
      gfx->fillRect(80, 54, 240, 80, BACKGROUND);
      gfx->setFont(Punk_Mono_Bold_320_200);
      gfx->setCursor(120, 88);
      gfx->print("Bitte Glas");
      gfx->setCursor(120, 120);
      gfx->print("aufstellen");
      glas_alt = -1;
    } 
    else if (gewicht != gewicht_alt) {
      gfx->fillRect(80, 54, 240, 80, BACKGROUND);
      gfx->setFont(Punk_Mono_Bold_600_375);
      gfx->setCursor(100, 115);
      sprintf(ausgabe, "%5ig", gewicht - tara_glas);
      gfx->print(ausgabe);
    }
    if (gewicht != gewicht_alt) {
      progressbar = 318.0*((float)gewicht/(float)(glaeser[fmenge_index].Gewicht));
      progressbar = constrain(progressbar,0,318);
      gfx->drawRect(0, 137, 320, 15, TEXT);
      if (glaeser[fmenge_index].Gewicht > gewicht) {
        gfx->fillRect  (1, 138, progressbar, 13, RED);
      }
      else if (gewicht >= glaeser[fmenge_index].Gewicht and gewicht <= glaeser[fmenge_index].Gewicht + kulanz_gr){
        gfx->fillRect  (1, 138, progressbar, 13, GREEN);
      }
      else {
        gfx->fillRect  (1, 138, progressbar, 13, ORANGE);
      }
      gfx->fillRect  (1 + progressbar, 138, 318 - progressbar, 13, BACKGROUND);
      gewicht_alt = gewicht;
    }
  }
  else if (gewicht != gewicht_alt) {
    gfx->fillRect(80, 54, 240, 80, BACKGROUND);
    gfx->fillRect(0, 137, 320, 15, BACKGROUND);
    gfx->setTextColor(RED);
    gfx->setFont(Punk_Mono_Bold_320_200);
    gfx->setCursor(120, 104);
    gfx->print("kein tara!");
    gfx->setTextColor(TEXT);
    gewicht_alt = gewicht;
  }
  if (alarm_overcurrent) {i = 1;}
  while (i > 0) {
    inawatchdog = 0;                    //schalte die kontiunirliche INA Messung aus
    //Servo ist zu
    if (servo.read() <= winkel_min  + offset_winkel and offset_winkel < 3) {
      while(offset_winkel < 3 and current_servo < current_mA) {
        offset_winkel = offset_winkel + 1;
        SERVO_WRITE(winkel_min + offset_winkel);
        current_mA = GetCurrent(10);
        delay(1000);
      }
      alarm_overcurrent = 0;
    }
    i = 0;
    inawatchdog = 1;
  }
  setPreferences();
}

void processHandbetrieb(void) {
  i = 0;
  int y_offset = 0;
  if (modus != MODE_HANDBETRIEB) {
     modus = MODE_HANDBETRIEB;
     winkel = winkel_min;          // Hahn schliessen
     servo_aktiv = 0;              // Servo-Betrieb aus
     SERVO_WRITE(winkel);
     rotary_select = SW_WINKEL;
     tara = 0;
     offset_winkel = 0;            // Offset vom Winkel wird auf 0 gestellt
    int x_pos;
    gewicht_alt = -9999999;
    pos_alt = -1;
    winkel_min_alt = -1;
    tara_alt = -1;
    current_mA_alt = -1;
    servo_aktiv_alt = -1;
    gfx->fillScreen(BACKGROUND);
    gfx->setTextColor(TEXT);
    gfx->setFont(Punk_Mono_Bold_320_200);
    sprintf(ausgabe,"Manuell");
    x_pos = CenterPosX(ausgabe, 18, 320);
    gfx->setCursor(x_pos, 27);
    gfx->print(ausgabe);
    gfx->drawLine(0, 30, 320, 30, TEXT);
    gfx->setFont(Punk_Mono_Bold_200_125);
    gfx->setCursor(5, 192);
    gfx->print("Servo:");
    gfx->drawLine(0, 170, 320, 170, TEXT);
    gfx->drawLine(160, 170, 160, 240, TEXT);
    gfx->setCursor(170, 192);
    gfx->print("Tara:");
    if (ina219_installed == 1) {
      gfx->setCursor(170, 215);
      sprintf(ausgabe, "INA:     %3s", (current_servo==0?"aus":"ein"));
      gfx->print(ausgabe);
      if (ina219_installed == 1 and current_mA != current_mA_alt and (current_servo > 0 or show_current == 1)) {
        gfx->setCursor(170, 238);
        gfx->print("Strom:");
      }
    }
  }
  pos = getRotariesValue(SW_WINKEL);
  gewicht = SCALE_GETUNITS(SCALE_READS) - tara;
  if ((digitalRead(button_start_pin)) == HIGH) {
    servo_aktiv = 1;
  }
  if ((digitalRead(button_stop_pin)) == HIGH) {
    servo_aktiv = 0;
  }
  if ((digitalRead(EncoderButton)) == LOW) {
      tara = SCALE_GETUNITS(SCALE_READS);
  }
  if (servo_aktiv == 1) {
    winkel = ((winkel_max * pos) / 100);
  }
  else { 
    winkel = winkel_min + offset_winkel;
  }
  winkel = constrain(winkel, winkel_min + offset_winkel, winkel_max);
  SERVO_WRITE(winkel);
  if (ina219_installed and (current_servo > 0 or show_current == 1)) {
    y_offset = 4;
  }
  #ifdef isDebug
    #if isDebug >= 4
      Serial.print("Handbetrieb:");  
      Serial.print(" Gewicht ");     Serial.print(gewicht);
      Serial.print(" Winkel ");      Serial.print(winkel);
      Serial.print(" servo_aktiv "); Serial.println(servo_aktiv);
    #endif
  #endif
  gfx->setFont(Punk_Mono_Bold_600_375);
  if (gewicht != gewicht_alt) {
    gfx->fillRect(80, 70, 200, 55, BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_600_375);
    gfx->setCursor(100, 120);
    sprintf(ausgabe, "%5ig", gewicht);
    gfx->print(ausgabe);
    gewicht_alt = gewicht;
  }
  if (winkel_min  + offset_winkel != winkel_min_alt) {
    gfx->fillRect(97, 198, 50, 20, BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_200_125);
    gfx->setCursor(5, 215);
    sprintf(ausgabe, "min:    %3i°", winkel_min + offset_winkel);
    gfx->print(ausgabe);
    winkel_min_alt = winkel_min + offset_winkel;
  }
  if (pos != pos_alt) {
    gfx->fillRect(97, 221, 50, 20, BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_200_125);
    gfx->setCursor(5, 238);
    sprintf(ausgabe, "max:    %3i°", winkel_max*pos/100);
    gfx->print(ausgabe);
    pos_alt = pos;
  }
  if (tara != tara_alt) {
    gfx->fillRect(230, 175, 90, 22, BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_200_125);
    gfx->setCursor(240, 192);
    sprintf(ausgabe, "%5ig", tara);
    gfx->print(ausgabe);
    tara_alt = tara;
  }
  if (ina219_installed == 1 and current_mA != current_mA_alt and (current_servo > 0 or show_current == 1)) {
    gfx->fillRect(240, 221, 80, 20, BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_200_125);
    gfx->setCursor(170, 238);
    if (current_mA > current_servo and current_servo > 0) {
      gfx->setTextColor(RED);
    }
    sprintf(ausgabe, "%10imA", current_mA);
    gfx->print(ausgabe);
    gfx->setTextColor(TEXT);
    current_mA_alt = current_mA;
  }
  if (servo_aktiv != servo_aktiv_alt) {
    gfx->fillRect(17, 79, 38, 38, BACKGROUND);
    gfx->setFont(Icons_Start_Stop);
    gfx->setCursor(10, 123);
    gfx->print("M");
    gfx->setCursor(10, 123);
    if (servo_aktiv == 1) {
      gfx->setTextColor(GREEN);
      gfx->print("A");
    }
    else {
      gfx->setTextColor(RED);
      gfx->print("B");
    }
    servo_aktiv_alt = servo_aktiv;
    gfx->setTextColor(TEXT);
  }
  if (alarm_overcurrent) {i = 1;}
  while (i > 0) {
    inawatchdog = 0;                    //schalte die kontiunirliche INA Messung aus
    //Servo ist zu
    if (servo.read() <= winkel_min  + offset_winkel and offset_winkel < 3) {
      while(offset_winkel < 3 and current_servo < current_mA) {
        offset_winkel = offset_winkel + 1;
        SERVO_WRITE(winkel_min + offset_winkel);
        current_mA = GetCurrent(10);
        delay(1000);
      }
    }
    //Servo ist offen
    //else {
    //  servo_aktiv = 0;
    //}
    i = 0;
    inawatchdog = 1;
    alarm_overcurrent = 0;
  }
  setPreferences();         //irgendwo anderst setzen. es muss nicht jede änderung geschrieben werden
}

void setup() {
  // enable internal pull downs for digital inputs 
  pinMode(button_start_pin, INPUT_PULLDOWN);
  pinMode(button_stop_pin, INPUT_PULLDOWN);
  pinMode(switch_betrieb_pin, INPUT_PULLDOWN);
  pinMode(switch_setup_pin, INPUT_PULLDOWN);
  Serial.begin(115200);
  while (!Serial) {}
  #ifdef isDebug
    Serial.println("Hanimandl Start");
  #endif
  // Try to initialize the INA219
  if (ina219.begin()) {
    ina219_installed = 1;
    #ifdef isDebug
      Serial.println("INA219 chip gefunden");
    #endif
  }
  else {
    current_servo = 0;                              // ignore INA wenn keiner gefunden wird
    #ifdef isDebug
      Serial.println("INA219 chip nicht gefunden");
    #endif
  }
  // Rotary
  pinMode(EncoderButton, INPUT_PULLUP);
  attachInterrupt(EncoderButton, isr1, FALLING);
  pinMode(EncoderA,INPUT);
  pinMode(EncoderB,INPUT);
  attachInterrupt(EncoderA, isr2, CHANGE);
  // Buzzer
  pinMode(buzzer_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);
  // short delay to let chip power up
  delay (100);
  // Preferences aus dem EEPROM lesen
  getPreferences();
  // Servo initialisieren und schliessen
  #ifdef SERVO_ERWEITERT
    servo.attach(servo_pin,  750, 2500); // erweiterte Initialisierung, steuert nicht jeden Servo an
    servo.setPeriodHertz(100);
  #else
    servo.attach(servo_pin, 1000, 2000); // default Werte. Achtung, steuert den Nullpunkt weniger weit aus!  
  #endif
  SERVO_WRITE(winkel_min);
  // Waage erkennen - machen wir vor dem Boot-Screen, dann hat sie 3 Sekunden Zeit zum aufwärmen
  scale.begin(hx711_dt_pin, hx711_sck_pin);
  if (scale.wait_ready_timeout(1000)) {               // Waage angeschlossen? :Roli - Ist immer null wenn kein HX711 angeschlossen ist
    scale.power_up();
    if (scale.read() != 0) {                          // Roli - Wenn 0, nehme ich an, das kein HX711 angeschlossen ist
      waage_vorhanden = 1;
      #ifdef isDebug
        Serial.println("Waage erkannt");
      #endif
    }
  }
  // Boot Screen
  tft_colors();
  tft_marker();
  gfx->begin();
  gfx->fillScreen(BACKGROUND);
  gfx->setUTF8Print(true);
  if (showlogo) {
    print_logo();
    delay(3000);
  }
  if (showcredits) {
    buzzer(BUZZER_SHORT);
    print_credits();   
    delay(2500);
  }
  // Setup der Waage, Skalierungsfaktor setzen
  if (waage_vorhanden ==1) {                         // Waage angeschlossen?
    if (faktor == 0) {                               // Vorhanden aber nicht kalibriert
      gfx->fillScreen(BACKGROUND);
      gfx->setTextColor(RED);
      gfx->setFont(Punk_Mono_Bold_320_200);
      sprintf(ausgabe,"Nicht");
      int x_pos = CenterPosX(ausgabe, 18, 320);
      gfx->setCursor(x_pos, 110);
      gfx->print(ausgabe);
      sprintf(ausgabe,"kalibriert");
      x_pos = CenterPosX(ausgabe, 18, 320);
      gfx->setCursor(x_pos, 154);
      gfx->print(ausgabe);
      buzzer(BUZZER_ERROR);
      #ifdef isDebug
        Serial.println("Waage nicht kalibriert!");
      #endif
      delay(2000);
    }
    else {                                          // Tara und Skalierung setzen
      scale.set_scale(faktor);
      scale.set_offset(long(gewicht_leer));
      #ifdef isDebug
        Serial.println("Waage initialisiert");
      #endif
    }
  }
  else {                                            // Keine Waage angeschlossen
    gfx->fillScreen(BACKGROUND);
    gfx->setTextColor(RED);
    gfx->setFont(Punk_Mono_Bold_320_200);
    sprintf(ausgabe,"Keine");
    int x_pos = CenterPosX(ausgabe, 18, 320);
    gfx->setCursor(x_pos, 110);
    gfx->print(ausgabe);
    sprintf(ausgabe,"Waage!");
    x_pos = CenterPosX(ausgabe, 18, 320);
    gfx->setCursor(x_pos, 154);
    gfx->print(ausgabe);
    buzzer(BUZZER_ERROR);
    #ifdef isDebug
      Serial.println("Keine Waage!");
    #endif
    delay(2000);
  }
  // initiale Kalibrierung des Leergewichts wegen Temperaturschwankungen
  // Falls mehr als 20g Abweichung steht vermutlich etwas auf der Waage.
  if (waage_vorhanden == 1) {
    gewicht = SCALE_GETUNITS(SCALE_READS);
    if ((gewicht > -20) && (gewicht < 20)) {
      scale.tare(10);
      buzzer(BUZZER_SUCCESS);
      #ifdef isDebug
        Serial.print("Tara angepasst um: ");
        Serial.println(gewicht);
      #endif
    }
    else if (faktor != 0) {
      gfx->fillScreen(BACKGROUND);
      gfx->setTextColor(RED);
      gfx->setFont(Punk_Mono_Bold_320_200);
      sprintf(ausgabe,"Waage");
      int x_pos = CenterPosX(ausgabe, 18, 320);
      gfx->setCursor(x_pos, 110);
      gfx->print(ausgabe);
      sprintf(ausgabe,"leeren!");
      x_pos = CenterPosX(ausgabe, 18, 320);
      gfx->setCursor(x_pos, 154);
      gfx->print(ausgabe);
      #ifdef isDebug
        Serial.print("Gewicht auf der Waage: ");
        Serial.println(gewicht);
      #endif
      delay(5000);
      // Neuer Versuch, falls Gewicht entfernt wurde
      gewicht = SCALE_GETUNITS(SCALE_READS);
      if ((gewicht > -20) && (gewicht < 20)) {
        scale.tare(10);
        buzzer(BUZZER_SUCCESS);
        #ifdef isDebug
          Serial.print("Tara angepasst um: ");
          Serial.println(gewicht);
        #endif
      }
      else {    // Warnton ausgeben
        buzzer(BUZZER_LONG);
      }
    }
  }
  // die drei Datenstrukturen des Rotaries initialisieren
  initRotaries(SW_WINKEL,    0,   0, 100, 5);     // Winkel
  initRotaries(SW_KORREKTUR, 0, -90,  20, 1);     // Korrektur
  initRotaries(SW_MENU,      0,   0,   7, 1);     // Menuauswahlen
  // Parameter aus den Preferences für den Rotary Encoder setzen
  setRotariesValue(SW_WINKEL,    pos);   
  setRotariesValue(SW_KORREKTUR, korrektur);
  setRotariesValue(SW_MENU,      fmenge_index);
}

void loop() {
  rotating = true;     // debounce Management
  //INA219 Messung
  if (ina219_installed and inawatchdog == 1 and (current_servo > 0 or show_current == 1) and (modus == MODE_HANDBETRIEB or modus == MODE_AUTOMATIK)) {
    ina219_measurement();
  }
  // Setup Menu 
  if ((digitalRead(switch_setup_pin)) == HIGH) {
    processSetup();
  }
  // Automatik-Betrieb 
  else if ((digitalRead(switch_betrieb_pin)) == HIGH) {
    processAutomatik();
  }
  // Handbetrieb 
  else if ((digitalRead(switch_betrieb_pin) == LOW) && (digitalRead(switch_setup_pin) == LOW)) {
    processHandbetrieb();
  }
}

void print_credits() {
  int offset = 23;
  gfx->fillScreen(BACKGROUND);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  gfx->setCursor(10, 1*offset);
  gfx->print("Idee:");
  gfx->setCursor(90, 1*offset);
  gfx->print("M. Vasterling");
  gfx->setCursor(10, 2*offset+10);
  gfx->print("Code:");
  gfx->setCursor(90, 2*offset+10);
  gfx->print("M. Vasterling");
  gfx->setCursor(90, 3*offset+10);
  gfx->print("M. Wetzel");
  gfx->setCursor(90, 4*offset+10);
  gfx->print("C. Gruber");
  gfx->setCursor(90, 5*offset+10);
  gfx->print("A. Holzhammer");
  gfx->setCursor(90, 6*offset+10);
  gfx->print("M. Junker");
  gfx->setCursor(90, 7*offset+10);
  gfx->print("J. Kuder");
  gfx->setCursor(90, 8*offset+10);
  gfx->print("J. Bruker");
}

void print_logo() {
  gfx->fillScreen(BACKGROUND);
  gfx->drawXBitmap(60, 10, LogoBieneTFT, 200, 160, TEXT);
  gfx->setTextColor(TEXT);
  gfx->setFont(Punk_Mono_Bold_600_375);
  gfx->setCursor(0, 180);
  gfx->print("HANI");
  gfx->setCursor(140, 235);
  gfx->print("MANDL");
  gfx->setFont(Punk_Mono_Bold_200_125);
  gfx->setCursor(255, 180);
  gfx->print(version);
}

// Wir nutzen einen aktiven Summer, damit entfällt die tone Library, die sich sowieso mit dem Servo beisst.
void buzzer(byte type) {
  if (buzzermode == 1 or ledmode == 1) {
    switch (type) {
      case BUZZER_SHORT: //short
        if (buzzermode == 1) {digitalWrite(buzzer_pin,HIGH);}
        if (ledmode == 1) {digitalWrite(led_pin,HIGH);}
        if (buzzermode == 1 or ledmode == 1) {delay(100);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,LOW);}
        if (ledmode == 1) {digitalWrite(led_pin,LOW);}
        break;
      case BUZZER_LONG: //long
        if (buzzermode == 1) {digitalWrite(buzzer_pin,HIGH);}
        if (ledmode == 1) {digitalWrite(led_pin,HIGH);}
        if (buzzermode == 1 or ledmode == 1) {delay(500);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,LOW);}
        if (ledmode == 1) {digitalWrite(led_pin,LOW);}
        break;
      case BUZZER_SUCCESS: //success
        if (buzzermode == 1) {digitalWrite(buzzer_pin,HIGH);}
        if (ledmode == 1) {digitalWrite(led_pin,HIGH);}
        if (buzzermode == 1 or ledmode == 1) {delay(100);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,LOW);}
        if (ledmode == 1) {digitalWrite(led_pin,LOW);}
        if (buzzermode == 1 or ledmode == 1) {delay(100);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,HIGH);}
        if (ledmode == 1) {digitalWrite(led_pin,HIGH);}
        if (buzzermode == 1 or ledmode == 1) {delay(100);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,LOW);}
        if (ledmode == 1) {digitalWrite(led_pin,LOW);}
        if (buzzermode == 1 or ledmode == 1) {delay(100);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,HIGH);}
        if (ledmode == 1) {digitalWrite(led_pin,HIGH);}
        if (buzzermode == 1 or ledmode == 1) {delay(100);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,LOW);}
        if (ledmode == 1) {digitalWrite(led_pin,LOW);}
        break;
      case BUZZER_ERROR: //error
        if (buzzermode == 1) {digitalWrite(buzzer_pin,HIGH);}
        if (ledmode == 1) {digitalWrite(led_pin,HIGH);}
        if (buzzermode == 1 or ledmode == 1) {delay(1500);}
        if (buzzermode == 1) {digitalWrite(buzzer_pin,LOW);}
        if (ledmode == 1) {digitalWrite(led_pin,LOW);}
        break;
    }
  }
}

// Supportfunktionen für stufenweise Gewichtsverstellung
int step2weight(int step) {
  int sum = 0;
  if ( step > 210 ) { sum += (step-210)*1000; step -= (step-210); }
  if ( step > 200 ) { sum += (step-200)* 500; step -= (step-200); }  
  if ( step > 160 ) { sum += (step-160)* 100; step -= (step-160); }
  if ( step > 140 ) { sum += (step-140)*  25; step -= (step-140); }
  if ( step >  50 ) { sum += (step- 50)*   5; step -= (step- 50); }
  sum += step;  
  return sum;
}

int weight2step(int sum) {
  int step = 0;
  if ( sum > 10000 ) { step += (sum-10000)/1000; sum -= ((sum-10000)); }
  if ( sum >  5000 ) { step += (sum-5000)/500;   sum -= ((sum-5000)); }
  if ( sum >  1000 ) { step += (sum-1000)/100;   sum -= ((sum-1000)); }
  if ( sum >   500 ) { step += (sum-500)/25;     sum -= ((sum-500));  }
  if ( sum >    50 ) { step += (sum-50)/5;       sum -= (sum-50);   }
  step += sum;
  return step;
}

//Sub function for Display
int StringLenght(String a) {
  a.trim();
  int res = a.length();
  return res;
}

int CenterPosX(const char a[], float font_width, int display_widht) {
  int res = 0;
  int counter = get_length(a);
  res = int(float(display_widht - font_width * counter) / 2);
  return res;
}

int get_length(const char a[]) {
  int counter = 0;
  for (int i = 0; i < strlen(a); i++) {
    if ((unsigned char) a[i] >= 0x00 and (unsigned char) a[i] <= 0x7F) {
      counter++;
    }
    else if ((unsigned char) a[i] >= 0x80 and (unsigned char) a[i] <= 0x07FF) {
      i += 1;
      counter++;
    }
    else if ((unsigned char) a[i] >= 0x0800 and (unsigned char) a[i] <= 0xFFFF) {
      i += 2;
      counter++;
    }
    else if ((unsigned char) a[i] >= 0x010000 and (unsigned char) a[i] <= 0x10FFFF) {
      i += 3;
      counter++;
    }
    else {
      i += 4;      //ist kein UTF8 Zeichen. Sollte also nie hier ankommen
    }
  }
  return counter;
}

//Sub function for INA219
int GetCurrent(int count) {
  int res = 0;
  for (int x = 0; x < count; x++) {
    res = res + ina219.getCurrent_mA();
  }
  if (count > 0) {
    res = res / count;
  }
  return res;
}

void ina219_measurement() {
  if (current_mA > current_servo and current_servo > 0) {
    if (last_overcurrenttime == 0) {
      last_overcurrenttime = millis();
    }
    else if (millis() - last_overcurrenttime >= overcurrenttime) {
      last_overcurrenttime = 0;
      alarm_overcurrent = 1;
    }
  }
  else {
    last_overcurrenttime = 0;
  }
  if (millis() - last_ina219_measurement >= updatetime_ina219) {
    last_ina219_measurement = millis();
    current_mA = GetCurrent(10);
  }
}

