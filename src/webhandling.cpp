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
//needed for library

#include <DNSServer.h>
#include<iostream>
#include <string.h>

#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#include <IotWebConfTParameter.h>
#include "common.h"
#include "webhandling.h"

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "A3"

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
void convertParams();

// -- Callback methods.
void configSaved();
void wifiConnected();

bool gParamsChanged = true;

DNSServer dnsServer;
WebServer server(80);


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


Relay Relay1 = Relay("Relay1", GPIO_NUM_1);
Relay Relay2 = Relay("Relay2", GPIO_NUM_2);
Relay Relay3 = Relay("Relay3", GPIO_NUM_3);
Relay Relay4 = Relay("Relay4", GPIO_NUM_4);


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


iotwebconf::OptionalGroupHtmlFormatProvider optionalGroupHtmlFormatProvider;

void wifiInit() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("starting up...");


    iotWebConf.setStatusPin(STATUS_PIN, ON_LEVEL);
    iotWebConf.setConfigPin(CONFIG_PIN);

    iotWebConf.setHtmlFormatProvider(&optionalGroupHtmlFormatProvider);

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

    iotWebConf.addParameterGroup(&Relay1);
    iotWebConf.addParameterGroup(&Relay2);
    iotWebConf.addParameterGroup(&Relay3);
    iotWebConf.addParameterGroup(&Relay4);

    Relay1.setActive(true);

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

    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);

    iotWebConf.getApTimeoutParameter()->visible = true;

    // -- Initializing the configuration.
    iotWebConf.init();

    convertParams();

    // -- Set up required URL handlers on the web server.
    server.on("/", handleRoot);
    server.on("/config", [] { iotWebConf.handleConfig(); });
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

void handleRoot() {
    String name;

    // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal())
    {
        // -- Captive portal request were already served.
        return;
    }

    String page = HTML_Start_Doc;
    page.replace("{v}", "Powerbox");
    page += "<style>";
    page += ".de{background-color:#ffaaaa;} .em{font-size:0.8em;color:#bb0000;padding-bottom:0px;} .c{text-align: center;} div,input,select{padding:5px;font-size:1em;} input{width:95%;} select{width:100%} input[type=checkbox]{width:auto;scale:1.5;margin:10px;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} fieldset{border-radius:0.3rem;margin: 0px;}";
    // page.replace("center", "left");
    page += ".dot-grey{height: 12px; width: 12px; background-color: #bbb; border-radius: 50%; display: inline-block; }";
    page += ".dot-green{height: 12px; width: 12px; background-color: green; border-radius: 50%; display: inline-block; }";
    page += ".blink-green{2s blink-green ease infinite; height: 12px; width: 12px; background-color: orange; border-radius: 50%; display: inline-block; }";

    page += "</style>";

    page += "<meta http-equiv=refresh content=30 />";
    page += HTML_Start_Body;
    page += "<table border=0 align=center>";
    page += "<tr><td>";

    page += HTML_Start_Fieldset;
    page += HTML_Fieldset_Legend;
    page.replace("{l}", "Power");
        page += HTML_Start_Table;
        page += "<tr><td align=left>Input power:</td><td>" + String(gInputPower) + "W" + "</td></tr>";
        page += "<tr><td align=left>Intervall  :</td><td>" + String(gInverterInterval) + "s" + "</td></tr>";

        page += HTML_End_Table;
    page += HTML_End_Fieldset;

    page += HTML_Start_Fieldset;
    page += HTML_Fieldset_Legend;
    page.replace("{l}", "Relays");
    page += HTML_Start_Table;

    Relay* _relay = &Relay1;
    while (_relay != nullptr) {
        if (_relay->isActive()) {
            if (_relay->IsEnabled()) {
                page += "<tr><td align=left>" + String(_relay->DesignationValue) + ":</td><td><span class = \"dot-green\"></span></td></tr>";
            }
            else {
                page += "<tr><td align=left>" + String(_relay->DesignationValue) + ":</td><td><span class=\"dot-grey\"></span></td></tr>";
            }
        }
        _relay = (Relay*)_relay->getNext();
    }

    page += HTML_End_Table;
    page += HTML_End_Fieldset;

    page += HTML_Start_Fieldset;
    page += HTML_Fieldset_Legend;
    page.replace("{l}", "Shellys");
    page += HTML_Start_Table;

    Shelly* _shelly = &Shelly1;
    bool _enabled = false;
    uint8_t i = 1;
    while (_shelly != nullptr) {
        if (_shelly->isActive()) {
            if (_shelly->IsEnabled() && (gInputPower > _shelly->GetPower())) {
                page += "<tr><td align=left>" + String(_shelly->DesignationValue) + ":</td><td><span class = \"dot-green\"></span></td></tr>";
            }
            else  if (_shelly->IsEnabled() && (gInputPower <= _shelly->GetPower())) {
                page += "<tr><td align=left>" + String(_shelly->DesignationValue) + ":</td><td><span class = \"blink-green\"></span></td></tr>";
            }
            else {
                page += "<tr><td align=left>" + String(_shelly->DesignationValue) + ":</td><td><span class=\"dot-grey\"></span></td></tr>";
            }

        }
        _shelly = (Shelly*)_shelly->getNext();
        i++;
    }

    page += HTML_End_Table;
    page += HTML_End_Fieldset;

    page += HTML_Start_Fieldset;
    page += HTML_Fieldset_Legend;
    page.replace("{l}", "Network");
    page += HTML_Start_Table;

    page += "<tr><td align=left>MAC Address:</td><td>" + String(WiFi.macAddress()) + "</td></tr>";
    page += "<tr><td align=left>IP Address:</td><td>" + String(WiFi.localIP().toString().c_str()) + "</td></tr>";

    page += HTML_End_Table;
    page += HTML_End_Fieldset;

    page += "<br>";
    page += "<br>";

    page += HTML_Start_Table;
    page += "<tr><td align=left>Go to <a href = 'config'>configure page</a> to change configuration.</td></tr>";
    // page += "<tr><td align=left>Go to <a href='setruntime'>runtime modification page</a> to change runtime data.</td></tr>";
    page += HTML_End_Table;
    page += HTML_End_Body;

    page += HTML_End_Doc;


    server.send(200, "text/html", page);
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