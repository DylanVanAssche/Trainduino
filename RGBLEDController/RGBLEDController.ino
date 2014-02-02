#include <SoftPWM_timer.h>
#include <SoftPWM.h>
#include <TimedAction.h>

/*
   ******************************************
   *               Trainduino               *
   *                                        *
   *----------> RGBLEDCONTROLLER <----------*
   *                                        *
   *             V 1.0.0 RELEASE            *
   ******************************************
   THIS IS A RELEASE! I'm never resposibly for any damage to your stuff! This version is completely tested.

   (c) Dylan Van Assche (2013 - 2014), producer of the Trainduino serie.
*/

#define RED_PIN 9
#define GREEN_PIN 6
#define BLUE_PIN 5
#define WHITE_PIN 3

int red                   =     230;
int green                 =     230;
int blue                  =     75;
int white                 =     0;

int redValue              =     255;
int greenValue            =     255;
int blueValue             =     255;
int whiteValue            =     254;

boolean Status            =     true;

TimedAction RGBTimer      =     TimedAction(300, RGB);
TimedAction Clock         =     TimedAction(1000, ClockFunction);

int currentHour           =     6; // Start ALWAYS the program in DAY mode. Day is from 07:00 to 19:00.
int currentMinutes        =     0;

void setup() {
  
  pinMode(RED_PIN,OUTPUT);
  pinMode(GREEN_PIN,OUTPUT);
  pinMode(BLUE_PIN,OUTPUT);
  pinMode(WHITE_PIN,OUTPUT);
  pinMode(15,OUTPUT);
  
  randomSeed(A0);

}

void loop() {

  Clock.check(); // Time tracking process.
  if (Status)    // DAY.
  {
    digitalWrite(15,HIGH);
    redValue    =  255;
    greenValue  =  200;
    blueValue   =  100;
    whiteValue  =  254;
    RGBTimer.check(); // Every time the RGBTimer is triggered, changes the PWM output. With this timer you can change the colors of the RGB LEDstrip very slowly.
  }
  else  // NIGHT.
  {
    digitalWrite(15,LOW);
    redValue    =  50;
    greenValue  =  12;
    blueValue   =  110;
    whiteValue  =  0;
    RGBTimer.check(); // Every time the RGBTimer is triggered, changes the PWM output. With this timer you can change the colors of the RGB LEDstrip very slowly.
  }
}

void RGB() // This function controls the RGB LEDstrip.
{
  if (red < redValue)
  {
    red = red++;
    analogWrite(RED_PIN, red);
  }
  else
  {
    red = red--;
    analogWrite(RED_PIN, red);
  }

  if (green < greenValue)
  {
    green = green++;
    analogWrite(GREEN_PIN, green);
  }
  else
  {
    green = green--;
    analogWrite(GREEN_PIN, green);
  }

  if (blue < blueValue)
  {
    blue = blue++;
    analogWrite(BLUE_PIN, blue);
  }
  else
  {
    blue = blue--;
    analogWrite(BLUE_PIN, blue);
  }
  
    if (white <= whiteValue)
  {
    if(whiteValue == 0 || whiteValue == 255)  // Keeps the white LED OFF or ON. Otherwise it sticks in an ON/OFF loop...
    {
      
    }
    else
    {
    white = white++;
    analogWrite(WHITE_PIN, white);
    }
  }
  else
  {
    white = white--;
    analogWrite(WHITE_PIN, white);
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
  
  if (currentHour == 7)
  {
    Status = true;  // DAY
  }
  if(currentHour == 20)
  {
    Status = false;  // NIGHT
  }
  if(currentHour == 24)
  {
   currentHour = 0; 
  }
}
