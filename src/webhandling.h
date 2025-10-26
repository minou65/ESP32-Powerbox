// webhandling.h

#ifndef _WEBHANDLING_h
#define _WEBHANDLING_h

#include <arduino.h>
#include <vector>
#include <HTTPClient.h>

#include <IotWebConf.h>
#include <IotWebConfOptionalGroup.h>
#include <WebSerial.h>

#include "common.h"


#define SERIAL_WEB_SERIAL(...) \
    do { \
        Serial.print(__VA_ARGS__); \
        WebSerial.print(__VA_ARGS__); \
    } while(0)

#define SERIAL_WEB_SERIALLN(...) \
    do { \
        Serial.println(__VA_ARGS__); \
        WebSerial.println(__VA_ARGS__); \
    } while(0)
#define SERIAL_WEB_SERIALF(fmt, ...) \
    do { \
        Serial.printf(fmt, __VA_ARGS__); \
        char _buf[128]; \
        snprintf(_buf, sizeof(_buf), fmt, __VA_ARGS__); \
        WebSerial.print(_buf); \
    } while(0)

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "123456789";

extern void wifiInit();
extern void wifiLoop();

static WiFiClient wifiClient;

extern IotWebConf iotWebConf;
extern bool ShouldReboot;

class Consumer {
protected:
    bool _Enabled = false;
    Neotimer _timer;
    bool _partialLoadAllowed = false;
    float _partialLoadThreshold = 0.5f;
    char _NameValue[STRING_LEN] = { 0 };
    char _PowerValue[NUMBER_LEN] = { 0 };
    char _DelayValue[NUMBER_LEN] = { 0 };
    char _TimeValue[STRING_LEN] = { 0 };

public:
    enum Status { Disabled, Enabled, DelayedOff };

    virtual ~Consumer() {}

    // Status und Aktivierung
    virtual bool isEnabled() const { return _Enabled; }
    virtual void setEnabled(bool enabled) {
        if (enabled == _Enabled) return;
        _Enabled = enabled;
        if (_Enabled) _timer.stop();
        else {
            _timer.set(getDelay() * 60000);
            _timer.start();
        }
    }
    virtual Status getStatus() {
        if (_Enabled) return Enabled;
        if (!_timer.done() && _timer.started()) return DelayedOff;
        return Disabled;
    }
    virtual bool isActive() const { return getPower() > 0; }

    // Partial Load
    virtual bool isPartialLoadAllowed() const { return _partialLoadAllowed; }
    virtual void setPartialLoadAllowed(bool allowed) { _partialLoadAllowed = allowed; }
    virtual float getPartialLoadThreshold() const { return _partialLoadThreshold; }
    virtual void setPartialLoadThreshold(float threshold) { _partialLoadThreshold = threshold; }

    // Parameter
    virtual uint32_t getPower() const { return static_cast<uint32_t>(atoi(_PowerValue)); }
    virtual String getName() const { return String(_NameValue); }
    virtual uint32_t getDelay() const { return static_cast<uint32_t>(atoi(_DelayValue)); }
    virtual String getEnableTime() const { return String(_TimeValue); }

    // Hooks für abgeleitete Klassen
    virtual void process() {}
    virtual void begin() {}
	virtual void end() {}
    virtual void applyDefaultValue() {}
};

class Relay : public iotwebconf::ParameterGroup, public Consumer {
public:
    Relay(const char* id, uint8_t gpio)
        : ParameterGroup(id, "Relay"),
        _prevGroup(nullptr),
        _nextGroup(nullptr) {
        snprintf(_NameId, STRING_LEN, "%s-name", id);
        snprintf(_PowerId, STRING_LEN, "%s-power", id);
        snprintf(_GpioId, STRING_LEN, "%s-gpio", id);
        snprintf(_DelayId, STRING_LEN, "%s-delay", id);
        snprintf(_TimeId, STRING_LEN, "%s-time", id);

        snprintf(_NameDefault, STRING_LEN, "%s", id);
        snprintf(_GpioDefault, STRING_LEN, "%u", gpio);

        addItem(&_NameParam);
        addItem(&_PowerParam);
        addItem(&_GpioParam);
        addItem(&_DelayParam);
        addItem(&_TimeParam);

		_Enabled = false;

    }

    uint8_t getGPIO() const { return static_cast<uint8_t>(atoi(_GpioValue)); }

    void setNext(Relay* nextGroup) {
        _nextGroup = nextGroup;
        if (nextGroup) nextGroup->_prevGroup = this;
    }
    Relay* getNext() const { return _nextGroup; }

    void process() override {
        if (!isActive()) return;

        // Get current time as string "HH:MM"
        time_t now_ = time(nullptr);
        struct tm* timeinfo_ = localtime(&now_);
        char timeStr_[6];
        strftime(timeStr_, sizeof(timeStr_), "%H:%M", timeinfo_);
        String currentTime_(timeStr_);

        bool shouldEnable_ = currentTime_ >= _TimeValue;

        if (_Enabled && shouldEnable_) {
            digitalWrite(getGPIO(), HIGH);
            _timer.stop();
        }
		else if (!_Enabled) {
            if (_timer.done() && _timer.started()) {
                digitalWrite(getGPIO(), LOW);
                _timer.stop();
                Serial.printf("Relay %s turned off after delay\n", getName().c_str());
			}
        }

        if (currentTime_ > "13:00") {
			setPartialLoadAllowed(true);
		}
        else if (currentTime_ > "18:00") {
            setPartialLoadAllowed(false);
        }

    }

    void begin() override {
        pinMode(getGPIO(), OUTPUT);
        digitalWrite(getGPIO(), LOW);
        setEnabled(false);
        _timer.stop();
    }

    void end() override {
        digitalWrite(getGPIO(), LOW);
	}

    void applyDefaultValue() override {
        _NameParam.applyDefaultValue();
        _PowerParam.applyDefaultValue();
        _GpioParam.applyDefaultValue();
        _DelayParam.applyDefaultValue();
        _TimeParam.applyDefaultValue();
    }

private:
    iotwebconf::TextParameter _NameParam = iotwebconf::TextParameter("Designation", _NameId, _NameValue, STRING_LEN, _NameDefault);
    iotwebconf::NumberParameter _PowerParam = iotwebconf::NumberParameter("Power (W)", _PowerId, _PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter _GpioParam = iotwebconf::NumberParameter("GPIO", _GpioId, _GpioValue, NUMBER_LEN, _GpioDefault, "0..255", "min='0' max='255' step='1'");
    iotwebconf::NumberParameter _DelayParam = iotwebconf::NumberParameter("Power-off delay (minutes)", _DelayId, _DelayValue, NUMBER_LEN, "1", "1..300", "min='1' max='300' step='1'");
    iotwebconf::TimeParameter _TimeParam = iotwebconf::TimeParameter("Do not enable before ", _TimeId, _TimeValue, STRING_LEN, "00:00");

    char _GpioValue[NUMBER_LEN] = { 0 };

    char _NameDefault[STRING_LEN] = { 0 };
    char _GpioDefault[STRING_LEN] = { 0 };
    char _NameId[STRING_LEN] = { 0 };
    char _PowerId[STRING_LEN] = { 0 };
    char _GpioId[STRING_LEN] = { 0 };
    char _DelayId[STRING_LEN] = { 0 };
    char _TimeId[STRING_LEN] = { 0 };

    Relay* _prevGroup;
    Relay* _nextGroup;
};

class Shelly : public iotwebconf::ChainedParameterGroup, public Consumer {
public:
    Shelly(const char* id)
        : ChainedParameterGroup(id, "Shelly") {
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

        _Enabled = false;
    }

    bool isActive() const override {
        return ChainedParameterGroup::isActive() && getPower() > 0;
    }

    void process() override {
        if (!isActive()) return;

        // Get current time as string "HH:MM"
        time_t now_ = time(nullptr);
        struct tm* timeinfo_ = localtime(&now_);
        char timeStr_[6];
        strftime(timeStr_, sizeof(timeStr_), "%H:%M", timeinfo_);
        String currentTime_(timeStr_);

        bool shouldEnable_ = currentTime_ >= _TimeValue;

        if (_Enabled && shouldEnable_) {
            sendHttpRequest(_UrlOnValue);
            _timer.stop();
        }
        else if (!_Enabled) {
            if (_timer.done() && _timer.started()) {
                sendHttpRequest(_UrlOffValue);
                _timer.stop();
                Serial.printf("Shelly %s disabled due to time condition\n", getName().c_str());
            }
        }
    }

    void begin() override {
        setEnabled(false);
        _timer.stop();
	}

    void end() override {
        sendHttpRequest(_UrlOffValue);
    }

    void applyDefaultValue() override {
        _NameParam.applyDefaultValue();
        _UrlOnParam.applyDefaultValue();
        _UrlOffParam.applyDefaultValue();
        _PowerParam.applyDefaultValue();
        _DelayParam.applyDefaultValue();
        _TimeParam.applyDefaultValue();
    }

private:
    iotwebconf::TextParameter _NameParam = iotwebconf::TextParameter("Designation", _NameId, _NameValue, STRING_LEN, _NameDefault);
    iotwebconf::TextParameter _UrlOnParam = iotwebconf::TextParameter("URL on", _UrlOnId, _UrlOnValue, STRING_LEN, "http://");
    iotwebconf::TextParameter _UrlOffParam = iotwebconf::TextParameter("URL off", _UrlOffId, _UrlOffValue, STRING_LEN, "http://");
    iotwebconf::NumberParameter _PowerParam = iotwebconf::NumberParameter("Power (W)", _PowerId, _PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter _DelayParam = iotwebconf::NumberParameter("Power-off delay (minutes)", _DelayId, _DelayValue, NUMBER_LEN, "1", "1..300", "min='1' max='300' step='1'");
    iotwebconf::TimeParameter _TimeParam = iotwebconf::TimeParameter("Do not enable before ", _TimeId, _TimeValue, STRING_LEN, "00:00");

    char _NameDefault[STRING_LEN] = { 0 };
    char _NameId[STRING_LEN] = { 0 };
    char _UrlOnId[STRING_LEN] = { 0 };
    char _UrlOffId[STRING_LEN] = { 0 };
    char _PowerId[STRING_LEN] = { 0 };
    char _DelayId[STRING_LEN] = { 0 };
    char _TimeId[STRING_LEN] = { 0 };

    char _UrlOnValue[STRING_LEN];
    char _UrlOffValue[STRING_LEN];

    HTTPClient _http;

    void sendHttpRequest(const String& url) {
        _http.begin(url);
        int httpCode = _http.GET();

        Serial.print("Invoked web request to "); Serial.println(url);

        if (httpCode > 0) {
            String payload = _http.getString();
            Serial.printf("Return code is %i\n", httpCode);
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

