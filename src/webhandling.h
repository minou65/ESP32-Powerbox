// webhandling.h

#ifndef _WEBHANDLING_h
#define _WEBHANDLING_h

#include <arduino.h>
#include <vector>
#include <HTTPClient.h>

#include <IotWebConf.h>
#include <IotWebConfOptionalGroup.h>

#include "common.h"

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "123456789";

extern void wifiInit();
extern void wifiLoop();

static WiFiClient wifiClient;

extern IotWebConf iotWebConf;

class Consumer {
public:
    enum Status {
        Disabled,
        Enabled,
		DelayedOff
    };

    virtual ~Consumer() {}
    virtual bool isEnabled() const = 0;
	virtual bool isActive() const { return true; }
    virtual void setEnabled(bool enabled) {};
    virtual uint32_t getPower() const = 0;
    virtual String getName() const = 0;
    virtual Status getStatus() {
        if (!isEnabled()) {
            return Disabled;
        } else {
			return Enabled;
        }
    }
    virtual void process() {}
    virtual void setup() {};
};

class Relay : public iotwebconf::ParameterGroup, public Consumer {
public:
    Relay(const char* id, uint8_t gpio)
        : ParameterGroup(id, "Relay"),
        _Enabled(false),
		_timer(60000),
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

    bool isEnabled() const override { 
		// Serial.print("Relay "); Serial.print(getName()); Serial.print(" enabled: "); Serial.println(_Enabled ? "true" : "false");
        return _Enabled;
    }
	bool isActive() const override { return getPower() > 0; }
    void setEnabled(bool enabled) override {
        if (_Enabled == enabled) return;
        _Enabled = enabled;
        if (!enabled) {
            _timer.start();
			//Serial.printf("Relay %s disabled, will turn off after delay\n", getName().c_str());
        }
        else {
            digitalWrite(getGPIO(), HIGH);
			//Serial.printf("Relay %s enabled\n", getName().c_str());
        }
        _Enabled = enabled; 
    }
    uint8_t getGPIO() const { return static_cast<uint8_t>(atoi(_GpioValue)); }
    uint32_t getPower() const override { return static_cast<uint32_t>(atoi(_PowerValue)); }
    Status getStatus() override {
        if (_Enabled) {
            return Enabled;
        }
        else {
            if (!_timer.done() && _timer.started()) {
                return DelayedOff;
            }
            else {
                return Disabled;
            }
		}
    }
    void setNext(Relay* nextGroup) {
        _nextGroup = nextGroup;
        if (nextGroup) nextGroup->_prevGroup = this;
    }
    Relay* getNext() const { return _nextGroup; }
	String getName() const override { return String(_DesignationValue); }


    void process() override {
        if (!isActive()) return;

        if (!_Enabled) {
            if (_timer.done()) {
                digitalWrite(getGPIO(), LOW);
			}
        }
	}

    void setup() override {
        pinMode(getGPIO(), OUTPUT);
        // Ensure the relay is off at startup
        digitalWrite(getGPIO(), LOW);
		setEnabled(false);
		_timer.stop();
	}

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

    Relay* _prevGroup;
    Relay* _nextGroup;
	Neotimer _timer;
};

class Shelly : public iotwebconf::ChainedParameterGroup, public Consumer {
public:
    Shelly(const char* id)
        : ChainedParameterGroup(id, "Shelly"),
        _Enabled(false),
        _timer(60000) {
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

    bool isEnabled() const override { return _Enabled; }
    bool isActive() const override {
        return ChainedParameterGroup::isActive() && getPower() > 0;
    }
    void setEnabled(bool enabled) override { _Enabled = enabled; }
    uint32_t getPower() const override { return static_cast<uint32_t>(atoi(_PowerValue)); }
    uint32_t getDelay() const { return static_cast<uint32_t>(atoi(_DelayValue)); }
	String getName() const { return String(_DesignationValue); }

    void process() override {
        if (!isActive()) return;

        // Get current time as string "HH:MM"
        time_t now_ = time(nullptr);
        struct tm* timeinfo_ = localtime(&now_);
        char timeStr_[6];
        strftime(timeStr_, sizeof(timeStr_), "%H:%M", timeinfo_);
        String currentTime_(timeStr_);

        bool shouldEnable_ = currentTime_ >= _TimeValue;
        if (shouldEnable_) {
            if (!isEnabled()) {
                sendHttpRequest(_UrlOnValue);
                setEnabled(true);
                _timer.start(getDelay() * 60 * 1000);
				Serial.printf("Shelly %s enabled, will turn off after %u minutes if power condition met\n", getName().c_str(), getDelay());
            }
        }
        else {
            if (isEnabled() && _timer.done()) {
                sendHttpRequest(_UrlOffValue);
                setEnabled(false);
				Serial.printf("Shelly %s disabled due to time condition\n", getName().c_str());
            }
        }
    }

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

    HTTPClient _http;
    Neotimer _timer;

    void sendHttpRequest(const String& url) {
        _http.begin(url);
        int httpCode = _http.GET();

        Serial.print("Invoked web request to "); Serial.println(url);

        if (httpCode > 0) {
            String payload = _http.getString();
            Serial.printf("Return code is %i\n", httpCode);
            //Serial.println(payload);
        }
        else {
            Serial.println("Error on HTTP request");
        }

        _http.end();
    }
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

extern std::vector<Consumer*> consumers;

#endif

