
#ifndef _HTTPHANDLING_h
#define _HTTPHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

void httpSetup();

void httpLoop();

void httpDisableAll();


#endif
