/*
 Name:		Powerbox.ino
 Created:	17.04.2023 07:05:09
 Author:	andy
*/

// the setup function runs once when you press reset or power the board


#include "RelayHandling.h"
#include "common.h"
#include "webhandling.h"
#include "inverterhandling.h"
#include "RelayHandling.h"


void setup() {
	RelaySetup();
	InverterSetup();
	wifiInit();
}

void loop() {
	wifiLoop();
	if (iotWebConf.getState() == iotwebconf::OnLine) {
		InverterLoop();
		RelayLoop();
	}
	else {
		RelayDisableAll();
	}

	gParamsChanged = false;
}