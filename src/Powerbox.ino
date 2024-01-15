/*
 Name:		Powerbox.ino
 Created:	17.04.2023 07:05:09
 Author:	andy
*/

// the setup function runs once when you press reset or power the board
#include <IotWebConf.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "common.h"

#include "inverterhandling.h"
#include "RelayHandling.h"
#include "webhandling.h"
#include "httphandling.h"

void setup() {
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
	RelaySetup();
	httpSetup();
	InverterSetup();
	wifiInit();
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); //enable brownout detector
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