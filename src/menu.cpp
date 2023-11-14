#include <Arduino.h>

#include <ESP32Servo.h>             /* aus dem Bibliotheksverwalter */
#include <Arduino_GFX_Library.h>    /* aus dem Bibliotheksverwalter */
#include <TFT_eSPI.h>               // more versatile display library

#include "hani.h"
#include <HX711_ADC.h>

extern void setPreferences(void);
extern int GetCurrent(int count);
extern void buzzer(byte type);
extern void initRotaries( int rotary_mode, int rotary_value, int rotary_min, int rotary_max, int rotary_step );
extern int getRotariesValue( int rotary_mode );
extern void setRotariesValue( int rotary_mode, int rotary_value );
extern void setupTara();              // Tara 
extern void setupCalibration();       // Kalibrieren
extern void setupFuellmenge();        // Füllmenge               
extern void setupAutomatik();         // Autostart/Autokorrektur konfigurieren 
extern void setupServoWinkel();       // Servostellungen Minimum, Maximum und Feindosierung
extern void setupParameter();         // Sonstige Einstellungen
extern void setupCounter();           // Zählwerk
extern void setupTripCounter();       // Zählwerk Trip
extern void setupINA219();            // INA219 Setup    
extern void setupClearPrefs();        // EEPROM löschen


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
extern int kali_gewicht; 

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

extern int start_button_f;
extern int stop_button_f;
extern int encoder_button_f;
extern int setup_switch_f;
extern int auto_switch_f;
extern int manual_switch_f;

extern TFT_eSPI tft;

extern int LastMenu;
extern int CurrentMenu;
extern int EditMenu;

extern Menu BigMenu[];
extern int ActLcdMode;
extern int NewWeight;
extern int OldWeight;


#define Y_SERVODATA 34

// local functions
void CalibrateScale(void);

void Menu(void)
{ static int state = 0;
  static int newmenu;
  static int editmode = 0;

  if(modus != MODE_SETUPMENU)state = 0; // new arrival

  switch(state)
  { case 0:
      // Select the right menu background stuff
      SelectMenu(HANI_MENU);
      modus = MODE_SETUPMENU;
      CurrentMenu = LastMenu;
      TFT_line_print(0, BigMenu[CurrentMenu].menuname);
      TFT_line_print(1, BigMenu[CurrentMenu].menuheader);
      TFT_line_print(5, "Choose & Select A Menu");
      TFT_line_blink(5, true);
      initRotaries(SW_MENU, LastMenu, SETUP_STARTOFMENU, SETUP_ENDOFMENU, 1);
      state++;
      return;
    case 1: // wandering in top menu selection
      rotary_select = SW_MENU;
      CurrentMenu = getRotariesValue(SW_MENU);
//      if(stop_button_f == deb_stop_button) // state change blue button next to display
//      { stop_button_f = 1234;  
//        if(deb_stop_button)  // pressed?
//        { CurrentMenu++;
//          if(CurrentMenu>SETUP_ENDOFMENU)CurrentMenu = SETUP_STARTOFMENU;
//          setRotariesValue( SW_MENU, CurrentMenu); 
//        }
//      }  
      if(CurrentMenu!=LastMenu)
      { LastMenu = CurrentMenu;
        TFT_line_print(0, BigMenu[CurrentMenu].menuname);
        TFT_line_print(1, BigMenu[CurrentMenu].menuheader);
      }

      break;
    default:
      break;
  }  

  if(encoder_button_f == deb_encoder_button) // state change 
  { encoder_button_f = 1234;  
    if(deb_encoder_button)  // pressed?)
    { editmode = true;
      TFT_line_print(1, "");
      TFT_line_print(5, "");
      delay(100); // give some time to background task for display update

      // use the old menus
      switch(LastMenu)
      { case 0:   
          setupTara();              // Tara 
          break;
        case 1:   
          CalibrateScale();         // new menu
          setPreferences();
          break;
        case 2:   
          setupFuellmenge();        // Füllmenge               
          break;  
        case 3:   
          setupAutomatik();         // Autostart/Autokorrektur konfigurieren 
          break;  
        case 4:   
          setupServoWinkel();       // Servostellungen Minimum, Maximum und Feindosierung
          break;  
        case 5:   
          setupParameter();         // Sonstige Einstellungen
          break;  
        case 6:   
          setupCounter();           // Zählwerk
          break;  
        case 7:   
          setupTripCounter();       // Zählwerk Trip
          break;  
        case 8:   
          setupINA219();            // INA219 Setup    
          break;  
        case 9:   
          setupClearPrefs();        // EEPROM löschen
          break;  
        case 10:   
          break;  
        default:   
           break;  
       }
       editmode = false;
       ActLcdMode = 999; // force new display build
       CurrentMenu = 999; 
       modus = -1;
    } 
  }     
}


void CalibrateScale(void)
 { float gewicht_raw;
   int state = 0;
   int gewicht_alt = -9999;
   int kali_gewicht_old = -9999;
   unsigned long now;

   OldWeight = 0;
   while(true)
   { if(state==7) // waiting loop, nice to display weight 
     { NewWeight = round(LoadCell.getData());
     }

    
     if(deb_stop_button || !deb_setup_switch)
     { if(state<5)
       { LoadCell.setCalFactor(faktor); 
       }
       state = 8; // Quick exit
     }
     switch(state)
     { case 0:
         TFT_line_color(4, TFT_BLACK, TFT_RED);
         TFT_line_print(4, "Empty Scale Please!");
         TFT_line_blink(4, true);
         TFT_line_print(5, "Continue When Done");
         if(!deb_encoder_button) state++; // proceed when button released
         break;
       case 1: // wait for button
         if(deb_encoder_button)  // pressed?)
         { LoadCell.setCalFactor(1);
           LoadCell.tare();
           delay(500);
           state++;
         }
         break;
       case 2: // wait for button released
         if(!deb_encoder_button)  // pressed?)
         { state++;
         }
         break;  
       case 3: // wait for button released
         TFT_line_color(3, TFT_BLACK, TFT_RED);
         TFT_line_print(3, "Place A Known Weight");
         TFT_line_blink(3, true);
         TFT_line_print(5, "Continue When Done");
         initRotaries(SW_MENU, kali_gewicht, 100, 9999, 1); 
         state++;
       case 4: // let user adjust the known weight
         kali_gewicht = getRotariesValue(SW_MENU);
         if (kali_gewicht != kali_gewicht_old)
         {  sprintf(ausgabe, "Calibrate As %d gram", kali_gewicht);
            TFT_line_print(4, ausgabe);
         }
         if(deb_encoder_button)  // pressed?)
         { state++; 
         }
         break;  
       case 5:
         gewicht_raw = round(LoadCell.getData());
         faktor = gewicht_raw / kali_gewicht;
         LoadCell.setCalFactor(faktor); 
         gewicht_leer = LoadCell.getTareOffset(); 
         TFT_line_print(3, "");
         TFT_line_print(4, "");
         TFT_line_print(5, "Thank You!");
         TFT_line_blink(5, true);
         EditMenu = CurrentMenu;  // bepaalt of gewicht geprint wordt
         state++;
         break;

       case 6:
         state++;
         now = millis(); 
         break;  
       
       case 7:
         if((millis()-now)>2500)state++;
         break;  
       case 8:
         EditMenu = 0;
         return;
         break;  
     }
     delay(10); 
   
   } 

}