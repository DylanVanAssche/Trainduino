/*
 ******************************************
 *               Trainduino               *
 *                                        *
 *--------> RANDOMLEDCONTROLLER <---------*
 *                                        *
 *              V 1.3 BETA                *
 ******************************************
 THIS IS A BETA! I'm never resposibly for any damage to your stuff! This version is NOT completely tested.
 
 FEATURES:
 ---------
 
 The RandomLEDController gives you the oppertunity to create a lively city on your modelrailroad! 
 It switchs automaticaly the streetlights and the lights in the buildings ON/OFF.
 The streetlights will be activated in the night mode (RED LED), they will be OFF in the day mode (GREEN LED).
 The lights in the buildings go on randomly in the day mode. When it's night they will be all OFF.

 [NEW] You can control the RandomLEDController with a DCC address. 
 
       ADDRESS:
         Force day mode: 301
         Force evening mode: 302
         Force night mode: 303
         Force morning mode: 304
         Automatic day/night: 305
       
 [NEW] Improved random LEDs in the buildings.
       You can define the maximum number of outputs that can be activated at the same time in every phase of the cycle.
       I wrote here my values down:
       
       HOUSES:
         DayMaximumOutput: 4
         EveningMaximumOutput: 11
         NightMaximumOutput: 2
         MorningMaximumOutput: 9
       
       OFFICES (WORK):
         DayMaximumOutput: 10
         EveningMaximumOutput: 4
         NightMaximumOutput: 0
         MorningMaximumOutput: 4
 
 [NEW] Improved day/night system.
       I added a 'morning' and 'evening' phase in the cycle to get a more realistic image of a real day.
 
 (c) Dylan Van Assche (2014), producer of the Trainduino serie.
 
 GROUP DEFINITION:
 -----------------
 
 0    =  DCC Locdecoders
 100  =  DCC Switch Controllers
 200  =  DCC Scenery Controllers
 -> 300  =  DCC Special Controllers <-
 
 RandomLEDController:
 GROUP:  300
 Number: 001
 Adres:  301 - 305
 */
 
#include <SoftPWM_timer.h>
#include <SoftPWM.h>
#include <TimedAction.h>
#include <NmraDcc.h>
#define STATUS A3

// Objects & libraries
  // NmraDCC
  NmraDcc  Dcc;
  DCC_MSG  Packet;
  struct CVPair
  {
    uint16_t  CV;
    uint8_t   Value;
  };
  
  CVPair FactoryDefaultCVs [] =
  {
    {
      CV_ACCESSORY_DECODER_ADDRESS_LSB, 1      }
    ,
    {
      CV_ACCESSORY_DECODER_ADDRESS_MSB, 0      }
    ,
  };

  uint8_t FactoryDefaultCVIndex = 0;

  // TimedAction
  TimedAction DayON    = TimedAction(random(6000, 9000), DayONFunction); // Define every timer for this sketch.
  TimedAction DayOFF   = TimedAction(random(9000, 11000), DayOFFFunction);
  TimedAction NightOFF = TimedAction(random(8000, 10000), NightOFFFunction);
  TimedAction NightON  = TimedAction(random(18000, 20000), NightONFunction);
  TimedAction Clock    = TimedAction(1000, ClockFunction);

// Booleans
boolean StatusDayNight = true; // Automatic mode is by default ACTIVATED!

// Integers
int PinChoose      = 1;
int currentHour    = 5; // Start ALWAYS the program in morning mode (6:00).
int currentMinutes = 59;
int NumberOfLightsOn = 0; // This will count the number of outputs that are activated during the loop.
int Mode = 0;

// Constants
const int DccAckPin = A2;
const int DayMaximumOutput = 4; // Change here the maximum number of outputs that can be activated at the same time.
const int EveningMaximumOutput = 11;
const int NightMaximumOutput = 2;
const int MorningMaximumOutput = 9;

void setup()
{
  // Setup for the outputs.
  SoftPWMBegin();
  for (int i = 2; i < 11; i++) 
  {
    SoftPWMSet(i, 0);
    SoftPWMSetFadeTime(i, 1000, 1000);
  }

  for (int i = 14; i < 17; i++)
  {
    SoftPWMSet(i, 0);
    SoftPWMSetFadeTime(i, 1000, 1000);
  }

  for (int i = 18; i < 21; i++)
  {
    SoftPWMSet(i, 0);
    SoftPWMSetFadeTime(i, 1000, 1000);
  }

  // Status LED on A3...
  pinMode(STATUS,OUTPUT);

  // Setup DCC
  pinMode( DccAckPin, OUTPUT );
  Dcc.pin(0, 3, 1); // INT 1 on pin 3
  Dcc.init( MAN_ID_DIY, 10, FLAGS_OUTPUT_ADDRESS_MODE | FLAGS_DCC_ACCESSORY_DECODER, 0 );
}

void loop()
{
   // Time tracking process.
  Clock.check();

  // Automatic mode
  if(StatusDayNight == true)
  {
    switch(currentHour)
    {
    case 6: // Activate morning mode on 6:00. People get up and make themself ready for the day.
      Mode = 4;
      break; 

    case 9: // Activate day mode on 9:00. People go to there work so there aren't a lot people at home.
      Mode = 1;
      break;

    case 17: // Activate evening mode on 17:00. People come home from there work so they turn the ligths ON.
      Mode = 2;
      break;

    case 22: // Activate deep night mode on 22;00. People go to there bed, they turn the lights OFF.
      Mode = 3;
      break;     
    }   
  }

  // Controls the DAY/NIGHT process.
  switch(Mode)
  {    
  case 1: // Day mode
    if(NumberOfLightsOn <= DayMaximumOutput) // Turn maximum 4 outputs of the 14 ON
    {
      DayON.check();
    }
    else
    {
      DayOFF.check();
    }
    SoftPWMSetPercent(2, 0);  // Deactivate the streetlights.
    digitalWrite(STATUS,LOW);
    break;

  case 2: // Evening mode
    if(NumberOfLightsOn <= EveningMaximumOutput) // Turn maximum 11 outputs of the 14 ON.
    {
      DayON.check();
    }
    else
    {
      DayOFF.check();
    }
    SoftPWMSetPercent(2, 100);  // Activate the streetlights.
    digitalWrite(STATUS,HIGH);
    break;

  case 3: // Deep night mode
    if(NumberOfLightsOn <= NightMaximumOutput) // Turn maximum 2 outputs of the 14 ON.
    {
      NightON.check();
    }
    else
    {
      NightOFF.check();
    }
    SoftPWMSetPercent(2, 100);  // Activate the streetlights.
    digitalWrite(STATUS,LOW);
    break;

  case 4: // Morning mode
    if(NumberOfLightsOn <= MorningMaximumOutput) // Turn maximum 11 outputs of the 14 ON.
    {
      DayON.check();
    }
    else
    {
      DayOFF.check();
    }
    SoftPWMSetPercent(2, 0);  // Deactivate the streetlights.
    digitalWrite(STATUS,HIGH);
    break;
  }

  // DCC process loop
  Dcc.process();
  if ( FactoryDefaultCVIndex && Dcc.isSetCVReady())
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
  }
}

void DayONFunction() // It's day, turn one output ON and another OFF.
{
  switch(PinChoose)
  { 
  case 1:
    SoftPWMSetPercent(random(3, 11), random(60,100));
    PinChoose++;
    NumberOfLightsOn++;
    break;

  case 2:
    SoftPWMSetPercent(random(14, 17), random(60,100));
    PinChoose++;
    NumberOfLightsOn++;
    break;

  case 3:
    SoftPWMSetPercent(random(18, 21), random(60,100));
    PinChoose = 1;
    NumberOfLightsOn++;
    break;

  default:
    PinChoose = 1;
    break;
  }
}

void DayOFFFunction()
{
  switch(PinChoose)
  { 
  case 1:
    SoftPWMSetPercent(random(3, 11), 0);
    PinChoose++;
    NumberOfLightsOn--;
    break;

  case 2:
    SoftPWMSetPercent(random(14, 17), 0);
    PinChoose++;
    NumberOfLightsOn--;
    break;

  case 3:
    SoftPWMSetPercent(random(18, 21), 0);
    PinChoose = 1;
    NumberOfLightsOn--;
    break;

  default:
    PinChoose = 1;
    break;
  }
}

void NightOFFFunction() // Turn most of the outputs OFF! It's night, most of the people sleeps at this moment.
{
  switch(PinChoose)
  { 
  case 1:
    SoftPWMSetPercent(random(3, 11), 0);
    PinChoose++;
    NumberOfLightsOn--;
    break;

  case 2:
    SoftPWMSetPercent(random(14, 17), 0);
    PinChoose++;
    NumberOfLightsOn--;
    break;

  case 3:
    SoftPWMSetPercent(random(18, 21), 0);
    PinChoose = 1;
    NumberOfLightsOn--;
    break;

  default:
    PinChoose = 1;
    break;
  }
}

void NightONFunction()
{
  switch(PinChoose)
  { 
  case 1:
    SoftPWMSetPercent(random(3, 11), 0);
    PinChoose++;
    NumberOfLightsOn++;
    break;

  case 2:
    SoftPWMSetPercent(random(14, 17), 0);
    PinChoose++;
    NumberOfLightsOn++;
    break;

  case 3:
    SoftPWMSetPercent(random(18, 21), 0);
    PinChoose = 1;
    NumberOfLightsOn++;
    break;

  default:
    PinChoose = 1;
    break;
  }
}

void ClockFunction()
{
  currentMinutes++;

  if (currentMinutes == 60)
  {
    currentHour++;
    currentMinutes = 0;
  }

  if (currentHour == 24)
  {
    currentHour = 0;
  }   
}

void notifyCVAck(void) // NmraDCC ACK part
{
  digitalWrite( DccAckPin, HIGH );
  delay( 6 );
  digitalWrite( DccAckPin, LOW );
}

void notifyCVResetFactoryDefault()
{
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
};

// This function is called whenever a normal DCC Turnout Packet is received
void notifyDccAccState( uint16_t Addr, uint16_t BoardAddr, uint8_t OutputAddr, uint8_t State)
{
  switch (Addr)
  {
  case 301: // Force day mode
    Mode = 1;
    StatusDayNight = false;
    break;

  case 302: // Force evening mode
    Mode = 2;
    StatusDayNight = false;
    break;

  case 303: // Force deepnight mode
    Mode = 3;
    StatusDayNight = false;
    break;

  case 304: // Force morning mode
    Mode = 4;
    StatusDayNight = false;
    break;

  case 305: // Automatic day/night
    // Switch smooth and in the correct order from manual mode to automatic mode.
    switch(Mode)
    { 
      case 1: // day mode
        currentHour = 9;
        break;
        
      case 2: // evening mode
        currentHour = 17;
        break;
        
      case 3: // deep night mode
        currentHour = 22;
        break;
        
      case 4: // morning mode
        currentHour = 6;
        break;
    }
    // Activate automatic mode
    StatusDayNight = true;
  }
}
