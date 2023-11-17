// HaniMandl rewrite by Frank
// For readabilty, I removed and changed as I saw fit
// Supports only 320x240 color tft display ST7789
// Code provided as is, use as you please

#include <Arduino.h>
#include <Wire.h>
#include <ESP32Servo.h>             /* aus dem Bibliotheksverwalter */
#include <Adafruit_INA219.h>        /* aus dem Bibliotheksverwalter */
#include <Preferences.h>            /* aus dem BSP von expressif, wird verfügbar wenn das richtige Board ausgewählt ist */
#include <nvs_flash.h>              /* aus dem BSP von expressif, wird verfügbar wenn das richtige Board ausgewählt ist */
#include <Arduino_GFX_Library.h>    /* aus dem Bibliotheksverwalter */
#include <TFT_eSPI.h>               // more versatile display library

#include "hani.h"
#include "Arialbd72.h"
#include <HX711_ADC.h>
#include "menustructure.h"

// Version
String version = "V1.0 by F.";
extern void SetupMyDisplay(void);
extern void TFT_line_print(int line, const char *content);
extern void TFT_line_color(int line, int textcolor, int backgroundcolor);
extern void TFT_line_blink(int line, bool blink);  
extern void SetupButtons(void);
extern void ReadButtons(void * pvParameters);
extern void SelectMenu(int menu);
extern void Menu(void);  // big new menu handler

extern bool bScrollNow;
extern bool bUpdateDisplay;
extern bool bBlinkDisplay;
extern int  BlinkTimer10mS;

extern int NewHaniDisplayMode;
extern int ActLcdMode;
extern int NewWeight;
extern int OldWeight;

extern volatile bool deb_start_button;
extern volatile bool deb_stop_button;
extern volatile bool deb_encoder_button;
extern volatile bool deb_setup_switch;
extern volatile bool deb_auto_switch;
extern volatile bool deb_manual_switch;

extern int start_button_f;
extern int stop_button_f;
extern int encoder_button_f;
extern int setup_switch_f;
extern int auto_switch_f;
extern int manual_switch_f;

extern int start_button_very_long_pressed;
extern int stop_button_very_long_pressed;
extern int encoder_button_very_long_pressed;
extern int setup_switch_very_long_pressed;
extern int auto_switch_very_long_pressed;
extern int manual_switch_very_long_pressed;

// setup menu 
int LastMenu = 0;    // last menu used, for comfortable re-entry
int CurrentMenu = 0; // 
int EditMenu = 0; // 


//HX711 constructor:
const int HX711_dout = 35; //mcu > HX711 dout pin, must be external interrupt capable!
const int HX711_sck = 17; //mcu > HX711 sck pin
HX711_ADC LoadCell(HX711_dout, HX711_sck);
float calibrationValue;
volatile boolean newDataReady;

extern void processAutomatik2(void);
void SetDefaultParameters(void);
void SaveParameters(void);
void LoadParameters(void);

//
// Hier den Code auf die verwendete Hardware einstellen
//

#define ROTARY_SCALE 2         // number of steps for each click of your rotary encoder used
// Ende Benutzereinstellungen!
// 

//
// Ab hier nur verstellen wenn Du genau weisst, was Du tust!
//
//#define isDebug 4             // serielle debug-Ausgabe aktivieren. Mit > 3 wird jeder Messdurchlauf ausgegeben
                                // mit 4 zusätzlich u.a. Durchlaufzeiten
                                // mit 5 zusätzlich rotary debug-Infos
                                // ACHTUNG: zu viel Serieller Output kann einen ISR-Watchdog Reset auslösen!

#define MAXIMALGEWICHT 4500     // maximum allowed weight in gram - 1500 for 2kg loadcell - 4500 for 5kg loadcell

// INA 219
Adafruit_INA219 ina219;


Arduino_DataBus *bus = new Arduino_HWSPI(13 /* DC */, 5 /* CS */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 14 /* RST */, 3 /* rotation */);
//Arduino_GFX *gfx;

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

// new display control with TFT_eSPI
// Display ST7789 - set per build flags in platformio.ini
// no changes made to TFT_ESPI library
// TFT_MISO=19  ; not available on my ST7789, combined with SDA MOSI=23
// TFT_MOSI=23  ; SDA of ST7789
// TFT_SCLK=18  ; SCL of ST7789
extern TFT_eSPI tft;  // invoke TFT_eSPI library in hani-display.cpp

// Hardware Pin Definitions located in HANI.H
// Target ESP32-DEVKIT 30 Pins Connector

// Buzzer and LED
static int buzzer_pin = 25;
int led_pin = 0;

Servo servo;
Preferences preferences;

// Denstrukturen für Rotary Encoder
struct rotary {                        
  int Value;
  int Minimum;
  int Maximum;
  int Step;
};

struct rotary rotaries[3]; // will be initialized in setup()
int rotary_select = SW_WINKEL;

JarName JarNames[6] = {{"DE Imker Bund","DIB"},{"TwistOff","TOF"},{"DeepTwist","DEE"},{"Special Jar","SPX"},{"Ferry's DeLuxe","FDL"},{"Eco Jar","ECO"}}; // name and shortname
JarParameter Jars[6] = {{125, 0, -9999, 0, 0}, // weight, name, tarra, tripcount, counter
                        {250, 1, -9999, 0, 0},
                        {250, 2, -9999, 0, 0},
                        {500, 1, -9999, 0, 0},
                        {450, 4, -9999, 0, 0},
                        {500, 5, -9999, 0, 0}};
 

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
char ausgabe[256];                  // FB: Increased lenght for use with long scrolling texts
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
bool bINA219_installed = false;     
int current_servo = 0;              // 0 = INA219 wird ignoriert, 1-1000 = erlaubter Maximalstrom vom Servo in mA
int current_mA;                     // Strom welcher der Servo zieht
int updatetime_ina219 = 500;        // Zeit in ms in welchem der INA219 gelesen wird (500 -> alle 0,5 sek wird eine Strommessung vorgenommen)
int last_ina219_measurement = 0;    // Letzte Zeit in welcher der Strom gemessen wurde
int overcurrenttime = 1500;         // Zeit in welcher der Servo mehr Strom ziehen darf als einfestellt in ms
int last_overcurrenttime = 0;       // Letzte Zeit in welcher keinen Ueberstrom gemessen wurde
int alarm_overcurrent = 0;          // Alarmflag wen der Servo zuwiel Strom zieht
int show_current = 0;               // 0 = aus, 1 = ein / Zeige den Strom an auch wenn der INA ignoriert wird
int inawatchdog = 1;                // 0 = aus, 1 = ein / wird benötigt um INA messung auszusetzen
int offset_winkel = 0;              // Offset in Grad vom Schlieswinkel wen der Servo Überstrom hatte (max +3Grad vom eingestelten Winkel min)
int color_scheme = 0;               // 0 = dunkel, 1 = hell / Wechsel vom color scheme für den TFT Display
int color_marker_idx = 3;           // Farbe für den Marker für das TFT Display
bool bOldMenu = false;               // use old rolling menu

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

int SysParams[LASTPARAMETER];

//Color Scheme für den TFT Display
unsigned long  COLOR_BACKGROUND;
unsigned long  COLOR_TEXT;
unsigned long  COLOR_MENU_POS1;
unsigned long  COLOR_MENU_POS2;
unsigned long  COLOR_MENU_POS3;
unsigned long  COLOR_MENU_POS4;
unsigned long  COLOR_MARKER;

// local function declarations
int CenterPosX(const char a[], float font_width, int display_width);
int get_length(const char a[]);
int weight2step(int sum);
int GetCurrent(int count);
int step2weight(int step);
int StringLenght(String a);
void ina219_measurement(void);
void buzzer(byte type);
void print_credits(void);
void print_logo(void);

void tft_colors() {
  if (color_scheme == 0) {          //dunkel
    COLOR_BACKGROUND  = TFT_BLACK;
    COLOR_TEXT        = TFT_WHITE;
    COLOR_MENU_POS1   = 0x73AF;
    COLOR_MENU_POS2   = 0x5AEC;
    COLOR_MENU_POS3   = 0x39C8;
    COLOR_MENU_POS4   = 0x20E6;
  }
  else {                            //hell
    COLOR_BACKGROUND  = TFT_WHITE;
    COLOR_TEXT        = TFT_BLACK;
    COLOR_MENU_POS1   = 0x1082;
    COLOR_MENU_POS2   = 0x2104;
    COLOR_MENU_POS3   = 0x3166;
    COLOR_MENU_POS4   = 0x4208;
  }
}

void tft_marker() {                
  if (color_marker_idx == 0)                              {COLOR_MARKER = 0x05ff;}
  else if (color_marker_idx == 1)                         {COLOR_MARKER = 0x021f;}
  else if (color_marker_idx == 2)                         {COLOR_MARKER = 0x001f;}
  else if (color_marker_idx == 3)                         {COLOR_MARKER = 0x4851;}
  else if (color_marker_idx == 4)                         {COLOR_MARKER = 0xc050;}
  else if (color_marker_idx == 5)                         {COLOR_MARKER = 0xf800;}
  else if (color_marker_idx == 6)                         {COLOR_MARKER = 0xfbc0;}
  else if (color_marker_idx == 7)                         {COLOR_MARKER = 0xfde0;}
  else if (color_marker_idx == 8)                         {COLOR_MARKER = 0xd6e1;}
  else if (color_marker_idx == 9)                         {COLOR_MARKER = 0xffe0;}
  else if (color_marker_idx == 10)                        {COLOR_MARKER = 0x2724;}
  else if (color_marker_idx == 11)                        {COLOR_MARKER = 0x0da1;}
}

// Rotary Taster. Der Interrupt kommt nur im Automatikmodus zum Tragen und nur wenn der Servo inaktiv ist.
// Der Taster schaltet in einen von drei Modi, in denen unterschiedliche Werte gezählt werden.
volatile int EncoderSleep10mS;

void IRAM_ATTR isr1() {
  static unsigned long last_interrupt_time = 0; 
  unsigned long interrupt_time = millis();

  // Clicking the encoder switch, sometimes also moved the encoder knob a bit, and results in a wrong selection
  // Another interrupt increases a time that indicates how long the encoder was not active
  if(EncoderSleep10mS>100)EncoderSleep10mS=100;
  EncoderSleep10mS-=10; // we need at least 10 interrupts here, to wake up the encoder
  if(EncoderSleep10mS>0)return;
  EncoderSleep10mS = 0;

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
  aState = digitalRead(ROTARY_ENCODER_A); // Reads the "current" state of the encoder
  bState = digitalRead(ROTARY_ENCODER_B); // Reads the "current" state of the encoder
  if (aState != aLastState) {     
    // If the outputB state is different to the outputA state, that means the encoder is turned clockwise
    if (aState != bState) {
      rotaries[rotary_select].Value -= rotaries[rotary_select].Step;
    }
    else {    // counter-clockwise
      rotaries[rotary_select].Value += rotaries[rotary_select].Step;
    }
    rotaries[rotary_select].Value = constrain( rotaries[rotary_select].Value, rotaries[rotary_select].Minimum, rotaries[rotary_select].Maximum);
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
{ if (digitalRead(ENCODER_BUTTON) == LOW) {
    while(digitalRead(ENCODER_BUTTON) == LOW);
    return true;
  }  
  else return false;
}    

boolean EncoderButtonPressed(void)
{ if (digitalRead(ENCODER_BUTTON) == LOW) {
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
  color_marker_idx    = preferences.getUInt("clr_marker_idx", color_marker_idx);
  preferences_chksum = faktor + pos + gewicht_leer + korrektur + autostart + autokorrektur + kulanz_gr + fmenge_index +
                       winkel_min + winkel_max + winkel_fein + buzzermode + ledmode + showlogo + showcredits + 
                       kali_gewicht + current_servo + glastoleranz + show_current + color_scheme + color_marker_idx;
  i = 0;
  int ResetGewichte[] = {125,250,250,500,500,};
  int ResetGlasTyp[] = {0,1,2,1,0,};
  while( i < 5) {
    sprintf(ausgabe, "Gewicht%d", i);
//    Jars[i].Gewicht = preferences.getInt(ausgabe, ResetGewichte[i]);
    preferences_chksum += Jars[i].Gewicht;
    sprintf(ausgabe, "GlasTyp%d", i);
//    Jars[i].GlasTyp = preferences.getInt(ausgabe, ResetGlasTyp[i]);
    preferences_chksum += Jars[i].GlasTyp;
    sprintf(ausgabe, "Tara%d", i);
//    Jars[i].Tara= preferences.getInt(ausgabe, -9999);
    preferences_chksum += Jars[i].Tara;
    sprintf(ausgabe, "TripCount%d", i);
//    Jars[i].TripCount = preferences.getInt(ausgabe, 0);
    preferences_chksum += Jars[i].TripCount;
    sprintf(ausgabe, "Count%d", i);
//    Jars[i].Count = preferences.getInt(ausgabe, 0);
    preferences_chksum += Jars[i].Count;
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
    Serial.print("color_marker_idx = ");    Serial.println(color_marker_idx);
    Serial.print("kali_gewicht = ");    Serial.println(kali_gewicht);
    Serial.print("glastoleranz = ");    Serial.println(glastoleranz);
    Serial.print("show_current = ");    Serial.println(show_current);
    i = 0;
    while( i < 5 ) {
      sprintf(ausgabe, "Gewicht%d = ", i);
      Serial.print(ausgabe);         
      Serial.println(Jars[i].Gewicht);
      sprintf(ausgabe, "GlasTyp%d = ", i);
      Serial.print(ausgabe);         
      Serial.println(JarNames[Jars[i].GlasTyp].shortname);
      sprintf(ausgabe, "Tara%d = ", i);
      Serial.print(ausgabe);         
      Serial.println(Jars[i].Tara);
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
    if (Jars[i].TripCount != preferences.getInt(ausgabe, 0)) {
      preferences.putInt(ausgabe, Jars[i].TripCount);
    }
    sprintf(ausgabe, "Count%d", i);
    if (Jars[i].Count != preferences.getInt(ausgabe, 0)) {
      preferences.putInt(ausgabe, Jars[i].Count);
    }
    #ifdef isDebug
      Serial.print("Counter gespeichert: Index ");
      Serial.print(i);
      Serial.print(" Trip ");
      Serial.print(Jars[fmenge_index].TripCount);
      Serial.print(" Gesamt ");
      Serial.println(Jars[fmenge_index].Count);      
    #endif
  }
  // Den Rest machen wir gesammelt, das ist eher statisch
  preferences_newchksum = faktor + gewicht_leer + korrektur + autostart + autokorrektur +
                          fmenge_index + winkel_min + winkel_max + winkel_fein + kulanz_gr +
                          buzzermode + ledmode + showlogo + showcredits + current_servo + kali_gewicht + 
                          glastoleranz + show_current + color_scheme + color_marker_idx;
  i = 0;
  while( i < 5 ) {
    preferences_newchksum += Jars[i].Gewicht;
    preferences_newchksum += Jars[i].GlasTyp;
    preferences_newchksum += Jars[i].Tara;
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
  preferences.putUInt("clr_marker_idx", color_marker_idx);
  i = 0;
  while( i < 5 ) {
    sprintf(ausgabe, "Gewicht%d", i);
    preferences.putInt(ausgabe, Jars[i].Gewicht);
    sprintf(ausgabe, "GlasTyp%d", i);
    preferences.putInt(ausgabe, Jars[i].GlasTyp);  
    sprintf(ausgabe, "Tara%d", i);
    preferences.putInt(ausgabe, Jars[i].Tara);
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
    Serial.print("color_marker_idx = ");    Serial.println(color_marker_idx);
    i = 0;
    while( i < 5 ) {
      sprintf(ausgabe, "Gewicht%d = ", i);
      Serial.print(ausgabe);         Serial.println(Jars[i].Gewicht);
      sprintf(ausgabe, "GlasTyp%d = ", i);
      Serial.print(ausgabe);         Serial.println(JarNames[Jars[i].GlasTyp].shortname);
      sprintf(ausgabe, "Tara%d = ", i);
      Serial.print(ausgabe);         Serial.println(Jars[i].Tara);
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
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  while (i > 0) { //Erster Screen: Anzahl pro Glasgröße
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    for(int j=0;j<5;j++) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", Jars[j].Gewicht, JarNames[Jars[j].GlasTyp].shortname);
      gfx->print(ausgabe);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%11d St.", Jars[j].TripCount);
      gfx->print(ausgabe);
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, COLOR_BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Zweiter Screen: Gewicht pro Glasgröße
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    for(int j=0;j<5;j++) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", Jars[j].Gewicht,JarNames[Jars[j].GlasTyp].shortname);
      gfx->print(ausgabe);
      gfx->setCursor(150, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%10.3fkg", Jars[j].Gewicht * Jars[j].TripCount / 1000.0);
      gfx->print(ausgabe);
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, COLOR_BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Dritter Screen: Gesamtgewicht
    TripAbfuellgewicht = 0;
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    for(int j=0;j<5;j++) {
      TripAbfuellgewicht += Jars[j].Gewicht * Jars[j].TripCount / 1000.0;
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
    if (digitalRead(ENCODER_BUTTON) == LOW) {
      //verlasse Screen
      while(digitalRead(ENCODER_BUTTON) == LOW);
      i = 0;
      gfx->fillRect(0, 31, 320, 209, COLOR_BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Vierter Screen: Zurücksetzen
    initRotaries(SW_MENU, 1, 0, 1, -1);
    rotaries[SW_MENU].Value = 1;
    i = 1;
    while (i > 0) {
      if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
        while (digitalRead(BUTTON_STOP) == HIGH);
        modus = -1;
        return;
      }
      pos = getRotariesValue(SW_MENU);
      gfx->setCursor(5, 30+(1 * y_offset_tft));
      if (pos == 0) {gfx->setTextColor(COLOR_MARKER);}
      else {gfx->setTextColor(COLOR_TEXT);}
      gfx->print("Reset");
      gfx->setCursor(5, 30+(2 * y_offset_tft));
      if (pos == 1) {gfx->setTextColor(COLOR_MARKER);}
      else {gfx->setTextColor(COLOR_TEXT);}
      gfx->print("Abbrechen");
      if (digitalRead(ENCODER_BUTTON) == LOW) {
        gfx->setTextColor(COLOR_MARKER);
        gfx->setCursor(283, 30+((pos+1) * y_offset_tft));
        gfx->print("OK");
        if ( pos == 0) {
          for(int j=0;j<5;j++)Jars[j].TripCount = 0;
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
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  while (i > 0) { //Erster Screen: Anzahl pro Glasgröße
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    for(int j=0;j<5;j++) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", Jars[j].Gewicht,JarNames[Jars[j].GlasTyp].shortname);
      gfx->print(ausgabe);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%11d St.", Jars[j].Count);
      gfx->print(ausgabe);
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, COLOR_BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Zweiter Screen: Gewicht pro Glasgröße
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    for(int j=0;j<5;j++) {
      gfx->setCursor(5, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg %3s", Jars[j].Gewicht,JarNames[Jars[j].GlasTyp].shortname);
      gfx->print(ausgabe);
      gfx->setCursor(150, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%10.3fkg", Jars[j].Gewicht * Jars[j].Count / 1000.0);
      gfx->print(ausgabe);
    }
    if (EncoderButtonTapped()) {
      //verlasse Screen
      i = 0;
      gfx->fillRect(0, 31, 320, 209, COLOR_BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Dritter Screen: Gesamtgewicht
    Abfuellgewicht = 0;
    if (digitalRead(BUTTON_STOP) == HIGH  or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    for(int j=0;j<5;j++)Abfuellgewicht += Jars[j].Gewicht * Jars[j].Count / 1000.0;
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
      gfx->fillRect(0, 31, 320, 209, COLOR_BACKGROUND);
    }
  }
  i = 1;
  while (i > 0) { //Vierter Screen: Zurücksetzen
    initRotaries(SW_MENU, 1, 0, 1, -1);
    rotaries[SW_MENU].Value = 1;
    i = 1;
    while (i > 0) {
      if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
        while (digitalRead(BUTTON_STOP) == HIGH);
        modus = -1;
        return;
      }
      pos = getRotariesValue(SW_MENU);
      gfx->setCursor(5, 30+(1 * y_offset_tft));
      if (pos == 0) {gfx->setTextColor(COLOR_MARKER);}
      else {gfx->setTextColor(COLOR_TEXT);}
      gfx->print("Reset");
      gfx->setCursor(5, 30+(2 * y_offset_tft));
      if (pos == 1) {gfx->setTextColor(COLOR_MARKER);}
      else {gfx->setTextColor(COLOR_TEXT);}
      gfx->print("Abbrechen");
      if (EncoderButtonPressed()) {
        gfx->setTextColor(COLOR_MARKER);
        gfx->setCursor(283, 30+((pos+1) * y_offset_tft));
        gfx->print("OK");
        if ( pos == 0) {
          for(int j=0;j<5;j++) {
            Jars[j].Count = 0;
            Jars[j].TripCount = 0;
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
  gfx->fillScreen(COLOR_BACKGROUND);
  bool change = false;
  const char menu[] = "Tarawerte Gläser";
  while (i == 0)  {
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    else if (EncoderButtonPressed()) {
      tara = round(LoadCell.getData());
      if (tara > 20) {                  // Gläser müssen mindestens 20g haben
         Jars[getRotariesValue(SW_MENU)].Tara = tara;
      }
      change = true;
      i++;
    }
    if (change) {
      gfx->fillScreen(COLOR_BACKGROUND);
    }
    gfx->setTextColor(COLOR_TEXT);
    gfx->setFont(Punk_Mono_Bold_240_150);
    x_pos = CenterPosX(menu, 14, 320);
    gfx->setCursor(x_pos, 25);
    gfx->println(menu);
    gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
    for(int j=0;j<5;j++) {
      if (j == getRotariesValue(SW_MENU)) {
        gfx->setTextColor(COLOR_MARKER);
      }
      else {
        gfx->setTextColor(COLOR_TEXT);
      }
      gfx->setCursor(5, 60+(j*30));
      if (Jars[j].Gewicht < 1000) {
        sprintf(ausgabe, " %3dg - %3s", Jars[j].Gewicht, JarNames[Jars[j].GlasTyp].shortname); 
      } 
      else {
        sprintf(ausgabe, "%.1fkg - %3s", float(Jars[j].Gewicht) / 1000, JarNames[Jars[j].GlasTyp].shortname); 
      }
      gfx->println(ausgabe);
      gfx->setCursor(215, 60+(j*30));
      if (Jars[j].Tara > 0) { 
        sprintf(ausgabe, "  %4dg", Jars[j].Tara); 
        gfx->print(ausgabe);
      }
      else {
        gfx->print("  fehlt");
      }
    }
  }
  modus = -1;
  delay(2000);
}

void setupCalibration(void) {
  float gewicht_raw;
  gfx->fillScreen(COLOR_BACKGROUND);
  int kali_gewicht_old = -1;
  const char menu[] = "Kalibrieren";
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(menu, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(menu);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
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
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    else if (EncoderButtonPressed()) {
      LoadCell.setCalFactor(1); 
      // scale.set_offset(long(gewicht_leer));
//      scale.set_scale(); // set scale to 1
      LoadCell.tare();
      delay(500);
      i = 0;
    }
  }
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  x_pos = CenterPosX(menu, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(menu);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  initRotaries(SW_MENU, kali_gewicht, 100, 9999, 1); 
  i = 1;
  while (i > 0) {
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    kali_gewicht = getRotariesValue(SW_MENU);
    if (kali_gewicht != kali_gewicht_old) {
      gfx->setTextColor(COLOR_BACKGROUND);
      if ((kali_gewicht >= 1000 and kali_gewicht_old <= 999) or (kali_gewicht_old >= 1000 and kali_gewicht <= 999)) {
        sprintf(ausgabe, "Bitte %dg", kali_gewicht_old);
      }
      else {
        sprintf(ausgabe, "      %dg", kali_gewicht_old);
      }
      x_pos = CenterPosX(ausgabe, 14, 320);
      gfx->setCursor(x_pos, 100); gfx->print(ausgabe);
      gfx->setTextColor(COLOR_TEXT);
      sprintf(ausgabe, "      %dg", kali_gewicht);
      x_pos = CenterPosX(ausgabe, 14, 320);
      gfx->setCursor(x_pos, 100); gfx->print("Bitte ");
      gfx->setTextColor(COLOR_MARKER);
      gfx->setCursor(x_pos, 100); gfx->print(ausgabe);
      gfx->setTextColor(COLOR_TEXT);
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
//      gewicht_raw  = scale.get_units(10);
      gewicht_raw = round(LoadCell.getData());
      faktor       = gewicht_raw / kali_gewicht;
//      scale.set_scale(faktor);
      LoadCell.setCalFactor(faktor); 
      gewicht_leer = LoadCell.getTareOffset();    // Leergewicht der Waage speichern
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
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  i = 1;
  while (i > 0) {
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
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
    for(int j=0; j < MenuepunkteAnzahl; j++) {
      gfx->setTextColor(COLOR_TEXT);
      if (j == pos and wert_aendern == false) {
        gfx->setTextColor(COLOR_MARKER);
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
          gfx->setTextColor(COLOR_MARKER);
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
            gfx->fillRect(251, 27+((j+1) * y_offset_tft)-19, 64, 25, COLOR_BACKGROUND);
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
      }
    
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) {
      while(EncoderButtonPressed());
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
      while(EncoderButtonPressed());
      if (servo_live == true) {
        SERVO_WRITE(winkel_min);
      }
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
      gfx->fillRect(170, 30+y_offset_tft+3, 96, 3*y_offset_tft + 2, COLOR_BACKGROUND);
    }
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed());
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
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
  i = 1;
  while (i > 0) {
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
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
    for(int j=0; j < MenuepunkteAnzahl; j++) {
      gfx->setTextColor(COLOR_TEXT);
      if (j == pos and wert_aendern == false) {
        gfx->setTextColor(COLOR_MARKER);
      }
      if (j < MenuepunkteAnzahl - 1) {
        gfx->setCursor(5, 30+((j+1) * y_offset_tft));
        gfx->print(menuepunkte[j]);
        if (j == pos and wert_aendern == true) {
          gfx->setTextColor(COLOR_MARKER);
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
          gfx->fillRect(251, 27+((j+1) * y_offset_tft)-19, 64, 25, COLOR_BACKGROUND);
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
    }
    // Menupunkt zum Ändern ausgewählt
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) { 
      while(EncoderButtonPressed());
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
      while(EncoderButtonPressed());
      rotary_select = SW_MENU;
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
    }
    // Menu verlassen 
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed());
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
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  int j;
  initRotaries(SW_MENU, fmenge_index, 0, 4, -1);
  i = 1;
  while (i > 0) {
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    pos = getRotariesValue(SW_MENU);
    for(int j=0; j < 5; j++) {
    if (j == pos) {
        gfx->setTextColor(COLOR_MARKER);
      }
      else {
        gfx->setTextColor(COLOR_TEXT);
      }
      gfx->setCursor(10, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg", Jars[j].Gewicht);
      gfx->print(ausgabe);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));

      gfx->print(JarNames[Jars[j].GlasTyp].name);
    }
    if (EncoderButtonTapped()) { // Füllmenge gewählt
      i = 0;
    }
  }
  i = 1;
  initRotaries(SW_MENU, weight2step(Jars[pos].Gewicht) , 25, weight2step(MAXIMALGEWICHT), 1);
  while (i > 0){
    if ((digitalRead(BUTTON_STOP)) == HIGH  or digitalRead(SWITCH_SETUP) == LOW) {
      modus = -1;
      return;
    }
    Jars[pos].Gewicht = step2weight(getRotariesValue(SW_MENU));
    for(int j=0; j < 5; j++) {
      gfx->setCursor(10, 30+((j+1) * y_offset_tft));
      if (j == pos) {
        if (wert_old != String(Jars[j].Gewicht)) {
          gfx->fillRect(0, 27+((j+1) * y_offset_tft)-19, 100, 27, COLOR_BACKGROUND);
          wert_old = String(Jars[j].Gewicht);
        }
        gfx->setTextColor(COLOR_MARKER);
      }
      sprintf(ausgabe, "%4dg", Jars[j].Gewicht);
      gfx->print(ausgabe);
      gfx->setTextColor(COLOR_TEXT);
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
      gfx->print(JarNames[Jars[j].GlasTyp].name);
    }
    if (EncoderButtonTapped()) { // Gewicht bestätigt
      i = 0;
    }
  }
  i = 1;
  initRotaries(SW_MENU, Jars[pos].GlasTyp, 0, sizeof(JarNames)/sizeof(JarNames[0]) - 1, 1);
  while (i > 0){ 
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    Jars[pos].GlasTyp = getRotariesValue(SW_MENU);
    for(int j=0;j<5;j++) {
      gfx->setCursor(10, 30+((j+1) * y_offset_tft));
      sprintf(ausgabe, "%4dg", Jars[j].Gewicht);
      gfx->print(ausgabe);
      if (j == pos) {
        if (wert_old != String(JarNames[Jars[j].GlasTyp].name)) {
          gfx->fillRect(100, 27+((j+1) * y_offset_tft)-19, 220, 27, COLOR_BACKGROUND);
          wert_old = String(JarNames[Jars[j].GlasTyp].name);
        }
        gfx->setTextColor(COLOR_MARKER);
      }
      gfx->setCursor(110, 30+((j+1) * y_offset_tft));
gfx->print(JarNames[Jars[j].GlasTyp].name);
      gfx->setTextColor(COLOR_TEXT);
    }
    if (EncoderButtonTapped()) { //GlasTyp bestätigt
      while(EncoderButtonTapped());
      i = 0;
    }
  }
  fmenge = Jars[pos].Gewicht;
  tara   = Jars[pos].Tara;
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
  int lastcolor_marker_idx = color_marker_idx;
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
    if (digitalRead(BUTTON_STOP) == HIGH or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      buzzermode = lastbuzzer;
      ledmode = lastled;
      showlogo = lastlogo;
      showcredits = lastcredits;
      color_scheme = lastcolor_scheme;
      color_marker_idx = lastcolor_marker_idx;
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
        case 5: color_marker_idx  = getRotariesValue(SW_MENU);
                break;
      }
    }
    // Menu
    if(change_scheme) {
      gfx->fillScreen(COLOR_BACKGROUND);
      gfx->setTextColor(COLOR_TEXT);
      gfx->setFont(Punk_Mono_Bold_240_150);
      int x_pos = CenterPosX(title, 14, 320);
      gfx->setCursor(x_pos, 25);
      gfx->println(title);
      gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
      change_scheme = false;
      change_marker = true;
    }
    for(int j=0;j < MenuepunkteAnzahl;j++) {
      gfx->setTextColor(COLOR_TEXT);
      if (j == pos and wert_aendern == false) {
        gfx->setTextColor(COLOR_MARKER);
      }
      if (j < MenuepunkteAnzahl - 1) {
        gfx->setCursor(5, 30+((j+1) * y_offset_tft));
        gfx->print(menuepunkte[j]);
        if (j == pos and wert_aendern == true) {
          gfx->setTextColor(COLOR_MARKER);
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
                  if (wert_old != color_marker_idx and wert_aendern == true and j == pos) {
                    change = true;
                    change_marker = true;
                    wert_old = color_marker_idx;
                    tft_marker();
                  }
                  break;
        }
        if (change) {
          gfx->fillRect(211, 27+((j+1) * y_offset_tft)-19, 104, 25, COLOR_BACKGROUND);
          change = false;
        }
        if (change_marker) {
          gfx->fillRect(265, 27+(6 * y_offset_tft)-17, 45, 21, COLOR_MARKER);
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
    }
    // Menupunkt zum Ändern ausgewählt
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == false) {
      while(EncoderButtonPressed());
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
        case 5: initRotaries(SW_MENU, color_marker_idx, 0, 11, 1);
                break;
      }
      wert_old = -1;
      wert_aendern = true;
    }
    // Änderung im Menupunkt übernehmen
    if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == true) {
      while(EncoderButtonPressed());
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
    }
    // Menu verlassen 
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed());
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
  gfx->fillScreen(COLOR_BACKGROUND);
  const char title[] = "Clear Preferences";
  int MenuepunkteAnzahl = 3;
  const char *menuepunkte[MenuepunkteAnzahl - 1] = {"Lösche Preferences", "Lösche NVS memory", "Zurück"};
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  i = 1;
  while (i > 0) {
    if (digitalRead(BUTTON_STOP) == HIGH  or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
      modus = -1;
      return;
    }
    pos = getRotariesValue(SW_MENU);
    for(int j=0;j<MenuepunkteAnzahl;j++) {
      if (j == pos) {
        gfx->setTextColor(COLOR_MARKER);
      }
      else {
        gfx->setTextColor(COLOR_TEXT);
      }
      if (j < MenuepunkteAnzahl - 1) {
        gfx->setCursor(10, 30+((j+1) * y_offset_tft));
        gfx->print(menuepunkte[j]);
      }
      else {
        gfx->setCursor(10, 30+(7 * y_offset_tft));
        gfx->print(menuepunkte[j]);
      }
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
  int j=0;
  int k = 0;
  int y_offset_tft = 28;
  bool change = false;
  int wert_old = -1;
  const char title[] = "INA219 Setup";
  int MenuepunkteAnzahl = 3;
  const char *menuepunkte_1[MenuepunkteAnzahl] = {"Servo Strom", "Cal. Quetschhan", "Speichern"};
  const char *menuepunkte_2[MenuepunkteAnzahl] = {"Servo Strom", "Zeige Strom an", "Speichern"};
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setFont(Punk_Mono_Bold_240_150);
  int x_pos = CenterPosX(title, 14, 320);
  gfx->setCursor(x_pos, 25);
  gfx->println(title);
  gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
  initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
  i = 1;
  while (i > 0) {
    if (digitalRead(BUTTON_STOP) == HIGH  or digitalRead(SWITCH_SETUP) == LOW) {
      while (digitalRead(BUTTON_STOP) == HIGH);
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
      gfx->setTextColor(COLOR_TEXT);
      if (a == pos and wert_aendern == false) {
        gfx->setTextColor(COLOR_MARKER);
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
          gfx->setTextColor(COLOR_MARKER);
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
            gfx->fillRect(215, 27+((a+1) * y_offset_tft)-19, 100, 25, COLOR_BACKGROUND);
            if (wert_old == 0 or wert_old == 50) {                                                //nicht ganz sauber bei den Schwellwerten. 
              gfx->fillRect(0, 27+((a+2) * y_offset_tft)-19, 320, 27, COLOR_BACKGROUND);
            }
          }
          else {
            gfx->fillRect(251, 27+((a+1) * y_offset_tft)-19, 64, 25, COLOR_BACKGROUND);
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
      while(EncoderButtonPressed());
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
      while(EncoderButtonPressed());
      initRotaries(SW_MENU, menuitem, 0, menuitem_used, -1);
      wert_aendern = false;
    }
    // Menu verlassen 
    if (EncoderButtonPressed() && menuitem == 6) {
      while(EncoderButtonPressed());
      gfx->setCursor(283, 30+(7 * y_offset_tft));
      gfx->print("OK");
      delay(1000);
      modus = -1;
      i = 0;
    }
    while (j > 0) {
      if (digitalRead(SWITCH_SETUP) == LOW) {
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
        gfx->fillScreen(COLOR_BACKGROUND);
        gfx->setTextColor(COLOR_TEXT);
        gfx->setFont(Punk_Mono_Bold_240_150);
        x_pos = CenterPosX(title, 14, 320);
        gfx->setCursor(x_pos, 25);
        gfx->println(title);
        gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
        gfx->setCursor(5, 30+(3 * y_offset_tft));
        sprintf(ausgabe,"max. Strom:     %4imA", current_servo - 30);
        gfx->print(ausgabe);
        gfx->setCursor(5, 30+(4 * y_offset_tft));
        sprintf(ausgabe,"min. Winkel:      %3i°", winkel_min);
        gfx->print(ausgabe);
        j++;
      }
      if (wert_aendern == false && menuitem < menuitem_used && kalibration_status == "Start") {
        gfx->setTextColor(COLOR_MARKER);
        gfx->setCursor(5, 30+(1 * y_offset_tft));
        gfx->print(kalibration_status);
        gfx->setTextColor(COLOR_TEXT);
        gfx->setCursor(5, 30+(7 * y_offset_tft));
        gfx->print("Zurück");
      }
      else {
        gfx->setTextColor(COLOR_TEXT);
        gfx->setCursor(5, 30+(1 * y_offset_tft));
        gfx->print(kalibration_status);
        gfx->setTextColor(COLOR_MARKER);
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
        while(EncoderButtonPressed());
        wert_aendern = true;
      }
      // Änderung im Menupunkt übernehmen
      if (EncoderButtonPressed() && menuitem < menuitem_used  && wert_aendern == true) {
        while(EncoderButtonPressed());
        initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
        wert_aendern = false;
      }
      //verlassen
      if ((EncoderButtonPressed() && menuitem == 6) or digitalRead(BUTTON_STOP) == HIGH) {
        while(EncoderButtonPressed() or digitalRead(BUTTON_STOP) == HIGH);
        gfx->fillScreen(COLOR_BACKGROUND);
        x_pos = CenterPosX(title, 14, 320);
        gfx->setCursor(x_pos, 25);
        gfx->println(title);
        gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
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
            gfx->fillScreen(COLOR_BACKGROUND);
            gfx->setTextColor(COLOR_TEXT);
            gfx->setFont(Punk_Mono_Bold_240_150);
            x_pos = CenterPosX(title, 14, 320);
            gfx->setCursor(x_pos, 25);
            gfx->println(title);
            gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
            gfx->setCursor(5, 30+(3 * y_offset_tft));
            change = true;
            k++;
          }
          if (change) {
            gfx->setTextColor(COLOR_TEXT);
            gfx->setCursor(5, 30+(1 * y_offset_tft));
            gfx->print(kalibration_status);
            gfx->fillRect(205, 27+(3 * y_offset_tft)-19, 110, 3 * y_offset_tft, COLOR_BACKGROUND);
            gfx->setCursor(5, 30+(3 * y_offset_tft));
            sprintf(ausgabe,"Strom:          %4imA", current_mA);
            gfx->print(ausgabe);
            gfx->setCursor(5, 30+(4 * y_offset_tft));
            sprintf(ausgabe,"Winkel:           %3i°", cal_winkel);
            gfx->print(ausgabe);
            gfx->setCursor(5, 30+(5 * y_offset_tft));
            sprintf(ausgabe,"Quetschhan:      %5s", quetschhan);
            gfx->print(ausgabe);
            gfx->setTextColor(COLOR_MARKER);
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
          if (EncoderButtonPressed() or digitalRead(BUTTON_STOP) == HIGH) {
            while(EncoderButtonPressed() or digitalRead(BUTTON_STOP) == HIGH);
            gfx->fillScreen(COLOR_BACKGROUND);
            gfx->setTextColor(COLOR_TEXT);
            x_pos = CenterPosX(title, 14, 320);
            gfx->setCursor(x_pos, 25);
            gfx->println(title);
            gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
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
            gfx->setTextColor(COLOR_TEXT);
            gfx->fillRect(0, 27+(1 * y_offset_tft)-19, 320, 25, COLOR_BACKGROUND);
            gfx->fillRect(0, 27+(5 * y_offset_tft)-19, 320, 25, COLOR_BACKGROUND);
            gfx->fillRect(0, 27+(7 * y_offset_tft)-19, 320, 25, COLOR_BACKGROUND);
            gfx->setCursor(5, 30+(1 * y_offset_tft));
            gfx->print(kalibration_status);
            gfx->setTextColor(COLOR_MARKER);
            gfx->setCursor(5, 30+(7 * y_offset_tft));
            gfx->print("Speichern");
            gfx->setTextColor(COLOR_TEXT);
            k++;
          }
          if (change) {
            gfx->fillRect(200, 27+(3 * y_offset_tft)-19, 120, 25, COLOR_BACKGROUND);
            gfx->setCursor(5, 30+(3 * y_offset_tft));
            sprintf(ausgabe,"Strom:          %4imA", current_mA);
            gfx->print(ausgabe);
            change = false;
          }
          //verlassen
          if (digitalRead(BUTTON_STOP) == HIGH) {
            while(digitalRead(BUTTON_STOP) == HIGH);
            k = 0;
            wert_aendern = false;
            menuitem_used = 2;
            winkel_min = lastwinkel_min;
            initRotaries(SW_MENU, 0, 0, menuitem_used, -1);
            gfx->fillScreen(COLOR_BACKGROUND);
            x_pos = CenterPosX(title, 14, 320);
            gfx->setCursor(x_pos, 25);
            gfx->println(title);
            gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
          }
          if (EncoderButtonTapped()) {
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
  int MenuepunkteAnzahl = 10;
  int menuitem_old = -1;
  //if (bINA219_installed) {MenuepunkteAnzahl++;}
  int posmenu[MenuepunkteAnzahl];
  const char *menuepunkte[MenuepunkteAnzahl] = {"1-Tarawerte","2-Kalibrieren","3-Füllmenge","4-Automatik","5-Servoeinst.","6-Parameter","7-Zählwerk","8-ZählwerkTrip","9-INA219 Setup", "10-Clear Prefs"};
  //MenuepunkteAnzahl = sizeof(menuepunkte)/sizeof(menuepunkte[0]);
  //if (bINA219_installed) {
  //    menuepunkte[MenuepunkteAnzahl - 1] = menuepunkte[MenuepunkteAnzahl -2];    //Clear Pref eins nach hinten schieben
  //    menuepunkte[MenuepunkteAnzahl - 2] = "INA219 Setup";
  //}
  modus = MODE_SETUP;
  winkel = winkel_min;          // Hahn schliessen
  servo_aktiv = 0;              // Servo-Betrieb aus
  SERVO_WRITE(winkel);
  rotary_select = SW_MENU;
  initRotaries(SW_MENU, lastpos, -1, MenuepunkteAnzahl, 1);
  while (modus == MODE_SETUP and (digitalRead(SWITCH_SETUP)) == HIGH) {
  
  if(start_button_very_long_pressed > 200)
  { // pressed for > 4 seconds
     start_button_very_long_pressed = 0;
     bOldMenu = ! bOldMenu;
     break; // out of here
  }   



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
      gfx->fillScreen(COLOR_BACKGROUND);
      gfx->drawLine(0, 100, 320, 100, COLOR_TEXT);
      gfx->drawLine(0, 139, 320, 139, COLOR_TEXT);
      gfx->setTextColor(COLOR_TEXT);
      gfx->setFont(Punk_Mono_Bold_320_200);
      x_pos = CenterPosX(menuepunkte[posmenu[0]], 18, 320);
      gfx->setCursor(x_pos, 130);
      gfx->println(menuepunkte[posmenu[0]]);
      gfx->setTextColor(COLOR_MENU_POS1);
      gfx->setFont(Punk_Mono_Bold_240_150);
      x_pos = CenterPosX(menuepunkte[posmenu[MenuepunkteAnzahl-1]], 14, 320);
      gfx->setCursor(x_pos, 86);
      gfx->println(menuepunkte[posmenu[MenuepunkteAnzahl-1]]);
      x_pos = CenterPosX(menuepunkte[posmenu[1]], 14, 320);
      gfx->setCursor(x_pos, 169);
      gfx->println(menuepunkte[posmenu[1]]);
      gfx->setTextColor(COLOR_MENU_POS2);
      gfx->setFont(Punk_Mono_Bold_200_125);                   //9 x 11 Pixel
      x_pos = CenterPosX(menuepunkte[posmenu[MenuepunkteAnzahl-2]], 12, 320);
      gfx->setCursor(x_pos, 60);
      gfx->println(menuepunkte[posmenu[MenuepunkteAnzahl-2]]);
      x_pos = CenterPosX(menuepunkte[posmenu[2]], 12, 320);
      gfx->setCursor(x_pos, 192);
      gfx->println(menuepunkte[posmenu[2]]);
      gfx->setTextColor(COLOR_MENU_POS3);
      gfx->setFont(Punk_Mono_Bold_160_100);                   //7 x 9 Pixel
      x_pos = CenterPosX(menuepunkte[posmenu[MenuepunkteAnzahl-3]], 9, 320);
      gfx->setCursor(x_pos, 37);
      gfx->println(menuepunkte[posmenu[MenuepunkteAnzahl-3]]);    
      x_pos = CenterPosX(menuepunkte[posmenu[3]], 9, 320);
      gfx->setCursor(x_pos, 213);
      gfx->println(menuepunkte[posmenu[3]]);
      gfx->setTextColor(COLOR_MENU_POS4);
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
      if (menuepunkte[menuitem] == "1-Tarawerte")     setupTara();              // Tara 
      if (menuepunkte[menuitem] == "2-Kalibrieren")   setupCalibration();       // Kalibrieren 
      if (menuepunkte[menuitem] == "3-Füllmenge")     setupFuellmenge();        // Füllmenge 
      if (menuepunkte[menuitem] == "4-Automatik")     setupAutomatik();         // Autostart/Autokorrektur konfigurieren 
      if (menuepunkte[menuitem] == "5-Servoeinst.")   setupServoWinkel();       // Servostellungen Minimum, Maximum und Feindosierung
      if (menuepunkte[menuitem] == "6-Parameter")     setupParameter();         // Sonstige Einstellungen
      if (menuepunkte[menuitem] == "7-Zählwerk")      setupCounter();           // Zählwerk
      if (menuepunkte[menuitem] == "8-ZählwerkTrip")  setupTripCounter();       // Zählwerk Trip
      if (menuepunkte[menuitem] == "9-INA219 Setup")  setupINA219();            // INA219 Setup
      setPreferences();
      if (menuepunkte[menuitem] == "10-Clear Prefs")   setupClearPrefs();        // EEPROM löschen
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
     gewicht_vorher = Jars[fmenge_index].Gewicht + korrektur;
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
    // gfx->fillScreen(COLOR_BACKGROUND);
    //gfx->setTextColor(COLOR_TEXT);
    //gfx->setFont(Punk_Mono_Bold_320_200);
    //sprintf(ausgabe,"Automatik");
    //x_pos = CenterPosX(ausgabe, 18, 320);
    //gfx->setCursor(x_pos, 27);
    //gfx->print(ausgabe);
    gfx->drawLine(0, 30, 320, 30, COLOR_TEXT);
    gfx->drawLine(0, 184, 320, 184, COLOR_TEXT);
    gfx->drawLine(160, 184, 160, 240, COLOR_TEXT);
    gfx->setCursor(2, 202);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->print("INA219");
    gfx->setFont(Checkbox);
    gfx->setCursor(140, 199);
    if (bINA219_installed == 0){
      gfx->setTextColor(RED);
      gfx->print("B");
    }
    else {
      gfx->setTextColor(GREEN);
      gfx->print("A");
    }
    gfx->setTextColor(COLOR_TEXT);
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
    gfx->setTextColor(COLOR_TEXT);
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
    gfx->setTextColor(COLOR_TEXT);
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
    gfx->drawLine(0, 50, 320, 50, COLOR_TEXT);
  }
  pos = getRotariesValue(SW_WINKEL);
  // nur bis winkel_fein regeln, oder über initRotaries lösen?
  if (pos < winkel_fein * 100 / winkel_max) {                      
    pos = winkel_fein * 100 / winkel_max;
    setRotariesValue(SW_WINKEL, pos);
  }
  korrektur    = getRotariesValue(SW_KORREKTUR);
  fmenge_index = getRotariesValue(SW_MENU);
  fmenge       = Jars[fmenge_index].Gewicht;
  tara         = Jars[fmenge_index].Tara;
  if (tara <= 0) { 
    auto_aktiv = 0;
  }
  // wir starten nur, wenn das Tara für die Füllmenge gesetzt ist!
  // Ein erneuter Druck auf Start erzwingt die Aktivierung des Servo
  if (deb_start_button && tara > 0) {
    while(deb_start_button)delay(10); // wait until button released
    if (auto_aktiv == 1) { 
      TFT_line_print(0, "AUTOMATIC FILLING");
      erzwinge_servo_aktiv = 1;
      #ifdef isDebug
        Serial.println("erzwinge Servo aktiv");      
      #endif
    }
    if(erzwinge_servo_aktiv)TFT_line_print(0, "AUTOMATIC FILLING");
    else TFT_line_print(0, "AUTOMATIC WAITING");
    auto_aktiv    = 1;                            // automatisches Füllen aktivieren
    rotary_select = SW_WINKEL;                    // falls während der Parameter-Änderung auf Start gedrückt wurde    
    setPreferences();                             // falls Parameter über den Rotary verändert wurden
    glas_alt = -1;                              // Glas Typ Farbe zurücksetzen fals markiert ist
    korr_alt = -99999;                          // Korrektur Farbe zurücksetzen fals markiert ist
  }
  if (deb_stop_button) {
    winkel      = winkel_min + offset_winkel;
    servo_aktiv = 0;
    auto_aktiv  = 0;
    tara_glas   = 0;
    TFT_line_print(0, "AUT0MATIC PAUSED");
  }

  gewicht = round(LoadCell.getData()) - tara;

  // Glas entfernt -> Servo schliessen
  if (gewicht < -20) 
  { winkel      = winkel_min + offset_winkel;
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
    gfx->fillRect(12, 74, 38, 38, COLOR_BACKGROUND);
    gfx->setFont(Icons_Start_Stop);
    gfx->setCursor(5, 118);
    gfx->print("M");
    gfx->setCursor(5, 118);
    gfx->setTextColor(GREEN);
    gfx->print("A");
    //Korrektur Wert auf Textfarbe setzen
    gfx->setTextColor(COLOR_TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 219);
    sprintf(ausgabe, "%5ig", korrektur + autokorrektur_gr);
    gfx->print(ausgabe);
    gfx->setTextColor(COLOR_TEXT);
    //Glasgewicht auf Textfarbe Setzen
    gfx->setCursor(2, 176);
    gfx->fillRect(0, 156, 320, 27, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_240_150);
    sprintf(ausgabe, "%dg ", (Jars[fmenge_index].Gewicht));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_120_075);
    gfx->setCursor(2 + 14*StringLenght(ausgabe), 168);
    sprintf(ausgabe, "+%dg ", (kulanz_gr));
    gfx->print(ausgabe);
    // END:   nicht schön aber es funktioniert
    gfx->fillRect(80, 54, 240, 80, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_600_375);
    gfx->setCursor(100, 115);
    gfx->print("START");
    // kurz warten und prüfen ob das Gewicht nicht nur eine zufällige Schwankung war 
    delay(1500);  
    gewicht = round(LoadCell.getData()) - tara;
    if (abs(gewicht) <= glastoleranz) {
      tara_glas   = gewicht;
      gfx->fillRect(109, 156, 211, 27, COLOR_BACKGROUND);
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
        Jars[fmenge_index].TripCount++;
        Jars[fmenge_index].Count++;
        gezaehlt = true;
      }
      #ifdef isDebug
        Serial.print("Nachtropfen:");
        Serial.print(" gewicht: ");        Serial.print(gewicht);
        Serial.print(" gewicht_vorher: "); Serial.print(gewicht_vorher);
        Serial.print(" sammler_num: ");    Serial.print(sammler_num);
        Serial.print(" Korrektur: ");      Serial.println(autokorrektur_gr);
        Serial.print(" Zähler Trip: ");    Serial.print(Jars[fmenge_index].TripCount); //Kud
        Serial.print(" Zähler: ");         Serial.println(Jars[fmenge_index].Count); //Kud
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
      Jars[fmenge_index].TripCount++;
      Jars[fmenge_index].Count++;
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
  if (bINA219_installed && (current_servo > 0 or show_current == 1)) {
    y_offset = 4;
  }
  if (bINA219_installed && (current_mA != current_mA_alt) && ((current_servo > 0) || (show_current == 1))) {
    gfx->fillRect(260, 187, 70, 18, COLOR_BACKGROUND);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 202);
    if (current_mA > current_servo and current_servo > 0) {
      gfx->setTextColor(RED);
    }
    sprintf(ausgabe, "%4imA", current_mA);
    gfx->print(ausgabe);
    gfx->setTextColor(COLOR_TEXT);
    current_mA_alt = current_mA;
  }
  else if (bINA219_installed && no_ina == 0 && show_current == 0) {
    no_ina = true;
    gfx->setTextColor(COLOR_TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 202);
    gfx->print("   aus");
  }
  else if (bINA219_installed && no_ina == 0) {
    no_ina = true;
    gfx->setTextColor(COLOR_TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 202);
    gfx->print("    NA");
  }
  if (korr_alt != korrektur + autokorrektur_gr) {
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setTextColor(COLOR_BACKGROUND);
    gfx->setCursor(265, 219);
    sprintf(ausgabe, "%5ig", korr_alt);
    gfx->print(ausgabe);
    gfx->setTextColor(COLOR_TEXT);
    if (rotary_select == SW_KORREKTUR and servo_aktiv == 0) {
      gfx->setTextColor(COLOR_MARKER);
    }
    gfx->setCursor(265, 219);
    sprintf(ausgabe, "%5ig", korrektur + autokorrektur_gr);
    gfx->print(ausgabe);
    gfx->setTextColor(COLOR_TEXT);
    korr_alt = korrektur + autokorrektur_gr;
  }
  if (autokorr_gr_alt != autokorrektur_gr) {
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setTextColor(COLOR_BACKGROUND);
    gfx->setCursor(265, 236);
    sprintf(ausgabe, "%5ig", autokorr_gr_alt);
    gfx->print(ausgabe);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(265, 236);
    sprintf(ausgabe, "%5ig", autokorrektur_gr);
    gfx->print(ausgabe);
    autokorr_gr_alt = autokorrektur_gr;
  }
  if ((glas_alt != fmenge_index and servo_aktiv == 0 and gewicht <= Jars[fmenge_index].Gewicht - tara) or (glas_alt != fmenge_index and rotary_select_alt == SW_KORREKTUR)) {
    if (rotary_select == SW_MENU and servo_aktiv == 0) {
      gfx->setTextColor(COLOR_MARKER);
    }
    gfx->setCursor(2, 176);
    gfx->fillRect(0, 156, 320, 27, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_240_150);
    sprintf(ausgabe, "%dg ", (Jars[fmenge_index].Gewicht));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_120_075);
    gfx->setCursor(2 + 14*StringLenght(ausgabe), 168);
    sprintf(ausgabe, "+%dg ", (kulanz_gr));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_240_150);
    gfx->setCursor(110, 176);
    gfx->print(JarNames[Jars[fmenge_index].GlasTyp].name);

    gfx->setTextColor(COLOR_TEXT);
    glas_alt = fmenge_index;
  }
  if (winkel_min  + offset_winkel != winkel_min_alt) {
    gfx->setTextColor(COLOR_TEXT);
    gfx->fillRect(37, 33, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(38, 47);
    sprintf(ausgabe, "%3i°", winkel_min + offset_winkel);
    gfx->print(ausgabe);
    winkel_min_alt = winkel_min + offset_winkel;
  }
  if (pos != pos_alt) {
    gfx->fillRect(154, 33, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(155, 47);
    sprintf(ausgabe, "%3i°", winkel_max*pos/100);
    gfx->print(ausgabe);
    pos_alt = pos;
  }
  if (winkel != winkel_ist_alt) {
    gfx->fillRect(271, 33, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(272, 47);
    sprintf(ausgabe, "%3i°", winkel);
    gfx->print(ausgabe);
    winkel_ist_alt = winkel;
  }
  if (auto_aktiv != auto_aktiv_alt) {
    gfx->fillRect(12, 74, 38, 38, COLOR_BACKGROUND);
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
    gfx->setTextColor(COLOR_TEXT);
  }
  if (tara > 0) {
    if (gewicht < -20 and gewicht != gewicht_alt) {
      gfx->fillRect(80, 54, 240, 80, COLOR_BACKGROUND);
      gfx->setFont(Punk_Mono_Bold_320_200);
      gfx->setCursor(120, 88);
      gfx->print("Bitte Glas");
      gfx->setCursor(120, 120);
      gfx->print("aufstellen");
      glas_alt = -1;
    } 
    else if (gewicht != gewicht_alt) {
      gfx->fillRect(80, 54, 240, 80, COLOR_BACKGROUND);
      gfx->setFont(Punk_Mono_Bold_600_375);
      gfx->setCursor(100, 115);
      sprintf(ausgabe, "%5ig", gewicht - tara_glas);
      gfx->print(ausgabe);
    }
    if (gewicht != gewicht_alt) {
      progressbar = 318.0*((float)gewicht/(float)(Jars[fmenge_index].Gewicht));
      progressbar = constrain(progressbar,0,318);
      gfx->drawRect(0, 137, 320, 15, COLOR_TEXT);
      if (Jars[fmenge_index].Gewicht > gewicht) {
        gfx->fillRect  (1, 138, progressbar, 13, RED);
      }
      else if (gewicht >= Jars[fmenge_index].Gewicht and gewicht <= Jars[fmenge_index].Gewicht + kulanz_gr){
        gfx->fillRect  (1, 138, progressbar, 13, GREEN);
      }
      else {
        gfx->fillRect  (1, 138, progressbar, 13, ORANGE);
      }
      gfx->fillRect  (1 + progressbar, 138, 318 - progressbar, 13, COLOR_BACKGROUND);
      gewicht_alt = gewicht;
    }
  }
  else if (gewicht != gewicht_alt) {
    gfx->fillRect(80, 54, 240, 80, COLOR_BACKGROUND);
    gfx->fillRect(0, 137, 320, 15, COLOR_BACKGROUND);
    gfx->setTextColor(RED);
    gfx->setFont(Punk_Mono_Bold_320_200);
    gfx->setCursor(120, 104);
    gfx->print("kein tara!");
    gfx->setTextColor(COLOR_TEXT);
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

void processHandbetrieb(void) 
{ i = 0;
  int y_offset = 0;
  
  if (modus != MODE_HANDBETRIEB) // draw screen just once
  { modus = MODE_HANDBETRIEB;
    winkel = winkel_min;          // Hahn schliessen
    servo_aktiv = 0;              // Servo-Betrieb aus
    SERVO_WRITE(winkel);
    rotary_select = SW_WINKEL;
    tara = 0;
    offset_winkel = 0;            // Offset vom Winkel wird auf 0 gestellt
    int x_pos;
    gewicht_alt = -9999999;
    OldWeight = -1234;
    pos_alt = -1;
    winkel_min_alt = -1;
    tara_alt = -1;
    current_mA_alt = -1;
    servo_aktiv_alt = -1;
  }
  
  pos = getRotariesValue(SW_WINKEL);
  gewicht = round(LoadCell.getData()) - tara;

  if(deb_start_button)servo_aktiv = 1;
  if(deb_stop_button)servo_aktiv = 0;

  if (servo_aktiv != servo_aktiv_alt) 
  { servo_aktiv_alt = servo_aktiv;
    if (servo_aktiv == 1)
    { TFT_line_print(0, "MANUAL MODE DOSING");
      TFT_line_blink(0, true);
      TFT_line_color(5, TFT_GREEN, TFT_DARKGREY);

    }
    else 
    { TFT_line_print(0, "MANUAL MODE PAUSED");
      TFT_line_blink(0, false);
      TFT_line_color(5, TFT_LIGHTGREY, TFT_DARKGREY);
    }
  }

  if(deb_encoder_button)tara = round(LoadCell.getData());
    
  if(servo_aktiv == 1)winkel = ((winkel_max * pos) / 100);
  else winkel = winkel_min + offset_winkel;
  winkel = constrain(winkel, winkel_min + offset_winkel, winkel_max);
  SERVO_WRITE(winkel);

  if (gewicht != gewicht_alt) 
  { gewicht_alt = gewicht;
    NewWeight = gewicht; // NewWeight is used by new graphics handling
  }

  if(((winkel_min + offset_winkel) != winkel_min_alt) || (pos != pos_alt))
  { sprintf(ausgabe, "Servo Min: %3d° Max: %3d°", (winkel_min + offset_winkel), (winkel_max*pos/100));
    TFT_line_print(5, ausgabe);
    winkel_min_alt = winkel_min + offset_winkel;
    pos_alt = pos;
  }

  // tarra value & current value for servo, printed on one line
  if((tara != tara_alt) || (bINA219_installed && (current_mA != current_mA_alt) && (current_servo > 0 or show_current == 1)))
  { sprintf(ausgabe, "Tarra: %3dg", tara);
    if(bINA219_installed && (current_mA != current_mA_alt) && (current_servo > 0 or show_current == 1))
    { sprintf(&ausgabe[strlen(ausgabe)], " Servo: %dmA", current_mA);
    }
    TFT_line_print(4, ausgabe);
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
    i = 0;
    inawatchdog = 1;
    alarm_overcurrent = 0;
  }
  setPreferences();         //irgendwo anderst setzen. es muss nicht jede änderung geschrieben werden
}


volatile int interruptCounter;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() // is called every 10mS
{ int x;
  
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;

  // timer example
  // if(TimeOutMs)
  // { digitalWrite(SOLENOID, HIGH);
  //   TimeOutMs--;
  // }
  // else digitalWrite(SOLENOID, LOW);
  
  if((interruptCounter % 5)==0)bScrollNow = true; // 50ms ticker for the vertical scrolling of text

  if((interruptCounter % 10)==0)
  { bUpdateDisplay = true; // display refresh max 10 times a second
  }

  BlinkTimer10mS++;
  if(BlinkTimer10mS>25)
  { BlinkTimer10mS=0;
    bBlinkDisplay = !bBlinkDisplay;
  }

  if((interruptCounter % 100)==0) // each second
  { 
  }

  EncoderSleep10mS++;
    
  portEXIT_CRITICAL_ISR(&timerMux);
}

//interrupt routine:
void dataReadyISR() {
  if (LoadCell.update()) {
    newDataReady = 1;
  }
}

void setup() 
{ // timer for all sorts of things including lcd display updates
  timer = timerBegin(0, 80, true); // prescaler 1MHz
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true); // elke 10mS
  timerAlarmEnable(timer);
  
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Hanimandl Start");
  
  SetDefaultParameters(); // fills global array with all editable parameters
  LoadParameters(); // pull from eeprom what we have

  tft_colors();
  tft_marker();
  gfx->begin();
  gfx->fillScreen(COLOR_BACKGROUND);
  gfx->setUTF8Print(true);
    
  SetupMyDisplay(); // in hani-display.cpp
  delay(500);

  Serial.println("Pin Config");
  // enable internal pull downs for digital inputs 
  pinMode(BUTTON_START,INPUT_PULLDOWN);
  pinMode(BUTTON_STOP, INPUT_PULLDOWN);
  pinMode(SWITCH_AUTO, INPUT_PULLDOWN);
  pinMode(SWITCH_SETUP, INPUT_PULLDOWN);

  xTaskCreatePinnedToCore(
                    ReadButtons,    // Task to read and debounce buttons and switch 
                    "ReadButtons", // name of task
                    2000,           // Stack size of task 
                    NULL,           // parameter of the task 
                    1,              // priority of the task 
                    NULL,           // Task handle to keep track of created task 
                    0);             // pin task to core 0 

  
  // Check if we have a INA219 current sensor installed or not
  if (ina219.begin()) {
    bINA219_installed = true;
    TFT_line_print(1, "INA219 Installed!");
    TFT_line_color(1, TFT_BLACK, TFT_GREEN);
  }
  else {
    bINA219_installed = false;
    current_servo = 0;                              // ignore INA wenn keiner gefunden wird
    TFT_line_print(1, "INA219 Not Installed!");
    TFT_line_color(1, TFT_BLACK, TFT_RED);
    TFT_line_blink(1, true);
  }
  delay(500);

  // Rotary
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  attachInterrupt(ENCODER_BUTTON, isr1, FALLING);
  pinMode(ROTARY_ENCODER_A,INPUT);
  pinMode(ROTARY_ENCODER_B,INPUT);
  attachInterrupt(ROTARY_ENCODER_A, isr2, CHANGE);
  // Buzzer
  pinMode(buzzer_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);
  // short delay to let chip power up
  delay (100);
  // Preferences aus dem EEPROM lesen
  getPreferences();

  // servo setup initialisation
  servo.attach(SERVO_PIN,  500, 2500); // pulse range for 180 degrees range
//    servo.setPeriodHertz(50);
  SERVO_WRITE(winkel_min); // set valve to closed position

  buzzer(BUZZER_SHORT);

  // new HX711 library used
  LoadCell.begin();
  LoadCell.start(3000, false);
  
  calibrationValue = faktor;
  LoadCell.setCalFactor(calibrationValue); 
  
  if(LoadCell.getTareTimeoutFlag())
  { waage_vorhanden = 0; // have check really
  }
  else 
  { waage_vorhanden = 1; // have check really
  }
  while (!LoadCell.update());
  Serial.print("Calibration value: ");
  Serial.println(LoadCell.getCalFactor());
  Serial.print("HX711 measured conversion time ms: ");
  Serial.println(LoadCell.getConversionTime());
  Serial.print("HX711 measured sampling rate HZ: ");
  Serial.println(LoadCell.getSPS());
  Serial.print("HX711 measured settlingtime ms: ");
  Serial.println(LoadCell.getSettlingTime());
  Serial.println("Note that the settling time may increase significantly if you use delay() in your sketch!");
  if (LoadCell.getSPS() < 7) {
    Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
  }
  else if (LoadCell.getSPS() > 100) {
    Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
  }




  attachInterrupt(digitalPinToInterrupt(HX711_dout), dataReadyISR, FALLING);

  // Setup der Waage, Skalierungsfaktor setzen
  if (waage_vorhanden ==1) {                         // Waage angeschlossen?
    if (faktor == 0) {                               // Vorhanden aber nicht kalibriert
      TFT_line_print(2, "Scale Not Calibrated!");
      TFT_line_color(2, TFT_BLACK, TFT_RED);
      TFT_line_blink(2, true);
      buzzer(BUZZER_ERROR);
    }
    else {                                          // Tara und Skalierung setzen
      // scale.set_scale(faktor);
      LoadCell.setCalFactor(calibrationValue); 
      // scale.set_offset(long(gewicht_leer));
      LoadCell.setTareOffset((long)(gewicht_leer));	

      TFT_line_print(2, "Scale Initialized!");
      TFT_line_color(2, TFT_BLACK, TFT_GREEN);
    }
  }
  else {                                            // Keine Waage angeschlossen
    TFT_line_color(2, TFT_BLACK, TFT_RED);
    TFT_line_print(2, "No Scale Connected!"); 
    TFT_line_blink(2, true);
    delay(5000);

    buzzer(BUZZER_ERROR);
  }
  delay(1000);

  // initiale Kalibrierung des Leergewichts wegen Temperaturschwankungen
  // Falls mehr als 20g Abweichung steht vermutlich etwas auf der Waage.
  if (waage_vorhanden == 1) {
    gewicht = round(LoadCell.getData());
    if ((gewicht > -20) && (gewicht < 20)) 
    {
      LoadCell.tare();
      buzzer(BUZZER_SUCCESS);
      #ifdef isDebugSCALE_GETUNITS
        Serial.print("Tara angepasst um: ");
        Serial.println(gewicht);
      #endif
    }
    else if (faktor != 0) {
      TFT_line_color(3, TFT_BLACK, TFT_RED);
      TFT_line_print(3, "Please Empty Scale"); 
      TFT_line_blink(3, true);
//      #ifdef isDebug
        Serial.print("Gewicht auf der Waage: ");
        Serial.println(gewicht);
        Serial.print("Faktor: ");
        Serial.println(faktor);
        Serial.print("Gewicht Leer: ");
        Serial.println(gewicht_leer);
        
//      #endif
      delay(5000);
      // Neuer Versuch, falls Gewicht entfernt wurde
      gewicht = round(LoadCell.getData());
      if ((gewicht > -20) && (gewicht < 20)) {
        LoadCell.tare();
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
 
  delay(2000);
  TFT_line_print(1, ""); // remove messages, warnings 
  TFT_line_print(2, ""); // remove messages, warnings 
  TFT_line_print(3, ""); // remove messages, warnings 
//  TFT_line_print(5, ""); // remove credits
  delay(2000);
//  tft.fillScreen(TFT_BLACK);
  delay(2000);
  
//  tft_colors();
//  tft_marker();
//  gfx->begin();
//  gfx->fillScreen(COLOR_BACKGROUND);
//  gfx->setUTF8Print(true);




  // die drei Datenstrukturen des Rotaries initialisieren
  initRotaries(SW_WINKEL,    0,   0, 100, 5);     // Winkel
  initRotaries(SW_KORREKTUR, 0, -90,  20, 1);     // Korrektur
  initRotaries(SW_MENU,      0,   0,   7, 1);     // Menuauswahlen
  // Parameter aus den Preferences für den Rotary Encoder setzen
  setRotariesValue(SW_WINKEL,    pos);   
  setRotariesValue(SW_KORREKTUR, korrektur);
  setRotariesValue(SW_MENU,      fmenge_index);
}

void loop() 
{  //INA219 Messung
  if (bINA219_installed and inawatchdog == 1 and (current_servo > 0 or show_current == 1) and (modus == MODE_HANDBETRIEB or modus == MODE_AUTOMATIK)) {
    ina219_measurement();
  }

  //Serial.println(start_button_very_long_pressed);

  if(start_button_very_long_pressed > 200)
  { // pressed for > 4 seconds
     start_button_very_long_pressed = 0;
     bOldMenu = ! bOldMenu;
  }   

  // Setup Menu 
  if(deb_setup_switch) 
  { if(bOldMenu)
    { if (modus != MODE_SETUP)
      { ActLcdMode = 999; // force display build
        SelectMenu(HANI_SETUP);
      } 
      processSetup();
    }
    else Menu(); // new menu style
  }
  // Automatik-Betrieb 
  else if(deb_auto_switch) {
//    if (modus != MODE_AUTOMATIK)
//    { ActLcdMode = 999; // force new display build
//      SelectMenu(HANI_AUTO);
//    }
    processAutomatik2();
    delay(10);
  }
  // Handbetrieb 
  else if(deb_manual_switch) 
  { if (modus != MODE_HANDBETRIEB)
    { ActLcdMode = 999; // force new display build
      SelectMenu(HANI_HAND);
    }
    processHandbetrieb();
  }
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

int CenterPosX(const char a[], float font_width, int display_width) {
  int res = 0;
  int counter = get_length(a);
  res = int(float(display_width - font_width * counter) / 2);
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

void ina219_measurement(void) {
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

