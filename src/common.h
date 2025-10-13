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

extern char Version[];

extern char gInverterIPAddress[15];
extern int gInverterPort;
extern uint8_t gInverterInterval;

extern bool gParamsChanged;

extern bool gUseNTPServer;
extern String gNTPServer;
extern String gTimeZone;

#endif

