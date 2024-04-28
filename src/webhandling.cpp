// 
// 
// 

#define DEBUG_WIFI(m) SERIAL_DBG.print(m)

#include <Arduino.h>
#include <ArduinoOTA.h>
#if ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>      
#endif

#include <time.h>
#include "ESP32Time.h"

#include <DNSServer.h>
#include<iostream>
#include <string.h>

#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#include <IotWebConfTParameter.h>
# include <IotWebConfESP32HTTPUpdateServer.h>
#include "common.h"
#include "webhandling.h"
#include "favicon.h"
#include "IotWebRoot.h"

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "A4"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN  -1

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN
#if ESP32 
#define ON_LEVEL HIGH
#else
#define ON_LEVEL LOW
#endif

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "PowerBox";

// -- Method declarations.
void handleRoot();
void handleFavIcon();
void handleDateTime();
void handlePower();
void handleData();
void handleReboot();
void convertParams();

// -- Callback methods.
void configSaved();
void wifiConnected();

bool gParamsChanged = true;

DNSServer dnsServer;
WebServer server(80);

class CustomHtmlFormatProvider : public iotwebconf::OptionalGroupHtmlFormatProvider {
protected:
    virtual String getFormEnd() {
        String _s = OptionalGroupHtmlFormatProvider::getFormEnd();
        _s += F("</br><form action='/reboot' method='get'><button type='submit'>Reboot</button></form>");
        _s += F("</br>Return to <a href='/'>home page</a>.");
		return _s;
	}
};
CustomHtmlFormatProvider customHtmlFormatProvider;

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

IotWebConfParameterGroup sysConfGroup = IotWebConfParameterGroup("SysConf", "Inverter");

iotwebconf::TextTParameter<15> InverterIPAddress =
    iotwebconf::Builder<iotwebconf::TextTParameter<sizeof(gInverterIPAddress)>>("InverterIPAddress").
    label("IPAddress").
    defaultValue("192.168.1.105").
    build();

iotwebconf::UIntTParameter<uint16_t> InverterPort =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("InverterPort").
    label("Port").
    defaultValue(502).
    min(1u).
    step(1).
    placeholder("1..65535").
    build();

iotwebconf::UIntTParameter<uint16_t> InverterInputPowerRegister =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("InverterInputPowerRegister").
    label("Register Input power").
    defaultValue(32064).
    min(1u).
    step(1).
    placeholder("1..65535").
    build();

iotwebconf::UIntTParameter<uint8_t> InverterInputPowerDataLength =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint8_t>>("InverterInputPowerDataLength").
    label("Data Length").
    defaultValue(2).
    min(1u).
    step(1).
    placeholder("1..255").
    build();

iotwebconf::UIntTParameter<uint16_t> InverterInputPowerGain =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("InverterInputPowerGain").
    label("Gain").
    defaultValue(1).
    min(1u).
    step(1).
    placeholder("1..65535").
    build();

iotwebconf::UIntTParameter<uint16_t> InverterActivePowerInterval =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("InverterActivePowerInterval").
    label("Interval (seconds)").
    defaultValue(10).
    min(10).
    max(255).
    step(1).
    placeholder("10..255").
    build();

char vUseNTPServer[STRING_LEN];
char vNTPServer[STRING_LEN];
char vTimeZone[STRING_LEN];
iotwebconf::ParameterGroup TimeSourceGroup = iotwebconf::ParameterGroup("TimeSourceGroup", "Time Source");
iotwebconf::CheckboxParameter cUseNTPServer = iotwebconf::CheckboxParameter("Use NTP server", "UseNTPServerParam", vUseNTPServer, STRING_LEN, true);
iotwebconf::TextParameter cNTPServerAddress = iotwebconf::TextParameter("NTP server (FQDN or IP address)", "NTPServerParam", vNTPServer, STRING_LEN, "pool.ntp.org");
iotwebconf::TextParameter cgmtOffset_sec = iotwebconf::TextParameter("POSIX timezones string", "TimeOffsetParam", vTimeZone, STRING_LEN, "CET-1CEST,M3.5.0,M10.5.0/3");


Relay Relay1 = Relay("Relay1", GPIO_NUM_22);
Relay Relay2 = Relay("Relay2", GPIO_NUM_21);
Relay Relay3 = Relay("Relay3", GPIO_NUM_17);
Relay Relay4 = Relay("Relay4", GPIO_NUM_16);


Shelly Shelly1 = Shelly("Shelly1");
Shelly Shelly2 = Shelly("Shelly2");
Shelly Shelly3 = Shelly("Shelly3");
Shelly Shelly4 = Shelly("Shelly4");
Shelly Shelly5 = Shelly("Shelly5");
Shelly Shelly6 = Shelly("Shelly6");
Shelly Shelly7 = Shelly("Shelly7");
Shelly Shelly8 = Shelly("Shelly8");
Shelly Shelly9 = Shelly("Shelly9");
Shelly Shelly10 = Shelly("Shelly10");

HTTPUpdateServer httpUpdater;

void wifiInit() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("starting up...");


    iotWebConf.setStatusPin(STATUS_PIN, ON_LEVEL);
    iotWebConf.setConfigPin(CONFIG_PIN);

    iotWebConf.setHtmlFormatProvider(&customHtmlFormatProvider);

    Relay1.setNext(&Relay2);
    Relay2.setNext(&Relay3);
    Relay3.setNext(&Relay4);

    Shelly1.setNext(&Shelly2);
    Shelly2.setNext(&Shelly3);
    Shelly3.setNext(&Shelly4);
    Shelly4.setNext(&Shelly5);
    Shelly5.setNext(&Shelly6);
    Shelly6.setNext(&Shelly7);
    Shelly7.setNext(&Shelly8);
    Shelly8.setNext(&Shelly9);
    Shelly10.setNext(&Shelly10);

    sysConfGroup.addItem(&InverterIPAddress);
    sysConfGroup.addItem(&InverterPort);
    sysConfGroup.addItem(&InverterInputPowerRegister);
    sysConfGroup.addItem(&InverterInputPowerDataLength);
    sysConfGroup.addItem(&InverterInputPowerGain);
    sysConfGroup.addItem(&InverterActivePowerInterval);

    iotWebConf.addParameterGroup(&sysConfGroup);

    TimeSourceGroup.addItem(&cUseNTPServer);
    TimeSourceGroup.addItem(&cNTPServerAddress);
    TimeSourceGroup.addItem(&cgmtOffset_sec);

    iotWebConf.addParameterGroup(&TimeSourceGroup);

    iotWebConf.addParameterGroup(&Relay1);
    iotWebConf.addParameterGroup(&Relay2);
    iotWebConf.addParameterGroup(&Relay3);
    iotWebConf.addParameterGroup(&Relay4);

    iotWebConf.addParameterGroup(&Shelly1);
    iotWebConf.addParameterGroup(&Shelly2);
    iotWebConf.addParameterGroup(&Shelly3);
    iotWebConf.addParameterGroup(&Shelly4);
    iotWebConf.addParameterGroup(&Shelly5);
    iotWebConf.addParameterGroup(&Shelly6);
    iotWebConf.addParameterGroup(&Shelly7);
    iotWebConf.addParameterGroup(&Shelly8);
    iotWebConf.addParameterGroup(&Shelly9);
    iotWebConf.addParameterGroup(&Shelly10);

    // -- Define how to handle updateServer calls.
    iotWebConf.setupUpdateServer(
        [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
        [](const char* userName, char* password) { httpUpdater.updateCredentials(userName, password); });

    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);

    iotWebConf.getApTimeoutParameter()->visible = true;

    // -- Initializing the configuration.
    iotWebConf.init();

    convertParams();

    // -- Set up required URL handlers on the web server.
    server.on("/", handleRoot);
    server.on("/favicon.ico", [] { handleFavIcon(); });
    server.on("/config", [] { iotWebConf.handleConfig(); });
    server.on("/reboot", HTTP_GET, []() { handleReboot(); });
    server.on("/DateTime", HTTP_GET, []() { handleDateTime(); });
    server.on("/power", HTTP_GET, []() { handlePower(); });
    server.on("/data", HTTP_GET, []() { handleData(); });
    server.onNotFound([]() { iotWebConf.handleNotFound(); });

    Serial.println("Ready.");
}

void wifiLoop() {
    // -- doLoop should be called as frequently as possible.
    iotWebConf.doLoop();
    ArduinoOTA.handle();
}

void wifiConnected(){
    ArduinoOTA.begin();
}

void handleFavIcon() {
   server.send_P(200, "image/x-icon", favicon, sizeof(favicon));
}

void handleDateTime() {
    ESP_LOGD("handleDateTime", "Time requested");
	ESP32Time rtc;
    String _time = rtc.getTime("%H:%M:%S");
	server.send(200, "text/plain", _time);
}

void handlePower() {
    server.send(200, "text/plain", String(gInputPower));
}

void handleData() {
    String _json = "{";
    
    _json += "\"Power\":\"" + String(gInputPower) + "\"";
    _json += ",\"RSSI\":\"" + String(WiFi.RSSI()) + "\"";
    _json += ",\"Relays\":{";
    Relay* _relay = &Relay1;
    uint8_t _i = 1;
    while (_relay != nullptr) {
        if (_relay->IsEnabled()) {
			_json += "\"relay" + String(_i) + "\":\"On\"";
		}
		else {
			_json += "\"relay" + String(_i) + "\":\"Off\"";
		}
        
        _relay = (Relay*)_relay->getNext();
        if (_relay != nullptr) {
			_json += ",";
		}
		_i++;
    }
    _json += "}";

    _json += ",\"Shellys\":{";
    Shelly* _shelly = &Shelly1;
    _i = 1;
    while (_shelly != nullptr) {
        if (_shelly->isActive()) {
            if(_shelly->IsEnabled() && (gInputPower > _shelly->GetPower())) {
				_json += "\"shelly" + String(_i) + "\":\"On\"";
			}
			else if (_shelly->IsEnabled() && (gInputPower <= _shelly->GetPower())) {
				_json += "\"shelly" + String(_i) + "\":\"DelayedOff\"";
			}
			else {
				_json += "\"shelly" + String(_i) + "\":\"Off\"";
			}
        }
        _shelly = (Shelly*)_shelly->getNext();
        if ((_shelly != nullptr) && (_shelly->isActive())) {
            _json += ",";
        }
        _i++;
    }
    _json += "}";
    _json += "}";
    server.send(200, "text/plain", _json);
}

void handleReboot() {
    String _message;

    // redirect to the root page after 15 seconds
    _message += "<HEAD><meta http-equiv=\"refresh\" content=\"15;url=/\"></HEAD>\n<BODY><p>\n";
    _message += "Rebooting...<br>\n";
    _message += "Redirected after 15 seconds...\n";
    _message += "</p></BODY>\n";

    server.send(200, "text/html", _message);
    ESP.restart();
}

class MyHtmlRootFormatProvider : public HtmlRootFormatProvider {
public:
    String getHtmlTableRowClass(String name, String htmlclass, String id) {
        String _s = F("<tr><td align=\"left\">{n}</td><td align=\"left\"><span id=\"{id}\" class=\"{c}\"></span></td></tr>\n");
        _s.replace("{n}", name);
        _s.replace("{c}", htmlclass);
        _s.replace("{id}", id);
        return _s;
    }

protected:
    virtual String getStyleInner() {
        String _s = HtmlRootFormatProvider::getStyleInner();
        _s += F(".led {display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 5px; }\n");
        _s += F(".led.off {background-color: grey;}\n");
        _s += F(".led.on {background-color: green;}\n");
        _s += F(".led.delayedoff {background-color: orange;}\n");
        return _s;
    }

    virtual String getScriptInner() {
        String _s = HtmlRootFormatProvider::getScriptInner();

        _s.replace("{millisecond}", "5000");
        _s += F("requestDateTime();\n");
        _s += F("setInterval(requestDateTime, 1000);\n");

        _s += F("function requestDateTime() { \n");
        _s += F("   var xhttp = new XMLHttpRequest();\n");
        _s += F("   xhttp.onreadystatechange = function() {\n");
        _s += F("       if (this.readyState == 4 && this.status == 200) {\n");
        _s += F("           document.getElementById('DateTimeValue').innerHTML = this.responseText;\n");
        _s += F("       }\n");
        _s += F("   };\n");
        _s += F("   xhttp.open('GET', 'DateTime', true);\n");
        _s += F("   xhttp.send(); \n");
        _s += F("}\n");

        _s += F("function updateLED(element, status) {\n");
        _s += F("   if (element) {\n");
        _s += F("       element.classList.remove('on', 'off', 'delayedoff');\n");
        _s += F("       element.classList.add(status);\n");
        _s += F("   }\n");
        _s += F("}\n");

        _s += F("function updateData(jsonData) {\n");
        _s += F("   document.getElementById('PowerValue').innerHTML = jsonData.Power + \"W\" \n");
        _s += F("   document.getElementById('RSSIValue').innerHTML = jsonData.RSSI + \"dBm\" \n");

        _s += F("   updateLED(document.getElementById('relay1'), jsonData.Relays.relay1.toLowerCase());\n");
        _s += F("   updateLED(document.getElementById('relay2'), jsonData.Relays.relay2.toLowerCase());\n");
        _s += F("   updateLED(document.getElementById('relay3'), jsonData.Relays.relay3.toLowerCase());\n");
        _s += F("   updateLED(document.getElementById('relay4'), jsonData.Relays.relay4.toLowerCase());\n");

        Shelly* _shelly = &Shelly1;
        uint8_t _i = 1;
        while (_shelly != nullptr) {
            if (_shelly->isActive()) {
                _s += "   updateLED(document.getElementById('shelly" + String(_i) + "'), jsonData.Shellys.shelly" + String(_i) + ".toLowerCase());\n";
            }
            _shelly = (Shelly*)_shelly->getNext();
            _i++;
        }

        _s += F("}\n");

        return _s;
    }
};

void handleRoot() {
    String name;

    // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal()){
        // -- Captive portal request were already served.
        return;
    }

    MyHtmlRootFormatProvider rootFormatProvider;

    String _response = "";
    _response += rootFormatProvider.getHtmlHead(iotWebConf.getThingName());
    _response += rootFormatProvider.getHtmlStyle();
    _response += rootFormatProvider.getHtmlHeadEnd();
    _response += rootFormatProvider.getHtmlScript();

    _response += rootFormatProvider.getHtmlTable();
    _response += rootFormatProvider.getHtmlTableRow() + rootFormatProvider.getHtmlTableCol();

    _response += F("<fieldset align=left style=\"border: 1px solid\">\n");
    _response += F("<table border=\"0\" align=\"center\" width=\"100%\">\n");
    _response += F("<tr><td align=\"left\"><span id=\"DateTimeValue\">not valid</span></td></td><td align=\"right\"><span id=\"RSSIValue\">-100</span></td></tr>\n");
    _response += rootFormatProvider.getHtmlTableEnd();
    _response += rootFormatProvider.getHtmlFieldsetEnd();

    _response += rootFormatProvider.getHtmlFieldset("Power");
    _response += rootFormatProvider.getHtmlTable();
    _response += rootFormatProvider.getHtmlTableRowSpan("Input power:", "no data", "PowerValue");
    _response += rootFormatProvider.getHtmlTableRowText("Intervall:", String(gInverterInterval) + "s");
	_response += rootFormatProvider.getHtmlTableEnd();
    _response += rootFormatProvider.getHtmlFieldsetEnd();

    _response += rootFormatProvider.getHtmlFieldset("Relays");
    _response += rootFormatProvider.getHtmlTable();
    Relay* _relay = &Relay1;
    uint8_t _i = 1;
    while (_relay != nullptr) {
        _response += rootFormatProvider.getHtmlTableRowClass(String(_relay->DesignationValue) + ":", "led off", "relay" + String(_i));
		_relay = (Relay*)_relay->getNext();
		_i++;
	}
    _response += rootFormatProvider.getHtmlTableEnd();
    _response += rootFormatProvider.getHtmlFieldsetEnd();

    _response += rootFormatProvider.getHtmlFieldset("Shellys");
    _response += rootFormatProvider.getHtmlTable();
    Shelly* _shelly = &Shelly1;
    _i = 1;
    while (_shelly != nullptr) {
        if (_shelly->isActive()) {
            _response += rootFormatProvider.getHtmlTableRowClass(String(_shelly->DesignationValue) + ":", "led off", "shelly" + String(_i));
        }
        _shelly = (Shelly*)_shelly->getNext();
        _i++;
    }
    _response += rootFormatProvider.getHtmlTableEnd();
    _response += rootFormatProvider.getHtmlFieldsetEnd();

    _response += rootFormatProvider.getHtmlFieldset("Network");
    _response += rootFormatProvider.getHtmlTable();
    _response += rootFormatProvider.getHtmlTableRowText("MAC Address:", WiFi.macAddress());
    _response += rootFormatProvider.getHtmlTableRowText("IP Address:", WiFi.localIP().toString().c_str());
    _response += rootFormatProvider.getHtmlTableEnd();
    _response += rootFormatProvider.getHtmlFieldsetEnd();

    _response += rootFormatProvider.addNewLine(2);

    _response += rootFormatProvider.getHtmlTable();
    //_response += rootFormatProvider.getHtmlTableRowSpan("Time:", "not valid", "DateTimeValue");
    _response += rootFormatProvider.getHtmlTableRowText("Go to <a href = 'config'>configure page</a> to change configuration.");
    _response += rootFormatProvider.getHtmlTableRowText(rootFormatProvider.getHtmlVersion(Version));
    _response += rootFormatProvider.getHtmlTableEnd();

    _response += rootFormatProvider.getHtmlTableColEnd() + rootFormatProvider.getHtmlTableRowEnd();
    _response += rootFormatProvider.getHtmlTableEnd();
    _response += rootFormatProvider.getHtmlEnd();

    server.send(200, "text/html", _response);
}

void convertParams() {
    strcpy(gInverterIPAddress, InverterIPAddress.value());
    gInverterPort = InverterPort.value();
    gInverterInputPowerRegister = InverterInputPowerRegister.value();
    gInverterInputPowerDataLength = InverterInputPowerDataLength.value();
    gInverterInputPowerGain = InverterInputPowerGain.value();
    gInverterInterval = InverterActivePowerInterval.value();
}

void configSaved() {
    convertParams();
    gParamsChanged = true;
}