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
};

struct JarName
{ char name[32];
  char shortname[4];
};

// Füllmengen für 5 verschiedene Gläser
struct JarParameter { 
  int Gewicht;
  int GlasTyp;
  int Tara;
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
};

struct Menu
{ char menuname[32];
  char menuheader[64];
  MenuLine line[6];
  char bottomline[32];
  int columns;
}; 

//struct Setup
//{/ Menu MyMenu[];
//}; 