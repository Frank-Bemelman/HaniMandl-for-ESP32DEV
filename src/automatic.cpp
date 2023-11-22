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
extern int SysParams[SERVOMIN];                 // konfigurierbar im Setup
extern Servo servo;
extern int inawatchdog;                // 0 = aus, 1 = ein / wird benötigt um INA messung auszusetzen
extern Arduino_GFX *gfx;
//Color Scheme für den TFT Display
extern unsigned long  COLOR_BACKGROUND;
extern unsigned long  COLOR_TEXT;
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
extern int gewicht;                        // aktuelles Gewicht
extern int actual_tarra;                      // Tara für das aktuelle Glas, falls Glasgewicht abweicht
extern float faktor;                       // Skalierungsfaktor für Werte der Waage

extern int progressbar;                // Variable für den Progressbar
extern char ausgabe[];                   // Fontsize 12 = 13 Zeichen maximal in einer Zeile
extern int modus;                     // Bei Modus-Wechsel den Servo auf Minimum fahren
extern int auto_aktiv;                 // Für Automatikmodus - System ein/aus?
extern float fein_dosier_gewicht;     // float wegen Berechnung des Schliesswinkels
extern int servo_aktiv;                // Servo aktivieren ja/nein
extern const char *GlasTypArray[];
extern int StringLenght(String a);
extern int rotary_select;
extern int show_current;               // 0 = aus, 1 = ein / Zeige den Strom an auch wenn der INA ignoriert wird
extern bool bINA219_installed;   
extern bool gezaehlt;               // Kud Zähl-Flag
extern HX711_ADC LoadCell;

extern JarType JarTypes[];
extern ProductParameter Products[];
extern int SysParams[];
extern int NewWeight;
extern int OldWeight;



extern void SetupMyDisplay(void);
extern void TFT_line_print(int line, const char *content);
extern void TFT_line_color(int line, int textcolor, int backgroundcolor);
extern void TFT_line_blink(int line, bool blink); 
extern void SelectMenu(int menu); 
extern bool IsPulsed(bool *button);

extern bool deb_start_button;
extern bool deb_stop_button;
extern bool deb_encoder_button;
extern bool deb_setup_switch;
extern bool deb_auto_switch;
extern bool deb_manual_switch;

extern bool bStartButtonPulsed;
extern bool bStopButtonPulsed;
extern bool bEncoderButtonPulsed;
extern bool bSetupSwitchPulsed;
extern bool bAutoSwitchPulsed;
extern bool bManualSwitchPulsed;

extern int GramsOnScale;
extern bool bScaleStable;

extern TFT_eSPI tft;

int Unstable10mS = 0;

#define Y_SERVODATA 34


void processAutomatik2(void) 
{ static int state = 0;
  static int runmode = 0; // 0-stopped 1-paused 2-dosing
  static int oldmode = -1;
  static int dosing_state = 0; // 0-stopped 1-filling 2-filled
  static int old_dosing_state = -1;
  static int scale_status = -1; // 0-empty, 1-jar is placed, 2-jar is filling, 3-jar is filled 
  static int old_scale_status = -1; // 0-empty, 1-jar is placed, 2-jar is filling, 3-jar is filled 
  static int weight_status = -1; // 0-empty, 1-empty jar is placed, 2-jar is filling, 3-jar is fully filled 
  static int old_weight_status = -1; // 0-empty, 1-jar is placed, 2-jar is filling, 3-jar is filled 
  
  static int autokorrektur_gr = 0; 
  int erzwinge_servo_aktiv = 0;
  bool voll = false;
  static int gewicht_vorher;       // Gewicht des vorher gefüllten Glases
  static int sammler_num = 5;      // Anzahl identischer Messungen für Nachtropfen
  int n;
  int y_offset = 0;
  static int expected_tarra;
  static int actual_tarra = 0; // weight of the placed jar
  static int actual_angle;
  static int full_dosage_angle = -1;   // angle used for dosing max 
  static int old_full_dosage_angle = -1; 
  static int fine_dosage_angle = -1;    
  static int old_fine_dosage_angle = -1; 
  static int full_dosage_percent = 100;    // continiously changeable by rotary
  static int old_full_dosage_percent = -1; // continiously changeable by rotary
  static int fine_dosage_percent = 100;    // continiously changeable by rotary
  static int old_fine_dosage_percent = -1; // continiously changeable by rotary
  static int percent_angle = -1; // from encoder
  static int old_angle = -1; // from encoder
  static int product_chosen = -1;
  static int old_product_chosen = -1;
  static int net_weight = 0;
  static int target_weight = 0; // net weight + actual tarra for glas + correction - correction is aimed to achieve the coulance for this product
  float  grams_to_go; 
  int i=0;

  static int offset_winkel = 0;
  char text[64];
  
  if(modus != MODE_AUTOMATIK)state = 0;

  // on each entry of this non-blocking function
  // get settings 
  if(state>0)
  { product_chosen = getRotariesValue(SW_MENU);
    if(product_chosen != old_product_chosen)
    { net_weight     = Products[product_chosen].Gewicht;
      expected_tarra = JarTypes[Products[product_chosen].GlasTyp].tarra;
      actual_tarra = expected_tarra; // when empty jar is placed, this will be updated to actual weight of jar with its apperent tolerance
      old_product_chosen = product_chosen;
    }  
    
    SysParams[CORRECTION] = getRotariesValue(SW_KORREKTUR);
    percent_angle = getRotariesValue(SW_WINKEL); // value 0-100 

    if(dosing_state == DOSING_FINE) 
    { fine_dosage_percent = percent_angle;
      fine_dosage_angle = (((SysParams[SERVOFINEDOS]-SysParams[SERVOMIN]) * percent_angle) / 100)+SysParams[SERVOMIN]; // get angle of MIN-FINE range
    }
    else
    { full_dosage_percent = percent_angle;
      full_dosage_angle = (((SysParams[SERVOMAX]-SysParams[SERVOFINEDOS]) * percent_angle) / 100)+SysParams[SERVOFINEDOS]; // get angle of FINE-MAX range
    }
  }
  
  switch(state)
  { case 0:
      // Select the right menu background stuff
      SelectMenu(HANI_AUTO);
      modus = MODE_AUTOMATIK;

      actual_angle = SysParams[SERVOMIN];          // Hahn schliessen

      //sprintf(text,"product_chosen=%d",product_chosen);
      //TFT_line_print(2, text);

      if(product_chosen < 0)initRotaries(SW_MENU, 1, 0, 5, 1);
      else initRotaries(SW_MENU, product_chosen, 0, 5, 1);

      rotary_select = SW_WINKEL;    // Einstellung für Winkel über Rotary
      
      if(scale_status < 0)scale_status = 0;

      oldmode = -1; 
      old_scale_status = -1;
      OldWeight = -1;
      state++;
      return;
      break;
    
    case 1:
      // reset variables   
      servo_aktiv = 0;              // Servo-Betrieb aus
      auto_aktiv = 0;               // automatische Füllung starten
      gewicht_vorher = Products[product_chosen].Gewicht + SysParams[CORRECTION];
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
      gfx->setFont(Punk_Mono_Thin_160_100);
      gfx->setCursor(170, 219);
      gfx->print("Korrektur:");
      gfx->setCursor(170, 236);
      gfx->print("Autokorr.:");
      gfx->setCursor(2, Y_SERVODATA+17);
      sprintf(ausgabe,"Range          Dos          Act");
      gfx->print(ausgabe);
      state++;
      return;
      break;
    
    case 3:
      // begin in stopped mode
      if(SysParams[AUTOSTART])TFT_line_print(0, "SEMI-AUT0 STOPPED");
      else TFT_line_print(0, "FULL-AUT0 STOPPED");
      state++;
      return;
      break;

    case 4: 
      // need a tarra value for empty jar
      if(expected_tarra < 25)
      { sprintf(ausgabe, "Bad Tarra Value %dg!", expected_tarra);
        TFT_line_print(2, ausgabe);
        TFT_line_blink(2, true);
        TFT_line_color(2, TFT_BLACK, TFT_RED);
        sprintf(ausgabe, "Please Set Valid Tarra For %s", JarTypes[Products[product_chosen].GlasTyp].name);
        TFT_line_print(3, ausgabe);
        state++;
        return;
      }
      state=6;
      break;  

    case 5: 
      if(expected_tarra>=25)state++;
      break;  

    case 6: // good to go or to continue
      IsPulsed(&bStartButtonPulsed); // reset the button flag
      IsPulsed(&bStopButtonPulsed); // reset the button flag
      IsPulsed(&bEncoderButtonPulsed); // reset the button flag
      IsPulsed(&bSetupSwitchPulsed); // reset the button flag
      IsPulsed(&bAutoSwitchPulsed); // reset the button flag
      IsPulsed(&bManualSwitchPulsed); // reset the button flag
      state++;
      break;  

    default:
      break;
  }

  // keep the servo up to date
  SERVO_WRITE(actual_angle);

  // refresh the unconditional data on screen
  if((offset_winkel != winkel_min_alt) || (full_dosage_angle != old_full_dosage_angle) || (fine_dosage_angle != old_fine_dosage_angle) || (actual_angle != winkel_ist_alt))
  { gfx->setTextColor(COLOR_TEXT);
    gfx->fillRect(2, Y_SERVODATA+3, 318, 16, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_160_100);
    gfx->setCursor(2, Y_SERVODATA+17);
    sprintf(ausgabe, "Range %d°-%d° - M %d° - F %d° - A %d°", SysParams[SERVOMIN] + offset_winkel, SysParams[SERVOMAX], full_dosage_angle, fine_dosage_angle, actual_angle );
    gfx->print(ausgabe);
    winkel_min_alt = offset_winkel;
    old_full_dosage_angle = full_dosage_angle;
    old_fine_dosage_angle = fine_dosage_angle;
    winkel_ist_alt = actual_angle;

  }
  
  // if state hangs in no tarra for jar
  if(state<6)return;

  if(scale_status <= SCALE_JAR_PLACED)
  { NewWeight = GramsOnScale; 
  }
  else
  { NewWeight = GramsOnScale - actual_tarra; // while dosing, subtract weight of empty jar
  }


  if(IsPulsed(&bStartButtonPulsed))
  { if(!Unstable10mS)runmode = 2; 
  }

  if (IsPulsed(&bStopButtonPulsed)) {
    actual_angle = SysParams[SERVOMIN] + offset_winkel;
    if(runmode)runmode--;    
  }

  if(runmode!= oldmode)
  { oldmode = runmode;
    if(runmode==0)
    { if(SysParams[AUTOSTART])TFT_line_print(0, "FULL-AUT0 STOPPED");
      else TFT_line_print(0, "SEMI-AUT0 STOPPED");
    }  
    else if(runmode==1)
    { if(SysParams[AUTOSTART])TFT_line_print(0, "FULL-AUT0 PAUSED");
      else TFT_line_print(0, "SEMI-AUT0 PAUSED");
    }  
    else 
    { if(SysParams[AUTOSTART])TFT_line_print(0, "FULL-AUT0 FILLING");
      else TFT_line_print(0, "SEMI-AUT0 FILLING");
    }  
  }

  // set new text according to status
  if(scale_status != old_scale_status)
  { old_scale_status = scale_status;
    switch(scale_status)
    { case SCALE_EMPTY:
        TFT_line_print(5, "Place Empty Jar");
        break;
      case SCALE_JAR_PLACED:
        TFT_line_print(5, "Jar Is Placed");
        break;
      case SCALE_WAIT_START:
        TFT_line_print(5, "Press Start To Fill");
        break;
      case SCALE_WAIT_RESUME:
        TFT_line_print(5, "Press Start To Resume");
        break;
      case SCALE_WILL_START:
        TFT_line_print(5, "Will Start Filling");
        break;
      case SCALE_WILL_RESUME:
        TFT_line_print(5, "Will Resume Filling");
        break;
      case SCALE_JAR_FILL_FULL_SPEED:
        TFT_line_print(5, "Full Speed Fill");
        break;
      case SCALE_JAR_FILL_SLOW_SPEED:
        TFT_line_print(5, "Fine Speed Fill");
        break;
      case SCALE_JAR_FILLING_PAUSED:
        TFT_line_print(5, "Paused Fill");
        break;
      case SCALE_JAR_FILLED:
        TFT_line_print(5, "Product Ready!");
        break;
    }
  }
  
  // what is the current weight telling us?
  if(bScaleStable)
  { if(GramsOnScale<10)weight_status = WEIGHT_EMPTY_SCALE;
    else if(abs(GramsOnScale - actual_tarra) <= SysParams[AUTO_JAR_TOLERANCE])weight_status = WEIGHT_EMPTY_JAR;
    else if((GramsOnScale) > actual_tarra + net_weight)weight_status = WEIGHT_FULL_JAR; // full jar actually weighs a bit more because of coulance setting
    else weight_status = WEIGHT_FILLING_JAR;
    //sprintf(text, "weight_status =  %d net-%d", weight_status, net_weight);
    //TFT_line_print(4, text);
  
  }
  //else TFT_line_print(4, "Wobbly");
  
  if(weight_status == WEIGHT_EMPTY_SCALE) // works also as emergency stop
  { scale_status = SCALE_EMPTY;
    actual_angle = SysParams[SERVOMIN] + offset_winkel;
    if (SysParams[AUTOSTART] != 1)runmode = 0;
    dosing_state = DOSING_STOPPED;
  }
  
  if(dosing_state==DOSING_STOPPED && scale_status == SCALE_EMPTY) // not started filling yet
  { if(weight_status == WEIGHT_EMPTY_JAR)
    { scale_status = SCALE_JAR_PLACED;
      Unstable10mS = 100; // let's wait a second before acting
      if (SysParams[AUTOSTART]!= 1)runmode = 0; // in semi mode, always need to start manually
    } 
    else if(weight_status == WEIGHT_FULL_JAR)
    { scale_status = SCALE_JAR_FILLED;
    }
  } 

  if((scale_status == SCALE_JAR_PLACED) && !Unstable10mS && bScaleStable)
  { if(runmode<2) // stopped or paused
    { scale_status = SCALE_WAIT_START; // press start to fill message 
      Unstable10mS = 100; // let's wait a second before acting
    }
    else if(runmode==2)
    { scale_status = SCALE_WILL_START; // will start filling message 
      Unstable10mS = 100; // let's wait a second before acting
    }
    dosing_state = DOSING_WAIT_START;
    actual_tarra = GramsOnScale; // this is the weight of the actual jar placed
  }

  if(((scale_status == SCALE_WILL_START) || (scale_status == SCALE_WAIT_START)) && !Unstable10mS && (runmode==2))
  { scale_status = SCALE_JAR_FILL_FULL_SPEED;
    dosing_state = DOSING_FULL;

  }


  if(dosing_state != old_dosing_state) // set the encoder to make eventual adjustments to servo according full or fine dosing state
  { old_dosing_state = dosing_state;
    if(dosing_state == DOSING_FULL) 
    { initRotaries(SW_WINKEL, full_dosage_percent, 0, 100, 5); 
    }
    else if(dosing_state == DOSING_FINE) 
    { initRotaries(SW_WINKEL, fine_dosage_percent, 0, 100, 5); 
    }
    else
    { initRotaries(SW_WINKEL, full_dosage_percent, 0, 100, 5); 
    }
  }

  // this is the part where we actively adjust the servo as we fill the jar
  target_weight = net_weight + actual_tarra + SysParams[CORRECTION] + autokorrektur_gr;
  // if not stopped or paused 
  if((runmode==2) && (dosing_state == DOSING_FULL || dosing_state == DOSING_FINE))
  { if(dosing_state == DOSING_FULL) actual_angle = full_dosage_angle;
    else actual_angle = fine_dosage_angle;
    grams_to_go = (target_weight - GramsOnScale);
    if(grams_to_go < fein_dosier_gewicht) // now squeeze the valve gradually and elegant
    { dosing_state = DOSING_FINE;
      scale_status = SCALE_JAR_FILL_SLOW_SPEED;
      if(grams_to_go < 0)grams_to_go=0; // don't act silly if we have an overfilled jar
      // calculate a elegant angle for closing the valve, between minimum allowed and chosen angle for fine_dosage
      actual_angle =  (round) ( ((grams_to_go / fein_dosier_gewicht) * (fine_dosage_angle - SysParams[SERVOMIN])) + SysParams[SERVOMIN]);
    }
    if(grams_to_go <= 0) // jar is filled
    { dosing_state = DOSING_STOPPED;
      scale_status = SCALE_JAR_FILLED;
      actual_tarra = expected_tarra; 
    }
  }

    
  target_weight = net_weight + actual_tarra + SysParams[CORRECTION] + autokorrektur_gr;
  // Anpassung des Autokorrektur-Werts
  if (SysParams[AUTO_CORRECTION] == 1) {                                                       
    if ( auto_aktiv == 1 && servo_aktiv == 0 && actual_angle == SysParams[SERVOMIN] + offset_winkel && gewicht >= target_weight && sammler_num <= 5) {     
      voll = true;                       
      if (gewicht == gewicht_vorher && sammler_num < 5) { // wir wollen 5x das identische Gewicht sehen  
        sammler_num++;
      } 
      else if (gewicht != gewicht_vorher) {               // sonst gewichtsänderung nachführen
        gewicht_vorher = gewicht;
        sammler_num = 0;
      } 
      else if (sammler_num == 5) {                        // gewicht ist 5x identisch, autokorrektur bestimmen
        autokorrektur_gr = (net_weight + SysParams[COULANCE] + actual_tarra) - (gewicht - autokorrektur_gr);
        if (SysParams[CORRECTION] + autokorrektur_gr > SysParams[COULANCE]) {   // Autokorrektur darf nicht überkorrigieren, max Füllmenge plus SysParams[COULANCE]
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
        Products[product_chosen].TripCount++;
        Products[product_chosen].Count++;
        gezaehlt = true;
      }
      #ifdef isDebug
        Serial.print("Nachtropfen:");
        Serial.print(" gewicht: ");        Serial.print(gewicht);
        Serial.print(" gewicht_vorher: "); Serial.print(gewicht_vorher);
        Serial.print(" sammler_num: ");    Serial.print(sammler_num);
        Serial.print(" Korrektur: ");      Serial.println(autokorrektur_gr);
        Serial.print(" Zähler Trip: ");    Serial.print(Products[product_chosen].TripCount); //Kud
        Serial.print(" Zähler: ");         Serial.println(Products[product_chosen].Count); //Kud
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
  
//  if (servo_aktiv == 1) {
//    actual_angle = (SysParams[SERVOMIN] * pos / 100);
//  }
  
//  if (servo_aktiv == 1 && (target_weight - gewicht <= fein_dosier_gewicht)) {
//    actual_angle = ((SysParams[SERVOMIN]*pos / 100) * ((target_weight-gewicht) / fein_dosier_gewicht) );
//  }
  
  
  if (servo_aktiv == 1 && actual_angle <= SysParams[SERVOFINEDOS]) {
    actual_angle = SysParams[SERVOFINEDOS];
  }
  // Glas ist voll
  if (servo_aktiv == 1 && gewicht >= target_weight) {
    actual_angle      = SysParams[SERVOMIN] + offset_winkel;
    servo_aktiv = 0;
    if (gezaehlt == false) {
      Products[product_chosen].TripCount++;
      Products[product_chosen].Count++;
      gezaehlt = true;
    }
    if (SysParams[AUTOSTART] != 1)       // autostart ist nicht aktiv, kein weiterer Start
      auto_aktiv = 0;
    if (SysParams[AUTO_CORRECTION] == 1)   // autokorrektur, gewicht merken
      gewicht_vorher = gewicht;
    buzzer(BUZZER_SHORT);
  }
//  SERVO_WRITE(actual_angle);
  #ifdef isDebug
    #if isDebug >= 4
      Serial.print("Automatik:");  
      Serial.print(" Gewicht: ");        Serial.print(gewicht);
      Serial.print(" Dauer ");           Serial.print(millis() - scaletime);
      Serial.print(" Füllmenge: ");      Serial.print(net_weight);
      Serial.print(" actual_tarra:");       Serial.print(actual_tarra);
      Serial.print(" Autokorrektur: ");  Serial.print(autokorrektur_gr);
      Serial.print(" target_weight ");     Serial.print(target_weight);
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
    gfx->fillRect(260, 187, 70, 18, COLOR_BACKGROUND);
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
  if ((glas_alt != product_chosen and servo_aktiv == 0 and gewicht <= Products[product_chosen].Gewicht - expected_tarra) or (glas_alt != product_chosen and rotary_select_alt == SW_KORREKTUR)) {
    if (rotary_select == SW_MENU and servo_aktiv == 0) {
      gfx->setTextColor(COLOR_MARKER);
    }
    gfx->setCursor(2, 176);
    gfx->fillRect(0, 156, 320, 27, COLOR_BACKGROUND);
    gfx->setFont(Punk_Mono_Thin_240_150);
    sprintf(ausgabe, "%dg ", (Products[product_chosen].Gewicht));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_120_075);
    gfx->setCursor(2 + 14*StringLenght(ausgabe), 168);
    sprintf(ausgabe, "+%dg ", (SysParams[COULANCE]));
    gfx->print(ausgabe);
    gfx->setFont(Punk_Mono_Thin_240_150);
    gfx->setCursor(110, 176);
    gfx->print(JarTypes[Products[product_chosen].GlasTyp].name);
    gfx->setTextColor(COLOR_TEXT);
    glas_alt = product_chosen;
  }
  if (auto_aktiv != auto_aktiv_alt) {
    gfx->fillRect(12, 74, 38, 38, COLOR_BACKGROUND);
    auto_aktiv_alt = auto_aktiv;
    gfx->setTextColor(COLOR_TEXT);
  }

  // progress bar
  if (expected_tarra > 0) {
    if(dosing_state == DOSING_STOPPED)gewicht = 0;
    else gewicht = NewWeight;
    if (gewicht != gewicht_alt) {
      progressbar = 318.0*((float)gewicht/(float)(Products[product_chosen].Gewicht));
      progressbar = constrain(progressbar,0,318);
      gfx->drawRect(0, 127, 320, 15, COLOR_TEXT);
      if (Products[product_chosen].Gewicht > gewicht) {
        gfx->fillRect  (1, 128, progressbar, 13, RED);
      }
      else if (gewicht >= Products[product_chosen].Gewicht and gewicht <= Products[product_chosen].Gewicht + SysParams[COULANCE]){
        gfx->fillRect  (1, 128, progressbar, 13, GREEN);
      }
      else {
        gfx->fillRect  (1, 128, progressbar, 13, ORANGE);
      }
      gfx->fillRect  (1 + progressbar, 128, 318 - progressbar, 13, COLOR_BACKGROUND);
      gewicht_alt = gewicht;
    }
  }
  
  
  // CURRENT STUFF
//  if (alarm_overcurrent) {i = 1;}
  while (i > 0) {
    inawatchdog = 0;                    //schalte die kontiunirliche INA Messung aus
    //Servo ist zu
    if (servo.read() <= SysParams[SERVOMIN]  + offset_winkel and offset_winkel < 3) {
      while(offset_winkel < 3 and current_servo < current_mA) {
        offset_winkel = offset_winkel + 1;
//        SERVO_WRITE(SysParams[SERVOMIN] + offset_winkel);
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
