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
#define CONFIG_VERSION "B8"

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

iotwebconf::UIntTParameter<uint16_t> InverterActivePowerRegister =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("InverterActivePowerRegister").
    label("Register Active Power").
    defaultValue(37113).
    min(1u).
    step(1).
    placeholder("1..65535").
    build();

iotwebconf::UIntTParameter<uint8_t> InverterActivePowerDataLength =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint8_t>>("InverterActivePowerDataLength").
    label("Data Length").
    defaultValue(2).
    min(1u).
    step(1).
    placeholder("1..255").
    build();

iotwebconf::UIntTParameter<uint16_t> InverterActivePowerGain =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("InverterActivePowerGain").
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

IotWebConfParameterGroup Output1Group = IotWebConfParameterGroup("Output1", "Output 1");

iotwebconf::TextTParameter<20> Output1Name =
    iotwebconf::Builder<iotwebconf::TextTParameter<sizeof(gOutput1Name)>>("Output1Name").
    label("Name").
    defaultValue("Relay 1").
    build();

iotwebconf::UIntTParameter<uint8_t> Output1GPIO =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint8_t>>("Output1GPIO").
    label("GPIO").
    defaultValue(D1).
    min(1u).
    step(1).
    placeholder("1..255").
    build();

iotwebconf::UIntTParameter<uint16_t> Output1Power =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("Output1Power").
    label("Power (W)").
    defaultValue(0).
    min(0).
    step(1).
    placeholder("0..10000").
    build();

IotWebConfParameterGroup Output2Group = IotWebConfParameterGroup("Output2", "Output 2");

iotwebconf::TextTParameter<20> Output2Name =
    iotwebconf::Builder<iotwebconf::TextTParameter<sizeof(gOutput2Name)>>("Output2Name").
    label("Name").
    defaultValue("Relay 2").
    build();

iotwebconf::UIntTParameter<uint8_t> Output2GPIO =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint8_t>>("Output2GPIO").
    label("GPIO").
    defaultValue(D2).
    min(1u).
    step(1).
    placeholder("1..255").
    build();

iotwebconf::UIntTParameter<uint16_t> Output2Power =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("Output2Power").
    label("Power (W)").
    defaultValue(0).
    min(0).
    step(1).
    placeholder("1..10000").
    build();

IotWebConfParameterGroup Output3Group = IotWebConfParameterGroup("Output3", "Output 3");

iotwebconf::TextTParameter<20> Output3Name =
    iotwebconf::Builder<iotwebconf::TextTParameter<sizeof(gOutput3Name)>>("Output3Name").
    label("Name").
    defaultValue("Relay 3").
    build();

iotwebconf::UIntTParameter<uint8_t> Output3GPIO =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint8_t>>("Output3GPIO").
    label("GPIO").
    defaultValue(D3).
    min(1u).
    step(1).
    placeholder("1..255").
    build();

iotwebconf::UIntTParameter<uint16_t> Output3Power =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("Output3Power").
    label("Power (W)").
    defaultValue(0).
    min(0).
    step(1).
    placeholder("0..10000").
    build();

IotWebConfParameterGroup Output4Group = IotWebConfParameterGroup("Output4", "Output 4");

iotwebconf::TextTParameter<20> Output4Name =
    iotwebconf::Builder<iotwebconf::TextTParameter<sizeof(gOutput4Name)>>("Output4Name").
    label("Name").
    defaultValue("Relay 4").
    build();

iotwebconf::UIntTParameter<uint8_t> Output4GPIO =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint8_t>>("Output4GPIO").
    label("GPIO").
    defaultValue(D4).
    min(1u).
    step(1).
    placeholder("1..255").
    build();

iotwebconf::UIntTParameter<uint16_t> Output4Power =
    iotwebconf::Builder<iotwebconf::UIntTParameter<uint16_t>>("Output4Power").
    label("Power (W)").
    defaultValue(0).
    min(0).
    step(1).
    placeholder("1..10000").
    build();


httpGroup httpgroup0 = httpGroup(Shelly1);
httpGroup httpgroup1 = httpGroup(Shelly2);
httpGroup httpgroup2 = httpGroup(Shelly3);
httpGroup httpgroup3 = httpGroup(Shelly4);
httpGroup httpgroup4 = httpGroup(Shelly5);
httpGroup httpgroup5 = httpGroup(Shelly6);
httpGroup httpgroup6 = httpGroup(Shelly7);
httpGroup httpgroup7 = httpGroup(Shelly8);
httpGroup httpgroup8 = httpGroup(Shelly9);
httpGroup httpgroup9 = httpGroup(Shelly10);

void wifiInit() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("starting up...");


    iotWebConf.setStatusPin(STATUS_PIN, ON_LEVEL);
    iotWebConf.setConfigPin(CONFIG_PIN);

    sysConfGroup.addItem(&InverterIPAddress);
    sysConfGroup.addItem(&InverterPort);
    sysConfGroup.addItem(&InverterActivePowerRegister);
    sysConfGroup.addItem(&InverterActivePowerDataLength);
    sysConfGroup.addItem(&InverterActivePowerGain);
    sysConfGroup.addItem(&InverterActivePowerInterval);

    Output1Group.addItem(&Output1Name);
    Output1Group.addItem(&Output1GPIO);
    Output1Group.addItem(&Output1Power);

    Output2Group.addItem(&Output2Name);
    Output2Group.addItem(&Output2GPIO);
    Output2Group.addItem(&Output2Power);

    Output3Group.addItem(&Output3Name);
    Output3Group.addItem(&Output3GPIO);
    Output3Group.addItem(&Output3Power);

    Output4Group.addItem(&Output4Name);
    Output4Group.addItem(&Output4GPIO);
    Output4Group.addItem(&Output4Power);

    httpgroup0.setNext(&httpgroup1);
    httpgroup1.setNext(&httpgroup2);
    httpgroup2.setNext(&httpgroup3);
    httpgroup3.setNext(&httpgroup4);
    httpgroup4.setNext(&httpgroup5);
    httpgroup5.setNext(&httpgroup6);
    httpgroup6.setNext(&httpgroup7);
    httpgroup7.setNext(&httpgroup8);
    httpgroup8.setNext(&httpgroup9);

    iotWebConf.addParameterGroup(&sysConfGroup);
    iotWebConf.addParameterGroup(&Output1Group);
    iotWebConf.addParameterGroup(&Output2Group);
    iotWebConf.addParameterGroup(&Output3Group);
    iotWebConf.addParameterGroup(&Output4Group);

    iotWebConf.addParameterGroup(&httpgroup0);
    iotWebConf.addParameterGroup(&httpgroup1);
    iotWebConf.addParameterGroup(&httpgroup2);
    iotWebConf.addParameterGroup(&httpgroup3);
    iotWebConf.addParameterGroup(&httpgroup4);
    iotWebConf.addParameterGroup(&httpgroup5);
    iotWebConf.addParameterGroup(&httpgroup6);
    iotWebConf.addParameterGroup(&httpgroup7);
    iotWebConf.addParameterGroup(&httpgroup8);
    iotWebConf.addParameterGroup(&httpgroup9);

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

    page += "</style>";

    page += "<meta http-equiv=refresh content=30 />";
    page += HTML_Start_Body;
    page += "<table border=0 align=center>";
    page += "<tr><td>";

    page += HTML_Start_Fieldset;
    page += HTML_Fieldset_Legend;
    page.replace("{l}", "Power");
        page += HTML_Start_Table;

        page += "<tr><td align=left>Active:</td><td>" + String(gActivePower) + "W" + "</td></tr>";

        page += HTML_End_Table;
    page += HTML_End_Fieldset;

    page += HTML_Start_Fieldset;
    page += HTML_Fieldset_Legend;
    page.replace("{l}", "Status");
    page += HTML_Start_Table;

    name = gOutput1Name;
    if (gRelay1) {
        page += "<tr><td align=left>" + name + ":</td><td><span class = \"dot-green\"></span></td></tr>";
    }
    else {
        page += "<tr><td align=left>" + name + ":</td><td><span class=\"dot-grey\"></span></td></tr>";
    }
        
    name = gOutput2Name;
    if (gRelay2) {
        page += "<tr><td align=left>" + name + ":</td><td><span class = \"dot-green\"></span></td></tr>";
    }
    else {
        page += "<tr><td align=left>" + name + ":</td><td><span class=\"dot-grey\"></span></td></tr>";
    }

    name = gOutput3Name;
    if (gRelay3) {
        page += "<tr><td align=left>" + name + ":</td><td><span class = \"dot-green\"></span></td></tr>";
    }
    else {
        page += "<tr><td align=left>" + name + ":</td><td><span class=\"dot-grey\"></span></td></tr>";
    }


    name = gOutput4Name;
    if (gRelay4) {
        page += "<tr><td align=left>" + name + ":</td><td><span class = \"dot-green\"></span></td></tr>";
    }
    else {
        page += "<tr><td align=left>" + name + ":</td><td><span class=\"dot-grey\"></span></td></tr>";
    }

    page += HTML_End_Table;
    page += HTML_End_Fieldset;

    page += HTML_Start_Fieldset;
    page += HTML_Fieldset_Legend;
    page.replace("{l}", "Shellys");
    page += HTML_Start_Table;

    for (uint8_t i = 0; i < ShellyMax; i++){
        if (Shellys[i].Power > 0) {
            if (Shellys[i].Enabled) {
                page += "<tr><td align=left>" + String(Shellys[i].Name) + ":</td><td><span class = \"dot-green\"></span></td></tr>";
            }
            else {
                page += "<tr><td align=left>" + String(Shellys[i].Name) + ":</td><td><span class=\"dot-grey\"></span></td></tr>";
            }
        }
    }


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

    strcpy(gOutput1Name, Output1Name.value());
    gOutput1Power = Output1Power.value();
    gOutput1GPIO = Output1GPIO.value();

    strcpy(gOutput2Name, Output2Name.value());
    gOutput2Power = Output2Power.value();
    gOutput2GPIO = Output2GPIO.value();

    strcpy(gOutput3Name, Output3Name.value());
    gOutput3Power = Output3Power.value();
    gOutput3GPIO = Output3GPIO.value();

    strcpy(gOutput4Name, Output4Name.value());
    gOutput4Power = Output4Power.value();
    gOutput4GPIO = Output4GPIO.value();

    strcpy(gInverterIPAddress, InverterIPAddress.value());
    gInverterPort = InverterPort.value();
    gInverterActivePowerRegister = InverterActivePowerRegister.value();
    gInverterActivePowerDataLength = InverterActivePowerDataLength.value();
    gInverterActivePowerGain = InverterActivePowerGain.value();
    gInverterActivePowerInterval = InverterActivePowerInterval.value();

}

void configSaved() {
    convertParams();
    gParamsChanged = true;
}