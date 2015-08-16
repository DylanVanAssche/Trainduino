#include <SPI.h>  // Libraries for the RF24 radio
#include "nRF24L01.h"
#include "RF24.h"
#include <digitalWriteFast.h> // Portmanipulation library
#include <Servo.h>  // Servo library
#include <NmraDcc.h>  // DCC library
#include <avr/wdt.h>  // Watchdogtimer library

/*
 ******************************************
 *               Trainduino               *
 *                                        *
 *-----> INTELLIGENT SERVO DECODER <------*
 *                                        *
 *             V 1.0 RELEASE              *
 ******************************************
 THIS IS A RELEASE! I'm never resposibly for any damage to your stuff! This version is tested on Arduino UNO.

 The Intelligent Servo Decoder controls up to 4 servo with DCC, configurable with CV's on your command station.
 In combination with the Trainduino S88-Wireless you can send the turnout states back to the command station like a Railcom+ capable decoder.

 It's also heavy documented, so you don't need a lot knowledge to understand it.

  >>> FEATURES:
 ---------

 * The Intelligent Servo Decoder can control up to 4 servo's.
   You need to change CV 99 when you use less then 4 servo's, to avoid wasted CPU cycles.
 * Galvanic isolation between the DCC track and the Intelligent Servo Decoder.
 * Full DCC compatible delivered by the Alex Shepards NMRA DCC library.
 * Full configurable with CV's (reading & writing ACK) only available on the PROG track (no POM!).
 * Reads the servo potmeter to determine if the turnout state match with the command station.
 * Automatically servo potmeter calibration at first boot and if you change a servo angle.
   You can read the calibrated servo potmeter CV values with your command station.
 * Optimized for the Trainduino C-rail turnout switch.
 * Anti-crash protection with a Watchdog timer.
 * Uses NRF24L01+ transceiver who operates at a 2,4 Ghz frequency.
 * Easy to change the channel and number of retries in the sketch. All the other settings of the NRF24L01+ are already optimized by myself.
   You can find a noise-free channel by using the example 'scanner' from the Maniacbugs RF24 library.
 * Built-in CRC and ACK are activated by default.
 * Fast port manipulation delivered by the digitalWriteFast library.
 * NRF24L01+ driver delivered by Maniacbugs RF24 library.


 >>> IMPROVEMENTS:
 -------------

 [NEW]  Added anti-noise capacitors on the PCB and shielded cable to minimize noise on the RF24 radio.

 [NEW]  The IRQ-pin from the RF24 radio is now connected to pin 8 and programmed in the sketch.
        The Intelligent Servo Decoder will operate more efficient, since he will send only the data to the base when it's available.

 [FIX]  Interference with DCC CV reading & writing while handling RF24 radio data (blocking code).
        With DCC address 1000 you can now deactivate the RF24 radio module if you face this problem.

 [FIX]  Only servo 1 replied to DCC commands. That's now fixed by removing the blocking for() loop in the function servoMove().


 >>> IMPORTANT NOTES:
 ----------------
 * There're only two free pins (pin 0 & 1) anymore on the Arduino (UNO, Leonardo, ...),
   you can use shitfregisters or other SPI devices to extend this Intelligent Servo Decoder or you can grab an Arduino MEGA.
 * To load the initial CV values you have to write in CV 8 value 8.
 * Feel free to modify these sketches but don't hijack them ;-)

 (c) Dylan Van Assche (2014 - 2015), producer of the Trainduino serie.
 */

///////////////////
///  CONSTANTS  ///
///////////////////

#define DCC_ACK 3
#define DCC_LED A5
#define IRQ_RF24 8
#define WIRELESS_LED A4
#define RADIO_CHANNEL 111  // The channel for the NRF24L01+ radio module. RANGE: 0 - 127.
#define RF24_SHUTDOWN_ADDRESS 1000 // Switch this turnout to deactivate the RF24 radio module and activate DCC CV writing & reading.
#define ACK_TIMEOUT 1000  // Disconnection time when reading & writing CV's.

const byte CV_SERVO_1_ANGLE_MIN =     30;
const byte CV_SERVO_1_ANGLE_MAX =     31;
const byte CV_SERVO_1_POTMETER_MIN =  32;
const byte CV_SERVO_1_POTMETER_MAX =  33;
const byte CV_SERVO_1_SPEED =         34;
const byte CV_SERVO_1_CURRENT_POS =   35;
const byte CV_SERVO_2_ANGLE_MIN =     36;
const byte CV_SERVO_2_ANGLE_MAX =     37;
const byte CV_SERVO_2_POTMETER_MIN =  38;
const byte CV_SERVO_2_POTMETER_MAX =  39;
const byte CV_SERVO_2_SPEED =         40;
const byte CV_SERVO_2_CURRENT_POS =   41;
const byte CV_SERVO_3_ANGLE_MIN =     42;
const byte CV_SERVO_3_ANGLE_MAX =     43;
const byte CV_SERVO_3_POTMETER_MIN =  44;
const byte CV_SERVO_3_POTMETER_MAX =  45;
const byte CV_SERVO_3_SPEED =         46;
const byte CV_SERVO_3_CURRENT_POS =   47;
const byte CV_SERVO_4_ANGLE_MIN =     48;
const byte CV_SERVO_4_ANGLE_MAX =     49;
const byte CV_SERVO_4_POTMETER_MIN =  50;
const byte CV_SERVO_4_POTMETER_MAX =  51;
const byte CV_SERVO_4_SPEED =         52;
const byte CV_SERVO_4_CURRENT_POS =   53;
const byte CV_NUMBER_OF_SERVOS =      99;
const byte CV_RADIO_ADDRESS =         100;
const byte servoPin[4] =              {4, 5, 6, 7};  // Servo signal pins.
const byte servoPotmeterPin[4] =      {A3, A2, A1, A0};  // Analogpins for reading the servo potmeters.
const uint64_t pipes[2] =             {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};  // Pipes RF24.

/////////////////
///  OBJECTS  ///
/////////////////

RF24 s88Radio(9, 10);
Servo servo[4];
NmraDcc  DCC ;
DCC_MSG  Packet ;

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

CVPair FactoryDefaultCVs [] =
{
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, 26}, // Address 26: turnout 101 - 104
  {CV_VERSION_ID, MAN_ID_DIY},
  {CV_MANUFACTURER_ID, MAN_ID_DIY},
  {CV_SERVO_1_ANGLE_MIN, 73},
  {CV_SERVO_1_ANGLE_MAX, 130},
  {CV_SERVO_1_POTMETER_MIN, 0}, // Preform autocalibration of the servopotmeter.
  {CV_SERVO_1_POTMETER_MAX, 0},
  {CV_SERVO_1_SPEED, 25},
  {CV_SERVO_1_CURRENT_POS, 90},
  {CV_SERVO_2_ANGLE_MIN, 73},
  {CV_SERVO_2_ANGLE_MAX, 130},
  {CV_SERVO_2_POTMETER_MIN, 0}, // Preform autocalibration of the servopotmeter.
  {CV_SERVO_2_POTMETER_MAX, 0},
  {CV_SERVO_2_SPEED, 25},
  {CV_SERVO_2_CURRENT_POS, 90},
  {CV_SERVO_3_ANGLE_MIN, 73},
  {CV_SERVO_3_ANGLE_MAX, 130},
  {CV_SERVO_3_POTMETER_MIN, 0}, // Preform autocalibration of the servopotmeter.
  {CV_SERVO_3_POTMETER_MAX, 0},
  {CV_SERVO_3_SPEED, 25},
  {CV_SERVO_3_CURRENT_POS, 90},
  {CV_SERVO_4_ANGLE_MIN, 73},
  {CV_SERVO_4_ANGLE_MAX, 130},
  {CV_SERVO_4_POTMETER_MIN, 0}, // Preform autocalibration of the servopotmeter.
  {CV_SERVO_4_POTMETER_MAX, 0},
  {CV_SERVO_4_SPEED, 25},
  {CV_SERVO_4_CURRENT_POS, 90},
  {CV_NUMBER_OF_SERVOS, 4},
  {CV_RADIO_ADDRESS, 1},
};

///////////////
///  BYTES  ///
///////////////

byte servoAngleLow[4];
byte servoAngleHigh[4];
byte servoSpeed[4];
byte servoRequestedPos[4];
byte servoHandler;
byte servoCounter;
byte numberOfServos;
byte turnoutCounter[8];  // The individual counter for every turnout to avoid noise and bad wheelcontacts.
byte turnoutState[8];  // We store here the state of every turnout in.
byte dipSwitchState[6];  // The state of every switch is stored here.
byte s88Data;  // We will store the s88 data in this variable.
byte slaveAddress;  // The slave address that will be read out at the setup from the DIP-switch configuration.
byte s88Address; // This will be the address that the base sends out, if it's the same as ours SLAVE_ADDRESS then we need to send back the s88 data.
byte DCCAddress;
byte FactoryDefaultCVIndex;

//////////////////
///  INTEGERS  ///
//////////////////

int servoPotmeterLow[4];
int servoPotmeterHigh[4];
int blinkTimeDCCLED;
int servoPotmeterValue;

///////////////
///  LONGS  ///
///////////////

long previousMillisACKTimeout;
long previousMillisServo;
long previousMillisDetach;
long previousMillisDCCLED;
long previousMillisRF24Shutdown;

//////////////////
///  BOOLEANS  ///
//////////////////

boolean RF24Shutdown;

void setup()
{
  // Setup DCC library with INT0 on pin 2.
  DCC.pin(0, 2, 1);
  DCC.init( MAN_ID_DIY, 10, FLAGS_OUTPUT_ADDRESS_MODE | FLAGS_DCC_ACCESSORY_DECODER, 0 );

  // If the decoder receive a reset flag, set all the CV to the factory defaults.
  for (int i = 0; i < FactoryDefaultCVIndex; i++)
  {
    DCC.setCV( FactoryDefaultCVs[i].CV, FactoryDefaultCVs[i].Value);
  }

  // Download the RF24 address from the CV register.
  slaveAddress = DCC.getCV(CV_RADIO_ADDRESS);
  DCCAddress = DCC.getCV(CV_MULTIFUNCTION_PRIMARY_ADDRESS);
  numberOfServos = DCC.getCV(99);

  // Setup the servo's and download the right values from the CV register.
  for (int i = 0; i < numberOfServos; i++)
  {
    servoAngleLow[i] = DCC.getCV(30 + 6 * i);
    servoAngleHigh[i] = DCC.getCV(31 + 6 * i);
    servoPotmeterLow[i] = DCC.getCV(32 + 6 * i);
    servoPotmeterLow[i] = map(servoPotmeterLow[i], 0, 255, 0, 1023);
    servoPotmeterHigh[i] = DCC.getCV(33 + 6 * i);
    servoPotmeterHigh[i] = map(servoPotmeterHigh[i], 0, 255, 0, 1023);
    servoSpeed[i] = DCC.getCV(34 + 6 * i);
    servoRequestedPos[i] = DCC.getCV(35 + 6 * i);
    servo[i].attach(servoPin[i]);
    servo[i].write(servoRequestedPos[i]);
    servoPotmeterValue = analogRead(servoPotmeterPin[i]);

    // Refresh the s88Data and update the potmetervalues (only when the servo angles are changed by the user).
    if (servoPotmeterValue <= servoPotmeterLow[i]) // Position LOW reached?
    {
      // Update the s88Data and save the current position into a CV (EEPROM).
      turnoutState[2 * i] = 0;
      turnoutState[(2 * i) + 1] = 1;
      DCC.setCV(35 + 6 * i, servoAngleLow[i]);
    }
    else
    {
      if (servoPotmeterValue >= servoPotmeterHigh[i]) // Position HIGH reached?
      {
        // Update the s88Data and save the current position into a CV (EEPROM).
        turnoutState[i] = 1;
        turnoutState[i + 1] = 0;
        DCC.setCV(35 + 6 * i, servoAngleHigh[i]);
      }
      else // Between 2 postions? Set both to 0.
      {
        turnoutState[2 * i] = 0;
        turnoutState[(2 * i) + 1] = 0;
      }
    }
  }

  // Set the pinModes for the different I/O.
  pinModeFast(WIRELESS_LED, OUTPUT);
  pinModeFast(IRQ_RF24, INPUT);
  pinModeFast(DCC_ACK, OUTPUT);
  pinModeFast(DCC_LED, OUTPUT);
  for (int i = 0; i < numberOfServos; i++)
  {
    pinMode(servoPotmeterPin[i], INPUT);
  }

  // Turn the LED's OFF.
  digitalWriteFast(WIRELESS_LED, HIGH);
  digitalWriteFast(DCC_LED, LOW);

  // Setup the RF24 radio.
  s88Radio.begin();
  s88Radio.setRetries(10, 10);
  s88Radio.setDataRate(RF24_250KBPS);
  s88Radio.setPayloadSize(2);
  s88Radio.setChannel(RADIO_CHANNEL);
  s88Radio.openWritingPipe(pipes[1]);
  s88Radio.openReadingPipe(1, pipes[0]);
  s88Radio.startListening();

  // Start the Watchdogtimer to check for loops in our sketch.
  wdt_enable(WDTO_2S);
}

void loop()
{
  // DCC process needs to be called frequently into our loop for a smooth library operation.
  DCC.process();

  // If the decoder receive a reset flag, set all the CV to the factory defaults.
  if (FactoryDefaultCVIndex && DCC.isSetCVReady())
  {
    FactoryDefaultCVIndex--;
    DCC.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);

    // Provide the user some ACK with the DCC_LED.
    previousMillisDCCLED = millis();
    blinkTimeDCCLED = 3000;
  }

  // Function which take care of the DCC_LED.
  DCCLED();

  // Function which take care of all the servo's.
  servoMove();

  // If the RF24 has data for us, he will turn the IRQ pin LOW.
  if (!digitalReadFast(IRQ_RF24) && !RF24Shutdown)
  {
    // Send our s88Data to the master.
    updateS88Data();
  }

  // Reset the Watchdogtimer
  wdt_reset();
}

void updateS88Data()
{
  // Combine our data array into one single byte.
  s88Data = turnoutState[0] | (turnoutState[1] << 1) | (turnoutState[2] << 2) | (turnoutState[3] << 3) | (turnoutState[4] << 4) | (turnoutState[5] << 5) | (turnoutState[6] << 6) | (turnoutState[7] << 7);

  // If we aren't reading or writing CV's, the IRQ pin is LOW and there's data available then we need to read it.
  if ((millis() - previousMillisACKTimeout > ACK_TIMEOUT) && s88Radio.available())
  {
    // This boolean will delay the microcontroller until he received everything.
    boolean transmissionComplete = false;
    digitalWriteFast(WIRELESS_LED, HIGH);

    if (!transmissionComplete && (millis() - previousMillisACKTimeout >= ACK_TIMEOUT))
    {
      transmissionComplete = s88Radio.read(&s88Address, sizeof(byte)); // Receive the data.
      delay(15);
    }

    if (s88Address == slaveAddress) // If this slave is polled, we have to send back the s88Data.
    {
      s88Radio.stopListening();
      s88Radio.write(&s88Data, sizeof(byte));
      s88Radio.startListening();
    }
    digitalWriteFast(WIRELESS_LED, LOW);
  }
}

void DCCLED()
{
  if (millis() - previousMillisDCCLED >= blinkTimeDCCLED)
  {
    digitalWriteFast(DCC_LED, LOW);
  }
  else
  {
    digitalWriteFast(DCC_LED, HIGH);
  }
}

void servoMove()
{
  if (millis() - previousMillisServo >= servoSpeed[0])
  {
    for (int i = 0; i < numberOfServos; i++)
    {
      previousMillisServo = millis();
      if (servo[i].read() < servoRequestedPos[i])
      {
        byte servoPos = servo[i].read() + 1;
        servo[i].write(servoPos);
        turnoutState[2 * i] = 0;
        turnoutState[(2 * i) + 1] = 0;
      }

      if (servo[i].read() > servoRequestedPos[i])
      {
        byte servoPos = servo[i].read() - 1;
        servo[i].write(servoPos);
        turnoutState[2 * i] = 0;
        turnoutState[(2 * i) + 1] = 0;
      }

      if (servo[i].read() == servoRequestedPos[i])
      {
        servo[i].detach();
        servoPotmeterValue = analogRead(servoPotmeterPin[i]);
        if (servoPotmeterValue <= servoPotmeterLow[i])
        {
          while (DCC.getCV(32 + 6 * i) == 0)
          {
            // Provide the user some ACK with the DCC_LED.
            previousMillisDCCLED = millis();
            blinkTimeDCCLED = 1000;
            DCC.setCV(32 + 6 * i, (map(servoPotmeterValue, 0, 1023, 0, 255) + 3));

          }
          turnoutState[2 * i] = 1;
          turnoutState[(2 * i) + 1] = 0;
          DCC.setCV(35 + 6 * i, servoAngleLow[i]);
        }

        if (servoPotmeterValue >= servoPotmeterHigh[i])
        {
          while (DCC.getCV(33 + 6 * i) == 0)
          {
            // Provide the user some ACK with the DCC_LED.
            previousMillisDCCLED = millis();
            blinkTimeDCCLED = 1000;
            DCC.setCV(33 + 6 * i, (map(servoPotmeterValue, 0, 1023, 0, 255) - 3));

          }
          turnoutState[2 * i] = 0;
          turnoutState[(2 * i) + 1] = 1;
          DCC.setCV(35 + 6 * i, servoAngleHigh[i]);
        }
      }
    }
  }
}

void notifyCVAck(void)
{
  // Reset the previousMillisACKTimeout, when this function isn't called anymore the RF24 will start again communicating with the base.
  previousMillisACKTimeout = millis();

  // Provide the user some ACK with the DCC_LED.
  previousMillisDCCLED = millis();
  blinkTimeDCCLED = 250;

  // To send a CV ACK to the command station we need to toggle the DCC_ACK pin for 6 ms.
  digitalWriteFast(DCC_ACK, HIGH);
  delay(6);
  digitalWriteFast(DCC_ACK, LOW);
}

void notifyCVChange( uint16_t CV, uint8_t Value)
{
  // Check if the user changed the CV_SERVO_X_ANGLE_MIN or CV_SERVO_X_ANGLE_MAX.
  for (int i = 0; i < numberOfServos; i++)
  {
    if (CV == 30 + 6 * i)
    {
      servoAngleLow[i] = DCC.getCV(30 + 6 * i);
      // Changed? Flag the servopotmeter calibration process.
      DCC.setCV(32 + 6 * i, 0);
    }

    if (CV == 31 + 6 * i)
    {
      servoAngleHigh[i] = DCC.getCV(31 + 6 * i);
      // Changed? Flag the servopotmeter calibration process.
      DCC.setCV(33 + 6 * i, 0);
    }

    if (CV == 34 + 6 * i)
    {
      servoSpeed[i] = DCC.getCV(34 + 6 * i);
    }
  }
  slaveAddress = DCC.getCV(CV_RADIO_ADDRESS);
  DCCAddress = DCC.getCV(CV_MULTIFUNCTION_PRIMARY_ADDRESS);
}

void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
};

void notifyDccAccState( uint16_t address, uint16_t boardAddress, uint8_t outputAddress, uint8_t state)
{
  // Provide the user some ACK with the DCC_LED.
  previousMillisDCCLED = millis();
  blinkTimeDCCLED = 100;

  if (boardAddress == DCCAddress)
  {
    // This switch statement will attach the right servo and set the requested position when it receives a corrent DCC command.
    switch (outputAddress)
    {
      case 0: // Servo 1
        servo[0].attach(servoPin[0]);
        servoRequestedPos[0] = servoAngleLow[0];
        break;

      case 1:
        servo[0].attach(servoPin[0]);
        servoRequestedPos[0] = servoAngleHigh[0];
        break;

      case 2: // Servo 2
        servo[1].attach(servoPin[1]);
        servoRequestedPos[1] = servoAngleLow[1];
        break;

      case 3:
        servo[1].attach(servoPin[1]);
        servoRequestedPos[1] = servoAngleHigh[1];
        break;

      case 4: // Servo 3
        servo[2].attach(servoPin[2]);
        servoRequestedPos[2] = servoAngleLow[2];
        break;

      case 5:
        servo[2].attach(servoPin[2]);
        servoRequestedPos[2] = servoAngleHigh[2];
        break;

      case 6: // Servo 4
        servo[3].attach(servoPin[3]);
        servoRequestedPos[3] = servoAngleLow[3];
        break;

      case 7:
        servo[3].attach(servoPin[3]);
        servoRequestedPos[3] = servoAngleHigh[3];
        break;
    }
  }
  if (address == RF24_SHUTDOWN_ADDRESS)
  {
    if ((outputAddress % 2) == 0)
    {
      RF24Shutdown = true;
    }
    else
    {
      RF24Shutdown = false;
    }
  }
}
