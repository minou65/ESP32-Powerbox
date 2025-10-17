#include <Arduino.h>
#include <WiFi.h>
#include <ModbusClientTCP.h>
#include <esp_log.h>
#include "common.h"
#include "inverterhandling.h"
#include "webhandling.h"

uint8_t gInverterInterval = 10;

char gInverterIPAddress[15] = "0.0.0.0";
int gInverterPort = 502;

InverterPowerData inverterPowerData;
InverterStatusData inverterStatusData;
MeterData meterData;

enum REQUEST_TYPE { 
    REQUEST_TYPE_INVERTER = 1001,
    REQUEST_TYPE_METER = 1002,
    REQUEST_TYPE_STATUS = 1003
};
struct RequestContext {
    REQUEST_TYPE type;
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
    ESP_LOGI("INVERTER", "Received meter data, token=%08X", token);
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

    ESP_LOGI("INVERTER", "    Meter voltages: A=%.1f V, B=%.1f V, C=%.1f V",
        meterData.gridVoltageA,
        meterData.gridVoltageB,
        meterData.gridVoltageC);

    ESP_LOGI("INVERTER", "    Meter currents: A=%.3f A, B=%.3f A, C=%.3f A",
        meterData.gridCurrentA,
        meterData.gridCurrentB,
        meterData.gridCurrentC);

    ESP_LOGI("INVERTER", "    Meter active power: %.1f W", meterData.activePower);
    ESP_LOGI("INVERTER", "    Meter reactive power: %.1f Var", meterData.reactivePower);
    ESP_LOGI("INVERTER", "    Meter frequency: %.2f Hz", meterData.gridFrequency);
    ESP_LOGI("INVERTER", "    Meter power factor: %.3f", meterData.powerFactor);
}

void handlePowerData(ModbusMessage response, uint32_t token) {
    ESP_LOGI("INVERTER", "Received inverter power data, token=%08X", token);
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

    ESP_LOGI("INVERTER", "    Grid currents: A=%.3f A, B=%.3f A, C=%.3f A",
        inverterPowerData.gridCurrentA,
        inverterPowerData.gridCurrentB,
        inverterPowerData.gridCurrentC);

    ESP_LOGI("INVERTER", "    Grid voltages: A=%.1f V, B=%.1f V, C=%.1f V",
        inverterPowerData.phaseAVoltage,
        inverterPowerData.phaseBVoltage,
        inverterPowerData.phaseCVoltage);

    ESP_LOGI("INVERTER", "    Inverter input power: %.3f kW", inverterPowerData.inputPower);
}

void handleInverterStatusData(ModbusMessage response, uint32_t token) {
    ESP_LOGI("INVERTER", "Received inverter status data, token=%08X", token);
    uint16_t idx_ = 3;

    inverterStatusData.state1 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.state2 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.state3 = ((uint32_t)getU16(response, idx_) << 16) | getU16(response, idx_ + 2); idx_ += 4;
    inverterStatusData.alarm1 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.alarm2 = getU16(response, idx_); idx_ += 2;
    inverterStatusData.alarm3 = getU16(response, idx_);

    ESP_LOGI("INVERTER", "    Inverter states: state1=0x%04X, state2=0x%04X, state3=0x%08X",
        inverterStatusData.state1,
        inverterStatusData.state2,
        inverterStatusData.state3);

    ESP_LOGI("INVERTER", "    Inverter alarms: alarm1=0x%04X, alarm2=0x%04X, alarm3=0x%04X",
        inverterStatusData.alarm1,
        inverterStatusData.alarm2,
        inverterStatusData.alarm3);
}

void handleData(ModbusMessage response, uint32_t token) {
    switch (token) {
    case REQUEST_TYPE_INVERTER:
        handlePowerData(response, token);
        break;
    case REQUEST_TYPE_METER:
        handleMeterData(response, token);
        break;
    case REQUEST_TYPE_STATUS:
        handleInverterStatusData(response, token);
        break;
    default:
        ESP_LOGW("INVERTER", "Unknown REQUEST_TYPE!");
        break;
    }
}

void handleError(Error error, uint32_t token) {
    ModbusError me_(error);
    ESP_LOGE("INVERTER", "Error response: %02X - %s", (int)me_, (const char*)me_);
}

void requestInverter(const String ipAddress, uint16_t port, uint16_t startaddress, uint16_t registerCount, uint32_t token) {
    IPAddress host_;
    if (!host_.fromString(ipAddress)) {
        ESP_LOGE("INVERTER", "Invalid IP address: %s", ipAddress.c_str());
        return;
    }
    ModbusClient.setTarget(host_, port);
    uint8_t slave_ = 1;

    Error err_ = ModbusClient.addRequest(token, slave_, READ_HOLD_REGISTER, startaddress, registerCount);
    if (err_ != SUCCESS) {
        ModbusError e_(err_);
        ESP_LOGE("INVERTER", "Error creating request: %02X - %s", (int)e_, (const char*)e_);
    }
    else {
        ESP_LOGI("INVERTER", "Request sent to inverter at %s:%d, start address=%d, count=%d, token=%08X",
            ip.c_str(), port, startaddress, registerCount, token);
    }
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
            uint32_t(REQUEST_TYPE_STATUS)
		);

        if (inverterStatusData.isStandby()){
            // Serial.println("Inverter is in standby mode, skipping power and meter data request.");
		}
        
        requestInverter(
            gInverterIPAddress, 
            gInverterPort, 
            inverterPowerData.startAddress, 
            inverterPowerData.dataLength, 
            uint32_t(REQUEST_TYPE_INVERTER)
        );

        requestInverter(
            gInverterIPAddress,
            gInverterPort,
            meterData.startAddress,
            meterData.dataLength,
            uint32_t(REQUEST_TYPE_METER)
		);
    }
}


