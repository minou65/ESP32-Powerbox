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

uint32_t Token;
uint8_t slave = 1;

ModbusClientTCP ModbusClient(wifiClient);
Neotimer InverterInterval = Neotimer(gInverterInterval * 1000);

void handleData(ModbusMessage response, uint32_t token){
    if (Token != token) { return; };

    //Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
    //for (auto& byte : response) {
    //    Serial.printf("%02X ", byte);
    //}

    int a = int((unsigned char)(response[3]) << 24 |
        (unsigned char)(response[4]) << 16 |
        (unsigned char)(response[5]) << 8 |
        (unsigned char)(response[6]));

    a = a / gInverterInputPowerGain;

    Serial.printf("Input power: %i\n\n", a); 
    gInputPower = a;
}

void handleError(Error error, uint32_t token){
    if (Token != token) { return; };
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    Serial.printf("Error response: %02X - %s\n", (int)me, (const char*)me);
}

void InverterSetup() {
    Token = (uint32_t)millis();

    ModbusClient.onDataHandler(&handleData);
    ModbusClient.onErrorHandler(&handleError);
    ModbusClient.setTimeout(2000, 200);
    ModbusClient.begin();
    InverterInterval.start();
}

void InverterLoop() {
    if (gParamsChanged) {
        InverterInterval.start(gInverterInterval * 1000);

        IPAddress host;
        host.fromString(gInverterIPAddress);
        ModbusClient.setTarget(host, gInverterPort);
    }

    if (InverterInterval.repeat()) {
        Error err = ModbusClient.addRequest(Token, slave, READ_HOLD_REGISTER, gInverterInputPowerRegister, gInverterInputPowerDataLength);
        if (err != SUCCESS) {
            ModbusError e(err);
            Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char*)e);
        }
    }
}


void InverterRequest(String ip, uint16_t port, uint16_t startaddress, uint16_t number ) {

    IPAddress host;
    host.fromString(ip);
    ModbusClient.setTarget(host, 502);

    Error err = ModbusClient.addRequest(Token, slave, READ_HOLD_REGISTER, startaddress, number);
    if (err != SUCCESS) {
        ModbusError e(err);
        Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char*)e);
    }
}


