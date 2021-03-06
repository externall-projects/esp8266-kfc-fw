/**
 * Author: sascha_lammers@gmx.de
 */

#pragma once

#if IOT_SENSOR_HAVE_INA219

#include <Arduino_compat.h>
#include <Adafruit_INA219.h>
#include <Wire.h>
#include "WebUIComponent.h"
#include "plugins.h"
#include "MQTTSensor.h"

#ifndef IOT_SENSOR_INA219_R_SHUNT
// the adafruit breakout board uses 0.1R
#define IOT_SENSOR_INA219_R_SHUNT           0.1
#endif

#ifndef IOT_SENSOR_INA219_GAIN
// maximum gain, 3.2A @ 0.1R
#define IOT_SENSOR_INA219_GAIN              INA219_CONFIG_GAIN_8_320MV
#endif

#ifndef IOT_SENSOR_INA219_BUS_URANGE
// bus voltage 32 = max. 26V
#define IOT_SENSOR_INA219_BUS_URANGE        INA219_CONFIG_BVOLTAGERANGE_32V
#endif

#ifndef IOT_SENSOR_INA219_SHUNT_ADC_RES
// average 128x 12bit samples
#define IOT_SENSOR_INA219_SHUNT_ADC_RES     INA219_CONFIG_SADCRES_12BIT_128S_69MS
#endif

#ifndef IOT_SENSOR_INA219_READ_INTERVAL
// should close to the sample/averaging rate
#define IOT_SENSOR_INA219_READ_INTERVAL     75
#endif

#ifndef IOT_SENSOR_INA219_PEAK_HOLD_TIME
// time in seconds until the peak current is reset
#define IOT_SENSOR_INA219_PEAK_HOLD_TIME    60
#endif

// webui update rate in seconds
#ifndef IN219_WEBUI_UPDATE_RATE
#define IN219_WEBUI_UPDATE_RATE             5
#endif

class Sensor_INA219 : public MQTTSensor {
public:
    typedef enum : char {
        VOLTAGE =       'u',
        CURRENT =       'i',
        POWER =         'p',
        PEAK_CURRENT =  'm',
    } SensorTypeEnum_t;

    Sensor_INA219(const JsonString &name, TwoWire &wire, uint8_t address = IOT_SENSOR_HAVE_INA219);
    virtual ~Sensor_INA219();

    virtual void createAutoDiscovery(MQTTAutoDiscovery::Format_t format, MQTTAutoDiscoveryVector &vector) override;
    virtual uint8_t getAutoDiscoveryCount() const override;

    virtual void publishState(MQTTClient *client) override;
    virtual void getValues(JsonArray &json, bool timer) override;
    virtual void createWebUI(WebUI &webUI, WebUIRow **row) override;
    virtual void getStatus(PrintHtmlEntitiesString &output) override;
    virtual MQTTSensorSensorType getType() const override;

#if AT_MODE_SUPPORTED
    virtual void atModeHelpGenerator() override;
    virtual bool atModeHandler(AtModeArgs &args) override;
#endif

public:
    Adafruit_INA219 &getSensor() {
        return _ina219;
    }

    // average values
    float getVoltage() const {
        return _data.U();
    }
    float getCurrent() const {
        return _data.I();
    }
    float getPower() const {
        return _data.P();
    }
    float getPeakCurrent() const {
        return _Ipeak;
    }

private:
    class SensorData {
    public:
        SensorData() : _V(0), _I(0), _count(0) {
        }

        float U() const {
            return _count ? (_V / _count) : NAN;
        }

        float I() const {
            return _count ? (_I / _count) : NAN;
        }

        float P() const {
            return _count ? U() * I() : NAN;
        }

        void add(float U, float I) {
            _V += U;
            _I += I;
            _count++;
        }

        void set(float U, float I) {
            _V = U;
            _I = I;
            _count = 1;
        }

    private:
        float _V;
        float _I;
        size_t _count;
    };

    void _loop();
    String _getId(SensorTypeEnum_t type);

    JsonString _name;
    uint8_t _address;

    uint32_t _updateTimer;
    uint32_t _holdPeakTimer;
    SensorData _data;
    SensorData _mqttData;
    float _Ipeak;

    Adafruit_INA219 _ina219;
};

#endif
