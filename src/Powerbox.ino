/*
 Name:		Powerbox.ino
 Created:	17.04.2023 07:05:09
 Author:	andy
*/

#include "ntp.h"
#include <IotWebConf.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "common.h"

#include "inverterhandling.h"
#include "RelayHandling.h"
#include "webhandling.h"
#include "httphandling.h"

void setup() {

	Serial.begin(115200);
	while (!Serial) {
		delay(1);
	}

	wifiInit();
	NTPInit();
	RelaySetup();
	httpSetup();
	InverterSetup();
}

void loop() {
	wifiLoop();
	if (iotWebConf.getState() == iotwebconf::OnLine) {
		InverterLoop();
		RelayLoop();
		httpLoop();
		NTPloop();
	}
	else {
		RelayDisableAll();
		// httpDisableAll();
	}

	gParamsChanged = false;
}