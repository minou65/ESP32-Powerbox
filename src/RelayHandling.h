// RelayHandling.h

#ifndef _RELAYHANDLING_h
#define _RELAYHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void RelaySetup();

void RelayLoop();

void RelayDisableAll();


#endif


