// 
// 
// 

#include "ShellyHandling.h"
#include <HTTPClient.h>
#include "common.h"
#include "webhandling.h"
#include "ESP32Time.h"

HTTPClient http;


void httpget(String URL_) {

    http.begin(URL_);
    int httpCode = http.GET();

    Serial.print("invoked web request to "); Serial.println(URL_);

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

void ShellySetup() {};

void ShellyLoop() {
    Shelly* _shelly = &Shelly1;
    bool _enabled = false;
    ESP32Time rtc;

    uint8_t i = 1;
    while (_shelly != nullptr) {
        if (_shelly->isActive()) {
            _enabled = (gInputPower > _shelly->GetPower()) && (rtc.getTime() > _shelly->TimeValue);

            if (_enabled) {

                if (_shelly->IsEnabled() == false) {
                    httpget(String(_shelly->url_OnValue));
                    _shelly->SetEnabled(true);
                    Serial.printf("Shelly %i enabled\n", i);

                }
                _shelly->timer.start(_shelly->GetDelay() * 60 * 1000);

            }
            else {

                if (_shelly->IsEnabled() && _shelly->timer.done()) {
                    httpget(String(_shelly->url_OffValue));
                    _shelly->SetEnabled(false);
                    Serial.printf("Shelly %i disabled\n", i);
                }
            }
        }
        _shelly = (Shelly*)_shelly->getNext();
        i++;
    }
}

void ShellyDisableAll() {
    Shelly* _shelly = &Shelly1;
    while (_shelly != nullptr) {
        if (_shelly->isActive()) {
            _shelly->SetEnabled(false);
            httpget(String(_shelly->url_OffValue));
        }
        _shelly = (Shelly*)_shelly->getNext();
    }
}