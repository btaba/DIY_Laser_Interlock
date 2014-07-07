DIY_Laser_Interlock
===================

I built a prototype Laser Interlock system for an optics laboratory using an Arduino interfaced to switches, LEDs, magnetic sensors, a keypad, and magnetic relays.

I followed these Class IV design specifications: http://ncsu.edu/ehs/laser/

The original design called for external laser shutters, but I resorted to using the laser kill switch found on most lasers. I wired a relay from the Arduino to these ports. All other peripheries are enumerated in the comments section of the main code. 

Dependencies on: Keypad.h, Password.h, and LiquidCrystal.h

http://playground.arduino.cc/Code/Keypad

http://www.arduino.cc/en/Reference/LiquidCrystal

http://playground.arduino.cc/Code/Password



