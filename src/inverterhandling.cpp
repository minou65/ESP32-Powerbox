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

int gActivePower = 0;
uint8_t gInverterActivePowerInterval = 10;

char gInverterIPAddress[15] = "0.0.0.0";
int gInverterPort = 502;
uint16_t gInverterActivePowerRegister = 37113;
uint8_t gInverterActivePowerDataLength = 2;
uint16_t gInverterActivePowerGain = 1;

// W5500 reset pin 
#define RESET_P GPIO_NUM_26

IPAddress gserver(192, 168, 1, 10); // update with the IP Address of your Modbus server

byte mac[] = { 0x30, 0x2B, 0x2D, 0x2F, 0x61, 0xE2 }; // MAC address (fill your own data here!)
IPAddress lIP;                      // IP address after Ethernet.begin()
ModbusClientTCP ModbusClient(wifiClient);
Neotimer InverterInterval = Neotimer(gInverterActivePowerInterval * 1000);

void handleData(ModbusMessage response, uint32_t token)
{
    Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
    //for (auto& byte : response) {
    //    Serial.printf("%02X ", byte);
    //}
    Serial.print("Power: ");

    int a = int((unsigned char)(response[3]) << 24 |
        (unsigned char)(response[4]) << 16 |
        (unsigned char)(response[5]) << 8 |
        (unsigned char)(response[6]));

    a = a / gInverterActivePowerGain;

    Serial.print(a);
    Serial.println("W");
    Serial.println("");

    gActivePower = a;
}

void handleError(Error error, uint32_t token)
{
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    Serial.printf("Error response: %02X - %s\n", (int)me, (const char*)me);
}

void InverterSetup() {

    IPAddress wIP = WiFi.localIP();
    Serial.printf("WIFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);

    ModbusClient.onDataHandler(&handleData);
    ModbusClient.onErrorHandler(&handleError);
    ModbusClient.setTimeout(2000, 200);
    ModbusClient.begin();

    gActivePower = 0;

}

void InverterLoop() {
    if (gParamsChanged) {
        InverterInterval.start(gInverterActivePowerInterval * 1000);
    }

    if (InverterInterval.repeat()) {
        InverterRequest(gInverterIPAddress, gInverterPort, gInverterActivePowerRegister, gInverterActivePowerDataLength);
    }
}


void InverterRequest(String ip, uint16_t port, uint16_t startaddress, uint16_t number ) {

    IPAddress host;
    
    host.fromString(ip);

    ModbusClient.setTarget(host, 502);

    uint32_t token = (uint32_t)millis();
    uint8_t slave = 1;

    Error err = ModbusClient.addRequest(token, slave, READ_HOLD_REGISTER, startaddress, number);
    if (err != SUCCESS) {
        ModbusError e(err);
        Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char*)e);
    }
}


