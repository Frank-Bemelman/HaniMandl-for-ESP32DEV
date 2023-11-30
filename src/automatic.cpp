#include <Arduino.h>
#include <ESP32Servo.h>             
#include <TFT_eSPI.h>               
#include "hani.h"

#define TRANS
#include "language.h"
#include "menustructure.h"
extern char* GetTrans(int idx);


extern int GetCurrent(int count);
extern void buzzer(int type);
extern void initRotaries( int rotary_mode, int rotary_value, int rotary_min, int rotary_max, int rotary_step );
extern int getRotariesValue( int rotary_mode );
extern void setRotariesValue( int rotary_mode, int rotary_value );
extern void UseFont(const uint8_t* usethisfont);
void SaveParameters(void);

extern int alarm_overcurrent;          // Alarmflag wen der Servo zuwiel Strom zieht
extern int current_mA;                     // Strom welcher der Servo zieht
extern int offset_winkel;              // Offset in Grad vom Schlieswinkel wen der Servo Überstrom hatte (max +3Grad vom eingestelten Winkel min)
extern Servo servo;
extern int inawatchdog;                // 0 = aus, 1 = ein / wird benötigt um INA messung auszusetzen
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

extern JarType JarTypes[];
extern ProductParameter Products[];
extern int SysParams[];
extern int NewWeight;
extern int OldWeight;
extern int JarIconFilled; // to mimic jar being filled
extern int OldJarIconFilled; // redraw jar if different
extern int JarIconFilledColor; // color of jar content





extern void SetupMyDisplay(void);
extern void TFT_line_print(int line, const char *content, bool blink = false);
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
extern TFTline TheAngles;

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
  
  static int fast_dosage_angle = -1;   // angle used for dosing max 
  static int old_fast_dosage_angle = -1; 
  
  static int slow_dosage_angle = -1;    
  static int old_slow_dosage_angle = -1; 
  
  static int fast_dosage_percent = 100;    // continiously changeable by rotary
  static int old_fast_dosage_percent = -1; // continiously changeable by rotary
  static int slow_dosage_percent = 100;    // continiously changeable by rotary
  static int old_slow_dosage_percent = -1; // continiously changeable by rotary
  
  static int percent_angle = -1; // from encoder
    static int old_angle = -1; // from encoder
  
  static int product_chosen = -1;
  static int old_product_chosen = -1;
  static int target_net_weight = 0;
  static int target_bruto_weight = 0; // net weight + actual tarra for glas + correction - correction is aimed to achieve the coulance for this product???????
  static float grams_to_go = 0; 
  static int net_weight_in_jar = 0;
  static float slow_down_at_net_weight = 0;

  int i=0;
  int textdatum;

  static int offset_winkel = 0;
  char text[64];
  
  if(modus != MODE_AUTOMATIK)state = 0;

  // on each entry of this non-blocking function
  // get settings 
  if(state>0)
  { product_chosen = getRotariesValue(SW_MENU);
    if(product_chosen != old_product_chosen)
    { target_net_weight     = Products[product_chosen].Gewicht;
      expected_tarra = JarTypes[Products[product_chosen].GlasTyp].tarra;
      actual_tarra = expected_tarra; // when empty jar is placed, this will be updated to actual weight of jar with its apperent tolerance
      old_product_chosen = product_chosen;
      SysParams[CHOSENPRODUCT] = product_chosen;
      slow_down_at_net_weight = (float)((SysParams[SLOWDOWNPERCENT] * target_net_weight)/100);
    }  
    
    SysParams[CORRECTION] = getRotariesValue(SW_KORREKTUR);
    percent_angle = getRotariesValue(SW_WINKEL); // value 0-100 

    if(dosing_state == DOSING_SLOW) 
    { slow_dosage_angle = percent_angle;
      slow_dosage_angle = (((SysParams[SERVOSLOWDOS]-SysParams[SERVOMIN]) * percent_angle) / 100)+SysParams[SERVOMIN]; // get angle of MIN-FINE range
    }
    else
    { fast_dosage_percent = percent_angle;
      fast_dosage_angle = (((SysParams[SERVOFASTDOS]-SysParams[SERVOSLOWDOS]) * percent_angle) / 100)+SysParams[SERVOSLOWDOS]; // get angle of FINE-MAX range
    }

    // refresh the servo data on screen
    if((offset_winkel != winkel_min_alt) || (fast_dosage_angle != old_fast_dosage_angle) || (slow_dosage_angle != old_slow_dosage_angle) || (actual_angle != winkel_ist_alt))
    { sprintf(TheAngles.content, "  Servo %d-%d\xB0 F %d\xB0 S %d\xB0 A %d\xB0  ", SysParams[SERVOMIN] + offset_winkel, SysParams[SERVOFASTDOS], fast_dosage_angle, slow_dosage_angle, actual_angle );
      TheAngles.refresh = true;
      winkel_min_alt = offset_winkel;
      old_fast_dosage_angle = fast_dosage_angle;
      old_slow_dosage_angle = slow_dosage_angle;
      winkel_ist_alt = actual_angle;
    }
  }
  
  switch(state)
  { case 0:
      // Select the right menu background stuff
      SelectMenu(HANI_AUTO);
      modus = MODE_AUTOMATIK;

      actual_angle = SysParams[SERVOMIN];          // Hahn schliessen
      fast_dosage_angle = SysParams[SERVOFASTDOS];
      slow_dosage_angle = SysParams[SERVOSLOWDOS];

      //sprintf(text,"product_chosen=%d",product_chosen);
      //TFT_line_print(2, text);
      product_chosen = SysParams[CHOSENPRODUCT];
      if(product_chosen < 0)product_chosen = SysParams[CHOSENPRODUCT];
      else initRotaries(SW_MENU, product_chosen, 0, 5, 1);
      setRotariesValue(SW_KORREKTUR, SysParams[CORRECTION]);

      //rotary_select = SW_WINKEL;    // Einstellung für Winkel über Rotary
      
      if(scale_status < 0)scale_status = 0;

      oldmode = -1; 
      old_scale_status = -1;
      OldWeight = -1;
      old_product_chosen = -1;
      TheAngles.oldbackgroundcolor = -1;
      

      state++;
      return;
      break;
    
    case 1:
      // reset variables   
      servo_aktiv = 0;              // Servo-Betrieb aus
      auto_aktiv = 0;               // automatische Füllung starten
      gewicht_vorher = Products[product_chosen].Gewicht + SysParams[CORRECTION];
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
      state++;
      return;
      break;
    
    case 2: // placeholder for perhaps something to do
      state++;
      return;
      break;
    
    case 3: 
      if(SysParams[AUTOSTART])TFT_line_print(0, GetTrans(LNG_FULL_AUTO_STOPPED));
      else
      { TFT_line_print(0, GetTrans(LNG_SEMI_AUTO_STOPPED));
      }    
      state++;
      return;
      break;

    case 4: 
      // need a tarra value for empty jar
      if(expected_tarra < 25)
      { sprintf(text, "%s %dg!", GetTrans(LNG_BAD_TARRA_VALUE), expected_tarra);
        TFT_line_print(4, text, true);
        TFT_line_color(4, TFT_BLACK, TFT_RED);
        sprintf(text, "%s %s", GetTrans(LNG_PLEASE_SET_TARRA_VALUE), JarTypes[Products[product_chosen].GlasTyp].name);
        TFT_line_print(5, text);
        state++;
        return;
      }
      state=6;
      break;  

    case 5: 
      if(expected_tarra>=25)state++;
      break;  

    case 6: // good to go or to continue
      sprintf(text, "%dg+%dg %s", target_net_weight, SysParams[COULANCE], JarTypes[Products[product_chosen].GlasTyp].name);
      TFT_line_print(4,text, rotary_select==SW_MENU); // blink when in setup for this item
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

 
  // if state hangs in no tarra for jar
  if(state<6)return;


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
    { TheAngles.backgroundcolor = TFT_BLACK;
      TheAngles.textcolor = TFT_WHITE;
      if(SysParams[AUTOSTART])TFT_line_print(0, GetTrans(LNG_FULL_AUTO_STOPPED));
      else
      { TFT_line_print(0, GetTrans(LNG_SEMI_AUTO_STOPPED));
      }
    }  
    else if(runmode==1)
    { TheAngles.backgroundcolor = TFT_ORANGE;
      if(SysParams[AUTOSTART])TFT_line_print(0, GetTrans(LNG_FULL_AUTO_PAUSED));
      else
      { TFT_line_print(0, GetTrans(LNG_SEMI_AUTO_PAUSED));
      }
    }  
    else 
    { TheAngles.backgroundcolor = TFT_GREEN;
      TheAngles.textcolor = TFT_BLACK;
    
      if(SysParams[AUTOSTART])TFT_line_print(0, GetTrans(LNG_FULL_AUTO_FILLING));
      else 
      { TFT_line_print(0, GetTrans(LNG_SEMI_AUTO_FILLING));
      }
    } 
    TheAngles.refresh = true; 
  }

  // set new text according to status
  if(scale_status != old_scale_status)
  { old_scale_status = scale_status;
    switch(scale_status)
    { case SCALE_EMPTY:
        TFT_line_color(5, TFT_BLACK, TFT_ORANGE);
        sprintf(text, GetTrans(LNG_PLACE_EMPTY_JAR), Products[product_chosen].Count+1);
        TFT_line_print(5, text, true);
        break;
      case SCALE_JAR_PLACED:
        TFT_line_color(5, TFT_BLACK, TFT_ORANGE);
        sprintf(text, GetTrans(LNG_JAR_IS_PLACED), Products[product_chosen].Count+1);
        TFT_line_print(5, text);
        break;
      case SCALE_WAIT_START:
        TFT_line_color(5, TFT_BLACK, TFT_GREEN);
        TFT_line_print(5, GetTrans(LNG_PRESS_START_TO_FILL), true);
        break;
      case SCALE_WAIT_RESUME:
        TFT_line_color(5, TFT_BLACK, TFT_GREEN);
        TFT_line_print(5, GetTrans(LNG_PRESS_START_TO_RESUME), true);
        break;
      case SCALE_WILL_START:
        TFT_line_color(5, TFT_BLACK, TFT_ORANGE);
        TFT_line_print(5, GetTrans(LNG_WILL_START_FILLING));
        break;
      case SCALE_WILL_RESUME:
        TFT_line_color(5, TFT_BLACK, TFT_ORANGE);
        TFT_line_print(5, GetTrans(LNG_WILL_RESUME_FILLING));
        break;
      case SCALE_JAR_FILL_FAST_SPEED:
        TFT_line_color(5, TFT_BLACK, TFT_ORANGE);
        TFT_line_print(5, GetTrans(LNG_FAST_FILLING), true);
        break;
      case SCALE_JAR_FILL_SLOW_SPEED:
        TFT_line_color(5, TFT_BLACK, TFT_ORANGE);
        TFT_line_print(5, GetTrans(LNG_SLOW_FILLING), true);
        break;
      case SCALE_JAR_FILLING_PAUSED:
        TFT_line_color(5, TFT_RED, TFT_DARKGREY);
        TFT_line_print(5, GetTrans(LNG_PAUSED_FILL));
        break;
      case SCALE_JAR_FILLED:
        sprintf(text, GetTrans(LNG_JAR_IS_READY), Products[product_chosen].Count);
        TFT_line_print(5, text);
        TFT_line_color(5, TFT_BLACK, TFT_GREEN);
        break;
    }
  }
  
  // what is the current weight telling us?
  if(bScaleStable)
  { if(GramsOnScale<10)weight_status = WEIGHT_EMPTY_SCALE;
    else if(abs(GramsOnScale - actual_tarra) <= SysParams[AUTO_JAR_TOLERANCE])weight_status = WEIGHT_EMPTY_JAR;
    else if((GramsOnScale) >= actual_tarra + target_net_weight)weight_status = WEIGHT_FULL_JAR; 
    else weight_status = WEIGHT_PARTIAL_JAR;
    if(dosing_state == DOSING_FINISHED) 
    { if((GramsOnScale) > actual_tarra + target_net_weight + SysParams[COULANCE]) // really too much
      { SysParams[CORRECTION]--;
      }
      else if((GramsOnScale) < actual_tarra + target_net_weight) // really too little
      { SysParams[CORRECTION]++;
      }
      setRotariesValue(SW_KORREKTUR, SysParams[CORRECTION]); // adjust the virtual knob
      dosing_state = DOSING_NEEDS_EXPECTED_TARRA; // for the new empty jar
    }
  }

  if(dosing_state == DOSING_NEEDS_EXPECTED_TARRA)
  { dosing_state = DOSING_STOPPED;
    actual_tarra = expected_tarra; // reset the tarra to value as given in setup
  }


  if(scale_status <= SCALE_JAR_PLACED)
  { NewWeight = GramsOnScale; 
  }
  else
  { NewWeight = GramsOnScale - actual_tarra; // while dosing, subtract weight of empty jar
  }

  
  if(weight_status == WEIGHT_EMPTY_SCALE) // works also as emergency stop
  { scale_status = SCALE_EMPTY;
    actual_angle = SysParams[SERVOMIN] + offset_winkel;
    if (SysParams[AUTOSTART] != 1)runmode = 0;
    dosing_state = DOSING_STOPPED;
  }
  
  if(dosing_state==DOSING_STOPPED && scale_status == SCALE_EMPTY) // not started filling yet
  { if(weight_status == WEIGHT_EMPTY_JAR)
    { scale_status = SCALE_JAR_PLACED;
      Unstable10mS = 150; // let's wait a 1,5 second before acting
      if (SysParams[AUTOSTART]!= 1)runmode = 0; // in semi mode, always need to start manually
    } 
    else if(weight_status == WEIGHT_FULL_JAR)
    { scale_status = SCALE_JAR_FILLED;
    }
    else if(weight_status == WEIGHT_PARTIAL_JAR)
    { scale_status = SCALE_WAIT_RESUME;
      Unstable10mS = 150;
      runmode = 1; // put in pauze automatically, bit weird if a half empty jar is placed
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

  if(((scale_status == SCALE_WILL_START) || (scale_status == SCALE_WAIT_START) || (scale_status == SCALE_WAIT_RESUME)) && !Unstable10mS && (runmode==2))
  { scale_status = SCALE_JAR_FILL_FAST_SPEED;
    dosing_state = DOSING_FAST;
  }


  if(dosing_state != old_dosing_state) // set the encoder to make eventual adjustments to servo according full or fine dosing state
  { old_dosing_state = dosing_state;
    if(dosing_state == DOSING_FAST) 
    { initRotaries(SW_WINKEL, fast_dosage_percent, 0, 100, 1); 
    }
    else if(dosing_state == DOSING_SLOW) 
    { initRotaries(SW_WINKEL, slow_dosage_angle, 0, 100, 1); 
    }
    else
    { initRotaries(SW_WINKEL, fast_dosage_percent, 0, 100, 1); 
    }
  }

  // this is the part where we actively adjust the servo as we fill the jar
  target_bruto_weight = target_net_weight + actual_tarra;
  // if not stopped or paused 
  if((runmode==2) && (dosing_state == DOSING_FAST || dosing_state == DOSING_SLOW))
  { if(dosing_state == DOSING_FAST) actual_angle = fast_dosage_angle;
    else actual_angle = slow_dosage_angle;
    grams_to_go = (target_bruto_weight - GramsOnScale + SysParams[CORRECTION]); // correction is between 0 and a negative number, to close more early
    net_weight_in_jar = GramsOnScale - actual_tarra;
    if(net_weight_in_jar > slow_down_at_net_weight)  // now squeeze the valve gradually and elegant
    { dosing_state = DOSING_SLOW;
      scale_status = SCALE_JAR_FILL_SLOW_SPEED;
      if(grams_to_go < 0)grams_to_go=0; // don't act silly if we have an overfilled jar
      // calculate a elegant angle for closing the valve, between minimum allowed and chosen angle for fine_dosage

      actual_angle =  (round) ( (grams_to_go / (target_net_weight - slow_down_at_net_weight + SysParams[CORRECTION]) * (slow_dosage_angle - SysParams[SERVOMIN])) + SysParams[SERVOMIN]);
    }
    if(grams_to_go <= 0) // jar is filled
    { dosing_state = DOSING_FINISHED;
      scale_status = SCALE_JAR_FILLED;
      Products[product_chosen].Count++;
      SaveParameters();
    }
  }

    
  target_bruto_weight = target_net_weight + actual_tarra + SysParams[CORRECTION] + autokorrektur_gr;
  // Anpassung des Autokorrektur-Werts
  if (SysParams[AUTO_CORRECTION] == 1) {                                                       
    if ( auto_aktiv == 1 && servo_aktiv == 0 && actual_angle == SysParams[SERVOMIN] + offset_winkel && gewicht >= target_bruto_weight && sammler_num <= 5) {     
      voll = true;                       
      if (gewicht == gewicht_vorher && sammler_num < 5) { // wir wollen 5x das identische Gewicht sehen  
        sammler_num++;
      } 
      else if (gewicht != gewicht_vorher) {               // sonst gewichtsänderung nachführen
        gewicht_vorher = gewicht;
        sammler_num = 0;
      } 
      else if (sammler_num == 5) {                        // gewicht ist 5x identisch, autokorrektur bestimmen
        autokorrektur_gr = (target_net_weight + SysParams[COULANCE] + actual_tarra) - (gewicht - autokorrektur_gr);
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
        Products[product_chosen].Count++;
        gezaehlt = true;
      }
    }
  }
  
  
  // update line 3 with new correction
  if (korr_alt != SysParams[CORRECTION] + autokorrektur_gr) 
  { sprintf(text, "Corr %dg", SysParams[CORRECTION] + autokorrektur_gr);
    TFT_line_print(3, text, rotary_select == SW_KORREKTUR );
    korr_alt = SysParams[CORRECTION] + autokorrektur_gr;
  }
  
  // encoder clicked
  if (rotary_select != rotary_select_alt) 
  { if (rotary_select == SW_MENU) 
    { TFT_line_blink(4, true); // blink the chosen product text
    }
    else TFT_line_blink(4, false);
    rotary_select_alt = rotary_select;
  }

  // Jar Icon Filling Indicator
  if (expected_tarra > 0) {
    // if(dosing_state == DOSING_STOPPED)gewicht = 0;
    if(weight_status == WEIGHT_PARTIAL_JAR || weight_status == WEIGHT_FULL_JAR || dosing_state==DOSING_FAST || dosing_state == DOSING_SLOW || scale_status == SCALE_JAR_FILLED)gewicht = NewWeight;
    else gewicht = 0;
    if (gewicht != gewicht_alt) {
      if (Products[product_chosen].Gewicht > gewicht) {
        JarIconFilledColor = TFT_RED;
      }
      else if (gewicht >= Products[product_chosen].Gewicht and gewicht <= Products[product_chosen].Gewicht + SysParams[COULANCE]){
        JarIconFilledColor = TFT_GREEN;
      }
      else {
        JarIconFilledColor = TFT_ORANGE;
      }
      gewicht_alt = gewicht;
      progressbar = 58.0*((float)gewicht/(float)(Products[product_chosen].Gewicht));
      progressbar = constrain(progressbar,0,68); // empty area of jar icon is 68 pixels high
      JarIconFilled = progressbar;
    }
  }
  
  // CURRENT STUFF
//  if (alarm_overcurrent) {i = 1;}
  while (i > 0) {
    inawatchdog = 0;                    //schalte die kontiunirliche INA Messung aus
    //Servo ist zu
    if (servo.read() <= SysParams[SERVOMIN]  + offset_winkel and offset_winkel < 3) {
      while(offset_winkel < 3 and SysParams[SERVOMAXCURRENT] < current_mA) {
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
}
