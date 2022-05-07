// Wiegand.cpp Rev 3.5

#include "Wiegand.h"

uint32_t  WIEGAND::_sysTick       = 0;
uint32_t  WIEGAND::_lastWiegand   = 0;
uint8_t   WIEGAND::_GateActive    = 0;    // 1 = Active A

uint32_t  WIEGAND::_cardTempHighA = 0;
uint32_t  WIEGAND::_cardTempA     = 0;
uint32_t  WIEGAND::_codeA         = 0;
uint32_t  WIEGAND::_rawCodeA      = 0;
uint8_t   WIEGAND::_bitCountA     = 0;	
uint8_t   WIEGAND::_wiegandTypeA  = 0;


WIEGAND::WIEGAND()
{

}


uint32_t  WIEGAND::getCode(){return _codeA;}

uint32_t  WIEGAND::getRawCode(){return _rawCodeA;}

uint8_t WIEGAND::getWiegandType(){return _wiegandTypeA;}

uint8_t WIEGAND::getGateActive(){return _GateActive;}

bool WIEGAND::available(){return DoWiegandConversion();}

void WIEGAND::begin(bool GateA)
{
	_sysTick=millis();
	_lastWiegand  = 0;

	if (GateA == 1 ) 
	{	
    _cardTempHighA  = 0;
    _cardTempA      = 0;
    _codeA          = 0;
    _rawCodeA       = 0;
    _wiegandTypeA   = 0;
    _bitCountA      = 0;  
		
		pinMode(D0PinA, INPUT);                   // Set D0 pin as input
		pinMode(D1PinA, INPUT);                   // Set D1 pin as input
		attachInterrupt(digitalPinToInterrupt(D0PinA), ReadD0A, FALLING);	// Hardware interrupt - high to low pulse
		attachInterrupt(digitalPinToInterrupt(D1PinA), ReadD1A, FALLING);	// Hardware interrupt - high to low pulse
		Serial.println("GateA Enabled");
	}
	else
	{
		Serial.println("GateA Disabled");
	}
}

void WIEGAND::ReadD0A ()
{
	_bitCountA++;                               // Increament bit count for Interrupt connected to D0
	if (_bitCountA>31)                          // If bit count more than 31, process high bits
	{
		_cardTempHighA |= ((0x80000000 & _cardTempA)>>31);	//	shift value to high bits
		_cardTempHighA <<= 1;
		_cardTempA <<=1;
	}
	else
	{
		_cardTempA <<= 1;                         // D0 represent binary 0, so just left shift card data
	}
	_lastWiegand = millis();                    // Keep track of last wiegand bit received
}

void WIEGAND::ReadD1A()
{
	_bitCountA ++;                              // Increment bit count for Interrupt connected to D1
	if (_bitCountA>31)                          // If bit count more than 31, process high bits
	{
		_cardTempHighA |= ((0x80000000 & _cardTempA)>>31);	// shift value to high bits
		_cardTempHighA <<= 1;
		_cardTempA |= 1;
		_cardTempA <<=1;
	}
	else
	{
		_cardTempA |= 1;                          // D1 represent binary 1, so OR card data with 1 then
		_cardTempA <<= 1;                         // left shift card data
	}
  _lastWiegand = millis();                    // Keep track of last wiegand bit received
//	_lastWiegand = _sysTick;                    // Keep track of last wiegand bit received
}



uint32_t WIEGAND::GetCardId (uint32_t *codehigh, uint32_t *codelow, uint8_t bitlength)
{
	uint32_t cardID=0;

	if (bitlength==26)                          // EM tag
		cardID = (*codelow & 0x1FFFFFE) >>1;

	if (bitlength==34)                          // Mifare 
	{
		*codehigh = *codehigh & 0x03;             // only need the 2 LSB of the codehigh
		*codehigh <<= 30;                         // shift 2 LSB to MSB		
		*codelow >>=1;
		cardID = *codehigh | *codelow;
	}
	return cardID;
}

uint8_t translateEnterEscapeKeyPress(uint8_t originalKeyPress)
{
	switch(originalKeyPress)
	{
	case 0x0b:                                  // 11 or # key
		return 0x23;                              // 35 or # ASCII ESCAPE

	case 0x0a:                                  // 10 or * key
		return 0x2A;                              // 42 or * ASCII ENTER

	default:
		return originalKeyPress;
	}
}

bool WIEGAND::DoWiegandConversion ()
{
	uint32_t cardIDA;

	
	_sysTick=millis();
	if ((_sysTick - _lastWiegand) > 25)         // if no more signal coming through after 25ms
	{
		if ((_bitCountA==26) || (_bitCountA==34) || (_bitCountA==8)|| (_bitCountA==4)) 	// bitCount for keypress=4,8, Wiegand 26=26, Wiegand 34=34
		{
			_cardTempA >>= 1;			// shift right 1 bit to get back the real value - interrupt done 1 left shift in advance
			if (_bitCountA>32)			// bit count more than 32 bits, shift high bits right to make adjustment
				_cardTempHighA >>= 1;	

			if((_bitCountA==26) || (_bitCountA==34))// wiegand 26 or wiegand 34
			{
				cardIDA = GetCardId (&_cardTempHighA, &_cardTempA, _bitCountA);
				_rawCodeA = (_cardTempHighA | _cardTempA);
				_wiegandTypeA=_bitCountA;
				_codeA=cardIDA;
				_GateActive=1;
				_bitCountA=0;
				_cardTempA=0;
				_cardTempHighA=0;
				return true;				
			}
			else if (_bitCountA==8)                 // keypress wiegand
			{
				// 8-bit Wiegand keyboard data, high nibble is the "NOT" of low nibble
				// eg if key 1 pressed, data=E1 in binary 11100001 , high nibble=1110 , low nibble = 0001 
				uint8_t highNibble = (_cardTempA & 0xf0) >>4;
				uint8_t lowNibble = (_cardTempA & 0x0f);
				_rawCodeA = _cardTempA;
				_wiegandTypeA=_bitCountA;					
				_GateActive=1;
				_bitCountA=0;
				_cardTempA=0;
				_cardTempHighA=0;
					
			if (lowNibble == (~highNibble & 0x0f))  // check if low nibble matches the "NOT" of high nibble.
			{
				_codeA = (uint8_t)translateEnterEscapeKeyPress(lowNibble);
				return true;
				}
			}
			else if (_bitCountA==4)                 // keypress wiegand 4 bit HEX value
			{
				// 4-bit Wiegand keyboard data, low nibble only. HEX value 0-9 = 0x00-0x09, * = 0x2A, # = 0x23.
				_codeA = (uint8_t)translateEnterEscapeKeyPress(_cardTempA & 0x0000000F);
				_wiegandTypeA=_bitCountA;					
				_rawCodeA = _cardTempA;
				_GateActive=1;
				_bitCountA=0;
				_cardTempA=0;
				_cardTempHighA=0;
				return true;
			}
		}
		else
		{
			// well time over 25 ms and bitCount !=8 , !=26, !=34 , must be noise or nothing then.
//			_lastWiegand=_sysTick;
      _lastWiegand=millis();
			_bitCountA=0;			
			_cardTempA=0;
			_cardTempHighA=0;
			_GateActive=0;
		}	
			// end access control A 
    return false;
	}
	else
	return false;
}

void WIEGAND::clear ()
{
	_cardTempHighA	=	0;
	_cardTempA			=	0;
	_codeA					=	0;
	_rawCodeA				=	0;
	_bitCountA			=	0;	
	_wiegandTypeA		=	0;
}
