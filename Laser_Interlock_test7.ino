/* @file Laser_Interlock_test7
 || #
 || #  Inerlock System consists of Control Panel, Channel Box, and Control Box
 || #      
 || #  |    Control Panel is outside workbench on wall. Contains keypad, LCD, buzzer, and is connected to illuminated sign.
 || #  |       Also Connected to magnetic switches on curtain.
 || #  |          LEDs indicate which Laser is ON so you know which precaution to take before entering
 || #  |          A password entered will Suspend the Interlock
 || #  |                                                 unless this state is disabled by lab tech.
 || #  |    
 || #  |    Control and Channel Boxes are located inside the workbench
 || #  |    Control Box: located near curtain
 || #  |                contains a Suspend, Active, and Authorization switch
 || #  |                   Authorization On/Off switch allows or disallows Suspends from Keypad
 || #  |                   Suspend switch, suspends interlock
 || #  |                   Active switch turns on/off laser interlock system (and illuminated sign) if conditions are met
 || #  |    Channel Box: located equidistant from three optical tables.
 || #  |                  contains a Reset Switch with LED indicator. 
 || #  |                  Has 4 channels to short Interlock pins on each of 4 lasers
 || #  |                  Each channel has a Momentary switch with LED indicator
 || #  |                        this switch shorts Interlock pins when pressed using the relay circuits
 || #
 || #   **** INTERLOCK STATES ****
 || #
 || #  #  Once in the Active mode, the system can Trip or Suspend (for 30 sec)
 || #  #    The Illuminated Laser sign outside workbench is automatically ON when in Active mode
 || #  #  A Trip will open all Interlock pins on all Channels of Lasers
 || #  #  You must clear Trip and press reset on Channel Box before shorting Interlock Pins on any one channel
 || #  #    To short interlock pins, a Channel momentary switch must be pressed until LED indicates Interlock pins have been shorted
 || #
 || #  #  Hit Reset again to open all Interlock pins (i.e. disable any laser from operating when in Active mode)
 || # 
 || #  #  To Suspend, press Suspend switch inside workbench, or enter password on Keypad
 || #  #      If Authorization switch is ON, users cannot Suspend using the keypad
 || #  #  Multiple presses of the Suspend switch will reset Suspend timer
 || #
 || # February 28, 2012
 || # by Baruch Tabanpour
 */ 

#include <Keypad.h>
#include <Password.h>
#include <LiquidCrystal.h>
#include "pitches.h"

// Interlock States
bool Active = false;
bool Trip = false;
bool Suspend = false;
bool Reset = false;

// Laser States
bool Laser1 = false;
bool Laser2 = false;
bool Laser3 = false;
bool Laser4 = false;

// Constants
const byte ROWS = 4; //four rows on Keypad
const byte COLS = 4; //four columns on Keypad

// Pin Definitions
const byte buzzerPin = A0;  //buzzer
    // Active, Suspend, and Trip LEDs below have two sets of LEDs on each pin
const byte Active_Pin = 10;  //Active LED
const byte Suspend_Pin = 11;   //Suspend LED
const byte Trip_Pin = 12;      //Trip LED
const byte Reset_Pin = 27;  //Reset LED on Channel Box
    // Laser Pins below have two sets of LEDs on each pin
const byte Laser1_Pin = 32;   
const byte Laser2_Pin = 33;
const byte Laser3_Pin = 34;
const byte Laser4_Pin = 35;
// Switches
const byte Switch_Enable_Pin = 36;    // To turn on/off Active State
const byte Switch_Suspend_Pin = 13;   // To turn on Suspend state     //might change this pin # since 13 is hooked to internal LED
const byte Switch_Reset_Pin = 22;     // Reset switch on channel box
const byte Switch_Laser4 = 23;
const byte Switch_Laser3 = 24;
const byte Switch_Laser2 = 25;
const byte Switch_Laser1 = 26;
const byte Switch_Authorize = 28;      // Authorize Keypad Suspend   //On = short, Off = open 
const byte Switch_Trip_Pin1 = 29;      // Magnetic Switches on Curtains
const byte Switch_Trip_Pin2 = 30;
const byte Switch_Trip_Pin3 = 31;

const byte Relay_Laser4 = 50;           // Relay pins
const byte Relay_Laser3 = 51;
const byte Relay_Laser2 = 52;
const byte Relay_Laser1 = 53;

/*LCD pins
4 -> pin 44
6 -> pin 45
14 -> pin 49
13 -> pin 48
12 -> pin 47
11 -> pin 46*/

// Variables //
// Keypad
char keys[ROWS][COLS] = {
  {'1','2','3','A'  },
  {'4','5','6','B'  },
  {'7','8','9','C'  },
  {'*','0','#','D'  }
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

// LCD Screen
LiquidCrystal lcd(44,45,46,47,48,49);

// Switches Debounce Time and State Change Detection ---- Reset, Suspend, & Lasers
int lastResetState = LOW;   // the previous reading
int lastSuspendState = LOW;
int lastEnableState = LOW;

// Laser Interlock states
int laser1State;
int lastLaser1State = LOW;
int laser2State;
int lastLaser2State = LOW;
int laser3State;
int lastLaser3State = LOW;
int laser4State;
int lastLaser4State = LOW;

// Authorization State
int authorizeState;

// 3 Magnetic Switches
int tripstate1 = HIGH;
int tripstate2 = HIGH;
int tripstate3 = HIGH;
int TRIPstate = HIGH;  //TRIP is a logic function taken from 3 tripstates

// Flags -- to prevent unnecessary looping in main loop()
bool trip_flag = false;  //In order for Trip once
bool suspend_flag = false;  //In order to Suspend Once
bool lcd_flag = 0;  //for LCD display TRIP function
bool lcd_flag1 = 0;  //for LCD diplay Authorize function
bool active_flag = 0; //for LCD display Authorize function

// Suspend Timing
unsigned long previousMillis = 0;   
unsigned long suspend_time = 30000;  //30 seconds
unsigned long interval = 1000;  //1 second between buzzer sounds
unsigned long previousMillis2 = 0;  //for suspend buzzer blink speed
unsigned int i = 0;

// Keypad Constructor
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// Password for Suspend
Password password = Password( "1234" );

// Laser Momentary Switches, Debounce Time = 100 ms
unsigned long lastDebounceTime_Laser1 = 0;  
unsigned long lastDebounceTime_Laser2 = 0; 
unsigned long lastDebounceTime_Laser3 = 0;
unsigned long lastDebounceTime_Laser4 = 0;
unsigned long debounceDelay = 100;    // the debounce time, increase if the output flickers

/***********************************************************************************************/
void setup(){
  Serial.begin(9600);
  // LEDs
  pinMode(buzzerPin, OUTPUT);
  pinMode(Active_Pin, OUTPUT);
  pinMode(Trip_Pin, OUTPUT);
  pinMode(Suspend_Pin, OUTPUT);
  pinMode(Reset_Pin, OUTPUT);
  pinMode(Laser1_Pin, OUTPUT); 
  pinMode(Laser2_Pin, OUTPUT); 
  pinMode(Laser3_Pin, OUTPUT); 
  pinMode(Laser4_Pin, OUTPUT);
  pinMode(Relay_Laser4, INPUT);
  pinMode(Relay_Laser3, INPUT);
  pinMode(Relay_Laser2, INPUT);
  pinMode(Relay_Laser1, INPUT); 

  // Switches
  pinMode(Switch_Reset_Pin, INPUT);
  pinMode(Switch_Suspend_Pin, INPUT);
  pinMode(Switch_Trip_Pin1, INPUT);
  pinMode(Switch_Trip_Pin2, INPUT);
  pinMode(Switch_Trip_Pin3, INPUT);
  pinMode(Switch_Laser4, INPUT);
  pinMode(Switch_Laser3, INPUT);
  pinMode(Switch_Laser2, INPUT);
  pinMode(Switch_Laser1, INPUT);
  pinMode(Switch_Authorize, INPUT);
  pinMode(Switch_Enable_Pin, INPUT);
  
  keypad.addEventListener(keypadEvent); //add an event listener for this keypad
  //keypad.setHoldTime(3000);    //Keypress hold set to 3.0 sec
  
  // LCD
  lcd.begin(16, 2);  // set the LCD's number of columns and rows
}

void loop(){

  // Enable will switch the Active State ON or OFF 
  int enable_reading = digitalRead(Switch_Enable_Pin);  // Enable is an ON/OFF switch
  
  // Get Reset and Suspend readings                        // Placed here since Reset and Suspend switches are On/Off
  int suspend_reading = digitalRead(Switch_Suspend_Pin);   //   therefore the last_state must always be stored even when Active mode is Off
  int reset_reading = digitalRead(Switch_Reset_Pin);       //   since we are detecting a state change
  
  if( millis() < 100 )  { // for reboots
    lastResetState = reset_reading;       // Get button initiailization state so that reboot 
    lastSuspendState = suspend_reading;   //   does not turn any functions On accidentally
    lastEnableState = enable_reading;
  }
  
  // Keypad keys
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
  }
  
  ///////////////////////////////////INTERLOCK PROGRAM MAIN//////////////////////////////////////
  // Interlock System always running in this loop...

  // Get Active State
  if ( enable_reading != lastEnableState)  {
    if( Active == false)  {
      Active = true;
      Serial.println("Active ON");
      digitalWrite(Active_Pin, HIGH);  // Illuminated Sign is ON
      
      lcd.clear();
      lcd.print("Active: ON");
      active_flag = true;
      
      delay(10);                       // eliminates debounce switching
    }
    else  if (Active == true && Trip==false && Suspend == false && Reset == false) {
      Active = false;
      Serial.println("Active OFF");
      digitalWrite(Active_Pin, LOW);
      
      lcd.clear();
      lcd.print("Active: OFF");
      
      delay(10);                       // eliminates debounce
    }
  }// end Get Active State

  if( Active == false)  {
    // Do nothing...Interlock is 'OFF'
  }
  else if(Active == true)  {
    // System can Trip, Suspend, Reset, and Lasers can be turned ON/OFF
    
    // Listen for system Trip
    tripstate1 = digitalRead(Switch_Trip_Pin1);
    tripstate2 = digitalRead(Switch_Trip_Pin2); 
    tripstate3 = digitalRead(Switch_Trip_Pin3);

    // SET Trip
    if( tripstate1 == LOW || tripstate2 == LOW || tripstate3 == LOW)  { // 1 when switch connected, 0 when disconnected
      TRIPstate = LOW;
    }
    else {
      TRIPstate = HIGH;
    }
    if (TRIPstate == HIGH)  {
        Trip = false;
        digitalWrite(Trip_Pin, LOW);  // LED for Trip is LOW if curtain is closed correctly
        trip_flag = false;
        
        // Set Flag for LCD display Trip function
        if(lcd_flag == 0)  {
           lcd.setCursor(0,1);
           lcd.print("                ");
           lcd_flag = 1;
        }
     }
     else{
         Trip = true; 
         digitalWrite(Trip_Pin, HIGH);    // Curtain not closed correctly, LED is HIGH
         if( Suspend != true )  {
           tone(buzzerPin, NOTE_A5, 10);  // Alarm sounds when not in Suspend state and curtain is Open
         }
     }
       
    // Get Authorization state
    authorizeState = digitalRead(Switch_Authorize);    // To Authorize keypad Suspends
    if(active_flag == true)  {
      if(authorizeState == true)  {
        lcd_flag1 = 0;
      }
      else  {
        lcd_flag1 = 1; 
      }
      active_flag = false;
    } 
    if(authorizeState == true && lcd_flag1 == 0)  {
       // Display on LCD
       lcd.setCursor(0,0);
       lcd.print("Not Authorized  ");
       lcd_flag1 = 1;
    }
    else if(authorizeState == false && lcd_flag1 == 1)  {
      lcd.setCursor(0,0);
      lcd.print("Active: ON      ");
      lcd_flag1 = 0;
    } // end Authorization State
    
    // Get Suspend button press, and switch Suspend State ON if conditions met
    if( suspend_reading != lastSuspendState && suspend_flag == false && Trip == false )  {
      Suspend = true;
      suspend_flag = true;              // to prevent looping the this 'if statement' for entire Suspend state
      digitalWrite(Suspend_Pin, HIGH);  // Suspend LED is ON
      Serial.println("Suspend ON");
      
      // Set Suspend timer
      previousMillis = millis();
      previousMillis2 = millis();
      i = 0; //set timer to 0
      interval = 1000;
      
      lcd.setCursor(0,1);
      lcd.print("Suspend: ON     ");
      
      delay(10); //prevents debounce on Suspend On/Off Switch
    }
    else if(suspend_reading != lastSuspendState && suspend_flag == true && Suspend == true)  {
      // Reset Suspend timer
      previousMillis = millis();
      previousMillis2 = millis();
      i = 0;            // set timer to 0
      interval = 1000;
      lcd.setCursor(0,1);
      lcd.print("Suspend: ON     ");
      delay(10);
    }
    else if( suspend_reading != lastSuspendState && suspend_flag == false && Trip == true)  {
      // Must Clear TRip before Suspending
      Serial.println("Must Clear trip!");
      lcd.setCursor(0,1);
      lcd.print("Must Clear trip!");
      delay(10);
    }
    else{}      // end Get Suspend State
    
    // Debounce Timer Initializing for Laser Momentary Switches
    int laser1_reading = digitalRead(Switch_Laser1);
    int laser2_reading = digitalRead(Switch_Laser2);
    int laser3_reading = digitalRead(Switch_Laser3);
    int laser4_reading = digitalRead(Switch_Laser4);

    // Laser1
    if (laser1_reading != lastLaser1State) {
      // reset the debouncing timer
      lastDebounceTime_Laser1 = millis();
    } 
    // Laser2
    if (laser2_reading != lastLaser2State) {
      // reset the debouncing timer
      lastDebounceTime_Laser2 = millis();
    } 
    // Laser3
    if (laser3_reading != lastLaser3State) {
      // reset the debouncing timer
      lastDebounceTime_Laser3 = millis();
    } 
    // Laser4
    if (laser4_reading != lastLaser4State) {
      // reset the debouncing timer
      lastDebounceTime_Laser4 = millis();
    } 

    // Get states
    // Laser1State
    if ((millis() - lastDebounceTime_Laser1) > debounceDelay) {
      laser1State = laser1_reading;
    }
    //Laser2State
    if ((millis() - lastDebounceTime_Laser2) > debounceDelay) {
      laser2State = laser2_reading;
    }
    //Laser3State
    if ((millis() - lastDebounceTime_Laser3) > debounceDelay) {
      laser3State = laser3_reading;
    }
    //Laser4State
    if ((millis() - lastDebounceTime_Laser4) > debounceDelay) {
      laser4State = laser4_reading;
    }
    // Now laser_n_State can be used in program

      
      //******************Main Trip and Suspend Functions************************//
      if( Trip == true && Suspend == false && trip_flag == false )  {
        // Trip system
        
        // TRIP Actions
        Reset = false;
        digitalWrite(Reset_Pin, LOW);
        Serial.println("Reset Off");
        
        // Interlock Lasers
        Laser1 = false;
        digitalWrite(Laser1_Pin, LOW); //OPEN INTERLOCK PINS FOR LASER 1
        digitalWrite(Relay_Laser1, LOW);  //open 
        
        Laser2 = false;
        digitalWrite(Laser2_Pin, LOW);
        digitalWrite(Relay_Laser2, LOW);  //open 
        
        Laser3 = false;
        digitalWrite(Laser3_Pin, LOW);
        digitalWrite(Relay_Laser3, LOW);  //open 
        
        Laser4 = false;
        digitalWrite(Laser4_Pin, LOW);
        digitalWrite(Relay_Laser4, LOW);  //open //Only Laser 4 relay has been connected so far
                            //Tested with Laser Shutter and it works
        
        // Display Trip
        Serial.println("TRIP");
        lcd.setCursor(0,1);
        lcd.print("----**TRIP**----");
        lcd_flag = 0;
        
        // Reset for next Trip
        trip_flag = true;  // prevents looping of this "if-statement"
        Trip=false;
      }
      else if( Suspend == true)  {  // Can only Suspend if Trip was false, see Suspend intialization above
        // Suspend for 30 seconds
         unsigned long currentMillis = millis();  //for suspend time and buzzer blink
         unsigned long currentMillis2 = millis();  //for buzzer blink
         
         if(currentMillis2 - previousMillis2 > interval) {
               // 'interval' seconds passed
            i += 1;
            // Serial.println(i);  //for testing
            // Serial.println(interval);
            // Print Seconds on LCD
            lcd.setCursor(0,1);
            lcd.print("Suspend: ON  ");
            lcd.print(30-(currentMillis2-previousMillis)/1000);
            // reset buzzer blink timer
            previousMillis2 = currentMillis2;   
            
            // Buzzer blink will speed up before Suspend is turned off
            if( i>23 && i<31)  interval = 500;  // indicates last 7 seconds
            else if( i>=31) interval = 250;     // indicates last 3 seconds
            
            // Buzzer Beeps
            if (i<40) tone(buzzerPin, 440, 100);
            else  tone(buzzerPin, 523, 1000);    // 1 second tone when Suspend turns off
         }
  
         if(currentMillis - previousMillis > suspend_time) {
             // 30 seconds passed 
             previousMillis = currentMillis;
             Suspend = false; //For next run through
             digitalWrite(Suspend_Pin, LOW);
             Serial.println("Suspend OFF");
             suspend_flag = false;
             
             lcd.setCursor(0,1);
             lcd.print("Suspend: OFF    ");
         }
      } // end Trip and Suspend functions
      
     // Reset and Laser Buttons to turn On/Off Laser Interlock Pins
     if(Trip==false || Suspend == true)  {
       // Set Reset
       if(reset_reading != lastResetState)  {
         if( Reset == false )  {  
          Reset = true;
          digitalWrite(Reset_Pin, HIGH);
          Serial.println("Reset On");
          delay(10);  // to prevent debounce
         }
        else if( Reset == true )  {
          Reset = false;
          digitalWrite(Reset_Pin, LOW);
          Serial.println("Reset Off");
          delay(10);
          
          // Reset Laser Interlocks
          Laser1 = false;
          digitalWrite(Laser1_Pin, LOW); //OPEN INTERLOCK PINS FOR LASER 1
          
          Laser2 = false;
          digitalWrite(Laser2_Pin, LOW);
          
          Laser3 = false;
          digitalWrite(Laser3_Pin, LOW);
          
          Laser4 = false;
          digitalWrite(Laser4_Pin, LOW);
          digitalWrite(Relay_Laser4, LOW);  //open 
          
          // LCD
          lcd.setCursor(0,1);
          lcd.print("                ");
        }
      }
      if( Reset == true )  {
        // Short Interlock wires if Reset is ON and Laser button pressed
      
          if( laser1_reading == HIGH && Laser1 == false)  {
            Laser1 = true;
            digitalWrite(Laser1_Pin, HIGH); //SHORT INTERLOCK PINS FOR LASER 1 OUTPUT
            lcd.setCursor(0,1);
            lcd.print("Laser ON         ");
          }
          
          if( laser2_reading == HIGH && Laser2 == false)  {
            Laser2 = true;
            digitalWrite(Laser2_Pin, HIGH); //SHORT INTERLOCK PINS FOR LASER 2 OUTPUT
            lcd.setCursor(0,1);
            lcd.print("Laser ON         ");
          }
          
          if( laser3_reading == HIGH && Laser3 == false)  {
            Laser3 = true;
            digitalWrite(Laser3_Pin, HIGH); //SHORT INTERLOCK PINS FOR LASER 3 OUTPUT
            lcd.setCursor(0,1);
            lcd.print("Laser ON         ");
          }
          
          if( laser4_reading == HIGH && Laser4 == false)  {
            Laser4 = true;
            digitalWrite(Laser4_Pin, HIGH); //SHORT INTERLOCK PINS FOR LASER 4 OUTPUT
            digitalWrite(Relay_Laser4, HIGH);  //closed 
            lcd.setCursor(0,1);
            lcd.print("Laser ON         ");
          }
      }
   } // end if Trip == false

  // save last Laser Switch reading for Debounce Timing
  lastLaser1State = laser1_reading;
  lastLaser2State = laser2_reading;
  lastLaser3State = laser3_reading;
  lastLaser4State = laser4_reading;
  
  } // end if Active
  
  // save last reading for Reset, Suspend, and Enable for State Change Detection
  lastEnableState = enable_reading;
  lastResetState = reset_reading;
  lastSuspendState = suspend_reading;
}
// end loop()
/***********************************************************************************************/

// Event Listener
void keypadEvent(KeypadEvent key){
  switch (keypad.getState()){
    case PRESSED:
      switch(key){
        case '*': 
          if( Active == true)  {
            guessPassword(); 
          }
          break;
        case '#':
          if(Active == true)  {
            if(Suspend == true)  {
              //Reset Suspend timer
              previousMillis = millis();
              previousMillis2 = millis();
              i = 0; //set timer to 0
              interval = 1000;
              lcd.setCursor(0,1);
              lcd.print("Suspend: ON     ");
            }
          }
          else{ password.append(key); }
          break;
        default:
          if(Active == true)  {
            password.append(key); 
            tone(buzzerPin,440,100);  // tone when key is pressed
          }
      }
     break;
  }
}

// Password Functions
void guessPassword(){
  if( authorizeState == false)  {
    if (password.evaluate()){
      // password entered correctly. Can only turn On/Off Suspend if Trip is Cleared
          if( Trip == false && Suspend == false)  {
            // Turn on Suspend  
            Suspend = true;
            suspend_flag == true;
            digitalWrite(Suspend_Pin, HIGH);
            previousMillis= millis();
            Serial.println("Suspend ON");
            tone(buzzerPin, 440, 500);
            i=0;  //set timer to 0
            interval = 1000;
            
            lcd.setCursor(0,1);
            lcd.print("Suspend: ON    ");
          }
          else if(Trip == false && Suspend == true)  {
            // Turn off Suspend
            Suspend = false;
            digitalWrite(Suspend_Pin, LOW);
            Serial.println("Suspend OFF");
            suspend_flag = false;
            tone(buzzerPin, 440, 500);
            
            lcd.setCursor(0,1);
             lcd.print("Suspend: OFF    ");
          }
          else {  // Password2 Error
            Serial.println("Pswrd2 Error :: Must Clear Trip");
            tone(buzzerPin, NOTE_AS3, 500);
            
            lcd.setCursor(0,1);
             lcd.print("Pswd Error      ");
          }
        }
    else  {  // else if password not correct
      Serial.println("Password Incorrect");
      tone(buzzerPin, NOTE_AS3, 500);
      
      lcd.setCursor(0,1);
      lcd.print("Pswd Error      ");
    }
  }
  else   {
    Serial.println("Not Authorized to Enter:::Contact Personnel");
    tone(buzzerPin, NOTE_AS3, 500);
    lcd.setCursor(0,1);
    lcd.print("Not Authorized  ");
  }

  password.reset(); 
}

