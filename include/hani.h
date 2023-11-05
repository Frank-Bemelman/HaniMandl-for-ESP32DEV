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
  int length;
  int pixelwidth;
  int scrollpos;
  int scrolldelay;
  int nchar;
  int toeat;
  int noffset;
  int textcolor;
  int backgroundcolor;
  int canvascolor;
};