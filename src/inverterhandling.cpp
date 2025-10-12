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

InverterPowerData inverterPowerData;
InverterStatusData inverterStatusData;
MeterData meterData;

enum RequestType { REQ_INPUT, REQ_METER, REQ_POWER };
struct RequestContext {
    RequestType type;
    uint32_t token;
};
RequestContext lastRequest;

ModbusClientTCP ModbusClient(wifiClient);
Neotimer InverterInterval = Neotimer(gInverterInterval * 1000);

uint16_t getU16(const ModbusMessage& msg, uint16_t idx) {
    return (msg[idx] << 8) | msg[idx + 1];
}

int32_t getI32(const ModbusMessage& msg, uint16_t idx) {
    return (msg[idx] << 24) | (msg[idx + 1] << 16) | (msg[idx + 2] << 8) | msg[idx + 3];
}

void handleMeterData(ModbusMessage response, uint32_t token) {
    uint16_t idx_ = 3; // Modbus-Daten beginnen ab Index 3

    meterData.meterStatus = getU16(response, idx_); idx_ += 2;
    meterData.gridVoltageA = getI32(response, idx_) / 10; idx_ += 4;
    meterData.gridVoltageB = getI32(response, idx_) / 10; idx_ += 4;
    meterData.gridVoltageC = getI32(response, idx_) / 10; idx_ += 4;
    meterData.gridCurrentA = getI32(response, idx_) / 100; idx_ += 4;
    meterData.gridCurrentB = getI32(response, idx_) / 100; idx_ += 4;
    meterData.gridCurrentC = getI32(response, idx_) / 100; idx_ += 4;
    meterData.activePower = getI32(response, idx_); idx_ += 4;
    meterData.reactivePower = getI32(response, idx_); idx_ += 4;
    meterData.powerFactor = getU16(response, idx_) / 1000; idx_ += 2;
    meterData.gridFrequency = getU16(response, idx_) / 100; idx_ += 2;
    meterData.positiveActiveElectricity = getI32(response, idx_) / 100; idx_ += 4;
    meterData.reverseActivePower = getI32(response, idx_) / 100; idx_ += 4;
    meterData.accumulatedReactivePower = getI32(response, idx_) / 100; idx_ += 4;
    meterData.meterType = getU16(response, idx_); idx_ += 2;
    meterData.abLineVoltage = getI32(response, idx_) / 10; idx_ += 4;
    meterData.bcLineVoltage = getI32(response, idx_) / 10; idx_ += 4;
    meterData.caLineVoltage = getI32(response, idx_) / 10; idx_ += 4;
    meterData.aPhaseActivePower = getI32(response, idx_); idx_ += 4;
    meterData.bPhaseActivePower = getI32(response, idx_); idx_ += 4;
    meterData.cPhaseActivePower = getI32(response, idx_); idx_ += 4;
    meterData.meterModelDetection = getU16(response, idx_);
}

void handlePowerData(ModbusMessage response, uint32_t token) {
    Serial.printf("Received inverter power data, token=%08X\n", token);
    uint16_t idx_ = 3;
    inverterPowerData.inputPower = (float)getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.gridVoltageAB = (float)getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.gridVoltageBC = (float)getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.gridVoltageCA = (float)getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.phaseAVoltage = (float)getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.phaseBVoltage = (float)getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.phaseCVoltage = (float)getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.gridCurrentA = (float)getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.gridCurrentB = (float)getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.gridCurrentC = (float)getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.peakActivePowerDay = (float)getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.activePower = (float)getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.reactivePower = (float)getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.powerFactor = (float)getU16(response, idx_) / 1000.0f;
}

void handleInverterStatusData(ModbusMessage response, uint32_t token) {
    if (lastRequest.token != token) return;
    uint16_t idx_ = 3;

    inverterStatusData.state1 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.state2 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.state3 = ((uint32_t)getU16(response, idx_) << 16) | getU16(response, idx_ + 2); idx_ += 4;
    inverterStatusData.alarm1 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.alarm2 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.alarm3 = getU16(response, idx_);
}

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

bool testInverterConnection(const char* ip, uint16_t port) {
    WiFiClient testClient;
    Serial.printf("Testing connection to %s:%d ...\n", ip, port);
    if (testClient.connect(ip, port)) {
        Serial.println("Connection successful!");
        testClient.stop();
        return true;
    }
    else {
        Serial.println("Connection failed!");
        return false;
    }
}

void setupInverter() {
    ModbusClient.onErrorHandler(&handleError);
    ModbusClient.setTimeout(10000, 1000);
    ModbusClient.begin();
    InverterInterval.start();
}

void loopInverter() {
    if (gParamsChanged) {
        InverterInterval.start(gInverterInterval * 1000);
    }

    if (InverterInterval.repeat()) {
        //lastRequest = { REQ_INPUT, millis() };
        //ModbusClient.onDataHandler(&handleData);
        //requestInverter(gInverterIPAddress, gInverterPort, gInverterInputPowerRegister, gInverterInputPowerDataLength);


  //      lastRequest = { REQ_METER, millis() };
  //      ModbusClient.onDataHandler(&handleMeterData);
  //      requestInverter(gInverterIPAddress, gInverterPort, 37100, 39);

        lastRequest = { REQ_POWER, millis() };
        ModbusClient.onDataHandler(&handlePowerData);
        requestInverter(gInverterIPAddress, gInverterPort, 32064, 21);

		//lastRequest = { REQ_POWER, millis() };
  //      ModbusClient.onDataHandler(&handleInverterStatusData);
  //      requestInverter(gInverterIPAddress, gInverterPort, 30000, 7);
		//// Update global input power variable

		//gInputPower = inverterPowerData.inputPower;

		////Serial.printf("Inverter status: Meter Power=%d W, Inverter Power=%d W\n", meterData.activePower, inverterPowerData.activePower);
  ////      Serial,printf("Inverter status: State1=%04X, State2=%04X, State3=%08X, Alarm1=%04X, Alarm2=%04X, Alarm3=%04X\n",
  ////          inverterStatusData.state1, inverterStatusData.state2, inverterStatusData.state3,
		////	inverterStatusData.alarm1, inverterStatusData.alarm2, inverterStatusData.alarm3);
		////Serial.printf("Inverter status: Grid Voltage AB=%d V, BC=%d V, CA=%d V\n", 
  ////          inverterPowerData.gridVoltageAB, inverterPowerData.gridVoltageBC, inverterPowerData.gridVoltageCA);
		////Serial.printf("Inverter power: %d W\n", gInputPower);

        Serial.printf("Grid voltages: A=%d V, B=%d V, C=%d V\n",
            inverterPowerData.phaseAVoltage,
            inverterPowerData.phaseBVoltage,
            inverterPowerData.phaseCVoltage);

        Serial.printf("Grid currents: A=%d mA, B=%d mA, C=%d mA\n",
            inverterPowerData.gridCurrentA,
            inverterPowerData.gridCurrentB,
            inverterPowerData.gridCurrentC);

		Serial.printf("Inverter input power: %d W\n", inverterPowerData.inputPower);

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
	Serial.printf("Sent request to inverter at %s:%d, startaddress=%d, number=%d, token=%08X\n", ip.c_str(), port, startaddress, number, token_);
}


