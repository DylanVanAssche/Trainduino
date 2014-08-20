ReadMe Trainduino
=================

Trainduino is a project that combines the power of the Arduino with the beautifull modeltrains. 
The electronics that are avaibale in the modeltrain world are overpriced for the limited functions they have, 
so this is your alternative!

RandomLEDController
-------------------

Controls a day/night process on my modelroad. It works with a cylcus of 24 minutes. 
Every minute counts the Arduino as one 'hour'.After the whole cyclus, the Arduino resets the timer.
The RandomLEDController can control 14 LED's in houses and has also 1 output for the streetlights.

RGBLEDController
----------------

Does the same as the RandomLEDController but this time it controls a RGB LEDstrip and a white LEDstrip to simulate a 
completely day/night. It runs synchrone with the RandomLEDController because it use the same timer system.

DCCSwitchController
-------------------

DCC accessory decoder with some extra nice functions. You can control 2 switches and 1 brake module.
There are also 4 outputs for a signal. The 3 LED's on the board show the status of the incomming commands.

S88Wireless
-----------

The S88Wireless system give you the possibilities to use your cheap S88 bus over a wireless connection. 
It's perfectly for a railroad track that has to be flexible and portable since you don't need the 6pin ribbon cable all around your layout!
