# RFID CONTROLLER

Description:
This is a project that started out as an interface to RFID keypads that support the Wiegand communication protocol. Althoough not highly secure, I assumed for 
home applications it would add a bit more security then that of standard keypads that trigger a dry contact relay where anyone could remove the keypad 
and short the relay contacts to gain access to the premesis. My controller stores the ID tag and password information on a controller located somewhere in the home
therefore making it more difficult to "hack". 

The controller consists of an Arduino Mege2560 with display and rotory encoder as an interface.

Features:
- Supports any Wiegand supported keypad or RFID device. 
- Supports keppad entry of ID Tag or password.
- Handles Up to four keypad/RFID devices for control of Front Door, Garage Door, Read Door and Shed door.
- Allows support up to 196 users based on a user name length of 10 characters (Expandable using external EEPROMS).
- Configurable permissions for access type: PERMANENT, ONE TIME, or TIME DURATION access.
- Allows configurable door access based on user assigned permissions.
- Adjustable door lock entry delay.
- Configurable garage door sensors (Enabled or Disabled).
- Adjustable automatic garage door closure timer (requires garagge door sensors to be enabled).
- Access logging, records time/date, user ID or password, keypad location, door access location.
