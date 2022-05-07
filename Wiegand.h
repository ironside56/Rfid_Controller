// Wiegand.h Rev 3.5

#ifndef _WIEGAND_H
#define _WIEGAND_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif



class WIEGAND
{

public:
	WIEGAND();
	void 							begin(bool GateA);
	
	bool 							available();
	uint32_t 					getCode();
	uint32_t 					getRawCode();
	uint8_t 					getWiegandType();
	uint8_t 					getGateActive();
	void							clear();
	
	uint8_t 					D0PinA;
	uint8_t 					D1PinA;
	
private:
	static void 			ReadD0A();
	static void 			ReadD1A();

	static bool 			DoWiegandConversion ();
	static uint32_t 	GetCardId (uint32_t *codehigh, uint32_t *codelow, uint8_t bitlength);
	
	static uint32_t 	_sysTick;
	static uint32_t 	_lastWiegand;
	static uint8_t		_GateActive;	
	
	static uint32_t 	_cardTempHighA;
	static uint32_t 	_cardTempA;
	static uint8_t		_bitCountA;	
	static uint8_t		_wiegandTypeA;
	static uint32_t		_codeA;
	static uint32_t		_rawCodeA;
};

#endif
