#include <Arduino.h>
#include <WiFi.h>
#include <ModbusClientTCP.h>
#include <esp_log.h>
#include "common.h"
#include "inverterhandling.h"
#include "webhandling.h"

uint16_t getU16(const ModbusMessage& msg, uint16_t idx) {
    return (msg[idx] << 8) | msg[idx + 1];
}

int32_t getI32(const ModbusMessage& msg, uint16_t idx) {
    return (msg[idx] << 24) | (msg[idx + 1] << 16) | (msg[idx + 2] << 8) | msg[idx + 3];
}

Inverter::Inverter(WiFiClient& client) : 
    _modbusClient(client), 
    _intervalTimer(10 * 1000) // Default 10s
{
}

void Inverter::begin(const String& ipAddress, const uint16_t port, const uint8_t interval) {
    _modbusClient.onErrorHandler([this](Error error, uint32_t token) { handleError(error, token); });
    _modbusClient.setTimeout(2000, 10);
    _modbusClient.onDataHandler([this](ModbusMessage response, uint32_t token) { handleData(response, token); });
    _modbusClient.closeConnectionOnTimeouts(3);
    _modbusClient.begin();
    _intervalTimer.start(interval * 1000);
	ipAddress.toCharArray(_IPAddress, sizeof(_IPAddress));
	_Port = port;
}

void Inverter::process() {
    if (_intervalTimer.repeat()) {
        requestData(
            _IPAddress,
            _Port,
            _StatusData.startAddress,
            _StatusData.dataLength,
            uint32_t(REQUEST_TYPE_STATUS)
        );

        if (!_StatusData.isStandby()) {
            requestData(
                _IPAddress,
                _Port,
                _PowerData.startAddress,
                _PowerData.dataLength,
                uint32_t(REQUEST_TYPE_INVERTER)
            );

            requestData(
                _IPAddress,
                _Port,
                _MeterData.startAddress,
                _MeterData.dataLength,
                uint32_t(REQUEST_TYPE_METER)
            );
        }
    }
}

void Inverter::end() {
    _modbusClient.end();
}

void Inverter::handleMeterData(ModbusMessage response, uint32_t token) {
    ESP_LOGI("INVERTER", "Received meter data, token=%08X", token);
    uint16_t idx_ = 3; // Modbus data starts at index 3

    _MeterData.meterStatus = getU16(response, idx_); idx_ += 2;
    _MeterData.gridVoltageA = getI32(response, idx_) / 10.0f; idx_ += 4;
    _MeterData.gridVoltageB = getI32(response, idx_) / 10.0f; idx_ += 4;
    _MeterData.gridVoltageC = getI32(response, idx_) / 10.0f; idx_ += 4;
    _MeterData.gridCurrentA = getI32(response, idx_) / 100.0f; idx_ += 4;
    _MeterData.gridCurrentB = getI32(response, idx_) / 100.0f; idx_ += 4;
    _MeterData.gridCurrentC = getI32(response, idx_) / 100.0f; idx_ += 4;
    _MeterData.activePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    _MeterData.reactivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    _MeterData.powerFactor = getU16(response, idx_) / 1000.0f; idx_ += 2;
    _MeterData.gridFrequency = getU16(response, idx_) / 100.0f; idx_ += 2;
    _MeterData.positiveActiveElectricity = getI32(response, idx_) / 100.0f; idx_ += 4;
    _MeterData.reverseActivePower = getI32(response, idx_) / 100.0f; idx_ += 4;
    _MeterData.accumulatedReactivePower = getI32(response, idx_) / 100.0f; idx_ += 4;
    _MeterData.meterType = getU16(response, idx_); idx_ += 2;
    _MeterData.abLineVoltage = getI32(response, idx_) / 10.0f; idx_ += 4;
    _MeterData.bcLineVoltage = getI32(response, idx_) / 10.0f; idx_ += 4;
    _MeterData.caLineVoltage = getI32(response, idx_) / 10.0f; idx_ += 4;
    _MeterData.aPhaseActivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    _MeterData.bPhaseActivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    _MeterData.cPhaseActivePower = getI32(response, idx_) * 1.0f; idx_ += 4;
    _MeterData.meterModelDetection = getU16(response, idx_);

    ESP_LOGI("INVERTER", "    Meter voltages: A=%.1f V, B=%.1f V, C=%.1f V",
        _MeterData.gridVoltageA,
        _MeterData.gridVoltageB,
        _MeterData.gridVoltageC);

    ESP_LOGI("INVERTER", "    Meter currents: A=%.3f A, B=%.3f A, C=%.3f A",
        _MeterData.gridCurrentA,
        _MeterData.gridCurrentB,
        _MeterData.gridCurrentC);

    ESP_LOGI("INVERTER", "    Meter active power: %.1f W", _MeterData.activePower);
    ESP_LOGI("INVERTER", "    Meter reactive power: %.1f Var", _MeterData.reactivePower);
    ESP_LOGI("INVERTER", "    Meter frequency: %.2f Hz", _MeterData.gridFrequency);
    ESP_LOGI("INVERTER", "    Meter power factor: %.3f", _MeterData.powerFactor);
}

void Inverter::handlePowerData(ModbusMessage response, uint32_t token) {
    ESP_LOGI("INVERTER", "Received inverter power data, token=%08X", token);
    uint16_t idx_ = 3;
    _PowerData.inputPower = getI32(response, idx_) / 1000.0f; idx_ += 4;
    _PowerData.gridVoltageAB = getU16(response, idx_) / 10.0f;   idx_ += 2;
    _PowerData.gridVoltageBC = getU16(response, idx_) / 10.0f;   idx_ += 2;
    _PowerData.gridVoltageCA = getU16(response, idx_) / 10.0f;   idx_ += 2;
    _PowerData.phaseAVoltage = getU16(response, idx_) / 10.0f;   idx_ += 2;
    _PowerData.phaseBVoltage = getU16(response, idx_) / 10.0f;   idx_ += 2;
    _PowerData.phaseCVoltage = getU16(response, idx_) / 10.0f;   idx_ += 2;
    _PowerData.gridCurrentA = getI32(response, idx_) / 1000.0f; idx_ += 4;
    _PowerData.gridCurrentB = getI32(response, idx_) / 1000.0f; idx_ += 4;
    _PowerData.gridCurrentC = getI32(response, idx_) / 1000.0f; idx_ += 4;
    _PowerData.peakActivePowerDay = getI32(response, idx_) / 1000.0f; idx_ += 4;
    _PowerData.activePower = getI32(response, idx_) / 1000.0f; idx_ += 4;
    _PowerData.reactivePower = getI32(response, idx_) / 1000.0f; idx_ += 4;
    _PowerData.powerFactor = getU16(response, idx_) / 1000.0f;

    ESP_LOGI("INVERTER", "    Grid currents: A=%.3f A, B=%.3f A, C=%.3f A",
        _PowerData.gridCurrentA,
        _PowerData.gridCurrentB,
        _PowerData.gridCurrentC);

    ESP_LOGI("INVERTER", "    Grid voltages: A=%.1f V, B=%.1f V, C=%.1f V",
        _PowerData.phaseAVoltage,
        _PowerData.phaseBVoltage,
        _PowerData.phaseCVoltage);

    ESP_LOGI("INVERTER", "    Inverter input power: %.3f kW", _PowerData.inputPower);
}

void Inverter::handleStatusData(ModbusMessage response, uint32_t token) {
    ESP_LOGI("INVERTER", "Received inverter status data, token=%08X", token);
    uint16_t idx_ = 3;

    _StatusData.state1 = getU16(response, idx_); idx_ += 2;
    _StatusData.state2 = getU16(response, idx_); idx_ += 2;
    _StatusData.state3 = ((uint32_t)getU16(response, idx_) << 16) | getU16(response, idx_ + 2); idx_ += 4;
    _StatusData.alarm1 = getU16(response, idx_); idx_ += 2;
    _StatusData.alarm2 = getU16(response, idx_); idx_ += 2;
    _StatusData.alarm3 = getU16(response, idx_);

    ESP_LOGI("INVERTER", "    Inverter states: state1=0x%04X, state2=0x%04X, state3=0x%08X",
        _StatusData.state1,
        _StatusData.state2,
        _StatusData.state3);

    ESP_LOGI("INVERTER", "    Inverter alarms: alarm1=0x%04X, alarm2=0x%04X, alarm3=0x%04X",
        _StatusData.alarm1,
        _StatusData.alarm2,
        _StatusData.alarm3);
}

void Inverter::handleData(ModbusMessage response, uint32_t token) {
    switch (token) {
    case REQUEST_TYPE_INVERTER:
        handlePowerData(response, token);
        break;
    case REQUEST_TYPE_METER:
        handleMeterData(response, token);
        break;
    case REQUEST_TYPE_STATUS:
        handleStatusData(response, token);
        break;
    default:
        ESP_LOGW("INVERTER", "Unknown REQUEST_TYPE!");
        break;
    }
}

void Inverter::handleError(Error error, uint32_t token) {
    ModbusError me_(error);
    ESP_LOGE("INVERTER", "Error response: %02X - %s", (int)me_, (const char*)me_);
}

void Inverter::requestData(const String& ipAddress, uint16_t port, uint16_t startaddress, uint16_t registerCount, uint32_t token) {
    IPAddress host_;
    if (!host_.fromString(ipAddress)) {
        ESP_LOGE("INVERTER", "Invalid IP address: %s", ipAddress.c_str());
        return;
    }
    _modbusClient.setTarget(host_, port);
    uint8_t slave_ = 1;

    Error err_ = _modbusClient.addRequest(token, slave_, READ_HOLD_REGISTER, startaddress, registerCount);
    if (err_ != SUCCESS) {
        ModbusError e_(err_);
        ESP_LOGE("INVERTER", "Error creating request: %02X - %s", (int)e_, (const char*)e_);
    }
    else {
        ESP_LOGI("INVERTER", "Request sent to inverter at %s:%d, start address=%d, count=%d, token=%08X",
            ip.c_str(), port, startaddress, registerCount, token);
    }
}
