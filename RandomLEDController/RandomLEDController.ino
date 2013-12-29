#include <SoftPWM_timer.h>
#include <SoftPWM.h>
#include <TimedAction.h>

/*
   ******************************************
   *               Trainduino               *
   *                                        *
   *-----------> LIGHT DECODER <------------*
   *                                        *
   *            V 0.9 PRERELEASE            *
   ******************************************
   THIS IS A PRERELEASE! I'm never resposibly for any damage to your stuff! This version is also NOT completely tested.

   (c) Dylan Van Assche, producer of the Trainduino serie.
*/

boolean outputStatus[] = {
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
}; // Saves the last status of the outputs.
boolean Status = true; // Starting always in DAY mode!

TimedAction DayON          =     TimedAction(random(1000, 2000), DayONFunction); // Define every timer for this sketch.
TimedAction DayOFF         =     TimedAction(random(2500, 350), DayOFFFunction);
TimedAction Night          =     TimedAction(random(1000, 2000), NightFunction);
TimedAction TimerDayNight  =     TimedAction(1000, TimerDayNightFunction);

int Hours = 0; // Saves the hours that are passed. 1 hour = 1 minute.

void setup()
{
  SoftPWMBegin();
  for (int i = 2; i < 18; i++) // Set pinMode for output. Pin 2 -> pin 18. Pin 19 is used for the RandomSeed function.
  {
    SoftPWMSet(i, OUTPUT);
    SoftPWMSetFadeTime(i, 1000, 1000);
  }
  randomSeed(A5);
}

void loop()
{
  TimerDayNight.check(); // Time tracking process.
  if (Status) // Controls the DAY/NIGHT process.
  {
    DayON.check();
    DayOFF.check();
    SoftPWMSetPercent(17, 0);  // Deactivate the streetlights.
  }
  else
  {
    Night.check();
    SoftPWMSetPercent(17, 100); // Activate the streetlights.
  }
}

void DayONFunction() // It's day, turn one output ON and another OFF.
{
  SoftPWMSetPercent(outputStatus[random(0, 16)], random(70, 100));
}

void DayOFFFunction()
{
  SoftPWMSetPercent(outputStatus[random(0, 16)], 0);
}

void NightFunction() // Turn all the outputs OFF! It's night, most of the people sleeps at this moment.
{
  SoftPWMSetPercent(outputStatus[random(0, 16)], 0);
}

void TimerDayNightFunction() // Time tracking function.
{
  Hours++;
  if (Hours >= 12)
  {
    Status = !Status;
    Hours = 0;
  }
}
