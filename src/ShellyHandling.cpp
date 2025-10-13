#include "ShellyHandling.h"
#include <HTTPClient.h>
#include "common.h"
#include "webhandling.h"
#include "inverterhandling.h"

HTTPClient http;

Neotimer ShellyIntervall = Neotimer(30000);


void getHTTP(String URL) {

    http.begin(URL);
    int httpCode = http.GET();

    Serial.print("invoked web request to "); Serial.println(URL);

    if (httpCode > 0) {
        String payload = http.getString();
        Serial.printf("return code is %i\n", httpCode);
        //Serial.println(payload);
    }
    else {
        Serial.println("Error on HTTP request");
    }

    http.end();
}

void setupShellys() {
    ShellyIntervall.start();
};

void loopShellys() {

    if (!ShellyIntervall.repeat()) { return; }

    Shelly* shelly_ = &Shelly1;
    bool enabled_ = false;

    // Hole die aktuelle Zeit als String im Format "HH:MM"
    time_t now_ = time(nullptr);
    struct tm* timeinfo_ = localtime(&now_);
    char timeStr_[6]; // "HH:MM" + '\0'
    strftime(timeStr_, sizeof(timeStr_), "%H:%M", timeinfo_);
    String currentTime_(timeStr_);

    uint8_t i = 1;
    while (shelly_ != nullptr) {
        if (shelly_->isActive()) {

            enabled_ = (inverterPowerData.inputPower * 1000 > shelly_->getPower()) && (currentTime_ >= shelly_->TimeValue());

            if (enabled_) {
                if (!shelly_->isEnabled()) {
                    getHTTP(shelly_->url_OnValue());
                    shelly_->setEnabled(true);
                    Serial.printf("Shelly %i enabled\n", i);
                }
                shelly_->_timer.start(shelly_->getDelay() * 60 * 1000);
            }
            else {
                if (shelly_->isEnabled() && shelly_->_timer.done()) {
                    getHTTP(shelly_->url_OffValue());
                    shelly_->setEnabled(false);
                    Serial.printf("Shelly %i disabled\n", i);
                }
            }
        }
        shelly_ = (Shelly*)shelly_->getNext();
        i++;
    }
}

void disableAllShellys() {
    Shelly* shelly_ = &Shelly1;
    while (shelly_ != nullptr) {
        if (shelly_->isActive()) {
            shelly_->setEnabled(false);
            getHTTP(shelly_->url_OffValue());
        }
        shelly_ = (Shelly*)shelly_->getNext();
    }
}