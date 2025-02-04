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
//#include <Arduino_GFX_Library.h>    /* aus dem Bibliotheksverwalter */
#include <TFT_eSPI.h>               // more versatile display library

#include "hani.h"
#include "Arialbd72.h"
#include <HX711_ADC.h>

#define TRANS
#include "language.h"
#include "menustructure.h"


extern void SetupMyDisplay(void);
//extern void TFT_line_print(int line, const char *content);
extern void TFT_line_print(int line, const char *content, bool blink = false);

extern void TFT_line_color(int line, int textcolor, int backgroundcolor);
extern void TFT_line_blink(int line, bool blink);  
extern void SetupButtons(void);
extern void ReadButtons(void * pvParameters);
extern void SelectMenu(int menu);
extern void MenuHandler(void);  // big new menu handler
extern bool IsPulsed(bool *button);

extern bool bScrollNow;
extern bool bUpdateDisplay;
extern bool bBlinkDisplay;
extern int  BlinkTimer10mS;
extern int  Unstable10mS;
int BeepTimes = 0;
int BeepLength = 100;   
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

extern bool bStartButtonPulsed;
extern bool bStopButtonPulsed;
extern bool bEncoderButtonPulsed;
extern bool bSetupSwitchPulsed;
extern bool bAutoSwitchPulsed;
extern bool bManualSwitchPulsed;

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

extern int GramsOnScale;
extern bool bScaleStable;


// setup menu 
int EditMenu = 0; // 


//HX711 constructor:
const int HX711_dout = 35; //mcu > HX711 dout pin, must be external interrupt capable!
const int HX711_sck = 17; //mcu > HX711 sck pin
HX711_ADC LoadCell(HX711_dout, HX711_sck);
float CalibrationFactor;
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


//Arduino_DataBus *bus = new Arduino_HWSPI(13 /* DC */, 5 /* CS */);
//Arduino_GFX *gfx = new Arduino_ST7789(bus, 14 /* RST */, 3 /* rotation */);
//Arduino_GFX *gfx;

// Fonts
//#include "./Fonts/Punk_Mono_Bold_120_075.h"           //10 x 7
//#include "./Fonts/Punk_Mono_Bold_160_100.h"           //13 x 9
//#include "./Fonts/Punk_Mono_Bold_200_125.h"           //16 x 12
//#include "./Fonts/Punk_Mono_Bold_240_150.h"           //19 x 14
//#include "./Fonts/Punk_Mono_Bold_280_175.h"          //22 x 16
//#include "./Fonts/Punk_Mono_Bold_320_200.h"           //25 x 18
//#include "./Fonts/Punk_Mono_Bold_600_375.h"           //48 x 36
//#include "./Fonts/Punk_Mono_Thin_120_075.h"           //10 x 7
//#include "./Fonts/Punk_Mono_Thin_160_100.h"           //13 x 9
//#include "./Fonts/Punk_Mono_Thin_240_150.h"           //19 x 14
//#include "./Fonts/Icons_Start_Stop.h"                 //A=Start, B=Stop, M=Rahmen
//#include "./Fonts/Checkbox.h"                         //A=OK, B=nOK

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

struct rotary rotaries[3]; // will be initialized in setup()
int rotary_select = SW_WINKEL;

JarType JarTypes[6] = {{"DE Imker Bund",-9999},{"TwistOff",-9999},{"DeepTwist",-9999},{"Special Jar",-9999},{"Ferry's DeLuxe",-9999},{"Eco Jar",-9999}}; // name and shortname
ProductParameter Products[6] = {{125, 0, 0, 0}, // net weight, jar type, notused, counter
                        {250, 1, 0, 0},
                        {250, 2, 0, 0},
                        {500, 1, 0, 0},
                        {450, 4, 0, 0},
                        {500, 5, 0, 0}};
 

// Allgemeine Variablen
int i;                              // allgemeine Zählvariable
int pos;                            // aktuelle Position des Poti bzw. Rotary 
int gewicht;                        // aktuelles Gewicht
int tara;                           // Tara für das ausgewählte Glas, für Automatikmodus
int tara_glas;                      // Tara für das aktuelle Glas, falls Glasgewicht abweicht
long rawtareoffset;                  // Gewicht der leeren Waage
int fmenge;                         // ausgewählte Füllmenge
int winkel;                         // aktueller Servo-Winkel
float fein_dosier_gewicht = 60;     // float wegen Berechnung des Schliesswinkels
int servo_aktiv = 0;                // Servo aktivieren ja/nein
char ausgabe[256];                  // FB: Increased lenght for use with long scrolling texts
int modus = -1;                     // Bei Modus-Wechsel den Servo auf Minimum fahren
int auto_aktiv = 0;                 // Für Automatikmodus - System ein/aus?
int waage_vorhanden = 0;            // HX711 nicht ansprechen, wenn keine Waage angeschlossen ist, sonst Crash
long preferences_chksum;            // Checksumme, damit wir nicht sinnlos Prefs schreiben
bool gezaehlt = true;               // Kud Zähl-Flag
int MenuepunkteAnzahl;              // Anzahl Menüpunkte im Setupmenü
int lastpos = 0;                    // Letzte position im Setupmenü
int progressbar = 0;                // Variable für den Progressbar
bool bINA219_installed = false;     
int current_mA;                     // Strom welcher der Servo zieht
int updatetime_ina219 = 500;        // Zeit in ms in welchem der INA219 gelesen wird (500 -> alle 0,5 sek wird eine Strommessung vorgenommen)
int last_ina219_measurement = 0;    // Letzte Zeit in welcher der Strom gemessen wurde
int overcurrenttime = 1500;         // Zeit in welcher der Servo mehr Strom ziehen darf als einfestellt in ms
int last_overcurrenttime = 0;       // Letzte Zeit in welcher keinen Ueberstrom gemessen wurde
int alarm_overcurrent = 0;          // Alarmflag wen der Servo zuwiel Strom zieht
int show_current = 0;               // 0 = aus, 1 = ein / Zeige den Strom an auch wenn der INA ignoriert wird
int inawatchdog = 1;                // 0 = aus, 1 = ein / wird benötigt um INA messung auszusetzen
int offset_winkel = 0;              // Offset in Grad vom Schlieswinkel wen der Servo Überstrom hatte (max +3Grad vom eingestelten Winkel min)

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
void buzzer(int type);

void tft_colors() {
    COLOR_BACKGROUND  = TFT_BLACK;
    COLOR_TEXT        = TFT_WHITE;
    COLOR_MENU_POS1   = 0x73AF;
    COLOR_MENU_POS2   = 0x5AEC;
    COLOR_MENU_POS3   = 0x39C8;
    COLOR_MENU_POS4   = 0x20E6;
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
  static int lastmillis;
  static int actmillis;
  static int speedmultiplier;
  static int newvalue;
  static int timepassed;
  static int multipliers[] = {1,2,4,8,16,32,64};

  aState = digitalRead(ROTARY_ENCODER_A); // Reads the "current" state of the encoder
  bState = digitalRead(ROTARY_ENCODER_B); // Reads the "current" state of the encoder
  if (aState != aLastState) 
  { // speed of rotation calculation
    // single channel puls/pauze is 25mS when turning slow, 10mS for nice turn, 5mS for fast, 2.5ms for a very aggressive twist
    actmillis = millis();
    
    timepassed = actmillis - lastmillis;
//    Serial.println(timepassed);
    speedmultiplier = 0;
    if((timepassed < 26) && (timepassed>1))
    { timepassed = 26-timepassed;
      while(timepassed>0)
      { speedmultiplier++;
        timepassed -= 5;
      }
//      Serial.print("Speed=");
//      Serial.println(speedmultiplier);
    }
    speedmultiplier = multipliers[speedmultiplier];

    lastmillis = actmillis;
    newvalue = rotaries[rotary_select].Value[2];
    // If the outputB state is different to the outputA state, that means the encoder is turned clockwise
    if (aState != bState) {
      newvalue -= rotaries[rotary_select].Step * speedmultiplier;
    }
    else {    // counter-clockwise
      newvalue += rotaries[rotary_select].Step * speedmultiplier;
    }
    rotaries[rotary_select].Value[2] = constrain( newvalue, rotaries[rotary_select].Minimum, rotaries[rotary_select].Maximum);
  }
  aLastState = aState; 
  
}

//
// Skalierung des Rotaries für verschiedene Rotary Encoder
int getRotariesValue( int rotary_mode ) {
  return ((rotaries[rotary_mode].Value[0] - (rotaries[rotary_mode].Value[0] % (rotaries[rotary_mode].Step * ROTARY_SCALE))) / ROTARY_SCALE );
}

void setRotariesValue( int rotary_mode, int rotary_value ) {
  rotaries[rotary_mode].Value[0] = rotary_value * ROTARY_SCALE;
  rotaries[rotary_mode].Value[1] = rotary_value * ROTARY_SCALE;
  rotaries[rotary_mode].Value[2] = rotary_value * ROTARY_SCALE;
}

void initRotaries( int rotary_mode, int rotary_value, int rotary_min, int rotary_max, int rotary_step ) {
  rotaries[rotary_mode].Value[0]     = rotary_value * ROTARY_SCALE;
  rotaries[rotary_mode].Value[1]     = rotary_value * ROTARY_SCALE;
  rotaries[rotary_mode].Value[2]     = rotary_value * ROTARY_SCALE;
  rotaries[rotary_mode].Minimum   = rotary_min   * ROTARY_SCALE;
  rotaries[rotary_mode].Maximum   = rotary_max   * ROTARY_SCALE;
  rotaries[rotary_mode].Step      = rotary_step;
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



/*
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
     winkel = SysParams[SERVOMIN];          // Hahn schliessen
     servo_aktiv = 0;              // Servo-Betrieb aus
     SERVO_WRITE(winkel);
     auto_aktiv = 0;               // automatische Füllung starten
     tara_glas = 0;
     rotary_select = SW_WINKEL;    // Einstellung für Winkel über Rotary
     offset_winkel = 0;            // Offset vom Winkel wird auf 0 gestellt
     initRotaries(SW_MENU, SysParams[CHOSENPRODUCT], 0, 5, 1);
     gewicht_vorher = Products[SysParams[CHOSENPRODUCT]].Gewicht + SysParams[CORRECTION];
     int x_pos;
    if (SysParams[SERVOMAXCURRENT] > 0) {
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
    if (SysParams[AUTOSTART] == 0){
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
    if (SysParams[AUTO_CORRECTION] == 0){
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
    sprintf(ausgabe,"Min:         Max:         Ist:");
    gfx->print(ausgabe);
    gfx->drawLine(0, 50, 320, 50, COLOR_TEXT);
  }
  pos = getRotariesValue(SW_WINKEL);
  // nur bis SysParams[SERVOSLOWDOS] regeln, oder über initRotaries lösen?
  if (pos < SysParams[SERVOSLOWDOS] * 100 / SysParams[SERVOMIN]) {                      
    pos = SysParams[SERVOSLOWDOS] * 100 / SysParams[SERVOMIN];
    setRotariesValue(SW_WINKEL, pos);
  }
  SysParams[CORRECTION]    = getRotariesValue(SW_KORREKTUR);
  SysParams[CHOSENPRODUCT] = getRotariesValue(SW_MENU);
  fmenge       = Products[SysParams[CHOSENPRODUCT]].Gewicht;
  tara         = JarTypes[SysParams[CHOSENPRODUCT]].tarra;
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
//    setPreferences();                             // falls Parameter über den Rotary verändert wurden
    glas_alt = -1;                              // Glas Typ Farbe zurücksetzen fals markiert ist
    korr_alt = -99999;                          // Korrektur Farbe zurücksetzen fals markiert ist
  }
  if (deb_stop_button) {
    winkel      = SysParams[SERVOMIN] + offset_winkel;
    servo_aktiv = 0;
    auto_aktiv  = 0;
    tara_glas   = 0;
    TFT_line_print(0, "AUT0MATIC PAUSED");
  }

  gewicht = GramsOnScale - tara;

  // Glas entfernt -> Servo schliessen
  if (gewicht < -20) 
  { winkel      = SysParams[SERVOMIN] + offset_winkel;
    servo_aktiv = 0;
    tara_glas   = 0;
    if (SysParams[AUTOSTART] != 1) {  // Autostart nicht aktiv
      auto_aktiv  = 0;
    }
  }
  
  // Automatik ein, leeres Glas aufgesetzt, Servo aus -> Glas füllen
  if (auto_aktiv == 1 && abs(gewicht) <= SysParams[AUTO_JAR_TOLERANCE] && servo_aktiv == 0) {
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
    sprintf(ausgabe, "%5ig", SysParams[CORRECTION] + autokorrektur_gr);
    gfx->print(ausgabe);
    gfx->setTextColor(COLOR_TEXT);
    //Glasgewicht auf Textfarbe Setzen
    gfx->setCursor(2, 176);
    gfx->fillRect(0, 156, 320, 27, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_240_150);
    sprintf(ausgabe, "%dg ", (Products[SysParams[CHOSENPRODUCT]].Gewicht));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_120_075);
    gfx->setCursor(2 + 14*StringLenght(ausgabe), 168);
    sprintf(ausgabe, "+%dg ", (SysParams[COULANCE]));
    gfx->print(ausgabe);
    // END:   nicht schön aber es funktioniert
    gfx->fillRect(80, 54, 240, 80, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Bold_600_375);
    gfx->setCursor(100, 115);
    gfx->print("START");
    // kurz warten und prüfen ob das Gewicht nicht nur eine zufällige Schwankung war 
    delay(1500);  
    gewicht = GramsOnScale - tara;
    if (abs(gewicht) <= SysParams[AUTO_JAR_TOLERANCE]) {
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
        Serial.print(" zielgewicht: ");       Serial.print(fmenge + SysParams[CORRECTION] + tara_glas + autokorrektur_gr);
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
  zielgewicht = fmenge + SysParams[CORRECTION] + tara_glas + autokorrektur_gr;
  // Anpassung des Autokorrektur-Werts
  if (SysParams[AUTO_CORRECTION] == 1) {                                                       
    if ( auto_aktiv == 1 && servo_aktiv == 0 && winkel == SysParams[SERVOMIN] + offset_winkel && gewicht >= zielgewicht && sammler_num <= 5) {     
      voll = true;                       
      if (gewicht == gewicht_vorher && sammler_num < 5) { // wir wollen 5x das identische Gewicht sehen  
        sammler_num++;
      } 
      else if (gewicht != gewicht_vorher) {               // sonst gewichtsänderung nachführen
        gewicht_vorher = gewicht;
        sammler_num = 0;
      } 
      else if (sammler_num == 5) {                        // gewicht ist 5x identisch, autokorrektur bestimmen
        autokorrektur_gr = (fmenge + SysParams[COULANCE] + tara_glas) - (gewicht - autokorrektur_gr);
        if (SysParams[CORRECTION] + autokorrektur_gr > SysParams[COULANCE]) {   // Autokorrektur darf nicht überkorrigieren, max Füllmenge plus Kulanz
          autokorrektur_gr = SysParams[COULANCE] - SysParams[CORRECTION];
          #ifdef isDebug
            Serial.print("Autokorrektur begrenzt auf ");
            Serial.println(autokorrektur_gr);
          #endif
        }
        buzzer(BUZZER_SUCCESS);
        sammler_num++;                                      // Korrekturwert für diesen Durchlauf erreicht
      }
      if (voll == true && gezaehlt == false) {
        Products[SysParams[CHOSENPRODUCT]].Count++;
        gezaehlt = true;
      }
      #ifdef isDebug
        Serial.print("Nachtropfen:");
        Serial.print(" gewicht: ");        Serial.print(gewicht);
        Serial.print(" gewicht_vorher: "); Serial.print(gewicht_vorher);
        Serial.print(" sammler_num: ");    Serial.print(sammler_num);
        Serial.print(" Korrektur: ");      Serial.println(autokorrektur_gr);
        Serial.print(" Zähler: ");         Serial.println(Products[SysParams[CHOSENPRODUCT]].Count); //Kud
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
    winkel = (SysParams[SERVOMIN] * pos / 100);
  }
  if (servo_aktiv == 1 && (zielgewicht - gewicht <= fein_dosier_gewicht)) {
    winkel = ((SysParams[SERVOMIN]*pos / 100) * ((zielgewicht-gewicht) / fein_dosier_gewicht) );
  }
  if (servo_aktiv == 1 && winkel <= SysParams[SERVOSLOWDOS]) {
    winkel = SysParams[SERVOSLOWDOS];
  }
  // Glas ist voll
  if (servo_aktiv == 1 && gewicht >= zielgewicht) {
    winkel      = SysParams[SERVOMIN] + offset_winkel;
    servo_aktiv = 0;
    if (gezaehlt == false) {
      Products[SysParams[CHOSENPRODUCT]].Count++;
      gezaehlt = true;
    }
    if (SysParams[AUTOSTART] != 1)       // autostart ist nicht aktiv, kein weiterer Start
      auto_aktiv = 0;
    if (SysParams[AUTO_CORRECTION] == 1)   // autokorrektur, gewicht merken
      gewicht_vorher = gewicht;
    buzzer(BUZZER_SHORT);
  }
  SERVO_WRITE(winkel);
  if (bINA219_installed && (SysParams[SERVOMAXCURRENT] > 0 or show_current == 1)) {
    y_offset = 4;
  }
  if (bINA219_installed && (current_mA != current_mA_alt) && ((SysParams[SERVOMAXCURRENT] > 0) || (show_current == 1))) {
    gfx->fillRect(260, 187, 70, 18, COLOR_BACKGROUND);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(265, 202);
    if (current_mA > SysParams[SERVOMAXCURRENT] and SysParams[SERVOMAXCURRENT] > 0) {
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
  if (korr_alt != SysParams[CORRECTION] + autokorrektur_gr) {
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
    sprintf(ausgabe, "%5ig", SysParams[CORRECTION] + autokorrektur_gr);
    gfx->print(ausgabe);
    gfx->setTextColor(COLOR_TEXT);
    korr_alt = SysParams[CORRECTION] + autokorrektur_gr;
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
  if ((glas_alt != SysParams[CHOSENPRODUCT] and servo_aktiv == 0 and gewicht <= Products[SysParams[CHOSENPRODUCT]].Gewicht - tara) or (glas_alt != SysParams[CHOSENPRODUCT] and rotary_select_alt == SW_KORREKTUR)) {
    if (rotary_select == SW_MENU and servo_aktiv == 0) {
      gfx->setTextColor(COLOR_MARKER);
    }
    gfx->setCursor(2, 176);
    gfx->fillRect(0, 156, 320, 27, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_240_150);
    sprintf(ausgabe, "%dg ", (Products[SysParams[CHOSENPRODUCT]].Gewicht));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_120_075);
    gfx->setCursor(2 + 14*StringLenght(ausgabe), 168);
    sprintf(ausgabe, "+%dg ", (SysParams[COULANCE]));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_240_150);
    gfx->setCursor(110, 176);
    gfx->print(JarTypes[Products[SysParams[CHOSENPRODUCT]].GlasTyp].name);

    gfx->setTextColor(COLOR_TEXT);
    glas_alt = SysParams[CHOSENPRODUCT];
  }
  if (SysParams[SERVOMIN]  + offset_winkel != winkel_min_alt) {
    gfx->setTextColor(COLOR_TEXT);
    gfx->fillRect(37, 33, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(38, 47);
    sprintf(ausgabe, "%3i°", SysParams[SERVOMIN] + offset_winkel);
    gfx->print(ausgabe);
    winkel_min_alt = SysParams[SERVOMIN] + offset_winkel;
  }
  if (pos != pos_alt) {
    gfx->fillRect(154, 33, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(155, 47);
    sprintf(ausgabe, "%3i°", SysParams[SERVOMIN]*pos/100);
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
      progressbar = 318.0*((float)gewicht/(float)(Products[SysParams[CHOSENPRODUCT]].Gewicht));
      progressbar = constrain(progressbar,0,318);
      gfx->drawRect(0, 137, 320, 15, COLOR_TEXT);
      if (Products[SysParams[CHOSENPRODUCT]].Gewicht > gewicht) {
        gfx->fillRect  (1, 138, progressbar, 13, RED);
      }
      else if (gewicht >= Products[SysParams[CHOSENPRODUCT]].Gewicht and gewicht <= Products[SysParams[CHOSENPRODUCT]].Gewicht + SysParams[COULANCE]){
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
    if (servo.read() <= SysParams[SERVOMIN]  + offset_winkel and offset_winkel < 3) {
      while(offset_winkel < 3 and SysParams[SERVOMAXCURRENT] < current_mA) {
        offset_winkel = offset_winkel + 1;
        SERVO_WRITE(SysParams[SERVOMIN] + offset_winkel);
        current_mA = GetCurrent(10);
        delay(1000);
      }
      alarm_overcurrent = 0;
    }
    i = 0;
    inawatchdog = 1;
  }
  //setPreferences();
}
*/

void processHandbetrieb(void) 
{ static int state = 0;
  // function is not blocking, conditional part of main loop
  i = 0;
  static int manualtarra_old;
  static int desired_angle;
  static int desired_angle_old;
  static int weight_status;
  static int old_weight_status;
  static int servoactive = 0;
  static int oldservoactive;
  char text[64];

  if(modus != MODE_HANDBETRIEB)state = 0; // new arrival
  
  switch(state)
  { case 0:
      // Select the right menu background stuff
      SelectMenu(HANI_HAND);
      modus = MODE_HANDBETRIEB;
      desired_angle = SysParams[SERVOMIN];
      desired_angle_old = -1;       
      SERVO_WRITE(desired_angle);
      offset_winkel = 0;            // Offset vom Winkel wird auf 0 gestellt
      rotary_select = SW_WINKEL; 
      manualtarra_old = -1;
      OldWeight = -1234;
      current_mA_alt = -1;
      servoactive = 0;
      oldservoactive = -1;
      old_weight_status = -1;
      state++;
      return;
  }

  
  if(IsPulsed(&bStopButtonPulsed))
  { servoactive = 0;
    manualtarra_old = -1; // force reprint
  }
  if(IsPulsed(&bStartButtonPulsed))servoactive = 1;

  if (servoactive != oldservoactive) 
  { oldservoactive = servoactive;
    if (servoactive == 1)
    { TFT_line_print(0, "MANUAL MODE DOSING");
      TFT_line_blink(0, true);
      TFT_line_color(4, TFT_WHITE, TFT_DARKGREY); 
      TFT_line_blink(4, false);
      TFT_line_color(5, TFT_GREEN, TFT_DARKGREY);
    }
    else 
    { TFT_line_print(0, "MANUAL MODE PAUSED");
      TFT_line_blink(0, false);
      TFT_line_color(4, TFT_WHITE, TFT_BLACK); 
      TFT_line_blink(4, true);
      tara_alt = -1; // force reprint without current shown
      TFT_line_color(5, TFT_WHITE, TFT_DARKGREY);
    }
  }

    // set tarra for empty jar with encoder button
  if(IsPulsed(&bEncoderButtonPulsed))
  { if(servoactive == 0) // only accept new tarra in paused mode
    SysParams[MANUALTARRA] = GramsOnScale; 
    SaveParameters();
  }

  // get encoder value
  pos = getRotariesValue(SW_WINKEL); // value 0-100  
  desired_angle = (SysParams[SERVOFASTDOS] * pos) / 100;
  desired_angle = constrain(desired_angle, SysParams[SERVOMIN] + offset_winkel, SysParams[SERVOFASTDOS]);

  // close valve in paused mode
  if(servoactive == 0)SERVO_WRITE(SysParams[SERVOMIN] + offset_winkel);
  else SERVO_WRITE(desired_angle); // update valve

// what is the current weight telling us?
  if(bScaleStable)
  { if(GramsOnScale<10)weight_status = WEIGHT_EMPTY_SCALE;
    else if(abs(GramsOnScale - SysParams[MANUALTARRA]) <= SysParams[AUTO_JAR_TOLERANCE])weight_status = WEIGHT_EMPTY_JAR;
    else weight_status = WEIGHT_PARTIAL_JAR;
    //sprintf(text, "weight_status =  %d net-%d", weight_status, net_weight);
    //TFT_line_print(4, text);
  
  }
  

  // print the big weight in grams, NewWeight is used by new graphics handling
  if(weight_status==WEIGHT_EMPTY_SCALE)
  { NewWeight = GramsOnScale;
    TFT_line_print(3, GetTrans(LNG_GRAM));
  }
  else
  { NewWeight = GramsOnScale - SysParams[MANUALTARRA];
    TFT_line_print(3, GetTrans(LNG_NETWEIGHT));
  }

  // tarra value & current value for servo, printed on one line
  if((SysParams[MANUALTARRA] != manualtarra_old) || (current_mA != current_mA_alt))
  { manualtarra_old = SysParams[MANUALTARRA];
    sprintf(text, "%s %s %dg", GetTrans(LNG_TARRA), GetTrans(LNG_JAR), SysParams[MANUALTARRA]);
    if(servoactive==1)
    { if(bINA219_installed && (current_mA != current_mA_alt))
      { if(show_current == 1)
        { sprintf(&text[strlen(text)], " Servo %dmA", current_mA);
        }
      }  
    }
    current_mA_alt = current_mA;
    TFT_line_print(4, text);
    if(servoactive==0)TFT_line_blink(4, true);
  }

  // print allowable range and angle chosen for servo
  if(desired_angle != desired_angle_old)
  { desired_angle_old = desired_angle;
    sprintf(text, "Range %d°-%d° Servo %d°", (SysParams[SERVOMIN] + offset_winkel), SysParams[SERVOFASTDOS], desired_angle);
    TFT_line_print(5, text);
  }


  if (alarm_overcurrent) {i = 1;}
  while (i > 0) {
    inawatchdog = 0;                    //schalte die kontiunirliche INA Messung aus
    //Servo ist zu
    if (servo.read() <= SysParams[SERVOMIN]  + offset_winkel and offset_winkel < 3) {
      while(offset_winkel < 3 and SysParams[SERVOMAXCURRENT] < current_mA) {
        offset_winkel = offset_winkel + 1;
        SERVO_WRITE(SysParams[SERVOMIN] + offset_winkel);
        current_mA = GetCurrent(10);
        delay(1000);
      }
    }
    i = 0;
    inawatchdog = 1;
    alarm_overcurrent = 0;
  }
}


volatile int interruptCounter;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() // is called every 10mS
{ static int OldBeepTimes;
  static int BeepTicks;
  static bool bBeepNow = false;
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
    
    // global BeepTimes is set to 1 or 3 beeps
    if(BeepTimes != OldBeepTimes)
    { OldBeepTimes = BeepTimes;
      bBeepNow = true;
      BeepTicks = BeepLength; 
    }

    // keep track of the beeps to make
    if(BeepTimes)
    { if(BeepTicks > 0)
      { BeepTicks -= 100;
      }
      else
      { bBeepNow = !bBeepNow;
        BeepTicks = BeepLength; 
        BeepTimes--;
      }  
      if(bBeepNow)
      { if(SysParams[BUZZER] == 1)digitalWrite(buzzer_pin, HIGH);
        if(SysParams[LED] == 1)digitalWrite(led_pin,HIGH);
      }  
      else
      { digitalWrite(buzzer_pin, LOW);
        digitalWrite(led_pin,LOW);
      }  
    }
  }

  BlinkTimer10mS++;
  if(BlinkTimer10mS>25)
  { BlinkTimer10mS=0;
    bBlinkDisplay = !bBlinkDisplay;
  }

  if((interruptCounter % 100)==0) // each second
  { 
  }

  if(Unstable10mS)Unstable10mS--;
    
  portEXIT_CRITICAL_ISR(&timerMux);
}

//interrupt routine:
void dataReadyISR() {
  if (LoadCell.update()) {
    newDataReady = 1;
  }
}

void setup() 
{ int wait;
  // timer for all sorts of things including lcd display updates
  timer = timerBegin(0, 80, true); // prescaler 1MHz
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true); // elke 10mS
  timerAlarmEnable(timer);
  
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Hanimandl Start");
  
  SetDefaultParameters(); // fills global array with safe parameters 
  LoadParameters(); // pull from eeprom what we have

  tft_colors();
  
  //gfx->begin();
  //gfx->fillScreen(COLOR_BACKGROUND);
  //gfx->setUTF8Print(true);
    
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
    TFT_line_print(1, GetTrans(LNG_INA219_INIT));
    TFT_line_color(1, TFT_BLACK, TFT_GREEN);
  }
  else {
    bINA219_installed = false;
    SysParams[SERVOMAXCURRENT] = 0;                              // ignore INA wenn keiner gefunden wird
    TFT_line_print(1, GetTrans(LNG_INA219_MISSING));
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
  

  // servo setup initialisation
  servo.attach(SERVO_PIN,  500, 2500); // pulse range for 180 degrees range
//    servo.setPeriodHertz(50);
  SERVO_WRITE(SysParams[SERVOMIN]); // set valve to closed position

  buzzer(BUZZER_SHORT); 

  // new HX711 library used
  LoadCell.begin();
  LoadCell.start(3000, false);
  LoadCell.setCalFactor(CalibrationFactor); 
  LoadCell.setTareOffset((long)(rawtareoffset));	
  LoadCell.setSamplesInUse(2); // default 16 samples is too slow
  
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
    if (CalibrationFactor == 0) {                               // Vorhanden aber nicht kalibriert
      TFT_line_print(2, GetTrans(LNG_SCALE_NOTCALIBRATED), true);
      TFT_line_color(2, TFT_BLACK, TFT_RED);
      buzzer(BUZZER_ERROR);
    }
    else {                                          
      TFT_line_print(2, GetTrans(LNG_SCALE_INIT));
      TFT_line_color(2, TFT_BLACK, TFT_GREEN);
    }
  }
  else {                                            // Keine Waage angeschlossen
    TFT_line_color(2, TFT_BLACK, TFT_RED);
    TFT_line_print(2, GetTrans(LNG_SCALE_MISSING), true); 
    buzzer(BUZZER_ERROR);
  }
  delay(1000);

  // initiale Kalibrierung des Leergewichts wegen Temperaturschwankungen
  // Falls mehr als 20g Abweichung steht vermutlich etwas auf der Waage.
  if (waage_vorhanden == 1) {
    gewicht = GramsOnScale;
    if ((gewicht > -20) && (gewicht < 20)) 
    { LoadCell.tare();
      TFT_line_print(2, GetTrans(LNG_SCALE_TARRED)); 
      buzzer(BUZZER_SUCCESS);
    }
    else if (CalibrationFactor != 0) {
      TFT_line_color(3, TFT_BLACK, TFT_RED);
      TFT_line_print(3, GetTrans(LNG_PLEASE_EMPTY_SCALE), true); 
      delay(5000);
      // Neuer Versuch, falls Gewicht entfernt wurde
      gewicht = GramsOnScale;
      if ((gewicht > -20) && (gewicht < 20)) {
        LoadCell.tare();
        buzzer(BUZZER_SUCCESS);
      }
      else {    // Warnton ausgeben
        buzzer(BUZZER_LONG);
      }
    }
  }
  TFT_line_print(3, GetTrans(LNG_ANY_BUTTON_TO_SKIP), true); 
  
  IsPulsed(&bStartButtonPulsed); // reset the button flag
  IsPulsed(&bStopButtonPulsed); // reset the button flag
  IsPulsed(&bEncoderButtonPulsed); // reset the button flag
  IsPulsed(&bSetupSwitchPulsed); // reset the button flag
  IsPulsed(&bAutoSwitchPulsed); // reset the button flag
  IsPulsed(&bManualSwitchPulsed); // reset the button flag
   
  wait = 2000;
  while(wait)
  { wait -= 100;
    delay(100);
    if(IsPulsed(&bStartButtonPulsed))break; 
    if(IsPulsed(&bStopButtonPulsed))break; 
    if(IsPulsed(&bEncoderButtonPulsed))break; 
    if(IsPulsed(&bSetupSwitchPulsed))break; 
    if(IsPulsed(&bAutoSwitchPulsed))break; 
    if(IsPulsed(&bManualSwitchPulsed))break;
  }
  TFT_line_print(1, ""); // remove messages, warnings 
  TFT_line_print(2, ""); // remove messages, warnings 
  TFT_line_print(3, ""); // remove messages, warnings 
  
  if(wait)wait = 0;
  else wait = 60000;
  while(wait)
  { wait -= 100;
    delay(100);
    if(IsPulsed(&bStartButtonPulsed))break; 
    if(IsPulsed(&bStopButtonPulsed))break; 
    if(IsPulsed(&bEncoderButtonPulsed))break; 
    if(IsPulsed(&bSetupSwitchPulsed))break; 
    if(IsPulsed(&bAutoSwitchPulsed))break; 
    if(IsPulsed(&bManualSwitchPulsed))break;
  } 

  // die drei Datenstrukturen des Rotaries initialisieren
  initRotaries(SW_WINKEL,    0,   0, 100, 5);     // Winkel
  initRotaries(SW_KORREKTUR, -20, -100, 0, 1);     // Korrektur
  initRotaries(SW_MENU,      0,   0,   7, 1);     // Menuauswahlen
  // Parameter aus den Preferences für den Rotary Encoder setzen
  setRotariesValue(SW_WINKEL,    pos);   
  setRotariesValue(SW_KORREKTUR, SysParams[CORRECTION]);
  setRotariesValue(SW_MENU,      SysParams[CHOSENPRODUCT]);
}

void loop() 
{  //INA219 Messung
  if (bINA219_installed and inawatchdog == 1 and (SysParams[SERVOMAXCURRENT] > 0 or show_current == 1) and (modus == MODE_HANDBETRIEB or modus == MODE_AUTOMATIK)) {
    ina219_measurement();
  }

  if(stop_button_very_long_pressed > 500)
  { // pressed for > 5 seconds
    stop_button_very_long_pressed = 0;
    { LoadCell.tare();
      TFT_line_color(2, TFT_BLACK, TFT_GREEN); 
      TFT_line_print(2, GetTrans(LNG_SCALE_TARRED), true); 
      delay(2000);
      TFT_line_print(2, ""); 
      OldWeight = -1; // force reprint of scale value
    }  
  }   

  // Setup Menu 
  if(deb_setup_switch) 
  { MenuHandler(); // new menu style
  }
  // Handbetrieb 
  else if(deb_manual_switch) 
  { processHandbetrieb();
  }
    // Automatik-Betrieb 
  else if(deb_auto_switch) 
  { processAutomatik2();
  }

}

void buzzer(int sound)
{ switch(sound)
  { case BUZZER_SHORT:
      BeepTimes = 1;
      BeepLength = 100;   
      break;
    case BUZZER_SUCCESS:
      BeepTimes = 5;   
      BeepLength = 100;   
      break;
    case BUZZER_LONG:
      BeepTimes = 1;   
      BeepLength = 500;  
      break;
    case BUZZER_ERROR:
      BeepTimes = 1;   
      BeepLength = 1500;  
      break;
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
  if (current_mA > SysParams[SERVOMAXCURRENT] and SysParams[SERVOMAXCURRENT] > 0) {
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

