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

#define Relay1 0
#define Relay2 1
#define Relay3 2
#define Relay4 3
#define RelayMax 4

#define Shelly1 0
#define Shelly2 1
#define Shelly3 2
#define Shelly4 3
#define Shelly5 4
#define Shelly6 5
#define Shelly7 6
#define Shelly8 7
#define Shelly9 8
#define Shelly10 9
#define ShellyMax 10


struct Relay {
	char Name[STRING_LEN];
	char Power[NUMBER_LEN];
	char GPIO[NUMBER_LEN];
	char Delay[NUMBER_LEN];
	bool Enabled;
	Neotimer timer = Neotimer(1000);
};

struct Shelly {
	char Name[STRING_LEN] = "Shelly";
	char url_On[STRING_LEN] = "http://";
	char url_Off[STRING_LEN] = "http://";
	uint16_t Power = 0;
	uint8_t Delay = 0;
	bool Enabled = false;
	Neotimer timer = Neotimer(1000);
};

extern Shelly Shellys[ShellyMax];
extern Relay Relays[RelayMax];

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

extern bool gUseNTPServer;
extern String gNTPServer;
extern String gTimeZone;

#endif

