/**
 * Author: sascha_lammers@gmx.de
 */

#pragma once

#if IOT_SENSOR && (IOT_SENSOR_HAVE_HLW8012 || IOT_SENSOR_HAVE_HLW8032)

// support for
// HLW8012 (and SSP1837, CSE7759, ...)
// HLW8032 (and CSE7759B, ...)

#include <Arduino_compat.h>
#include "WebUIComponent.h"
#include "plugins.h"
#include "MQTTSensor.h"

// voltage divider for V2P
#ifndef IOT_SENSOR_HLW80xx_V_RES_DIV
#define IOT_SENSOR_HLW80xx_V_RES_DIV            ((4 * 470) / 1.0)               // 4x470K : 1K
#endif

// current shunt resistance
#ifndef IOT_SENSOR_HLW80xx_SHUNT
#define IOT_SENSOR_HLW80xx_SHUNT                0.001
#endif

// update rate webui and MQTT
#ifndef IOT_SENSOR_HLW80xx_UPDATE_RATE
#define IOT_SENSOR_HLW80xx_UPDATE_RATE          2
#endif

// // update rate MQTT
// #ifndef IOT_SENSOR_HLW80xx_UPDATE_RATE_MQTT
// #define IOT_SENSOR_HLW80xx_UPDATE_RATE_MQTT     30
// #endif

// internal voltage reference
#ifndef IOT_SENSOR_HLW80xx_VREF
#define IOT_SENSOR_HLW80xx_VREF                 2.43
#endif

// oscillator frequency in MHz
#ifndef IOT_SENSOR_HLW80xx_F_OSC
#define IOT_SENSOR_HLW80xx_F_OSC                3.579000
#endif

// interval to save energy counter to SPIFFS, 0 to disable
#ifndef IOT_SENSOR_HLW80xx_SAVE_ENERGY_CNT
#define IOT_SENSOR_HLW80xx_SAVE_ENERGY_CNT      (5 * 60 * 1000)
#endif

// number of energy counters, must be >= 2
#ifndef IOT_SENSOR_HLW80xx_NUM_ENERGY_COUNTERS
#define IOT_SENSOR_HLW80xx_NUM_ENERGY_COUNTERS  5
#endif

// https://datasheet.lcsc.com/szlcsc/1811151452_Hiliwei-Tech-HLW8012_C83804.pdf

// Fcf = (((v1 * v2) * 48) / (Vref * Vref)) * (fosc / 128), v1 = (current * Rshunt), v2 = (voltage / Rdivider), power = voltage * current, Fcf = 1 / (FcfDutyCycleUs * 2 / 1000000)
//  (current * Rshunt) * (voltage / Rdivider) = (4000000 * Vref * Vref) / (FcfDutyCycleUs * 3 * fosc)
//  voltage = (4000000 * Rdivider * Vref * Vref) / (3 * FcfDutyCycleUs * Rshunt * fosc * current), voltage = power / current
//  power = (4000000 * Rdivider * Vref * Vref) / (3 * FcfDutyCycleUs * Rshunt * fosc)

// Fcfi = ((v1 * 24) / Vref) * (fosc / 512), v1 = current * Rshunt
//  1 / (FcfiDutyCycleUs * 2 / 1000000) = (((current * Rshunt) * 24) / Vref) * (fosc / 512)
//  current = (32000000 * Vref) / (FcfiDutyCycleUs * 3 * Rshunt * fosc)

// Fcfu = ((v2 * 2) / Vref) * (fosc / 512), v2 = voltage / Rdivider
//  1 / (FcfuDutyCycleUs * 2 / 1000000) = ((voltage / Rdivider * 2) / Vref) * (fosc / 512)
//  voltage = (128000000 * Vref * Rdivider) / (FcfuDutyCycleUs * fosc)

// v1 = voltage @ v1p/v1n
// v2 = voltage @ v2p
// fosc = 3.579Mhz
// Vref = 2.43V

// pulse is the duty cycle in µs (50% PWM)
#define IOT_SENSOR_HLW80xx_CALC_U(pulse)        ((128.000000 * IOT_SENSOR_HLW80xx_VREF * IOT_SENSOR_HLW80xx_V_RES_DIV * _calibrationV) / (pulse * IOT_SENSOR_HLW80xx_F_OSC))
#define IOT_SENSOR_HLW80xx_CALC_I(pulse)        ((32.000000 * IOT_SENSOR_HLW80xx_VREF) / (pulse * (3 * IOT_SENSOR_HLW80xx_F_OSC * IOT_SENSOR_HLW80xx_SHUNT * _calibrationI)))
#define IOT_SENSOR_HLW80xx_CALC_P(pulse)        ((4.000000 * IOT_SENSOR_HLW80xx_V_RES_DIV * IOT_SENSOR_HLW80xx_VREF * IOT_SENSOR_HLW80xx_VREF * _calibrationP) / (3 * pulse * IOT_SENSOR_HLW80xx_SHUNT * _calibrationI * IOT_SENSOR_HLW80xx_F_OSC))
  // count is incremented on falling and raising edge
#define IOT_SENSOR_HLW80xx_PULSE_TO_KWH(count)  (count * IOT_SENSOR_HLW80xx_CALC_P(1000000.0) / (1000.0 * 3600.0))

class Sensor_HLW80xx : public MQTTSensor {
public:
    Sensor_HLW80xx(const String &name);

    virtual void createAutoDiscovery(MQTTAutoDiscovery::Format_t format, MQTTAutoDiscoveryVector &vector) override;
    virtual uint8_t getAutoDiscoveryCount() const override;

    virtual void publishState(MQTTClient *client) override;
    virtual void getValues(JsonArray &json) override;
    virtual void createWebUI(WebUI &webUI, WebUIRow **row) override;

    virtual String _getId(const __FlashStringHelper *type = nullptr) {
        return String();
    }

    virtual void restart() override;

    void resetEnergyCounter();
    uint64_t *getEnergyCounters() {
        return _energyCounter;
    }
    uint64_t &getEnergyPrimaryCounter() {
        return _energyCounter[0];
    }
    uint64_t &getEnergySecondaryCounter() {
        return _energyCounter[1];
    }

protected:
    void _saveEnergyCounter();
    void _loadEnergyCounter();
    void _incrEnergyCounters(uint32_t count);

    JsonNumber _currentToNumber(float current) const;
    JsonNumber _energyToNumber(float energy) const;
    JsonNumber _powerToNumber(float power) const;

    float _getPowerFactor() const;
    float _getEnergy(uint8_t num = 0) const;

    String _name;
    String _topic;
    float _power;
    float _voltage;
    float _current;

    float _calibrationI;
    float _calibrationV;
    float _calibrationP;

    unsigned long _saveEnergyCounterTimeout;

public:
    uint64_t _energyCounter[IOT_SENSOR_HLW80xx_NUM_ENERGY_COUNTERS];
};

#endif
