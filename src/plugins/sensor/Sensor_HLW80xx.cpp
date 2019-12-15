/**
 * Author: sascha_lammers@gmx.de
 */

#if IOT_SENSOR && (IOT_SENSOR_HAVE_HLW8012 || IOT_SENSOR_HAVE_HLW8032)

#include "Sensor_HLW80xx.h"
#include "sensor.h"
#include "MicrosTimer.h"

#if DEBUG_IOT_SENSOR
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

Sensor_HLW80xx::Sensor_HLW80xx(const String &name) : MQTTSensor(), _name(name) {
#if DEBUG_MQTT_CLIENT
    debug_printf_P(PSTR("Sensor_HLW80xx(): component=%p\n"), this);
#endif
    _topic = MQTTClient::formatTopic(-1, F("/%s/"), _getId().c_str());
    _energyCounter = 0;
    _power = NAN;
    _voltage = NAN;
    _current = NAN;
    setUpdateRate(IOT_SENSOR_HLW80xx_UPDATE_RATE);
}

void Sensor_HLW80xx::createAutoDiscovery(MQTTAutoDiscovery::Format_t format, MQTTAutoDiscoveryVector &vector) {
    auto discovery = _debug_new MQTTAutoDiscovery();
    discovery->create(this, 0, format);
    discovery->addStateTopic(_topic);
    discovery->addUnitOfMeasurement(F("W"));
    discovery->addValueTemplate(F("power"));
    discovery->finalize();
    vector.emplace_back(MQTTAutoDiscoveryPtr(discovery));

    discovery = _debug_new MQTTAutoDiscovery();
    discovery->create(this, 1, format);
    discovery->addStateTopic(_topic);
    discovery->addUnitOfMeasurement(F("kWh"));
    discovery->addValueTemplate(F("energy"));
    discovery->finalize();
    vector.emplace_back(MQTTAutoDiscoveryPtr(discovery));

    discovery = _debug_new MQTTAutoDiscovery();
    discovery->create(this, 2, format);
    discovery->addStateTopic(_topic);
    discovery->addUnitOfMeasurement(F("V"));
    discovery->addValueTemplate(F("voltage"));
    discovery->finalize();
    vector.emplace_back(MQTTAutoDiscoveryPtr(discovery));

    discovery = _debug_new MQTTAutoDiscovery();
    discovery->create(this, 3, format);
    discovery->addStateTopic(_topic);
    discovery->addUnitOfMeasurement(F("A"));
    discovery->addValueTemplate(F("current"));
    discovery->finalize();
    vector.emplace_back(MQTTAutoDiscoveryPtr(discovery));

    discovery = _debug_new MQTTAutoDiscovery();
    discovery->create(this, 4, format);
    discovery->addStateTopic(_topic);
    discovery->addUnitOfMeasurement(F(""));
    discovery->addValueTemplate(F("pf"));
    discovery->finalize();
    vector.emplace_back(MQTTAutoDiscoveryPtr(discovery));
}

uint8_t Sensor_HLW80xx::getAutoDiscoveryCount() const {
    return 5;
}

void Sensor_HLW80xx::getValues(JsonArray &array) {
    _debug_printf_P(PSTR("Sensor_HLW8012::getValues()\n"));

    auto obj = &array.addObject(3);
    obj->add(JJ(id), _getId(F("power")));
    obj->add(JJ(state), !isnan(_power));
    obj->add(JJ(value), _powerToNumber(_power));

    obj = &array.addObject(3);
    obj->add(JJ(id), _getId(F("energy")));
    auto energy = _getEnergy();
    obj->add(JJ(state), !isnan(energy));
    obj->add(JJ(value), _energyToNumber(energy));

    obj = &array.addObject(3);
    obj->add(JJ(id), _getId(F("voltage")));
    obj->add(JJ(state), !isnan(_voltage));
    obj->add(JJ(value), JsonNumber(_voltage, 1));

    obj = &array.addObject(3);
    obj->add(JJ(id), _getId(F("current")));
    obj->add(JJ(state), !isnan(_current));
    obj->add(JJ(value), _currentToNumber(_current));

    obj = &array.addObject(3);
    obj->add(JJ(id), _getId(F("pf")));
    auto pf = _getPowerFactor();
    obj->add(JJ(state), !isnan(pf));
    obj->add(JJ(value), String(pf, 2));
}

void Sensor_HLW80xx::createWebUI(WebUI &webUI, WebUIRow **row) {
    _debug_printf_P(PSTR("Sensor_HLW8012::createWebUI()\n"));

    if ((*row)->size() > 1) {
        // *row = &webUI.addRow();
    }

    (*row)->addSensor(_getId(F("power")), _name + F(" Power"), F("W"));
    (*row)->addSensor(_getId(F("energy")), _name + F(" Energy"), F("kWh"));
    (*row)->addSensor(_getId(F("voltage")), _name + F(" Voltage"), F("V"));
    (*row)->addSensor(_getId(F("current")), _name + F(" Current"), F("A"));
    (*row)->addSensor(_getId(F("pf")), _name + F(" Power Factor"), F(""));
}

void Sensor_HLW80xx::publishState(MQTTClient *client) {
    if (client) {
        PrintString str;
        JsonUnnamedObject json;
        json.add(F("power"), _powerToNumber(_power));
        json.add(F("energy"), _energyToNumber(_getEnergy()));
        json.add(F("voltage"), JsonNumber(_voltage, 1));
        json.add(F("current"), _currentToNumber(_current));
        auto pf = _getPowerFactor();
        json.add(F("pf"), String(pf, 2));
        json.printTo(str);
        client->publish(_topic, _qos, 1, str);
    }
}

JsonNumber Sensor_HLW80xx::_currentToNumber(float current) const {
    if (current < 0.2) {
        return JsonNumber(current, 3);
    }
    return JsonNumber(current, 2);
}

JsonNumber Sensor_HLW80xx::_energyToNumber(float energy) const {
    auto tmp = energy;
    uint8_t digits = 0;
    while(tmp >= 1 && digits < 3) {
        digits++;
        tmp *= 0.1;
    }
    return JsonNumber(energy, 3 - digits);
}

JsonNumber Sensor_HLW80xx::_powerToNumber(float power) const {
    if (power < 10) {
        return JsonNumber(power, 2);
    }
    return JsonNumber(power, 1);
}

float Sensor_HLW80xx::_getPowerFactor() const {
    if (isnan(_power) || isnan(_voltage) || isnan(_current)) {
        return NAN;
    }
    return std::min(_power / (_voltage * _current), 1.0f);
}

float Sensor_HLW80xx::_getEnergy() const {
    return IOT_SENSOR_HLW80xx_PULSE_TO_KWH(_energyCounter);
}

#endif