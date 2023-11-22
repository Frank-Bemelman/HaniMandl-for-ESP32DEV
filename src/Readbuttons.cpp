#include <arduino.h>
//#include <AiEsp32RotaryEncoder.h>
// functions to read and debounce keys
#include "hani.h"
#include <HX711_ADC.h>


#define DEBOUNCE 3 // in 10mS units

// volatile, so compiler retrieves value every time
volatile bool deb_start_button = false;
volatile bool deb_stop_button = false;
volatile bool deb_encoder_button = false;
volatile bool deb_setup_switch = false;
volatile bool deb_auto_switch = false;
volatile bool deb_manual_switch = false;

volatile bool bStartButtonPulsed = false;
volatile bool bStopButtonPulsed = false;
volatile bool bEncoderButtonPulsed = false;
volatile bool bSetupSwitchPulsed = false;
volatile bool bAutoSwitchPulsed = false;
volatile bool bManualSwitchPulsed = false;


int start_button_f = 1234;
int stop_button_f = 1234;
int encoder_button_f = 1234;
int setup_switch_f = 1234;
int auto_switch_f = 1234;
int manual_switch_f = 1234;

int start_button_very_long_pressed = 0;
int stop_button_very_long_pressed = 0;
int encoder_button_very_long_pressed = 0;
int setup_switch_very_long_pressed = 0;
int auto_switch_very_long_pressed = 0;
int manual_switch_very_long_pressed = 0;

extern rotary rotaries[]; // will be initialized in setup()

int GramsOnScale; // actual weight on scale, maintained in interrupt

bool bScaleStable; // flag, true = stable, false is unstable
extern HX711_ADC LoadCell;


//int rotary_loop(int resetvalue);

//AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(
//    ROTARY_ENCODER_A_PIN, 
//    ROTARY_ENCODER_B_PIN, 
//    ROTARY_ENCODER_BUTTON_PIN, 
//    ROTARY_ENCODER_VCC_PIN,
//    ROTARY_ENCODER_STEPS
//    );

//void IRAM_ATTR readEncoderISR()
//{ rotaryEncoder.readEncoder_ISR();
//}    


void SetupButtons(void)
{ pinMode(BUTTON_START, INPUT_PULLDOWN);
  pinMode(BUTTON_STOP, INPUT_PULLDOWN);
  pinMode(SWITCH_AUTO, INPUT_PULLDOWN);
  pinMode(SWITCH_SETUP, INPUT_PULLDOWN);
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  

    
//  pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);
//  pinMode(ROTARY_ENCODER_A_PIN, INPUT);  
//  pinMode(ROTARY_ENCODER_B_PIN, INPUT);
//  pinMode(CANCEL_BUTTON, INPUT);  // dit is de grote witte knop links, normally closed - zit op D35 en heeft een externe 10K pullup
//  pinMode(BLUE_BUTTON, INPUT_PULLUP);  // dit is de blauwe knop rechts, normally open
//  pinMode(ENCODER_BUTTON, INPUT_PULLUP);  // dit is de volume drukknop links, normally open
//  rotaryEncoder.begin();
//  rotaryEncoder.setup(readEncoderISR); 
//  rotaryEncoder.setBoundaries(0,100,false); // with end stop
//  rotaryEncoder.setAcceleration(0); // at 25 already questionable behaviour
}  

bool IsPulsed(bool *button)
{ if(*button)
  { *button = false;
    return true;
  }
  return false;
}

// task to read the keys at a regular interval
void ReadButtons(void * pvParameters)
{ bool act_start_button;
  bool act_stop_button;
  bool act_setup_switch;
  bool act_auto_switch;
  bool act_manual_switch;
  int act_encoder_button;

  int start_button_changed =0;
  int stop_button_changed =0;;
  int setup_switch_changed =0;;
  int auto_switch_changed =0;;
  int manual_switch_changed =0;;
  int encoder_button_changed = 0;

  int n;
  int readcounter;  
  int oldgramsonscale;
  int scalestable;
   
  while(1)
  { readcounter++;
    // shift register for rotary value, shift one place
    // this is to preventing the encoder knob to register an accidental last moment change of the encoder
    for(n=0;n<3;n++)
    { rotaries[n].Value[0] = rotaries[n].Value[1];
      rotaries[n].Value[1] = rotaries[n].Value[2];
    }
    
    // read scale every 100mS
    if((readcounter % 10) == 0)
    { GramsOnScale = (int)(LoadCell.getData()+0.5);
      if(abs(oldgramsonscale - GramsOnScale)>2)scalestable=5; // 5x100mS or half a second of stable readings needed
      else if(scalestable)scalestable--;
      bScaleStable = (scalestable==0); // set flag true if scale has settled
      oldgramsonscale = GramsOnScale;
    }
    
    act_start_button = digitalRead(BUTTON_START); 
    act_stop_button = digitalRead(BUTTON_STOP); 
    act_encoder_button = !digitalRead(ENCODER_BUTTON); // normally high, open switch with pullup
    act_setup_switch = digitalRead(SWITCH_SETUP); // high when selected
    act_auto_switch = digitalRead(SWITCH_AUTO); // high when selected
    act_manual_switch = !(act_setup_switch || act_auto_switch);

    // debounce start button
    if(act_start_button != deb_start_button)
    { if(start_button_changed<DEBOUNCE)start_button_changed++;
      else
      { deb_start_button = act_start_button;
        start_button_f = deb_start_button;
        start_button_changed = 0;
        if(deb_start_button)bStartButtonPulsed = true;
      } 
    }
    else start_button_changed = 0;

    if(deb_start_button==true) // pressed?
    { start_button_very_long_pressed++; // use that to activate portal wifi manager
    }
    else
    { start_button_very_long_pressed = 0;
    }

    // debounce stop button
    if(act_stop_button != deb_stop_button)
    { if(stop_button_changed<DEBOUNCE)stop_button_changed++;
      else
      { deb_stop_button = act_stop_button;
        stop_button_f = deb_stop_button;
        stop_button_changed = 0;
        if(deb_stop_button)bStopButtonPulsed = true;
      } 
    }
    else stop_button_changed = 0;

    if(deb_stop_button==true) // pressed?
    { stop_button_very_long_pressed++; // use that to activate portal wifi manager
    }
    else
    { stop_button_very_long_pressed = 0;
    }

    // debounce encoder button
    if(act_encoder_button != deb_encoder_button)
    { if(encoder_button_changed<DEBOUNCE)encoder_button_changed++;
      else
      { deb_encoder_button = act_encoder_button;
        encoder_button_f = deb_encoder_button;
        encoder_button_changed = 0;
        if(deb_encoder_button)bEncoderButtonPulsed = true;
      } 
    }
    else encoder_button_changed = 0;

    if(deb_encoder_button==false) // pressed?
    { encoder_button_very_long_pressed++; // use that to activate portal wifi manager
    }
    else
    { encoder_button_very_long_pressed = 0;
    }



    // debounce manual switch
    if(act_manual_switch != deb_manual_switch)
    { if(manual_switch_changed<DEBOUNCE)manual_switch_changed++;
      else
      { deb_manual_switch = act_manual_switch;
        manual_switch_f = deb_manual_switch;
        manual_switch_changed = 0;
        if(deb_manual_switch)bManualSwitchPulsed = true;
      } 
    }
    else manual_switch_changed = 0;

    if(deb_manual_switch==true) // pressed?
    { manual_switch_very_long_pressed++; // use that to activate portal wifi manager
    }
    else
    { manual_switch_very_long_pressed = 0;
    }

    // debounce setup switch
    if(act_setup_switch != deb_setup_switch)
    { if(setup_switch_changed<DEBOUNCE)setup_switch_changed++;
      else
      { deb_setup_switch = act_setup_switch;
        setup_switch_f = deb_setup_switch;
        setup_switch_changed = 0;
        if(deb_setup_switch)bSetupSwitchPulsed = true;
      } 
    }
    else setup_switch_changed = 0;

    if(deb_setup_switch==true) // pressed?
    { setup_switch_very_long_pressed++; // use that to activate portal wifi manager
    }
    else
    { setup_switch_very_long_pressed = 0;
    }

    // debounce auto switch
    if(act_auto_switch != deb_auto_switch)
    { if(auto_switch_changed<DEBOUNCE)auto_switch_changed++;
      else
      { deb_auto_switch = act_auto_switch;
        auto_switch_f = deb_auto_switch;
        auto_switch_changed = 0;
        if(deb_auto_switch)bAutoSwitchPulsed = true;


      } 
    }
    else auto_switch_changed = 0;

    if(deb_auto_switch==true) // pressed?
    { auto_switch_very_long_pressed++; // use that to activate portal wifi manager
    }
    else
    { auto_switch_very_long_pressed = 0;
    }



    vTaskDelay(10 / portTICK_PERIOD_MS); // 10 msek
  } 
}






// end of file
