// inverterhandling.h

#ifndef _INVERTERHANDLING_h
#define _INVERTERHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ModbusClientTCP.h>


// startet mit Adresse 37100, Laenge 39
struct MeterData {
    int startAddress = 37100;
    int dataLength = 39;
    uint16_t meterStatus;              // 37100, 1 Reg, UINT16, Meter status (0: offline, 1: normal)
    float gridVoltageA;                // 37101, 2 Reg, V
    float gridVoltageB;                // 37103, 2 Reg, V
    float gridVoltageC;                // 37105, 2 Reg, V
    float gridCurrentA;                // 37107, 2 Reg, A
    float gridCurrentB;                // 37109, 2 Reg, A
    float gridCurrentC;                // 37111, 2 Reg, A
    float activePower;                 // 37113, 2 Reg, W
    float reactivePower;               // 37115, 2 Reg, Var
    float powerFactor;                 // 37117, 1 Reg, NA
    float gridFrequency;               // 37118, 1 Reg, Hz
    float positiveActiveElectricity;   // 37119, 2 Reg, kWh
    float reverseActivePower;          // 37121, 2 Reg, kWh
    float accumulatedReactivePower;    // 37123, 2 Reg, kVarh
    uint16_t meterType;                // 37125, 1 Reg, UINT16, Meter type (0: singlephase, 1: threephase)
    float abLineVoltage;               // 37126, 2 Reg, V
    float bcLineVoltage;               // 37128, 2 Reg, V
    float caLineVoltage;               // 37130, 2 Reg, V
    float aPhaseActivePower;           // 37132, 2 Reg, W
    float bPhaseActivePower;           // 37134, 2 Reg, W
    float cPhaseActivePower;           // 37136, 2 Reg, W
    uint16_t meterModelDetection;      // 37138, 1 Reg, UINT16, Meter model detection result
};

// startet mit Adresse 32064, Laenge 21
struct PowerData {
	int startAddress = 32064;
	int dataLength = 21;
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

struct StatusData {
	// startet mit Adresse 32000, Laenge 7
	int startAddress = 32000;
	int dataLength = 7;
    uint16_t state1;      // 32000, 1 Reg, Bitfield16
    uint16_t state2;      // 32002, 1 Reg, Bitfield16
    uint32_t state3;      // 32003, 2 Reg, Bitfield32
    uint16_t alarm1;      // 32008, 1 Reg, Bitfield16
    uint16_t alarm2;      // 32009, 1 Reg, Bitfield16
    uint16_t alarm3;      // 32010, 1 Reg, Bitfield16

    bool isStandby() const { return state1 & (1 << 0); }
    bool isGridConnected() const { return state1 & (1 << 1); }
    bool isGridConnectedNormally() const { return state1 & (1 << 2); }
    bool isDeratingPowerRationing() const { return state1 & (1 << 3); }
    bool isDeratingInternal() const { return state1 & (1 << 4); }
    bool isNormalStop() const { return state1 & (1 << 5); }
    bool isFaultStop() const { return state1 & (1 << 6); }
    bool isPowerRationingStop() const { return state1 & (1 << 7); }
    bool isShutdown() const { return state1 & (1 << 8); }
    bool isSpotCheck() const { return state1 & (1 << 9); }
};

class Inverter {
public:
    Inverter(WiFiClient& client);

    void begin(const String& ipAddress, const uint16_t port = 502, const uint8_t interval = 10);
    void process();
    void end();

    PowerData getPowerData() const { return _PowerData; };
	StatusData getStatusData() const { return _StatusData; };
	MeterData getMeterData() const { return _MeterData; };


private:
    ModbusClientTCP _modbusClient;
    Neotimer _intervalTimer;
    enum REQUEST_TYPE { 
        REQUEST_TYPE_INVERTER = 1001, 
        REQUEST_TYPE_METER = 1002, 
        REQUEST_TYPE_STATUS = 1003 
    };
    
    struct RequestContext {
        REQUEST_TYPE type;
        uint32_t token;
    };
    RequestContext _lastRequest;

    PowerData _PowerData;
    StatusData _StatusData;
    MeterData _MeterData;

    char _IPAddress[15] = "0.0.0.0";
    int _Port = 502;

    void handleMeterData(ModbusMessage response, uint32_t token);
    void handlePowerData(ModbusMessage response, uint32_t token);
    void handleStatusData(ModbusMessage response, uint32_t token);
    void handleData(ModbusMessage response, uint32_t token);
    void handleError(Error error, uint32_t token);
    void requestData(const String& ipAddress, uint16_t port, uint16_t startaddress, uint16_t registerCount, uint32_t token);
};
extern Inverter inverter;

#endif