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

        float inverterActivePower_ = inverterPowerData.activePower * 1000; // kW to W
        float gridPower_ = meterData.activePower; // W

        // Calculate total power of enabled consumers
        float totalConsumerLoad_ = 0;
        for (Consumer* c_ : consumers) {
            if (c_->isEnabled() && c_->isActive()) {
                totalConsumerLoad_ += c_->getPower();
            }
        }

        // Calculate available PV power
        // Note: In the inverter, gridPower_ > 0 means feed-in, gridPower_ < 0 means grid consumption!
        float availablePVPower_;
        if (gridPower_ > 0) {
            // When power is being fed into the grid (gridPower_ > 0), the available PV surplus for additional consumers
            // is calculated as the inverter's active power minus the power fed into the grid and minus the current consumer load.
            // This ensures that only the actual surplus (which would otherwise be exported) is used for enabling more consumers.
            availablePVPower_ = inverterActivePower_ - gridPower_ - totalConsumerLoad_;
        }
        else {
            // Power is being drawn from the grid or balanced: no PV surplus available
            availablePVPower_ = 0;
        }

        //Serial.println("----");
        //Serial.print("Inverter active power: "); Serial.print(inverterActivePower_); Serial.println(" W, ");
        //Serial.print("    Grid power: "); Serial.print(gridPower_); Serial.println(" W, ");
        //Serial.print("    Total consumer load: "); Serial.print(totalConsumerLoad_); Serial.println(" W, ");
        //Serial.print("    Available PV power: "); Serial.print(availablePVPower_); Serial.println(" W");


        // Enable consumers by priority
        float remainingPVPower_ = availablePVPower_;
        for (Consumer* c_ : consumers) {
            if (!c_->isEnabled() && c_->isActive() && c_->getPower() <= remainingPVPower_) {
                c_->setEnabled(true);
                Serial.printf("Enabled consumer: %s, with a load of %d W\n", c_->getName().c_str(), c_->getPower());
                remainingPVPower_ -= c_->getPower();
                Serial.print("    Remaining PV power: "); Serial.print(remainingPVPower_); Serial.println(" W");
            }
        }

        // Try to enable higher-priority consumers by disabling lower-priority ones
        for (size_t i = 0; i < consumers.size(); ++i) {
            Consumer* highPrio_ = consumers[i];
            if (!highPrio_->isEnabled() && highPrio_->isActive()) {
                uint32_t requiredPower_ = highPrio_->getPower();
                uint32_t availablePower_ = availablePVPower_;
                std::vector<size_t> toDisable_;
                // Collect enabled lower-priority consumers
                for (size_t j = consumers.size(); j > i; --j) {
                    Consumer* lowPrio_ = consumers[j - 1];
                    if (lowPrio_->isEnabled() && lowPrio_->isActive()) {
                        availablePower_ += lowPrio_->getPower();
                        toDisable_.push_back(j - 1);
                        if (availablePower_ >= requiredPower_) break;
                    }
                }
                // If enough power can be freed, switch
                if (availablePower_ >= requiredPower_) {
                    for (size_t idx_ : toDisable_) {
                        consumers[idx_]->setEnabled(false);
                    }
                    highPrio_->setEnabled(true);
                    break; // Only one switch per loop
                }
            }
        }

        // Recalculate used power
        float usedPower_ = 0;
        for (Consumer* c_ : consumers) {
            if (c_->isEnabled() && c_->isActive()) {
                usedPower_ += c_->getPower();
            }
        }

        // If too much power is used, disable consumers with lowest priority (from the end)
        float maxPVPower_ = inverterActivePower_ - gridPower_;
        if (usedPower_ > maxPVPower_) {
            for (auto it_ = consumers.rbegin(); it_ != consumers.rend() && usedPower_ > maxPVPower_; ++it_) {
                Consumer* c_ = *it_;
                if (c_->isEnabled() && c_->isActive()) {
                    c_->setEnabled(false);
                    usedPower_ -= c_->getPower();
					Serial.printf("Disabled consumer: %s, with a load of %d W\n", c_->getName().c_str(), c_->getPower());
					Serial.print("    New used power: "); Serial.print(usedPower_); Serial.println(" W");
                }
            }
        }

        // For each consumer, call process
        for (Consumer* c_ : consumers) {
            c_->process();
        }

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
		//setupInverter();
	}

	gParamsChanged = false;
}