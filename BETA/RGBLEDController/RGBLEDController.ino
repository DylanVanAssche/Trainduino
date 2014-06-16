/*
   ******************************************
   *               Trainduino               *
   *                                        *
   *----------> RGBLEDCONTROLLER <----------*
   *                                        *
   *               V 1.2 BETA               *
   ******************************************
   THIS IS A BETA! I'm never resposibly for any damage to your stuff! This version is NOT completely tested.

    
 FEATURES:
 ---------
 
 The RGBLEDController gives you the oppertunity to create a lively city on your modelrailroad! 
 It controls a RGB ledstrip and a normal ledstrip to simulate a whole day/night on your modelrailroad.
 You can change the values of the colours as you like of course in the sketch.

 [NEW] You can control the RGBLEDController with a DCC address. 
 
       ADDRESS:
         Force day mode: 301
         Force evening mode: 302
         Force night mode: 303
         Force morning mode: 304
         Automatic day/night: 305
 
 [NEW] Improved day/night system.
       I added a 'morning' and 'evening' phase in the cycle to get a more realistic image of a real day.
       
 [BUGFIX] PWM timer problem on the WHITE_PIN fixed with SoftPWM.
 
 (c) Dylan Van Assche (2014), producer of the Trainduino serie.
 
 GROUP DEFINITION:
 -----------------
 
 0    =  DCC Locdecoders
 100  =  DCC Switch Controllers
 200  =  DCC Scenery Controllers
 -> 300  =  DCC Special Controllers <-
 
 RGBLEDController:
 GROUP:  300
 Number: 001
 Adres:  301 - 305

*/

#include <TimedAction.h>
#include <NmraDcc.h>
#include <SoftPWM_timer.h>
#include <SoftPWM.h>

#define RED_PIN 9
#define GREEN_PIN 6
#define BLUE_PIN 5
#define WHITE_PIN 3
#define STATUS 15

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
  TimedAction RGBTimer      =     TimedAction(500, RGB);
  TimedAction Clock         =     TimedAction(1000, ClockFunction);

// Integers
int red                   =     180;
int green                 =     90;
int blue                  =     30;
int white                 =     200;
int redValue              =     255;
int greenValue            =     255;
int blueValue             =     255;
int whiteValue            =     255;
int currentHour           =     5; // Start ALWAYS the program in morning mode (6:00).
int currentMinutes        =     59;
int Mode = 0;

//Booleans
boolean StatusDayNight    =     true; // Automatic mode is by default ACTIVATED!

// Constants
const int DccAckPin = A2;

void setup() {
  
  // Setup the outputs
  SoftPWMBegin();  
  SoftPWMSet(RED_PIN, red);
  SoftPWMSet(BLUE_PIN, blue);
  SoftPWMSet(GREEN_PIN, green);
  SoftPWMSet(WHITE_PIN, white);
  SoftPWMSetFadeTime(RED_PIN, 4000, 4000);
  SoftPWMSetFadeTime(BLUE_PIN, 4000, 4000);
  SoftPWMSetFadeTime(GREEN_PIN, 4000, 4000);
  SoftPWMSetFadeTime(WHITE_PIN, 4000, 4000);
  pinMode(STATUS,OUTPUT);
  
  // RandomSeed on A0 to improve the random() function
  randomSeed(A0);
  
  // Setup DCC
  pinMode( DccAckPin, OUTPUT );
  Dcc.pin(0, 3, 1); // INT 1 on pin 3
  Dcc.init( MAN_ID_DIY, 10, FLAGS_OUTPUT_ADDRESS_MODE | FLAGS_DCC_ACCESSORY_DECODER, 0 );
Serial.begin(9600);
}

void loop() {

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
    redValue    =  255;
    greenValue  =  150;
    blueValue   =  100;
    whiteValue  =  255;
    RGBTimer.check(); // Every time the RGBTimer is triggered, changes the PWM output. With this timer you can change the colors of the RGB LEDstrip very slowly.
    break;

  case 2: // Evening mode
    redValue    =  180;
    greenValue  =  90;
    blueValue   =  30;
    whiteValue  =  200;
    RGBTimer.check(); // Every time the RGBTimer is triggered, changes the PWM output. With this timer you can change the colors of the RGB LEDstrip very slowly.
    break;

  case 3: // Deep night mode
    redValue    =  2;
    greenValue  =  2;
    blueValue   =  50;
    whiteValue  =  0;
    RGBTimer.check(); // Every time the RGBTimer is triggered, changes the PWM output. With this timer you can change the colors of the RGB LEDstrip very slowly.
    break;

  case 4: // Morning mode
    redValue    =  180;
    greenValue  =  90;
    blueValue   =  30;
    whiteValue  =  200;
    RGBTimer.check(); // Every time the RGBTimer is triggered, changes the PWM output. With this timer you can change the colors of the RGB LEDstrip very slowly.
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

void RGB() // This function controls the RGB LEDstrip.
{
  if(red == redValue && green == greenValue && blue == blueValue && white == whiteValue)
  {
   digitalWrite(STATUS, LOW); 
  }
  else
  {
   digitalWrite(STATUS, HIGH); 
  }
   if(red == redValue)
  {
  }
  else
  {
  if (red < redValue)
  {
    red++;
    SoftPWMSet(RED_PIN, red);
  }
  else
  {
    red--;
    SoftPWMSet(RED_PIN, red);
  }
  }
  
 if(green == greenValue)
  {
  }
  else
  {
  if (green < greenValue)
  {
    green++;
    SoftPWMSet(GREEN_PIN, green);
  }
  else
  {
    green--;
    SoftPWMSet(GREEN_PIN, green);
  }
  }

 if(blue == blueValue)
  {
  }
  else
  {
  if (blue < blueValue)
  {
    blue++;
    SoftPWMSet(BLUE_PIN, blue);
  }
  else
  {
    blue--;
    SoftPWMSet(BLUE_PIN, blue);
  }
  }
  
  if(white == whiteValue)
  {
  }
  else
  {
  if (white < whiteValue)
  {
    white++;
    SoftPWMSet(WHITE_PIN, white);
  }
  else
  {
    white--;
    SoftPWMSet(WHITE_PIN, white);
  }
  Serial.println(red);
  Serial.println(blue);
  Serial.println(green);
  Serial.println(white);
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
  
  if(currentHour == 24)
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
