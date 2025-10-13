// 
// 
// 

#define DEBUG_WIFI(m) SERIAL_DBG.print(m)

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <time.h>
#include <DNSServer.h>

#include <IotWebConf.h>
#include <IotWebRoot.h>
#include <IotWebConfUsing.h>
#include <IotWebConfTParameter.h>
#include <IotWebConfESP32HTTPUpdateServer.h>

#include "common.h"
#include "webhandling.h"
#include "favicon.h"
#include "inverterhandling.h"

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "A5"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN  -1

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN
#define ON_LEVEL HIGH

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
void connectWifi(const char* ssid, const char* password);
iotwebconf::WifiAuthInfo* handleConnectWifiFailure();

// -- Callback methods.
void configSaved();
void wifiConnected();

bool gParamsChanged = true;
bool startAPMode = true;

DNSServer dnsServer;
WebServer server(80);

class CustomHtmlFormatProvider : public iotwebconf::OptionalGroupHtmlFormatProvider {
protected:
    virtual String getFormEnd() {
        String s_ = OptionalGroupHtmlFormatProvider::getFormEnd();
        s_ += F("</br><form action='/reboot' method='get'><button type='submit'>Reboot</button></form>");
        s_ += F("</br>Return to <a href='/'>home page</a>.");
		return s_;
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
    iotWebConf.setWifiConnectionHandler(&connectWifi);
    iotWebConf.setWifiConnectionFailedHandler(&handleConnectWifiFailure);

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

    // Get the current time
    time_t now_ = time(nullptr);
    struct tm* timeinfo_ = localtime(&now_);
    char timeStr_[20];
    char dateStr_[20];
    strftime(timeStr_, sizeof(timeStr_), "%H:%M:%S", timeinfo_);
    strftime(dateStr_, sizeof(dateStr_), "%Y-%m-%d", timeinfo_);

	server.send(200, "text/plain", String(timeStr_));
}

void handlePower() {
    server.send(200, "text/plain", String(inverterPowerData.inputPower * 1000, 0));
}

void handleData() {
    String json_ = "{";
    json_ += "\"RSSI\":\"" + String(WiFi.RSSI()) + "\"";

    // Grid node
    json_ += ",\"Grid\":{";
    json_ += "\"VoltageA\":\"" + String(meterData.gridVoltageA, 1) + "\"";
    json_ += ",\"VoltageB\":\"" + String(meterData.gridVoltageB, 1) + "\"";
    json_ += ",\"VoltageC\":\"" + String(meterData.gridVoltageC, 1) + "\"";
    json_ += ",\"CurrentA\":\"" + String(meterData.gridCurrentA, 3) + "\"";
    json_ += ",\"CurrentB\":\"" + String(meterData.gridCurrentB, 3) + "\"";
    json_ += ",\"CurrentC\":\"" + String(meterData.gridCurrentC, 3) + "\"";
    json_ += ",\"Frequency\":\"" + String(meterData.gridFrequency, 2) + "\"";
    json_ += ",\"PowerFactor\":\"" + String(meterData.powerFactor, 3) + "\"";
	json_ += ",\"ActivePower\":\"" + String(meterData.activePower, 0) + "\"";
	json_ += ",\"ReactivePower\":\"" + String(meterData.reactivePower, 0) + "\"";
    json_ += "}";

    // Inverter node
    json_ += ",\"Inverter\":{";
    json_ += "\"InputPower\":\"" + String(inverterPowerData.inputPower * 1000, 0) + "\"";
    json_ += ",\"ActivePower\":\"" + String(inverterPowerData.activePower * 1000, 0) + "\"";
    json_ += ",\"ReactivePower\":\"" + String(inverterPowerData.reactivePower * 1000, 0) + "\"";
    json_ += ",\"GridVoltageAB\":\"" + String(inverterPowerData.gridVoltageAB, 1) + "\"";
    json_ += ",\"GridVoltageBC\":\"" + String(inverterPowerData.gridVoltageBC, 1) + "\"";
    json_ += ",\"GridVoltageCA\":\"" + String(inverterPowerData.gridVoltageCA, 1) + "\"";
    json_ += ",\"GridCurrentA\":\"" + String(inverterPowerData.gridCurrentA, 3) + "\"";
    json_ += ",\"GridCurrentB\":\"" + String(inverterPowerData.gridCurrentB, 3) + "\"";
    json_ += ",\"GridCurrentC\":\"" + String(inverterPowerData.gridCurrentC, 3) + "\"";
    json_ += ",\"PowerFactor\":\"" + String(inverterPowerData.powerFactor, 3) + "\"";
    json_ += ",\"Standby\":\"" + String(inverterStatusData.isStandby() ? "Yes" : "No") + "\"";
    json_ += "}";

	// Relay node
    json_ += ",\"Relays\":{";
    Relay* relay_ = &Relay1;
    uint8_t i_ = 1;
    while (relay_ != nullptr) {
        if (relay_->isEnabled()) {
			json_ += "\"relay" + String(i_) + "\":\"On\"";
		}
		else {
			json_ += "\"relay" + String(i_) + "\":\"Off\"";
		}
        
        relay_ = (Relay*)relay_->getNext();
        if (relay_ != nullptr) {
			json_ += ",";
		}
		i_++;
    }
    json_ += "}";

	// Shelly node
    json_ += ",\"Shellys\":{";
    Shelly* shelly_ = &Shelly1;
    i_ = 1;
    while (shelly_ != nullptr) {
        if (shelly_->isActive()) {
            if(shelly_->isEnabled() && (inverterPowerData.inputPower * 1000 > shelly_->getPower())) {
				json_ += "\"shelly" + String(i_) + "\":\"On\"";
			}
			else if (shelly_->isEnabled() && (inverterPowerData.inputPower * 1000 <= shelly_->getPower())) {
				json_ += "\"shelly" + String(i_) + "\":\"DelayedOff\"";
			}
			else {
				json_ += "\"shelly" + String(i_) + "\":\"Off\"";
			}
        }
        shelly_ = (Shelly*)shelly_->getNext();
        if ((shelly_ != nullptr) && (shelly_->isActive())) {
            json_ += ",";
        }
        i_++;
    }
    json_ += "}";
    json_ += "}";
    server.send(200, "text/plain", json_);
}

void handleReboot() {
    String message_;

    // redirect to the root page after 15 seconds
    message_ += "<HEAD><meta http-equiv=\"refresh\" content=\"15;url=/\"></HEAD>\n<BODY><p>\n";
    message_ += "Rebooting...<br>\n";
    message_ += "Redirected after 15 seconds...\n";
    message_ += "</p></BODY>\n";

    server.send(200, "text/html", message_);
    ESP.restart();
}

class MyHtmlRootFormatProvider : public HtmlRootFormatProvider {
public:
    String getHtmlTableRowClass(String name, String htmlclass, String id) {
        String s_ = F("<tr><td align=\"left\">{n}</td><td align=\"left\"><span id=\"{id}\" class=\"{c}\"></span></td></tr>\n");
        s_.replace("{n}", name);
        s_.replace("{c}", htmlclass);
        s_.replace("{id}", id);
        return s_;
    }

protected:
    virtual String getStyleInner() {
        String s_ = HtmlRootFormatProvider::getStyleInner();
        s_ += F(".led {display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 5px; }\n");
        s_ += F(".led.off {background-color: grey;}\n");
        s_ += F(".led.on {background-color: green;}\n");
        s_ += F(".led.delayedoff {background-color: orange;}\n");
        return s_;
    }

    virtual String getScriptInner() {
        String s_ = HtmlRootFormatProvider::getScriptInner();

        s_.replace("{millisecond}", "5000");
        s_ += F("requestDateTime();\n");
        s_ += F("setInterval(requestDateTime, 1000);\n");

        s_ += F("function requestDateTime() { \n");
        s_ += F("   var xhttp = new XMLHttpRequest();\n");
        s_ += F("   xhttp.onreadystatechange = function() {\n");
        s_ += F("       if (this.readyState == 4 && this.status == 200) {\n");
        s_ += F("           document.getElementById('DateTimeValue').innerHTML = this.responseText;\n");
        s_ += F("       }\n");
        s_ += F("   };\n");
        s_ += F("   xhttp.open('GET', 'DateTime', true);\n");
        s_ += F("   xhttp.send(); \n");
        s_ += F("}\n");

        s_ += F("function updateLED(element, status) {\n");
        s_ += F("   if (element) {\n");
        s_ += F("       element.classList.remove('on', 'off', 'delayedoff');\n");
        s_ += F("       element.classList.add(status);\n");
        s_ += F("   }\n");
        s_ += F("}\n");

        s_ += F("function updateData(jsonData) {\n");
        s_ += F("   document.getElementById('RSSIValue').innerHTML = jsonData.RSSI + \"dBm\" \n");

		s_ += F("   document.getElementById('gridVoltageA').innerHTML = jsonData.Grid.VoltageA + \"V\" \n");
		s_ += F("   document.getElementById('gridVoltageB').innerHTML = jsonData.Grid.VoltageB + \"V\" \n");
		s_ += F("   document.getElementById('gridVoltageC').innerHTML = jsonData.Grid.VoltageC + \"V\" \n");
		s_ += F("   document.getElementById('gridCurrentA').innerHTML = jsonData.Grid.CurrentA + \"A\" \n");
		s_ += F("   document.getElementById('gridCurrentB').innerHTML = jsonData.Grid.CurrentB + \"A\" \n");
		s_ += F("   document.getElementById('gridCurrentC').innerHTML = jsonData.Grid.CurrentC + \"A\" \n");
		s_ += F("   document.getElementById('gridActivePower').innerHTML = jsonData.Grid.ActivePower + \"W\" \n");

		s_ += F("   document.getElementById('inverterStandby').innerHTML = jsonData.Inverter.Standby \n");
		s_ += F("   document.getElementById('inverterInputPower').innerHTML = jsonData.Inverter.InputPower + \"W\" \n");
		s_ += F("   document.getElementById('inverterActivePower').innerHTML = jsonData.Inverter.ActivePower + \"W\" \n");

        s_ += F("   updateLED(document.getElementById('relay1'), jsonData.Relays.relay1.toLowerCase());\n");
        s_ += F("   updateLED(document.getElementById('relay2'), jsonData.Relays.relay2.toLowerCase());\n");
        s_ += F("   updateLED(document.getElementById('relay3'), jsonData.Relays.relay3.toLowerCase());\n");
        s_ += F("   updateLED(document.getElementById('relay4'), jsonData.Relays.relay4.toLowerCase());\n");

        Shelly* shelly_ = &Shelly1;
        uint8_t i_ = 1;
        while (shelly_ != nullptr) {
            if (shelly_->isActive()) {
                s_ += "   updateLED(document.getElementById('shelly" + String(i_) + "'), jsonData.Shellys.shelly" + String(i_) + ".toLowerCase());\n";
            }
            shelly_ = (Shelly*)shelly_->getNext();
            i_++;
        }

        s_ += F("}\n");

        return s_;
    }
};

void handleRoot() {
    // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal()){
        // -- Captive portal request were already served.
        return;
    }

    MyHtmlRootFormatProvider fp_;

    String response_ = "";
    response_ += fp_.getHtmlHead(iotWebConf.getThingName());
    response_ += fp_.getHtmlStyle();
    response_ += fp_.getHtmlHeadEnd();
    response_ += fp_.getHtmlScript();

    response_ += fp_.getHtmlTable();
    response_ += fp_.getHtmlTableRow() + fp_.getHtmlTableCol();

    response_ += F("<fieldset align=left style=\"border: 1px solid\">\n");
    response_ += F("<table border=\"0\" align=\"center\" width=\"100%\">\n");
    response_ += F("<tr><td align=\"left\"><span id=\"DateTimeValue\">not valid</span></td></td><td align=\"right\"><span id=\"RSSIValue\">-100</span></td></tr>\n");
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.getHtmlFieldset("Grid");
    response_ += fp_.getHtmlTable();
    response_ += fp_.getHtmlTableRowSpan("Voltage A:", "no data", "gridVoltageA");
    response_ += fp_.getHtmlTableRowSpan("Voltage B:", "no data", "gridVoltageB");
    response_ += fp_.getHtmlTableRowSpan("Voltage C:", "no data", "gridVoltageC");
    response_ += fp_.getHtmlTableRowSpan("Current A:", "no data", "gridCurrentA");
    response_ += fp_.getHtmlTableRowSpan("Current B:", "no data", "gridCurrentB");
    response_ += fp_.getHtmlTableRowSpan("Current C:", "no data", "gridCurrentC");
    response_ += fp_.getHtmlTableRowSpan("Active Power:", "no data", "gridActivePower");
    response_ += fp_.getHtmlTableRowText(">0: feed-in to the power grid.", "");
	response_ += fp_.getHtmlTableRowText("<0: consumption from the power grid.", "");

    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.getHtmlFieldset("Inverter");
    response_ += fp_.getHtmlTable();
    response_ += fp_.getHtmlTableRowText("Polling intervall:", String(gInverterInterval) + "s");
    response_ += fp_.getHtmlTableRowSpan("Standby:", "no data", "inverterStandby");
    response_ += fp_.getHtmlTableRowSpan("Input Power:", "no data", "inverterInputPower");
    response_ += fp_.getHtmlTableRowSpan("Active Power:", "no data", "inverterActivePower");
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.getHtmlFieldset("Relays");
    response_ += fp_.getHtmlTable();
    Relay* relay_ = &Relay1;
    uint8_t i_ = 1;
    while (relay_ != nullptr) {
        response_ += fp_.getHtmlTableRowClass(relay_->getName() + ":", "led off", "relay" + String(i_));
		relay_ = (Relay*)relay_->getNext();
		i_++;
	}
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.getHtmlFieldset("Shellys");
    response_ += fp_.getHtmlTable();
    Shelly* shelly_ = &Shelly1;
    i_ = 1;
    while (shelly_ != nullptr) {
        if (shelly_->isActive()) {
            response_ += fp_.getHtmlTableRowClass(shelly_->getName() + ":", "led off", "shelly" + String(i_));
        }
        shelly_ = (Shelly*)shelly_->getNext();
        i_++;
    }
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.getHtmlFieldset("Network");
    response_ += fp_.getHtmlTable();
    response_ += fp_.getHtmlTableRowText("MAC Address:", WiFi.macAddress());
    response_ += fp_.getHtmlTableRowText("IP Address:", WiFi.localIP().toString().c_str());
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.addNewLine(2);

    response_ += fp_.getHtmlTable();
    //response_ += fp_.getHtmlTableRowSpan("Time:", "not valid", "DateTimeValue");
    response_ += fp_.getHtmlTableRowText("Go to <a href = 'config'>configure page</a> to change configuration.");
    response_ += fp_.getHtmlTableRowText(fp_.getHtmlVersion(Version));
    response_ += fp_.getHtmlTableEnd();

    response_ += fp_.getHtmlTableColEnd() + fp_.getHtmlTableRowEnd();
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlEnd();

    server.send(200, "text/html", response_);
}

void connectWifi(const char* ssid, const char* password) {
    Serial.println("Connecting to WiFi ...");
    WiFi.begin(ssid, password);
}

iotwebconf::WifiAuthInfo* handleConnectWifiFailure() {
    static iotwebconf::WifiAuthInfo auth_;
    auth_ = iotWebConf.getWifiAuthInfo();
    return &auth_;
}

void convertParams() {
    strcpy(gInverterIPAddress, InverterIPAddress.value());
    gInverterPort = InverterPort.value();
    gInverterInterval = InverterActivePowerInterval.value();
}

void configSaved() {
    convertParams();
    gParamsChanged = true;
}