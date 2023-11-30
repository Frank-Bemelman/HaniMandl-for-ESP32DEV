#include <Arduino.h>

#include <ESP32Servo.h>             
#include <TFT_eSPI.h>               // more versatile display library
#include <Preferences.h>            // eeprom acces library
#include <nvs_flash.h>              /* aus dem BSP von expressif, wird verfügbar wenn das richtige Board ausgewählt ist */

#include "hani.h"
#include <HX711_ADC.h>
#include "language.h"
#include "menustructure.h"


extern int GetCurrent(int count);
extern void buzzer(int type);
extern void initRotaries( int rotary_mode, int rotary_value, int rotary_min, int rotary_max, int rotary_step );
extern int getRotariesValue( int rotary_mode );
extern void setRotariesValue( int rotary_mode, int rotary_value );

extern int alarm_overcurrent;          // Alarmflag wen der Servo zuwiel Strom zieht
extern int current_mA;                     // Strom welcher der Servo zieht
extern int offset_winkel;              // Offset in Grad vom Schlieswinkel wen der Servo Überstrom hatte (max +3Grad vom eingestelten Winkel min)
extern Servo servo;
extern int inawatchdog;                // 0 = aus, 1 = ein / wird benötigt um INA messung auszusetzen
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
extern long rawtareoffset;                  // Gewicht der leeren Waage
extern float CalibrationFactor;            // Skalierungsfaktor für Werte der Waage
extern int fmenge;                         // ausgewählte Füllmenge
extern int korrektur;                      // Korrekturwert für Abfüllmenge

extern int progressbar;                // Variable für den Progressbar
extern int modus;                     // Bei Modus-Wechsel den Servo auf Minimum fahren
extern int auto_aktiv;                 // Für Automatikmodus - System ein/aus?
extern int winkel;                         // aktueller Servo-Winkel
extern float fein_dosier_gewicht;     // float wegen Berechnung des Schliesswinkels
extern int servo_aktiv;                // Servo aktivieren ja/nein
extern int StringLenght(String a);
extern int rotary_select;
extern int show_current;               // 0 = aus, 1 = ein / Zeige den Strom an auch wenn der INA ignoriert wird
extern bool bINA219_installed;   
extern bool gezaehlt;               // Kud Zähl-Flag
extern HX711_ADC LoadCell;

extern Preferences preferences;

extern JarType JarTypes[];
extern ProductParameter Products[];

extern int SysParams[];
extern int GramsOnScale;
extern int ScaleStable;
int FromLanguage = 0;



extern void SetupMyDisplay(void);
extern void TFT_line_print(int line, const char *content, bool blink = false);
extern void TFT_line_color(int line, int textcolor, int backgroundcolor);
extern void TFT_line_blink(int line, bool blink); 
extern TFTline MyDisplay[];
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

extern int EditMenu;

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

char MissedInTranslation[128];

char* GetTrans(int idx)
{ int n = 0;
  while(Trans[n].index < LNG_LAST)
  { if(Trans[n].index == idx)
    { if(Trans[n].word[SysParams[LANGUAGE]][0]=='?') // Translation not defined yet, replace for hint & english
      { sprintf(MissedInTranslation, "%s-%s", Trans[n].word[SysParams[LANGUAGE]], Trans[n].word[0]);
        return MissedInTranslation;
      }
      else return Trans[n].word[SysParams[LANGUAGE]];
    }
    n++;
  }
  return Trans[n].word[SysParams[LANGUAGE]]; // last translation contains "???"
} 
 
void MenuHandler(void)
{ static int state = 0;
  static int currentmenu;
  char text[64];

  if(modus != MODE_SETUPMENU)
  { FromLanguage = SysParams[LANGUAGE];
    state = 0; // new arrival
  }

  switch(state)
  { case 0:
      // Select the right menu background stuff
      SelectMenu(HANI_MENU);
      modus = MODE_SETUPMENU;
      currentmenu = SysParams[LASTMENUSED];
      sprintf(text, "%d - %s", currentmenu+1, GetTrans(BigMenu[currentmenu].menuidx));
      TFT_line_print(0, text);
      TFT_line_color(1, TFT_BLACK, TFT_ORANGE);
      TFT_line_print(1, GetTrans(BigMenu[currentmenu].longdescriptionidx));
      sprintf(text, "%02d*%02d", currentmenu, SETUP_ENDOFMENU); // print the dots
      TFT_line_print(5, text);
      TFT_line_color(5, TFT_WHITE, TFT_BLACK);
      initRotaries(SW_MENU, currentmenu, SETUP_STARTOFMENU, SETUP_ENDOFMENU, 1);
      state++;
      return;
    case 1: // wandering in top menu selection
      rotary_select = SW_MENU;
      currentmenu = getRotariesValue(SW_MENU);
      if(currentmenu!=SysParams[LASTMENUSED])
      { SysParams[LASTMENUSED] = currentmenu;
        sprintf(text, "%d - %s", currentmenu+1, GetTrans(BigMenu[currentmenu].menuidx));
        TFT_line_print(0, text);   
        TFT_line_color(1, TFT_BLACK, TFT_ORANGE);
        TFT_line_print(1, GetTrans(BigMenu[currentmenu].longdescriptionidx));
        sprintf(text, "%02d*%02d", currentmenu, SETUP_ENDOFMENU); // print the dots
        TFT_line_print(5, text);
      }
      break;
    default:
      break;
  }  

  if(encoder_button_f == deb_encoder_button) // state change 
  { encoder_button_f = 1234;  
    if(deb_encoder_button)  // pressed?)
    { TFT_line_print(1, "");
      TFT_line_print(5, "");
      delay(100); // give some time to background task for display update
      if(currentmenu == SETUP_CALIBRATE)CalibrateScale(); 
      else ParameterMenu(currentmenu);  
      ActLcdMode = 999; // force new display build
      modus = -1;
    } 
  }     
}


void CalibrateScale(void)
 { float gewicht_raw;
   int state = 0;
   int calibrationweight;
   int oldcalibrationweight = -1;
   unsigned long now;
   char text[64];

   IsPulsed(&bStartButtonPulsed); // reset the button flag
   IsPulsed(&bStopButtonPulsed); // reset the button flag
   IsPulsed(&bEncoderButtonPulsed); // reset the button flag
   IsPulsed(&bSetupSwitchPulsed); // reset the button flag
   IsPulsed(&bAutoSwitchPulsed); // reset the button flag
   IsPulsed(&bManualSwitchPulsed); // reset the button flag


   OldWeight = 0;
   while(true)
   { if(state==7) // waiting loop, nice to display weight after calibration
     { NewWeight = GramsOnScale;
     }

    
     if(IsPulsed(&bStopButtonPulsed) || !deb_setup_switch)
     { if(state<5)
       { LoadCell.setCalFactor(CalibrationFactor); 
       }
       state = 8; // Quick exit
     }

     switch(state)
     { case 0:
         TFT_line_color(3, TFT_BLACK, TFT_RED);
         TFT_line_print(3, GetTrans(LNG_PLEASE_EMPTY_SCALE), true);
         TFT_line_print(5, GetTrans(LNG_CONTINUE_WHEN_DONE));
         state++; 
         break;
       case 1: // wait for button
         if(IsPulsed(&bEncoderButtonPulsed))  // encoder pressed?)now
         { LoadCell.setCalFactor(1);
           TFT_line_print(3, GetTrans(LNG_SCALE_TARRED), true);
           state++;
         }
         break;
       case 2: 
         LoadCell.tare(); // returns after actual tarring
         state++;
         break;  
       case 3: 
         TFT_line_print(3, GetTrans(LNG_PLACE_KNOWN_WEIGHT), true);
         TFT_line_print(5, GetTrans(LNG_CONTINUE_WHEN_DONE));
         initRotaries(SW_MENU, SysParams[CALWEIGHT], 100, 9999, 1); 
         state++;
         break;
       case 4: // let user adjust the known weight
         calibrationweight = getRotariesValue(SW_MENU);
         if (calibrationweight != oldcalibrationweight)
         { oldcalibrationweight = calibrationweight;
           SysParams[CALWEIGHT] = calibrationweight;
           sprintf(text, "%s %d %s", GetTrans(LNG_CALIBRATE_AS), calibrationweight, GetTrans(LNG_GRAM));
           TFT_line_print(4, text);
         }
         if(IsPulsed(&bEncoderButtonPulsed))
         { state++; 
         }
         break;  
       case 5:
         gewicht_raw = GramsOnScale;
         CalibrationFactor = gewicht_raw / calibrationweight;
         LoadCell.setCalFactor(CalibrationFactor); 
         rawtareoffset = LoadCell.getTareOffset(); // should be zero after fresh calibration
         TFT_line_print(5, GetTrans(LNG_THANKS), true);
         SaveParameters();
         delay(1000);
         EditMenu = SETUP_CALIBRATE;  // bepaalt of gewicht geprint wordt
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
     delay(100); 
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

   EditMenu = menu;

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
         BigMenu[menu].line[editline].selected = false;
         if(BigMenu[menu].line[editline].parmtype == SET_TARRA)TFT_line_color(editline-scrollpos+1, TFT_WHITE, TFT_BLACK); // back to normal
         if(BigMenu[menu].line[editline].parmtype == SET_JAR_PRESET)
         { Products[editline].Gewicht = restorevalue;
           Products[editline].GlasTyp = restorevalue2;
         }
         else if(BigMenu[menu].line[editline].parmtype == SET_TO_ZERO)
         { Products[editline].Count = restorevalue;
         }
         else
         { SysParams[BigMenu[menu].line[editline].targetidx] = restorevalue;
         }
         SysParams[BigMenu[menu].line[editline].selected] = false;
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
             if(editline != (n+scrollpos))
             { TFT_line_color(n+1, TFT_WHITE, TFT_DARKGREY);
             }
             else
             { TFT_line_color(n+1, TFT_WHITE, TFT_BLACK); // display as chosen
             }
           }  
         }
         TFT_line_print(5, GetTrans(LNG_EXIT), editline==menuitems);
         if(editline==menuitems)
         { TFT_line_color(5, TFT_WHITE, TFT_BLACK);
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
             { BigMenu[menu].line[editline_old].selected = false;
               BigMenu[menu].line[editline].selected = true;
           
               TFT_line_blink(editline-scrollpos+1, true);
               if(MyDisplay[editline-scrollpos+1].scroll)  // is a a scrolling menuline, maybe a shortened text for this?
               { GetTextForMenuLine(text, menu, editline);
                 TFT_line_print(editline-scrollpos+1, text, true);
               }

               // setup encoder for this edit range
               if(BigMenu[menu].line[editline].parmtype==SET_JAR_PRESET)
               { restorevalue=Products[editline].Gewicht;
                 restorevalue2=Products[editline].GlasTyp;
                 TFT_line_print(5, "Please Set Net Weight");
                 initRotaries(SW_MENU, Products[editline].Gewicht, BigMenu[menu].line[editline].min, BigMenu[menu].line[editline].max, 1);
               }
               else if(BigMenu[menu].line[editline].parmtype==SET_TO_ZERO)
               {  restorevalue = Products[editline].Count;
                  initRotaries(SW_MENU, 0, BigMenu[menu].line[editline].min, BigMenu[menu].line[editline].max, 1);
               }
               else if(BigMenu[menu].line[editline].parmtype==SET_LANGUAGE)
               { FromLanguage = SysParams[LANGUAGE];
                 restorevalue = SysParams[BigMenu[menu].line[editline].targetidx];
                 initRotaries(SW_MENU, SysParams[BigMenu[menu].line[editline].targetidx], BigMenu[menu].line[editline].min, BigMenu[menu].line[editline].max, 1);
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
         { newvalue = GramsOnScale;
           if (newvalue<10)TFT_line_color(editline-scrollpos+1, TFT_RED, TFT_BLACK);
           else TFT_line_color(editline-scrollpos+1, TFT_GREEN, TFT_BLACK);
         }
         else if(BigMenu[menu].line[editline].parmtype == SET_DEGREES)
         { if(SysParams[LIVESETUP])SERVO_WRITE(newvalue);
         }
         


         if(editline<menuitems) // we have a target
         { if(BigMenu[menu].line[editline].parmtype==SET_JAR_PRESET) // different treatment of jar presets
           { if(column==1)
             { if(newvalue!=Products[editline].Gewicht)
               { Products[editline].Gewicht = newvalue;
                 GetTextForMenuLine(text, menu, editline);
                 TFT_line_print(editline-scrollpos+1, text, true);
               }
             }
             else // 2nd column
             { if(newvalue!=Products[editline].GlasTyp)
               { Products[editline].GlasTyp = newvalue;
                 GetTextForMenuLine(text, menu, editline);
                 TFT_line_print(editline-scrollpos+1, text, true);
               }
             }
           }
           else if (BigMenu[menu].line[editline].parmtype==SET_TO_ZERO)
           { if(newvalue!=Products[editline].Count)
              {Products[editline].Count = newvalue;
               GetTextForMenuLine(text, menu, editline);
               TFT_line_print(editline-scrollpos+1, text,true);
             }
           }
           else if (BigMenu[menu].line[editline].parmtype==SET_TARRA)
           { if(newvalue!=JarTypes[editline].tarra)
              {JarTypes[editline].tarra = newvalue;
               GetTextForMenuLine(text, menu, editline);
               TFT_line_print(editline-scrollpos+1, text, true);
             }
           }
           else if(newvalue!=SysParams[BigMenu[menu].line[editline].targetidx])
           { SysParams[BigMenu[menu].line[editline].targetidx] = newvalue;
             GetTextForMenuLine(text, menu, editline);
             TFT_line_print(editline-scrollpos+1, text, true);
           }

           if(IsPulsed(&bEncoderButtonPulsed))  // pressed to set/save?
           { if(column==BigMenu[menu].columns) // time to leave
             { TFT_line_blink(editline-scrollpos+1, false);
               if(BigMenu[menu].line[editline].parmtype==SET_LANGUAGE) 
               { FromLanguage = SysParams[LANGUAGE];
               }  
               if(BigMenu[menu].line[editline].parmtype == RESETPREFS)
               { preferences.begin("EEPROM", false);
                 preferences.clear();
                 preferences.end();
                 TFT_line_print(5, "Will Reboot Now!", true);
                 delay(2000);
                 ESP.restart();
               }
               if(BigMenu[menu].line[editline].parmtype == RESETEEPROM)
               { nvs_flash_erase();    // erase the NVS partition and...
                 nvs_flash_init();     // initialize the NVS partition.
                 //Da machen wir gerade einen restart
                 TFT_line_print(5, "Will Reboot Now!", true);
                 delay(2000);
                 ESP.restart();
               }
               if(BigMenu[menu].line[editline].parmtype == SET_TARRA)TFT_line_color(editline-scrollpos+1, TFT_WHITE, TFT_BLACK); // back to normal
               initRotaries(SW_MENU, editline, 0, menuitems, 1);
               column=1;  
               if(BigMenu[menu].line[editline].parmtype==SET_LANGUAGE)
               { sprintf(text, "%d - %s", menu+1, GetTrans(BigMenu[menu].menuidx));
                 TFT_line_print(0, text);
                 state=0; 
               }
               else state =1;
               BigMenu[menu].line[editline].selected = false; // not selected as target under edit anymore
               TFT_line_print(5, GetTrans(LNG_SAVED), true);
               SaveParameters();
               delay(1000);
               TFT_line_print(5, GetTrans(LNG_EXIT));
             }
             else
             { column++; 
               initRotaries(SW_MENU, Products[editline].GlasTyp, 0, 5, 1); // 5 different jar styles
               TFT_line_print(5, "Please Select Jar Type");

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
         TFT_line_print(5, GetTrans(LNG_THANKS), true);
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
      sprintf(text, "%dg - %s", Products[line].Gewicht, JarTypes[Products[line].GlasTyp].name);
      break;
    case SET_ON_OFF:
      if(SysParams[BigMenu[menu].line[line].targetidx])sprintf(text, "%s %s", GetTrans(BigMenu[menu].line[line].labelidx), GetTrans(LNG_ON));
      else sprintf(text, "%s %s", GetTrans(BigMenu[menu].line[line].labelidx), GetTrans(LNG_OFF));
      break;
    case SET_YES_NO:
      sprintf(text, "%s %s", GetTrans(BigMenu[menu].line[line].labelidx), (SysParams[BigMenu[menu].line[line].targetidx]==0) ? "No" : "Yes");
      break;
    case SET_GRAMS:
      sprintf(text, "%s %dg", GetTrans(BigMenu[menu].line[line].labelidx), targetvalue);
      break;
    case SET_INTEGER:
      sprintf(text, "%s %d", GetTrans(BigMenu[menu].line[line].labelidx), targetvalue);
      break;
    case SET_DEGREES:
      sprintf(text, "%s %d°", GetTrans(BigMenu[menu].line[line].labelidx), targetvalue);
      break;
    case SET_MILLIAMPSMAX:
      sprintf(text, "%s %dmA", GetTrans(BigMenu[menu].line[line].labelidx), targetvalue);
      break;
    case SET_LANGUAGE:
      sprintf(text, "%s - %s", Trans[LNG_LANGUAGE].word[FromLanguage], Trans[LNG_ENGLISH+targetvalue].word[FromLanguage]);
      break;

    case RESETPREFS:
    case RESETEEPROM:
      sprintf(text, "%s", GetTrans(BigMenu[menu].line[line].labelidx));
      break;
    case SET_TARRA:
      sprintf(text, "%s %dg", JarTypes[line].name, JarTypes[line].tarra);
      break;
    case SET_TO_ZERO:
      sprintf(text, "%dg %s %d", Products[line].Gewicht, JarTypes[Products[line].GlasTyp].name, Products[line].Count);
      break;
    case SET_GRAM_TOLERANCE:
      sprintf(text, "%s ±%dg", GetTrans(BigMenu[menu].line[line].labelidx), targetvalue);
      break;
    case SET_CHOSEN:
      if(BigMenu[menu].line[line].selected)
      { sprintf(text, "%dg+%dg %s", Products[targetvalue].Gewicht, SysParams[COULANCE], JarTypes[Products[targetvalue].GlasTyp].name);
      }
      else
      { sprintf(text, "%s %dg+%dg %s", GetTrans(LNG_DEFAULT_PRODUCT), Products[targetvalue].Gewicht, SysParams[COULANCE], JarTypes[Products[targetvalue].GlasTyp].name);
      }
      break;
    case SET_PERCENT:
      sprintf(text, "%s %d%%", GetTrans(BigMenu[menu].line[line].labelidx), targetvalue);
      break;


    default:
      sprintf(text, "%s", GetTrans(BigMenu[menu].line[line].labelidx));
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
  SysParams[MANUALTARRA] = 25;
  SysParams[AUTO_JAR_TOLERANCE] = 20; // 20 gram tolerance on empty jar
  SysParams[AUTO_CORRECTION] = 1;
  SysParams[CHOSENPRODUCT] = 0;
  SysParams[LASTMENUSED] = 0;
  SysParams[CALWEIGHT] = 500; // assumme this as calibration standard 
  
//  CalibrationFactor = 0; // assume not calibrated
}

void SaveParameters(void)
{ int parameteridx;
  char label[16];
  int n;
  Serial.println("Parameter Saving");
  preferences.begin("EEPROM", false);
  for(parameteridx=AUTOSTART; parameteridx<LASTPARAMETER;parameteridx++)
  { sprintf(label, "%d", parameteridx);
    if( SysParams[parameteridx] != preferences.getUInt(label, -12345)) // check if new value
    { preferences.putUInt(label, SysParams[parameteridx]);
      Serial.print("Parameter saved - ");
      Serial.print(label);
      Serial.print(" = ");
      Serial.println(SysParams[parameteridx]);

    }
  }

  // save other settings if need be
  if(CalibrationFactor != preferences.getFloat("calfaktor", 0))preferences.putFloat("calfaktor", CalibrationFactor);
  if(rawtareoffset !=  preferences.getUInt("rawtareoffset", 0))preferences.putUInt("rawtareoffset", rawtareoffset);

  for(n=0;n<6;n++)
  { sprintf(label, "jar%d-gewicht", n);
    if(Products[n].Gewicht != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Products[n].Gewicht);
    }
    sprintf(label, "jar%d-glastyp", n);
    if(Products[n].GlasTyp != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Products[n].GlasTyp);
    }
    sprintf(label, "jar%d-count", n);
    if(Products[n].Count != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, Products[n].Count);
    }
    sprintf(label, "jar%d-tara", n);
    if(JarTypes[n].tarra != preferences.getUInt(label, -12345))
    { preferences.putUInt(label, JarTypes[n].tarra);
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
  for(parameteridx=AUTOSTART; parameteridx<LASTPARAMETER;parameteridx++)
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

  CalibrationFactor = preferences.getFloat("calfaktor", 0);
  rawtareoffset = preferences.getUInt("rawtareoffset", 0);


    for(n=0;n<6;n++)
  { sprintf(label, "jar%d-gewicht", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Products[n].Gewicht = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

    sprintf(label, "jar%d-glastyp", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Products[n].GlasTyp = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

    sprintf(label, "jar%d-count", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)Products[n].Count = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

    sprintf(label, "jar%d-tara", n);
    val = preferences.getUInt(label, -12345);
    if(val != -12345)JarTypes[n].tarra = val;
    Serial.print("Parameter loaded - ");
    Serial.print(label);
    Serial.print(" = ");
    Serial.println(val);

  }

  preferences.end();
}


// eof