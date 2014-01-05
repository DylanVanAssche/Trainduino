#include <SoftPWM_timer.h>
#include <SoftPWM.h>
#include <TimedAction.h>

/*
   ******************************************
   *               Trainduino               *
   *                                        *
   *--------> RANDOMLEDCONTROLLER <---------*
   *                                        *
   *             V 1.0 RELEASE              *
   ******************************************
   THIS IS A RELEASE! I'm never resposibly for any damage to your stuff! This version is completely tested.

   (c) Dylan Van Assche (2014), producer of the Trainduino serie.
*/

boolean Status = true; // Starting always in DAY mode!

TimedAction DayON          =     TimedAction(random(1000, 2000), DayONFunction); // Define every timer for this sketch.
TimedAction DayOFF         =     TimedAction(random(2500, 3500), DayOFFFunction);
TimedAction Night          =     TimedAction(random(1000, 2000), NightFunction);
TimedAction Clock          =     TimedAction(1000, ClockFunction);

int currentHour           =    7; // Start ALWAYS the program in DAY mode. Day is from 07:00 to 19:00.
int currentMinutes        =    0;

void setup()
{
  SoftPWMBegin();
  for (int i = 2; i < 18; i++) // Set pinMode for output. Pin 2 -> pin 18. Pin 19 is used for the RandomSeed function.
  {
    SoftPWMSet(i, 0);
    SoftPWMSetFadeTime(i, 1000, 1000);
  }
  randomSeed(A5);
}

void loop()
{
  Clock.check(); // Time tracking process.
  if(Status) // Controls the DAY/NIGHT process.
  {
    DayON.check();
    DayOFF.check();
    SoftPWMSetPercent(19, 0);  // Deactivate the streetlights.
  }
  else
  {
    Night.check();
    SoftPWMSetPercent(19, 100); // Activate the streetlights.
  }
}

void DayONFunction() // It's day, turn one output ON and another OFF.
{
  SoftPWMSetPercent(random(0, 19), random(70, 100));
}

void DayOFFFunction()
{
  SoftPWMSetPercent(random(0, 19), 0);
}

void NightFunction() // Turn all the outputs OFF! It's night, most of the people sleeps at this moment.
{
  SoftPWMSetPercent(random(0, 19), 0);
}

void ClockFunction()
{
  currentMinutes++;

  if (currentMinutes == 60)
  {
    currentHour++;
    currentMinutes = 0;

  if (currentHour == 12)
  {
    Status = !Status;
    currentHour = 0;
  }
    
    
  }
}
