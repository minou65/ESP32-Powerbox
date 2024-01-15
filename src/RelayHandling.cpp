// 
// 
// 

#include "common.h"
#include "RelayHandling.h"

char gOutput1Name[20] = "Relay 1";
int gOutput1Power = 0;
uint8_t gOutput1GPIO = GPIO_NUM_22;

char gOutput2Name[20] = "Relay 2";
int gOutput2Power = 0;
uint8_t gOutput2GPIO = GPIO_NUM_21;

char gOutput3Name[20] = "Relay 3";
int gOutput3Power = 0;
uint8_t gOutput3GPIO = GPIO_NUM_17;

char gOutput4Name[20] = "Relay 4";
int gOutput4Power = 0;
uint8_t gOutput4GPIO = GPIO_NUM_16;


bool gRelay1 = false;
bool gRelay2 = false;
bool gRelay3 = false;
bool gRelay4 = false;


void RelaySetup() {
	pinMode(gOutput1GPIO, OUTPUT);
	pinMode(gOutput2GPIO, OUTPUT);
	pinMode(gOutput3GPIO, OUTPUT);
	pinMode(gOutput4GPIO, OUTPUT);
}

void RelayLoop() {
	gRelay1 = (gActivePower > gOutput1Power);
	gRelay2 = (gActivePower > gOutput2Power);
	gRelay3 = (gActivePower > gOutput3Power);
	gRelay4 = (gActivePower > gOutput4Power);

	if (gRelay1) {
		digitalWrite(gOutput1GPIO, HIGH);
	}
	else {
		digitalWrite(gOutput1GPIO, LOW);
	}
	
	if (gRelay2) {
		digitalWrite(gOutput2GPIO, HIGH);
	}
	else {
		digitalWrite(gOutput2GPIO, LOW);
	}

	if (gRelay3) {
		digitalWrite(gOutput3GPIO, HIGH);
	}
	else {
		digitalWrite(gOutput3GPIO, LOW);
	}

	if (gRelay4) {
		digitalWrite(gOutput4GPIO, HIGH);
	}
	else {
		digitalWrite(gOutput4GPIO, LOW);
	}
}

void RelayDisableAll() {
	digitalWrite(gOutput1GPIO, LOW);
	digitalWrite(gOutput2GPIO, LOW);
	digitalWrite(gOutput3GPIO, LOW);
	digitalWrite(gOutput4GPIO, LOW);
}