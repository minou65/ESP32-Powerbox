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

enum RequestType { 
    REQ_INVERTER = 1001, 
    REQ_METER = 1002, 
    REQ_STATUS = 1003
};
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
	Serial.printf("Received meter data, token=%08X\n", token);
    uint16_t idx_ = 3; // Modbus data starts at index 3

    meterData.meterStatus = getU16(response, idx_); idx_ += 2;
    meterData.gridVoltageA = getI32(response, idx_) / 10.0f; idx_ += 4;
    meterData.gridVoltageB = getI32(response, idx_) / 10.0f; idx_ += 4;
    meterData.gridVoltageC = getI32(response, idx_) / 10.0f; idx_ += 4;
    meterData.gridCurrentA = getI32(response, idx_) / 100.0f; idx_ += 4;
    meterData.gridCurrentB = getI32(response, idx_) / 100.0f; idx_ += 4;
    meterData.gridCurrentC = getI32(response, idx_) / 100.0f; idx_ += 4;
    meterData.activePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    meterData.reactivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    meterData.powerFactor = getU16(response, idx_) / 1000.0f; idx_ += 2;
    meterData.gridFrequency = getU16(response, idx_) / 100.0f; idx_ += 2;
    meterData.positiveActiveElectricity = getI32(response, idx_) / 100.0f; idx_ += 4;
    meterData.reverseActivePower = getI32(response, idx_) / 100.0f; idx_ += 4;
    meterData.accumulatedReactivePower = getI32(response, idx_) / 100.0f; idx_ += 4;
    meterData.meterType = getU16(response, idx_); idx_ += 2;
    meterData.abLineVoltage = getI32(response, idx_) / 10.0f; idx_ += 4;
    meterData.bcLineVoltage = getI32(response, idx_) / 10.0f; idx_ += 4;
    meterData.caLineVoltage = getI32(response, idx_) / 10.0f; idx_ += 4;
    meterData.aPhaseActivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    meterData.bPhaseActivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    meterData.cPhaseActivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    meterData.meterModelDetection = getU16(response, idx_);

    Serial.printf("    Meter voltages: A=%.1f V, B=%.1f V, C=%.1f V\n",
        meterData.gridVoltageA,
        meterData.gridVoltageB,
        meterData.gridVoltageC);

    Serial.printf("    Meter currents: A=%.3f A, B=%.3f A, C=%.3f A\n",
        meterData.gridCurrentA,
        meterData.gridCurrentB,
        meterData.gridCurrentC);

    Serial.printf("    Meter active power: %.1f W\n", meterData.activePower);
    Serial.printf("    Meter reactive power: %.1f Var\n", meterData.reactivePower);
    Serial.printf("    Meter frequency: %.2f Hz\n", meterData.gridFrequency);
    Serial.printf("    Meter power factor: %.3f\n", meterData.powerFactor);
}

void handlePowerData(ModbusMessage response, uint32_t token) {
    Serial.printf("Received inverter power data, token=%08X\n", token);
    uint16_t idx_ = 3;
    inverterPowerData.inputPower = getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.gridVoltageAB = getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.gridVoltageBC = getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.gridVoltageCA = getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.phaseAVoltage = getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.phaseBVoltage = getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.phaseCVoltage = getU16(response, idx_) / 10.0f;   idx_ += 2;
    inverterPowerData.gridCurrentA = getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.gridCurrentB = getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.gridCurrentC = getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.peakActivePowerDay = getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.activePower = getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.reactivePower = getI32(response, idx_) / 1000.0f; idx_ += 4;
    inverterPowerData.powerFactor = getU16(response, idx_) / 1000.0f;

    Serial.printf("    Grid currents: A=%.3f A, B=%.3f A, C=%.3f A\n",
        inverterPowerData.gridCurrentA,
        inverterPowerData.gridCurrentB,
        inverterPowerData.gridCurrentC);

    Serial.printf("    Grid voltages: A=%.1f V, B=%.1f V, C=%.1f V\n",
        inverterPowerData.phaseAVoltage,
        inverterPowerData.phaseBVoltage,
        inverterPowerData.phaseCVoltage);

    Serial.printf("    Inverter input power: %.3f kW\n", inverterPowerData.inputPower);
}

void handleInverterStatusData(ModbusMessage response, uint32_t token) {
    Serial.printf("Received inverter status data, token=%08X\n", token);
    uint16_t idx_ = 3;

    inverterStatusData.state1 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.state2 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.state3 = ((uint32_t)getU16(response, idx_) << 16) | getU16(response, idx_ + 2); idx_ += 4;
    inverterStatusData.alarm1 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.alarm2 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.alarm3 = getU16(response, idx_);

    Serial.printf("    Inverter states: state1=0x%04X, state2=0x%04X, state3=0x%08X\n",
        inverterStatusData.state1,
        inverterStatusData.state2,
        inverterStatusData.state3);

    Serial.printf("    Inverter alarms: alarm1=0x%04X, alarm2=0x%04X, alarm3=0x%04X\n",
        inverterStatusData.alarm1,
        inverterStatusData.alarm2,
        inverterStatusData.alarm3);
}

void handleData(ModbusMessage response, uint32_t token){
    switch (token) {
        case REQ_INVERTER:
            handlePowerData(response, token);
            break;
        case REQ_METER:
            handleMeterData(response, token);
            break;
        case REQ_STATUS:
            handleInverterStatusData(response, token);
            break;
        default:
            Serial.println("Unknown RequestType!");
            break;
    }
}

void handleError(Error error, uint32_t token){
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me_(error);
    Serial.printf("Error response: %02X - %s\n", (int)me_, (const char*)me_);
}

void setupInverter() {
    ModbusClient.onErrorHandler(&handleError);
    ModbusClient.setTimeout(2000, 10);
    ModbusClient.onDataHandler(&handleData);
	ModbusClient.closeConnectionOnTimeouts(3);
    ModbusClient.begin();
    InverterInterval.start();
}

void loopInverter() {
    if (gParamsChanged) {
        InverterInterval.start(gInverterInterval * 1000);
    }

    if (InverterInterval.repeat()) {
        requestInverter(
            gInverterIPAddress, 
            gInverterPort, 
            inverterStatusData.startAddress, 
            inverterStatusData.dataLength, 
            uint32_t(RequestType::REQ_STATUS)
		);

        if (inverterStatusData.isStandby()){
            Serial.println("Inverter is in standby mode, skipping power and meter data request.");
            //return;
		}
        
        requestInverter(
            gInverterIPAddress, 
            gInverterPort, 
            inverterPowerData.startAddress, 
            inverterPowerData.dataLength, 
            uint32_t(RequestType::REQ_INVERTER)
        );

        requestInverter(
            gInverterIPAddress,
            gInverterPort,
            meterData.startAddress,
            meterData.dataLength,
            uint32_t(RequestType::REQ_METER)
		);
    }
}


void requestInverter(const String ip, uint16_t port, uint16_t startaddress, uint16_t number, uint32_t token) {
    IPAddress host_;
    if (!host_.fromString(ip)) {
        Serial.printf("Invalid IP address: %s\n", ip.c_str());
        return;
    }
    ModbusClient.setTarget(host_, port);
    uint8_t slave_ = 1;

    Error err_ = ModbusClient.addRequest(token, slave_, READ_HOLD_REGISTER, startaddress, number);
    if (err_ != SUCCESS) {
        ModbusError e_(err_);
        Serial.printf("Error creating request: %02X - %s\n", (int)e_, (const char*)e_);
    }
    else {
        Serial.printf("Request sent to inverter at %s:%d, start address=%d, count=%d, token=%08X\n",
            ip.c_str(), port, startaddress, number, token);
    }
}


