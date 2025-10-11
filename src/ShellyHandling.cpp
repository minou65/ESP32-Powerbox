#include "ShellyHandling.h"
#include <HTTPClient.h>
#include "common.h"
#include "webhandling.h"
#include "ESP32Time.h"

HTTPClient http;

Neotimer ShellyIntervall = Neotimer(30000);


void httpget(String URL) {

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

void ShellySetup() {
    ShellyIntervall.start();
};

void ShellyLoop() {

    if (!ShellyIntervall.repeat()) { return; }

    Shelly* shelly_ = &Shelly1;
    bool _enabled = false;
    ESP32Time rtc;

    String _time = rtc.getTime("%H:%M");
    Serial.println(_time);

    uint8_t i = 1;
    while (shelly_ != nullptr) {
        if (shelly_->isActive()) {

            _enabled = (gInputPower > shelly_->getPower()) && (_time > shelly_->TimeValue());

            if (_enabled) {

                if (shelly_->isEnabled() == false) {
                    httpget(shelly_->url_OnValue());
                    shelly_->setEnabled(true);
                    Serial.printf("Shelly %i enabled\n", i);

                }
                shelly_->_timer.start(shelly_->getDelay() * 60 * 1000);

            }
            else {

                if (shelly_->isEnabled() && shelly_->_timer.done()) {
                    httpget(shelly_->url_OffValue());
                    shelly_->setEnabled(false);
                    Serial.printf("Shelly %i disabled\n", i);
                }
            }
        }
        shelly_ = (Shelly*)shelly_->getNext();
        i++;
    }
}

void ShellyDisableAll() {
    Shelly* shelly_ = &Shelly1;
    while (shelly_ != nullptr) {
        if (shelly_->isActive()) {
            shelly_->setEnabled(false);
            httpget(shelly_->url_OffValue());
        }
        shelly_ = (Shelly*)shelly_->getNext();
    }
}