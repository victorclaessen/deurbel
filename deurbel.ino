/*
 * requires jled and inputdebounce libaries, both can be installed from the library manager
 * (C) 2021 Victor Claessen
 */

#include "InputDebounce.h"
#define BUTTON_DEBOUNCE_DELAY   20   // [ms]
#include <jled.h>

static const int pinLED = LED_BUILTIN; // 13
static const int pinHardIn = 2;
static const int pinSoftIn = 3;
static const int pinTestIn = 4;
static const int pinInhibit = 5;
static const int pinTrgOut = 6;
static const int pinBelOut = 7;
static const int pinSoftOut = 8;

static const int belOnTime = 1000;
static const int belOffTime = 3000;
static const int belOffTimeSftRq = 100;   // off time when turned on by soft request
static const int softOnTime = 100;
static const int softOffTime = 100;
static const int blinkOnTime = 50;
static const int blinkOffTime = 50;

bool inhibited = false;

JLed led = JLed(pinLED).Off();
JLed trg = JLed(pinTrgOut).LowActive().Off();
JLed bel = JLed(pinBelOut).LowActive().Off();
JLed sft = JLed(pinSoftOut).LowActive().Off();

static InputDebounce buttonHardknop; // not enabled yet, setup has to be called first, see setup() below
static InputDebounce buttonSoftknop;
static InputDebounce buttonTestknop;
static InputDebounce buttonInhibit;

void buttonTest_pressedCallback(uint8_t pinIn)
{
  // handle pressed state
  switch(pinIn) {
    case pinInhibit:
      inhibited = true;
      Serial.println("Inhibit active");
      break;
    case pinHardIn:
      trg.On();                                         // trigger follows
      if(inhibited){
        led.Blink(blinkOnTime, blinkOffTime).Forever(); // led blinks
        // no ringing of the bel
        if (!sft.IsRunning()) {                         // notify soft output
          sft.Blink(softOnTime, softOffTime);        
        }
      } else {
        led.On();                                       // led on
        if (!bel.IsRunning()) {                         // ring bel if not already ringing/waiting
          bel.Blink(belOnTime, belOffTime);
        }
        if (!sft.IsRunning()) {                         // notify soft output
          sft.Blink(softOnTime, softOffTime);
        }
      }
      Serial.print("HardIn handled ");
      Serial.println(inhibited ? "[INHIBIT ACTIVE]":"");
      break;
    case pinSoftIn:
      trg.On();                                         // trigger follows
      if(inhibited){
        led.Blink(blinkOnTime, blinkOffTime).Forever(); // led blinks
        if (!bel.IsRunning()) {                         // ring bel if not already ringing/waiting
          bel.Blink(belOnTime, belOffTime);
        }
        // no notification to soft output
      } else {
        led.On();                                       // led on
        if (!bel.IsRunning()) {                         // ring bel if not already ringing/waiting
          bel.Blink(belOnTime, belOffTimeSftRq);        // notice special off time for soft request
        }
        if (!sft.IsRunning()) {                         // notify soft output
          sft.Blink(softOnTime, softOffTime);
        }
      }
      Serial.print("SoftIn handled ");
      Serial.println(inhibited ? "[INHIBIT ACTIVE]":"");   
      break;
    case pinTestIn:
      trg.On();                                         // trigger follows
      // no ringing of the bel
      // no notification to soft output
      if(inhibited){
        led.Blink(blinkOnTime, blinkOffTime).Forever(); // led blinks
      } else {
        led.On();                                       // led on
      }
      Serial.print("TestIn handled ");
      Serial.println(inhibited ? "[INHIBIT ACTIVE]":""); 
      break;
   }
}

void buttonTest_releasedCallback(uint8_t pinIn)
{
  // handle released state
  switch(pinIn) {
    case pinInhibit:
      inhibited = false;
      bel.Off();                                        // set ready for immediate triggering
      sft.Off();                                        // set ready for immediate triggering
      Serial.println("Inhibit inactive");
      break;
    case pinHardIn:
      trg.Off();                                        // trigger follows
      led.Off();                                        // led off
      break;
    case pinSoftIn:
      trg.Off();                                        // trigger follows
      led.Off();                                        // led off
      break;
    case pinTestIn:
      trg.Off();                                        // trigger follows
      led.Off();                                        // led off
      break;
   }
}

void setup()
{
  pinMode(pinHardIn,  INPUT_PULLUP);
  pinMode(pinSoftIn,  INPUT_PULLUP);
  pinMode(pinTestIn,  INPUT_PULLUP);
  pinMode(pinInhibit, INPUT_PULLUP);
  pinMode(pinTrgOut,  OUTPUT);
  pinMode(pinBelOut,  OUTPUT);
  pinMode(pinSoftOut, OUTPUT);
  digitalWrite(pinBelOut,  HIGH);
  digitalWrite(pinTrgOut,  HIGH);
  digitalWrite(pinSoftOut, HIGH);  

  // init serial
  Serial.begin(9600);
  Serial.println("HuizeZeezicht deurbel");
  
  // register callback functions (shared, used by all buttons)
  buttonHardknop.registerCallbacks(buttonTest_pressedCallback, buttonTest_releasedCallback);
  buttonSoftknop.registerCallbacks(buttonTest_pressedCallback, buttonTest_releasedCallback);
  buttonTestknop.registerCallbacks(buttonTest_pressedCallback, buttonTest_releasedCallback);
  buttonInhibit.registerCallbacks(buttonTest_pressedCallback, buttonTest_releasedCallback);
  
  // setup input buttons (debounced)
  buttonHardknop.setup(pinHardIn, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  buttonSoftknop.setup(pinSoftIn, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  buttonTestknop.setup(pinTestIn, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  buttonInhibit.setup(pinInhibit, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
}

void loop()
{
  unsigned long now = millis();
  
  // poll button state
  buttonHardknop.process(now); // callbacks called in context of this function
  buttonSoftknop.process(now);
  buttonTestknop.process(now);
  buttonInhibit.process(now);
  led.Update();
  trg.Update();
  bel.Update();
  sft.Update();
}
