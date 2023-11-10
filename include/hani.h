// useful defines for all source files
#define FIRMWARE_VERSION "HANIMANDL V4.0"
#define HANI_LOGO 0
#define HANI_SETUP 1
#define HANI_AUTO 2
#define HANI_HAND 3


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
  int  lastpixelwidth;
  bool rounded; // background box rounded or not
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

