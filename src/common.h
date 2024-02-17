// common.h

#ifndef _COMMON_h
#define _COMMON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "neotimer.h"

#define STRING_LEN 50
#define NUMBER_LEN 6

extern int gInputPower;
// > 0: feeding power to the power grid < 0: obtaining 	power from the power grid
extern int gActivePower;

extern char gInverterIPAddress[15];
extern int gInverterPort;
extern uint16_t gInverterInputPowerRegister;
extern uint8_t gInverterInputPowerDataLength;
extern uint16_t gInverterInputPowerGain;
extern uint16_t gInverterActivePowerRegister;
extern uint8_t gInverterActivePowerDataLength;
extern uint16_t gInverterActivePowerGain;
extern uint8_t gInverterActivePowerInterval;

extern bool gParamsChanged;

extern bool gUseNTPServer;
extern String gNTPServer;
extern String gTimeZone;

#endif

