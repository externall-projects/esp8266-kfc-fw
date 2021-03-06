/**
 * Author: sascha_lammers@gmx.de
 */

#pragma once

#if IOT_SENSOR_HAVE_LM75A

#include <Arduino_compat.h>
#include <Wire.h>
#include <vector>
#include "WebUIComponent.h"
#include "plugins.h"
#include "MQTTSensor.h"

class Sensor_CCS811;

class Sensor_LM75A : public MQTTSensor {
public:
    Sensor_LM75A(const JsonString &name, TwoWire &wire, uint8_t address = 0x48);

    virtual void createAutoDiscovery(MQTTAutoDiscovery::Format_t format, MQTTAutoDiscoveryVector &vector) override;
    virtual uint8_t getAutoDiscoveryCount() const override;

    virtual void publishState(MQTTClient *client) override;
    virtual void getValues(JsonArray &json, bool timer) override;
    virtual void createWebUI(WebUI &webUI, WebUIRow **row) override;
    virtual void getStatus(PrintHtmlEntitiesString &output) override;
    virtual MQTTSensorSensorType getType() const override;

    float readSensor() {
        return _readSensor();
    }

private:
    friend Sensor_CCS811;

    String _getId();
    float _readSensor();

    JsonString _name;
    TwoWire &_wire;
    uint8_t _address;
};

#endif
