#include <TimedAction.h>
#include <NmraDcc.h>

#define GO      A0
#define STOP    15
#define STATUS  A1

/*
   ******************************************
   *               Trainduino               *
   *                                        *
   *--------> DCCSWITCHCONTROLLER <---------*
   *                                        *
   *             V 1.0 RELEASE              *
   ******************************************
   THIS IS A RELEASE! I'm never resposibly for any damage to your stuff! This version is completely tested.

   (c) Dylan Van Assche (2013 - 2014), producer of the Trainduino serie.

   GROUP DEFINITION:
   -----------------

   0    =  DCC Locdecoders
   100  =  DCC Switch Controllers
   200  =  DCC Scenery Controllers
   300 =  DCC Special Controllers

   Controller:
   GROUP:  100
   Number: 001
   Adres:  101 - 106
*/

NmraDcc  Dcc ;
DCC_MSG  Packet ;
const int NumberOfOutputs = 10;
const int DccAckPin = A2 ;
int Output[] = {2, 4, 5, 6, 7, 8, 9, 10, 16, 14}; // Arduino Pro Micro, Arduino MEGA
// int Output[] = {2, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // Arduino Uno, Arduino Leonardo, Arduino Mini
boolean RouteStatus = false;

unsigned long OutputpreviousMillis = 0;
int OutputNumber = 0;
int RoutePhase = 1;

TimedAction RouteTimer   =   TimedAction(200, RouteFunction);

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

CVPair FactoryDefaultCVs [] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, 1},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},
};

uint8_t FactoryDefaultCVIndex = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("DCC Switch Controller V0.1");

  // Setup DCC
  pinMode( DccAckPin, OUTPUT );
  Dcc.pin(0, 3, 1);
  Dcc.init( MAN_ID_DIY, 10, FLAGS_OUTPUT_ADDRESS_MODE | FLAGS_DCC_ACCESSORY_DECODER, 0 );

  for (int i = 0; i < NumberOfOutputs; i++)
  {
    pinMode(Output[i], OUTPUT);
  }
  pinMode(STOP, OUTPUT); // STOP
  pinMode(GO, OUTPUT); // GO
  pinMode(STATUS, OUTPUT); // STATUS
}

void loop()
{
  Dcc.process();
  if ( FactoryDefaultCVIndex && Dcc.isSetCVReady())
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
  }
  
  if (RouteStatus == true)
  {
    RouteTimer.check(); // Delay between 2 actions in the route.
    digitalWrite(GO, HIGH);
    digitalWrite(STOP, LOW);
  }
  else
  {
    digitalWrite(GO, LOW);
    digitalWrite(STOP, HIGH);
  }

}
void notifyCVAck(void)
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
    case 101: // Switch 1
      if ((OutputAddr % 2) == 0)
      {
        if(State == 8)
        {
          digitalWrite(Output[4], HIGH);
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(Output[4], LOW); 
          digitalWrite(STATUS, LOW);
        }
      }
      else
      {
        if(State == 8)
        {
          digitalWrite(Output[5], HIGH);
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(Output[5], LOW); 
          digitalWrite(STATUS, LOW);
        }
      }
      break;

    case 102: // Switch 2
      if ((OutputAddr % 2) == 0)
      {
        if(State == 8)
        {
          digitalWrite(Output[6], HIGH);
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(Output[6], LOW); 
          digitalWrite(STATUS, LOW);
        }
      }
      else
      {
        if(State == 8)
        {
          digitalWrite(Output[7], HIGH);
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(Output[7], LOW); 
          digitalWrite(STATUS, LOW);
        }
      }
      break;

    case 103: // Brake module
      if ((OutputAddr % 2) == 0)
      {
        if(State == 8)
        {
          digitalWrite(Output[8], HIGH);
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(Output[8], LOW); 
          digitalWrite(STATUS, LOW);
        }
      }
      else
      {
        if(State == 8)
        {
          digitalWrite(Output[9], HIGH);
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(Output[9], LOW); 
          digitalWrite(STATUS, LOW);
        }
      }
      break;

    case 104: // Signal address 1
      if ((OutputAddr % 2) == 0)
      {
        digitalWrite(Output[0], HIGH); // STOP (RED)
        digitalWrite(Output[1], LOW);
        digitalWrite(Output[2], LOW); 
        digitalWrite(Output[3], LOW);
        
        if(State == 8)
        {
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(STATUS, LOW);
        }
      }
      else
      {
        digitalWrite(Output[0], LOW); // GO (GREEN)
        digitalWrite(Output[1], HIGH);
        digitalWrite(Output[2], LOW); 
        digitalWrite(Output[3], LOW); 
        
        if(State == 8)
        {
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(STATUS, LOW);
        }
      }
      break;

    case 105: // Signal address 2
      if ((OutputAddr % 2) == 0)
      {
        digitalWrite(Output[0], LOW); 
        digitalWrite(Output[1], LOW); 
        digitalWrite(Output[2], HIGH); // HALF-SPEED (DOUBLE YELLOW)
        digitalWrite(Output[3], HIGH);
        
        if(State == 8)
        {
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(STATUS, LOW);
        }
      }
      else
      {
        digitalWrite(Output[0], HIGH); 
        digitalWrite(Output[1], LOW); 
        digitalWrite(Output[2], HIGH); // HALF-SPEED + GO (YELLOW + GREEN);
        digitalWrite(Output[3], LOW);
        
        if(State == 8)
        {
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(STATUS, LOW);
        }
      }
      break;

    case 106: // Route
      RouteStatus = true;
      
      if(State == 8)
        {
          digitalWrite(STATUS, HIGH);
        }
        else
        {
          digitalWrite(STATUS, LOW);
        }
      break;
  }
}

 void RouteFunction() // Delay between GO and STOP: X * 250ms. 7,5 seconds: 30x 250ms. The STOP command has number 31, 32, 33, ...
{
  switch (RoutePhase)
  {
    case 1: // 1st action: Set switches
      digitalWrite(Output[4], HIGH);
      digitalWrite(Output[6], HIGH);
      RoutePhase++;
      break;
      
    case 2:
      digitalWrite(Output[4], LOW);
      digitalWrite(Output[6], LOW);
      RoutePhase++;
      break;

    case 3: // 2nd action: Set Brake module & signal
      digitalWrite(Output[8], HIGH);
      digitalWrite(Output[0], HIGH); // GO (GREEN)
      digitalWrite(Output[1], LOW);
      digitalWrite(Output[2], LOW); 
      digitalWrite(Output[3], LOW);
      RoutePhase++;
      break;
      
    case 4:
      digitalWrite(Output[8], LOW);
      RoutePhase++;
      break;

    case 35: // 3rd action: Set Brake module & signal BACK to previous state
      digitalWrite(Output[9], HIGH);
      digitalWrite(Output[0], LOW); // STOP (RED)
      digitalWrite(Output[1], HIGH);
      digitalWrite(Output[2], LOW); 
      digitalWrite(Output[3], LOW);
      RoutePhase++;
      break;
    
    case 36:
      digitalWrite(Output[9], LOW);
      RoutePhase++;
      break;
    
    case 37: // 4th action: Set switches BACK to previous state
      digitalWrite(Output[5], HIGH);
      digitalWrite(Output[7], HIGH); 
      RoutePhase++;
      break; 
      
    case 38:
      digitalWrite(Output[5], LOW);
      digitalWrite(Output[7], LOW); 
      RoutePhase = 1;
      RouteStatus = false;
      break;
      
    default:
      RoutePhase++;
  }
}
