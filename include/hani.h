// useful defines for all source files
#define FIRMWARE_VERSION "HANIMANDL V4.0"

// rotary knob has different targets
#define SW_WINKEL    0
#define SW_KORREKTUR 1
#define SW_MENU      2

// operation mode as set by 3 way switch
#define MODE_SETUP       0
#define MODE_AUTOMATIK   1
#define MODE_HANDBETRIEB 2
#define MODE_SETUPMENU   3


// Buzzer Sounds
#define BUZZER_SHORT   1
#define BUZZER_LONG    2
#define BUZZER_SUCCESS 3
#define BUZZER_ERROR   4

//#define SERVO_REVERSED       // Servo invertieren, falls der Quetschhahn von links geöffnet wird. Mindestens ein Exemplar bekannt
#ifdef SERVO_REVERSED
  #define SERVO_WRITE(n)     servo.write(180-n)
#else
  #define SERVO_WRITE(n)     servo.write(n)
#endif




#define HANI_LOGO 0  // startup display
#define HANI_SETUP 1 // setup menu
#define HANI_AUTO 2  // automatic filling display
#define HANI_HAND 3  // manual operation display
#define HANI_MENU 4  // new style menu

#define SETUP_STARTOFMENU 0
#define SETUP_TARRA 0
#define SETUP_CALIBRATE 1
#define SETUP_NETWEIGHTS 2
#define SETUP_AUTOS 3
#define SETUP_SERVO 4
#define SETUP_PARAMS 5
#define SETUP_COUNTERS 6
#define SETUP_BATCHES 7
#define SETUP_INA219 8
#define SETUP_RESET 9
#define SETUP_LANGUAGE 10
#define SETUP_ENDOFMENU 10 // adjust this as menu gets expanded



// for lines with backgrounds, rounded bars with text
struct TFTline
{ bool refresh;
  char content[256];
  bool scroll;
  int  length;
  int  pixelwidth;
  int  scrollpos;
  int  scrolldelay;
  int  nchar;
  int  toeat;
  int  noffset;
  int  textcolor;
  int  backgroundcolor;
  int  canvascolor;
  bool blink;
  bool blinkold;
  int  lastpixelwidth;
  bool rounded; // background box rounded or not
  int  dotactive; // for bar with dots
  int  dotmax; // max dots in bar
};

// structure that describes the various jars we use
struct JarType
{ char name[32];
  char shortname[4];
  int tarra; // weight of empty jar
};

// Füllmengen für 5 verschiedene Gläser 
struct ProductParameter { 
  int Gewicht;
  int GlasTyp;
  int TripCount;
  int Count;
};

#define SWITCH_SETUP 4 
#define SWITCH_AUTO 15
#define BUTTON_START 12 
#define BUTTON_STOP 27 


#define ROTARY_ENCODER_A 33
#define ROTARY_ENCODER_B 26
#define ENCODER_BUTTON 32
//#define ROTARY_ENCODER_BUTTON_PIN 16
//#define ROTARY_ENCODER_VCC_PIN -1
//#define ROTARY_ENCODER_STEPS 2

#define SERVO_PIN 2

struct MenuLine
{ char name[32];
  int  min;
  int  max;
  int  value;
  int  selected;
  int  targetidx; // indexes to system parameter array
  int  parmtype; // type of parameter
};

struct Menu
{ char menuname[32];
  char menuheader[64];
  MenuLine line[6];
  char bottomline[32];
  int columns;
}; 

// system parameters and settings are stored in array
// indexes as follows
#define NOT_USED 0
#define AUTOSTART 1
#define AUTO_JAR_TOLERANCE 2
#define CORRECTION 3
#define AUTO_CORRECTION 4
#define COULANCE 5

#define LIVESETUP 6
#define SERVOMIN 7
#define SERVOFINEDOS 8
#define SERVOMAX 9

#define JARSUSED 10
#define MANUALTARRA 11

#define JARARRAY 22

#define BUZZER 28
#define LED 29


#define LANGUAGE 36




#define LASTPARAMETER 100

// types of parameters to edit
#define SET_JAR_PRESET 0
#define SET_ON_OFF 1
#define SET_YES_NO 2
#define SET_GRAMS 3
#define SET_INTEGER 4
#define SET_DEGREES 5
#define SET_CURRENT 6
#define SET_LANGUAGE 7
#define SET_TARRA 8
#define SET_TO_ZERO 9
#define SET_TRIPCOUNT 10
#define SET_CLICK 11
#define RESETPREFS 12
#define RESETEEPROM 13
#define SET_GRAM_TOLERANCE 14


// language stuff, lot todo

#define LNG_FIRST 0
#define LNG_SET_TARA_VAL 0
#define LNG_CALIBRATE_SCALE 1
#define LNG_WEIGTH_PRESET 2
#define LNG_SAVE_AND_EXIT 3
#define LNG_LAST 4

struct Trans
{ int  index; // a define value such as LNG_SET_TARA_VAL
  char name[6][32]; // 6 translations of one text
};

// Denstrukturen für Rotary Encoder
struct rotary {                        
  int Value[3];
  int Minimum;
  int Maximum;
  int Step;
};

#define SCALE_EMPTY 0
#define SCALE_JAR_PLACED 1
#define SCALE_WAIT_START 2
#define SCALE_WAIT_RESUME 3
#define SCALE_WILL_START 4
#define SCALE_WILL_RESUME 5
#define SCALE_JAR_FILL_FULL_SPEED 6
#define SCALE_JAR_FILL_SLOW_SPEED 7
#define SCALE_JAR_FILLING_PAUSED 8
#define SCALE_JAR_FILLED 9

#define WEIGHT_EMPTY_SCALE 0
#define WEIGHT_EMPTY_JAR 1
#define WEIGHT_FILLING_JAR 2
#define WEIGHT_FULL_JAR 3

#define DOSING_STOPPED 0
#define DOSING_WAIT_START 1
#define DOSING_FULL 2
#define DOSING_FINE 3

