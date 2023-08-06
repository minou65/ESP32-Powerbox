// common.h

#ifndef _COMMON_h
#define _COMMON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// > 0: feeding power to the power grid < 0: obtaining 	power from the power grid
extern int gActivePower;

extern char gOutput1Name[20];
extern int gOutput1Power;
extern uint8_t gOutput1GPIO;

extern char gOutput2Name[20];
extern int gOutput2Power;
extern uint8_t gOutput2GPIO;

extern char gOutput3Name[20];
extern int gOutput3Power;
extern uint8_t gOutput3GPIO;

extern char gOutput4Name[20];
extern int gOutput4Power;
extern uint8_t gOutput4GPIO;


extern bool gRelay1;
extern bool gRelay2;
extern bool gRelay3;
extern bool gRelay4;

extern char gInverterIPAddress[15];
extern int gInverterPort;
extern uint16_t gInverterActivePowerRegister;
extern uint8_t gInverterActivePowerDataLength;
extern uint16_t gInverterActivePowerGain;
extern uint8_t gInverterActivePowerInterval;

extern bool gParamsChanged;

#endif

