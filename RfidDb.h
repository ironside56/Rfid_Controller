#ifndef RFID_DB_H
#define RFID_DB_H

#include "Arduino.h"
#include "EEPROM.h"

// Rev 1.1.10 - Added eepromSize parameter to rfIdDb method.
//            - Changed "maxSize" variable to "totalUsers"
// Rev 1.1.9	- Corrected writename method to write character array length + null instead of maxNameLength.
//            - Corrected NULL terminator which was not written to correct location in EEPROM in "writeNam" method.
// Rev 1.1.8	- Cleaned up comments, removed debug lines.
// Rev 1.1.7A	- Fixed issues with timestamp when removing (moving)record.
// Rev 1.1.7 	- Added time stamp to ID or password.
//						- Fixed readTm() and writeTm().
//						- Moved initDb to public and modified function to initialize database locations
//							to "0x00".
// Rev 1.1.6A - Corected passord not being stored seperately.
// Rev 1.1.5	- Added removeid, removepwd, removename functions.
// Rev 1.1.4	- Added insert(id Tag, Password, NAME)
// Rev 1.1.3	- Added the following class functions:
// 							insertId(password), adds an Id in the same location as the password.
// 							insertPwd(identifier), adds the password in the same location as the Id.
// 							removeId, Removes the identifier from the database, keeps other user perameters.
// 							removePwd, Removes password from user location. Leaves other user parameters unchanged.
// 						- This database designed to store a number of RFIDs up to a fixed of 32 bits 
// 							in size. It is also designed to hold a user name associated with the RFID
// 							and an optional password which can be up to 32 -bits in size. One "Permission" byte
// 							is also included to define the access capabilities of each user.
// 							The database is stored in EEPROM and requires (N * (13 + NameLength) + 2 bytes of 
// 							storage where "N" is the maximum number of entries.
//
// Identifiers can be added, removed and checked for existence.
//
// Performance of insert, remove and contains is O(N).
// Performance of get at index is O(1)
class RfidDb {
  public:
    // Creates an RFID database which does not store names.
    // Parameters:
    //   totalUsers:       The maximum number of ids that the database can hold
    //   eepromOffset:  the byte offset from 0 where the database resides
    //                  in EEPROM
    RfidDb(uint8_t totalUsers, uint16_t eepromOffset);

    // Creates an RFID database which store names.
    // Parameters:
    //   totalUsers:       The maximum number of ids that the database can hold
    //   eepromOffset:  The byte offset from 0 where the databse starts in EEPROM
    //   maxNameSize:   The maximum number of bytes (including null terminator)
    //                  for each name that is stored with Ids or passwords..
    RfidDb(uint8_t totalUsers, uint16_t eepromOffset, uint8_t maxNameSize);

    // Creates an RFID database which store names and uses the all available EEPROM space.
    // Parameters:
    //   eepromSize:    The maximum CPU EEPROM size.
    //   eepromOffset:  The byte offset from 0 where the databse starts in EEPROM
    //   maxNameSize:   The maximum number of bytes (including null terminator)
    //                  for each name that is stored with Ids or passwords..
    RfidDb(uint16_t eepromSize, uint16_t eepromOffset, uint8_t maxNameSize);

    // Initialises the database in EEPROM if the location at EEPROM
    // offset does not contain the magic number.
    void begin();

    // Returns the maximum number of identifiers that the database can
    // contain.
    uint8_t totalUsers();

    // The number of bytes that the entire database takes up in EEPROM
    uint32_t dbSize();

    // Returns the maximum length of name that can be stored, including null
    // terminator
    uint8_t maxNameLength();

    // Returns the number of identifiers currently in the database.
    uint8_t count();

    // Inserts the identifier in the database. If the identifier already exists
		// nothing is added.
    // Returns true if the indentifier was successfully inserted or already
    // existed in the database. Returns false if the database is at maximum
    // capacity.
    bool insertId(uint32_t id);

    // Inserts the identifier in the same user location as the password.
		// If the identifier already exists or if the password is not found,
		// nothing, is done.
    // Returns true if the indentifier was successfully inserted or already
    // existed in the database. Returns false if the database is at maximum
    // capacity or if the password was not found.
    bool insertId(uint32_t id, uint32_t pwd);

		// Inserts the password in the database. If the passwword already exists
		// nothing is added.
    // Returns true if the password was successfully inserted or already
    // existed in the database. Returns false if the database is at maximum
    // capacity.
    bool insertPwd(uint32_t pwd);
		
    // Inserts the password in the same user location as the identifier.
		// If the password already exists or if the identifier is not found,
		// nothing, is done.
    // Returns true if the password was successfully inserted or already
    // existed in the database. Returns false if the database is at maximum
    // capacity or if the identifier was not found.
    bool insertPwd(uint32_t id, uint32_t pwd);

		// Inserts or modifies the name in the database for the user that matches the 
		// identifier provided. If the identifier is not found, no name is added
		// and the function returns false.
    bool insertIdNam(uint32_t id, char* name);

		// Inserts or modifies the name in the database for the user that matches the 
		// password provided. If the password is not found, no name is added
		// and the function returns false.
    bool insertPwdNam(uint32_t pwd, char* name);

		// Inserts the attribute (permission) in the database for the user that matches the 
		// identifier or password provided. If the identifier or password is not found,
		// no name is added and the function returns false.
		bool insertAtt(uint32_t idPwd, uint8_t att);

		// Inserts the time stamp in the database for the user that matches the 
		// identifier or password provided. If the identifier or password is not found,
		// no name is added and the function returns false.
		bool insertTm(uint32_t idPwd, uint32_t tm);

    // Removes the identifier (ID tag) of a user from the database,
		// if it exists. If the identifier is associated with a password,
		// only the identifier is removed. The password, name and attribute 
		// remain unchanged. If no password exists, all parameters for this
		// user, including name, attributes, etc will be deleted from the 
		// database.
    bool removeId(uint32_t id);
   
		// Removes the password of a given user from the database,
		// if it exists. If the password is associated with an identifier,
		// only the password is removed. The identifier, name and attribute 
		// remain unchanged. If no identifier exists for the given user,
		// all parameters for this user, including name, attributes, etc.
		// will be deleted from the database.
		bool removePwd(uint32_t pwd);
    
		// Removes the name associated to an identifier for a user in the database,
		// If the identifier is not in the database, nothing is changed, and an
		// FALSE status is returned. 
		bool removeIdNam(uint32_t id);

		// Removes the name associated to a user password identifier for a user
		// in the database. If the identifier is not in the database, nothing is
		// changed, and an FALSE status is returned showing an error occured. 
		bool removePwdNam(uint32_t pwd);

    // Changes the identifier or password. If the position > 1000 the method
    // assumes a password is being mofified. If the position value is < 1000
    // the method assumes an identifier is being modified.
    bool modifyIdPwd(int16_t pos, uint32_t idPwd);

    
    // Changes the user name using the identifier to locate
    // the user position in the database. Returns true if
    // the user name is sucessfully updated. Returns false if the identifier
    // is not found.
    bool modifyNam(int16_t pos, char* name);

		// Returns the identfier at the given position. Callers should check
    // the return value before using the identifier. Returns true if
    // the position is less than the count and writes the identifier value
    // at the given address.
    // Returns false if pos >= count
    bool readId(int16_t pos, uint32_t &id);
		
		// Returns the password at the given position. Callers should check
    // the return value before using the password. Returns true if
    // the position is less than the count and writes the password value
    // at the given address.
    // Returns false if pos >= count
    bool readPwd(int16_t pos, uint32_t &pwd);

		// Returns the attribute at the given position. Callers should check
    // the return value before using the attribute. Returns true if
    // the position is less than the count and writes the password value
    // at the given address.
    // Returns false if pos >= count
    bool readAtt(int16_t pos, uint8_t &att);

    // Returns the time stamp at the given position. Callers should check
    // the return value before using the time stamp. Returns true if
    // the position is less than the count and writes the password value
    // at the given address.
    // Returns false if pos >= count
    bool readTm(int16_t pos, uint32_t &tm);

    // Returns the name at the given position. Callers should check
    // the return value before using the name. Returns true if
    // the position is less than the count and writes the identifier value
    // to the given string.
    // Callers should allocate a string of at least maxNameSize bytes
    // Returns false if pos >= count or names are not stored in the database
    bool readNam(int16_t pos, char* name);

    // Returns whether the database contains the given identifier or password.
    bool contains(uint32_t id);

    // Returns whether the database contains an indentifier where the low 24
    // bits matches the low 24 bits of the given identifier. This is useful
    // when reading from a Wiegand 26 device (that returns a 24 bit id) with
    // identifers stored as 32 bit ids.
    bool contains24(uint32_t id);

    // Searches the database to find a matching identifier or password.
    // If a matching identifier is found, the user location is returned.
    // If a matching password is found, the user location of the password
    // is returned with an offset of 4096(0x10000). To obtain the true position
		// of the password subtract 4096 from the returned value. If no identifier
		// or password is found, -1 is returned.
    int16_t		posOf(uint32_t idPwd);
    int16_t 	posOf24(uint32_t idPwd);

  // Erases database and intializes database by adding magic number and clears "count"
	void 			initDb();
  
  private:
    uint16_t 	_eepromOffset;
    uint16_t  _eepromSize;
    uint8_t 	_totalUsers;
    uint8_t 	_maxNameLength;

    bool      insert(uint32_t id, uint32_t pwd);
    bool      remove(uint32_t id, uint32_t pwd);
    bool      modify(int16_t pos, uint32_t id, uint32_t pwd, char* name);
    int16_t   posOf(uint32_t idPwd, uint32_t mask);
    uint32_t  readId(int16_t pos);
    uint32_t  readPwd(int16_t pos);
    uint8_t   readAtt(int16_t pos);
    uint32_t  readTm(int16_t pos);
    void      writeId(int16_t pos, uint32_t id);
    void      writePwd(int16_t pos, uint32_t pwd);
    void      writeAtt(int16_t pos, uint8_t att);
    void      writeTm(int16_t pos, uint32_t tm);
    void      writeNam(int16_t pos, const char* name);
		bool      removeNam(int16_t pos);
    void      copyNam(uint8_t srcPos, uint8_t destPos);
		bool      moveLast(uint8_t orgCount, int16_t pToRemove);
		bool      hasMagic();
    void      commitEeprom();
    void      init(uint8_t totalUsers, uint16_t eepromOffset, uint8_t maxNameSize, uint16_t eepromSize);
};

#endif
