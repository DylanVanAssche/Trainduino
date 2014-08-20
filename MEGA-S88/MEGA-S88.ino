#include <wiring.h>
#include <digitalWriteFast.h>  // LAZY port manipulation library...
#include <digital_write_macros.h>

/*
   ******************************************
   *               Trainduino               *
   *                                        *
   *--------------> MEGA - S88 <------------*
   *                                        *
   *              V 1.0 RELEASE             *
   ******************************************
   THIS IS A RELEASE! I'm never resposibly for any damage to your stuff! This version is tested on Arduino UNO.

   This sketch can simulate up to 32 S88 modules with each 16 feedback pins. That means that you can monitor 512 I/O pins :)
   It's also heavy documented, so you don't need a lot knowledge to understand it.
   The only thing you have to add is a digitalReadFast() function to monitor the I/O pins or you can also take a look at the wireless based version of MEGA-S88.
   
   (c) Dylan Van Assche (2013 - 2014), producer of the Trainduino serie.   
*/

#define S88_CLOCK    2  // CLOCK line = HIGH? -> Do something NOW!
#define S88_LOAD     3  // LOAD line = HIGH? -> Master request the data NOW!
#define S88_DATAOUT  4  // DATA line = This line will be used to write out the requested data in sync with the CLOCK pulses when the LOAD line is triggered.
#define S88_RESET    5  // Not necessary since this RESET line only affects the 4044 chips but we want to simulate the 4014 chips.
#define NUMBER_OF_S88MODULES  10  // The number of S88 units we want to simulate with the Arduino.

uint16_t s88Data[NUMBER_OF_S88MODULES];

volatile boolean LOADTrigger = false;

byte s88[2 * NUMBER_OF_S88MODULES] = {B00100100, B00011000, 0, B10000001, 0, B01000010, B00100100, B00011000, B10000001, B01000010, B00100100, B00011000,         B00100100, B00011000, 0, B10000001, 0, B01000010, B00100100, B00011000}; // The S88 data that will be received from the slaves. 
byte bitIndex = 0;
byte s88DataIndex = 0;

void setup()
{
  attachInterrupt(0, CLOCK, RISING); // Activate CLOCK interrupt on EVERY rising pulse of the CLOCK line.
  
  pinModeFast(S88_LOAD, INPUT); // pinModeFast -> pinMode for digitalWriteFast port manipulation library.
  pinModeFast(S88_DATAOUT, OUTPUT);
}

void loop()
{  
  // Combines 2x 8bit shift registers into 1x 16bit shift register. 
  for(byte dataManipulationIndex = 0;  dataManipulationIndex < NUMBER_OF_S88MODULES; dataManipulationIndex++)
  {
  s88Data[dataManipulationIndex] = (s88[dataManipulationIndex * 2 + 1] << 8)  |  s88[dataManipulationIndex * 2];
  }
}
    

void CLOCK() // If CLOCK rises, check the LOAD line to see if we have to do something!
{
  detachInterrupt(0); // Deactivate interrupt CLOCK.
  
  if(digitalReadFast(S88_LOAD) || LOADTrigger != false) // The LOAD line has been triggered by the command station, he wants the data NOW! Let's give it to him.
  {
    LOADTrigger = true; // The LOAD line will not stay high until all the bits are written out, only a pulse occurs every cycle but this function must go on until all the bits are written out.
    
    if(s88Data[s88DataIndex] & (1 << bitIndex)) // Write every bit out.
    { 
      digitalWriteFast(S88_DATAOUT, HIGH);
    }
    else
    {
      digitalWriteFast(S88_DATAOUT, LOW);
    }
    
    bitIndex++; // Next bit
  }

  if(bitIndex > 15) // 16 bits passed? Reset the counter...
  {
    s88DataIndex++;
    bitIndex = 0;
    
    if(s88DataIndex > NUMBER_OF_S88MODULES - 1) // Human counting to array counting conversion. We want to simulate 5 S88 modules, so we give the variable NUMBER_OF_S88MODULES a value of 5. However, the array starts always with a 0.
    {
      s88DataIndex = 0; // Reset the s88DataIndex counter, this counter tracks the number of 16bit shift register 4014.
      LOADTrigger = false; // Everything is written out so we can stop writing data out.
    }
  } 
 
  attachInterrupt(0, CLOCK, RISING); // Activate interrupt CLOCK again.
}
