// inverterhandling.h

#ifndef _INVERTERHANDLING_h
#define _INVERTERHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


// startet mit Adresse 37100, Laenge 39
struct MeterData {
    uint16_t meterStatus;              // 37100, 1 Reg, UINT16, Meter status (0: offline, 1: normal)
    int32_t gridVoltageA;              // 37101, 2 Reg, INT32, V, Gain 10
    int32_t gridVoltageB;              // 37103, 2 Reg, INT32, V, Gain 10
    int32_t gridVoltageC;              // 37105, 2 Reg, INT32, V, Gain 10
    int32_t gridCurrentA;              // 37107, 2 Reg, INT32, A, Gain 100
    int32_t gridCurrentB;              // 37109, 2 Reg, INT32, A, Gain 100
    int32_t gridCurrentC;              // 37111, 2 Reg, INT32, A, Gain 100
    int32_t activePower;               // 37113, 2 Reg, INT32, W, Gain 1
    int32_t reactivePower;             // 37115, 2 Reg, INT32, Var, Gain 1
    int16_t powerFactor;               // 37117, 1 Reg, INT16, NA, Gain 1000
    int16_t gridFrequency;             // 37118, 1 Reg, INT16, Hz, Gain 100
    int32_t positiveActiveElectricity; // 37119, 2 Reg, INT32, kWh, Gain 100
    int32_t reverseActivePower;        // 37121, 2 Reg, INT32, kWh, Gain 100
    int32_t accumulatedReactivePower;  // 37123, 2 Reg, INT32, kVarh, Gain 100
    uint16_t meterType;                // 37125, 1 Reg, UINT16, Meter type (0: singlephase, 1: threephase)
    int32_t abLineVoltage;             // 37126, 2 Reg, INT32, V, Gain 10
    int32_t bcLineVoltage;             // 37128, 2 Reg, INT32, V, Gain 10
    int32_t caLineVoltage;             // 37130, 2 Reg, INT32, V, Gain 10
    int32_t aPhaseActivePower;         // 37132, 2 Reg, INT32, W, Gain 1
    int32_t bPhaseActivePower;         // 37134, 2 Reg, INT32, W, Gain 1
    int32_t cPhaseActivePower;         // 37136, 2 Reg, INT32, W, Gain 1
    uint16_t meterModelDetection;      // 37138, 1 Reg, UINT16, Meter model detection result
};
extern MeterData meterData;

// startet mit Adresse 32064, Laenge 21
struct InverterPowerData {
    float inputPower;           // 32064, 2 Reg, kW
    float gridVoltageAB;        // 32066, 1 Reg, V
    float gridVoltageBC;        // 32067, 1 Reg, V
    float gridVoltageCA;        // 32068, 1 Reg, V
    float phaseAVoltage;        // 32069, 1 Reg, V
    float phaseBVoltage;        // 32070, 1 Reg, V
    float phaseCVoltage;        // 32071, 1 Reg, V
    float gridCurrentA;         // 32072, 2 Reg, A
    float gridCurrentB;         // 32074, 2 Reg, A
    float gridCurrentC;         // 32076, 2 Reg, A
    float peakActivePowerDay;   // 32078, 2 Reg, kW
    float activePower;          // 32080, 2 Reg, kW
    float reactivePower;        // 32082, 2 Reg, kVar
    float powerFactor;          // 32084, 1 Reg, NA
};
extern InverterPowerData inverterPowerData;

struct InverterStatusData {
    uint16_t state1;      // 32000, 1 Reg, Bitfield16
    uint16_t state2;      // 32002, 1 Reg, Bitfield16
    uint32_t state3;      // 32003, 2 Reg, Bitfield32
    uint16_t alarm1;      // 32008, 1 Reg, Bitfield16
    uint16_t alarm2;      // 32009, 1 Reg, Bitfield16
    uint16_t alarm3;      // 32010, 1 Reg, Bitfield16
};
extern InverterStatusData inverterStatusData;


void setupInverter();
void loopInverter();
void requestInverter(String ip, uint16_t port, uint16_t startaddress, uint16_t number);

#endif