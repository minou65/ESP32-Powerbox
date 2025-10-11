// ShellyHandling.h

#ifndef _SHELLYHANDLING_h
#define _SHELLYHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

extern void setupShellys();
extern void loopShellys();
extern void disableAllShellys();

#endif

