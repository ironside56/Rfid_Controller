//#################################################################################################################
// RFID_LOCK V1.1.10a
//#################################################################################################################
/* 
  Copyright (c) 2020, geeb450@gmail.com
  All rights reserved.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  I NCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 

  
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  


 This program will interface with an RFID keypad supporting Wiegand W26 protocol, to provide more flexability in 
 controlling a door lock solenoid.  This system will support three door RFID keypads and allow a user to unlock 
 the front door, rear door or open a garage door by RFID tag or through the keypad. This program will mimic a 
 stanadard access control system allowing the user to add,delete RFID tags, and keypad entries, set the unlock 
 duration time and log user tracfic in real time. The access control system will provide feed back using the 
 keypad's backlight display, buzzer and optional status LEDs.  A CLI (Command Line Interface is also available 
 throught the USB port. Total of 100 users tags/passwordscan be stored  using the Arduino UNO and 200 users 
 using the ATMega 2650.
*/

// Operation:
//    The default programming password (PPWD) is 666666. The default user password (APWD) is 123456.
//
//      - NORMAL OPERATING MODE.
//        When in normal operating mode the user must press any key to activate the keypad. Note this keypress will
//        not be registeed by the controller and will be ignored. The access controller will respond
//        by turning on the keypad's backlight.
//        Subsequent keypresses will be recorded and monitored. When a valid 4-6 digit password is entered followed
//        by the # key the keypad will respond with a silgle long beep and the door assigned to that user will 
//        unlock. If an invalid password is entered the keypad will respond with 3 short "beeps". If an icorrect 
//        key is pressed, the last keypress can be erased by pressing the "*" key. Pressing the "*" key twice,
//        clears the complete paswword entered. If a registered RFID tag is presented to the keypad, the keypad 
//        will respond with a long beep. An unregistered RFID tag will cause the keypad to respond with 3 short
//         beeps.
//
//      - PROGRAMMING MODE. 
//        The programming mode is initiated by the *[PPWD]#. Note if no key is pressed after 30 seconds, the access 
//        controller will exit programmming mode and resume normal operation. to exit programming mode press the 
//        * key twice.
//        The following options are available in programming mode.
//
//        Add user password.
//        *,[PPWD],#,10,#,{4-6 digit password},#, 
//
//        COMPILED USING ARDUINO IDE REV 1.8.19 ON AN MEGA2560
/*
The Access controller can also be controlled via the serial port. Here are a list of commands:

rvb or RVB      Reads/display all values and parameters.
rar or RAR      Reads/display the maximum keypad retry count.
rkt or RKT      Reads/display the keypad timeout delay, Default = 30 seconds.
rud or RUD      Reads/display the unlock delay. Default = 5 seconds.
rgt or RGT      Reads/display the garage door lock timer. Default = 30 minutes.
rkp or RKP      Reads/displays the keypad data.
rdi or RKP      Reads/displays the Database user id for a given user number.
                If user 999 is entered,all user id and names are displayed.
rdn or RDN      Reads/displays the user name for a give id in the database.
rle or RLE      Reads/display the last error code recorded.
rep or REP      Displays all internal EEPROM contents.

svb or SVB      Turn ON/ continuous verbose monitoring every second to serial port.
sar or SAR      Set the lock retry count, default = 3.
skt or SKT      Set the keypad timeout, Default - 30 seconds.
slk or SLK      Set the unlock delay. Default = 5 seconds.
sgt or SGT      Set the garage door lock timer. Default = 30 minutes.
adi or ADI      Adds user id number in database.
adp or ADP      Adds user password in database.
ain or AIN      Adds user name in database for a given id number.
apn or APN      Adds user name in database for a given password number.
ada or ADA      Adds user permission in database for a given id number or password.
adt or ADT      Adds time stamp for user.
ddi or DDI      Deletes user id number in database.
ddp or DDP      Deletes user password in database.
din or DIN      Deletes user name in database for a given id number.
dpn or DPN      Deletes user name in database for a given id number.
dda or DDA      Deletes user permission byte in database for a given id number or password.
cdb or CDB      Clears user database (ID Tags, passwords, user names and attributes_.
cep or CEP      Clears All of EEPROM including database and all configuration settings.

menu or MENU or ?     Displays menu (commands).
*/  

// Rev 1.00   - Created to run on Arduino Arduino NANO. Compliled with Arduino IDE rev 1.8.12.
// Rev 1.01   - Added Weigard test code to normal mode operation.
// Rev 1.02   - Corrected printBits method tyo show MSB.
//            - Removed LED FLASH method. (overkill for this application}).
// Rev 1.03   - Changed EEPROMEX library for standard Arduino EEPROM for compatability with rfidb library.
//            - Added addDbId, delDbId, addDbNam, delDbNam serial commands.
//            - Fixed most serial commands to function correctly.
// Rev 1.04   - Fixed adi command.
//            - Fixed argNum function.
//            - Added "#" entry termination detection to argNum function for keypad entry.
//            - Bug fixes.
// Rev 1.0.5A - Changed idTag variable from global to local variable to save memory.
//            - Added chkKeypad method to scan for ID enrty during run mode.
// Rev 1.0.6  - Added permission method.
//            - Inserted Admin and programming password when "cdb" command is executed.
//            - Completed UNLOCK method.
//            - Added support for user timestamp.
// Rev 1.0.6A - Updated HELP menu.
// Rev 1.0.6B - Corrected procAtt method.
// Rev 1.0.6C - Test moving char strings to PROGEM.
//            - Fixed userInfo method to print fixed numberr length for ID, PASSWORD, TIMESTAMP.
//            - Changed userInfo method to show attribute in binary format. 
// Rev 1.0.7  - Corrected number of leading zeros for ID,password, and Timestamp in RDI command.
// Rev 1.0.8  - Changed I/O pinout.
//            - Melody now plays only on Mega 2560 boards. Not enough room in the Mega 328P boatrds.
//            - Fixed "printDecimal" function which was not producing the correct number of leading
//              zeros when id or password was "0".
//            - Added support for Mega2560 to handle 2 addition access control keypads.
// Rev 1.0.8B - Fixed chkKeypad function to pass keypad data only after "#" was pressed.
// Rev 1.0.8C - Added timer for garage door to close if left open.
//            - Fixed readGarDrTmr function.
//            - Fixed setGarDrTmr function.
//            - Removed duplicate garage door open/close message.
// Rev 1.0.8D - Fixed adding id/passwords through keypad or terminal.
//            - Fixed frtDrLckDlyTmr function to properly display value.
//            - Added beeper control to pin 8.
//            - Moved pin assignment for rear and shed lock so it could be controller by an UNO.
// Rev 1.1.0  - Added feature to close garage door with the "#" key when door is open.
//            - EEPROM size now changes with board detection.
// Rev 1.1.1	- Added Garage switch input to control garage relay. Note garage door signal is a 2.5Vdc
//              signal with a 8msec period and a 3usec negative going pulse. The signal is therefore
//              through a relay to provide universal garage door control. The garage door switch is 
//              connected to the Arduino input so the garage door can alslo be open via the intercom
//              AUX button.
//            - Added intercom unlock and auxiliary buttons. Also added garage door wall switch.
//            - Added RFID bell button for front, garage and rear door.
//            - Corrected 10Hz timer default value.
//            - Updated the serial commands listed in the comments at beginning of this program.
// Rev 1.1.2  - Added button monitor routine.
//            - Fixed compile errors from 1.1.1.
//            - Moved Pins 5, 25, 8, 29 to 36, 38, 40, 42 respectively for Mega2560
// Rev 1.1.3  - Moved all three keypads to pins 2,3.
//            - Added keypad detection pins 11,12,13.
// Rev 1.1.4  - Added JC_Button library to manage all switch inputs except encoder button.
//            - Added Adafruit SSD1306 OLED library to control display.
//            - Added RESET command in serial monitor commands.
//            - Fixed issue with RFID tag not working. See notes below "wg.begin".
//            - Added a 100ms while loop while checking RTC. If RTC fails, error is indicated
//              on console and program continues.
//            - Fixed "sdt" command which was not setting year correctly.
//            - Commented out Blue LED flashing when PIR detected.
//            - Added Pin Change interrupt map in comments.
// Rev 1.1.5  - Work!Changed "keypd" location pin to a more meaningfull label "garKypad", etc.
//            - Added "rgs" (Read Garage Status) command.
//            - Added "rot" command (Read Oled Timer) Displays the amount of time the OLED
//              stay on after being detected by the PIR (Proximity InfaRred) detector.
//            - Added "sot" command to set the OLED on time when detected by PIR.
//            - Added DST routine to automatically update RTC to DST or standard time.
//            - Modified "unlockDoor" method to accept either id Tag with permission
//              to open the garage door when placed on the garage door keypad or
//              a password configured with NO id Tag with garage door permission
//              to open the garage door from any keypad.
//            - Replaced "drawText" method with "drawMsg" method. Also ported "drawHdr"
//              method from LPG detector.
//            - Removed the "drUnlockFlag" for front, rear and shed doors. The door timer
//              is used instead to determine if a door is open (unlocked).
//            - Renamed drUnlockFlag to garDrFlag.
//            - Corrected keypad beeper polarity. Now set to active low.
//            - Added Name of person who locks/unlocks doors to console status output.
//            - Added "rto" and "sto" commands to read and set RTC's temperature offset.
//            - Added Yes/No argument response for command line input.
//            - During EEPROM erase, progress of erase process is now shown on the console.
// Rev 1.1.6  - Fixed "ada" command which did not erase previous name entred from buffer.
//            - Moved attribute status of "RDI" command to make it easier to read.
// Rev 1.1.7  - Added temperature scale command.
//            - Added "begin" statements for button objects.
// Rev 1.1.8  - Changed garage door control so that timer would only function when garage door 
//              sensors are available. Otherwise we can only guess the door's position,
//              which could lead to a security risk if the soft state becomes out of sync.
//              If garage door position sensors are disabled using the "GDS" command, the
//              garage door timer is also disabled, and you cannot close the door using the 
//              "#" key on the keypad. You must scan a tag ID or enter a password to close
//              the garage door.
//            - Updated menu to show new commands and required parameters for each command.
//            - Updated "RGS" read garage status command.
// Rev 1.1.8a - Updated keypad lacation pins to add internal pullups so that the inputs will be
//              read correctly.
//            - The hall effect sensors used to detect the garage door position cannot supply
//              enough current to drive the relay module used to trigger ZONE 6 of the alarm system,
//              therefore, the relay module is connected to A7 and follows the status of the bottom 
//              (closed) garage door sensor.
//            - Removed GARDRLOCK bit from drUnlockFlag as it add no value. If the garage door sensors
//              are disabled, there is no reliable way of determining the garage door's position.
//            - Added GARDRLOCK bit back in for drUnlockFlag due to the "checkLocks" function no longer 
//              functioned correctly. Multiple "garDrCntl" funtions would occur until the garage door
//              sensors would change state. 
//            - Changed pin designation "almZone7Pin" to "almZone67Pin"
// Rev 1.1.8b - Moved garage door sensor function to button object.t
// Rev 1.1.8d - Same as REV 1.1.8b except added definition statements at beginning of code to help
//            - isolate the RFID keypad no longer respoding after 12-24hrs,
// Rev 1.1.9  - Garage door open function "GARAGE DOOR OPEN" relay to cycle 3 times, due to speed of
//              the program comapared to that of the door. Needed to simplify garage door control.
//            - Added Wiegand_V35_GB as a local file. This library only has one RFID port. Other
//              RFID readers are added on the same port through diodes.
//            - Changed Wiegand library attached here to see if it fixes the RFID keypad freeze
//              after 24-48 hrs.
//            - Note in menu too long. Split into two lines of text.
//            - Added Time/Date to Cosole messages when user entry is detected.
// Rev 1.2.0  - Added "modify" functions to Rfdb library to modify/change existing Tag Ids, passwords
//              user names.
// Rev 1.1.9b - Works!!! but mofify commands could commands could be improved. "adi", "adp", "ain", and "apn"
//              commands can be also used to modify existing idTags, passwords, names, etc. 
//            - Changed "Modify" methods in rfidDb library to find location in EEPROM based on user 
//              postion instead of Id or password.
//            - Fixed "WriteNam" method in RfDb library which would leave stray characters from previous name.
// Rev 1.1.9d - Updated rfIdDb library to v1.1.10. Now supports EEPROM parameter to calculate number of users.
//            - Updated "LOCK" and "UNLOCK" messages to better display it on the console.
// Rev 1.1.10 - Added status to OLED display's main menu when encoder knob is turned.
//            - Optimized code.
// Rev 1.1.10a- Replaced JC-Button library with EasyButton library in order to get garage door wall switch to function correctly.
// 
//  TIMER PINS
//  ==========
//          328P  MEGA      TIMER   ARDUINO
//  TIMERS  PINS  PINS      SIZE    FUNCTION
//  ======  ====  ====      =====   ==========================
//  0       5,6   4,13      8-Bit   delay(), millis(),micros()
//  1       9,10  11,12     16-Bit  SERVO'S 12-23, SPI
//  2       3,11  9,10      8-Bit   tone()
//  3       ----  2,3,5     16-Bit  SERVO'S 24-35
//  4       ----  6,7,8     16-Bit  --------
//  5       ----  44,45,46  16-Bit  SERVO'S 0-11

// INTERRUPT INPUT PINS
// ====================
//
// H= HIGH, L=LOW, R=RISE, F=FALL, C=CHANGE
//INT     328P  2560  328P      2560
//        PINS  PINS  FUNCTION  FUNCTION
//====    ====  ====  ========  ========
//INT0    2     2     HLRFC     HLRFC
//INT1    3     3     HLRFC     HLRFC
//INT2    --    21    --        HLRFC, SCL
//INT3    --    20    --        HLRFC, SDA
//INT4    --    19    --        HLRFC, RX1
//INT5    --    18    --        HLRFC, TX1
//

// PIN CHANGE INTERRUPTS
// =====================
// UNO
// PCINT0 D8 - D13 (PORT B : B0 - B5)
// PCINT1 A0 - A5  (PORT C : C0 - C5)
// PCINT2 D0 à D7  (PORT D : D0 à D7)
//
// MEGA 2560 
// PCINT0 PIN D53 - D50 (PCINT 0-3), D10 - D13 (PCINT 4-7)( PORT B : B0 - PB7)  
// PCINT1 PIN D15 - D14 (PCINT9-10), (PORT PJ0 : PJ1)  
// PCINT2 A8 - A15 (PCINT 16-23)(PORT K : K0 - K7)

// ARDUINO INTERRUPT PRIORITY ORDER
// ================================
//
// 1  Reset
// 2  External Interrupt Request 0  (pin D2)          (INT0_vect)
// 3  External Interrupt Request 1  (pin D3)          (INT1_vect)
// 4  Pin Change Interrupt Request 0 (pins D8 to D13) (PCINT0_vect)
// 5  Pin Change Interrupt Request 1 (pins A0 to A5)  (PCINT1_vect)
// 6  Pin Change Interrupt Request 2 (pins D0 to D7)  (PCINT2_vect)
// 7  Watchdog Time-out Interrupt                     (WDT_vect)
// 8  Timer/Counter2 Compare Match A                  (TIMER2_COMPA_vect)
// 9  Timer/Counter2 Compare Match B                  (TIMER2_COMPB_vect)
// 10 Timer/Counter2 Overflow                         (TIMER2_OVF_vect)
// 11 Timer/Counter1 Capture Event                    (TIMER1_CAPT_vect)
// 12 Timer/Counter1 Compare Match A                  (TIMER1_COMPA_vect)
// 13 Timer/Counter1 Compare Match B                  (TIMER1_COMPB_vect)
// 14 Timer/Counter1 Overflow                         (TIMER1_OVF_vect)
// 15 Timer/Counter0 Compare Match A                  (TIMER0_COMPA_vect)
// 16 Timer/Counter0 Compare Match B                  (TIMER0_COMPB_vect)
// 17 Timer/Counter0 Overflow                         (TIMER0_OVF_vect)
// 18 SPI Serial Transfer Complete                    (SPI_STC_vect)
// 19 USART Rx Complete                               (USART_RX_vect)
// 20 USART, Data Register Empty                      (USART_UDRE_vect)
// 21 USART, Tx Complete                              (USART_TX_vect)
// 22 ADC Conversion Complete                         (ADC_vect)
// 23 EEPROM Ready                                    (EE_READY_vect)
// 24 Analog Comparator                               (ANALOG_COMP_vect)
// 25 2-wire Serial Interface  (I2C)                  (TWI_vect)
// 26 Store Program Memory Ready                      (SPM_READY_vect)

#define RFID
#define ENCODER
#define COMMANDS
#define SOUND
#define CLOCK
#define OLEDDISPLAY
#define LCDDISPLAY
#define SWITCHES
#define RGBLED
#define PIR

/* ArduinoSerialCommand library modified as follows
   Serial.Command.h 
                    - Line 66 "SERIALCOMMANDBUFFER 20" changed to "SERIALCOMMANDBUFFER 16"
                      Command argument size includes NULL character.
                    - Line 67 "MAXSERIALCOMMANDS 35" changed to "MAXSERIALCOMMANDS 40"
                      to support the number of commands required.
                  
  SerialCommand.cpp
                    - Line 128 modified to ignore caps so cammands could be entered in upper or lower case.
                      This reduced the buffer size needed to support commands and arguments.
              WAS:    if (strncmp(token,CommandList[i].command,SERIALCOMMANDBUFFER) == 0)
              IS NOW: if (strncasecmp(token,CommandList[i].command,SERIALCOMMANDBUFFER) == 0)
*/ 

#define PROTOTYPE 0                               // Use code for prototype box used for the aquarium development.
#define REV "1.1.10a"

//uncomment the next line for debugging mode`
//#define DEBUG 1                                 // General debug information sent to PC.
//#define SERIALCOMMANDDEBUG

//#define F_CPU 16000000L                         // Sets the compiler to 16Mhz clock.


#if defined RFID
  #include "Wiegand.h"                              // Wiegand Rev 3.4 library for keypad (modified by Guy Bastien).
  #include "RfidDb.h"                               // https://www.arduinolibraries.info/libraries/rfid-db v1.1.1.
#endif

#if defined ENCODER
  #include <ClickEncoder.h>
#endif

#include <SerialCommand.h>                        // https://github.com/scogswell/ArduinoSerialCommand
#include <EEPROM.h>                               // Mega328P EEPROM (1KB), Mega2560 EEPROM (4KB).

#if defined SOUND
  #include "pitches.h"                              // Pitch data for each note. https://www.arduino.cc/en/Tutorial/BuiltInExamples/toneMelody
#endif

#if defined CLOCK
  #include <RTClib.h>                               // https://github.com/adafruit/RTClib
#endif

#if defined SWITCHES
//  #include <JC_Button.h>                            // https://github.com/JChristensen/JC_Button
  #include <EasyButton.h>
#endif

// Libraries below are used for OLED display.
#if defined OLEDDISPLAY
  #include <SPI.h>                                  // Built-in library.
  #include <Wire.h>                                 // Built-in library.
  #include <Adafruit_GFX.h>                         // https://github.com/adafruit/Adafruit-GFX-Library
  #include <Adafruit_SSD1306.h>                     // https://github.com/acrobotic/Ai_Ardulib_SSD1306#endif
  #include "Dialog_bold_11.h"
  #include <Fonts/FreeMonoBoldOblique12pt7b.h>        // Part of Adafruit GFX library.
  #include <Fonts/FreeMonoBoldOblique18pt7b.h>        // Part of Adafruit GFX library.
#endif

#if defined LCDDISPLAY
  #include <LiquidCrystal.h>
#endif
  
// PIN DEFINITIONS ------------------------------------------------------------------------------------------------
#if defined(__AVR_ATmega2560__)
  const uint8_t   vinPin          = A0;           // +12V VIN Monitoring input.
  const uint8_t   vccPin          = A1;           // +5V VCC Monitoring input.
  const uint8_t   garDrUpSwPin    = A4;           // Garage Door up position switch (0 = Closed).
  const uint8_t   garDrDnSwPin    = A5;           // Garage Door down position switch (0 = down).
  const uint8_t   almZone6Pin     = A7;           // Alarm ZONE 7 relay control for garage door status.
  const uint8_t   frtBelBtnPin    = A8;           // Front door RFID Access keypad Bell button.
  const uint8_t   garBelBtnPin    = A9;           // Garage door RFID Access keypad Bell button.
  const uint8_t   rearBelBtnPin   = A10;          // Rear door RFID Access keypad Bell button.
  const uint8_t   garDrWalSwPin   = A11;          // Garage Door Open/close switch (wall switch).
  const uint8_t   intAuxBtnPin    = A12;          // Intercom door unlock button (used to unlock front door).
  const uint8_t   intDrBtnPin     = A13;          // Intercom auxiliary button (used to open garage door).
  const uint8_t   encSwPin        = A14;          // Rotory encoder select switch (pushbutton).

  const uint8_t   D0APin          = 2;            // INT4,(RESERVED) Front door RFID D0 weigand signal (See setup).
  const uint8_t   D1APin          = 3;            // INT5,(RESERVED) Front door RFID D1 wiegand signal (See setup).
  const uint8_t   SdSelPin        = 4;            // (RESERVED) W5100 Ethernet controller SD card select.
  const uint8_t   frtLedPin       = 6;            // RFID Green LED OUTPUT (operation indication).
  const uint8_t   frtBprPin       = 7;            // Access keypad Beeper control OUTPUT.
  const uint8_t   spkrPin         = 9;            // Speaker ouput (Error/condition feedback).
  const uint8_t   W5100SelPin     = 10;           // (RESERVED) W5100 Ethernet controller select.
  const uint8_t   frtDrKeyPdPin   = 11;           // Keypad 1 location pin. Used to identify which keypad is active.
  const uint8_t   garDrKeyPdPin   = 12;           // Keypad 2 location pin. Used to identify which keypad is active.
  const uint8_t   rearDrKeyPdPin  = 13;           // Keypad 3 location pin. Used to identify which keypad is active.

//const uint8_t   txd2Pin         = 16;           // (RESERVED) TXD2 Transmit data UART2.
//const uint8_t   rxd2Pin         = 17;           // (RESERVED) RXD2 Receive  data UART2.
  const uint8_t   encAPin         = 18;           // Rotory encoder A input (INT3).
  const uint8_t   encBPin         = 19;           // Rotory encoder B input (INT2).

  const uint8_t   i2cSdaPin       = 20;           // (RESERVED) I2C SDA/INT1. for OLED and RTC.
  const uint8_t   i2cSclPin       = 21;           // (RESERVED) I2C SCL/INT0. for OLED and RTC.

  const uint8_t   garDrOpnPin     = 22;           // Garage Door Opener solenoid (OUTPUT).
  const uint8_t   pirPin          = 23;           // Infared detector to control OLED display.

  const uint8_t   garLedPin       = 24;           // Garage door RFID access keypad LED.
  const uint8_t   garBprPin       = 25;           // Garage door RFID access keypad BEEPER.

  const uint8_t   rearLedPin      = 26;           // Rear door RFID access keypad LED.
  const uint8_t   rearBprPin      = 27;           // Rear door RFID access keypad BEEPER.

  const uint8_t   shedDrLckPin    = 29;           // Shed Door Lock solenoid.
  const uint8_t   shedLedPin      = 30;           // Shed door RFID access keypad LED.
  const uint8_t   shedBprPin      = 31;           // Shed door RFID access keypad BEEPER.
  const uint8_t   pwrLedPin       = 32;           // SMD/Power LED.
  const uint8_t   rsPin           = 33;           // Lcd RS pin.
  const uint8_t   enPin           = 34;           // LCD EN pin.
  const uint8_t   bkLtPin         = 35;           // LCD backlight Pin
  const uint8_t   frtDrLckPin     = 36;           // Front door Lock solenoid.
  const uint8_t   rearDrLckPin    = 38;           // Rear door Lock solenoid.
  const uint8_t   frtBelPin       = 40;           // Front door BELL OUTPUT.
  const uint8_t   rearBelPin      = 42;           // Rear door BELL OUTPUT.
  const uint8_t   d4Pin           = 43;           // LCD Data 4 Pin.
  const uint8_t   d5Pin           = 44;           // LCD Data 5 Pin.
  const uint8_t   bLedPin         = 45;           // BLUE Status LED.
  const uint8_t   gLedPin         = 46;           // GREEN Status LED.
  const uint8_t   rLedPin         = 47;           // RED Status LED.
  const uint8_t   d6Pin           = 48;           // LCD Data 6 Pin.
  const uint8_t   d7Pin           = 49;           // LCD Data 7 Pin.
//const uint8_t   misoPin         = 50;           // (RESERVED) for SPI (W5100 Ethernet board).
//const uint8_t   moisPin         = 51;           // (RESERVED) for SPI (W5100 Ethernet board).
//const uint8_t   sckPin          = 52;           // (RESERVED) for SPI (W5100 Ethernet board).
//const uint8_t   ssPin           = 53;           // (RESERVED) for SPI (W5100 Ethernet board).

#elif defined(__AVR_AT90USB1286__)             // Teensy ++2.0 board.
  const uint8_t   frtDrKeyPdPin   = A1;           // Keypad 1 location pin. Used to identify which keypad is active.
  const uint8_t   garDrKeyPdPin   = A1;           // Keypad 2 location pin. Used to identify which keypad is active.
  const uint8_t   rearDrKeyPdPin  = A1;           // Keypad 3 location pin. Used to identify which keypad is active.
  const uint8_t   D0APin          = A1;           // INT4,(RESERVED) Front door RFID D0 weigand signal (See setup).
  const uint8_t   D1APin          = A1;           // INT5,(RESERVED) Front door RFID D1 wiegand signal (See setup).
  const uint8_t   garDrUpSwPin    = A1;           // Garage Door up position switch (0 = Closed).
  const uint8_t   garDrDnSwPin    = A1;           // Garage Door down position switch (0 = down).
  const uint8_t   almZone6Pin     = A1;           // Alarm ZONE 7 relay control for garage door status.
  const uint8_t   frtBelBtnPin    = A1;           // Front door RFID Access keypad Bell button.
  const uint8_t   garBelBtnPin    = A1;           // Garage door RFID Access keypad Bell button.
  const uint8_t   rearBelBtnPin   = A1;           // Rear door RFID Access keypad Bell button.
  const uint8_t   garDrWalSwPin   = A1;           // Garage Door Open/close switch (wall switch).
  const uint8_t   intAuxBtnPin    = A1;           // Intercom door unlock button (used to unlock front door).
  const uint8_t   intDrBtnPin     = A1;           // Intercom auxiliary button (used to open garage door).
  const uint8_t   frtLedPin       = A2;           // RFID Green LED OUTPUT (operation indication).
  const uint8_t   frtBprPin       = A2;           // Access keypad Beeper control OUTPUT.
  const uint8_t   garDrOpnPin     = A2;           // Garage Door Opener solenoid (OUTPUT).
  const uint8_t   garLedPin       = A2;           // Garage door RFID access keypad LED.
  const uint8_t   garBprPin       = A2;           // Garage door RFID access keypad BEEPER.
  const uint8_t   rearLedPin      = A2;           // Rear door RFID access keypad LED.
  const uint8_t   rearBprPin      = A2;           // Rear door RFID access keypad BEEPER.
  const uint8_t   shedDrLckPin    = A2;           // Shed Door Lock solenoid.
  const uint8_t   shedLedPin      = A2;           // Shed door RFID access keypad LED.
  const uint8_t   shedBprPin      = A2;           // Shed door RFID access keypad BEEPER.
  const uint8_t   frtDrLckPin     = A2;           // Front door Lock solenoid.
  const uint8_t   rearDrLckPin    = A2;           // Rear door Lock solenoid.
  const uint8_t   frtBelPin       = A2;           // Front door BELL OUTPUT.
  const uint8_t   rearBelPin      = A2;           // Rear door BELL OUTPUT.

  const uint8_t   spkrPin         = A4;           // Speaker ouput (Error/condition feedback).
  const uint8_t   pirPin          = A5;           // Infared detector to control OLED display.
  const uint8_t   vccPin          = A6;           // +5V VCC Monitoring input.
  const uint8_t   vinPin          = A7;           // +12V VIN Monitoring input.
  const uint8_t   i2cSclPin       = 0;            // (RESERVED) I2C SCL/INT0. for OLED and RTC.
  const uint8_t   i2cSdaPin       = 1;            // (RESERVED) I2C SDA/INT1. for OLED and RTC.
  const uint8_t   rxdPin          = 2;            // (RESERVED) UART RXD Pin.
  const uint8_t   txdPin          = 3;            // (RESERVED) UART TXD Pin.
  const uint8_t   rsPin           = 4;            // LCD Display RS pin.
  const uint8_t   enPin           = 5;            // LCD Display Enable.
  const uint8_t   pwrLedPin       = 6;            // SMD/Power LED.
  const uint8_t   d4Pin           = 7;            // LCD display Data 4 pin.
  const uint8_t   d5Pin           = 8;            // LCD display Data 5 pin.
  const uint8_t   d6Pin           = 9;            // LCD display Data 6 pin.
  const uint8_t   d7Pin           = 10;           // LCD display Data 7 pin.
  const uint8_t   bkLt            = 11;           // LCD display Data 4 pin.
  const uint8_t   encSwPin        = 12;           // Rotory encoder select switch (pushbutton).
  const uint8_t   encAPin         = 13;           // Rotory encoder A input (INT3).
  const uint8_t   encBPin         = 14;           // Rotory encoder B input (INT2).
  const uint8_t   rLedPin         = 15;           // RED Status LED.
  const uint8_t   gLedPin         = 16;           // GREEN Status LED.
  const uint8_t   bLedPin         = 17;           // BLUE Status LED.
#endif

// EEPROM LOCATION DEFINITONS -------------------------------------------------------------------------------------
  const uint16_t  EEPROMSIZE      = 4096;         // Total number of bytes for the internal EEPROM.

const uint16_t  eAddr             = 0x00;         // 0x000 Mega328P EEPROM Starting address
const uint16_t  eAddrSetMon       = (eAddr + 1);  // 0x001 EEPROM location for continuous verbose monitoring ON/OFF.
const uint16_t  eAddrDsplyTmr     = (eAddr + 2);  // 0x002 EEPROM location for OLED display ON timer.
const uint16_t  eAddrProgTime     = (eAddr + 3);  // 0x003 EEPROM location for the config. mode countdown timer.
const uint16_t  eAddrRetryCnt     = (eAddr + 4);  // 0x004 EEPROM location for keypad password retry count def. = 3.
const uint16_t  eAddrFrtDrLckTmr  = (eAddr + 5);  // 0x005 EEPROM location for unlock time, default = 5 seconds.
const uint16_t  eAddrRrDrLckTmr   = (eAddr + 6);  // 0x006 EEPROM location for unlock time, default = 5 seconds.
const uint16_t  eAddrShdDrLckTmr  = (eAddr + 7);  // 0x007 EEPROM location for unlock time, default = 5 seconds.
const uint16_t  eAddrBkLtTmr      = (eAddr + 8);  // 0x008 EEPROM location for keypad backlight on time, def.=10 sec.
const uint16_t  eAddrKeyTmr       = (eAddr + 9);  // 0x009 EEPROM location for key entry timeout, def.=10 sec.
const uint16_t  eAddrKpLckTm      = (eAddr + 10); // 0x00A EEPROM location for keypad lock-out time, def.= 5 minutes.
const uint16_t  eAddrErrCode      = (eAddr + 11); // 0x00B EEPROM location for  error code.
const uint16_t  eAddrGarDrTmr     = (eAddr + 12); // 0x00C EEPROM location for garage door timer.
const uint16_t  eAddrDst          = (eAddr + 13); // 0x00D EEPROM location for DST flag used with RTC.
const uint16_t  eAddrTOffset      = (eAddr + 14); // 0x00E EEPROM location for temperature offset for RTC reading.
const uint16_t  eAddrTemprScale   = (eAddr + 15); // 0x00F EEPROM location for Fahrenheit/Celsius scale selection.
const uint16_t  eAddrPpwd         = (eAddr + 16); // 0X010 EPPROM location for Programmer password.
const uint16_t  eAddrApwd         = (eAddr + 20); // 0X014 EPPROM location for Administrator password.
const uint16_t  eAddrGarDrSn      = (eAddr + 24); // 0X018 EPPROM location for garage door sensors.
const uint16_t  eAddrMenuTimeout  = (eAddr + 25); // 0X019 EPPROM location for menu timer.
const uint16_t  eAddrBkLtLed      = (eAddr + 26); // 0X01A EPPROM location for LCD backlight setting.
const uint16_t  eAddrLcdContr     = (eAddr + 27); // 0X01B EPPROM location for LCD contrast setting.

// DEFAULT CONSTANTS ----------------------------------------------------------------------------------------------
const bool      SETMONDEFAULT     = 0;            // Sets continuous verbose monitoring to serial port ON/OFF.
const bool      TEMPRSCALEDEFAULT = 1;            // TempScale 0 = Fahrenheit, 1 = Celsius.
const bool      DSTDEFAULT        = 0;            // DST setting: 0 = Standard time, 1 = DST, Default = 0.
const bool      GARDRSNDEFAULT    = 1;            // Garage door sensors: 0 = Disabled, 1 = Enabled, Default = 1.
const uint8_t   IDANDPWD          = B10000000;    // Attibute bit location for ID + password required.
const uint8_t   PERMACCESS        = B01000000;    // Attibute bit location for permanent access rights.
const uint8_t   TEMPACCESS        = B00100000;    // Attibute bit location for temporary access (time limited).
const uint8_t   ONETMACCESS       = B00010000;    // Attibute bit location for one time access rights.
const uint8_t   SHEDDRLOCK        = B00001000;    // Attibute bit location for shed door lock access.
const uint8_t   REARDRLOCK        = B00000100;    // Attibute bit location for rear door lock access.
const uint8_t   GARDRLOCK         = B00000010;    // Attibute bit location for garage door lock access.
const uint8_t   FRTDRLOCK         = B00000001;    // Attibute bit location for fornt door lock access.
const uint8_t   DSPLYTMRDEFAULT   = 10;           // Display on time, default = 10 sec.
const uint8_t   BKLTLEDDEFAULT    = 160;          // LCD backlight brightnes level.
const uint8_t   LCDCONTRDEFAULT   = 160;          // LCD contrast default level.
const uint8_t   MENUTIMEOUTDEFAULT = 10;          // Timeout before exiting menu 0 (default 10 seconds).
const uint8_t   PROGTIMERDEFAULT  = 30;           // Programming mode timer default time out = 30 sec.
const uint8_t   RETRYCNTDEFAULT   = 3;            // Number of password retrys before a lockout condition.
const uint8_t   FRTDRLOCKTMRDEFAULT = 5;          // Front door unlock time, default = 5 seconds.
const uint8_t   RRDRLOCKTMRDEFAULT  = 15;         // Rear door unlock time, default = 15 seconds.
const uint8_t   SHDDRLOCKTMRDEFAULT = 30;         // Shed door unlock time, default = 60 seconds.
const uint8_t   BKLTTMRDEFAULT    = 10;           // Backlight on timer, default = 10 seconds.
const uint8_t   KEYTMRDEFAULT     = 20;           // Keypad entry timer, default = 20 seconds. 
const uint8_t   KPLCKTMDEFAULT    = 5;            // Keypad lock-out time, default = 5 minutes. 
const uint8_t   TENHZTIMERDEFAULT = 12;           // Ten hertz time generator (12 X 8.333 MSEC). 
const uint8_t   ONEHZTIMERDEFAULT = 120;          // One hertz time generator (120 X 8.333MSEC).
const uint8_t   ONEMNTIMERDEFAULT = 60;           // One minute time generator (60 X 1HZ).
const uint8_t   ERRORCODEDEFAULT  = 0;            // Error code storage.
const uint8_t   GARDRTMRDEFAULT   = 30;           // Garage door close timer (30 minutes).
const int8_t    TEMPROFFSETDEFAULT= 0;            // RTC's temperature offset.
const uint8_t   DBUSERS           = 30;           // Number of users to be stored in the database.
const uint8_t   NAMELENGTH        = 11;           // Name length (including null character).
const uint16_t  DBSTART           = 32;           // 0x20 Location in EEPROM where database starts.
const uint32_t  APWD              = 123456;       // Default user password.
const uint32_t  PPWD              = 666666;       // Default programming password.
const uint8_t   NUMBER_OF_ITEMS   = 8;            // Number of items in the attribute list. 
const uint8_t   NUMBER_OF_PERMS   = 9;            // Number of items in the permissions list. 
const int       MAX_SIZE          = 55;           // Number of charaters in each list item.
const uint16_t  DEBOUNCE          = 400;          // Switch debounce time 50 milliseconds.
const uint32_t  HOLDTM            = 1000;         // Switch hold time. Default = 1000 milliseconds.
const uint8_t   LCDROW            = 4;            // LCD number of rows (20x4 LCD display).
const uint8_t   LCDCOL            = 20;           // LCD number of colums (20x4 LCD display.

// ATTRIBUTE LIST ITEMS -------------------------------------------------------------------------------------------
const char attList [NUMBER_OF_ITEMS] [MAX_SIZE] PROGMEM = 
{ 
  {"1 - FRONT DOOR ACCESS"},
  {"2 - GARAGE DOOR ACCESS"},
  {"3 - REAR DOOR ACCESS"},
  {"4 - SHED DOOR ACCESS"},
  {"5 - ONE TIME ACCESS CODE"},
  {"6 - TEMPORARY ACCESS CODE (TIME DELAY)"},
  {"7 - PERMANENT ACCESS CODE"},
  {"8 - ID TAG + PASSWORD REQUIRED"},
};

// ATTRIBUTE PERMISSION LIST --------------------------------------------------------------------------------------
const char attPerm [NUMBER_OF_PERMS] [MAX_SIZE] PROGMEM = 
{ 
  {"|||| ||||"},
  {"|||| ||||_____  FRONT DOOR LOCK"},
  {"|||| |||______  GARAGE DOOR LOCK"},
  {"|||| ||_______  BACK DOOR LOCK"},
  {"|||| |________  SHED DOOR LOCK"},
  {"||||__________  ONE TIME ACCESS CODE"},
  {"|||___________  TEMPORARY ACCESS CODE (TIME DELAY)"},
  {"||____________  PERMANENT ACCESS CODE"},
  {"|_____________  TAG ID + PASSWORD"},
};

// OTHER CONSTANTS ------------------------------------------------------------------------------------------------
const bool      FALSE             = 0;            // LOGICAL FALSE.
const bool      TRUE              = 1;            // LOGICAL TRUE.
const int16_t   PWDFLAG           = 0x1000;       // If "posOf" function return is >= 0x1000, then a pwd was found.
                                                  // If "posOf" function return is < 0x1000, then a id was found.
                                                  // If "posOf" funtion = -1, then no id or password was found.
const int16_t   PWDMASK           = 0xFFF;        // Removes password flag to provide true location of password.



// OLED MESSAGES IN PROGRAM MEMORY --------------------------------------------------------------------------------
const char      daysOfTheWeek[7][12] PROGMEM = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char      months[12][4] PROGMEM = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

const int maxMsgSize = 16;                        // Maximum number of characters in each message.
//const int maxMsgSize = 22;                        // Maximum number of characters in each message.
const int msgNumber = 10;                         // Number of messages in the array. 
const char oledMsg[msgNumber] [maxMsgSize] PROGMEM =
{
  "** RFID CONTROLLER **",    // 0 INIT
  "***   FIRMWARE   ***",     // 1 FIRMW
  "***  REV  1.1.10a  ***"    // 2 REV
};


//VARIABLES USED DURING INTERRUPTS --------------------------------------------------------------------------------
volatile bool     tenHzTick       = 0;            // Shows 10Hz timer has counted down to zero.
volatile bool     oneHzTick       = 0;            // Shows 1Hz timer has counted down to zero.
volatile bool     oneMnTick       = 0;            // Shows 1 minute timer has counted down to zero.
volatile bool     pirFlag         = 0;            // Shows when PIR detected a presents. Used to keep ISR running fast.
volatile uint8_t  frtDrLckDlyTmr  = 0;            // Front door unlock time delay in seconds.
volatile uint8_t  rrDrLckDlyTmr   = 0;            // Rear door unlock time delay in seconds.
volatile uint8_t  shdDrLckDlyTmr  = 0;            // Shed door unlock time delay in seconds.
volatile uint8_t  progTimer       = 0;            // Configuration counter timeout.
volatile uint8_t  garDrTmr        = 0;            // Garage door timer.
volatile uint8_t  keyPdLoc        = 0;            // Keypad location. 0=No keypad, 1=Frt Dr, 2=Gar Dr, 3=Rear Dr.
volatile uint8_t  tenHzTimer      = TENHZTIMERDEFAULT; 	// Delay for ten hertz timer needed for flash led routine 
  																		                  // (uses Timer1 interrupt).
volatile uint8_t  oneHzTimer      = ONEHZTIMERDEFAULT;  // Delay for Master timer for all delays (uses Timer1 interrupt).
volatile uint16_t oneMnTimer      = ONEMNTIMERDEFAULT;  // Delay for Master timer for all delays (uses Timer1 interrupt).
volatile uint32_t timeStmp        = 0;            // Delay for temporary unlock timer (counts in minutes).
volatile uint16_t intKypdFlag     = 0;            // Keypad interrupt flag.

// GLOBAL VARIABLE DEFINITIONS ------------------------------------------------------------------------------------
uint8_t       dowFlag             = 0;            // Used to keep track of the day of week. If same day, the date is not refreshed on OLED.
uint8_t       drUnlockFlag        = 0;            // Used to check status of door lock during UNLOCK/LOCK cycle.
uint8_t       runState            = 0;            // Indicates what mode we are in. 0 = normal, 1 = Programming mode.
uint8_t       ConfigTime          = 30;           // Programming mode time-out, default 30 seconds.
uint8_t       retryCnt            = 0;
uint8_t       keyTmr              = 20;           // Key entry timer. Is set to default when a key is pressed.
uint8_t       dsplyTmr            = DSPLYTMRDEFAULT;    // Display timer. OLED is on if timer > 0.
uint8_t       menuTimeout         = MENUTIMEOUTDEFAULT;
int8_t        encPosition         = 0;            // Current encoder postion read from encoder.
int8_t        oldEncPosition      = 0;            // Previous encoder position.
uint16_t      timer1_counter      = 0;            // timer1 counter variable.
uint32_t      keyVal              = 0;            // Holds the numeric information entered from the keypad (0-9).
uint32_t      KpLckTm             = 0;            // Keypad lockout time.
char          name[NAMELENGTH];                   // Temp location for input/output for id Name.
uint32_t      idPwd = 0;
int16_t       pos   = 0;
uint8_t       att   = 0;
  
// MELODY VARIABLE ------------------------------------------------------------------------------------------------
// notes in the melody: (CLOSE ENCOUNTER's OF THE THIRD KIND).
const int melody[] = {NOTE_G4, NOTE_A4,NOTE_F4, NOTE_F3, NOTE_C3};
const int noteDurations[] = {8, 4, 2, 4, 1}; // note durations: 4 = quarter note, 8 = eighth note, etc.

// ENUMERATION DEFINITIONS ----------------------------------------------------------------------------------------
enum RUNSTATE
{
  NORMAL,                                         // Indicates we are in normal operating mode.
  PROGRAM                                         // Indicates we are in configuration mode (sensor detection).
};

enum RUNMODE
{
  TMDT,                                           // Displays Time/Date (Main display page)
  VINVAL,                                         // Displays (+12V).
  VCCVAL                                          // Displays the board's 5VDC voltage.
};

enum CONFIGMODE                                   // Indicates the programing opsion selected.
{
  ADDTAG,
  DELTAG,
  ADDPWD,
  DELPWD
};

enum OPENCLOSE                                    // Garage door position switch status
{
  CLOSE,                                          // 0
  OPEN                                            // 1
};

enum LOCKUNLOCK                                   // Front/Rear door lock solenoid action.
{
  LOCK,                                           // 0 - Lock door solenoid (turned off).
  UNLOCK                                          // 1 - Unlock door solenoid (Activate).
};

enum ONOFF                                        // LED and other basic function requiring a logic "1" for ON.
{
  OFF,                                            // Led control. When off (deactivated), Red LED is on.
  ON                                              // Led control. When on (active), Green LED os on.
};

enum ONOFFINV                                     // LED and other basic function requiring a logic "0" for ON.
{
  ONINV,                                          // Led control. When on (deactived), Green LED os on.
  OFFINV                                          // Led control. When off (activated), Red LED is on.
};

enum
{
  NOBELL,
  FRTBEL,
  REARBEL,
  GARBEL
};

enum KEYPADLOC
{
  NOKEYPD,
  FRTDRKEYPD,
  GARDRKEYPD,
  REARDRKEYPD,
  SHEDDRKEYPD
};
  
// LCD DEFINITIONS ------------------------------------------------------------------------------------------------
#if defined LCDDISPLAY
  // LCD PINS       RS      EN      D4    D5    D6      D7
  LiquidCrystal lcd(rsPin,enPin, d4Pin, d5Pin, d6Pin, d7Pin);
#endif

// OLED DEFINITIONS -----------------------------------------------------------------------------------------------
#if defined OLEDDISPLAY
  #define OLEDADR       0x3C                        // OLED display I2C address.
  #define SCREEN_WIDTH  128                         // OLED display width, in pixels
  #define SCREEN_HEIGHT 32                          // OLED display height, in pixels
  #define OLED_RESET    -1                          // Disable reset pin/
#endif

//CREATE A NEW BUTTON OBJECT (PUSHBUTTON SWITCH DEFINITION)--------------------------------------------------------
//const uint32_t  LONG_PRESS(HOLDTM);                   // Time in millisec to press garage dr wall sw to disable close timer.

//     INSTANCE--PIN---------DEBOUNCETM PULLUP INVERT
#if defined SWITCHES
  EasyButton frtBelBtn(frtBelBtnPin, DEBOUNCE, TRUE, TRUE); // Instantiate a Bounce object
  EasyButton garDrWalSw(garDrWalSwPin, DEBOUNCE, TRUE, TRUE);
  EasyButton garDrDnSw(garDrDnSwPin,DEBOUNCE, TRUE, TRUE); // Switch is HIGH, door is open. Also used to resync GARDRLOCK flag.
  EasyButton garDrUpSw(garDrUpSwPin,DEBOUNCE, TRUE, TRUE); // Switch is HIGH, door is open. Also used to resync GARDRLOCK flag.
  EasyButton garBelBtn(garBelBtnPin, DEBOUNCE, TRUE, TRUE);
  EasyButton rearBelBtn(rearBelBtnPin,DEBOUNCE, TRUE, TRUE);
  EasyButton intDrBtn(intDrBtnPin,DEBOUNCE, TRUE, TRUE);
  EasyButton intAuxBtn(intAuxBtnPin,DEBOUNCE, TRUE, TRUE);
#endif


//CREATE A NEW DISPLAY OBJECT -------------------------------------------------------------------------------------
#if defined OLEDDISPLAY
  Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif
SerialCommand SCmd;                               // INITIALIZING CLI OBJECT.

#if defined RFID
  WIEGAND wg;

// RFID DATABASE SETUP --------------------------------------------------------------------------------------------
// Setup user database for 30 users with 10 character names starting at EEPROM location 48 (0x30)

  RfidDb db = RfidDb(DBUSERS, DBSTART, NAMELENGTH);	// Used to configure database with a fixed amount of users.
//  RfidDb db = RfidDb(EEPROMSIZE, DBSTART, NAMELENGTH);// Used to configure database with max EEPROM size.
#endif

//CREATE A NEW RTC OBJECT -----------------------------------------------------------------------------------------
#if defined CLOCK
  RTC_DS3231 rtc;
#endif

//CREATE A NEW ENCODER OBJECT -------------------------------------------------------------------------------------
#if defined ENCODER
  ClickEncoder *encoder;
#endif

//#################################################################################################################
// FUNCTION DECLARATION
//#################################################################################################################
// DECLARING MENU FUNCTIONS FIRST SO THE COMPLIER WILL WORK.
void      runMode();                              // Mode Control method.
void      progMode();                             // Programming Method.
bool      procAtt(uint8_t user);                  // Process attrbute.
uint32_t  chkKeypad();                            // Checks keypad for ID or password during run mode.
void      processId(uint32_t idVal);              // Process tag method.
uint32_t  getId();                                // Get id from keypad or RFID tag.
uint8_t   getType();                              // Get Wiegand type last scanned.
uint8_t   getKeypadId();                          // Get last active keypad location.
uint8_t   getLastErr();                           // Read last error code reported.
void      drawText(byte x_pos, byte y_pos, char *text, byte text_size); // Draw text message on OLED.
void      drawTmDt();                             // Draws time/date on OLED display.
void      drawVin();                              // Shows 12VDC VIN value on OLED display.
void      drawVcc();                              // Shows 5VDC VCC value on OLED display.
void      readVerbose();                          // Read all battery parametrs and set points.
void      readAcsCnt();                           // Read and display the last number of access retries.
void      readKpTmOut();                          // Read keypad Timeout setting.
void      readTime();                             // Reads and displays the time/date and temperature from RTC chip.
void      readTmDt();                              // Reads and displays the date and time on the console. 
void      readUnlckDly();                         // Read and display the Unlock delay time in seconds.
void      readGarDrTmr();                         // Read and display the garage door lock timer in minutes.
void      readDsplyTmr();                         // Read and display the OLED on timer.
void      readTOffset();
void      readGarStatus();                        // Reads the garage door position switches.
void      readKeypad();                           // Read/display information on last keypad/Tag scanned.
void      readLastErr();                          // Read and display the last error code logged.
void      readIdPwd();
void      readDbNam();
void      setMon();                               // Set verbose monitoring ON or OFF.
void      setAcsCnt();                            // Set access control panel retry count(Default = 3).
void      setKpTmOut();                           // Set keypad time out delay.
void      setUnlckDly();                          // Set unlock delay time, default = 5 seconds).
void      setGarSensor();                         // Set Enable/Disable garage door sensors, Default = Enabled.
void      setGarDrTmr();                          // Set garage dorr lock timer, default = 30 minutes).
void      setdsplyTmr();                          // Sets the OLED display ON timer after PIR detected.
void      setTime();                              // Sets the time and DST bit on the RTC chip.
void      setDate();                              // Sets the date on the RTC chip.
void      setTOffset();                           // Set RTC's temperature offset value ,default = 0.
void      setTemprScale();                        // Set temperature scale to Farhenheit or celsius ,default = Celsius.
void      addDbId();                              // Add an ID to the database.
void      addDbPwd();                             // Add a password to the database.
void      addDbIdNam();                           // Add a name in the database to an ID number.
void      addDbPwdNam();                          // Add a name in the database to a password.
void      addDbAtt();                             // Add an attribute to an ID or password.
void      addDbTm();                              // Add time stamp to user for temporary access.
void      delDbId();                              // Deletes user id/tag from the database. 
void      delDbPwd();
void      delDbNam();                             // Deletes user mane for a given id/tag from the database. 
void      delDbIdNam();
void      delDbPwdNam();
void      delDbAtt();
void      modDbId();
void      modDbPwd();
void      modDbIdNam();
void      modDbPwdNam();
//
bool      unlockDoor(int16_t user);               // Unlock solenoid for the duration of the unlock delay.
void      menu();                                 // Display commands.
int8_t    argOnOff();                             // Processes ON/OFF parameter from command input.
int8_t    argyn();                                // Processes YES/NO response from command line input.
int       argcf();                                // Gets "C" or "F" argument from console input.
bool      argNum(uint32_t &tagId);                // Gets id tag number and store it in variable tagId.
int16_t   argNumMinMax(uint16_t argMin, uint16_t argMax);  // Processes numerical parameter from command input.
int16_t   argAtt();
bool      argNam(char * name);
void      syntaxError();                          // Displays syntax error message.
void      missingArg();                           // Displays Missing Argument error method.
void      ledSelfTest();                          // Run led selftest.
void      bprSelfTest();                          // Run beeper selftest.
void      lckSelfTest();
void      printBits(uint32_t n, uint8_t numBits); // prints decimal number with leading zero's
void      printDecimal(uint32_t idPwdTm);
void      logErr(uint8_t errNum);                 // Displays error logs.
void      userInfo(uint8_t user);                 // Displays database record for user position.
void      eepromDump();                           // EEPROM Dump function.
void      displayAtt(uint8_t att);
void      printAttList (const char * str);
void      displayPerm ();                         // Displays permissions (attributes) diectly from Program Memory to save RAM.
void      checkLocks();                           // Locks all doors (except garage door).
void      garDrCntl();                            // Opens/closes garage door. 
void      ringFrtBel();                           // Rings frontt door bell if front or garage door RFID bell button is pressed.
void      ringRearBel();                          // Rings door bell according to door position.
void      help();                                 // Displays Help command.
void      checkBtn();                             // Checks all push button status. Must be run in the loop function.
void      startupTone();                          // Play startup melody.
void      errorTone();                            // Error tone.
bool      unlockFrtDr();                          // Unlocks front door strike.
bool      unlockRearDr();                         // Unlocks rear door strike.
bool      unlockShedDr();                         // Unlocks shed door strike.
void      drawDigits(int digits);                 // Adds leading "0" to time/date if needed.
void      printDigits(int digits);                // Used to print leading "0" to time/date if needed on serial console.
void      checkDst();                             // Check and adjusts RTC for DST or standard time.
//char      ynReply();                              // Returns Y/N response from console. Waits 10 senconds for input.
bool      ynReply();                              // Returns Y/N response from console. Waits 10 senconds for input.
void      clrDb();
void      clrConfig();                            // Clears configuration values in EEPROM.
void      clrEeprom();               
void      eraseEeprom(uint16_t eepromStart);      // Clears database.
void      keypadLoc();                            // Locates which keypad IdTag was read from.
void      dsplyMsg (const char * str);            // Takes text stored in program memory and displays it on OLED display.
void      (* resetFunc) (void) = 0;               // Declare reset function at address 0.


//#################################################################################################################
// SETUP AND INITIALIZATION
//#################################################################################################################
void setup()
{
  noInterrupts();                                 // disable all interrupts
  // INITIALIZE WIEGAND OBJECT ---------------------------------------------------------------------
  // CONFIGURING WIEGAND D0,D1 PINS WITH PULLUPS MUST BE DONE AFTER THE wg.begin STATEMENT AS THE WIEGAND LIBRARY
  // CONFIGURES THE INPUTS WITHOUT PULLUPS. INPUT PULLUPS ARE ONLY NEEDED WHEN CONNECTING MULTIPLE KEYPADS TO
  // THE SAME ARDUINO INPUT VIA STEERING DIODES.
  #if defined RFID
    wg.D0PinA     = D0APin;                       // Front door keypad Data 0 W26 signal.
    wg.D1PinA     = D1APin;                       // Front door keypad Data 1 W26 signal.
    wg.begin(TRUE);                               // enable 1 Reader (GateA)
  #endif
  
  // CONFIGURE I/O PINS -------------------------------------------------------------------------------------------
  // RFID RELATED OI/O PINS
  pinMode(D0APin,         INPUT_PULLUP);          // Wiegand D0 keypad input pin.
  pinMode(D1APin,         INPUT_PULLUP);          // Wiegand D1 keypad input pin.
  pinMode(garDrUpSwPin,   INPUT_PULLUP);          // Garage door up position switch detection.
  pinMode(garDrDnSwPin,   INPUT_PULLUP);          // Garage door down position switch detection.
  pinMode(frtDrKeyPdPin,  INPUT_PULLUP);          // Front door KeyPad location pin.
  pinMode(garDrKeyPdPin,  INPUT_PULLUP);          // Garage door KeyPad location pin.
  pinMode(rearDrKeyPdPin, INPUT_PULLUP);          // Rear door KeyPad location pin.
  pinMode(almZone6Pin,    OUTPUT);                // Controls 5V relay which handles the garage door status for the alarm zone7.
  pinMode(frtLedPin,      OUTPUT);                // Showns when door is open. Also shows config mode when flashing.
  digitalWrite(frtLedPin, OFFINV);                // Turns LED RED OFF (Deavitvated HIGH).
  pinMode(frtBprPin,      OUTPUT);                // keypad buzzer output.
  digitalWrite(frtBprPin, OFFINV);                // Turns front door beeper OFF (Deavitvated HIGH).
  pinMode(frtBelPin,      OUTPUT);                // Front door BELL OUTPUT.
  pinMode(frtDrLckPin,    OUTPUT);                // Front door lock control.
  digitalWrite(frtDrLckPin, OFF);                 // Lock front door.
  pinMode(garLedPin,      OUTPUT);                // Garage door access keypad led control.
  digitalWrite(garLedPin, OFFINV);                // Turns LED RED (Deavitvated HIGH).
  pinMode(garBprPin,      OUTPUT);                // Garage door access keypad beeper control.
  digitalWrite(garBprPin, OFFINV);                // Turns garage door beeper OFF (Deavitvated HIGH).
  pinMode(garDrOpnPin,    OUTPUT);                // Garage door lock control.
  pinMode(rearLedPin,     OUTPUT);                // Rear door access keypad led control.
  digitalWrite(rearLedPin,OFFINV);                // Turns LED RED (Deavitvated HIGH).
  pinMode(rearBprPin,     OUTPUT);                // Rear door access keypad beeper control.
  digitalWrite(rearBprPin, OFFINV);               // Turns rear door beeper OFF (Deavitvated HIGH).
  pinMode(rearDrLckPin,   OUTPUT);                // Rear door lock control.
  digitalWrite(rearDrLckPin, OFF);                // Lock front door.
  pinMode(rearBelPin,     OUTPUT);                // Rear door BELL OUTPUT.
  pinMode(shedDrLckPin,   OUTPUT);                // Shed door lock control.

  // BUTTON RELATED I/O PINS
  pinMode(frtBelBtnPin,   INPUT_PULLUP);          // Front door RFID Access keypad Bell button. Handled by EasyButton Library.
  pinMode(rearBelBtnPin,  INPUT_PULLUP);          // Library.Rear door RFID Access keypad Bell button.
  pinMode(garBelBtnPin,   INPUT_PULLUP);          // Front door RFID Access keypad Bell button.
  pinMode(garDrWalSwPin,  INPUT_PULLUP);          // Garage Door Open/close switch (wall switch). Handled by EasyButton Library.
  pinMode(intDrBtnPin,    INPUT_PULLUP);          // Library.Intercom door unlock button (used to unlock front door).
  pinMode(intAuxBtnPin,   INPUT_PULLUP);          // Intercom door unlock button (used to unlock front door).

  // SOUND RELATED I/O PIN
  pinMode(spkrPin,        OUTPUT);                // Speaker output.
    
  // RGB LED RELATED I/O PINS
  pinMode(rLedPin,        OUTPUT);                // Red status LED.
  pinMode(gLedPin,        OUTPUT);                // Green status LED.
  pinMode(bLedPin,        OUTPUT);                // Blue status LED.

  // ENCODER RELATED I/O PINS
  pinMode(encAPin,        INPUT);                 // Rotory Ecncoder "A" input.
  pinMode(encBPin,        INPUT);                 // Rotory Ecncoder "B" input.
  pinMode(encSwPin,       INPUT_PULLUP);          // Rotory Select switch (pushbutton).

  // PIR DETECTOR RELATED I/O PIN
  pinMode(pirPin,         INPUT);                 // PIR detector.

  // POWER LED RELATED I/O PIN
  pinMode(pwrLedPin,      OUTPUT);                // Power and smdLed pin to show program is running.

  // INITIALIZE TIMER1 --------------------------------------------------------------------------------------------
  // This sets up timer1 to overflow at a rate of once per second. 
  // See "ISR(TIMER1_OVF_vect)" method.

  TCCR1A = 0;
  TCCR1B = 0;
  //NON Inverted PWM
  TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);
  //PRESCALER=64 MODE 14(FAST PWM)
  TCCR1B|=(1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10);
                                                  // Freq/Prescale/Output Freq = ICR1 Value
  ICR1=2082;                                      // fPWM=120Hz (20msec). (16MHz/64)/120Hz=2083-1=2082
  TIMSK1 |= (1 << TOIE1);                         // enable timer overflow interrupt

  // INITIALIZE PIN CHANGE INTERRUPT FOR PORTB PINS D11, D12, D13 for keypad location detection -------------------
  PCICR |=  B00000001;                            // Bit0 = 1 -> "PCIE0" enabeled (PCINT0 - PCINT5)
  PCMSK0 |= B11100000;           // Bit2,3 = 1 -> "PCINT5,6,7" enabeled -> D11, D12, D13 will trigger an interrupt.
    
  // INITIALIZE ENCODER -------------------------------------------------------------------------------------------
  #if defined ENCODER
    //clickEncoder(A,B,Btn)
    encoder = new ClickEncoder(encAPin, encBPin, encSwPin);    //(Encoder-A,Encoder-B,Select Button)
  #endif
  
  interrupts();                                   // enable all interrupts
     
  #if defined SOUND
    noTone(spkrPin);                              // Turn off speaker output.
  #endif
  
  // INITIALIZE SERIAL CONSOLE ------------------------------------------------------------------------------------
  Serial.begin (115200);
  
  // INITIALIZE RFID DATABASE -------------------------------------------------------------------------------------
  #if defined RFID
    db.begin();
  #endif
  
  // INITIALIZE SWITCHES ------------------------------------------------------------------------------------------
  #if defined SWITCHES
    frtBelBtn.begin();
    garDrWalSw.begin();
    garBelBtn.begin();
    garDrDnSw.begin();
    garDrUpSw.begin();
    rearBelBtn.begin();
    intDrBtn.begin();
    intAuxBtn.begin();
    
    garDrWalSw.onPressedFor(HOLDTM, longPressGarDrWalSw);
    garDrWalSw.onPressed(singlePressGarDrWalSw);    
  #endif
  
  // INITIALIZE DEFAULT CONFIGURATION VALUES IN EEPROM ------------------------------------------------------------
  // USED TO RESET EEPROM SHOULD BE COMMENTED OUT DURING NORMAL OPERATION.
  //   EEPROM.write(eAddr,0xFF);// Used to reset EEPROM values (Run once to reset values).
  //   delay(10); // More writes will only give errors when _EEPROMEX_DEBUG is set.

  if(!(EEPROM.read(eAddr) == 0xAA))
  {
    EEPROM.write(eAddr, 0xAA);          // Test byte to see if data has been written to EEPROM at least once.
    delay(10);                          // More writes will only give errors when _EEPROMEX_DEBUG is set.
    EEPROM.write(eAddrSetMon, SETMONDEFAULT);     // Initialize verbose default setting (OFF).
    delay(10);
    EEPROM.write(eAddrDsplyTmr, DSPLYTMRDEFAULT); // Initialize OLED on timer (default = 10 seconds).
    delay(10);
    EEPROM.write(eAddrProgTime, PROGTIMERDEFAULT);// Initialize configuration timeout, (default = 30 seconds).
    delay(10);
    EEPROM.write(eAddrRetryCnt, RETRYCNTDEFAULT); // Initialize user ID retry count, (default = 3 retrys)
    delay(10);
    EEPROM.write(eAddrFrtDrLckTmr, FRTDRLOCKTMRDEFAULT);// Initialize unlock delay time, (default = 5 seconds).
    delay(10);
    EEPROM.write(eAddrRrDrLckTmr, RRDRLOCKTMRDEFAULT);  // Initialize unlock delay time, (default = 5 seconds).
    delay(10);
    EEPROM.write(eAddrShdDrLckTmr, SHDDRLOCKTMRDEFAULT);// Initialize unlock delay time, (default = 5 seconds).
    delay(10);
    EEPROM.write(eAddrBkLtTmr, BKLTTMRDEFAULT);   // Initialize keypad backlight on delay, (default = 15 seconds).
    delay(10);
    EEPROM.write(eAddrKeyTmr, KEYTMRDEFAULT);     // Initialize keypad time-out delay, (default = 20 seconds).
    delay(10);
    EEPROM.write(eAddrKpLckTm, KPLCKTMDEFAULT);   // Initialize keypad Lock delay, after retry count is exceeded (default = 5 minutes).
    delay(10);
    EEPROM.write(eAddrErrCode, ERRORCODEDEFAULT); // Initialize last errorcode, (default = 00).
    delay(10);
    EEPROM.write(eAddrGarDrTmr, GARDRTMRDEFAULT); // Initialize Garage door timer, (default = 30).
    delay(10);
    EEPROM.write(eAddrDst, DSTDEFAULT);           // Initialize DST. 0 = Standard time, 1 = DST, Default = 0.
    delay(10);
    EEPROM.write(eAddrTOffset, TEMPROFFSETDEFAULT);// RTC's temperature offset value.
    delay(10);
    EEPROM.write(eAddrTemprScale,TEMPRSCALEDEFAULT);
    delay(10);
    EEPROM.write(eAddrGarDrSn,GARDRSNDEFAULT);
    delay(10);
    EEPROM.write(eAddrMenuTimeout,MENUTIMEOUTDEFAULT);
    delay(10);
    EEPROM.write(eAddrBkLtLed,BKLTLEDDEFAULT);
    delay(10);
    EEPROM.write(eAddrLcdContr,LCDCONTRDEFAULT);
    delay(10);
  }

  // Setup callbacks for SerialCommand commands
  SCmd.addCommand("rvb", readVerbose);            // Reads/display all values and parameters.
  SCmd.addCommand("rar", readAcsCnt);             // Reads/display the maximum keypad retry count.
  SCmd.addCommand("rkt", readKpTmOut);            // Reads/display the keypad timeout delay, Default = 30 seconds.
  SCmd.addCommand("rud", readUnlckDly);           // Reads/display the unlock delay. Default = 5 seconds.
  SCmd.addCommand("rgs", readGarStatus);          // Reads/display the garage door position.
  SCmd.addCommand("rkp", readKeypad);             // Reads/displays the keypad data.
  SCmd.addCommand("rip", readIdPwd);              // Reads/displays the Database user id for a given user number.
                                                  // If user 999 is entered,all user id and names are displayed.
  SCmd.addCommand("rdn", readDbNam);              // Reads/displays the user name for a give id in the database.
  SCmd.addCommand("rle", readLastErr);            // Reads/display the last error code recorded.
  SCmd.addCommand("rep", eepromDump);             // Displays all internal EEPROM contents.
  SCmd.addCommand("rtm", readTime);               // Displays current RTC time from the DS3231 I2C chip.
  SCmd.addCommand("rto", readTOffset);            // Displays current RTC temperature offset set in EEPROM.
  SCmd.addCommand("rot", readDsplyTmr);           // Reads/displays the OLED OFF display timer.
  //
  SCmd.addCommand("svb", setMon);                 // Turn ON/ continuous verbose monitoring every second to serial port.
  SCmd.addCommand("stm", setTime);                // Set RTC time.
  SCmd.addCommand("sdt", setDate);                // Set RTC date.
  SCmd.addCommand("sto", setTOffset);             // Set temperature offset of RTC to calibrate temperature reading.
  SCmd.addCommand("skt", setKpTmOut);             // Set the keypad timeout, Default - 20 seconds.
  SCmd.addCommand("sar", setAcsCnt);              // Set the keypad retry count, Default = 3.
  SCmd.addCommand("slk", setUnlckDly);            // Set the unlock delay. Default = 5 seconds.
  SCmd.addCommand("sgs", setGarSensor);           // Enable or disable garage door sensors (default = Enabled).
  SCmd.addCommand("sgt", setGarDrTmr);            // Set the garage door lock timer. Default = 30 minutes.
  SCmd.addCommand("sot", setdsplyTmr);            // Set the OLED OFF timer. Default = 10 seconds.
  SCmd.addCommand("sts", setTemprScale);          // Set the temperature scale.
  //
  SCmd.addCommand("adi", addDbId);                // Adds user id number in database.
  SCmd.addCommand("adp", addDbPwd);               // Adds user password in database.
  SCmd.addCommand("ain", addDbIdNam);             // Adds/modifies user name in database for a given id number.
  SCmd.addCommand("apn", addDbPwdNam);            // Adds/modifies user name in database for a given password number.
  SCmd.addCommand("ada", addDbAtt);               // Adds/Modifies user permission in database for a given id number or password.
  SCmd.addCommand("adt", addDbTm);                // Adds time stamp for user.
  //
  SCmd.addCommand("ddi", delDbId);                // Deletes user id number in database.
  SCmd.addCommand("ddp", delDbPwd);               // Deletes user password in database.
  SCmd.addCommand("din", delDbIdNam);             // Deletes user name in database for a given id number.
  SCmd.addCommand("dpn", delDbPwdNam);            // Deletes user name in database for a given id number.
  SCmd.addCommand("dda", delDbAtt);               // Deletes user permission byte in database for a given id number or password.
  //
  SCmd.addCommand("mdi", modDbId);                // Modifies a user id number in database.
  SCmd.addCommand("mdp", modDbPwd);               // Modifies a user password in database.
  SCmd.addCommand("min", modDbIdNam);             // Modifies a user name in database for a given id number.
  SCmd.addCommand("mpn", modDbPwdNam);            // Modifies a user name in database for a given password.
  //
  SCmd.addCommand("cdb", clrDb);                  // Clears user database (ID Tags, passwords, user names and attributes.
  SCmd.addCommand("ccf", clrConfig);              // Clears configuration in EEPROM RFID Database is left unchanged..
  SCmd.addCommand("cep", clrEeprom);              // Clears All of EEPROM including database and all configuration settings.
  //
  SCmd.addCommand("rst", resetFunc);              // Displays menu (commands).
  SCmd.addCommand("menu", menu);                  // Displays menu (commands).
  SCmd.addCommand("?", menu);                     // Displays menu (commands).
  SCmd.addDefaultHandler(menu);                   // Handler for command that isn't matched.

  runState = NORMAL;                              // Stert in normal operating mode.

  #if PROTOTYPE
    Serial.println (F("PROTOTYPE CONFIGURATION USED"));
  #endif
     
  #if DEBUG
    Serial.println (F("DEBUGGING MODE IS ON"));
  #endif

 // INITIALIZE RTC OBJECT -----------------------------------------------------------------------------------------
  #if defined CLOCK
    if (! rtc.begin())
    {
      uint8_t i=0;
      while (i < 10)
      {
        delay(10);
        Serial.println("Couldn't find RTC");
        i++;
      }
    }
    if (rtc.lostPower())
    {  
      Serial.println("RTC lost power, let's set the time!");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      DateTime now = rtc.now();
      // PC's compile time is in STANDARD TIME only. The line below adjusts for DST if DST flag is set in EEPROM.
      if(EEPROM.read(eAddrDst)){rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour()+1, now.minute(), now.second()));}
    }

    // When time needs to be re-set on a previously configured device, the    
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

    // SETUP OF RTC SO OSC KEEPS RUNNING WHEN ON BATTAERY POWER -----------------------------------------------------
    #define DS3231_I2C_ADDRESS 0x68               // 0x68 is the RTC address
    //  Wire.begin();                             // Not needed. Called by rtc library.
    //  Wire.beginTransmission(DS3231_I2C_ADDRESS);// Not needed. Called by rtc library.
    Wire.write(0xE);                              // Address the Control Register
    Wire.write(0x00);                             // Write 0x0 to control Register
    Wire.endTransmission();
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0xF);                              // Address the Status register
    Wire.write(0x00);                             // Write 0x0 to Status Register
    Wire.endTransmission();
  #endif
            

    lcd.begin(LCDCOL,LCDROW);                  // columns, rows.  use 20,4 for a 20x4 LCD, etc.

  
  // INITIALIZE OLED DISPLAY --------------------------------------------------------------------------------------
  // initialize with the I2C addr 0x3C (for the 128x32)
  // Check your I2C address and enter it here, in Our case address is 0x3C
  #if defined OLEDDISPLAY
    oled.begin(SSD1306_SWITCHCAPVCC, OLEDADR);
    oled.clearDisplay();
    oled.display();                               // this command will display all the data which is in buffer
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    oled.setTextColor(WHITE, BLACK);
  #endif

  readVerbose();

  #if defined OLEDDISPLAY
    drawRev();
  #endif

  #if defined SOUND
    startupTone();                                // Play close encounters of the 3rd kind.
  #endif

  #if defined RGBLED
    ledSelfTest();                                // Toogle all leds
  #endif

  #if defined OLEDDISPLAY
    oled.clearDisplay();                          // Clear the buffer
    oled.display();                               // Show buffer containts on the display.
  #endif

  //  bprSelfTest();                              // Check keypad buzzers.
  help();
}

//#################################################################################################################
// MAIN PROGRAM
//#################################################################################################################
void loop()
{
  switch (runState)
  {
    case NORMAL:
      runMode();
    break;
      
    case PROGRAM:
      progMode();
    break;
  }
  SCmd.readSerial();                              // We don't do much, just process serial commands}
  readEncoder();                                  // Check if encoder has moved, display temperature in hires (smallFont).
}

//#################################################################################################################
// RUN MODE METHOD
//#################################################################################################################
void runMode()
{
  //uint32_t      idPwd = 0;
  
  // TEN HZ TIMER RUNTIME -----------------------------------------------------------------------------------------
  if(tenHzTick)
  {
    tenHzTick = OFF;
  }

  // ONE HZ TIMER RUNTIME -----------------------------------------------------------------------------------------
  if (oneHzTick)
  {
    if(pirFlag)
    {
      dsplyTmr = EEPROM.read(eAddrDsplyTmr);
      pirFlag = OFF;
    }

    if(dsplyTmr)
    {
      oled.ssd1306_command(SSD1306_DISPLAYON);

      if(encPosition > 4){encPosition = 0;}
      else if(encPosition < 0){encPosition = 4;}

      drawTmDt();                                 // Display Time, date and temperature on OLED display.
      if(encPosition == TMDT){drawTmDt();}        // Encoder position 3
      else if(encPosition == VCCVAL){drawVcc();}  // Encoder position 2
      else if(encPosition == VINVAL){drawVin();}  // Encoder Position 1
      dsplyTmr--;
    }
    else
    {
      oldEncPosition  = 0;                            // Initialize encoder to detect change.
      encPosition     = 0;          
      oled.ssd1306_command(SSD1306_DISPLAYOFF);
    } 

    if (EEPROM.read(eAddrSetMon)){readVerbose();} // Display all data (if set through "svb" command).
    if (retryCnt >= EEPROM.read(eAddrRetryCnt)){digitalWrite(frtLedPin, !digitalRead(frtLedPin));}
    oneHzTick = OFF;                              // Reset 1Hz timer flag.
  }

  if(oneMnTick)
  {
    checkDst();                                   // Check for DST.
    oneMnTick = OFF;
  }

  if(retryCnt < EEPROM.read(eAddrRetryCnt))
  {
    idPwd = chkKeypad();                          // Checks to see if valid ID tag or password is entered.
    if(idPwd)                                     // gets ID tag from keypad.
    {
      if(db.contains(idPwd))                      // Check if id Tag or password found in database.
      {
        retryCnt = 0;                             // id Tag or passord valid, so reset ID/Paswword retry count.
        pos = db.posOf(idPwd);
        //userInfo(pos);
        //displayPerm();
        procAtt(pos);                             // Checks permission to unlock door for given ID or password.
        idPwd = 0;                                // Clear id tag/password buffer.    
      }
      else
      {
        readTmDt();
        Serial.println(F("ID OR PASSWORD NOT FOUND IN DATBASE"));
        retryCnt++;                               // Flag incorrect TagId or password.
        KpLckTm = timeStmp + EEPROM.read(eAddrKpLckTm);          // Set the keypad lockout time period. 
        errorTone();
      }
    }
  }
  else if(timeStmp > KpLckTm)                     // If keypad timeout has expired, reset keypad lockout.
  {
    KpLckTm =0;                                   
    retryCnt = 0;                                 // Remove ketpad lock-out.
    digitalWrite(frtLedPin, OFFINV);              // Reset keypad LED.
  }
  checkLocks();                                   // Monitor lock status and lock timer. Lock doors once expired.
  checkBtn();                                     // Checks all push buttons for change in status.
}

//#################################################################################################################
// CONFIGURATION METHOD
//#################################################################################################################
// Checks Keypad Timeout IRQ and resets normal operation if expires.
void progMode()
{
  if(!progTimer)                                  // See IRQ for Configuration mode counter decrement function.
  {
    runState = NORMAL;
  }
}

//#################################################################################################################
// PROCESS ATTRIBUTE METHOD
//#################################################################################################################
// Checks if password is required with ID tag.
bool procAtt(int16_t user)
{
//  uint8_t att;
//  uint32_t  idPwd = 0;
  if (user >= PWDFLAG){db.readAtt((user & PWDMASK), att);} // Remove password flag from position info.
  else{db.readAtt(user, att);}
  if (att & IDANDPWD)
  {
    keyTmr = EEPROM.read(eAddrKeyTmr);
    while(keyTmr)
    {
      idPwd = chkKeypad();
      if (idPwd)
      {
        if (user >= PWDFLAG)                      // 1ST entered password, so now looking for ID.
        {
          user &= PWDMASK;
          if ((db.posOf(idPwd) < PWDFLAG) && (db.posOf(idPwd) == user)){return unlockDoor(user);}
          else{return false;}
        }
        else                                      // 1ST entered ID so now looking for password.
        {
          if ((db.posOf(idPwd) >= PWDFLAG) && ((db.posOf(idPwd) & PWDMASK ) == user)){return unlockDoor(user);}
          else{return false;}
        }
      }
    }
    return false;                                 // If "WHILE" loop times out, then return.
  }
  else                                            // Code + Pwd not required so just open door.
  {
    if (user >= PWDFLAG){user &= PWDMASK;}
    return unlockDoor(user);
  }
}

//#################################################################################################################
// OPEN LOCK METHOD
//#################################################################################################################
// Checks open type (permanent, temporary, onetime) in attribute for user to confirm access and on which door.
// Note "user" variable passed from "procAtt"  method shows position only. PWDFLAG has been stripped as it is 
// not needed.
bool unlockDoor(int16_t userPos)                     // Open (lock) solenoid.
{
  //uint8_t att;
  //char name[NAMELENGTH];
  //uint32_t      idPwd = 0;
  uint32_t  tm    = 0;

  readTmDt();
  db.readNam(userPos, name);                                           // Get name for id or password.
  Serial.print(name);
  
  db.readAtt(userPos, att);                                             // Reads attribute of user position in database.
  if((att & ONETMACCESS) || (att & TEMPACCESS) || (att & PERMACCESS))   // Processes the type of access (onetime, temporary or permanent).
  {
    if (att & ONETMACCESS)                                              // Check for ONE TIME ACCESS.
    {
      if(!db.readId(userPos, idPwd)){db.readPwd(userPos,idPwd);}              // Get ID or password.
      db.insertAtt(idPwd, (att & ~ONETMACCESS));                        // Turn off one time use for this user.
    }

    if (att & TEMPACCESS)                                               // Check for TIME LIMITED ACCESS
    {
      // If timer is greater than time period set then return  .
      if (!db.readTm(userPos,tm)){return false;}
      else if (timeStmp > tm)                                           // Time expired, disable temporary access.
      {
        if(!db.readId(userPos, idPwd)){db.readPwd(userPos,idPwd);}            // Find id or password for user.
        db.insertAtt(idPwd,att & ~TEMPACCESS);                          // Turn off temp access.
        return false;                                                   // Temp access expired, so do NOT unlock door.
      }
    }

    if((att & FRTDRLOCK) && (keyPdLoc == FRTDRKEYPD))                      // Unlock front door.
    {
      Serial.print(F(" IS UNLOCKING FRONT DOOR..."));
      unlockFrtDr();
    }

    else if((att & REARDRLOCK) && (keyPdLoc == REARDRKEYPD))               // Unlock rear door.
    {
      Serial.print(F(" IS UNLOCKING REAR DOOR..."));     
      unlockRearDr();
    }

    else if((att & SHEDDRLOCK) && (keyPdLoc == SHEDDRKEYPD))               // Unlock Shed door.
    {
      Serial.print(F(" IS UNLOCKING SHED DOOR..."));     
      unlockShedDr();
    }

   // Check id Tag or password from garage keypad ONLY with GARAGE attribute set.
    else if((att & GARDRLOCK) && (keyPdLoc == GARDRKEYPD))
    {
      if(digitalRead(garDrDnSwPin) == ONINV){Serial.print(F(" IS UNLOCKING"));}
      else{Serial.print(F(" IS LOCKING"));}
      Serial.print(F(" GARAGE DOOR, "));
      garDrCntl();
    }
    else
    {
      Serial.println(F(" DOES NOT PERMISSION TO OPEN DOOR AT THIS KEYPAD"));
      return false;
    }
  }
  return true;
}

//#################################################################################################################
// CHECK KEYPAD METHOD
//#################################################################################################################
// Waits for a numerical input from keypad during normal operation.
uint32_t chkKeypad()
{
  uint32_t arg = 0;
  if(wg.available())
  {
    if(wg.getWiegandType() == 26 || wg.getWiegandType() == 34)
    {return wg.getCode();}                        // TagID received so exit.
    else if(wg.getWiegandType() == 4)
    {
      keyTmr = EEPROM.read(eAddrKeyTmr);          // Reset keypad timer.  
      arg =  wg.getCode();                        // larger variable to make math easier.
      if (arg == '#')                             // # acts as "Enter" key.
      {
        // If garage door is open and no value was entered, just pressing the # key will close the garage door.
       if(keyVal == 0 && digitalRead(garDrDnSwPin) == OFFINV && keyPdLoc == GARDRKEYPD) 
        {
        readTmDt();
        Serial.print(F("# KEY PRESSED ON GARAGE KEYPAD, "));
          keyTmr = 0;
          arg = 0;                              // Clear "#" for next pass.
          garDrCntl();
        }
        
        arg = keyVal;
        keyVal = 0;                               // Clear accumilator for next cycle
        return arg;                               // Returns keypad etntry.
      }
      else if(arg == '*')
      {
        progTimer = EEPROM.read(eAddrProgTime);
        runState = PROGRAM;
        keyVal = 0;
      }
      else
      {
        keyVal = keyVal * 10UL + arg;
      }
    }
  }
  return 0;
}

//#################################################################################################################
// PROCESS KEY METHOD
//#################################################################################################################

// Checks ID scanned 
// KEYPAD COMMAND STRUCTURE
// PPW = PROGRAMMING PASSWORD
// APW = ADMINISTRATOR PASSWORD

// ADD NEW ID TAG                             * - <PPW> - # -  1 - # <SCAN RFID OR KEYPAD ENTRY> - ##
// ADD NEW PASSWORD                           * - <PPW> - # -  2 - # <SCAN RFID OR KEYPAD ENTRY> - ##
// ADD ID TAG TO EXISTING PASSWORD            * - <PPW> - # -  3 - # <PASSWORD> - # - <SCAN RFID OR KEYPAD ENTRY> - ##
// ADD PASSWORD TO EXISTING PASSWORD ID TAG   * - <PPW> - # -  4 - # <SCAN RFID OR KEYPAD ENTRY> - # <PASSWORD> - ##
// ADD NAME TO ID TAG                         * - <PPW> - # -  5 - # <SCAN RFID OR KEYPAD ENTRY> - # - <NAME> - ##
// ADD NAME TO PASSWORD                       * - <PPW> - # -  6 - # <PASSWORD> - # - <NAME> - ##
// ADD ATTRIBUTE TO ID TAG                    * - <PPW> - # -  7 - # <SCAN RFID OR KEYPAD ENTRY> - # - <ATTRIBUTE> - ##
// ADD ATTRIBUTE TO PASSWORD                  * - <PPW> - # -  8 - # <PASSWORD> - # - <ATTRIBUTE> - ##
// DELETE ID TAG                              * - <PPW> - # -  11 - # <SCAN RFID OR KEYPAD ENTRY> - ##
// DELETE A PASSWORD                          * - <PPW> - # -  12 - # <PASSWORD> - ##
// DELETE NAME TO ID TAG                      * - <PPW> - # -  15 - # <SCAN RFID OR KEYPAD ENTRY> - ##
// DELETE NAME TO PASSWORD                    * - <PPW> - # -  16 - # <PASSWORD> - ##
// DELETE ATTRIBUTE FROM ID TAG               * - <PPW> - # -  17 - # <SCAN RFID OR KEYPAD ENTRY> - ##
// ADD ATTRIBUTE FROM PASSWORD                * - <PPW> - # -  18 - # <PASSWORD> - ##

//#################################################################################################################
// PROCESS ID METHOD
//#################################################################################################################
void processId(uint32_t idVal)
{
//  int16_t pos = 0;
//  uint8_t att = 0;
  pos = 0;
  att = 0;

  if(db.contains(idVal))                          // Check if id Tag or password found in database.
  {
    pos = db.posOf(idVal);
    db.readAtt(pos,att);    
  }
}

//#################################################################################################################
// DRAW REVISION METHOD
//#################################################################################################################
// Draws the firmware revision onto the OLED display.
void drawRev()
{
  oled.ssd1306_command(SSD1306_DISPLAYON);
  oled.clearDisplay();                            // Clear the buffer
  drawHdr("** RFID CONTROLLER **");                    // Draw firmware revision header.
  drawMsg(1,1,10,8,"FIRMWARE REVISION");
  drawMsg(12,1,25,30,REV);                        // Display revision.
  oled.display();                                 // Show buffer containts on the display.
}

//#################################################################################################################
// DRAW HEADER METHOD
//#################################################################################################################
// Draws the header at the top of the OLED display. 
void drawHdr(char *msg)
{
  oled.setFont();
  oled.setTextSize(1);                            // Normal 1:1 pixel scale
  oled.setTextColor(WHITE,BLACK);               // Draw white text
  oled.setCursor(0,0);                           // Start at top-left corner
  oled.println(msg);                              // Draw display header message. 
  oled.setFont();                                 // return to the system font
} 

//#################################################################################################################
// DRAW MESSAGE METHOD
//#################################################################################################################
// Draws text message on the OLED display. 
// Variables passed FONT,TEXT_SIZE,XPOS,YPOS,TEXT MESSAGE
void drawMsg(uint8_t fnt, uint8_t fntSiz, uint8_t xPos, uint8_t yPos, char *msg)
{
  if(fnt == 1){oled.setFont();}                 // reset to default font size.
  else if(fnt == 11) {oled.setFont(&Dialog_bold_11);}               // choose 11 pitch bold font
  else if(fnt == 12) {oled.setFont(&FreeMonoBoldOblique12pt7b);}    // choose 12 pitch bold font
  else{oled.setFont(&FreeMonoBoldOblique18pt7b);}                   // choose 18 pitch bold font
  oled.setTextSize(fntSiz);
  oled.setTextColor(WHITE,BLACK); 
  oled.setTextWrap(false);
  oled.setCursor(xPos,yPos);                    // Center text.
  oled.print(msg);
  oled.setFont();                               // return to the system font
}

//#################################################################################################################
// DRAW TEXT METHOD
//#################################################################################################################
// Draws text on OLED display.
void drawText(uint8_t x_pos, uint8_t y_pos, char *text, uint8_t text_size)
{
  oled.setCursor(x_pos, y_pos);
  oled.setTextSize(text_size);
  oled.print(text);
  oled.display();
}

//#################################################################################################################
// DRAW DATE METHOD
//#################################################################################################################
// Displays time, date and temperature on the OLED display.
void drawTmDt()
{
  if(dsplyTmr)
  { 
    int8_t tOffset = EEPROM.read(eAddrTOffset);
    oled.ssd1306_command(SSD1306_DISPLAYON);
    DateTime now = rtc.now();
    oled.setTextColor(WHITE,BLACK); 

    // Draw date --------------------------------------------------------------------------------------------------
    if(dowFlag != now.dayOfTheWeek())
    {
      dowFlag = now.dayOfTheWeek();                   // Update flag.
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setCursor(0,0);
      dsplyMsg ((const char *) &daysOfTheWeek[now.dayOfTheWeek()]); // Get DayOfWeek from array in Program memory.
      oled.setTextSize(1);
      oled.setCursor(62,0);
      drawDigits(now.day());
      oled.setCursor(77,0);
      dsplyMsg ((const char *) &months[now.month()]);               // Get months from array in Program memory.
      oled.setTextSize(1);
      oled.setCursor(98,0);
      drawDigits(now.year());
    }
    
    // Draw time --------------------------------------------------------------------------------------------------
    oled.setTextSize(2);
    oled.setCursor(15,9);
    drawDigits(now.hour());
    oled.print(":");
    drawDigits(now.minute());
    oled.print(":");
    drawDigits(now.second());

    // Draw PIR Timer ---------------------------------------------------------------------------------------------
    oled.setTextSize(1);
    oled.setCursor(0,25);
    if(dsplyTmr < 10){oled.print(" ");}
    oled.print(dsplyTmr);
    
    // Draw temperature -------------------------------------------------------------------------------------------
    float tmprVal = rtc.getTemperature();
    oled.setTextSize(1);
    oled.setCursor(82,25);
    if(tOffset < 127){(tmprVal + tOffset);}
//    else{tmprVal - ~tOffset - 1;}
    else{tmprVal -= ~tOffset - 1;}

    if(EEPROM.read(eAddrTemprScale)){oled.print(tmprVal);}
    else{oled.print((tmprVal * 1.8) + 32);}
    oled.drawRect(117, 25, 3, 3, WHITE);            // Put degree symbol ( � )
    if(EEPROM.read(eAddrTemprScale)){drawText(122, 25, "C", 1);}
    else{drawText(122, 25, "F", 1);}
    oled.display();
  }
  else{oled.ssd1306_command(SSD1306_DISPLAYOFF);} 
}

//#################################################################################################################
// DISPLAY COLON AND LEADING ZERO FUNCTION
//#################################################################################################################
// utility function for digital clock display: draws leading 0 on OLED display.
void drawDigits(int digits)
{
  if(digits < 10)
  oled.print('0');
  oled.print(digits);
}

//#################################################################################################################
// DRAW VIN METHOD
//#################################################################################################################
// Displays time, date and temperature on the OLED display.
void drawVin()
{
  //X,Y,TEXT,TEXT_SIZE
  drawText(0,0,"*** VIN ***",1);
  oled.setTextSize(2);
  oled.setCursor(15,9);
  oled.print(analogRead(vinPin),1);
  oled.print(F("VDC"));
}

//#################################################################################################################
// DRAW VCC METHOD
//#################################################################################################################
// Displays time, date and temperature on the OLED display.
void drawVcc()
{
  //X,Y,TEXT,TEXT_SIZE
  drawText(0,0,"*** VCC ***",1);
  oled.setTextSize(2);
  oled.setCursor(15,9);
  oled.print(analogRead(vccPin),1);
  oled.print(F("VDC"));
}

//#################################################################################################################
// GET KEYPAD OR TAG TYPE DATA METHOD
//#################################################################################################################
// Returns the Wiegand type 26,34 for RFID, 4 or 8 for keypad.
uint8_t getType()
{
  return wg.getWiegandType();
}

//#################################################################################################################
// GET ACCESS CONTROLLER (KEYPAD) ID METHOD
//#################################################################################################################
// Determines which keypad is active. 1= Front Door, 2=Garage door, 3=Rear Door.
uint8_t getKeypadId()
{
  return wg.getGateActive();
}

//#################################################################################################################
// GET ACCESS CONTROLLER (KEYPAD) METHOD
//#################################################################################################################
// Returns the last id tag from RFID or keypad entry.
uint32_t getId()
{
  uint32_t arg = 0;
  uint32_t  tagId = 0;
  for (int8_t i=0; i<10; i)                       // Gets 10 digit (32bit) ID Tag.
  {  
    if(wg.available())
      {
      if(wg.getWiegandType() == 26 || wg.getWiegandType() == 34){return wg.getCode();} // TagID received so exit.
      else if(wg.getWiegandType() == 4)
      {
        uint32_t arg =  wg.getCode();             // larger variable to make math easier.
        if (arg == '#'){return tagId;}
        else{tagId = tagId * 10UL + arg;} 
      }
    }
  }
  return 0;
}

//#################################################################################################################
// GET ERROR CODE METHOD
//#################################################################################################################
// Returns last error code recorded.
uint8_t getLastErr()
{
  return(EEPROM.read (eAddrErrCode));
}

//#################################################################################################################
// DISPLAY ALL SETTINGS VALUES METHOD
//#################################################################################################################
void readVerbose()                                // Read all battery parametrs and set points.
{
  Serial.flush();
  Serial.print(F("RFID_LOCK, "));
  Serial.print(F("FIRMWARE REVISION "));
  Serial.println(REV);
  readTime();
  //
  // RUN STATE
  Serial.print(F("RUN STATE = "));
  if (runState == NORMAL){Serial.println(F("NORNAL MODE\t\t"));}
  else if (runState == PROGRAM){Serial.println(F("PROGRAMMING MODE\t"));}  
  //
  // MAX USER CAPACITY, TOTAL NUMBER OF USERS STORED
  Serial.print(F("MAX USER CAPACITY = "));
  Serial.print(db.totalUsers()); 
  Serial.print(F(", "));
  Serial.print(db.count());
  Serial.println(F(" USERS CURRENTLY STORED\r\n"));
  //
  // TOTAL DATABASE SIZE IN BYTES
  Serial.print(F("TOTAL DATABASE SIZE = "));
  Serial.print(db.dbSize());
  Serial.println(F(" Bytes"));
  Serial.print(F(", NAME LENGTH = "));
  Serial.println(NAMELENGTH - 1);
  //
  // GARAGE DOOR STATUS
  Serial.print(F("GARAGE DOOR IS "));
  readGarStatus();
  readAcsCnt();                                   // Displays the number of keypad input attempts.
  Serial.println();
}

//#################################################################################################################
// READ LOCK RETRY COUNT METHOD
//#################################################################################################################
// Displays the number of password entries attempted before time-out occurs.
void readAcsCnt()
{
  Serial.print(F("LOCK RETRY COUNT IS SET TO = "));
  Serial.print(EEPROM.read (eAddrRetryCnt));
  Serial.print(F(", CURRENT RETRY COUNT IS = "));
  Serial.println(retryCnt);
}

//#################################################################################################################
// READ KEY TIMEOUT METHOD
//#################################################################################################################
// Displays the keypad time-out when keypad number entry before returning to normal operation.
void readKpTmOut()
{
  Serial.print(F("KEYPAD TIME-OUT IS = "));
  Serial.print(EEPROM.read (eAddrKeyTmr));
  Serial.println(F(" SECONDS"));
}

//#################################################################################################################
// READ ENCODER FUNCTION
//#################################################################################################################
bool readEncoder()
{
  encPosition += encoder->getValue();
  if (encPosition == oldEncPosition){return false;}
  
  oldEncPosition = encPosition;
  dsplyTmr = EEPROM.read(eAddrDsplyTmr);
  if (runState == NORMAL) {dsplyTmr = EEPROM.read(eAddrDsplyTmr);}  // Set display timeout (10 sencond default).
  else {menuTimeout = EEPROM.read(eAddrMenuTimeout);}       // Reset menu Timeout as long as encoder is moved.
Serial.print(F("Encoder position = "));
Serial.println(encPosition);
  return true;
}

//#################################################################################################################
// ENCODER SELECT BUTTON FUNCTION (INTERRUPT DRIVEN)
//#################################################################################################################
void readSelBtn()
{  
  switch (encoder->getButton())
  {
      case ClickEncoder::Clicked:
        Serial.println(F("Select Buttun: CLICKED"));
      break;

      case ClickEncoder::Held:
//        menuState = 1;
        Serial.println(F("Select Buttun: HELD"));
      break;

      case ClickEncoder::DoubleClicked:
        Serial.println(F("Select Buttun: DOUBLE-CLICKED"));
      break;

      case ClickEncoder::Released:
        Serial.println(F("Select Buttun: RELEASED"));
      break;

      case ClickEncoder::Open:
      break;

      case ClickEncoder::Closed:
      break;

      case ClickEncoder::Pressed:
      break;
  }
}

//#################################################################################################################
// READ UNLOCK TIME
//#################################################################################################################
// Displays the solenoid unlock time in seconds.

void readUnlckDly()
{
  Serial.print(F("FRONT DOOR UNLOCK TIME IS = "));
  Serial.print(EEPROM.read (eAddrFrtDrLckTmr));   // Value stored in 1/10 sec, so divide by 10.
  Serial.println(F(" SECONDS"));

  Serial.print(F("REAR DOOR UNLOCK TIME IS = "));
  Serial.print(EEPROM.read (eAddrRrDrLckTmr));    // Value stored in 1/10 sec, so divide by 10.
  Serial.println(F(" SECONDS"));

  Serial.print(F("SHED DOOR UNLOCK TIME IS = "));
  Serial.print(EEPROM.read (eAddrShdDrLckTmr));   // Value stored in 1/10 sec, so divide by 10.
  Serial.println(F(" SECONDS"));
}

//#################################################################################################################
// READ UNLOCK TIME METHOD
//#################################################################################################################
// Displays the solenoid unlock time in seconds.

void readGarDrTmr()
{
  Serial.print(F("GARAGE DOOR LOCK DELAY TIMER SET TO "));
  Serial.print(EEPROM.read (eAddrGarDrTmr));
  Serial.print(F(" MINUTES"));
  Serial.print(F(", TIMER CURRENTLY AT "));
  Serial.print(garDrTmr);
  Serial.println(F(" MINUTES\r\n"));
}

//#################################################################################################################
// READ TEMPERATURE OFFSET METHOD
//#################################################################################################################
// Displays the solenoid unlock time in seconds.

void readTOffset()
{
  Serial.print(F("RTC'S TEMPERATURE OFFSET = "));
  Serial.print(EEPROM.read (eAddrTOffset));
  Serial.println(F(" DEGs"));
}

//#################################################################################################################
// READ OLED DISPLAY OFF TIMER METHOD
//#################################################################################################################
// Displays the solenoid unlock time in seconds.

void readDsplyTmr()
{
  Serial.print(F("OLED OFF TIMER (AFTER PIR DETECTION) IS = "));
  Serial.print(EEPROM.read (eAddrDsplyTmr));
  Serial.println(F(" SECONDS"));
}

//#################################################################################################################
// READ RTC METHOD
//#################################################################################################################
// Reads current time from DS3231 I2C chip.
void readTime()
{
  DateTime now = rtc.now();
  int8_t tOffset = EEPROM.read(eAddrTOffset);
  Serial.print(" (");
  printMsg ((const char *) &daysOfTheWeek[now.dayOfTheWeek()]); // Get DayOfWeek from array in Program memory.
  Serial.print("), ");
  readTmDt();
  Serial.print(", ");
  Serial.print("Temperature: ");
  if(tOffset < 127){Serial.print(rtc.getTemperature() + tOffset);}
  else{Serial.print(rtc.getTemperature() - ~tOffset -1);}
  Serial.print(" C, ");
  if(EEPROM.read(eAddrDst)){Serial.println(F("DST"));}
  else{Serial.println(F("STANDARD TIME"));}
  Serial.println();
}

//#################################################################################################################
// READ AND DISPLAY TIME DATE METHOD
//#################################################################################################################
// Reads current time from DS3231 I2C chip.
void readTmDt()
{
  DateTime now = rtc.now();
  printDigits(now.day());
  Serial.print('-');
  printMsg ((const char *) &months[now.month()]);               // Get months from array in Program memory.
  Serial.print('-');
  printDigits(now.year());
  Serial.print(", ");
  printDigits(now.hour());
  Serial.print(':');
  printDigits(now.minute());
  Serial.print(':');
  printDigits(now.second());
  Serial.print(' ');
}

//#################################################################################################################
// READ ACCESS CONTROLLER (KEYPAD) DATA METHOD
//#################################################################################################################
void readKeypad()
{
//    int8_t kypdDec = uint8_t(wg.getCode();
//    char  kypdVal = wg.getCode();
    Serial.print(F("KEYPAD VALUE:\t"));
    Serial.print(char(wg.getCode()));
    Serial.print(F(", DECIMAL = "));
    Serial.print(wg.getCode());
    Serial.print(F(", HEX =")); 
    Serial.print(wg.getCode(),HEX);
    Serial.print(F(", Type W"));
    Serial.print(wg.getWiegandType());
    keypadLoc();
    Serial.print(F("BINARY = "));
    Serial.print(wg.getCode(),BIN);
    Serial.print(F(", RAW = "));
    Serial.println(wg.getRawCode(),BIN);
    keyPdLoc = 0;
    Serial.println("");
//    wg.clear();
}

//#################################################################################################################
// READ ERROR CODE METHOD
//#################################################################################################################
// Displays last error code recorded.
void readLastErr()
{
  Serial.print(F("LAST ERROR CODE IS "));
  Serial.println(getLastErr());
}

//#################################################################################################################
// READ DATABASE ID METHOD
//#################################################################################################################
// Displays User id for a given user number in the database.
// If user 999 is entered, all user ids and names will be displayed.
void readIdPwd()
{
  int16_t user = 0;
  user = argNumMinMax(0,999);
  if (user == 999)                                    // Display all users
  {
    uint8_t count = db.count();
    Serial.print(F("Total Users = "));
    Serial.println(count);
    for (int i = 0; i < count; i++){userInfo(i);}
    displayPerm();
  }
  else if (user <= db.count())                      // Display specific user by User number.
  {
    userInfo(user);
    displayPerm();
  }
  else
  {
    Serial.print(F("USER NUMBER IS OUT OF RANGE. "));
    Serial.print(db.count());
    Serial.println(F(" USERS CURRENTLY STORED IN THE DATABASE"));
  }
}

//#################################################################################################################
// READ DATABASE NAME METHOD
//#################################################################################################################
// Displays User name for a given id number in the database.
void readDbNam()
{
  //char name[NAMELENGTH];
  //char name;
  //uint32_t idPwd = 0;
  idPwd = 0;
  int16_t user;
  
  Serial.println(F("ENTER A TAG ID OR PASSWORD, OR PLACE A TAG ON THE READER"));
  if(!argNum(idPwd)){Serial.println(F("NO ID OR PASSWORD ENTERED"));}
//  else if(!db.contains24(idPwd)){Serial.println(F("ID OR PASSWORD NOT FOUND IN DATABASE"));}
  else
  {
    user = db.posOf(idPwd);
    Serial.print(F("NAME FOR ID/PASSWORD \""));
    Serial.print(idPwd);
    Serial.print(F("\" IS "));
    Serial.print(idPwd);
    Serial.print(F(" IS "));
    db.readNam(user, name);
    Serial.println(name);
  }
}

//#################################################################################################################
// READ GARAGE DOOR STATUS METHOD
//#################################################################################################################
// utility function for digital clock display: prints preceding colon and leading 0.
void readGarStatus()
{
  Serial.print(F("GARAGE DOOR SENSORS ARE "));

  if(EEPROM.read(eAddrGarDrSn))
  { 
    Serial.println(F("ENABLED"));
    Serial.print(F("GARAGE DOOR IS "));
    if(!digitalRead(garDrUpSwPin)){Serial.println(F("OPEN"));}
    else if(!digitalRead(garDrDnSwPin)){Serial.println(F("CLOSED"));}
    else{Serial.println(F("IN MOVEMENT"));}
  }
  else
  {
    Serial.println(F("DISABLED"));
  }
  readGarDrTmr();
}

//#################################################################################################################
// SET VERBOSE MONITORING METHOD
//#################################################################################################################
// Turns Verbose mode ON or OFF.
void setMon()
{
  int returnVal;
  returnVal = argOnOff();
  if (returnVal == 1)
  {
    EEPROM.write(eAddrSetMon,ON);
    Serial.println(F("CONTINUOUS MONITORING STARTED..."));
  }
  else if (returnVal == 0)
  {
    EEPROM.write(eAddrSetMon,OFF);
    Serial.println(F("CONTINUOUS MOMITORING STOPPED..."));
  }
}

//#################################################################################################################
// SET ACCESS CODE RETRY COUNT METHOD
//#################################################################################################################
void setAcsCnt()                                  // Set lock retry count (default = 3).
{
  int8_t arg = argNumMinMax(1,5);           // get 32 bit number argument and convert to 8 bits (min = 1, Max = 5).
  if(arg <= 0)
  {
    Serial.println("");
    Serial.println(F("SETTING ACCESS CODE RETRY ENTRY FAILED"));
  }
  else
  {  
    Serial.print(F("ACCESS CODE RETRY IS SET TO "));
    Serial.println(arg);
    EEPROM.write(eAddrRetryCnt,arg);
  }
}

//#################################################################################################################
// SET ACCESS CODE TIMEOUT LOCK METHOD
//#################################################################################################################
void setKpTmOut()                                 // Set lock retry count (default = 20).
{
  int8_t arg = argNumMinMax(1,240);     // get 32 bit number argument and convert to 8 bits (min = 1, Max = 240).
  if(arg <= 0){Serial.println(F("SETTING KEYPAD TIMEOUT ENTRY FAILED"));}
  else
  {  
    Serial.print(F("KEYPAD TIMEOUT IS SET TO "));
    Serial.print(arg);
    Serial.println(F(" SECONDS"));
    EEPROM.write(eAddrKeyTmr,arg);
  }
}

//#################################################################################################################
// SET UNLOCK TIME METHOD
//#################################################################################################################
void setUnlckDly()                                // Set the unlock delay time (default = 5).
{
  Serial.println(F("SELECT WHICH DOOR DELAY TO CHANGE:"));
  Serial.println(F("1 - FRONT DOOR UNLOCK TIME"));
  Serial.println(F("2 - REAR DOOR UNLOCK TIME"));
  Serial.println(F("3 - SHED DOOR UNLOCK TIME"));

  int8_t sel = argNumMinMax(1,3);                 // Select which delay to change.
  if(sel <= 0){Serial.println(F("INCORRECT SELCTION"));}
  else
  {
    int8_t arg = argNumMinMax(1,240);             // Select which delay to change.
    if(arg <= 0){Serial.println(F("SETTING UNLOCK DELAY VALUE FAILED"));}
    else
    {  
      switch(sel)
      {
        case 1: 
        Serial.print(F("FRONT DOOR UNLOCK DELAY TIME IS SET TO "));
        Serial.print(arg);
        Serial.println(F(" SECONDS"));
        EEPROM.write(eAddrFrtDrLckTmr, arg);
        break;

        case 2: 
        Serial.print(F("REAR DOOR UNLOCK DELAY TIME IS SET TO "));
        Serial.print(arg);
        Serial.println(F(" SECONDS"));
        EEPROM.write(eAddrRrDrLckTmr, arg);
        break;

        case 3: 
        Serial.print(F("SHED DOOR UNLOCK DELAY TIME IS SET TO "));
        Serial.print(arg);
        Serial.println(F(" SECONDS"));
        EEPROM.write(eAddrShdDrLckTmr, arg);
        break;
      }
    }
  }
}

//#################################################################################################################
// SET GARAGE DOOR SENSOR METHOD
//#################################################################################################################
void setGarSensor()
{
  int returnVal;
  returnVal = argOnOff();
  if (returnVal < 0){return;}                         // Syntax error or missing argument occured.
  Serial.print(F("GARAGE DOOR POSITION SENSORS ARE "));
  if (returnVal){Serial.println(F("ENABLED"));}
  else{Serial.println(F("DISABLED"));}
  EEPROM.write(eAddrGarDrSn,returnVal);
}

//#################################################################################################################
// SET GARAGE DOOR TIMER METHOD
//#################################################################################################################
void setGarDrTmr()                                // Set the garage door lock time (default = 30).
{
  int8_t arg = argNumMinMax(1,240);               // (min = 1, Max = 240 minutes).
  if(arg <= 0){Serial.println(F("SETTING GARAGE DOOR TIMER VALUE FAILED"));}
  else
  {  
    Serial.print(F("GARAGE DOOR TIMER IS SET TO "));
    Serial.print(arg);
    Serial.println(F(" MINUTES"));
    EEPROM.write(eAddrGarDrTmr,(arg));
  }
}

//#################################################################################################################
// SET OLED OFF TIMER METHOD
//#################################################################################################################
void setdsplyTmr()                                // Set the OLED off time (default = 10 seconds).
{
  int8_t arg = argNumMinMax(1,240);               // (min = 1, Max = 240 seconds).
  if(arg <= 0){Serial.println(F("SETTING OLED TIMER VALUE FAILED"));}
  else
  {  
    Serial.print(F("THE OLED OFF TIMER IS SET TO "));
    Serial.print(arg);
    Serial.println(F(" SECONDS"));
    EEPROM.write(eAddrDsplyTmr,(arg));
  }
}

//#################################################################################################################
// SET RTC TIME METHOD
//#################################################################################################################
// Sets an port configured as an output High or Low.
void setTime()
{
  uint8_t hr  = 0;
  uint8_t mn  = 0;
  uint8_t sc  = 0;
  uint8_t arg;

  DateTime now = rtc.now();

  hr = argNumMinMax(0,23);                        // Get hours.
  if(hr < 0){return;}                             // Check for argument error.
  
   mn = argNumMinMax(0,59);                       // Get minutes.
  if(mn < 0){return;}                             // Check for argument error.
  
  sc = argNumMinMax(0,59);                        // Get minutes.
  if(sc < 0){return;}                             // Check for argument error.

  arg = argOnOff();
  if(arg < 0){return;}
  
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), hr, mn, sc));
  EEPROM.write(eAddrDst,arg);
  readTime();
}

//#################################################################################################################
// SET DATE TIME METHOD
//#################################################################################################################
// Sets an port configured as an output High or Low.
void setDate()
{
  uint8_t dy  = 0;
  uint8_t mth  = 0;
  uint16_t yr  = 0;

  DateTime now = rtc.now();

  dy = argNumMinMax(1,31);                        // Get day.
  if(dy <= 0){return;}                            // Check for argument error.
  
  mth = argNumMinMax(1,12);                       // Get month.
  if(mth <= 0){return;}                           // Check for argument error.
  
  yr = argNumMinMax(1970,2099);                   // Get minutes.
  if(yr <= 0){return;}                            // Check for argument error.

  rtc.adjust(DateTime(yr, mth, dy, now.hour(), now.minute(), now.second()));  
  readTime();
}
 
//#################################################################################################################
// SET RTCs TEMPERATURE OFFSET METHOD
//#################################################################################################################
void setTOffset()                               // Set RTC's temperature offset.
{
  int8_t arg = argNumMinMax(-10,10);               // (min = -10, Max = +10).
  Serial.print(F("THE TEMPERATURE OFFSET IS SET TO "));
  Serial.print(arg);
  Serial.println(F(" DEGs"));
  EEPROM.write(eAddrTOffset,(arg));
}

//#################################################################################################################
// SET TEMPERATURE SCALE METHOD
//#################################################################################################################
void setTemprScale()
{
  int returnVal;
  returnVal = argcf();
  if (returnVal < 0){return;}                         // Syntax error or missing argument occured.
  else if (returnVal){Serial.println(F("TEMPERATURE SCALE IS SET TO CELSIUS"));}
  else{Serial.println(F("TEMPERATURE SCALE IS SET TO FAHRENHEIT"));}
  EEPROM.write(eAddrTemprScale,returnVal);
}

//#################################################################################################################
// ADD USER ID/TAG METHOD
//#################################################################################################################
// Adds an ID tag 
void addDbId()
{
// uint32_t idPwd = 0;
  uint32_t  pwd   = 0;
  Serial.println(F("ENTER A TAG ID TO BE ADDED OR PLACE A TAG ON THE READER"));
  Serial.println(F("TO ASSOCIATE A TAG TO A PASSWORD, FIRST ENTER A PASSWORD"));
  if(argNum(idPwd))
  {
    if(db.contains24(idPwd))                      // Checks to see if idTag is in the database.
    {
      if(db.posOf(idPwd) < PWDFLAG)  
      {
        Serial.print(F("ID TAG ALREADY IN DATABASE"));
        return;
      }
      else if(db.posOf(idPwd) >= PWDFLAG)
      {
        pwd = idPwd;
        Serial.print(F("NOW ENTER TAG ID"));
        if(argNum(idPwd))
        {
          if(!db.insertId(idPwd,pwd)){Serial.println(F("FAILED TO INSERT TAG ID, DATABASE MAY BE FULL"));}
          else
          {
            Serial.print(F("TAG "));
            Serial.print(idPwd);
            Serial.print(F(" STORED IN EEPROM WITH PASSWORD "));
            Serial.println(pwd);
            return;
          }
        }
      }
    }
    if(!db.insertId(idPwd)){Serial.println(F("FAILED TO INSERT TAG ID, DATABASE MAY BE FULL"));}
    else
    {
      Serial.print(F("TAG "));
      Serial.print(idPwd);
      Serial.println(F(" STORED IN EEPROM"));
    }
  }
  else{Serial.println(F("NO TAG ID OR PASSWORD ENTERED"));}
}

//#################################################################################################################
// ADD USER PASSWORD METHOD
//#################################################################################################################
void addDbPwd()
{
  // uint32_t idPwd = 0;
  uint32_t    tag   = 0;
  Serial.println(F("ENTER A PASSWORD TO BE ADDED"));
  Serial.println(F("TO ASSOCIATE A PASSWORD TO A TAG ID, FIRST ENTER THE TAG ID"));
  if(argNum(idPwd))
  {
    if(db.contains24(idPwd))                      // Checks to see if idTag is in the database.
    {
      if(db.posOf(idPwd) >= PWDFLAG)  
      {
        Serial.print(F("PASSWORD ALREADY IN DATABASE"));
        return;
      }
      else if(db.posOf(idPwd) < PWDFLAG)
      {
        tag = idPwd;
        idPwd = 0;
        Serial.print(F("NOW ENTER PASSWORD"));
        if(argNum(idPwd))
        {
          if(!db.insertPwd(tag,idPwd)){Serial.println(F("FAILED TO INSERT PASSWORD, DATABASE MAY BE FULL"));}
          else
          {
            Serial.print(F("PASSWORD "));
            Serial.print(idPwd);
            Serial.print(F(" STORED IN EEPROM WITH TAG ID "));
            Serial.println(tag);
            return;
          }
        }
      }
    }
    if(!db.insertPwd(idPwd)){Serial.println(F("FAILED TO INSERT PASSWORD, DATABASE MAY BE FULL"));}
    else
    {
      Serial.print(F("PASSWORD "));
      Serial.print(idPwd);
      Serial.println(F(" STORED IN EEPROM"));
    }
  }
  else{Serial.println(F("NO TAG ID OR PASSWORD ENTERED"));}
}

//#################################################################################################################
// ADD ID USER NAME METHOD
//#################################################################################################################
void addDbIdNam()
{
//  char *arg;
  char arg;
  static char sdata[NAMELENGTH], *psdata = sdata;
  uint32_t  tagId = 0;
  Serial.println(F("ENTER TAG NUMBER: "));
  if(argNum(tagId))
  {
    if(db.contains24(tagId))                      // Checks to see if idTag is in the database.
    {
      Serial.print(F("ENTER NAME FOR TAG NUMBER "));
      Serial.println(tagId);
      keyTmr = EEPROM.read(eAddrKeyTmr);          // Reset keypad timer.  
      while(keyTmr)                               // Loop until CR detected.
      {
        if(Serial.available() > 0)
        {
          keyTmr = EEPROM.read(eAddrKeyTmr);      // Reset keypad timer.  
          arg = Serial.read();
          if ((psdata - sdata) > NAMELENGTH)
          {
            psdata--; 
            Serial.println(F("NAME TOO LONG")); 
          }
          *psdata++ = (char)arg;
          if (arg == '\r' || arg == '\n')         // Command received and ready.
          {
            psdata--;                             // Don't add carrage return (\r) to string.
            *psdata = '\0';                       // Null terminate the string.
            break;
          }
        }
      }
      if (*sdata == '\0')
      {
        Serial.println(F("NO NAME ENTERED (TIMEOUT)"));
        return;
      }
      else
      {
        db.insertIdNam(tagId,sdata);
        Serial.print(F("NAME: \""));
        Serial.print(sdata);
        Serial.print(F("\" ADDED TO DATABASE FOR ID TAG "));
        Serial.println(tagId);
        psdata = sdata;                           // Needed to reset pointer before next name entry.
      }
    }
    else{Serial.println(F("TAG ID NOT FOUND IN DATABASE"));}
  }
  else{Serial.println(F("NO TAG ID ENTERED"));}
}

//#################################################################################################################
// ADD PASSWORD USER NAME METHOD
//#################################################################################################################
void addDbPwdNam()
{
//  char *arg;
  char arg;
//  static char pwdNam[NAMELENGTH], *pPwdNam = pwdNam;
  char pwdNam[NAMELENGTH];
  char *pPwdNam = pwdNam;
  uint32_t  pwd = 0;

  Serial.println(F("ENTER PASSWORD: "));
  if(argNum(pwd))
  {
    if(db.contains24(pwd))                        // Checks to see if password is in the database.
    {
      Serial.print(F("ENTER NAME FOR PASSWORD NUMBER "));
      Serial.println(pwd);
      keyTmr = EEPROM.read(eAddrKeyTmr);          // Reset keypad timer.  
      while(keyTmr)                               // Loop until CR detected.
      {
        if(Serial.available() > 0)
        {
          keyTmr = EEPROM.read(eAddrKeyTmr);      // Reset keypad timer.  
          arg = Serial.read();
          if ((pPwdNam - pwdNam) >= NAMELENGTH-1)
          {
            pPwdNam--; 
            Serial.println(F("NAME TOO LONG")); 
          }
          *pPwdNam++ = (char)arg;
          if (arg == '\r' || arg == '\n')         // Command received and ready.
          {
            pPwdNam--;                            // Don't add \r to string.
            *pPwdNam = '\0';                      // Null terminate the string.
            break;
          }
        }
      }
      if (*pwdNam == '\0')
      {
        Serial.println(F("NO NAME ENTERED (TIMEOUT)"));
        return;
      }
      else
      {
        db.insertPwdNam(pwd,pwdNam);
        Serial.print(F("NAME: \""));
        Serial.print(pwdNam);
        Serial.print(F("\" ADDED TO DATABASE FOR PASSWORD "));
        Serial.println(pwd);
      }
    }
    else{Serial.println(F("PASSWORD NOT FOUND IN DATABASE"));}
  }
  else{Serial.println(F("NO PASSWORD ENTERED"));}
}

//#################################################################################################################
// ADD USER PERMISSIONS METHOD
//#################################################################################################################
void addDbAtt()
{
//  uint8_t att = 0;
  uint32_t  tagId = 0;
  Serial.println(F("ENTER TAG NUMBER OR PASSWORD OF USER "));
  if(argNum(tagId))
  {
    if(!db.contains(tagId)){Serial.println(F("ID OR PASSWORD NOT FOUND IN DATABASE"));}
    else
    {
      uint8_t att =  argAtt();
      db.insertAtt(tagId,att);
      Serial.print(F("\nATTRIBUTES STORED IN EEPROM FOR "));
      Serial.println(tagId);
      displayAtt(att);
      Serial.println("");
      help();
    }
  }
  else{Serial.println(F("NO TAG ID OR PASWORD ENTERED"));}
}

//#################################################################################################################
// ADD TIME STAMP METHOD
//#################################################################################################################
// Sets the timestamp for a user ID or password so that temporary access can be provided based on the number
// of days the time stamp is set to.
// When set, the temporary access bit in the attibute byte for the user will also be set. 
// Note, when the timestamp expires, access for this user will be disabled. until a new timestamp is set.
void addDbTm()
{
  //  uint8_t att = 0;
  // uint32_t idPwd = 0;
  uint32_t    tm    = 0;
  
  Serial.println(F("ENTER TAG NUMBER OR PASSWORD OF USER "));
  if(argNum(idPwd))                               // gets a 32 bit number from serial port or keypad.
  {
    Serial.print(F("TAG OR PASSWORD NUMBER ENTERED = "));
    Serial.println(idPwd);
    if(!db.contains(idPwd))
    {
      Serial.println(F("ID OR PASSWORD NOT FOUND IN DATABASE"));
      return;
    }
    else
    {
      Serial.println(F("ENTER NUMBER OF DAYS FOR TEMPORARY ACCESS"));
      if (argNum(tm))
      {
        db.insertTm(idPwd, (timeStmp + (tm * 1440)));  // timestamp + (tagId * 60 mins. * 24Hrs). 
        pos = db.posOf(idPwd);
        db.readAtt(pos,att);
        att |= TEMPACCESS;                        // Enable temporary access.
        att &= ~ONETMACCESS;                      // Disable One time access (if enabled).
        att &= ~PERMACCESS;                       // Disable permanent access.
        db.insertAtt(idPwd,att);                  // Update attribute for user. 
      }
      Serial.println(F("NO TIME STAMP ENTERED"));
    }
  }
  else{Serial.println(F("NO TAG ID ENTERED"));}
}

//#################################################################################################################
// DELETE USER ID/TAG METHOD
//#################################################################################################################
void delDbId()
{
  uint32_t tagId  = 0;
  Serial.println(F("ENTER TAG NUMBER TO BE DELETED OR PLACE TAG ON READER "));
  if(argNum(tagId))
  {
    int16_t idLoc = db.posOf(tagId);
    if( idLoc >= 0 && idLoc < PWDFLAG)    
    {
      db.removeId(tagId);
      Serial.print(F("TAG ID "));
      Serial.print(tagId);
      Serial.println(F(" REMOVED FROM DATABASE."));
    }
    else
    {
      Serial.print(F("ID "));
      Serial.print(tagId);
      Serial.println(F(" NOT FOUND IN DATABASE"));
    }
  }
  else{Serial.println(F("NO TAG ID ENTERED"));}
}

//#################################################################################################################
// DELETE USER PASSWORD METHOD
//#################################################################################################################
void delDbPwd()
{
  uint32_t tagId = 0;
  Serial.println(F("ENTER PASSWORD TO BE DELETED "));
  if(argNum(tagId))
  {
    int16_t idLoc = db.posOf(tagId);
    if( idLoc >=  PWDFLAG)    
    {
      db.removePwd(tagId);
      Serial.print(F("PASSWORD "));
      Serial.print(tagId);
      Serial.println(F(" REMOVED FROM DATABASE."));
    }
    else
    {
      Serial.print(F("PASSWORD "));
      Serial.print(tagId);
      Serial.println(F(" NOT FOUND IN DATABASE"));
    }
  }
  else{Serial.println(F("NO PASSWORD ENTERED"));}
}

//#################################################################################################################
// DELETE ID USER NAME METHOD
//#################################################################################################################
void delDbIdNam()
{
  uint32_t  tagId = 0;
  //char name[NAMELENGTH];
  
  Serial.print(F("ENTER USER TAG ID: "));
  if(argNum(tagId))
  {
    Serial.println(tagId);
    Serial.print(F("TAGID OR PASSWORD = "));
    Serial.println(tagId);
    Serial.print(F("POSTION = "));
    pos = db.posOf(tagId);
    Serial.println(pos);
    if (pos >= 0 && pos < PWDFLAG)
    {
      db.readNam(pos,name);
      Serial.print(F("NAME: \""));
      Serial.print(name);
      Serial.print(F("\" DELETED FROM DATABASE FOR TAG "));
      Serial.println(tagId);
      db.removeIdNam(tagId);
    }
  }
  else{Serial.println(F("TAG ID NOT FOUND IN DATABASE"));}
}

//#################################################################################################################
// DELETE PASSWORD USER NAME METHOD
//#################################################################################################################
void delDbPwdNam()
{
  uint32_t  tagId = 0;
  //char name[NAMELENGTH];
  
  Serial.print(F("ENTER USER PASSWORD: "));
  if(argNum(tagId))
  {
    pos = db.posOf(tagId);
    if (pos >= PWDFLAG)
    {
      pos &= PWDMASK;
      db.readNam(pos,name);
      Serial.print(F("NAME: \""));
      Serial.print(name);
      Serial.print(F("\" DELETED FROM DATABASE FOR PASSWORD "));
      Serial.println(tagId);
      db.removePwdNam(tagId);
    }
  }
  else{Serial.println(F("PASSWORD NOT FOUND IN DATABASE"));}
}

//#################################################################################################################
// DELETE USER PERMISSIONS METHOD
//#################################################################################################################
void delDbAtt()
{
  uint32_t  tagId = 0;
  
  Serial.println(F("ENTER TAG NUMBER OR PASSWORD OF USER "));
  if(argNum(tagId))                               // function stores result in variable "tagId".
  {
    if(!db.contains(tagId)){Serial.println(F("ID OR PASSWORD NOT FOUND IN DATABASE"));}
    else
    {
      db.insertAtt(tagId,0);
      Serial.print(F("ATTRIBUTES FOR ID TAG/PASWORD \""));
      Serial.print(tagId);
      Serial.println(F("\" DELETED"));
    }
  }
  else{Serial.println(F("NO TAG ID ENTERED"));}
}

//#################################################################################################################
// MODIFY USER ID/TAG METHOD
//#################################################################################################################
void modDbId()
{
  //  uint32_t  idPwd = 0;
  //  char      name[NAMELENGTH];
  uint32_t      oldId = 0;
  uint32_t      newId = 0;

  Serial.println(F("ENTER OLD TAG NUMBER OR PLACE TAG ON READER "));
  if(argNum(oldId))
  {
    pos = db.posOf(oldId);
    if( pos >= 0 && pos <= PWDFLAG)    
    {
      Serial.print(F("OLD TAG ID \""));
      Serial.print(oldId);
      Serial.print(F("\" FOUND FOR USER "));
      db.readNam(pos, name);
      Serial.println(name);
      Serial.println(F("ENTER NEW TAG NUMBER OR PLACE TAG ON READER"));
      if(argNum(newId))
      {
        db.modifyIdPwd(pos, newId);
        Serial.print(F("TAG ID "));
        Serial.print(oldId);
        Serial.print(F(" REPLACED BY TAG ID "));
        Serial.println(newId);
      }
      else{Serial.println(F("NO TAG ID ENTERED"));}
    }
    else
    {
      Serial.print(F("TAG ID \""));
      Serial.print(oldId);
      Serial.println(F("\" NOT FOUND IN DATABASE"));
    }
  }
}

//#################################################################################################################
// DELETE USER PASSWORD METHOD
//#################################################################################################################
void modDbPwd()
{
  //  char      name[NAMELENGTH];
  //  uint32_t  idPwd   = 0;
  uint32_t      newPwd  = 0;
  uint32_t      tempPwd = 0;

  Serial.println(F("ENTER OLD PASSCODE "));
  if(argNum(idPwd))
  {
    pos = db.posOf(idPwd);
    if(pos > PWDFLAG)    
    {
      Serial.print(F("OLD PASSCODE \""));
      Serial.print(idPwd);
      Serial.print(F("\" FOUND FOR USER "));
      db.readNam(pos & PWDMASK, name);
      Serial.println(F("ENTER NEW PASSCODE "));
      if(argNum(newPwd))
      {
        Serial.println(F("REENTER NEW PASSCODE..."));
        if(argNum(tempPwd))
        {
          if(newPwd == tempPwd)
          {
            Serial.print(F("OLD PASSCODE REPLACED BY "));
            Serial.print(newPwd);
            Serial.print(F(" FOR USER "));
            db.readNam(pos & PWDMASK, name);
            Serial.println(name);
            db.modifyIdPwd(pos, newPwd);
          }
          else{Serial.println(F("NEW PASSCODE ENTERED DID NOT MATCH"));}
        }
        else{Serial.println(F("PASSCODE NOT ENTERED A 2ND TIME"));}
      }
      else{Serial.println(F("NEW PASSCODE NOT ENTERED"));}
    }
    else{Serial.println(F("PASSCODE NOT FOUND IN DAPTABASE"));}
  }
  else{Serial.println(F("PASSCODE NOT ENTERED"));}
}

//#################################################################################################################
// DELETE ID USER NAME METHOD
//#################################################################################################################
void modDbIdNam()
{
  addDbIdNam();

//  uint32_t  idPwd = 0;
//  char      name[NAMELENGTH];
//  char      *name;
//  char      oldNam[db.maxNameLength()];
//  char      newNam[db.maxNameLength()];
  
//  Serial.println(F("ENTER A TAG ID, OR PLACE A TAG ON THE READER"));
//  if(argNum(idPwd))
//  {
//    pos = db.posOf(idPwd);
//    if(pos < PWDMASK)
//    {
//      db.readNam(pos,oldNam);
//      Serial.print(F("ENTER NEW USER NAME: "));
//      argNam(name);                                                 // Get new password name.
//      Serial.print(F("FOR USER # "));
//      Serial.print(pos);
//      Serial.print(F("NAME CHANGED FROM "));
//      Serial.print(oldNam);
//      Serial.print(F(" TO "));
//      Serial.println(name);
      
//      db.modifyNam(pos,name);
//    }
//    else{Serial.println(F("ID NOT FOUND IN DATABASE"));}
//  }
}



//#################################################################################################################
// DELETE PASSWORD USER NAME METHOD
//#################################################################################################################
void modDbPwdNam()
{
  addDbPwdNam();
//  char oldNam[NAMELENGTH];
//  char newNam[NAMELENGTH];
  
//  Serial.print(F("ENTER USER PASSWORD: "));
//  if(argNum(idPwd))
//  {
//    pos = db.posOf(idPwd);
//    if(pos >= PWDMASK)
//    {
//      pos &= PWDMASK;                                           // Remove password flag.
//      db.readNam(pos,oldNam);
//      Serial.print(F("ENTER NEW USER NAME: "));
//      argNam(newNam);                                                 // Get new password name.
//      Serial.print(F("FOR USER # "));
//      Serial.print(pos);
//      Serial.print(F("NAME CHANGED FROM "));
//      Serial.print(oldNam);
//      Serial.print(F(" TO "));
//      Serial.println(newNam);
//      
//      db.modifyNam(pos,newNam);
//    }
//    else{Serial.println(F("PASSWORD NOT FOUND IN DATABASE"));}
//  }
}


//#################################################################################################################
// MENU COMMAND METHOD
//#################################################################################################################
void menu()                             // Display commands.
{
  Serial.println(F("COMMAND (HELP) MENU..."));
  Serial.println(F("rvb or RVB\t\t\tDISPLAY ALL PARAMETERS AND SETTINGS"));
  Serial.println(F("rar or RAR\t\t\tDISPLAY KEYPAD RETRY COUNT BEFORE LOCKOUT, DEFAULT = 3"));
  Serial.println(F("rkt or RKT\t\t\tDISPLAY KEYPAD TIMEOUT DELAY, DEFAULT = 30 SEC"));
  Serial.println(F("rud or RUD\t\t\tDISPLAY THE UNLOCK DELAY, DEFAULT = 5 SECONDS"));
  Serial.println(F("rgs or RGS\t\t\tDISPLAY GARAGE DOOR POSITION STATUS AND TIMER"));
  Serial.println(F("rkp or RKP\t\t\tDISPLAY KEYPAD DATA OR SCANNED ID"));
  Serial.println(F("rip or RIP <1-999>\t\tDISPLAYS THE ID OR PASSWORD FOR A USER NUMBER, 999 DISPLAYS ALL USERS"));
  Serial.println(F("rdn or RDN <ID TAG>\t\tDISPLAYS THE USER NAME FOR A GIVEN ID NUMBER OR PASSWORD"));
  Serial.println(F("rle or RLE\t\t\tDISPLAY LAST ERROR CODE RECORDED"));
  Serial.println(F("rep or REP\t\t\tDISPLAYS INTERNAL EEPROM CONTENTS"));
  Serial.println(F("rtm or RTM\t\t\tDISPLAYS RTC TIME/DATE AND TEMPERATURE"));
  Serial.println(F("rto or RTO\t\t\tDISPLAYS RTC's TEMPERATURE OFFEST VALUE, DEFAULT = 0 DEGs"));
  Serial.println(F("rot or ROT\t\t\tDISPLAYS THE OLED OFF TIMER, DEFAULT = 10 SECONDS"));
  Serial.println("");
  Serial.println(F("svb or SVB <ON-OFF>\t\tSET VERBOSE DISPLAY ON OR OFF (REFRESH RATE EVERY SECOND)"));
  Serial.println(F("stm or STM <HH MM SS>\t\tSETS THE RTC's TIME"));
  Serial.println(F("sdt or SDT <DD MM YY>\t\tSETS THE RTC's DATE"));
  Serial.println(F("sto or STO <DEG>\t\tSETS THE RTC's TEMPERATURE OFFSET VALUE IN DEG's C (RANGE IS 10 to + 10)"));
  Serial.println(F("skt or SKT <1-240>\t\tSET KEYPAD TIMEOUT, DEFAULT = 20 SEC."));
  Serial.println(F("sar or SAR <1-5>\t\tSET ACCESS CODE RETRY COUNT BEFORE LOCKOUT OCCURS (DEFAULT = 3)"));
  Serial.println(F("slk or SLK <1-240>\t\tSET UNLOCK DELAY TIME, DEFAULT = 5 SECONDS."));
  Serial.println(F("sgt or SGT <1-240>\t\tSET GARAGE DOOR LOCK TIME, DEFAULT = 30 MINUTES"));
  Serial.println(F("sot or SOT <1-240>\t\tSET OLED OFF TIMER, DEFAULT = 10 SECONDS"));
  Serial.println(F("sts or STS <C/F>\t\tSET THE TEMPERATURE SCALE"));
  Serial.println(F("sgs or SGS <ON/OFF>\t\tENABLES/DISABLES GARAGE DOOR POSITION SENSORS"));
  Serial.println(F("\t\t\t\tNOTE: IF SENSORS ARE DISABLED, GARAGE DOOR TIMER and CLOSING DOOR USING # KEY IS ALSO DISABLED"));
  
  Serial.println("");
  Serial.println(F("adi or ADI <Tag ID>\t\tADD ID NUMBER TO DATABASE"));
  Serial.println(F("adp or ADP <PWD>\t\tADD PASSWORD TO DATABASE"));
  Serial.println(F("\t\t\t\tNOTE: IF TAG ID IS ENTERED FIRST AND IS ALREADY IN DATABASE, THE PASSWORD WILL BE ASSOCIATED TO TAG ID"));
  Serial.println(F("ain or AIN <Tag ID> <ID Name>\tADD USER NAME TO ID TAG"));
  Serial.println(F("apn or APN <PWD> <ID Name>\tADD USER NAME TO PASSWORD"));
  Serial.println(F("ada or ADA <Tag ID OR PWD>\tADDS USER PERMISSIONS TO ID TAG OR PASSWORD"));
  Serial.println(F("adt or ADT <Tag ID OR PWD>\tADDS TIME STAMP TO ID TAG OR PASSWORD FOR TEMPORARY ACCESS"));
  Serial.println("");
  Serial.println(F("ddi or DDI <Tag ID>\t\tDELETE USER ID"));
  Serial.println(F("ddp or DDP <PWD>\t\tDELETE USER PASSWORD IN DATABASE"));
  Serial.println(F("din or DIN <Tag ID>\t\tDELETE ID TAG NAME"));
  Serial.println(F("dpn or DPN <PWD>\t\tDELETE PASSWORD NAME"));
  Serial.println(F("dda or DDA <Tag ID/PWD>\t\tDELETE PERMISSIONS FOR A USER ID OR PASSWORD"));
  Serial.println("");
  Serial.println(F("mdi or MDI <TAG ID>\t\tMODIFY TAG ID FOR A USER"));
  Serial.println(F("mdp or MDP <PWD>\t\tMODIFY A PASSORD FOR A USER"));
  Serial.println(F("min or MIN <TAG ID>\t\tMODIFY USER NAME FOR A SPECIFIC USER USING THE ID TAG"));
  Serial.println(F("mpn or MPN <PWD>\t\tMODIFY USER NAME FOR A SPECIFIC USER USING THE PASSWORD"));
  

  SCmd.addCommand("min", modDbIdNam);             // Modifies a user name in database for a given id number.
  SCmd.addCommand("mpn", modDbPwdNam);            // Modifies a user name in database for a given password.

  
  Serial.println("");
  Serial.println(F("cdb or CDB <Y/N>\t\tCLEAR USER DATABASE"));
  Serial.println(F("ccf or CCF <Y/N>\t\tCLEAR CONFIGURATION (RFID DATABASE IS UNCHANGED)"));
  Serial.println(F("cep or CEP <Y/N>\t\tCLEAR USER ALL EEPROM LOCATIONS"));
  //
  Serial.println("");
  Serial.println(F("rst or RST\t\t\tRESETS THE RFID CONTROLLER"));
  Serial.println(F("menu or ?\t\t\tHELP MENU"));
}

//#################################################################################################################
// ARGUMENT ON/OFF TEST METHOD
//#################################################################################################################
int8_t argOnOff()
{
  char *arg;
  arg = SCmd.next();                              // Get the next argument from the SerialCommand object buffer

  if (arg != NULL)
  {
    if (strcasecmp(arg, "on") == 0){return 1;}
    else if (strcasecmp(arg, "off") == 0){return 0;}
    else
    {
      syntaxError();
      return -1;
    }
  }
  else
  {
    missingArg();
    return -2;
  }
}

//#################################################################################################################
// ARGUMENT SELECTION YES/NO METHOD
//#################################################################################################################
int8_t argyn()
{
  char *arg;  

  arg = SCmd.next();                                  // Get the next argument from the SerialCommand object buffer
  if (arg != NULL)
  {
    if (strcasecmp(arg, "n") == 0){return 0;}         // NO selected.

    else if (strcasecmp(arg, "y") == 0){return 1;}    // YES selected.
    else
    {
      syntaxError();
      return -1;
    }
  }  
 else
  {
    missingArg();
    return -2;
  }
}

//#################################################################################################################
// ARGUMENT SELECTION FAHRENHEIT/CELSIUS METHOD
//#################################################################################################################
int argcf()
{
  char *arg;  

  arg = SCmd.next();                                  // Get the next argument from the SerialCommand object buffer
  if (arg != NULL)
  {
    if (strcasecmp(arg, "f") == 0){return 0;}         // Fahrenheit scale selected.

    else if (strcasecmp(arg, "c") == 0){return 1;}    // Celsius scale selected.
    else
    {
      syntaxError();
      return -1;
    }
  }  
 else
  {
    missingArg();
    return -2;
  }
}

//#################################################################################################################
// ARGUMENT NUMBER METHOD
//#################################################################################################################
// Waits for a numerical input and returns a 32 bit number or error if outside the range of 0-9,294,295,967
// Could have used the Serial.parse method but could not establish input limits.
bool argNum(uint32_t &tagId)
{
  uint32_t arg = 0;
  
  for (int8_t i=0; i<10; i++)                     // Gets 10 digit (32bit) ID Tag.
  {  
    while(Serial.available() == 0)                // do until CR detected.
    {
      if(wg.available())
      {
        if(wg.getWiegandType() == 26 || wg.getWiegandType() == 34)
        {
          tagId = wg.getCode();
          return 1;                               // TagID received so exit.
        }
        else if(wg.getWiegandType() == 4)
        {
          arg =  wg.getCode(); // larger variable to make math easier.
          if (arg == '#'){return true;}
          else{tagId = tagId * 10UL + arg;} 
          break;                                  // Exits "WHILE" loop back to "FOR" loop above.
        }
      }
    }
    
    arg = Serial.read();                          // get the character.
    if (arg == '\r'){break;}                      // CR detected so exit serial input.
    else if ((arg >= '0') && (arg <= '9')){tagId = tagId * 10UL + arg - 48UL;}
    else{i--;}                          // Move character counter"i" back one so non numeric characters are ignored.
  }

  if(tagId){return 1;}                            // Return successful status.
  else
  {
    tagId = 0;
    return 0;
  }
}

//#################################################################################################################
// GET ARGUMENT WITH RANGE METHOD
//#################################################################################################################
// Waits for number input. Checks to see if it falls in the specified range set by the calling method and returns
// number. If the range is outside the number range specified, an error is returned.
//
int16_t argNumMinMax(uint16_t argMin, uint16_t argMax)
{
  uint16_t  aNumber = 0;
  char      *arg;

  arg = SCmd.next();
  if (arg != NULL)
  {
    aNumber=atoi(arg);                            // Converts a char string to an integer
    if (aNumber >= argMin || aNumber <= argMax)
    {
      return aNumber;
    }
    else
    {
      Serial.println(F("Value out of range"));
      return -128;
    }
  }
  else
  {
    Serial.println(F("No arguments"));
    return 0;
  }
}

//#################################################################################################################
// ARGUMENT ATTRUIBUTE METHOD
//#################################################################################################################
// Waits for a numerical input and returns a 16 bit number or error if outside the range of 0-4,294,967,295
// Could have used the Serial.parse method but could not establish input limits.
int16_t argAtt()
{
//  uint8_t att = 0;
  uint8_t arg = 0;
  
  while (arg != '#')
  {  
    Serial.println(F("ATTRIBUTE OPTIONS"));
    Serial.println(F("================="));
    for (int i = 0; i < NUMBER_OF_ITEMS; i++)
    {
      printAttList ((const char *) &attList [i]);
      Serial.println("\n");
    }
    Serial.println(F("ENTER 1-8 TO SELECT ATTIBUTE(S),# TO EXIT:"));
    while(Serial.available() == 0)                // do until CR detected.
    {
      if(wg.available())
      {
        if(wg.getWiegandType() == 4)
        {
          arg =  wg.getCode();                    // larger variable to make math easier.
          if (arg == '#'){return att;}
          else if((arg >= 1) && (arg <= 8)){att |= (1 << (arg-1));}        // Set permission bit.
        }
      }
    }
    arg = Serial.read();                          // get the character.
    if (arg == '#'){return att;}                  // CR detected so exit serial input. 
    else if ((arg >= '1') && (arg <= '8')){att |= (1 << (arg - 49));} // convert from ASCII to int and offset by -1.
  }
}

//#################################################################################################################
// ARGUMENT NAME METHOD
//#################################################################################################################
// Waits for name to be entered.
bool argNam(char name)
{
  char *arg;

  arg = SCmd.next();
  if (arg != NULL)
  {
    name = *arg;                                 // Place id nane in global variable.
    return true;
  }
  else
  {
    Serial.println(F("No arguments entered"));
//    name = NULL;
    name = '\0';
    return false;
  }
}

//#################################################################################################################
// UNRECOGNIZED COMMEND METHOD
//#################################################################################################################
void syntaxError()                                // Unregognized command.
{
  Serial.println(F("Command Syntax Error, Please re-enter command"));
}

//#################################################################################################################
// MISSING ARGUMENT METHOD
//#################################################################################################################
void missingArg()                                 // Incorrect argument.
{
  Serial.println(F("Missing argument"));
}

//#################################################################################################################
// LED SELFTEST METHOD
//#################################################################################################################
void ledSelfTest()
{
  Serial.print(F("LED SELFTEST STARTED..."));

  for (uint8_t i = 0; i < 3; i++)
  {
    digitalWrite((rLedPin - i),!digitalRead(rLedPin - i));
    delay(1000);
    digitalWrite((rLedPin - i),!digitalRead(rLedPin - i));
  }
  Serial.println(F("LED SELFTEST COMPLETED"));
}

//#################################################################################################################
// BEEPER SELFTEST METHOD
//#################################################################################################################
void bprSelfTest()
{
  Serial.print(F("BEEPER SELFTEST STARTED..."));
// SELFTEST SEQUENCE ----------------------------------------------------------------------------------------------
  for(uint8_t i = 1; i == 3; i++)
  {
    digitalWrite(frtBprPin, ONINV);               // Turn on keypad BEEPER.
    delay (1000);
    digitalWrite(frtBprPin, OFFINV);              // Turn off keypad BEEPER.
    
    digitalWrite(garBprPin, ONINV);               // Turn on keypad BEEPER.
    delay (1000);
    digitalWrite(garBprPin, OFFINV);              // Turn off keypad BEEPER.
    
    digitalWrite(rearBprPin, ONINV);              // Turn on keypad BEEPER.
    delay (1000);
    digitalWrite(rearBprPin, OFFINV);             // Turn off keypad BEEPER.
  }  
  Serial.println(F("BEEPER SELFTEST COMPLETED"));
}

//#################################################################################################################
// PRINT BITS WITH LEADING ZEROS METHOD
//#################################################################################################################
// prints raw keypad value with leading zeros, N-bit integer in this form: 0000 0000 0000 0000
// works for 4 - 32 bits
// accepts signed numbers
void printBits(uint32_t n, uint8_t numBits) 
{
  char b;
  
  for (uint8_t i = 0; i < (numBits); i++)         // shift 1 and mask to identify each bit value
  {
    b = (n & (1 << (numBits - 1 - i))) > 0 ? '1' : '0'; // slightly faster to print chars than ints (saves conversion)
    Serial.print(b);
    if (i < (numBits - 1) && ((numBits-i - 1) % 4 == 0 )){Serial.print(" ");} // print a separator at every 4 bits
  }
}

//#################################################################################################################
// PRINT DECIMAL WITH LEADING ZEROS METHOD
//#################################################################################################################
// prints decimal number with leading zero's
void printDecimal(uint32_t idPwdTm) 
{
  uint8_t   numOfDig  = 0;
  uint32_t  n         = idPwdTm;
  do
  {
      n/=10;
      numOfDig++;
  }while (n != 0);
  for (uint8_t i = 1; i <= 10 - numOfDig;i++){Serial.print("0");}
  Serial.print(idPwdTm);  
}

//#################################################################################################################
// PIN CHANGE INTERRUPT METHOD
//#################################################################################################################
ISR (PCINT0_vect)                                 // Interrupt vector for PCINT0-7 (Pins D8-D13)
{
  if(!digitalRead(frtDrKeyPdPin)){keyPdLoc = FRTDRKEYPD;}
  else if(!digitalRead(garDrKeyPdPin)){keyPdLoc = GARDRKEYPD;}
  else if(!digitalRead(rearDrKeyPdPin)){keyPdLoc = REARDRKEYPD;}
  
//  if(~PINB & B00100000){keyPdLoc = FRTDRKEYPD;}
//  else if(~PINB & B01000000){keyPdLoc = GARDRKEYPD;}
//  else if(~PINB & B10000000){keyPdLoc = REARDRKEYPD;}
}

//#################################################################################################################
// TIMER 1 INTERRUPT SERVICE METHOD (FOR MAIN TIMEBASE)
//#################################################################################################################
// Time base for ISR is set to 120Hz to allow lock solenoid to cycle ON/OFF at 60Hz.
ISR(TIMER1_OVF_vect)                              // interrupt service routine 
{
  // If door unlock sequence active, simulate 60Hz AC using PWM square wave)
  if(frtDrLckDlyTmr){digitalWrite(frtDrLckPin,(!digitalRead(frtDrLckPin)));}       // Unlock front door with 60Hz.
  if(rrDrLckDlyTmr){digitalWrite(rearDrLckPin,(!digitalRead(rearDrLckPin)));}      // Unlock rear door with 60Hz.
  if(shdDrLckDlyTmr){digitalWrite(shedDrLckPin,(!digitalRead(shedDrLckPin)));}     // Unlock shed door with 60Hz.

  if(tenHzTimer){tenHzTimer--;}
  else
  {
    tenHzTick = ON;
    tenHzTimer = TENHZTIMERDEFAULT;               // Reset ten hertz timer.
    if(digitalRead(pirPin))
    {
      pirFlag = ON;
//      digitalWrite(bLedPin, ON);
    }
//    else{digitalWrite(bLedPin, OFF);}
  }
  
  if (oneHzTimer){oneHzTimer--;}
  else
  {
    oneHzTick = ON;                               // Used when verbose is set to "ON".
    oneHzTimer = ONEHZTIMERDEFAULT;               // Reset one Hz timer.
    if (keyTmr){keyTmr--;}                        // Keypad timer resets to 30 sec. when key is pressed.
    if(frtDrLckDlyTmr){frtDrLckDlyTmr--;}         // Unlock front door for the duration of the unlock delay timer.
    if(rrDrLckDlyTmr){rrDrLckDlyTmr--;}           // Unlock rear door for the duration of the unlock delay timer.
    if(shdDrLckDlyTmr){shdDrLckDlyTmr--;}         // Unlock shed door for the duration of the unlock delay timer.

    if (oneMnTimer){oneMnTimer--;}
    else
    {
      oneMnTimer = ONEMNTIMERDEFAULT;
      if(garDrTmr){garDrTmr--;}                   // Decrement garage door timer once per minute.
      timeStmp++;                                 // Timer used to trigger temporary permission.
      oneMnTick = ON;                             // Usede to check DST for DS3231 RTC chip.
    }                                             // Increments every minute.

    if (runState == NORMAL)
    {
      digitalWrite(pwrLedPin,(!digitalRead(pwrLedPin)));
    }
    else if (runState == PROGRAM)
    {
      if (progTimer){progTimer--;}                // If in diagnostic or config mode, decrement timeout period.
    }
  }
  encoder->service();                             // Maintain encoder
}


//#################################################################################################################
// ERROR LOG METHOD
//#################################################################################################################
void logErr(uint8_t errNum)
{
  EEPROM.write(eAddrErrCode, errNum);             // Record last error.
    errorTone();                                  // Sound error tone.
}

//#################################################################################################################
// DUMP USER INFO METHOD
//#################################################################################################################
// Displays User ID, Password and attributes with the user's position number in the database.
void userInfo(int16_t user) 
{
  char      name[NAMELENGTH];
  uint8_t   att     = 0;
  uint32_t  idPwdTm = 0;

  if (user >= db.count()){Serial.println(F("USER NOT IN DATABASE"));}
  {
    Serial.print(F("["));
    Serial.print(F("USER NO. = "));
    Serial.print(user);

    db.readAtt(user,att);
    Serial.print(F("\tATTRIBUTE = "));
    printBits(att,8);

    db.readId(user, idPwdTm);
    Serial.print(F("\tID = "));
    printDecimal(idPwdTm);

    db.readPwd(user,idPwdTm);
    Serial.print(F("\t\tPASSWORD = "));
    printDecimal(idPwdTm);
    
    db.readNam(user, name);
    Serial.print(F("\tNAME = "));
    att = strlen(name);
    att = NAMELENGTH - att -1;
    Serial.print(name);
    for(uint8_t i = 0;i < att;i++){Serial.print(F(" "));}
    
    db.readTm(user,idPwdTm);
    Serial.print(F("\t\tTIMESTAMP = "));
    printDecimal(idPwdTm);

    Serial.println(F("]"));
  }
}

//#################################################################################################################
// EEPROM DUMP METHOD
//#################################################################################################################
//Displays the contents of the Arduino's internal EEPROM.
void eepromDump()
{
  char ascChar;                                   // ASCII character to be displayed.
  uint16_t j = 15;                                // Display 16 characters per line.
  Serial.println();
  Serial.println(F("\t\t0x00\t0x01\t0x02\t0x03\t0x04\t0x05\t0x06\t0x07\t0x08\t0x09\t0x0A\t0x0B\t0x0C\t0x0D\t0x0E\t0x0F"));
  Serial.print("0x0000\t\t");
  for(uint16_t i=0; i < EEPROMSIZE; i++)
  {
    if (EEPROM.read(i) < 0x10) {Serial.print("0");}
    Serial.print(EEPROM.read(i),HEX);
    Serial.print("\t");
    if(i >= j)                                    // Check for 16 byte boundry. 
    {
      Serial.print("[");
      for(j = i-15; j <= i; j++)
      {
        ascChar = EEPROM.read(j);                 // Get characters for row to be displayed.
        if((ascChar >= 48 && ascChar <= 90) || (ascChar >= 97 && ascChar <= 122)){Serial.print(ascChar);}
        else{Serial.print(".");}
      }
      Serial.println("]");
      j+=15;
      Serial.print("0x");
      if(i+1 < 0x10){Serial.print("000");}  
      else if(i+1 < 0x100){Serial.print("00");}  
      else if(i+1 < 0x1000){Serial.print("0");}  
      Serial.print(i+1,HEX);
      Serial.print("\t\t");
    }
  }
  Serial.println();
}

//#################################################################################################################
// DISPALY ATTRIBUTE LIST METHOD
//#################################################################################################################
void displayAtt(uint8_t att)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (bitRead (att, i))
    {
      printAttList ((const char *) &attList [i]); // Displays attribute bit description.
      Serial.println("");
    }
  }
}

//#################################################################################################################
// PRINT ATTRIBUTE LIST METHOD
//#################################################################################################################
// Print a string from Program Memory directly to save RAM 
void printAttList(const char * str)
{
  char c;
  if (!str){return;}                            // Check for NULL character.
  while ((c = pgm_read_byte(str++))){Serial.print (c);}
}

//#################################################################################################################
// PRINT ATTRIBUTE LIST METHOD
//#################################################################################################################
// Print a string from Program Memory directly to save RAM 
void displayPerm()
{
  for (uint8_t i = 0; i < NUMBER_OF_PERMS; i++)
  {
    Serial.print(F("\t\t\t    "));
    printAttList ((const char *) &attPerm [i]);
    Serial.println();
  }
    Serial.println();
}

//#################################################################################################################
// PRINT ATTRIBUTE LIST METHOD
//#################################################################################################################
void checkLocks()
{
  if(drUnlockFlag)
  {
    if(!frtDrLckDlyTmr && (drUnlockFlag & FRTDRLOCK))
    {
      digitalWrite(frtDrLckPin,LOCK);
      Serial.println(F("FRONT DOOR LOCKED"));
      drUnlockFlag &= ~FRTDRLOCK;                 // Turn off flag to show door is now locked.
      digitalWrite(frtLedPin,OFFINV);             // Turn Front door access keypad LED to red.
      digitalWrite(frtBprPin,OFFINV);             // Turn Front door access keypad beeper.
     digitalWrite(gLedPin, OFF);                  // Turn on GREEN control panel Status LED.
   }

    if(!rrDrLckDlyTmr && (drUnlockFlag & REARDRLOCK))
    {
      digitalWrite(rearDrLckPin,LOCK);
      Serial.println(F("REAR DOOR LOCKED"));
      drUnlockFlag &= ~REARDRLOCK;                // Turn off flag to show door is now locked.
      digitalWrite(frtLedPin,OFFINV);             // Turn Front door access keypad LED to red.
      digitalWrite(frtBprPin,OFFINV);             // Turn off front door access keypad beeper.
      digitalWrite(rearLedPin,OFF);               // Turn rear door access keypad LED to red.
      digitalWrite(rearBprPin,OFFINV);            // Turn off rear door access keypad beeper.
      digitalWrite(gLedPin, OFF);                 // Turn on GREEN control panel Status LED.
    }

    if(!shdDrLckDlyTmr && (drUnlockFlag & SHEDDRLOCK))
    {
      digitalWrite(shedDrLckPin,LOCK);
      Serial.println(F("SHED DOOR LOCKED"));
      drUnlockFlag &= ~SHEDDRLOCK;                // Turn off flag to show door is now locked.
      digitalWrite(frtLedPin,OFFINV);             // Turn Front door access keypad LED to red.
      digitalWrite(frtBprPin,OFFINV);             // Turn off front door access keypad beeper.
      digitalWrite(shedLedPin,OFF);               // Turn rear door access keypad LED to red.
      digitalWrite(shedBprPin,OFFINV);            // Turn off shed door access keypad beeper.
      digitalWrite(gLedPin, OFF);                 // Turn on GREEN control panel Status LED.
    }
  }
  if(EEPROM.read(eAddrGarDrSn))                   // Garage door sensors enabled, so check garage door timer.
  {
    if(!garDrTmr && (drUnlockFlag & GARDRLOCK) && digitalRead(garDrUpSwPin) == ONINV)
    {
      readTmDt();
      Serial.print(F("GARAGE DOOR TIMEOUT, "));   // Flag that door was closed due to timer at 0.
      drUnlockFlag &= ~GARDRLOCK;                 // Turn off  "DISABLE GARAGE DOOR TIMER" flag.
      garDrCntl();
    }
  }
}

//#################################################################################################################
// GARAGE DOOR CONTROL METHOD
//#################################################################################################################
void garDrCntl()
//bool garDrCntl()
{
  // if door is closed or door is in mid-way postion, then turn on garage door flag to show door was opened.
  if(digitalRead(garDrDnSwPin) == ONINV)
  {
    drUnlockFlag |= GARDRLOCK;
    garDrTmr = EEPROM.read(eAddrGarDrTmr);
  }
  
  digitalWrite(bLedPin,ON);                       // Turn on BLUE status LED.
  digitalWrite(garDrOpnPin,1);                    // Turn on relay.
  delay (500);                
  digitalWrite(bLedPin,OFF);                      // Turn off BLUE status LED.
  digitalWrite(garDrOpnPin,0);                    // Turn off relay.
  delay (1500);                                   // Give time for door position switch to change state.          
}   

//#################################################################################################################
// GARAGE DOOR WALL SWITCH SINGLE PRESS METHOD
//#################################################################################################################
// Turns off garage door timer so door always stays open until the next open/close cycle.
void singlePressGarDrWalSw()
  {
    garDrCntl();
    readTmDt();
    Serial.print(F("GARAGE DOOR WALL SWITCH PRESSED, "));
  }
  
//#################################################################################################################
// GARAGE DOOR WALL SWITCH LONG PRESS METHOD
//#################################################################################################################
// Turns off garage door timer so door always stays open until the next open/close cycle.
void longPressGarDrWalSw()
{
  garDrCntl();
  if(EEPROM.read(eAddrGarDrSn))                   // Garage door sensors enabled, allow timer to be disabled.
  {
    readTmDt();
    Serial.print(F("GARAGE DOOR WALL SWITCH LONG PRESS"));
    Serial.println(F(", GARAGE DOOR TIMER IS DIABLED"));
    garDrTmr =0;
    drUnlockFlag &= ~GARDRLOCK;                   // Turn off garage door flag to show timer is off;
  }
}

//#################################################################################################################
// CHECK SWITCH METHOD
//#################################################################################################################
// Updates the status of all switch inputs and performs the appropriate action if pressed.
// Monitors the Garage wall switch, Intercom unlock switch, Intercom auxilary switch, Intercom bell
// button switch, RFID bell button switch for press event and performsrequired action if pressed.
void  checkBtn()                                  // Update all Button instances.
{
  frtBelBtn.read();
  garDrWalSw.read();
  garBelBtn.read();
  garDrDnSw.read();
  garDrUpSw.read();
  rearBelBtn.read();
  intDrBtn.read();
  intAuxBtn.read();
  
  
  if (frtBelBtn.wasPressed())                     // RFID "BELL" button pressed, Ring front door bell.
  {
    readTmDt();
    Serial.println(F("FRONT RFID BELL BUTTON PRESSED"));
    ringFrtBel();
  }
  
  if (garBelBtn.wasPressed())                     // Garage RFID "BELL" button pressed.Ring front door bell.
  {
    readTmDt();
    Serial.println(F("GARAGE RFID BELL BUTTON PRESSED"));
    ringFrtBel();
  }
  
  if (rearBelBtn.wasPressed())                    // Rear RFID "BELL" button pressed, Ring front door bell.
  {
    readTmDt();
    Serial.println(F("REAR RFID BELL BUTTON PRESSED"));
    ringRearBel();
  }
  
  if (intDrBtn.wasPressed())                      // Unlock front door.
  {
    readTmDt();
    Serial.println(F("INTERCOM DOOR BUTTON PRESSED"));
    unlockFrtDr();
  }
  
  if (intAuxBtn.wasPressed())                     // Intercom Aux sw pressed, Open/Close garage door.
  {
    readTmDt();
    Serial.println(F("INTERCOM AUX BUTTON PRESSED"));
//    garDrCntl();
  }
  
//  if (garDrWalSw.wasPressed())                    // Garage wall sw pressed, Open/Close garage door.
//  {
//    garDrCntl();
//    readTmDt();
//    Serial.print(F("GARAGE DOOR WALL SWITCH PRESSED, "));
//  }
  
//  if (garDrWalSw.pressedFor(HOLDTM))          // Bypass garage door timer.
//  {
//    garDrWalSwLongPress();
//    readTmDt();
//    Serial.println(F("GARAGE DOOR WALL SWITCH LONG PRESS"));
//  }

  if (garDrDnSw.wasPressed())                     // Garage door is closed.
  {
    Serial.println(F("CLOSED"));
    digitalWrite(almZone6Pin, OFF);               // Turn off relay for alarm zone 6 (Door closed).
    drUnlockFlag &= ~GARDRLOCK;                   // Turn off  "DISABLE GARAGE DOOR TIMER" flag.
    garDrTmr = 0;
  }
  
  if (garDrDnSw.wasReleased())                    // Garage door is opening.
  {
    digitalWrite(almZone6Pin, ON);                // Turn on relay for alarm zone 6 (Door open).
    Serial.print(F("GARAGE DOOR IS OPENING..."));
  }

  if (garDrUpSw.wasPressed()){Serial.println(F("OPEN"));} // Garage door is opened.
  if (garDrUpSw.wasReleased()){Serial.print(F("GARAGE DOOR IS CLOSING..."));} // Garage door is opening.
}

//#################################################################################################################
// RING FRONT DOOR BELL METHOD
//#################################################################################################################
void ringFrtBel()
{
  readTmDt();
  Serial.println(F("FRONT RFID BELL BUTTON PRESSED"));
  digitalWrite(frtBelPin,ON);                     // Turn front doorbell FET.
  digitalWrite(gLedPin,ON);                       // Turn on GREEN status LED.
  delay (500);                
  digitalWrite(frtBelPin,OFF);                    // Turn off relay.
  digitalWrite(gLedPin,OFF);                      // Turn off GREEN status LED.
}

//#################################################################################################################
// RING REAR DOOR BELL METHOD
//#################################################################################################################
void ringRearBel()
{
  readTmDt();
  Serial.println(F("FRONT RFID BELL BUTTON PRESSED"));
  digitalWrite(rearBelPin,ON);                    // Turn on relay.
  digitalWrite(bLedPin,ON);                       // Turn on BLUE status LED.
  delay (500);                
  digitalWrite(rearBelPin,OFF);                   // Turn off relay.
  digitalWrite(bLedPin,OFF);                      // Turn off BLUE status LED.
}

//#################################################################################################################
// HELP METHOD
//#################################################################################################################
void help()
{
  Serial.println(F("\r\r"));                      // provide line spacing before printing verbose display.
  Serial.println(F("help or ?\t\t\tHELP MENU"));
  Serial.println(F("Ready..."));
}

//#################################################################################################################
// STARTUP MELODY METHOD (CLOSE ENCOUNTER's OF THE THIRD KIND)
//#################################################################################################################
void startupTone()                                // Play startup melody.
{
  Serial.print("POWERUP MELODY STARTED...");
  for(int thisNote = 0; thisNote < 5; thisNote++)
  {
    int noteDuration = 1000/noteDurations[thisNote];
    int pauseBetweenNotes = noteDuration * 1.30;
  
    tone(spkrPin, melody[thisNote],noteDuration);
    delay(pauseBetweenNotes);
    noTone(spkrPin);
    delay (10);
  }
  Serial.println("POWERUP MELODY COMPLETED");
}

//#################################################################################################################
// ERROR TONE METHOD
//#################################################################################################################
void errorTone()                                  // Error tone meoldy.
{
  for(uint8_t i =0;i < 4;i++)
  {
    digitalWrite(frtBprPin, ONINV);             // Turn on beeper.
    digitalWrite(garBprPin, ONINV);             // Turn on beeper.
    digitalWrite(rearBprPin, ONINV);            // Turn on beeper.
    delay(100); 
    digitalWrite(frtBprPin, OFFINV);            // Turn off beeper.
    digitalWrite(garBprPin, OFFINV);            // Turn off beeper.
    digitalWrite(rearBprPin, OFFINV);           // Turn off beeper.
  }
  
  tone(spkrPin,100,100); 
  delay(100); 
  tone(spkrPin,500,100); 
  delay(100); 
  tone(spkrPin,1000,100); 
  delay(100); 
  tone(spkrPin,1500,100); 
  delay(100); 
  noTone(spkrPin);
}

//#################################################################################################################
// UNLOCK FRONT DOOR METHOD
//#################################################################################################################
bool unlockFrtDr()
{
      frtDrLckDlyTmr = EEPROM.read(eAddrFrtDrLckTmr);
      digitalWrite(frtDrLckPin,UNLOCK);           // Check ISR for 60Hz unlock simulation.
      digitalWrite(frtLedPin, ONINV);             // Turn on green LED.
      digitalWrite(gLedPin, ON);                  // Turn on GREEN control panel Status LED.
      Serial.print(F(", FRONT DOOR UNLOCKED..."));
      digitalWrite(frtBprPin, ONINV);             // Turn on beeper.
      drUnlockFlag |= FRTDRLOCK;                  // Show door lock status.
      return true;
}

//#################################################################################################################
// UNLOCK REAR DOOR METHOD
//#################################################################################################################
bool unlockRearDr()
{
    rrDrLckDlyTmr = EEPROM.read(eAddrRrDrLckTmr);
    digitalWrite(rearDrLckPin,UNLOCK);            // Check ISR for 60Hz unlock simulation.
    digitalWrite(frtLedPin, ONINV);               // Turn on green LED.
    digitalWrite(gLedPin, ON);                    // Turn on GREEN control panel Status LED.
    Serial.print(F(", UNLOCKED REAR DOOR..."));
    drUnlockFlag |= REARDRLOCK;                   // Show door lock status.
    digitalWrite(rearLedPin, ONINV);              // Turn on green LED.
    digitalWrite(rearBprPin, ONINV);              // Turn on beeper.
    return true;
}

//#################################################################################################################
// UNLOCK SHED DOOR METHOD
//#################################################################################################################
bool unlockShedDr()
{
    shdDrLckDlyTmr = EEPROM.read(eAddrShdDrLckTmr);
    digitalWrite(shedDrLckPin,UNLOCK);
    digitalWrite(frtLedPin, ONINV);               // Turn on green LED.
    digitalWrite(gLedPin, ON);                    // Turn on GREEN control panel Status LED.
    Serial.print(F(", UNLOCKED SHED DOOR..."));
    drUnlockFlag |= SHEDDRLOCK;                   // Show door lock status.
    digitalWrite(shedLedPin, ONINV);              // Turn on green LED.
    digitalWrite(shedBprPin, ONINV);              // Turn on beeper.
    return true;
}

//#################################################################################################################
// CLEAR ENCODER POSITION METHOD
//#################################################################################################################
// Resets encoder position to "0".
void clrEncoder()
{
  encPosition         = 0;             // Current encoder postion read from encoder.
  oldEncPosition      = 0;
}

//#################################################################################################################
// PRINT LEADING ZERO FUNCTION
//#################################################################################################################
// utility function for digital clock display: displays leading 0 on serial console.
void printDigits(int digits)
{
  if(digits < 10)
  Serial.print('0');
  Serial.print(digits);
}

//#################################################################################################################
// CHECK DST METHOD
//#################################################################################################################
// Tests for DST and ajusts RTC if needed.
void checkDst()
{
  uint8_t dst;
  dst = EEPROM.read(eAddrDst);
  DateTime now = rtc.now();   

  if (now.dayOfTheWeek() == 0 && now.month() == 3 && now.day() >= 8 && now.day() <= 16 && now.hour() == 2 && now.minute() == 0 && now.second() == 0 && dst == 0)
  {       
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour()+1, now.minute(), now.second()));
    dst = 1;
    EEPROM.write(eAddrDst, dst);
  }

  else if(now.dayOfTheWeek() == 0 && now.month() == 11 && now.day() >= 1 && now.day() <= 8 && now.hour() == 2 && now.minute() == 0 && now.second() == 0 && dst == 1)
  {
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour()-1, now.minute(), now.second()));
    dst = 0;
    EEPROM.write(eAddrDst, dst);     
  }
}

//#################################################################################################################
// YES NO RESPONSE METHOD
//#################################################################################################################
//char ynReply()
bool ynReply()
{
  char yn;
  Serial.print(F("PLEASE CONFIRM Y/N? "));
  for(uint8_t i = 0; i < 10; i++)                               //get a new char if available     
  { 
    if( Serial.available() > 0 )
    {
      yn = Serial.read();
      Serial.println(yn);
      if(yn == 'Y' || yn == 'y'){return 1;}        
      else if(yn == 'N' || yn == 'n'){return 0;}
      else
      {
        Serial.println(F(" Input Error..."));
        return 0;        
      }
    }
    delay(1000);                                                // Wait 10 x 1 sec for serial input.
  }
  Serial.println(F("SERIAL INPUT TIMEOUT"));
  return 0;
}

//#################################################################################################################
// CLEAR EEPROM METHOD
//#################################################################################################################
void clrDb()
{
  bool yn = ynReply();
  Serial.println(F("CLEAR DATABASE"));
  if(yn)
  {
    Serial.print(F("CLEARING..."));
    db.initDb();                                    // clear database, but keep magic number.
    delay(100);
    db.insertPwd(PPWD);                             // Add default programming password.
    db.insertPwd(APWD);                             // Add default Admin. password.
    db.insertPwdNam(PPWD, "PROG PWD");              // Add name to programming password.
    db.insertPwdNam(APWD, "ADMIN. PWD");            // Add name to default user password.
    db.insertAtt(APWD,B01000001);                   // Set permission to front door, permanent access.
  }
  else{Serial.print(F("CANCELLED"));}
}

//#################################################################################################################
// CLEAR EEPROM CONFIGURATION METHOD
//#################################################################################################################
void clrConfig()
{
  Serial.println(F("CLEAR EEPROM CONFIGURATION"));
  bool yn = ynReply();
  if(yn)
  {
    Serial.print(F("ERASING CONFIGURATION IN EEPROM..."));
    eraseEeprom(0x0000, DBSTART,0xFF);
    Serial.println(F("CONFIGURATION CLEARED"));
    resetFunc();                          // Resets controller so new configuration in EEPROM can be initializzed.
  }
  else{Serial.print(F("CANCELLED"));}
}

//#################################################################################################################
// CLEAR EEPROM METHOD
//#################################################################################################################
void clrEeprom()
{
  Serial.println(F("CLEAR EVERYTHING IN EEPROM"));
  bool yn = ynReply();
  if(yn)
  {
    Serial.print(F("ERASING EEPROM..."));
    eraseEeprom(0x0000, EEPROMSIZE,0xFF);
    Serial.println(F("EEPROM ERASED"));
  }
  else{Serial.print(F("CANCELLED"));}
}


//#################################################################################################################
// ERASE EEPROM METHOD
//#################################################################################################################
void eraseEeprom(uint16_t eepStart, uint16_t eepEnd, uint8_t fillVal)
{
  for(uint16_t i=eepStart; i<eepEnd;i++)
  {
    if(i % 20 == 0){Serial.print(".");}        // One every 20 erases, show progress.
    if(i % 800 == 0){Serial.println();}
    EEPROM.write(i, fillVal);
    delay(10);
  } 
}

//#################################################################################################################
// KEYPAD LOCATION METHOD
//#################################################################################################################
void keypadLoc()
{
  Serial.print(F(", KEYPAD = "));
  Serial.print(keyPdLoc);
  if(keyPdLoc == NOKEYPD){Serial.println(F(" UNKOWN KEYPAD DOOR"));}
  if(keyPdLoc == FRTDRKEYPD){Serial.println(F(" FRONT DOOR"));}
  else if(keyPdLoc == GARDRKEYPD){Serial.println(F(", GARAGE DOOR"));}
  else if(keyPdLoc == REARDRKEYPD){Serial.println(F(", REAR DOOR"));}
  else if(keyPdLoc == SHEDDRKEYPD){Serial.println(F(", SHED DOOR"));}
}

//################################################################################################
// DRAW MESSAGE METHOD
//################################################################################################ 
void dsplyMsg (const char * str)
{
  char c;
  if (!str) return;
  while ((c = pgm_read_byte(str++)))
  oled.print (c);
}

//################################################################################################
// DRAW MESSAGE METHOD
//################################################################################################ 
void printMsg (const char * str)
{
  char c;
  if (!str) return;
  while ((c = pgm_read_byte(str++)))
  Serial.print (c);
}


//#################################################################################################################
// DRAW PAGE METHOD
//#################################################################################################################
void drawPage()
{
  switch(encPosition)
  {
    case VINVAL:
      drawVin();
    break;

    case VCCVAL:
      drawVcc();
    break;

    default:
      drawTmDt();
    break;
  }
}
