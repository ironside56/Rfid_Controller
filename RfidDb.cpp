//Author: Greg Tan (Rev 1.0.0, 1.0.1)
//Contributor Guy Bastien (Rev 1.1.10)

#include "Arduino.h"
#include "RfidDb.h"

// REV 1.1.10

// Magic number to verify RFID database in EEPROM
#define RFID_DB_MAGIC 0x75

// Position values below 0x1000 are id positions. Values above 0x1000 are password postions.
#define PWDFLAG 0x1000

// To remove PWDFLAG and obtain raw position value for password.
#define PWDMASK 0xFFF

// returns the EEPROM location of the number of items in the database
#define countOffset() (_eepromOffset + 1)

// returns the EEPROM location of the first id in the database
#define firstIdOffset() (countOffset() + 1)

// returns the EEPROM location of the Ith id in the database
#define idOffset(I) (firstIdOffset() + ((I) * sizeof(uint32_t)))

// returns the EEPROM location of the first password in the database
#define firstPwdOffset() (idOffset(_totalUsers))

// returns the EEPROM location of the Ith password in the database
#define pwdOffset(I) (firstPwdOffset() + ((I) * sizeof(uint32_t)))

// returns the EEPROM location of the first name in the database
#define firstNameOffset() (pwdOffset(_totalUsers))

// returns the EEPROM location of the Ith name in the database
#define nameOffset(I) (firstNameOffset() + (I) * _maxNameLength)

// returns the EEPROM location of the first user permission in the database
#define firstAttOffset() (nameOffset(_totalUsers))

// returns the EEPROM location of the Ith user permission in the database
#define attOffset(I) (firstAttOffset() + ((I) * sizeof(uint8_t)))

// returns the EEPROM location of the first time stamp in the database
#define firstTmOffset() (attOffset(_totalUsers))

// returns the EEPROM location of the Ith user permission in the database
#define tmOffset(I) (firstTmOffset() + ((I) * sizeof(uint32_t)))

// RfidDb Setup Method --------------------------------------------------------------------------------------------------
RfidDb::RfidDb(uint8_t totalUsers, uint16_t eepromOffset){init(totalUsers, eepromOffset, 0, 0);}

// RfidDb Setup Method --------------------------------------------------------------------------------------------------
RfidDb::RfidDb(uint8_t totalUsers, uint16_t eepromOffset, uint8_t maxNameLength){init(totalUsers, eepromOffset, maxNameLength, 0);}

// RfidDb Setup Method --------------------------------------------------------------------------------------------------
RfidDb::RfidDb(uint16_t eepromSize, uint16_t eepromOffset, uint8_t maxNameLength){init(0, eepromOffset, maxNameLength, eepromSize);}

// Begin Method ---------------------------------------------------------------------------------------------------------
void RfidDb::begin() 
{
  if (!hasMagic()){initDb();}
}

// totalUsers Method -------------------------------------------------------------------------------------------------------
uint8_t RfidDb::totalUsers() {return _totalUsers;}

// maxNameLength Method --------------------------------------------------------------------------------------------------
uint8_t RfidDb::maxNameLength() {return _maxNameLength;}

// count Method ---------------------------------------------------------------------------------------------------------
uint8_t RfidDb::count() {return EEPROM.read(countOffset());}

// dbSize Method --------------------------------------------------------------------------------------------------------
uint32_t RfidDb::dbSize() 
{
  uint32_t recordSize;
    //                    id size            password size      Name size         Attribute size    Time Stamp
    recordSize = sizeof(uint32_t) + sizeof(uint32_t) + maxNameLength() + sizeof(uint8_t) + sizeof(uint32_t);
  if(!_eepromSize)                       // EEPROM size not specified so use totalUsers to calculate database size.
  {
  
    return  1 // magic number
            + 1    // record count
            + (recordSize * totalUsers()); // size of each record multiplied by the maximum number of records
  }
  else
  {
    _totalUsers = (_eepromSize - _eepromOffset) / recordSize;
    return (_eepromSize - _eepromOffset);
  }
}

// insertId Method -------------------------------------------------------------------------------------------------------
bool RfidDb::insertId(uint32_t id) {return insert(id, 0);}
bool RfidDb::insertId(uint32_t id, uint32_t pwd) {return insert(id, pwd);}

// insertPwd Method ------------------------------------------------------------------------------------------------------
bool RfidDb::insertPwd(uint32_t pwd) {return insert(0, pwd);}
bool RfidDb::insertPwd(uint32_t id, uint32_t pwd) {return insert(id, pwd);}

// insertIdNam Method ----------------------------------------------------------------------------------------------------
bool RfidDb::insertIdNam(uint32_t id, char* name)
{
  int16_t pos = posOf(id);
  if ((pos != -1) && (pos < PWDFLAG))
	{
    writeNam(pos, name);
    return true;
	}
	else{return false;}
	
}

// insertPwdNam Method ---------------------------------------------------------------------------------------------------
bool RfidDb::insertPwdNam(uint32_t pwd, char* name)
{
  int16_t pos = posOf(pwd);
  if ((pos != -1) && (pos >= PWDFLAG))
	{
		pos &= PWDMASK;                           // Remove flag showing this is a password.
    writeNam(pos, name);
    return true;
	}
	else{return false;}
}

// insertAtt Method ------------------------------------------------------------------------------------------------------
bool RfidDb::insertAtt(uint32_t idPwd, uint8_t att)
{
		int16_t pos = posOf(idPwd);
		if (pos != -1)
		{
			if (pos > PWDFLAG){pos &= PWDMASK;}     // If > 0x1000 then it's an password position.
			writeAtt(pos, att);
			commitEeprom();
			return true;
		}
		else return false;
}

// insertTm Method -------------------------------------------------------------------------------------------------------
bool RfidDb::insertTm(uint32_t idPwd, uint32_t tm)
{
		int16_t pos = posOf(idPwd);
		if (pos != -1)
		{
			if (pos > PWDFLAG){pos &= PWDMASK;}     // If > 0x1000 then it's an password position.
			writeTm(pos, tm);
			commitEeprom();
			return true;
		}
		else return false;
}

// insert Method ---------------------------------------------------------------------------------------------------------
bool RfidDb::insert(uint32_t id, uint32_t pwd) 
{
// if id already exists in the database, we update the password
	if (id)                                     // Write password to dsatabase using id to locate position.
	{
		int16_t pos = posOf(id);
		if ((pos != -1) && (pos < PWDFLAG))       // If less than 0x10000 then it's an id position.
		{
//			Serial.println(F("PASSWORD ADDED TO ID IN DATABASE"));
			if (pwd){writePwd(pos, pwd);}           // If password value given, add to database. 
			commitEeprom();
			return true;
		}
	}

// if password already exists in the database, we update the id.
	if (pwd)											// Write id using password to find location in database.
	{
		int16_t pos = posOf(pwd);
		if ((pos != -1) && (pos >= PWDFLAG))      // If greater than 0x10000 then it's a password position.
		{
//			Serial.println(F("ID ADDED TO PASSWORD IN DATABASE"));
			if (id){writeId((pos & PWDMASK), id);}
			commitEeprom();
			return true;
		}
	}

	// id or password not found so this is a new entry in the database.
	uint8_t c = count();
	if (c >= _totalUsers)                        // id or password not found so this is a new entry in the database.
	{
		return false;                           // no room in database, return false
	}
	
	if ((id) || (pwd))
	{
//		Serial.println(F("NEW ENTRY ADDED"));
		if (id){writeId(c, id);}
		if (pwd){writePwd(c, pwd);}
		EEPROM.write(countOffset(), c + 1);
		commitEeprom();
		return true;
	}
}

// removeId Method -------------------------------------------------------------------------------------------------------
bool RfidDb::removeId(uint32_t id) {return remove(id, 0);}

// removePwd Method ------------------------------------------------------------------------------------------------------
bool RfidDb::removePwd(uint32_t pwd) {return remove(0, pwd);}

// removeIdNam Method ----------------------------------------------------------------------------------------------------
bool RfidDb::removeIdNam(uint32_t id)
{
	int16_t pos;
	pos = posOf(id);
	if(pos >= PWDFLAG || pos == -1){return false;}
	else{return removeNam(pos);}
}

// removePwdNam ---------------------------------------------------------------------------------------------------------
bool RfidDb::removePwdNam(uint32_t pwd)
{
	int16_t pos;
	pos = posOf(pwd);			// Position should be 0x1000 or greater to indicate its a password location.
	if(pos < PWDFLAG || pos == -1){return false;}
	else
	{
		pos &= PWDMASK;
		return removeNam(pos);
	}
}

// remove Method -------------------------------------------------------------------------------------------------------
bool RfidDb::remove(uint32_t id, uint32_t pwd)// Removes ID or password from the database.
// if id already exists in the database, we update the password and, or name.
{
	bool returnVal = false;
  uint8_t originalCount = count();  
  if (originalCount == 0){return;}

	if (id)                                     // Write password to dsatabase using id to locate position.
	{
  int16_t posToRemove = posOf(id);            // If no ID or password found, exit.
		if ((posToRemove != -1))                  // If less than 0x10000 then it's an id position.
		{
			if(posToRemove >= PWDFLAG){return false;}	// Check if password value was given in error.
			if (readPwd(posToRemove)){writeId(posToRemove,0);}		// Clear ID.
			else{moveLast(originalCount,posToRemove);}// Password does not exist so move last user data to this location.
			returnVal = true;
		}
		else{return false;}
	}

	if (pwd)                                      // Remove password. If no associated ID found, remove entire record.
	{
  int16_t posToRemove = posOf(pwd);             // If no ID or password found, exit.
		if ((posToRemove != -1))                    // If less than 0x10000 then it's an id position.
		{
			if(posToRemove < PWDFLAG){
				return false;}                          // Check if id value was given in error.
			posToRemove &= PWDMASK;
			if (readId(posToRemove)){writePwd(posToRemove,0);}  // Is ID found in database, clear password.
			else{moveLast(originalCount,posToRemove);}// ID does not exist, so move last user data to this location.
			returnVal = true;
		}
		else{return false;}
	}
	commitEeprom();
	return returnVal;
}

// modifyNam Method ----------------------------------------------------------------------------------------------------
bool RfidDb::modifyNam(int16_t pos, char* name)
{
  if(pos > PWDFLAG){pos &= PWDMASK;}
  if(pos >= count() || pos < 0) {return false;}
  writeNam(pos, name);
  return true;
}

// modify Method -------------------------------------------------------------------------------------------------------
bool RfidDb::modifyIdPwd(int16_t pos, uint32_t idPwd)       // Modifies ID or password from the database.
// If no id or password is found, return false.
{
  if (pos < 0) {return false;}
  if(pos >= 0 && pos < PWDFLAG){writeId(pos,idPwd);}        // Add new Id to database.
  else{writePwd(pos & PWDMASK,idPwd);         // Add new password at same user location as old password.
  }
  return true;
}

// readId --------------------------------------------------------------------------------------------------------------
bool RfidDb::readId(int16_t pos, uint32_t &id)
{
  if (pos >= count() || pos < 0) {return false;}
	id = readId(pos);
	return true;
}

// readPwd Method -----------------------------------------------------------------------------------------------------
bool RfidDb::readPwd(int16_t pos, uint32_t &pwd)
{
  if (pos >= count() || pos < 0) {return false;}
  pwd = readPwd(pos);
  return true;
}

// readAtt Method -----------------------------------------------------------------------------------------------------
bool RfidDb::readAtt(int16_t pos, uint8_t &att)
{
  if (pos >= count() || pos < 0) {return false;}
  att = readAtt(pos);
  return true;
}

// readTm Method ------------------------------------------------------------------------------------------------------
bool RfidDb::readTm(int16_t pos, uint32_t &tm)
{
  if (pos >= count() || pos < 0) {return false;}
  tm = readTm(pos);
  return true;
}

// readNam ------------------------------------------------------------------------------------------------------------
bool RfidDb::readNam(int16_t pos, char* name)
{
  if (pos >= count() || pos < 0 || _maxNameLength == 0){return false;}

  uint16_t base = nameOffset(pos);
  for (int i = 0; i < _maxNameLength; i++)
	{
    name[i] = EEPROM.read(base + i);
    if (name[i] == '\0') {break;}
  }
  return true;
}

// posOf Method -------------------------------------------------------------------------------------------------------
// Returns the position of the given id in the database or -1 if the id is 
// not in the database
int16_t RfidDb::posOf(uint32_t idPwd) {return posOf(idPwd, 0xFFFFFFFF);}

// posOf24 Method -----------------------------------------------------------------------------------------------------
// Returns the position of the given id in the database when compared on the
// low 24 bits of the id.
int16_t RfidDb::posOf24(uint32_t idPwd) {return posOf(idPwd, 0x00FFFFFF);}

// contains Method ----------------------------------------------------------------------------------------------------
bool RfidDb::contains(uint32_t id){return posOf(id) != -1;}

// contains24 Method --------------------------------------------------------------------------------------------------
bool RfidDb::contains24(uint32_t id) {return posOf24(id) != -1;}

// posOf Method (PRIVATE)----------------------------------------------------------------------------------------------
//PRIVATE FUNCTIONS
// Returns the position of the given id or password when compared with ids and
// passwords in the database after both the database id and the given id are
// bit masked with the given mask.
int16_t RfidDb::posOf(uint32_t idPwd, uint32_t mask)
{
  uint32_t maskedId = idPwd & mask;
  uint32_t maskedPwd = idPwd;
  for (uint8_t i = 0, n = count(); i < n; i++)
	{
    if (maskedId == (readId(i) & mask)) {return i;}
		else if(maskedPwd == readPwd(i)){return (i |  PWDFLAG);}
  }
  return -1;
}

// readId Method (PRIVATE)-------------------------------------------------------------------------------------------
// Returns the id at the given position
uint32_t RfidDb::readId(int16_t pos)
{
  if(pos < 0){return false;}
  uint32_t id;
  EEPROM.get(idOffset(pos), id);
  return id;
}

// readPwd Method (PRIVATE)---------------------------------------------------------------------------------------------
// Returns the password at the given position
uint32_t RfidDb::readPwd(int16_t pos)
{
  if(pos < 0){return false;}
  uint32_t pwd;
  EEPROM.get(pwdOffset(pos), pwd);
  return pwd;
}

// readAtt Method (PRIVATE)------------------------------------------------------------------------------------------
// Returns the attribute byte at the given position
uint8_t RfidDb::readAtt(int16_t pos)
{
  if(pos < 0){return false;}
  uint8_t att;
  att = EEPROM.read(attOffset(pos));
  return att;
}

// readTm (PRIVATE)--------------------------------------------------------------------------------------------------
// Returns the time stamp byte at the given position
uint32_t RfidDb::readTm(int16_t pos)
{
  if(pos < 0){return false;}
  uint32_t tm;
  EEPROM.get(tmOffset(pos), tm);
  return tm;
}

// writeId Method (PRIVATE)------------------------------------------------------------------------------------------
// Writes an id to the database at a given position
inline void RfidDb::writeId(int16_t pos, uint32_t id)
{
  if(pos < 0){return false;}
  EEPROM.put(idOffset(pos), id);
  commitEeprom();
}

// writePwd Method (PRIVATE)-----------------------------------------------------------------------------------------
// Writes a password to the database at a given position
inline void RfidDb::writePwd(int16_t pos, uint32_t pwd)
{
  if(pos < 0){return false;}
  EEPROM.put(pwdOffset(pos), pwd);
  commitEeprom();
}

// writeAtt Method (PRIVATE)-----------------------------------------------------------------------------------------
// Writes an attribute to the database at a given position
inline void RfidDb::writeAtt(int16_t pos, uint8_t att)
{
  if(pos < 0){return false;}
  EEPROM.write(attOffset(pos), att);
  commitEeprom();
}

// writeTm Method (PRIVATE)------------------------------------------------------------------------------------------
// Writes a time stamp to the database at a given position
inline void RfidDb::writeTm(int16_t pos, uint32_t tm) 
{
  if(pos < 0){return false;}
  EEPROM.put(tmOffset(pos), tm);
  commitEeprom();
}

// writeNam Method (PRIVATE)-----------------------------------------------------------------------------------------
// Writes a name to the database at a given position
void RfidDb::writeNam(int16_t pos, const char* name)
{
  if(pos < 0){return false;}
  if (strlen(name) > 0)                         // Do not store name if parameter was not set up.
	{
    uint16_t base = nameOffset(pos);
	  uint16_t nameSize = strlen(name);
    for (uint16_t i = 0; i < nameSize; i++)
		{
			EEPROM.write(base + i, *name);
			name++;
		}
    EEPROM.write(base + nameSize, '\0');        // Ensure we null terminate
	}
  commitEeprom();
}

// removeNam Method (PRIVATE)----------------------------------------------------------------------------------------
// Clears a name from a given position.
bool RfidDb::removeNam(int16_t pos)
{
  if (_maxNameLength > 0)
	{
    if(pos < 0){return false;}
		uint16_t base = nameOffset(pos);
		for (int i = 0; i < _maxNameLength; i++){EEPROM.write(base + i,'\0');}	// Includes terminating character.
		commitEeprom();
		return true;
  }
	else{return false;}
}

// copyName Method (PRIVATE)-----------------------------------------------------------------------------------------
void RfidDb::copyNam(uint8_t srcPos, uint8_t destPos)
{
  if (_maxNameLength > 0)
	{
    uint16_t srcbase = nameOffset(srcPos);
    uint16_t destBase = nameOffset(destPos);
    for (int i = 0; i < _maxNameLength; i++)
		{
      char c = EEPROM.read(srcbase + i);
      EEPROM.write(destBase + i, c);
      if (c == '\0') {break;}
    }
    commitEeprom();
  }
}
// moveLast Method (PRIVATE)-----------------------------------------------------------------------------------------
// Moves last entry in database to location of removed entry to reduce keep entries in sequence.
bool RfidDb::moveLast(uint8_t orgCount, int16_t pToRemove)
{
	uint8_t newCount = orgCount - 1;              // Remove last entry from database.
	uint8_t attToMove = readAtt(newCount);
	uint32_t pwdToMove = readPwd(newCount);

	if (newCount > 0 || newCount == pToRemove)
	{
		uint32_t idToMove = readId(newCount);	
		writeId(pToRemove, idToMove);               // Move id from last location in database to location to be removed.
		writePwd(pToRemove, pwdToMove);             // Move password from last location in database to location to be removed.
		writeAtt(pToRemove, attToMove);             // Move attribute from last location in database to location to be removed.
		copyNam(newCount, pToRemove);              // Move name from last location in database to location to be removed.

		writeId(newCount, 0);                       // Clear old id location.
		writePwd(newCount, 0);                      // Clear old password location.
		writeAtt(newCount, 0);                      // Clear old attribute location.
		writeTm(newCount, 0);                       // Clear old timestamp location.
		removeNam(newCount);

		EEPROM.write(countOffset(), newCount);      // Update number of user entries.
		return 1;
	}
	else{return 0;}
}

// hasMagic Method (PRIVATE)-----------------------------------------------------------------------------------------
// Returns whether the EEPROM location at the EEPROM base address
// contains the magic number
bool RfidDb::hasMagic() {return EEPROM.read(_eepromOffset) == RFID_DB_MAGIC;}

// init Mehtod (PRIVATE)---------------------------------------------------------------------------------------------
void RfidDb::init(uint8_t totalUsers, uint16_t eepromOffset, uint8_t maxNameLength, uint16_t eepromSize)
{
  _totalUsers = totalUsers;
  _eepromOffset = eepromOffset;
  _maxNameLength = maxNameLength;
  _eepromSize = eepromSize;
}

//initDb Method (PRIVATE)--------------------------------------------------------------------------------------------
// Initialises the database by writing the magic number to the base
// EEPROM address, followed by a zero count.
void RfidDb::initDb()
{
  Serial.print(F("INITIALIZING DATABASE..."));
	for (uint16_t i = firstIdOffset(); i < (dbSize() -2);i++){EEPROM.write(i,0);}
	EEPROM.write(_eepromOffset, RFID_DB_MAGIC);   // Magic Number
  EEPROM.write(countOffset(), 0);               // Initialize Count.
  Serial.println(F("COMPLETED"));
  commitEeprom();
}

// commit Method (PRIVATE)-------------------------------------------------------------------------------------------
void RfidDb::commitEeprom()
{
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  EEPROM.commit();
#endif
}
