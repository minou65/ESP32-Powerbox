// webhandling.h

#ifndef _WEBHANDLING_h
#define _WEBHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#if ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>      
#endif

#include <IotWebConf.h>
#include <IotWebConfOptionalGroup.h>
#include "common.h"

#define HTML_Start_Doc "<!DOCTYPE html>\
    <html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>\
    <title>{v}</title>\
";

#define HTML_Start_Body "</head><body>";
#define HTML_Start_Fieldset "<fieldset align=left style=\"border: 1px solid\">";
#define HTML_Start_Table "<table border=0 align=center>";
#define HTML_End_Table "</table>";
#define HTML_End_Fieldset "</fieldset>";
#define HTML_End_Body "</body>";
#define HTML_End_Doc "</html>";
#define HTML_Fieldset_Legend "<legend>{l}</legend>"
#define HTML_Table_Row "<tr><td>{n}</td><td>{v}</td></tr>";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "123456789";

extern void wifiInit();
extern void wifiLoop();

static WiFiClient wifiClient;

extern IotWebConf iotWebConf;


class httpGroup : public iotwebconf::ChainedParameterGroup {
public:
    httpGroup(const char* id) : ChainedParameterGroup(id, "httpGroup") {
        // -- Update parameter Ids to have unique ID for all parameters within the application.
        snprintf(Name_Id, STRING_LEN, "%s-name", this->getId());
        snprintf(urlON_Id, STRING_LEN, "%s-urlon", this->getId());
        snprintf(urlOFF_Id, STRING_LEN, "%s-urloff", this->getId());
        snprintf(Power_Id, STRING_LEN, "%s-power", this->getId());
        snprintf(Delay_Id, STRING_LEN, "%s-delay", this->getId());

        snprintf(Name_Default, STRING_LEN, "%s", this->getId());

        // -- Add parameters to this group.
        this->addItem(&this->NameParam);
        this->addItem(&this->urlONParam);
        this->addItem(&this->urlOFFParam);
        this->addItem(&this->powerParam);
        this->addItem(&this->DelayParam);
    };

    char DesignationValue[STRING_LEN];
    char url_OnValue[STRING_LEN];
    char url_OffValue[STRING_LEN];
    char PowerValue[NUMBER_LEN];
    char DelayValue[NUMBER_LEN];



    iotwebconf::TextParameter NameParam = iotwebconf::TextParameter("Designation", Name_Id, DesignationValue, STRING_LEN, Name_Default);
    iotwebconf::TextParameter urlONParam = iotwebconf::TextParameter("URL on", urlON_Id, url_OnValue, STRING_LEN, "http://");
    iotwebconf::TextParameter urlOFFParam = iotwebconf::TextParameter("URL off", urlOFF_Id, url_OffValue, STRING_LEN, "http://");
    iotwebconf::NumberParameter powerParam = iotwebconf::NumberParameter("Power (W)", Power_Id, PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter DelayParam = iotwebconf::NumberParameter("Power-off delay (minutes)", Delay_Id, DelayValue, NUMBER_LEN, "0", "0..300", "min='0' max='300' step='1'");

private:
    char Name_Default[STRING_LEN];
    char Name_Id[STRING_LEN];
    char urlON_Id[STRING_LEN];
    char urlOFF_Id[STRING_LEN];
    char Power_Id[STRING_LEN];
    char Delay_Id[STRING_LEN];
};



#endif

