#include <Arduino.h>

#include <ESP32Servo.h>             /* aus dem Bibliotheksverwalter */
#include <Arduino_GFX_Library.h>    /* aus dem Bibliotheksverwalter */
#include <TFT_eSPI.h>               // more versatile display library

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

#include "hani.h"
#include <HX711_ADC.h>

extern void setPreferences(void);
extern int GetCurrent(int count);
extern void buzzer(byte type);
extern void initRotaries( int rotary_mode, int rotary_value, int rotary_min, int rotary_max, int rotary_step );
extern int getRotariesValue( int rotary_mode );
extern void setRotariesValue( int rotary_mode, int rotary_value );

extern int alarm_overcurrent;          // Alarmflag wen der Servo zuwiel Strom zieht
extern int current_mA;                     // Strom welcher der Servo zieht
extern int current_servo;              // 0 = INA219 wird ignoriert, 1-1000 = erlaubter Maximalstrom vom Servo in mA
extern int offset_winkel;              // Offset in Grad vom Schlieswinkel wen der Servo Überstrom hatte (max +3Grad vom eingestelten Winkel min)
extern int winkel_min;                 // konfigurierbar im Setup
extern Servo servo;
extern int inawatchdog;                // 0 = aus, 1 = ein / wird benötigt um INA messung auszusetzen
extern Arduino_GFX *gfx;
//Color Scheme für den TFT Display
extern unsigned long  COLOR_BACKGROUND;
extern unsigned long  COLOR_TEXT;
extern unsigned long  COLOR_MENU_POS1;
extern unsigned long  COLOR_MENU_POS2;
extern unsigned long  COLOR_MENU_POS3;
extern unsigned long  COLOR_MENU_POS4;
extern unsigned long  COLOR_MARKER;
//Variablen für TFT update
extern bool no_ina;
extern int gewicht_alt;
extern int winkel_min_alt;
extern int pos_alt;
extern int winkel_ist_alt;
extern int tara_alt;
extern int current_mA_alt;
extern int servo_aktiv_alt;
extern int auto_aktiv_alt;
extern int glas_alt;
extern int korr_alt;
extern int rotary_select_alt;
extern int autokorr_gr_alt;
// Allgemeine Variablen
extern int i;                              // allgemeine Zählvariable
extern int pos;                            // aktuelle Position des Poti bzw. Rotary 
extern int gewicht;                        // aktuelles Gewicht
extern int tara;                           // Tara für das ausgewählte Glas, für Automatikmodus
extern int tara_glas;                      // Tara für das aktuelle Glas, falls Glasgewicht abweicht
extern long gewicht_leer;                  // Gewicht der leeren Waage
extern float faktor;                       // Skalierungsfaktor für Werte der Waage
extern int fmenge;                         // ausgewählte Füllmenge
extern int fmenge_index;                   // Index in gläser[]
extern int korrektur;                      // Korrekturwert für Abfüllmenge
extern int autostart;                      // Vollautomatik ein/aus
extern int autokorrektur;                  // Autokorrektur ein/aus
extern int kulanz_gr;                      // gewollte Überfüllung im Autokorrekturmodus in Gramm

extern int progressbar;                // Variable für den Progressbar
extern char ausgabe[];                   // Fontsize 12 = 13 Zeichen maximal in einer Zeile
extern int modus;                     // Bei Modus-Wechsel den Servo auf Minimum fahren
extern int auto_aktiv;                 // Für Automatikmodus - System ein/aus?
extern int winkel;                         // aktueller Servo-Winkel
extern int winkel_min;                 // konfigurierbar im Setup
extern int winkel_max;                // konfigurierbar im Setup
extern int winkel_fein;               // konfigurierbar im Setup
extern float fein_dosier_gewicht;     // float wegen Berechnung des Schliesswinkels
extern int servo_aktiv;                // Servo aktivieren ja/nein
extern const char *GlasTypArray[];
extern int StringLenght(String a);
extern int rotary_select;
extern int show_current;               // 0 = aus, 1 = ein / Zeige den Strom an auch wenn der INA ignoriert wird
extern bool bINA219_installed;   
extern bool gezaehlt;               // Kud Zähl-Flag
extern HX711_ADC LoadCell;
extern int glastoleranz;              // Gewicht für autostart darf um +-20g schwanken, insgesamt also 40g!

extern JarName JarNames[];
extern JarParameter Jars[];




extern void SetupMyDisplay(void);
extern void TFT_line_print(int line, const char *content);
extern void TFT_line_color(int line, int textcolor, int backgroundcolor);
extern void TFT_line_blink(int line, bool blink); 
extern void SelectMenu(int menu); 

extern bool deb_start_button;
extern bool deb_stop_button;
extern bool deb_encoder_button;
extern bool deb_setup_switch;
extern bool deb_auto_switch;
extern bool deb_manual_switch;

extern TFT_eSPI tft;

#define Y_SERVODATA 34


void processAutomatik2(void) {
  static int state = 0;
  int zielgewicht;                 // Glas + Korrektur
  static int autokorrektur_gr = 0; 
  int erzwinge_servo_aktiv = 0;
  bool voll = false;
  static int gewicht_vorher;       // Gewicht des vorher gefüllten Glases
  static int sammler_num = 5;      // Anzahl identischer Messungen für Nachtropfen
  int n;
  int y_offset = 0;
  
  if(modus != MODE_AUTOMATIK)state = 0;

  // get settings for this automatic mode
  korrektur    = getRotariesValue(SW_KORREKTUR);
  fmenge_index = getRotariesValue(SW_MENU);
  fmenge       = Jars[fmenge_index].Gewicht;
  tara         = Jars[fmenge_index].Tara;

  switch(state)
  { case 0:
      // Select the right menu background stuff
      SelectMenu(HANI_AUTO);
      modus = MODE_AUTOMATIK;
      state++;
      return;
      break;
    
    case 1:
      // reset variables   
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
      state++;
      return;
      break;
    
    case 2: 
      // print fixed graphic elements on display  
      tft.drawLine(0, Y_SERVODATA, 320, Y_SERVODATA, TFT_WHITE);
      tft.drawLine(0, Y_SERVODATA+20, 320, Y_SERVODATA+20, TFT_WHITE);

      tft.drawLine(0, 184, 320, 184, TFT_WHITE);
      tft.drawLine(160, 184, 160, 240, TFT_WHITE);
      gfx->setTextColor(COLOR_TEXT);
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
      gfx->setCursor(2, Y_SERVODATA+17);
      //sprintf(ausgabe,"min:000°     max:000°      ist:000°", (winkel_min, winkel_max*pos/100));
      sprintf(ausgabe,"Min:         Max:         Ist:");
      gfx->print(ausgabe);
      state++;
      return;
      break;
    
    case 3:
      // automatic paused mode
      TFT_line_print(0, "AUT0MATIC PAUSED");
      state++;
      return;
      break;

    case 4: 
      // need a tarra value for empty jar
      if(tara < 30)
      { TFT_line_print(2, "No Tarra Value For Jar!");
        TFT_line_blink(2, true);
        TFT_line_color(2, TFT_BLACK, TFT_RED);
        TFT_line_print(3, "Please Check Setup!");
        state++;
        return;
      }
      state=6;
      break;  

    case 5: 
      if(tara>=30)state++;
      break;  

    case 6: 
      state++;
      break;  
      

    default:
      break;
  }

  // refresh the unconditional data on screen
  if (winkel_min  + offset_winkel != winkel_min_alt) 
  { gfx->setTextColor(COLOR_TEXT);
    gfx->fillRect(37, Y_SERVODATA+3, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(38, Y_SERVODATA+17);
    sprintf(ausgabe, "%3i°", winkel_min + offset_winkel);
    gfx->print(ausgabe);
    winkel_min_alt = winkel_min + offset_winkel;
  }
  if (pos != pos_alt) 
  { gfx->fillRect(154, Y_SERVODATA+3, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(155, Y_SERVODATA+17);
    sprintf(ausgabe, "%3i°", winkel_max*pos/100);
    gfx->print(ausgabe);
    pos_alt = pos;
  }
  if (winkel != winkel_ist_alt)
  { gfx->fillRect(271, Y_SERVODATA+3, 40, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(272, Y_SERVODATA+17);
    sprintf(ausgabe, "%3i°", winkel);
    gfx->print(ausgabe);
    winkel_ist_alt = winkel;
  }

  // if state hangs in no tarra for jar
  if(state<6)return;


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
    gfx->fillRect(80, 57, 240, 80, COLOR_BACKGROUND);
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
      gfx->fillRect(80, 57, 240, 80, COLOR_BACKGROUND);
      gfx->setFont(Punk_Mono_Bold_320_200);
      gfx->setCursor(120, 91);
      gfx->print("Bitte Glas");
      gfx->setCursor(120, 123);
      gfx->print("aufstellen");
      glas_alt = -1;
    } 
    else if (gewicht != gewicht_alt) {
      gfx->fillRect(80, 57, 240, 80, COLOR_BACKGROUND);
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
