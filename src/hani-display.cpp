#include <Arduino.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(240,320);

#include "hani.h"
#include "Arialnarrow.h"
#include "Arialbd72.h"

#include "flowers.h"


bool bScrollNow = false;
bool bUpdateDisplay = false;
bool bBlinkDisplay = false;
bool bBlinkOld = false;
int  BlinkTimer10mS = 0;
int  ActLcdMode = 999; 

bool bPrintWeight = false;
int  NewWeight = 0;
int  OldWeight = -1234;


SemaphoreHandle_t  xDisplayMutex = NULL;

int BackGroundColor = TFT_DARKGREY; // default backgroundcolor to use for printing new text lines
int TextColor = TFT_WHITE; // default textcolor to use for printing new lines


char Credits[] = "Original Idea - M. Vasterling - Other Contributors - M. Wetzel - C. Gruber - A. Holzhammer - M. Junker - J. Kuder - J. Bruker - F. Bemelman";

int NewHaniDisplayMode = HANI_LOGO;


// local function prototypes
void UpdateLCD(void);



#include "dial3.h"

void SelectMenu(int menu)
{ NewHaniDisplayMode = menu; // only once
  // wait until this menu is actually initialized by interrupt driven code
  while(NewHaniDisplayMode != ActLcdMode)delay(10);
  delay(100);
}

void UseFont(const uint8_t* usethisfont)
{ static const uint8_t* activefont = (uint8_t*)-1;
  if(usethisfont != activefont)
  { activefont = usethisfont;
    tft.loadFont(activefont);     
  }
}

// interrupt task for updating display
void UpdateLCDTask(void * pvParameters)
{ while(1)
  { if(bScrollNow)
    { UpdateLCD();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // portTICK_PERIOD_MS = 1 ;-)
  }
}



TFT_eSprite needle = TFT_eSprite(&tft); // Sprite object for volume needle
#define DIAL_CENTRE_X 120
#define DIAL_CENTRE_Y 121
uint16_t* tft_buffer;
void createNeedle(void);
void plotNeedle(int16_t angle, uint16_t ms_delay);


#define NEEDLE_LENGTH 18  // Visible length
#define NEEDLE_WIDTH   7  // Width of needle - make it an odd number
#define NEEDLE_RADIUS 53  // Radius at tip
#define NEEDLE_COLOR1 TFT_BLACK  // Needle periphery colour

//int ShowRadio10mS = 0;
//int ShowArt10mS = 0;
//int Actual45RPMShown = 999;
//int ActualRadioShown = 999;
//int ActualArtShown = 999;


void BuildGdxTable(void);

uint16_t  bg_color = 0;
int CanvasColor; // TFT_RED or something - for use of jpg as canvas set to -1

extern int rotary_loop(int resetvalue);

// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

void SetupMyDisplay(void)
{ 
  
  tft.init();
  tft.fillScreen(TFT_RED);
  tft.setRotation(3); // 3 - using ST7789 320(w) x 240(h) with connector on lefthand side
  tft.setRotation(1); // 3 - using ST7789 320(w) x 240(h) with connector on righthand side
  UseFont(Arialnarrow26);
  //tft.loadFont(Arialnarrow26);
  tft.setAttribute(UTF8_SWITCH, false); 
  
  tft.setTextSize(2);
  tft.setTextWrap(false, false);
  tft.setPivot(DIAL_CENTRE_X, DIAL_CENTRE_Y);
  createNeedle(); // volume dial needle
  // voor de jpeg decoder
  // The byte order can be swapped (set true for TFT_eSPI)
  TJpgDec.setSwapBytes(true);
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);
  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);
  BuildGdxTable(); 

  xTaskCreatePinnedToCore(
                    UpdateLCDTask,   /* Task function. */
                    "UpdateLCDTask",     /* name of task. */
                    5000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    NULL,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */   

   xDisplayMutex = xSemaphoreCreateMutex(); 

  if(xDisplayMutex == NULL) Serial.println("NULL");
  else Serial.println("OK");                      
}

#define TFTNUMOFLINES 6

TFTline MyDisplay[TFTNUMOFLINES] = {{ true, "", false }, { true, "", false }, { true, "", false }, { true, "", false }, { true, "", false }, { true, "", false }};


// need gxAdvance and gdX for all characters for vertical scrolling, but don't want to change the TFT_eSPI library itself - so have to make some references

int TablegdX[256];
int TablegxAdvance[256];

void BuildGdxTable(void)
{ int i = 0;
  int extascii = 0;
//  Serial.println(tft.gFont.gCount);
  for(extascii = 0; extascii<256; extascii++)
  { TablegdX[extascii] = 0;
//    Serial.print("Doe:");Serial.println(extascii);
    for (i = 0; i < tft.gFont.gCount; i++)
    { if (tft.gUnicode[i] == extascii)
      { // Serial.print("i=");Serial.print(i);Serial.print(" extascii=");Serial.println(extascii, HEX);Serial.print(" gdX=");Serial.println(tft.gdX[i]);
        TablegdX[extascii] = tft.gdX[i];
        TablegxAdvance[extascii] = tft.gxAdvance[i];
        break;
      }
    }
  }
  TablegxAdvance[0x20] = tft.gFont.spaceWidth;
  TablegdX[0x20] = 0;
}



void convertToExtAscii(char *target)
{ char *p;
  char *p2;
  p = target;
  p2 = target;
  while (*p)
  { if (*p == 0xc2)
    { p++; *p2++ = *p;
    }
    else if (*p == 0xc3)
    { p++; *p2++ = *p + (0xC0 - 0x80);
    }
    else *p2++ = *p;
    p++;
  }
  *p2 = 0;
}

void TFT_line_color(int line, int textcolor, int backgroundcolor)
{ // use as new global color settings
  //BackGroundColor = backgroundcolor;
  //TextColor = textcolor;  

  // set refresh flags accordingly
  if(MyDisplay[line].textcolor != textcolor)
  { MyDisplay[line].textcolor = textcolor;
    MyDisplay[line].refresh = true;
  }  
  if(MyDisplay[line].backgroundcolor != backgroundcolor)
  { MyDisplay[line].backgroundcolor = backgroundcolor;
    MyDisplay[line].refresh = true;
  }  
}

void TFT_line_blink(int line, bool blink)
{ MyDisplay[line].blink = blink;
  BlinkTimer10mS = 0; // reset the blink timer so blinking starts elegant
  bBlinkDisplay = 0;
}



// buffers for 6 lines of text to display
// deals with lenghts, checks if it fits on display or flags it as horizontal scroll text
// converts UTF-8 up to U+00FF (latin-1 supplement) back to old school extended ascii
void TFT_line_print(int line, const char *content)
{ char *p;
  char extAscii[256];
  int n;
  int done = 0;
  
  if(line>=TFTNUMOFLINES)return;
  while(!done)
  { if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
    { MyDisplay[line].blink = false; 
      // max 255 characters
      strncpy(extAscii, content, 255);
      extAscii[255]=0;
      convertToExtAscii(extAscii);

      if (MyDisplay[line].scroll == false) // non extended content, no " --- " added
      { if (strcmp(MyDisplay[line].content, extAscii) == 0)
        { // nothing new, why bother
          xSemaphoreGive(xDisplayMutex);
          return;
        }
      }

      if (MyDisplay[line].scroll == true)
      { if (strncmp(MyDisplay[line].content, extAscii, MyDisplay[line].length - 5) == 0)
        { // nothing new, why bother
          xSemaphoreGive(xDisplayMutex);
          return;
        }
      }

      strcpy(MyDisplay[line].content, extAscii);
      MyDisplay[line].refresh = true;
      MyDisplay[line].length = strlen(MyDisplay[line - 1].content);
      MyDisplay[line].pixelwidth = tft.textWidth(MyDisplay[line].content); // myTextWidth(MyDisplay[line].content); //

  

//      if (MyDisplay[line].pixelwidth > (320-50)) // does not fit, resort to horizontal scroll of this text
      if (MyDisplay[line].pixelwidth > (320 - (MyDisplay[line].rounded ? 50 : 6)) ) // does not fit, resort to horizontal scroll of this text
      { MyDisplay[line].scroll = true;
        strcat(extAscii, " --- "); // add --- to make it nicer when looping around
        strcpy(MyDisplay[line].content, extAscii);
        MyDisplay[line].length = strlen(MyDisplay[line].content);
        MyDisplay[line].pixelwidth = tft.textWidth(MyDisplay[line].content); // myTextWidth(MyDisplay[line].content); //tft.textWidth(MyDisplay[line].content);
      }
      else
      { MyDisplay[line].scroll = false;
      }

      MyDisplay[line].scrollpos = 0;
      MyDisplay[line].scrolldelay = 200; // delayed start of scolling (not implemented)
      xSemaphoreGive(xDisplayMutex);
    }
    done = true;  
  } 
}

// please uncomment section
// if (gdY[gNum] > gFont.maxAscent)
// in smooth_font.ccp ~line 210

#define TYOFF 7 // y offset for all text lines to position it nicely in background


bool UpdateLCDpotentiometer(int NewShowVolume10mS)
{ static int OldShowVolume10mS = 0;
  static int VolumeShown = -1;
  static int NewVolumeToShow;
  char text[32];
  int tw;

  if (OldShowVolume10mS != NewShowVolume10mS)
  { if (OldShowVolume10mS == 0)
    { // init display
      //tft.fillScreen(TFT_YELLOW);
      uint16_t w = 0, h = 0;
      // dial3 is defined in dial3.h for fast loading
      TJpgDec.drawJpg(0, 0, dial3, sizeof(dial3));
      VolumeShown = -1;
    }
    OldShowVolume10mS = NewShowVolume10mS;
    if (NewShowVolume10mS)
    { // laat zien dan
      NewVolumeToShow = rotary_loop(-1);
      if (VolumeShown != NewVolumeToShow)
      { VolumeShown = NewVolumeToShow;
        plotNeedle( ((NewVolumeToShow * 270) / 100), 0);
        snprintf(text, 30, "VOLUME %ddB", NewVolumeToShow);
//Serial.println(text);
        TFT_line_print(5, text);
        tw = MyDisplay[5].pixelwidth;
        tft.fillRoundRect(  ((240 - tw) / 2) - 15   , (5*40)+2, tw + 30, 32, 16, BackGroundColor); // 12=radius 5 is de helft van 10
        tft.setTextDatum(TC_DATUM);
        tft.drawString(MyDisplay[5].content, 120, (5*40)+TYOFF); // centered around x coordinate 120
      }
    }
    else
    { // LCD weer naar normale weergave
      // en dus refresh display
      MyDisplay[0].refresh = true;
      MyDisplay[1].refresh = true;
      MyDisplay[2].refresh = true;
      MyDisplay[3].refresh = true;
      MyDisplay[4].refresh = true;
      MyDisplay[5].refresh = true;

      return true;
    }
  }
  return false;
}

extern int songcount;
void UpdateLCD(void)
{ int line;
  int ycor;
  char text[32];
  int tw;
  
  static int scrollpos = 0;
  static bool refreshdisplay = false;
  int TextDatum;

  static unsigned long startMillis;
  static unsigned long currentMillis;

  // first print static content on LCD 
  if (NewHaniDisplayMode != ActLcdMode)
  { Serial.println(NewHaniDisplayMode);
    for (line = 0; line < TFTNUMOFLINES; line++)
    { TFT_line_print(line, "");
      MyDisplay[line].rounded = (line>0);  
      MyDisplay[line].refresh = true;  
      MyDisplay[line].lastpixelwidth = -1;  
    }
    switch (NewHaniDisplayMode) // on this mode change, fill the text lines with appropriate data
    { case HANI_LOGO:
        for (line = 0; line < TFTNUMOFLINES; line++)
        { MyDisplay[line].backgroundcolor = TFT_DARKGREY;
          MyDisplay[line].textcolor = TFT_WHITE;  
          MyDisplay[line].canvascolor = -1;  
        }
        TJpgDec.drawJpg(0, 0, flowers, sizeof(flowers));
        MyDisplay[5].rounded = false;
        TFT_line_print(5, Credits);
        break;
      case HANI_SETUP:
        for (line = 0; line < TFTNUMOFLINES; line++)
        { MyDisplay[line].backgroundcolor = TFT_DARKGREY;
          MyDisplay[line].textcolor = TFT_WHITE;  
          MyDisplay[line].canvascolor = TFT_BLACK;  
        }
        CanvasColor = TFT_BLACK; 
        tft.fillScreen(CanvasColor);
        //TFT_line_print(0, "SETUP");
        //TFT_line_print(5, "Choose Parameter & Select It");
        break;
      case HANI_AUTO:
        for (line = 0; line < TFTNUMOFLINES; line++)
        { MyDisplay[line].backgroundcolor = TFT_DARKGREY;
          MyDisplay[line].textcolor = TFT_WHITE;  
          MyDisplay[line].canvascolor = TFT_BLACK;  
        }

        CanvasColor = TFT_BLACK; 
        tft.fillScreen(CanvasColor);
        BackGroundColor = TFT_DARKGREY;
        TextColor = TFT_WHITE;
        TFT_line_print(0, "AUTOMATIC PAUSED");
        break;
      case HANI_HAND:
        for (line = 0; line < TFTNUMOFLINES; line++)
        { MyDisplay[line].backgroundcolor = TFT_DARKGREY;
          MyDisplay[line].textcolor = TFT_WHITE;  
          MyDisplay[line].canvascolor = TFT_BLACK;  
        }
        // MyDisplay[5].rounded = false;
        CanvasColor = TFT_BLACK; 
        tft.fillScreen(CanvasColor);
        BackGroundColor = TFT_DARKGREY;
        TextColor = TFT_WHITE;
        TFT_line_print(0, "MANUAL MODE PAUSED");
        TFT_line_print(3, "gram");
        TFT_line_color(3, TFT_YELLOW, TFT_BLACK);
        TFT_line_color(1, TFT_YELLOW, TFT_BLACK); // Big Weight number
        // TFT_line_print(5, "Choose Parameter & Select It");
        break;
      default: // Mode not yet covered
        CanvasColor = TFT_WHITE; 
        tft.fillScreen(CanvasColor);
        BackGroundColor = TFT_DARKGREY;
        TextColor = TFT_WHITE;
        sprintf(text, " UNDEFINED MODE: %d", NewHaniDisplayMode);
        TFT_line_print(0, text);
        break;        
    }
    refreshdisplay = true;
    ActLcdMode = NewHaniDisplayMode; 
  }

  if (refreshdisplay == true)
  { refreshdisplay = false;

    switch (ActLcdMode)
    { case HANI_LOGO:
        MyDisplay[0].refresh = false; // protect the logo displayed
        // MyDisplay[1].refresh = false;
        MyDisplay[2].refresh = false;
        MyDisplay[3].refresh = false;
        MyDisplay[4].refresh = false;
        //MyDisplay[5].refresh = false; // 
        break;
      case HANI_AUTO:
      case HANI_HAND:
      case HANI_SETUP:
        break;

      default: // Mode not yet covered
        break;
    }
  }

 //Serial.println("Pak hem2"); 
 if(xSemaphoreTake(xDisplayMutex, (TickType_t)10)==pdTRUE)
 {  
 for (line = 0; line < 6; line++)
 { if (MyDisplay[line].refresh)
   { tft.setTextDatum(TC_DATUM); // horizontally centered for text that is not scrolling
     tw = MyDisplay[line].pixelwidth;
     if (tw > (320-40))tw = (320-40);
     tft.setViewport(0, (line * 40), 320, 40, true);
     
     // first wipe out old stuff by reprinting canvas or printing partial jpg
     if (MyDisplay[line].rounded) // not needed for square lines, only for rounded lines
     { if (MyDisplay[line].scroll == false) // not needed for scroll texts as these are full size anyway
       { if(MyDisplay[line].canvascolor >= 0) // fixed color, not jpg canvas 
         {  if(MyDisplay[line].pixelwidth < MyDisplay[line].lastpixelwidth) // restauration canvas is needed
            { tft.fillRoundRect(0, 2, 320, 32, 0, MyDisplay[line].canvascolor); // radius 0 makes it a square
            }
         }
         else
         { // drawJpg respects the viewport, but we have to move the jpg upwards to get the right portion of the jpg printed in viewport
           TJpgDec.drawJpg(0, -(line * 40), flowers, sizeof(flowers));
         }
       }
     }
     
     // now print background box for text, square for line 0 and rounded for other lines
     if (!MyDisplay[line].rounded)
     { if(MyDisplay[line].pixelwidth != MyDisplay[line].lastpixelwidth)
       { //tft.fillRoundRect(0, 0, 320, 34, 0, MyDisplay[line].backgroundcolor); // radius 0 makes it a square
         tft.fillRect(0, 0, 320, 34, MyDisplay[line].backgroundcolor); // radius 0 makes it a square
       }
     }  
     else
     { if(tw>0) // there is something to print
       { if(MyDisplay[line].pixelwidth != MyDisplay[line].lastpixelwidth) // restauration is needed
         { if(MyDisplay[line].canvascolor<0) // jpg as background
           { tft.fillRoundRect(  ((320 - tw) / 2) - 15   ,  2, tw + 30, 32, 16, MyDisplay[line].backgroundcolor); 
           }
           else
           { tft.fillSmoothRoundRect(((320 - tw) / 2) - 15, 2, tw+30, 32, 16, MyDisplay[line].backgroundcolor, MyDisplay[line].canvascolor);
           }
         }
       }
     }
     
     if(tw>0) // there is something to print
     { if (!MyDisplay[line].scroll) // this text does not scroll
       { tft.setTextColor(MyDisplay[line].textcolor, MyDisplay[line].backgroundcolor, true);
         tft.drawString(MyDisplay[line].content, 320/2, TYOFF); // centered around x coordinate 120
         MyDisplay[line].lastpixelwidth = MyDisplay[line].pixelwidth; // so we can check later if this is a shorter or longer text to fine tune canvas restoration
       }
     }  
     MyDisplay[line].refresh = false;
     if(MyDisplay[line].blink == true)
     { BlinkTimer10mS = 0; // reset the blink timer so blinking starts elegant
       bBlinkDisplay = 0;
       MyDisplay[line].blinkold = !bBlinkDisplay; 
     }  
   }
   else if(MyDisplay[line].blink == true)  
   { // Serial.print("line="); Serial.println(line);
     if(MyDisplay[line].blinkold != bBlinkDisplay)
     { MyDisplay[line].blinkold = bBlinkDisplay;
       // Serial.print("BlinkTimer10mS="); Serial.println(BlinkTimer10mS);
       tft.setTextDatum(TC_DATUM); // horizontally centered for text that is not scrolling
       tw = MyDisplay[line].pixelwidth;
       if (tw > (320-40))tw = (320-40);
       tft.setViewport(0, (line * 40), 320, 40, true);
       if(tw>0) 
       { if(!bBlinkDisplay) // print normal
         { tft.setTextColor(MyDisplay[line].textcolor, MyDisplay[line].backgroundcolor, true);
           tft.drawString(MyDisplay[line].content, 320/2, TYOFF); // centered around x coordinate 120
         }
         else // print nothing as part of the blink display
         { if(MyDisplay[line].canvascolor<0) // jpg as background
           { tft.fillRoundRect(  ((320 - tw) / 2) - 15   ,  2, tw + 30, 32, 16, MyDisplay[line].backgroundcolor); 
           }
           else
           { tft.fillSmoothRoundRect(((320 - tw) / 2) - 15, 2, tw+30, 32, 16, MyDisplay[line].backgroundcolor, MyDisplay[line].canvascolor);
           }
         }
       } 
     }
   }
 } 
  
 if(bScrollNow == true) // set true again every 50mS by interrupt
 { bScrollNow = false; 
   for (line = 1; line < 6; line++)
   { if (MyDisplay[line].scroll)
     { startMillis = micros();
       TextDatum = tft.getTextDatum();
       tft.setTextDatum(TL_DATUM);

       if (MyDisplay[line].scrollpos >= MyDisplay[line].pixelwidth)
       { MyDisplay[line].scrollpos = 0;
       }
        
       if (MyDisplay[line].scrollpos==0)
       { MyDisplay[line].nchar = 0;
         MyDisplay[line].toeat = 0;
       }

       int c;
       if(MyDisplay[line].toeat<1)
       { c = MyDisplay[line].content[MyDisplay[line].nchar];
         if(MyDisplay[line].nchar==0) MyDisplay[line].toeat = TablegxAdvance[c] -  TablegdX[c];
         else MyDisplay[line].toeat =   TablegxAdvance[c];   
         MyDisplay[line].noffset = MyDisplay[line].toeat;
         MyDisplay[line].nchar++;
       }
  
       if(bUpdateDisplay) // set true every 100mS for a 10 frames per seconde update
       { tft.setViewport((MyDisplay[line].rounded ? 25 : 3), (line * 40), (320 - (MyDisplay[line].rounded ? 50 : 6)), 40); // bigger viewport for square box
         tft.setTextColor(MyDisplay[line].textcolor, MyDisplay[line].backgroundcolor, true);
         tft.drawString(&MyDisplay[line].content[MyDisplay[line].nchar-1], MyDisplay[line].toeat - MyDisplay[line].noffset, TYOFF);
         if((MyDisplay[line].pixelwidth - MyDisplay[line].scrollpos) < (320-25))
         { tft.drawString(MyDisplay[line].content, MyDisplay[line].pixelwidth - MyDisplay[line].scrollpos, TYOFF);
         }
       }
  

       MyDisplay[line].toeat -=1;
       MyDisplay[line].scrollpos++;

       // restore TextDatum
       tft.setTextDatum(TextDatum);
     }
   }
   bUpdateDisplay = false;
   tft.setViewport(0, 0, 320, 240, true);
  }
  //Serial.println("En los2");

  if(ActLcdMode == HANI_HAND)
  { if(OldWeight != NewWeight)
    { OldWeight = NewWeight;
      UseFont(Arialbd72);
      // print it 
      sprintf(text, "  %d  ", NewWeight);
      tft.setTextDatum(TC_DATUM);
      tft.setTextColor(MyDisplay[1].textcolor, MyDisplay[1].canvascolor, true);
      tft.drawString(text, 320/2, 60);
      UseFont(Arialnarrow26);
    }  
  }
  xSemaphoreGive(xDisplayMutex);  
 }
}



void RemoveHtmlEntities(char* target)
{ /* This mapping table can be extended if necessary. */
  static const struct {
    const char* encodedEntity;
    const char decodedChar;
  } entityToChars[] = {
    {"&lt;", '<'},
    {"&gt;", '>'},
    {"&amp;", '&'},
    {"&quot;", '"'},
    {"&apos;", '\''},
    {"&#039;", '\''},
  };

  int n = 0;
  int n1 = 0;
  int cnt;

  cnt = strlen(target);
  for (n = 0; n < cnt; n++)
  { for (n1 = 0; n1 < 5; n1++)
    { if (strncmp(&target[n], entityToChars[n1].encodedEntity, strlen(entityToChars[n1].encodedEntity)) == 0)
      { strcpy(&target[n + 1], &target[n + strlen(entityToChars[n1].encodedEntity)]);
        target[n] = entityToChars[n1].decodedChar;
        n--;
        break;
      }
    }
  }

}




// =======================================================================================
// Create the needle Sprite
// =======================================================================================
void createNeedle(void)
{
  needle.setColorDepth(16);
  needle.createSprite(NEEDLE_WIDTH, NEEDLE_LENGTH);  // create the needle Sprite

  needle.fillSprite(TFT_BLACK); // Fill with black

  // Define needle pivot point relative to top left corner of Sprite
  uint16_t piv_x = NEEDLE_WIDTH / 2; // pivot x in Sprite (middle)
  uint16_t piv_y = NEEDLE_RADIUS;    // pivot y in Sprite
  needle.setPivot(piv_x, piv_y);     // Set pivot point in this Sprite

  // Draw the red needle in the Sprite
  needle.fillRect(0, 0, NEEDLE_WIDTH, NEEDLE_LENGTH, TFT_DARKGREY);
  needle.fillRect(1, 1, NEEDLE_WIDTH - 2, NEEDLE_LENGTH - 2, TFT_BLACK);

  // Bounding box parameters to be populated
  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;

  // Work out the worst case area that must be grabbed from the TFT,
  // this is at a 45 degree rotation
  needle.getRotatedBounds(45, &min_x, &min_y, &max_x, &max_y);

  // Calculate the size and allocate the buffer for the grabbed TFT area
  tft_buffer =  (uint16_t*) malloc( ((max_x - min_x) + 2) * ((max_y - min_y) + 2) * 2 );
}

// =======================================================================================
// Move the needle to a new position
// =======================================================================================

extern int ActualVolumeFromSonos;
void plotNeedle(int16_t angle, uint16_t ms_delay)
{ static bool buffer_loaded = false;
  static int16_t old_angle = -135; // Starts at -120 degrees

  // Bounding box parameters
  static int16_t min_x;
  static int16_t min_y;
  static int16_t max_x;
  static int16_t max_y;

  if (angle == 1234) // magic value
  { old_angle = ((ActualVolumeFromSonos * 270) / 100) - 135; // initial display should show needle at right position right away
    return;
  }

  if (angle < 0) angle = 0; // Limit angle to emulate needle end stops
  if (angle > 270) angle = 270;

  angle -= 135; // Starts at -120 degrees

  // Move the needle until new angle reached
  while (angle != old_angle || !buffer_loaded) {

    #ifndef ST7789_DRIVER
    if (old_angle < angle) old_angle++;
    else old_angle--;
    #else
    old_angle = angle;
    #endif
    // Only plot needle at even values and final position to improve plotting performance
    if (((old_angle & 1) == 0) || (old_angle==angle))
    { // fb - note to self - corruption seen in this buffer - very rare, not yet figured out
      if (buffer_loaded) {
        // Paste back the original needle free image area
        #ifndef ST7789_DRIVER
        tft.pushRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
        #else
//        tft.setViewport(50, min_y, 100, 1 + max_y - min_y);
//        TJpgDec.drawJpg(-50, -min_y, dial3, sizeof(dial3));
        tft.setViewport(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y);
        TJpgDec.drawJpg(-min_x, -min_y, dial3, sizeof(dial3));

        tft.setViewport(0, 0, 240, 240);
        #endif
        buffer_loaded = false; 
      }

      if ( needle.getRotatedBounds(old_angle, &min_x, &min_y, &max_x, &max_y) )
      {
        // Grab a copy of the area before needle is drawn
        #ifndef ST7789_DRIVER
        tft.readRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
        #endif
        buffer_loaded = true;
      }

      // Draw the needle in the new position, black in needle image is transparent
      needle.pushRotated(old_angle, TFT_VIOLET);

      // Wait before next update
      delay(ms_delay);
    }

    // Slow needle down slightly as it approaches the new position
    if (abs(old_angle - angle) < 10) ms_delay += ms_delay / 5;
  }
}
