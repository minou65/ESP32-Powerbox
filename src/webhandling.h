// webhandling.h

#ifndef _WEBHANDLING_h
#define _WEBHANDLING_h

#include "arduino.h"

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
    Relay(const char* id, uint8_t gpio)
        : ParameterGroup(id, "Relay"),
        _Enabled(false),
        _prevGroup(nullptr),
        _nextGroup(nullptr) {
        snprintf(_NameId, STRING_LEN, "%s-name", getId());
        snprintf(_PowerId, STRING_LEN, "%s-power", getId());
        snprintf(_GpioId, STRING_LEN, "%s-gpio", getId());

        snprintf(_NameDefault, STRING_LEN, "%s", getId());
        snprintf(_GpioDefault, STRING_LEN, "%u", gpio);

        addItem(&_NameParam);
        addItem(&_PowerParam);
        addItem(&_GpioParam);
    }

    bool isEnabled() const { return _Enabled; }
    void setEnabled(bool enabled) { _Enabled = enabled; }
    uint8_t getGPIO() const { return static_cast<uint8_t>(atoi(_GpioValue)); }
    uint32_t getPower() const { return static_cast<uint32_t>(atoi(_PowerValue)); }

    void setNext(Relay* nextGroup) {
        _nextGroup = nextGroup;
        if (nextGroup) nextGroup->_prevGroup = this;
    }
    Relay* getNext() const { return _nextGroup; }
	String getName() const { return String(_DesignationValue); }



protected:
    Relay* _prevGroup;
    Relay* _nextGroup;

private:
    iotwebconf::TextParameter _NameParam = iotwebconf::TextParameter("Designation", _NameId, _DesignationValue, STRING_LEN, _NameDefault);
    iotwebconf::NumberParameter _PowerParam = iotwebconf::NumberParameter("Power (W)", _PowerId, _PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter _GpioParam = iotwebconf::NumberParameter("GPIO", _GpioId, _GpioValue, NUMBER_LEN, _GpioDefault, "0..255", "min='0' max='255' step='1'");

    char _PowerValue[NUMBER_LEN] = { 0 };
    char _GpioValue[NUMBER_LEN] = { 0 };
    char _DesignationValue[STRING_LEN];

    char _NameDefault[STRING_LEN] = { 0 };
    char _GpioDefault[STRING_LEN] = { 0 };
    char _NameId[STRING_LEN] = { 0 };
    char _PowerId[STRING_LEN] = { 0 };
    char _GpioId[STRING_LEN] = { 0 };

    bool _Enabled;
};

class Shelly : public iotwebconf::ChainedParameterGroup {
public:
    Shelly(const char* id)
        : ChainedParameterGroup(id, "Shelly"),
        _Enabled(false),
        _timer(1000) {
        snprintf(_NameId, STRING_LEN, "%s-name", getId());
        snprintf(_UrlOnId, STRING_LEN, "%s-urlon", getId());
        snprintf(_UrlOffId, STRING_LEN, "%s-urloff", getId());
        snprintf(_PowerId, STRING_LEN, "%s-power", getId());
        snprintf(_DelayId, STRING_LEN, "%s-delay", getId());
        snprintf(_TimeId, STRING_LEN, "%s-time", getId());

        snprintf(_NameDefault, STRING_LEN, "%s", getId());

        addItem(&_NameParam);
        addItem(&_UrlOnParam);
        addItem(&_UrlOffParam);
        addItem(&_PowerParam);
        addItem(&_DelayParam);
        addItem(&_TimeParam);
    }

    bool isEnabled() const { return _Enabled; }
    void setEnabled(bool enabled) { _Enabled = enabled; }
    uint32_t getPower() const { return static_cast<uint32_t>(atoi(_PowerValue)); }
    uint32_t getDelay() const { return static_cast<uint32_t>(atoi(_DelayValue)); }
	String getName() const { return String(_DesignationValue); }
	String url_OnValue() const { return String(_UrlOnValue); }
	String url_OffValue() const { return String(_UrlOffValue); }
	String TimeValue() const { return String(_TimeValue); }

    Neotimer _timer;

private:
    iotwebconf::TextParameter _NameParam = iotwebconf::TextParameter("Designation", _NameId, _DesignationValue, STRING_LEN, _NameDefault);
    iotwebconf::TextParameter _UrlOnParam = iotwebconf::TextParameter("URL on", _UrlOnId, _UrlOnValue, STRING_LEN, "http://");
    iotwebconf::TextParameter _UrlOffParam = iotwebconf::TextParameter("URL off", _UrlOffId, _UrlOffValue, STRING_LEN, "http://");
    iotwebconf::NumberParameter _PowerParam = iotwebconf::NumberParameter("Power (W)", _PowerId, _PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter _DelayParam = iotwebconf::NumberParameter("Power-off delay (minutes)", _DelayId, _DelayValue, NUMBER_LEN, "0", "0..300", "min='0' max='300' step='1'");
    iotwebconf::TimeParameter _TimeParam = iotwebconf::TimeParameter("Do not enable before ", _TimeId, _TimeValue, STRING_LEN, "00:00");

    char _NameDefault[STRING_LEN] = { 0 };
    char _NameId[STRING_LEN] = { 0 };
    char _UrlOnId[STRING_LEN] = { 0 };
    char _UrlOffId[STRING_LEN] = { 0 };
    char _PowerId[STRING_LEN] = { 0 };
    char _DelayId[STRING_LEN] = { 0 };
    char _TimeId[STRING_LEN] = { 0 };

    char _PowerValue[NUMBER_LEN] = { 0 };
    char _DelayValue[NUMBER_LEN] = { 0 };
    char _DesignationValue[STRING_LEN];
    char _UrlOnValue[STRING_LEN];
    char _UrlOffValue[STRING_LEN];
    char _TimeValue[STRING_LEN];

    bool _Enabled;
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

