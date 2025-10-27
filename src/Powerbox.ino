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
#include "webhandling.h"
#include "version.h"
#include "neotimer.h"

char Version[] = VERSION_STR; // Manufacturer's Software version code

NTPClient ntpClient;
String gNTPServer = "ntp.metas.ch"; // "pool.ntp.org";
String gTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
bool gUseNTPServer = true;

const float PV_POWER_RESERVE_ON = 200.0f;   // Einschalt-Reserve
const float PV_POWER_RESERVE_OFF = 400.0f;  // Ausschalt-Reserve (größer!)
Neotimer printTimer(60000); // 60 Sekunden
Inverter inverter(wifiClient);

void setup() {
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
	Serial.begin(115200);
	while (!Serial) {
		delay(1);
	}
	Serial.println("PV Powerbox v" + String(Version) + " started");

	wifiInit();
    inverter.begin(inverterConfig.getIPAddress(), inverterConfig.getPort(), inverterConfig.getInterval());

	if (ntpConfig.useNtpServer()) {
		ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
	}
	else {
		Serial.println(F("NTP not used"));
	}

	printTimer.start();

    for (Consumer* c_ : consumers) {
        c_->begin();
    }
}

void loop() {
	wifiLoop();
    if (iotWebConf.getState() == iotwebconf::OnLine) {

		inverter.process();

		PowerData powerData_ = inverter.getPowerData();
		MeterData meterData_ = inverter.getMeterData();

        float inverterActivePower_ = powerData_.activePower * 1000; // kW to W
        float gridPower_ = meterData_.activePower; // W

		float totalEnabledPower_ = 0;
        for(Consumer* c_ : consumers) {
            float power_ = c_->getPower();
            float partialPower_ = power_ * c_->getPartialLoadThreshold();
            if (c_->isEnabled() && c_->isActive()) {
                totalEnabledPower_ += power_;
            }
		}

        // Calculate available PV power
        // Note: In the inverter, gridPower_ > 0 means feed-in, gridPower_ < 0 means grid consumption!
        float availablePVPower_;
        if (gridPower_ > 0) {
            availablePVPower_ = gridPower_ - PV_POWER_RESERVE_ON + totalEnabledPower_;
            if (availablePVPower_ < 0) availablePVPower_ = 0;
			if (availablePVPower_ > inverterActivePower_) availablePVPower_ = inverterActivePower_;
        }
        else {
            // Power is being drawn from the grid or balanced: no PV surplus available
            availablePVPower_ = 0;
        }

        for (Consumer* c_ : consumers) {
            c_->setEnabled(false);
        }

        float remainingPVPower_ = availablePVPower_;
        float totalConsumerLoad_ = 0;
        for (Consumer* c_ : consumers) {
            float power_ = c_->getPower();
			float partialPower_ = power_ * c_->getPartialLoadThreshold();
            if (c_->isActive() && (
                power_ <= remainingPVPower_ ||
                (c_->isPartialLoadAllowed() && partialPower_ <= remainingPVPower_)
                )) {
                c_->setEnabled(true);
                remainingPVPower_ -= power_;
                totalConsumerLoad_ += power_;
            }
        }

        // For each consumer, call process
        for (Consumer* c_ : consumers) {
            c_->process();
        }

        if (ntpClient.isInitialized()) {
            ntpClient.process();
        }

        if (printTimer.repeat()) {
            SERIAL_WEB_SERIALF("Inverter active power:%.2f W\n", inverterActivePower_);
            SERIAL_WEB_SERIALF("Grid power:%.2f W\n", gridPower_);
            SERIAL_WEB_SERIALF("Total consumer load:%.2f W\n", totalConsumerLoad_);
            SERIAL_WEB_SERIALF("Available PV power:%.2f W\n", availablePVPower_);
            SERIAL_WEB_SERIALLN("----");
        }
    }
	else {
        for (Consumer* c_ : consumers) {
            if (c_->isEnabled() && c_->isActive()) {
                c_->setEnabled(false);
                SERIAL_WEB_SERIALF("Disabled consumer: %s\n", c_->getName().c_str());
            }
        };
	}

	if (gParamsChanged) {
        for (Consumer* c_ : consumers) {
            c_->end();
            c_->begin();

            if (!c_->isActive()){
                c_->setEnabled(false);
				c_->applyDefaultValue();
			}

        }

		inverter.end();
		inverter.begin(inverterConfig.getIPAddress(), inverterConfig.getPort(), inverterConfig.getInterval());

        if (ntpConfig.useNtpServer()) {
            ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
        }
	}

    if (ShouldReboot) {
        SERIAL_WEB_SERIALLN("Rebooting...");
        for (Consumer* c_ : consumers) {
            c_->end();
        }
		inverter.end();
        delay(1000);
        ESP.restart();
	}

	gParamsChanged = false;
}