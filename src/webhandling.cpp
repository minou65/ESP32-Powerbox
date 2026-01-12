#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "common.h"
#include "webhandling.h"
#include "favicon.h"
#include "inverterhandling.h"

#include <DNSServer.h>

#include <IotWebConfAsyncUpdateServer.h>
#include <IotWebRoot.h>
#include <IotWebConfUsing.h>
#include <IotWebConfTParameter.h>

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
void onProgress(size_t prg, size_t sz);
void handleRoot(AsyncWebServerRequest* request);
void handleDateTime(AsyncWebServerRequest* request);
void handlePower(AsyncWebServerRequest* request);
void handleData(AsyncWebServerRequest* request);
void handlePost(AsyncWebServerRequest* requestrequest);
void handleAPPasswordMissingPage(iotwebconf::WebRequestWrapper* webRequestWrapper);
void handleSSIDNotConfiguredPage(iotwebconf::WebRequestWrapper* webRequestWrapper);
void handleConfigSavedPage(iotwebconf::WebRequestWrapper* webRequestWrapper);
void convertParams();
void connectWifi(const char* ssid, const char* password);
iotwebconf::WifiAuthInfo* handleConnectWifiFailure();

// -- Callback methods.
void configSaved();
void wifiConnected();

bool gParamsChanged = true;
bool startAPMode = true;
bool ShouldReboot = false;  

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebServerWrapper asyncWebServerWrapper(&server);
AsyncUpdateServer AsyncUpdater;

const char html_form_end[] PROGMEM = R"=====(
</br><form action='/reboot' method='get'><button type='submit'>Reboot</button></form>
</br><a href='/'>Home</a>
</br><a href='#' onclick="postReset()">Reset to factory defaults</a>
<script>
function postReset() {
    fetch('/post', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'reset=true'
    })
    .then(response => {
        if (response.ok) { window.location.href = '/'; }
    })
    .catch(error => {
        console.error('Reset fehlgeschlagen:', error);
    });
}
</script>
)=====";

const char html_end_template[] PROGMEM = R"=====(
</div></body></html>
    <script>
        function updateDeviceFields(deviceNum) {
            var typeSelect = document.getElementById('Device' + deviceNum + '-typeselect');
            var type = typeSelect.value;

            var fields = [
                'designation',
                'power',
                'delay',
                'ontime',
                'offtime',
                'relayselect',
                'urlon',
                'urloff',
                'partialloadfactor',
                'partialloadallowed'
            ];

            // Hide all fields first
            fields.forEach(function (field) {
                var el = document.getElementById('Device' + deviceNum + '-' + field);
                if (el) el.parentElement.style.display = 'none';
            });

            if (type !== '-') {
                // Show common fields
                ['designation', 'power', 'delay', 'ontime', 'offtime'].forEach(function (field) {
                    var el = document.getElementById('Device' + deviceNum + '-' + field);
                    if (el) el.parentElement.style.display = '';
                });

                // Show specific fields
                if (type === 'Relay') {
                    var el = document.getElementById('Device' + deviceNum + '-relayselect');
                    if (el) el.parentElement.style.display = '';
                } else if (type === 'URL') {
                    ['urlon', 'urloff'].forEach(function (field) {
                        var el = document.getElementById('Device' + deviceNum + '-' + field);
                        if (el) el.parentElement.style.display = '';
                    });
                }

                // Show partial load checkbox always
                var partialLoadAllowedEl = document.getElementById('Device' + deviceNum + '-partialloadallowed');
                if (partialLoadAllowedEl) partialLoadAllowedEl.parentElement.style.display = '';

                // Show/hide partial load factor depending on checkbox
                var partialLoadFactorEl = document.getElementById('Device' + deviceNum + '-partialloadfactor');
                if (partialLoadAllowedEl && partialLoadFactorEl) {
                    if (partialLoadAllowedEl.checked) {
                        partialLoadFactorEl.parentElement.style.display = '';
                    } else {
                        partialLoadFactorEl.parentElement.style.display = 'none';
                    }
                }
            }
        }

        // Initial setup and event binding for all devices
        for (var i = 1; i <= 10; i++) {
            (function (i) {
                var select = document.getElementById('Device' + i + '-typeselect');
                var partialLoadAllowedEl = document.getElementById('Device' + i + '-partialloadallowed');
                if (select) {
                    select.addEventListener('change', function () {
                        updateDeviceFields(i);
                    });
                }
                if (partialLoadAllowedEl) {
                    partialLoadAllowedEl.addEventListener('change', function () {
                        updateDeviceFields(i);
                    });
                }
                // Initial call
                updateDeviceFields(i);
            })(i);
        }
    </script>
)=====";

class CustomHtmlFormatProvider : public iotwebconf::OptionalGroupHtmlFormatProvider {
protected:
    String getFormEnd() override {
        return OptionalGroupHtmlFormatProvider::getFormEnd() + String(FPSTR(html_form_end));
	}
    String getEnd() override {
        return String(FPSTR(html_end_template)) + HtmlFormatProvider::getEnd();
                
    }
};
CustomHtmlFormatProvider customHtmlFormatProvider;

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
    String getStyleInner() override {
        String s_ = HtmlRootFormatProvider::getStyleInner();
        s_ += F(".led {display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 5px; }\n");
        s_ += F(".led.off {background-color: grey;}\n");
        s_ += F(".led.on {background-color: green;}\n");
        s_ += F(".led.delayedoff {background-color: orange;}\n");
        return s_;
    }

    String getScriptInner() override {
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
        s_ += F("   document.getElementById('inverterActivePower').innerHTML = jsonData.Inverter.ActivePower + \"W\" \n");

        // Set grid power direction
        s_ += F("   if (parseFloat(jsonData.Grid.ActivePower) > 0) {\n");
        s_ += F("       document.getElementById('gridPowerDirection').innerHTML = \"to Grid\";\n");
        s_ += F("   } else {\n");
        s_ += F("       document.getElementById('gridPowerDirection').innerHTML = \"from Grid\";\n");
        s_ += F("   }\n");

        for (size_t i_ = 0; i_ < devices.size(); ++i_) {
            if (devices[i_]->isActive()) {
                s_ += "   updateLED(document.getElementById('device" + String(i_ + 1) + "'), jsonData.SwitchDevices.device" + String(i_ + 1) + ".toLowerCase());\n";
            }
        }

        s_ += F("}\n");

        return s_;
    }
};

AsyncIotWebConf iotWebConf(thingName, &dnsServer, &asyncWebServerWrapper, wifiInitialApPassword, CONFIG_VERSION);

InverterConfig inverterConfig = InverterConfig();
NtpConfig ntpConfig = NtpConfig();

std::vector<SwitchDevice*> devices;

void wifiInit() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("starting up...");


    iotWebConf.setStatusPin(STATUS_PIN, ON_LEVEL);
    iotWebConf.setConfigPin(CONFIG_PIN);

    iotWebConf.setHtmlFormatProvider(&customHtmlFormatProvider);

	// -- Create SwitchDevice instances and chain them.
    SwitchDevice* prev_ = nullptr;
    for (int i_ = 1; i_ <= 10; ++i_) {
        auto* dev_ = new SwitchDevice(("Device" + String(i_)).c_str());
        devices.push_back(dev_);
        if (prev_) prev_->setNext(dev_);
        prev_ = dev_;
    }

    iotWebConf.addParameterGroup(&inverterConfig);
	iotWebConf.addParameterGroup(&ntpConfig);

    for (auto device_ : devices) {
        iotWebConf.addParameterGroup(device_);
    }

    // -- Define how to handle updateServer calls.
    iotWebConf.setupUpdateServer(
        [](const char* updatePath) { AsyncUpdater.setup(&server, updatePath, onProgress); },
        [](const char* userName, char* password) { AsyncUpdater.updateCredentials(userName, password); });

    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);
    iotWebConf.setWifiConnectionHandler(&connectWifi);
    iotWebConf.setWifiConnectionFailedHandler(&handleConnectWifiFailure);
    iotWebConf.setConfigSavedPage(&handleConfigSavedPage);
    iotWebConf.setConfigAPPasswordMissingPage(&handleAPPasswordMissingPage);
    iotWebConf.setConfigSSIDNotConfiguredPage(&handleSSIDNotConfiguredPage);

    iotWebConf.getApTimeoutParameter()->visible = true;

    // -- Initializing the configuration.
    iotWebConf.init();

    convertParams();

    // -- Set up required URL handlers on the web server.
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { handleRoot(request); });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response_ = request->beginResponse_P(200, "image/x-icon", favicon_ico, sizeof(favicon_ico));
        request->send(response_);
        }
    );

    server.on("/config", HTTP_ANY, [](AsyncWebServerRequest* request) {
        auto* asyncWebRequestWrapper_ = new AsyncWebRequestWrapper(request);
        iotWebConf.handleConfig(asyncWebRequestWrapper_);
        }
    );

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response_ = request->beginResponse(200, "text/html",
            "<html>"
            "<head>"
            "<meta http-equiv=\"refresh\" content=\"15; url=/\">"
            "<title>Rebooting...</title>"
            "</head>"
            "<body>"
            "Please wait while the device is rebooting...<br>"
            "You will be redirected to the homepage shortly."
            "</body>"
            "</html>");
        request->client()->setNoDelay(true); // Disable Nagle's algorithm so the client gets the response immediately
        request->send(response_);
        ShouldReboot = true;
        }
    );

    server.on("/DateTime", HTTP_GET, [](AsyncWebServerRequest* request) { handleDateTime(request); });
    server.on("/power", HTTP_GET, [](AsyncWebServerRequest* request) { handlePower(request); });
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest* request) { handleData(request); });
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest* request) { handlePost(request); });

    server.onNotFound([](AsyncWebServerRequest* request) {
        AsyncWebRequestWrapper asyncWebRequestWrapper_(request);
        iotWebConf.handleNotFound(&asyncWebRequestWrapper_);
        }
    );

    WebSerial.begin(&server, "/webserial");

    Serial.println("Ready.");
}

void wifiLoop() {
    iotWebConf.doLoop();
    ArduinoOTA.handle();

    if (!ShouldReboot) {
		ShouldReboot = AsyncUpdater.isFinished();
	}
}

void wifiConnected(){
    ArduinoOTA.begin();
}

void onProgress(size_t prg, size_t sz) {
    static size_t lastPrinted_ = 0;
    size_t currentPercent_ = (prg * 100) / sz;

    if (currentPercent_ % 5 == 0 && currentPercent_ != lastPrinted_) {
        Serial.printf("Progress: %d%%\n", currentPercent_);
        lastPrinted_ = currentPercent_;
    }
}

void handleDateTime(AsyncWebServerRequest* request) {
    ESP_LOGD("handleDateTime", "Time requested");

    // Get the current time
    time_t now_ = time(nullptr);
    struct tm* timeinfo_ = localtime(&now_);
    char timeStr_[20];
    char dateStr_[20];
    strftime(timeStr_, sizeof(timeStr_), "%H:%M:%S", timeinfo_);
    strftime(dateStr_, sizeof(dateStr_), "%Y-%m-%d", timeinfo_);

	request->send(200, "text/plain", timeStr_);
}

void handlePower(AsyncWebServerRequest* request) {
	request->send(200, "text/plain", String(inverter.getPowerData().activePower * 1000, 0));
}

void handleData(AsyncWebServerRequest* request) {
    PowerData powerData_ = inverter.getPowerData();
    MeterData meterData_ = inverter.getMeterData();
	StatusData statusData_ = inverter.getStatusData();   

    String json_ = "{";
    json_ += "\"RSSI\":\"" + String(WiFi.RSSI()) + "\"";

    // Grid node
    json_ += ",\"Grid\":{";
    json_ += "\"VoltageA\":\"" + String(meterData_.gridVoltageA, 1) + "\"";
    json_ += ",\"VoltageB\":\"" + String(meterData_.gridVoltageB, 1) + "\"";
    json_ += ",\"VoltageC\":\"" + String(meterData_.gridVoltageC, 1) + "\"";
    json_ += ",\"CurrentA\":\"" + String(meterData_.gridCurrentA, 3) + "\"";
    json_ += ",\"CurrentB\":\"" + String(meterData_.gridCurrentB, 3) + "\"";
    json_ += ",\"CurrentC\":\"" + String(meterData_.gridCurrentC, 3) + "\"";
    json_ += ",\"Frequency\":\"" + String(meterData_.gridFrequency, 2) + "\"";
    json_ += ",\"PowerFactor\":\"" + String(meterData_.powerFactor, 3) + "\"";
	json_ += ",\"ActivePower\":\"" + String(meterData_.activePower, 0) + "\"";
	json_ += ",\"ReactivePower\":\"" + String(meterData_.reactivePower, 0) + "\"";
    json_ += "}";

    // Inverter node
    json_ += ",\"Inverter\":{";
    json_ += "\"InputPower\":\"" + String(powerData_.inputPower * 1000, 0) + "\"";
    json_ += ",\"ActivePower\":\"" + String(powerData_.activePower * 1000, 0) + "\"";
    json_ += ",\"ReactivePower\":\"" + String(powerData_.reactivePower * 1000, 0) + "\"";
    json_ += ",\"GridVoltageAB\":\"" + String(powerData_.gridVoltageAB, 1) + "\"";
    json_ += ",\"GridVoltageBC\":\"" + String(powerData_.gridVoltageBC, 1) + "\"";
    json_ += ",\"GridVoltageCA\":\"" + String(powerData_.gridVoltageCA, 1) + "\"";
    json_ += ",\"GridCurrentA\":\"" + String(powerData_.gridCurrentA, 3) + "\"";
    json_ += ",\"GridCurrentB\":\"" + String(powerData_.gridCurrentB, 3) + "\"";
    json_ += ",\"GridCurrentC\":\"" + String(powerData_.gridCurrentC, 3) + "\"";
    json_ += ",\"PowerFactor\":\"" + String(powerData_.powerFactor, 3) + "\"";
    json_ += ",\"Standby\":\"" + String(statusData_.isStandby() ? "Yes" : "No") + "\"";
    json_ += "}";

    json_ += ",\"SwitchDevices\":{";
    for (size_t i_ = 0; i_ < devices.size(); ++i_) {
        if (devices[i_]->isActive()) {
            switch (devices[i_]->getStatus()) {
            case SwitchDevice::Enabled:
                json_ += "\"device" + String(i_ + 1) + "\":\"On\"";
                break;
            case  SwitchDevice::DelayedOff:
                json_ += "\"device" + String(i_ + 1) + "\":\"DelayedOff\"";
                break;
            case  SwitchDevice::Disabled:
            default:
                json_ += "\"device" + String(i_ + 1) + "\":\"Off\"";
                break;
            }
            // Komma nur, wenn noch ein aktives Device folgt
            if (i_ + 1 < devices.size()) {
                // Suche, ob noch ein aktives Device folgt
                bool moreActive_ = false;
                for (size_t j_ = i_ + 1; j_ < devices.size(); ++j_) {
                    if (devices[j_]->isActive()) {
                        moreActive_ = true;
                        break;
                    }
                }
                if (moreActive_) json_ += ",";
            }
        }
    }
    json_ += "}";
    json_ += "}";
	request->send(200, "application/json", json_);  
}

void handlePost(AsyncWebServerRequest* request) {
    for (size_t i_ = 0; i_ < devices.size(); ++i_) {
        String paramName = "device" + String(i_ + 1);
        if (request->hasParam(paramName, true)) {
            String value_ = request->getParam(paramName, true)->value();
            devices[i_]->setEnabled(value_ == "on");
        }
    }

    if (request->hasParam("reset", true)) {
        String value_ = request->getParam("reset", true)->value();
        if (value_ == "true") {
            Serial.println("apply default values...");
            for (auto device_ : devices) {
                device_->applyDefaultValue();
            }
        }
    }

    request->redirect("/");
}

void handleAPPasswordMissingPage(iotwebconf::WebRequestWrapper* webRequestWrapper) {
    String content_;
    MyHtmlRootFormatProvider fp_;

    content_ += fp_.getHtmlHead(iotWebConf.getThingName());
    content_ += fp_.getHtmlStyle();
    content_ += fp_.getHtmlHeadEnd();
    content_ += "<body>";
    content_ += "You should change the default AP password to continue.";
    content_ += fp_.addNewLine(2);
    content_ += "Return to <a href='config'>configuration page</a>.";
    content_ += fp_.addNewLine(1);
    content_ += "Return to <a href='/'>home page</a>.";
    content_ += "</body>";
    content_ += fp_.getHtmlEnd();

    webRequestWrapper->sendHeader("Content-Length", String(content_.length()));
    webRequestWrapper->send(200, "text/html", content_);
}

void handleSSIDNotConfiguredPage(iotwebconf::WebRequestWrapper* webRequestWrapper) {
    webRequestWrapper->sendHeader("Location", "/", true);
    webRequestWrapper->send(302, "text/plain", "SSID not configured");
}

void handleConfigSavedPage(iotwebconf::WebRequestWrapper* webRequestWrapper) {
    webRequestWrapper->sendHeader("Location", "/", true);
    webRequestWrapper->send(302, "text/plain", "Config saved");
}

void handleRoot(AsyncWebServerRequest* request) {
    AsyncWebRequestWrapper asyncWebRequestWrapper(request);
    if (iotWebConf.handleCaptivePortal(&asyncWebRequestWrapper)) {
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
    response_ += fp_.getHtmlTableRowSpan("Power Direction:", "no data", "gridPowerDirection");

    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.getHtmlFieldset("Inverter");
    response_ += fp_.getHtmlTable();
    response_ += fp_.getHtmlTableRowText("Polling intervall:", String(inverterConfig.getInterval()) + "s");
    response_ += fp_.getHtmlTableRowSpan("Standby:", "no data", "inverterStandby");
    response_ += fp_.getHtmlTableRowSpan("Active Power:", "no data", "inverterActivePower");
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.getHtmlFieldset("Device");
    response_ += fp_.getHtmlTable();
    for (size_t i_ = 0; i_ < devices.size(); ++i_) {
        if (devices[i_]->isActive()) {
            response_ += fp_.getHtmlTableRowClass(
                devices[i_]->getDesignation() + ":", "led off", "device" + String(i_ + 1)
            );
        }
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

    response_ += fp_.getHtmlTableRowText("Open <a href='/config'>configuration page</a>.");
    response_ += fp_.getHtmlTableRowText("Open <a href='/webserial' target='_blank'>WebSerial Console</a> for live logs.");
    response_ += fp_.getHtmlTableRowText(fp_.getHtmlVersion(Version));
    response_ += fp_.getHtmlTableEnd();

    response_ += fp_.getHtmlTableColEnd() + fp_.getHtmlTableRowEnd();
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlEnd();

	request->send(200, "text/html", response_);
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
    ArduinoOTA.setHostname(iotWebConf.getThingName());
}

void configSaved() {
    convertParams();
    gParamsChanged = true;
}