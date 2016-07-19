/*
 ***************************************
 *             Trainduino              *
 *                                     *
 *--------> ROCRAIL THROTTLE <---------*
 *                                     *
 *             V 1.0 RELEASE           *
 ***************************************
  THIS IS A RELEASE! I'm never resposibly for any damage to your stuff! This version is completely tested on an Arduino Pro Micro.

  The Rocrail throttle is a gamepad emulator based on the HID-libraries for Arduino provided by Nicohood.
  The Rocrail throttle can control a locomotives speed, direction and functions and the command station's track power.

  >>> FEATURES:
  -------------
     Rocrail trottle compatible through gamepad emulation (thanks Nicohood for your great work!).
     Full control of your locomotive his functions (max 24), speed and direction.
     Shift between F1-F12 and F13-F24 with the shift key Fn. The shift key state is visible to the user with a LED.
     I used Fn since that's the same key Rocrail uses for shifting between functions.
     Select other locomotives by scrolling through the list provided by Rocrail. While in selecting mode, all unnecessary keys are disabled.
     Status of the E-stop and the PC communication can be followed by LED's. The E-stop LED will blink when the emergency mode has been activated.
     Workaround for the gamepad implentation in Rocrail, if you select a certain locomotive for the first time the functions won't be synced.
     Trainduino's Rocrail throttle will send then once all the functions to Rocrail to sync them. If you press then a function key, it will work as expected.
     All keys are equiped with an algoritme to avoid the phenomen 'deboucing'. Witout such an algoritme we can get false positives.

  >>> IMPORTANT NOTES:
  --------------------
   Only 3 pins are left on the Arduino Pro Micro (TX, RX and a random pin), you can add here extra hardware if you wish.
   No extra drivers are necessary for Windows and Linux for the Rocrail Throttle. I can't test it out on a Mac since I don't have an Apple Mac, however it should work out of the box.
   There's NO feedback between Rocrail and this throttle since it will be seen as a gamepad.
   You can't get any information about your locomotive from Rocrail with this method, you need to look in Rocview for information about your locomotive.
   If you have more then 100 locomotives stored in Rocrail you should increase sizeArrayLocomotiveFunctionsSynced. WARNING: the SRAM is limited!!!
   We need to avoid that you reach the end of the relative list we have created as a workaround of the gamepad implentation in Rocrail.
   You can adjust the deboucing algoritme by changing this parameter: debounceTime (in milliseconds).

  (c) Dylan Van Assche (2016), producer of the Trainduino serie.

*/

#include "HID-Project.h"
#include <Keypad.h>

// Constants for the pinouts
const byte shiftStatusLED = A2;
const byte changeLocomotiveLED = A3;
const byte ESTOPButton = 2;
const byte pinA = 7;  // Connected to CLK on KY-040
const byte pinB = A1;  // Connected to DT on KY-040
const byte buttonChangeDirection = 16;
const byte buttonLightF0 = 15;
const byte buttonChangeLocomotive = 14;
const byte ESTOP_LED = 0;
const byte buttonShift = 10;
const byte communicationLED = 17;
const byte ROWS = 4; // Four rows
const byte COLS = 3; // Three columns

// Constant data
const byte sizeArrayLocomotiveFunctionsSynced = 100;
const byte debounceTime = 15;

// Gamepad buttons for Rocrail
const byte CHANGE_DIRECTION = 5;
const byte POWER_OFF = 8;
const byte POWER_ON = 9;
const byte CHANGE_LOCOMOTIVE = 10;
const byte THROTTLE_UP = GAMEPAD_DPAD_UP;
const byte THROTTLE_DOWN = GAMEPAD_DPAD_DOWN;
const byte THROTTLE_NEUTRAL = GAMEPAD_DPAD_CENTERED;
const byte F0 = 2;
const byte F1 = 3;
const byte F2 = 4;
const byte F3 = 6;
const byte F4 = 12;
const byte F5 = 13;
const byte F6 = 14;
const byte F7 = 15;
const byte F8 = 16;
const byte F9 = 17;
const byte F10 = 18;
const byte F11 = 19;
const byte F12 = 20;
const byte F13 = 21;
const byte F14 = 22;
const byte F15 = 23;
const byte F16 = 24;
const byte F17 = 25;
const byte F18 = 26;
const byte F19 = 27;
const byte F20 = 28;
const byte F21 = 29;
const byte F22 = 30;
const byte F23 = 31;
const byte F24 = 32;

boolean changeLocomotiveStatus;
boolean changeLocomotiveButtonStatus;
boolean previousChangeLocomotiveButtonStatus;
boolean ESTOPStatus;
boolean ESTOPButtonStatus;
boolean ESTOPLEDStatus;
boolean previousESTOPButtonStatus;
boolean communicatingLEDStatus;
boolean changeLocomotiveLEDStatus;
boolean shiftStatus;
boolean shiftButtonStatus;
boolean previousShiftButtonStatus;
boolean locomotiveFunctionsSynced[sizeArrayLocomotiveFunctionsSynced];
boolean changeDirectionButtonStatus;
boolean previousChangeDirectionButtonStatus;
boolean lightButtonStatus;
boolean previousLightButtonStatus;
boolean directionEncoder = false;
boolean rotatingCW;

char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {6, 8, 9, 5};  // Connect to the row pinouts of the keypad.
byte colPins[COLS] = {4, 3, 1};  // Connect to the column pinouts of the keypad.
byte numberSelectedLocomotive = (sizeArrayLocomotiveFunctionsSynced / 2);  // A variable where we store the relative position of the locolist from Rocrail. We start in the middle of the list.

long previousMillisDebounceESTOP;
long previousMillisDebounceShift;
long previousMillisDebounceChangeLocomotive;
long previousMillisDebounceDirectionLocomotive;
long previousMillisDebounceLight;
long previousMillisUpdate;

int pinALast;
int aValue;
int bValue;
int filter;

//initialize an instance of class NewKeypad
Keypad functionsKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {

  // Setup the buttons & the LED's
  pinMode(buttonChangeDirection, INPUT_PULLUP);
  pinMode(buttonLightF0, INPUT_PULLUP);
  pinMode(buttonChangeLocomotive, INPUT_PULLUP);
  pinMode(ESTOPButton, INPUT_PULLUP);
  pinMode(buttonShift, INPUT_PULLUP);
  pinMode(communicationLED, OUTPUT);
  pinMode(shiftStatusLED, OUTPUT);
  pinMode(ESTOP_LED, OUTPUT);
  pinMode(changeLocomotiveLED, OUTPUT);
  digitalWrite(ESTOP_LED, LOW);
  pinMode(pinA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinA), speedEncoder, CHANGE);
  pinMode (pinB, INPUT);
  pinALast = digitalRead(pinA);

  // Refresh and start the gamepad emulator.
  Gamepad.begin();
}
void loop()
{
  // ESTOP
  ESTOPButtonStatus = !digitalRead(ESTOPButton);
  if (ESTOPStatus)
  {
    ESTOPLEDStatus = !ESTOPLEDStatus;
    digitalWrite(ESTOP_LED, ESTOPLEDStatus);
  }
  else
  {
    digitalWrite(ESTOP_LED, LOW);
  }
  if (ESTOPButtonStatus == HIGH && previousESTOPButtonStatus == LOW && millis() - previousMillisDebounceESTOP > debounceTime)
  {
    ESTOPStatus = !ESTOPStatus;
    previousMillisDebounceESTOP = millis();
    digitalWrite(ESTOP_LED, HIGH);
    if (!ESTOPStatus) // Power ON: ESTOPStatus = false.
    {
      Gamepad.press(POWER_ON);
      Gamepad.release(POWER_OFF);
    }
    else // Power OFF: ESTOPStatus = true.
    {
      Gamepad.press(POWER_OFF);
      Gamepad.release(POWER_ON);
    }
  }
  previousESTOPButtonStatus = ESTOPButtonStatus;

  // Functions keypad
  if (!digitalRead(buttonShift))
  {
    shiftButtonStatus = HIGH;
  }
  else
  {
    shiftButtonStatus = LOW;
  }

  if (!changeLocomotiveStatus && shiftButtonStatus == HIGH && previousShiftButtonStatus == LOW && millis() - previousMillisDebounceShift > debounceTime) // Change the status of the SHIFT key.
  {
    previousMillisDebounceShift = millis();
    shiftStatus = !shiftStatus;
    digitalWrite(shiftStatusLED, shiftStatus);
  }
  previousShiftButtonStatus = shiftButtonStatus;

  char functionKey = functionsKeypad.getKey();
  if (functionKey && !changeLocomotiveStatus)
  {
    Gamepad.release(POWER_OFF);
    Gamepad.release(POWER_ON);

    switch (functionKey)
    {
      case '1': // F1 or F13
        if (shiftStatus)
        {
          Gamepad.press(F13);
        }
        else
        {
          Gamepad.press(F1);
        }
        break;

      case '2': // F2 or F14
        if (shiftStatus)
        {
          Gamepad.press(F14);
        }
        else
        {
          Gamepad.press(F2);
        }
        break;

      case '3': // F3 or F15
        if (shiftStatus)
        {
          Gamepad.press(F15);
        }
        else
        {
          Gamepad.press(F3);
        }
        break;

      case '4': // F4 or F16
        if (shiftStatus)
        {
          Gamepad.press(F16);
        }
        else
        {
          Gamepad.press(F4);
        }
        break;

      case '5': // F5 or F17
        if (shiftStatus)
        {
          Gamepad.press(F17);
        }
        else
        {
          Gamepad.press(F5);
        }
        break;

      case '6': // F6 or F18
        if (shiftStatus)
        {
          Gamepad.press(F18);
        }
        else
        {
          Gamepad.press(F6);
        }
        break;

      case '7': // F7 or F19
        if (shiftStatus)
        {
          Gamepad.press(F19);
        }
        else
        {
          Gamepad.press(F7);
        }
        break;

      case '8': // F8 or F20
        if (shiftStatus)
        {
          Gamepad.press(F20);
        }
        else
        {
          Gamepad.press(F8);
        }
        break;

      case '9': // F9 or F21
        if (shiftStatus)
        {
          Gamepad.press(F21);
        }
        else
        {
          Gamepad.press(F9);
        }
        break;

      case '*': // F10 or F22
        if (shiftStatus)
        {
          Gamepad.press(F22);
        }
        else
        {
          Gamepad.press(F10);
        }
        break;

      case '0': // F11 or F23
        if (shiftStatus)
        {
          Gamepad.press(F23);
        }
        else
        {
          Gamepad.press(F11);
        }
        break;

      case '#': // F12 or F24
        if (shiftStatus)
        {
          Gamepad.press(F24);
        }
        else
        {
          Gamepad.press(F12);
        }
        break;
    }
  }
  else
  {
    Gamepad.release(F1);
    Gamepad.release(F2);
    Gamepad.release(F3);
    Gamepad.release(F4);
    Gamepad.release(F5);
    Gamepad.release(F6);
    Gamepad.release(F7);
    Gamepad.release(F8);
    Gamepad.release(F9);
    Gamepad.release(F10);
    Gamepad.release(F11);
    Gamepad.release(F12);
    Gamepad.release(F13);
    Gamepad.release(F14);
    Gamepad.release(F15);
    Gamepad.release(F16);
    Gamepad.release(F17);
    Gamepad.release(F18);
    Gamepad.release(F19);
    Gamepad.release(F20);
    Gamepad.release(F21);
    Gamepad.release(F22);
    Gamepad.release(F23);
    Gamepad.release(F24);
  }

  // Change locomotive

  if (!digitalRead(buttonChangeLocomotive))
  {
    changeLocomotiveButtonStatus = HIGH;
  }
  else
  {
    changeLocomotiveButtonStatus = LOW;
  }

  if (changeLocomotiveButtonStatus == HIGH && previousChangeLocomotiveButtonStatus == LOW && millis() - previousMillisDebounceChangeLocomotive > debounceTime) // Press the change locomotive button.(#GPbutton: 10).
  {
    previousMillisDebounceChangeLocomotive = millis();
    Gamepad.press(CHANGE_LOCOMOTIVE);
    Gamepad.release(POWER_OFF);
    Gamepad.release(POWER_ON);
    changeLocomotiveStatus = !changeLocomotiveStatus;
    digitalWrite(changeLocomotiveLED, changeLocomotiveStatus);
    if (!changeLocomotiveStatus && !locomotiveFunctionsSynced[numberSelectedLocomotive]) // We haven't synced yet the functions for this locomotive, let's do that first!
    {
      Gamepad.press(F1);
      Gamepad.press(F2);
      Gamepad.press(F3);
      Gamepad.press(F4);
      Gamepad.press(F5);
      Gamepad.press(F6);
      Gamepad.press(F7);
      Gamepad.press(F8);
      Gamepad.press(F9);
      Gamepad.press(F10);
      Gamepad.press(F11);
      Gamepad.press(F12);
      Gamepad.press(F13);
      Gamepad.press(F14);
      Gamepad.press(F15);
      Gamepad.press(F16);
      Gamepad.press(F17);
      Gamepad.press(F18);
      Gamepad.press(F19);
      Gamepad.press(F20);
      Gamepad.press(F21);
      Gamepad.press(F22);
      Gamepad.press(F23);
      Gamepad.press(F24);
      locomotiveFunctionsSynced[numberSelectedLocomotive] = true;
    }
  }
  else
  {
    Gamepad.release(CHANGE_LOCOMOTIVE);
  }
  previousChangeLocomotiveButtonStatus = changeLocomotiveButtonStatus;

  // Change locomotive direction.
  if (!digitalRead(buttonChangeDirection))
  {
    changeDirectionButtonStatus = LOW;
  }
  else
  {
    changeDirectionButtonStatus = HIGH;
  }

  if (!changeLocomotiveStatus && changeDirectionButtonStatus == HIGH && previousChangeDirectionButtonStatus == LOW && millis() - previousMillisDebounceDirectionLocomotive > debounceTime) // Press the direction button.(#GPbutton: 4).
  {
    previousMillisDebounceDirectionLocomotive = millis();
    Gamepad.press(CHANGE_DIRECTION);
    Gamepad.release(POWER_OFF);
    Gamepad.release(POWER_ON);
  }
  else
  {
    Gamepad.release(CHANGE_DIRECTION);
  }
  previousChangeDirectionButtonStatus = changeDirectionButtonStatus;

  // Control the light of the locomotive.
  if (!digitalRead(buttonLightF0))
  {
    lightButtonStatus = HIGH;
  }
  else
  {
    lightButtonStatus = LOW;
  }

  if (!changeLocomotiveStatus && lightButtonStatus == HIGH && previousLightButtonStatus == LOW && millis() - previousMillisDebounceLight > debounceTime) // Press the light function F0 (#GPbutton: 5).
  {
    previousMillisDebounceLight = millis();
    Gamepad.press(F0);
    Gamepad.release(POWER_OFF);
    Gamepad.release(POWER_ON);
  }
  else
  {
    Gamepad.release(F0);
  }
  previousLightButtonStatus = lightButtonStatus;

  // Write all the changes to the USB host.
  Gamepad.write();
  delay(100);
  communicatingLEDStatus = !communicatingLEDStatus;
  digitalWrite(communicationLED, communicatingLEDStatus);
  Gamepad.dPad2(THROTTLE_NEUTRAL);
}

// Change locomotive speed or choose locomotive (depending on changeLocomotiveStatus).
void speedEncoder() {
  aValue = digitalRead(pinA);
  bValue = digitalRead(pinB);
  if (aValue != pinALast) // Rotating
  {
    if (bValue != aValue) // Pin A changed first -> CW rotating.
    {
      rotatingCW = false;
    }
    else // Pin B changed first -> CCW rotating.
    {
      rotatingCW = true;
    }

    if (rotatingCW)
    {
      if (filter < 3)
      {
        filter++;
      }
    }
    else
    {
      if (filter > -3)
      {
        filter--;
      }
    }

    if (filter < -2)
    {
      Gamepad.dPad2(THROTTLE_DOWN);

      if (changeLocomotiveStatus && numberSelectedLocomotive > 0)
      {
        numberSelectedLocomotive--;
      }

      Gamepad.release(POWER_OFF);
      Gamepad.release(POWER_ON);
    }

    if (filter > 2)
    {
      Gamepad.dPad2(THROTTLE_UP);

      if (changeLocomotiveStatus && numberSelectedLocomotive < sizeArrayLocomotiveFunctionsSynced)
      {
        numberSelectedLocomotive++;
      }

      Gamepad.release(POWER_OFF);
      Gamepad.release(POWER_ON);
    }
  }
  pinALast = aValue;
  Gamepad.write();
}
