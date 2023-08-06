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

int gActivePower;

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

// Include the header for the ModbusClient TCP style
#include <ModbusClientTCP.h>

// Create a ModbusTCP client instance
ModbusClientTCP ModbusClient(wifiClient);


// Define an onData handler function to receive the regular responses
// Arguments are the message plus a user-supplied token to identify the causing request
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

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token)
{
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    Serial.printf("Error response: %02X - %s\n", (int)me, (const char*)me);
}


void InverterSetup() {
    // Init Serial monitor
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("__ OK __");

    IPAddress wIP = WiFi.localIP();
    Serial.printf("WIFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);

    // Set up ModbusTCP client.
    // - provide onData handler function
    ModbusClient.onDataHandler(&handleData);
    // - provide onError handler function
    ModbusClient.onErrorHandler(&handleError);
    // Set message timeout to 2000ms and interval between requests to the same host to 200ms
    ModbusClient.setTimeout(2000, 200);
    // Start ModbusTCP background task
    ModbusClient.begin();

    gActivePower = 0;

}

uint8_t gInverterActivePowerInterval = 10;
unsigned long time_now = 0;

void InverterLoop() {
    if (millis() > time_now + (gInverterActivePowerInterval * 1000)) {
        time_now = millis();
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


