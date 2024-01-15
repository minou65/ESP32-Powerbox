#include "httphandling.h"

#include <HTTPClient.h>
#include "common.h"

HTTPClient http;

void httpget(String URL_) {
    http.begin(URL_);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
    }
    else {
        Serial.println("Error on HTTP request");
    }

    http.end();
}

void httpSetup() {
    for (uint8_t i = 0; i < ShellyMax; i++){
        Shellys[i].timer.start();
    }
    httpDisableAll();
}

void httpLoop() {
    for (uint8_t i = 0; i < ShellyMax; i++){
        if (Shellys[i].Power > 0) {
            Shellys[i].Enabled = gActivePower > Shellys[i].Power;
            Shellys[i].timer.set(Shellys[i].Delay *1000 * 60);
            Shellys[i].timer.start();
        } else {
            Shellys[i].Enabled = false;
        }

        if (Shellys[i].Enabled) {
            httpget(Shellys[i].url_On);
        } else if (Shellys[i].timer.done()) {
           // httpget(Shellys[i].url_Off);
        }
    }
}

void httpDisableAll() {
    for (uint8_t i = 0; i < ShellyMax; i++) {
        Shellys[i].Enabled = false;
        httpget(Shellys[i].url_Off);
    }
}

