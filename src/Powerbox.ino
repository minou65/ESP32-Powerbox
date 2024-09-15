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
#include "ShellyHandling.h"
#include "webhandling.h"
#include "version.h"

char Version[] = VERSION_STR; // Manufacturer's Software version code

void setup() {
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
	Serial.begin(115200);
	while (!Serial) {
		delay(1);
	}

	wifiInit();
	NTPInit();
	RelaySetup();
	ShellySetup();
	InverterSetup();
}

void loop() {
	wifiLoop();
	if (iotWebConf.getState() == iotwebconf::OnLine) {

		InverterLoop();
		RelayLoop();
        ShellyLoop();
		NTPloop();
	}
	else {
		RelayDisableAll();
		// ShellyDisableAll();
	}

	if (gParamsChanged) {
		RelaySetup();
		InverterSetup();
	}

	gParamsChanged = false;
}