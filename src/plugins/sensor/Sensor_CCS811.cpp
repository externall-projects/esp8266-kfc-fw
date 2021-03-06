/**
 * Author: sascha_lammers@gmx.de
 */

#if IOT_SENSOR_HAVE_CCS811

#include "Sensor_CCS811.h"
#include "Sensor_LM75A.h"
#include "Sensor_BME280.h"
#include "Sensor_BME680.h"
#include "sensor.h"

#if DEBUG_IOT_SENSOR
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

Sensor_CCS811::Sensor_CCS811(const String &name, uint8_t address) : MQTTSensor(), _name(name), _address(address)
{
    REGISTER_SENSOR_CLIENT(this);
    config.initTwoWire();
    _ccs811.beg
    setUpdateRate(10); // faster update rate until valid data is available
    _sensor.eCO2 = 0;
    _sensor.TVOC = 0;
    _sensor.available = false;
}

void Sensor_CCS811::createAutoDiscovery(MQTTAutoDiscovery::Format_t format, MQTTAutoDiscoveryVector &vector)
{
    String topic = MQTTClient::formatTopic(-1, F("/%s/"), _getId().c_str());

    auto discovery = new MQTTAutoDiscovery();
    discovery->create(this, 0, format);
    discovery->addStateTopic(topic);
    discovery->addUnitOfMeasurement(F("ppm"));
    discovery->addValueTemplate(F("eCO2"));
    discovery->finalize();
    vector.emplace_back(discovery);

    discovery = new MQTTAutoDiscovery();
    discovery->create(this, 1, format);
    discovery->addStateTopic(topic);
    discovery->addUnitOfMeasurement(F("ppb"));
    discovery->addValueTemplate(F("TVOC"));
    discovery->finalize();
    vector.emplace_back(discovery);
}

uint8_t Sensor_CCS811::getAutoDiscoveryCount() const
{
    return 2;
}

void Sensor_CCS811::getValues(JsonArray &array, bool timer)
{
    _debug_printf_P(PSTR("Sensor_CCS811::getValues()\n"));

    auto &sensor = _readSensor();
    auto obj = &array.addObject(3);
    obj->add(JJ(id), _getId(F("eco2")));
    obj->add(JJ(state), sensor.available);
    obj->add(JJ(value), sensor.eCO2);
    obj = &array.addObject(3);
    obj->add(JJ(id), _getId(F("tvoc")));
    obj->add(JJ(state), sensor.available);
    obj->add(JJ(value), sensor.TVOC);
}

void Sensor_CCS811::createWebUI(WebUI &webUI, WebUIRow **row)
{
    _debug_printf_P(PSTR("Sensor_CCS811::createWebUI()\n"));

    if ((*row)->size() > 2) {
        // *row = &webUI.addRow();
    }

    (*row)->addSensor(_getId(F("eco2")), _name + F(" CO2"), F("ppm"));
    (*row)->addSensor(_getId(F("tvoc")), _name + F(" TVOC"), F("ppb"));
}

void Sensor_CCS811::getStatus(PrintHtmlEntitiesString &output)
{
    output.printf_P(PSTR("CCS811 @ I2C address 0x%02x" HTML_S(br)), _address);
}

MQTTSensorSensorType Sensor_CCS811::getType() const
{
    return MQTTSensorSensorType::CCS811;
}

void Sensor_CCS811::publishState(MQTTClient *client)
{
    if (client) {
        auto &sensor = _readSensor();
        if (sensor.available) {
            PrintString str;
            JsonUnnamedObject json;
            json.add(F("eCO2"), sensor.eCO2);
            json.add(F("TVOC"), sensor.TVOC);
            json.printTo(str);

            client->publish(MQTTClient::formatTopic(-1, F("/%s/"), _getId().c_str()), _qos, 1, str);
        }
    }
}

Sensor_CCS811::SensorData_t &Sensor_CCS811::_readSensor()
{
// use temperature and humidity for compensation
#if IOT_SENSOR_HAVE_LM75A || IOT_SENSOR_HAVE_BME280 || IOT_SENSOR_HAVE_BME680
    auto &sensors = SensorPlugin::getSensors();
    for(auto sensor: sensors) {
#if IOT_SENSOR_HAVE_BME280
        if (sensor->getType() == BME280) {
            auto data = reinterpret_cast<Sensor_BME280 *>(sensor)->_readSensor();
            _debug_printf_P(PSTR("Sensor_CCS811::_readSensor(): setEnvironmentalData(): BME280: humidity %u, temperature %.3f\n"), (uint8_t)data.humidity, data.temperature);
            _ccs811.setEnvironmentalData(data.humidity, data.temperature);
        }
#elif IOT_SENSOR_HAVE_BME680
        if (sensor->getType() == BME680) {
            auto data = reinterpret_cast<Sensor_BME680 *>(sensor)->_readSensor();
            _debug_printf_P(PSTR("Sensor_CCS811::_readSensor(): setEnvironmentalData(): BME680: humidity %u, temperature %.3f\n"), (uint8_t)data.humidity, data.temperature);
            _ccs811.setEnvironmentalData(data.humidity, data.temperature);
        }
#elif IOT_SENSOR_HAVE_LM75A
        if (sensor->getType() == LM75A) {
            auto temperature = reinterpret_cast<Sensor_BME680 *>(sensor)->_readSensor();
            _debug_printf_P(PSTR("Sensor_CCS811::_readSensor(): setEnvironmentalData(): LM75A: humidity 55, temperature %.3f\n"), temperature);
            _ccs811.setEnvironmentalData(55, temperature);
        }
#endif
    }
#endif

    bool available = _ccs811.available() && (_ccs811.readData() == 0);
    if (!available) {
        _debug_printf_P(PSTR("Sensor_CCS811::_readSensor(): address 0x%02x: available=0\n"), _address, available);
        return _sensor;
    }

    _sensor.available = available;

    if (_sensor.available) {
        _sensor.eCO2 = _ccs811.geteCO2();
        _sensor.TVOC = _ccs811.getTVOC();
        if (!_sensor.eCO2 || !_sensor.TVOC) {
            _sensor.available = false;
        }
        else {
            setUpdateRate(DEFAULT_UPDATE_RATE);
        }
    }
    else {
        _sensor.eCO2 = 0;
        _sensor.TVOC = 0;
    }

    _debug_printf_P(PSTR("Sensor_CCS811::_readSensor(): address 0x%02x: available=%u, eCO2 %uppm, TVOC %uppb, error=%u\n"),
        _address, _sensor.available, _sensor.eCO2, _sensor.TVOC, _ccs811.checkError()
    );

    return _sensor;
}

String Sensor_CCS811::_getId(const __FlashStringHelper *type)
{
    PrintString id(F("ccs811_0x%02x"), _address);
    if (type) {
        id.write('_');
        id.print(type);
    }
    return id;
}

#endif
