/*
 Name:		Powerbox.ino
 Created:	17.04.2023 07:05:09
 Author:	andy

 use WEMOS D1 MINI ESP32

*/

#define IOTWEBCONF_DEBUG_DISABLED

#include <Arduino.h>
#include <NTPClient.h>
#include <IotWebConf.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <EventLogger.h> // must be included after webhandling.h
#include <neotimer.h>
#include <build_version.h>

#include "common.h"

#include "inverterhandling.h"
#include "webhandling.h"




char Version[] = VERSION_STR; // Manufacturer's Software version code

NTPClient ntpClient;
String gNTPServer = "ntp.metas.ch"; // "pool.ntp.org";
String gTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
bool gUseNTPServer = true;

const float PV_POWER_RESERVE_ON = 200.0f;   // Power reserve for turning on
const float PV_POWER_RESERVE_OFF = 400.0f;  // Power reserve for turning off (greater!)
Neotimer printTimer(60000); // 60 Sekunden
Neotimer inverterStartupDelay(60000); // 1 minute delay after system goes online before starting inverter polling
bool inverterStartupDelayTriggered = false;
Inverter inverter(wifiClient);

void setup() {
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
	Serial.begin(115200);
	while (!Serial) {
		delay(1);
	}

    LOG_INFO("PV Powerbox v%s started", Version);

	wifiInit();
    LOG_OK("WiFi initialized");

    inverter.begin(inverterConfig.getIPAddress(), inverterConfig.getPort(), inverterConfig.getInterval());
    LOG_OK("Inverter initialized");

	if (ntpConfig.useNtpServer()) {
		ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
        LOG_OK("NTP client initialized");
	}
	else {
        LOG_INFO("NTP not used");
	}

	printTimer.start();

    for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
        device[i_].begin();
    }
}

void loop() {
    wifiLoop();
    if (iotWebConf.getState() == iotwebconf::OnLine) {
        // Start delay timer on first OnLine transition
        if (!inverterStartupDelayTriggered) {
            inverterStartupDelay.start();
            inverterStartupDelayTriggered = true;
            LOG_INFO("System OnLine - Waiting 1 minute before inverter polling starts");
        }

        // Only start inverter polling after startup delay
        if (inverterStartupDelay.done()) {
            inverter.process();

            PowerData powerData_ = inverter.getPowerData();
            MeterData meterData_ = inverter.getMeterData();

            float inverterActivePower_ = powerData_.activePower * 1000; // kW to W
            float gridPower_ = meterData_.activePower; // W

            float totalEnabledPower_ = 0;
            for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
                float power_ = device[i_].getPower();

                if (device[i_].isEnabled() && device[i_].isActive()) {
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

            for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
                device[i_].setEnabled(false);
            }

            float remainingPVPower_ = availablePVPower_;
            float totalConsumerLoad_ = 0;
            for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
                float power_ = device[i_].getPower();
                float partialPower_ = power_ * device[i_].getPartialLoadThreshold();
                if (device[i_].isActive() && (
                    power_ <= remainingPVPower_ ||
                    (device[i_].isPartialLoadAllowed() && partialPower_ <= remainingPVPower_)
                    )) {
                    device[i_].setEnabled(true);
                    remainingPVPower_ -= power_;
                    totalConsumerLoad_ += power_;
                }
            }

            for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
                device[i_].process();
            }

            if (printTimer.repeat()) {
                WEB_LOG_INFO("Inverter active power:%.2f W", inverterActivePower_);
                WEB_LOG_INFO("Grid power:%.2f W", gridPower_);
                WEB_LOG_INFO("Total consumer load:%.2f W", totalConsumerLoad_);
                WEB_LOG_INFO("Available PV power:%.2f W", availablePVPower_);
                WEB_LOG_INFO("----");
            }
        }

        if (ntpClient.isInitialized()) {
            ntpClient.process();
        }
    }
    else {
        for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
            if (device[i_].isEnabled() && device[i_].isActive()) {
                device[i_].setEnabled(false);
                LOG_INFO("Disabled consumer: %s", device[i_].getDesignation().c_str());
            }
        }
    }

    if (gParamsChanged) {
        for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
            device[i_].end();
            device[i_].begin();

            if (!device[i_].isActive()) {
                device[i_].setEnabled(false);
                device[i_].applyDefaultValue();
            }
        }

        inverter.end();
        inverter.begin(inverterConfig.getIPAddress(), inverterConfig.getPort(), inverterConfig.getInterval());

        if (ntpConfig.useNtpServer()) {
            ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
        }
    }

    if (ShouldReboot) {
        LOG_INFO("Rebooting...");
        for (size_t i_ = 0; i_ < MAX_DEVICES; ++i_) {
            device[i_].end();
        }
        inverter.end();
        delay(1000);
        ESP.restart();
    }

    gParamsChanged = false;
}