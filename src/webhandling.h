// webhandling.h

#include <Arduino.h>
#ifndef _WEBHANDLING_h
#define _WEBHANDLING_h

#include <vector>
#include <HTTPClient.h>

#include <IotWebConf.h>
#include <IotWebConfAsync.h>
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

extern AsyncIotWebConf iotWebConf;
extern bool ShouldReboot;

class MySelectParameter : public iotwebconf::SelectParameter {
public:
    MySelectParameter(
        const char* label,
        const char* id,
        char* valueBuffer,
        int length,
        const char* optionValues,
        const char* optionNames,
        size_t optionCount,
        size_t nameLength,
        const char* defaultValue,
        const char* customHtml
    ) : iotwebconf::SelectParameter(label, id, valueBuffer, length, optionValues, optionNames, optionCount, nameLength, defaultValue) {
        this->customHtml = customHtml;
    }
};

const char DeviceTypeValues[][8] = {"-", "Relay", "URL" };
const char RelayTypeNames[][9] = { "Relay 1", "Relay 2", "Relay 3", "Relay 4"};
const char RelayTypeValues[][3] = { "22", "21", "17", "16" };

class SwitchDevice : public iotwebconf::ChainedParameterGroup {
public:
    enum DeviceType {notdefined, RelayType, UrlType };
    enum Status { Disabled, Enabled, DelayedOff };

        SwitchDevice(const char* id)
            :
            _id(id),
            ChainedParameterGroup(_id.c_str(), "Device")
        {

        snprintf(_DesignationValue, STRING_LEN, "%s", _id);
        snprintf(_PowerValue, NUMBER_LEN, "%u", 0);
        snprintf(_DelayValue, NUMBER_LEN, "%u", 1);
        snprintf(_OnTimeValue, STRING_LEN, "00:00");
        snprintf(_OffTimeValue, STRING_LEN, "23:59");
        snprintf(_UrlOnValue, STRING_LEN, "http://");
        snprintf(_UrlOffValue, STRING_LEN, "http://");

        snprintf(_DesignationId, STRING_LEN, "%s-designation", _id);
        snprintf(_PowerId, STRING_LEN, "%s-power", _id);
        snprintf(_DelayId, STRING_LEN, "%s-delay", _id);
        snprintf(_OnTimeId, STRING_LEN, "%s-ontime", _id);
        snprintf(_OffTimeId, STRING_LEN, "%s-offtime", _id);
        snprintf(_UrlOnId, STRING_LEN, "%s-urlon", _id);
        snprintf(_UrlOffId, STRING_LEN, "%s-urloff", _id);
		snprintf(_TypeSelectId, STRING_LEN, "%s-typeselect", _id);
        snprintf(_RelaySelectId, STRING_LEN, "%s-relayselect", _id);
		snprintf(_PartialLoadFactorId, STRING_LEN, "%s-partialloadfactor", _id);
		snprintf(_PartialLoadAllowedId, STRING_LEN, "%s-partialloadallowed", _id);

        addItem(&_TypeSelectParam);
        addItem(&_DesignationParam);
        addItem(&_PowerParam);
        addItem(&_DelayParam);
        addItem(&_OnTimeParam);
        addItem(&_OffTimeParam);
		addItem(&_RelaySelectParam);
        addItem(&_UrlOnParam);
        addItem(&_UrlOffParam);
        addItem(&_PartialLoadAllowedParam);
        addItem(&_PartialLoadFactorParam);
    }

    // Status und Aktivierung
    bool isEnabled() const { return _Enabled; }
    void setEnabled(bool enabled) {
        if (enabled == _Enabled) return;
        _Enabled = enabled;
        if (_Enabled) {
            _timer.stop();
        }
        else {
            _timer.set(getDelay() * 60000);
            _timer.start();
        }
    }
    Status getStatus() {
        if (_Enabled) return Enabled;
        if (!_timer.done() && _timer.started()) return DelayedOff;
        return Disabled;
    }
    
    bool isActive() const override {
        return ChainedParameterGroup::isActive() && getPower() > 0;
    }

    void process() {
        if (!isActive()) return;
        // Get current time as string "HH:MM"
        time_t now_ = time(nullptr);
        struct tm* timeinfo_ = localtime(&now_);
        char timeStr_[6];
        strftime(timeStr_, sizeof(timeStr_), "%H:%M", timeinfo_);
        String currentTime_(timeStr_);

        bool shouldEnable_ = currentTime_ >= getOnTime();
        bool shouldDisable_ = currentTime_ >= getOffTime();

        if (_Enabled && shouldEnable_ && !shouldDisable_) {
            if (getType() == RelayType) {
                pinMode(getGPIO(), OUTPUT);
                digitalWrite(getGPIO(), HIGH);
            }
            else if (getType() == UrlType) {
                sendHttpRequest(getUrlOn());
            }
            _timer.stop();
        }
        else if (!_Enabled && _timer.done() && _timer.started()) {
            if (getType() == RelayType) {
                digitalWrite(getGPIO(), LOW);
            }
            else if (getType() == UrlType) {
                sendHttpRequest(getUrlOff());
            }
            _timer.stop();
            Serial.printf("Device %s turned off after delay\n", getDesignation().c_str());
        }
    }

    void begin() {
        if (getType() == RelayType) {
            pinMode(getGPIO(), OUTPUT);
            digitalWrite(getGPIO(), LOW);
        }
        setEnabled(false);
        _timer.stop();
    }

    void end() {
        if (getType() == RelayType) {
            digitalWrite(getGPIO(), LOW);
        }
        else if (getType() == UrlType) {
            sendHttpRequest(getUrlOff());
        }
    }

    void applyDefaultValue() {
        _DesignationParam.applyDefaultValue();
        _PowerParam.applyDefaultValue();
        _DelayParam.applyDefaultValue();
        _OnTimeParam.applyDefaultValue();
        _OffTimeParam.applyDefaultValue();
        _UrlOnParam.applyDefaultValue();
        _UrlOffParam.applyDefaultValue();
		_TypeSelectParam.applyDefaultValue();
		_RelaySelectParam.applyDefaultValue();
        _PartialLoadAllowedParam.applyDefaultValue();
		_PartialLoadFactorParam.applyDefaultValue();
    }

    String getDesignation() const { return String(_DesignationValue); }
    uint32_t getPower() const { return static_cast<uint32_t>(atoi(_PowerValue)); }
    uint32_t getDelay() const { return static_cast<uint32_t>(atoi(_DelayValue)); }
    DeviceType getType() const {
        String typeStr = String(_TypeSelectValue);
        if (typeStr == "Relay") return RelayType;
        else if (typeStr == "URL") return UrlType;
        else return notdefined; // Default
	}
    String getOnTime() const { return String(_OnTimeValue); }
    String getOffTime() const { return String(_OffTimeValue); }
    uint8_t getGPIO() const { return static_cast<uint8_t>(atoi(_RelaySelectValue)); }
    String getUrlOn() const { return String(_UrlOnValue); }
    String getUrlOff() const { return String(_UrlOffValue); }

    float getPartialLoadThreshold() const { return atof(_PartialLoadFactorValue); }
    void setPartialLoadThreshold(float threshold) {
        snprintf(_PartialLoadFactorValue, sizeof(_PartialLoadFactorValue), "%.2f", threshold);
    }
    bool isPartialLoadAllowed() const {
        return strcmp(_PartialLoadAllowedValue, "1") == 0 || strcmp(_PartialLoadAllowedValue, "true") == 0;
    }


private:
    char _DesignationValue[STRING_LEN] = { 0 };
    char _PowerValue[NUMBER_LEN] = { 0 };
    char _DelayValue[NUMBER_LEN] = { 0 };
    char _OnTimeValue[STRING_LEN] = { 0 };
    char _OffTimeValue[STRING_LEN] = { 0 };
    char _UrlOnValue[STRING_LEN] = { 0 };
    char _UrlOffValue[STRING_LEN] = { 0 };
	char _TypeSelectValue[STRING_LEN] = { 0 };
    char _RelaySelectValue[NUMBER_LEN] = { 0 };
    char _PartialLoadFactorValue[NUMBER_LEN] = { 0 };
	char _PartialLoadAllowedValue[STRING_LEN] = { 0 };

    char _DesignationId[STRING_LEN];
    char _PowerId[STRING_LEN];
    char _DelayId[STRING_LEN];
    char _OnTimeId[STRING_LEN];
    char _OffTimeId[STRING_LEN];
    char _TypeId[STRING_LEN];
    char _UrlOnId[STRING_LEN];
    char _UrlOffId[STRING_LEN];
	char _TypeSelectId[STRING_LEN];
	char _RelaySelectId[STRING_LEN];
	char _PartialLoadFactorId[STRING_LEN];
	char _PartialLoadAllowedId[STRING_LEN];

    String _id;

    iotwebconf::TextParameter _DesignationParam = iotwebconf::TextParameter("Device Name", _DesignationId, _DesignationValue, STRING_LEN, "");
    iotwebconf::NumberParameter _PowerParam = iotwebconf::NumberParameter("Power (W)", _PowerId, _PowerValue, NUMBER_LEN, "0", "0..10000", "min='0' max='10000' step='1'");
    iotwebconf::NumberParameter _DelayParam = iotwebconf::NumberParameter("Power-off Delay (minutes)", _DelayId, _DelayValue, NUMBER_LEN, "1", "1..300", "min='1' max='300' step='1'");
    iotwebconf::TimeParameter _OnTimeParam = iotwebconf::TimeParameter("Allowed ON from", _OnTimeId, _OnTimeValue, STRING_LEN, "00:00");
    iotwebconf::TimeParameter _OffTimeParam = iotwebconf::TimeParameter("Allowed ON until", _OffTimeId, _OffTimeValue, STRING_LEN, "23:59");
    iotwebconf::TextParameter _UrlOnParam = iotwebconf::TextParameter("URL On", _UrlOnId, _UrlOnValue, STRING_LEN, "http://");
    iotwebconf::TextParameter _UrlOffParam = iotwebconf::TextParameter("URL Off", _UrlOffId, _UrlOffValue, STRING_LEN, "http://");

    MySelectParameter _TypeSelectParam = MySelectParameter(
        "Type", 
        _TypeSelectId, 
        _TypeSelectValue, 
        sizeof(DeviceTypeValues[0]), 
        (char*)DeviceTypeValues, 
        (char*)DeviceTypeValues, 
        sizeof(DeviceTypeValues) / sizeof(DeviceTypeValues[0]), 
        sizeof(DeviceTypeValues[0]), 
        DeviceTypeValues[0], 
        nullptr
    );

    MySelectParameter _RelaySelectParam = MySelectParameter(
        "Relay", 
        _RelaySelectId, 
        _RelaySelectValue,
        sizeof(RelayTypeValues[0]),
        (char*)RelayTypeValues,
        (char*)RelayTypeNames,
        sizeof(RelayTypeValues) / sizeof(RelayTypeValues[0]),
        sizeof(RelayTypeNames[0]),
        RelayTypeValues[0],
        nullptr
    );

    iotwebconf::CheckboxParameter _PartialLoadAllowedParam = iotwebconf::CheckboxParameter(
        "Allow Partial Load Activation", 
        _PartialLoadAllowedId,
        _PartialLoadAllowedValue,
        STRING_LEN, 
        false
    );

	iotwebconf::NumberParameter _PartialLoadFactorParam = iotwebconf::NumberParameter(
        "Partial Load Threshold (0.0 - 1.0)", 
        _PartialLoadFactorId,
        _PartialLoadFactorValue, 
        NUMBER_LEN, 
        "0.50", 
        "0.0..1.0", 
        "min='0.0' max='1.0' step='0.01'"
    );


    bool _Enabled;
    Neotimer _timer;
    HTTPClient _http;
    Status _lastStatus;
    Neotimer _sendTimer;

    void sendHttpRequest(const String& url) {
        if (iotWebConf.getState() != iotwebconf::OnLine) return;
        if (_lastStatus != getStatus()) {
            _lastStatus = getStatus();
            _sendTimer.start();
        }
        else if (!_sendTimer.repeat()) {
            return;
        }
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

class InverterConfig : public iotwebconf::ParameterGroup {
public:
    InverterConfig()
        : iotwebconf::ParameterGroup("SysConf", "Inverter") {
        snprintf(_IPAddressValue, sizeof(_IPAddressValue), "%s", "192.168.1.105");
        snprintf(_PortValue, sizeof(_PortValue), "%u", 502);
        snprintf(_IntervalValue, sizeof(_IntervalValue), "%u", 10);

        addItem(&_IPAddressParam);
        addItem(&_PortParam);
        addItem(&_IntervalParam);
    }

    void applyDefaultValue() {
        _IPAddressParam.applyDefaultValue();
        _PortParam.applyDefaultValue();
        _IntervalParam.applyDefaultValue();
    }

    String getIPAddress() const { return String(_IPAddressValue); }
    uint16_t getPort() const { return static_cast<uint16_t>(atoi(_PortValue)); }
    uint16_t getInterval() const { return static_cast<uint16_t>( atoi(_IntervalValue)); }

private:
    char _IPAddressValue[STRING_LEN] = { 0 };
    char _PortValue[NUMBER_LEN] = { 0 };
    char _IntervalValue[NUMBER_LEN] = { 0 };

    iotwebconf::TextParameter _IPAddressParam = iotwebconf::TextParameter("IPAddress", "InverterIPAddress", _IPAddressValue, STRING_LEN, "192.168.1.105");
    iotwebconf::NumberParameter _PortParam = iotwebconf::NumberParameter("Port", "InverterPort", _PortValue, NUMBER_LEN, "502", "1..65535", "min='1' max='65535' step='1'");
    iotwebconf::NumberParameter _IntervalParam = iotwebconf::NumberParameter("Interval (seconds)", "InverterActivePowerInterval", _IntervalValue, NUMBER_LEN, "10", "10..255", "min='10' max='255' step='1'");
};

class NtpConfig : public iotwebconf::ParameterGroup {
public:
    NtpConfig()
        : iotwebconf::ParameterGroup("TimeSourceGroup", "Time Source") {
        snprintf(_UseNtpValue, sizeof(_UseNtpValue), "%s", "1");
        snprintf(_NtpServerValue, sizeof(_NtpServerValue), "%s", "pool.ntp.org");
        snprintf(_TimeZoneValue, sizeof(_TimeZoneValue), "%s", "CET-1CEST,M3.5.0,M10.5.0/3");

        addItem(&_UseNtpParam);
        addItem(&_NtpServerParam);
        addItem(&_TimeZoneParam);
    }

    void applyDefaultValue() {
        _UseNtpParam.applyDefaultValue();
        _NtpServerParam.applyDefaultValue();
        _TimeZoneParam.applyDefaultValue();
    }

    bool useNtpServer() const {
        return (strcmp(_UseNtpValue, "selected") == 0);
    }
    String ntpServer() const { return String(_NtpServerValue); }
    String timeZone() const { return String(_TimeZoneValue); }

private:
    char _UseNtpValue[STRING_LEN] = { 0 };
    char _NtpServerValue[STRING_LEN] = { 0 };
    char _TimeZoneValue[STRING_LEN] = { 0 };

    iotwebconf::CheckboxParameter _UseNtpParam = iotwebconf::CheckboxParameter("Use NTP server", "UseNTPServerParam", _UseNtpValue, STRING_LEN, true);
    iotwebconf::TextParameter _NtpServerParam = iotwebconf::TextParameter("NTP server (FQDN or IP address)", "NTPServerParam", _NtpServerValue, STRING_LEN, "pool.ntp.org");
    iotwebconf::TextParameter _TimeZoneParam = iotwebconf::TextParameter("POSIX timezones string", "TimeOffsetParam", _TimeZoneValue, STRING_LEN, "CET-1CEST,M3.5.0,M10.5.0/3");
};

extern InverterConfig inverterConfig;
extern NtpConfig ntpConfig;

extern std::vector<SwitchDevice*> devices;

#endif

