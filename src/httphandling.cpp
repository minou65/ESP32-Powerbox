#include "httphandling.h"

#include <HTTPClient.h>
#include "common.h"

HTTPClient http;

Shelly Shellys[ShellyMax];

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

void httpSetup() {

    // httpDisableAll();
}


Neotimer mytimer3 = Neotimer(5000);

void httpLoop() {
    bool _enabled = false;
    bool _debug = mytimer3.repeat();

    for (uint8_t i = 0; i < ShellyMax; i++){
        _enabled = false;

        if (Shellys[i].Power > 0) {
            _enabled = gActivePower > Shellys[i].Power;

            //if (_debug) {
            //    Serial.printf("Shelly %i should be powered ", i); Serial.println(_enabled ? "on" : "off");
            //    Serial.printf("Shelly %i timer preset is ", i); Serial.println(Shellys[i].timer.get());
            //    Serial.printf("Shelly %i timer ", i); Serial.println(Shellys[i].timer.waiting() ? "is waiting" : "is not waiting");
            //    Serial.println("");
            //}
 
            if (_enabled) {
                
                if (Shellys[i].Enabled == false) {
                    httpget(Shellys[i].url_On);
                    Shellys[i].Enabled = true;
                    Serial.printf("Shelly %i enabled\n", i);

                }
                Shellys[i].timer.start(Shellys[i].Delay * 60 * 1000);
                
            }
            else {
                
                if (Shellys[i].Enabled == true && Shellys[i].timer.done()) {
                    httpget(Shellys[i].url_Off);
                    Shellys[i].Enabled = false;
                    Serial.printf("Shelly %i disabled\n", i);
                }
            }
        }
    }
}

void httpDisableAll() {
    for (uint8_t i = 0; i < ShellyMax; i++) {
        Shellys[i].Enabled = false;
        httpget(Shellys[i].url_Off);
    }
}

