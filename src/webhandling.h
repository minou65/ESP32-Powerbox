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

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "123456789";

extern void wifiInit();
extern void wifiLoop();

static WiFiClient wifiClient;

extern IotWebConf iotWebConf;

class Relay : public iotwebconf::ParameterGroup {
public:
    Relay(const char* id, const uint8_t gpio_) : ParameterGroup(id, "Relay") {
        // -- Update parameter Ids to have unique ID for all parameters within the application.
        snprintf(Name_Id, STRING_LEN, "%s-name", this->getId());
        snprintf(Power_Id, STRING_LEN, "%s-power", this->getId());
        snprintf(gpio_Id, STRING_LEN, "%s-gpio", this->getId());

        snprintf(Name_Default, STRING_LEN, "%s", this->getId());
        snprintf(GPIO_Default, STRING_LEN, "%i", gpio_);

        // -- Add parameters to this group.
        this->addItem(&this->NameParam);
        this->addItem(&this->powerParam);
        this->addItem(&this->gpioParam);
    }

    char DesignationValue[STRING_LEN];

    bool IsEnabled() { return Enabled; };
    void SetEnabled(bool enabled_) { Enabled = enabled_; };
    uint8_t GetGPIO() { return atoi(GPIOValue); };
    uint32_t GetPower() { return atoi(PowerValue); };

    void setNext(Relay* nextGroup) { this->nextGroup = nextGroup; nextGroup->prevGroup = this; };
    Relay* getNext() { return this->nextGroup; };

protected:
    Relay* prevGroup = nullptr;
    Relay* nextGroup = nullptr;

private:

    iotwebconf::TextParameter NameParam = iotwebconf::TextParameter("Designation", Name_Id, DesignationValue, STRING_LEN, Name_Default);
    iotwebconf::NumberParameter powerParam = iotwebconf::NumberParameter("Power (W)", Power_Id, PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter gpioParam = iotwebconf::NumberParameter("GPIO", gpio_Id, GPIOValue, NUMBER_LEN, GPIO_Default, "0..255", "min='0' max='255' step='1'");

    char PowerValue[NUMBER_LEN];
    char GPIOValue[NUMBER_LEN];

    char Name_Default[STRING_LEN];
    char GPIO_Default[STRING_LEN];
    char Name_Id[STRING_LEN];
    char Power_Id[STRING_LEN];
    char gpio_Id[STRING_LEN];

    bool Enabled = false;
};

class Shelly : public iotwebconf::ChainedParameterGroup {
public:
    Shelly(const char* id) : ChainedParameterGroup(id, "Shelly") {
        // -- Update parameter Ids to have unique ID for all parameters within the application.
        snprintf(Name_Id, STRING_LEN, "%s-name", this->getId());
        snprintf(urlON_Id, STRING_LEN, "%s-urlon", this->getId());
        snprintf(urlOFF_Id, STRING_LEN, "%s-urloff", this->getId());
        snprintf(Power_Id, STRING_LEN, "%s-power", this->getId());
        snprintf(Delay_Id, STRING_LEN, "%s-delay", this->getId());
        snprintf(Time_Id, STRING_LEN, "%s-time", this->getId());

        snprintf(Name_Default, STRING_LEN, "%s", this->getId());

        // -- Add parameters to this group.
        this->addItem(&this->NameParam);
        this->addItem(&this->urlONParam);
        this->addItem(&this->urlOFFParam);
        this->addItem(&this->powerParam);
        this->addItem(&this->DelayParam);
        this->addItem(&this->TimeParam);

    };

    char DesignationValue[STRING_LEN];
    char url_OnValue[STRING_LEN];
    char url_OffValue[STRING_LEN];
    char TimeValue[STRING_LEN];

    uint32_t GetPower() { return atoi(PowerValue); };
    uint32_t GetDelay() { return atoi(DelayValue); };
    void SetEnabled(bool enabled_) { Enabled = enabled_; };
    bool IsEnabled(){ return Enabled; };

    Neotimer timer = Neotimer(1000);

private:
    iotwebconf::TextParameter NameParam = iotwebconf::TextParameter("Designation", Name_Id, DesignationValue, STRING_LEN, Name_Default);
    iotwebconf::TextParameter urlONParam = iotwebconf::TextParameter("URL on", urlON_Id, url_OnValue, STRING_LEN, "http://");
    iotwebconf::TextParameter urlOFFParam = iotwebconf::TextParameter("URL off", urlOFF_Id, url_OffValue, STRING_LEN, "http://");
    iotwebconf::NumberParameter powerParam = iotwebconf::NumberParameter("Power (W)", Power_Id, PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter DelayParam = iotwebconf::NumberParameter("Power-off delay (minutes)", Delay_Id, DelayValue, NUMBER_LEN, "0", "0..300", "min='0' max='300' step='1'");
    iotwebconf::TimeParameter TimeParam = iotwebconf::TimeParameter("Do not enable before ", Time_Id, TimeValue, STRING_LEN, "00:00");

    char Name_Default[STRING_LEN];
    char Name_Id[STRING_LEN];
    char urlON_Id[STRING_LEN];
    char urlOFF_Id[STRING_LEN];
    char Power_Id[STRING_LEN];
    char Delay_Id[STRING_LEN];
    char Time_Id[STRING_LEN];

    bool Enabled = false;

    char PowerValue[NUMBER_LEN];
    char DelayValue[NUMBER_LEN];
    
};

extern Relay Relay1;
extern Relay Relay2;
extern Relay Relay3;
extern Relay Relay4;

extern Shelly Shelly1;
extern Shelly Shelly2;
extern Shelly Shelly3;
extern Shelly Shelly4;
extern Shelly Shelly5;
extern Shelly Shelly6;
extern Shelly Shelly7;
extern Shelly Shelly8;
extern Shelly Shelly9;
extern Shelly Shelly10;

#endif

