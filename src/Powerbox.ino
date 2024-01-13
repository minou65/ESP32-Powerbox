/*
 Name:		Powerbox.ino
 Created:	17.04.2023 07:05:09
 Author:	andy
*/

// the setup function runs once when you press reset or power the board


#include <IotWebConf.h>
#include "common.h"
#include "webhandling.h"
#include "inverterhandling.h"
#include "RelayHandling.h"
#include "httphandling.h"


void setup() {
	RelaySetup();
	httpSetup();
	InverterSetup();
	wifiInit();
}

void loop() {
	wifiLoop();
	if (iotWebConf.getState() == iotwebconf::OnLine) {
		InverterLoop();
		RelayLoop();
		httpLoop();
	}
	else {
		RelayDisableAll();
		// httpDisableAll();
	}

	gParamsChanged = false;
}