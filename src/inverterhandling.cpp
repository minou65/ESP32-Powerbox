// 
// 
// 

// Includes: <Arduino.h> for Serial etc., EThernet.h for Ethernet support
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "common.h"
#include "inverterhandling.h"
#include "webhandling.h"
#include <ModbusClientTCP.h>

int gInputPower = 0;
uint8_t gInverterInterval = 10;

char gInverterIPAddress[15] = "0.0.0.0";
int gInverterPort = 502;
uint16_t gInverterInputPowerRegister = 32064;
uint8_t gInverterInputPowerDataLength = 2;
uint16_t gInverterInputPowerGain = 1;

ModbusClientTCP ModbusClient(wifiClient);
Neotimer InverterInterval = Neotimer(gInverterInterval * 1000);

void handleData(ModbusMessage response, uint32_t token){
    //if (Token != token) { return; };

    //Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
    //for (auto& byte : response) {
    //    Serial.printf("%02X ", byte);
    //}

    Serial.print("Input power: ");

    int a_ = int((unsigned char)(response[3]) << 24 |
        (unsigned char)(response[4]) << 16 |
        (unsigned char)(response[5]) << 8 |
        (unsigned char)(response[6]));

    a_ = a_ / gInverterInputPowerGain;

    Serial.println(a_); 
    gInputPower = a_;
}

void handleError(Error error, uint32_t token){
    //if (Token != token) { return; };
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me_(error);
    Serial.printf("Error response: %02X - %s\n", (int)me_, (const char*)me_);
}

void setupInverter() {

    ModbusClient.onDataHandler(&handleData);
    ModbusClient.onErrorHandler(&handleError);
    ModbusClient.setTimeout(2000, 200);
    ModbusClient.begin();
    InverterInterval.start();
}

void loopInverter() {
    if (gParamsChanged) {
        InverterInterval.start(gInverterInterval * 1000);
    }

    if (InverterInterval.repeat()) {
        requestInverter(gInverterIPAddress, gInverterPort, gInverterInputPowerRegister, gInverterInputPowerDataLength);
    }
}


void requestInverter(String ip, uint16_t port, uint16_t startaddress, uint16_t number ) {

    IPAddress host_;

    host_.fromString(ip);

    ModbusClient.setTarget(host_, port);

    uint32_t token_ = (uint32_t)millis();
    uint8_t slave_ = 1;

    Error err_ = ModbusClient.addRequest(token_, slave_, READ_HOLD_REGISTER, startaddress, number);
    if (err_ != SUCCESS) {
        ModbusError e_(err_);
        Serial.printf("Error creating request: %02X - %s\n", (int)e_, (const char*)e_);
    }
}


