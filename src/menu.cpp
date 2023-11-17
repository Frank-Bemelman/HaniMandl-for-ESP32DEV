#include <Arduino.h>

#include <ESP32Servo.h>             
#include <Arduino_GFX_Library.h>    
#include <TFT_eSPI.h>               // more versatile display library
#include <Preferences.h>            // eeprom acces library

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

extern Preferences preferences;

extern JarName JarNames[];
extern JarParameter Jars[];

extern int SysParams[];


extern void SetupMyDisplay(void);
extern void TFT_line_print(int line, const char *content);
extern void TFT_line_color(int line, int textcolor, int backgroundcolor);
extern void TFT_line_blink(int line, bool blink); 
extern void SelectMenu(int menu); 
extern bool IsPulsed(bool *button);

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
void ParameterMenu(int menu);
void GetTextForMenuLine(char* text, int menu, int line); // build a single line with text and parameters 
void SetDefaultParameters(void);
void SaveParameters(void);
void LoadParameters(void);


 
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
      TFT_line_print(5, "Navigate And Select");
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
          //setupTara();              // Tara 
          ParameterMenu(0);
          break;
        case 1:   
          CalibrateScale();         // new menu
          setPreferences();
          break;
        case 2:   
//          setupFuellmenge();        // Füllmenge               
          ParameterMenu(2);
          break;  
        case 3:   
//          setupAutomatik();         // Autostart/Autokorrektur konfigurieren 
          ParameterMenu(3);
          break;  
        case 4:   
//          setupServoWinkel();       // Servostellungen Minimum, Maximum und Feindosierung
          ParameterMenu(4);
          break;  
        case 5:   
//          setupParameter();         // Sonstige Einstellungen
          ParameterMenu(5);
          break;  
        case 6:   
//          setupCounter();           // Zählwerk
          ParameterMenu(6);
          break;  
        case 7:   
//          setupTripCounter();       // Zählwerk Trip
          ParameterMenu(7);
          break;  
        case 8:   
          setupINA219();            // INA219 Setup    
          break;  
        case 9:   
          setupClearPrefs();        // EEPROM löschen
          break;  
        case 10:   
          ParameterMenu(10);  // languages
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



void ParameterMenu(int menu)
 { int state = 0;
   unsigned long now;
   int scrollpos = 0; // defines offset
   int editline = 0; // line to be changed
   int editline_old;
   int n;
   int menuitems;
   char text[128];
   int restorevalue;
   int restorevalue2;
   state = 0;
   int newvalue;
   int column; 

   IsPulsed(&bStartButtonPulsed); // reset the button flag
   IsPulsed(&bStopButtonPulsed); // reset the button flag
   IsPulsed(&bEncoderButtonPulsed); // reset the button flag
   IsPulsed(&bSetupSwitchPulsed); // reset the button flag
   IsPulsed(&bAutoSwitchPulsed); // reset the button flag
   IsPulsed(&bManualSwitchPulsed); // reset the button flag

   // first count menuitems
   menuitems=0;
   for(n=0;n<6;n++) // max 6 lines in menu
   { if(BigMenu[menu].line[n].targetidx != NOT_USED)menuitems++;
   }
   column = 1; // work on the first column

   while(true)
   { if(IsPulsed(&bStopButtonPulsed) || !deb_setup_switch)
     { if(state==4) // stop button and setup switch works as cancel while fiddling with the value
       { TFT_line_blink(editline-scrollpos+1,false); // unblink
         if(BigMenu[menu].line[editline].parmtype == SET_TARRA)TFT_line_color(editline-scrollpos+1, TFT_WHITE, TFT_BLACK); // back to normal
         if(BigMenu[menu].line[editline].parmtype == SET_JAR_PRESET)
         { Jars[editline].Gewicht = restorevalue;
           Jars[editline].GlasTyp = restorevalue2;
         }
         else if(BigMenu[menu].line[editline].parmtype == SET_TO_ZERO)
         { Jars[editline].Count = restorevalue;
         }
         else
         { SysParams[BigMenu[menu].line[editline].targetidx] = restorevalue;
         }
         GetTextForMenuLine(text, menu, editline);
         TFT_line_print(editline-scrollpos+1, text);
         initRotaries(SW_MENU, editline, 0, menuitems, 1);
         state = 1;  
       }
       else return;  
     }
    
     switch(state)
     { case 0: // (re)print content
         for(n=0;n<4;n++) // max 4 lines to print
         { if(BigMenu[menu].line[n+scrollpos].targetidx != NOT_USED)
           { GetTextForMenuLine(text, menu, n+scrollpos);
             TFT_line_print(n+1, text);
             if(editline != (n+scrollpos))TFT_line_color(n+1, TFT_WHITE, TFT_DARKGREY);
             else TFT_line_color(n+1, TFT_WHITE, TFT_BLACK); // display as selected
           }  
         }
         TFT_line_print(5, BigMenu[menu].bottomline);
         if(editline==menuitems)
         { TFT_line_color(5, TFT_WHITE, TFT_BLACK);
           TFT_line_blink(5, true);
         }
         else
         { TFT_line_color(5, TFT_WHITE, TFT_DARKGREY); 
         }
         initRotaries(SW_MENU, editline, 0, menuitems, 1);
         editline_old = editline; 
         state++; 
         break;

       case 1: 
         state++;
         break;         

       case 2: // wander through the available menu items
         editline = getRotariesValue(SW_MENU);
         if(editline!=editline_old)
         { if(editline+scrollpos>3) // menuitems = 6, editline = 0-6 (7 values)
           { if(editline<menuitems) // not on the exit line
             { scrollpos = editline-3;
             }
           }
           else
           { if(editline<scrollpos)
             { scrollpos = editline;
             } 
           }
           state = 0;
         }
         else
         { if(IsPulsed(&bEncoderButtonPulsed))  // choice is made
           { if(editline==menuitems) // on the exit line
             { state = 5; 
             }
             else
             { TFT_line_blink(editline-scrollpos+1, true);
               // setup encoder for this edit range
               if(BigMenu[menu].line[editline].parmtype==SET_JAR_PRESET)
               { restorevalue=Jars[editline].Gewicht;
                 restorevalue2=Jars[editline].GlasTyp;
                 initRotaries(SW_MENU, Jars[editline].Gewicht, BigMenu[menu].line[editline].min, BigMenu[menu].line[editline].max, 1);
               }
               else if(BigMenu[menu].line[editline].parmtype==SET_TO_ZERO)
               {  restorevalue = Jars[editline].Count;
                  initRotaries(SW_MENU, 0, BigMenu[menu].line[editline].min, BigMenu[menu].line[editline].max, 1);
               }
               else if(BigMenu[menu].line[editline].parmtype==SET_TRIPCOUNT)
               {  restorevalue = Jars[editline].TripCount;
                  initRotaries(SW_MENU, Jars[editline].TripCount, BigMenu[menu].line[editline].min, BigMenu[menu].line[editline].max, 1);
               }
               else
               { restorevalue = SysParams[BigMenu[menu].line[editline].targetidx];
                 initRotaries(SW_MENU, SysParams[BigMenu[menu].line[editline].targetidx], BigMenu[menu].line[editline].min, BigMenu[menu].line[editline].max, 1);
               }  
               state++;
             }  
           }
         }   
         break;

       case 3: // wait for button released
         state++;
         break;  
       case 4: // change the value with the rotary
         newvalue = getRotariesValue(SW_MENU);

         if(BigMenu[menu].line[editline].parmtype == SET_TARRA)
         { newvalue = round(LoadCell.getData());
           if (newvalue<10)TFT_line_color(editline-scrollpos+1, TFT_RED, TFT_BLACK);
           else TFT_line_color(editline-scrollpos+1, TFT_GREEN, TFT_BLACK);
         }
         else if(BigMenu[menu].line[editline].parmtype == SET_DEGREES)
         { if(SysParams[LIVESETUP])SERVO_WRITE(newvalue);
         }
         


         if(editline<menuitems) // we have a target
         { if(BigMenu[menu].line[editline].parmtype==SET_JAR_PRESET) // different treatment of jar presets
           { if(column==1)
             { if(newvalue!=Jars[editline].Gewicht)
               { Jars[editline].Gewicht = newvalue;
                 GetTextForMenuLine(text, menu, editline);
                 TFT_line_print(editline-scrollpos+1, text);
                 TFT_line_blink(editline-scrollpos+1, true);
               }
             }
             else // 2nd column
             { if(newvalue!=Jars[editline].GlasTyp)
               { Jars[editline].GlasTyp = newvalue;
                 GetTextForMenuLine(text, menu, editline);
                 TFT_line_print(editline-scrollpos+1, text);
                 TFT_line_blink(editline-scrollpos+1, true);
               }
             }
           }
           else if (BigMenu[menu].line[editline].parmtype==SET_TO_ZERO)
           { if(newvalue!=Jars[editline].Count)
              {Jars[editline].Count = newvalue;
               GetTextForMenuLine(text, menu, editline);
               TFT_line_print(editline-scrollpos+1, text);
               TFT_line_blink(editline-scrollpos+1, true);
             }
           }
           else if (BigMenu[menu].line[editline].parmtype==SET_TARRA)
           { if(newvalue!=Jars[editline].Tara)
              {Jars[editline].Tara = newvalue;
               GetTextForMenuLine(text, menu, editline);
               TFT_line_print(editline-scrollpos+1, text);
               TFT_line_blink(editline-scrollpos+1, true);
             }
           }
           else if (BigMenu[menu].line[editline].parmtype==SET_TRIPCOUNT)
           { if(newvalue!=Jars[editline].TripCount)
              {Jars[editline].TripCount = newvalue;
               GetTextForMenuLine(text, menu, editline);
               TFT_line_print(editline-scrollpos+1, text);
               TFT_line_blink(editline-scrollpos+1, true);
             }
           }
           
           else if(newvalue!=SysParams[BigMenu[menu].line[editline].targetidx])
           { SysParams[BigMenu[menu].line[editline].targetidx] = newvalue;
             GetTextForMenuLine(text, menu, editline);
             TFT_line_print(editline-scrollpos+1, text);
             TFT_line_blink(editline-scrollpos+1, true);
           }
           if(IsPulsed(&bEncoderButtonPulsed))  // pressed?
           { if(column==BigMenu[menu].columns) // time to leave
             { TFT_line_blink(editline-scrollpos+1, false);
               if(BigMenu[menu].line[editline].parmtype == SET_TARRA)TFT_line_color(editline-scrollpos+1, TFT_WHITE, TFT_BLACK); // back to normal
               initRotaries(SW_MENU, editline, 0, menuitems, 1);
               column=1;  
               state=1;  
               TFT_line_print(5, "Saved!");
               TFT_line_blink(5, true);
               SaveParameters();
               delay(1000);
               TFT_line_print(5, BigMenu[menu].bottomline);
             }
             else
             { column++; 
               initRotaries(SW_MENU, Jars[editline].GlasTyp, 0, 5, 1); // 5 different jar styles
             }
           }
         }
//         sprintf(text, "targetval %d", SysParams[BigMenu[menu].line[editline].targetidx]);
//         TFT_line_print(0, text);
         break;
       case 5: 
         state++; 
         break;  
       case 6:
         TFT_line_print(5, "Thank You!");
         TFT_line_blink(5, true);
         SaveParameters();
         state++;
         break;

       case 7:
         state++;
         now = millis(); 
         break;  
       
       case 8:
         if((millis()-now)>1000)state++;
         break;  
       case 9:
         EditMenu = 0;
         return;
         break;  
     }
     delay(10); 
   
   } 

}

void GetTextForMenuLine(char* text, int menu, int line)
{ int targetvalue;

  targetvalue = SysParams[BigMenu[menu].line[line].targetidx];
  
  switch(BigMenu[menu].line[line].parmtype)
  { case SET_JAR_PRESET:
      sprintf(text, "%dg - %s", Jars[line].Gewicht, JarNames[Jars[line].GlasTyp].name);
      
      break;
    case SET_ON_OFF:
      sprintf(text, "%s %s", BigMenu[menu].line[line].name, (SysParams[BigMenu[menu].line[line].targetidx]==0) ? "Off" : "On");
      break;
    case SET_YES_NO:
      sprintf(text, "%s %s", BigMenu[menu].line[line].name, (SysParams[BigMenu[menu].line[line].targetidx]==0) ? "No" : "Yes");
      break;
    case SET_GRAMS:
      sprintf(text, "%s %dg", BigMenu[menu].line[line].name, targetvalue);
      break;
    case SET_INTEGER:
      sprintf(text, "%s %d", BigMenu[menu].line[line].name, targetvalue);
      break;
    case SET_DEGREES:
      sprintf(text, "%s %d°", BigMenu[menu].line[line].name, targetvalue);
      break;
    case SET_CURRENT:
      sprintf(text, "%smA", BigMenu[menu].line[line].name, targetvalue);
      break;
    case SET_LANGUAGE:
      sprintf(text, "%s", BigMenu[menu].line[line].name);
      break;
    case SET_TARRA:
      sprintf(text, "%s %dg", JarNames[line].name, Jars[line].Tara);
      break;
    case SET_TO_ZERO:
      sprintf(text, "%dg %s %d", Jars[line].Gewicht, JarNames[Jars[line].GlasTyp].name, Jars[line].Count);
      break;
    case SET_TRIPCOUNT:
      sprintf(text, "%d-%s %d", Jars[line].Gewicht, JarNames[Jars[line].GlasTyp].name, Jars[line].TripCount);
      break;
    default:
      break;
  }
}

// set default values
void SetDefaultParameters(void)
{ int menu;
  int line;
  for(menu=SETUP_STARTOFMENU; menu<SETUP_ENDOFMENU;menu++)
  { for(line=0;line<6;line++)
    { SysParams[BigMenu[menu].line[line].targetidx] = BigMenu[menu].line[line].value;
    }
  }
  SysParams[LANGUAGE] = 0; // default is English
}

void SaveParameters(void)
{ int parameteridx;
  char label[16];
  int n;
  Serial.println("Parameter Saving");
  preferences.begin("EEPROM", false);
  for(parameteridx=AUTO_START; parameteridx<LASTPARAMETER;parameteridx++)
  { sprintf(label, "%d", parameteridx);
    if( SysParams[parameteridx] != preferences.getUInt(label, -12345)) // check if new value
    { preferences.putUInt(label, SysParams[parameteridx]);
      Serial.print("Parameter saved - ");
      Serial.print(label);
      Serial.print(" = ");
      Serial.println(SysParams[parameteridx]);

    }
  }

  for(n=0;n<6;n++)
  { sprintf(label, "jar%d-gewicht", n);
    if(Jars[n].Gewicht != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Jars[n].Gewicht);
    }
    sprintf(label, "jar%d-glastyp", n);
    if(Jars[n].GlasTyp != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Jars[n].GlasTyp);
    }
    sprintf(label, "jar%d-tara", n);
    if(Jars[n].Tara != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Jars[n].Tara);
    }
    sprintf(label, "jar%d-tripcount", n);
    if(Jars[n].TripCount != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Jars[n].TripCount);
    }
    sprintf(label, "jar%d-count", n);
    if(Jars[n].Count != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Jars[n].Count);
    }
  }
  preferences.end();
}

void LoadParameters(void)
{ int parameteridx;
  int val;
  char label[16];
  int n;
  Serial.println("Parameter loading");
  preferences.begin("EEPROM", false);
  for(parameteridx=AUTO_START; parameteridx<LASTPARAMETER;parameteridx++)
  { sprintf(label, "%d", parameteridx);
    val = preferences.getUInt(label, -12345); // see if we have that
    if(val != -12345)
    { SysParams[parameteridx] = val;
      Serial.print("Parameter loaded - ");
      Serial.print(label);
      Serial.print(" = ");
      Serial.println(SysParams[parameteridx]);
    }
  }

    for(n=0;n<6;n++)
  { sprintf(label, "jar%d-gewicht", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Jars[n].Gewicht = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

    sprintf(label, "jar%d-glastyp", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Jars[n].GlasTyp = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

    sprintf(label, "jar%d-tara", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Jars[n].Tara = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

    sprintf(label, "jar%d-tripcount", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Jars[n].TripCount = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

    sprintf(label, "jar%d-count", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Jars[n].Count = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

  }

  preferences.end();
}


// eof