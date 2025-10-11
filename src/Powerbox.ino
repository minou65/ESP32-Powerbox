/*
 Name:		Powerbox.ino
 Created:	17.04.2023 07:05:09
 Author:	andy

 use WEMOS D1 MINI ESP32

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

NTPClient ntpClient;
String gNTPServer = "ntp.metas.ch"; // "pool.ntp.org";
String gTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
bool gUseNTPServer = true;

void setup() {
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
	Serial.begin(115200);
	while (!Serial) {
		delay(1);
	}
	Serial.println("PV Powerbox v" + String(Version) + " started");

	wifiInit();
	setupRelays();
	setupShellys();
	setupInverter();

	if (gUseNTPServer) {
		ntpClient.begin(gNTPServer, gTimeZone, 0);
	}
	else {
		Serial.println(F("NTP not used"));
	}
}

void loop() {
	wifiLoop();
	if (iotWebConf.getState() == iotwebconf::OnLine) {

		loopInverter();
		loopRelays();
        loopShellys();

		if (ntpClient.isInitialized()) {
			ntpClient.process();
		}
	}
	else {
		disableAllRelays();
		// ShellyDisableAll();
	}

	if (gParamsChanged) {
		setupRelays();
		setupInverter();
	}

	gParamsChanged = false;
}