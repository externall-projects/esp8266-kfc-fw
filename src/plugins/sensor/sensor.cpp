/**
 * Author: sascha_lammers@gmx.de
 */

#if IOT_SENSOR

#include "sensor.h"
#include <PrintHtmlEntitiesString.h>
#include <KFCJson.h>
#include "WebUISocket.h"
#include "Sensor_LM75A.h"
#include "Sensor_BME280.h"
#include "Sensor_BME680.h"
#include "Sensor_CCS811.h"
#include "Sensor_HLW80xx.h"
#include "Sensor_HLW8012.h"
#include "Sensor_HLW8032.h"
#include "Sensor_Battery.h"

#if DEBUG_IOT_SENSOR
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

void SensorPlugin::getValues(JsonArray &array) {
    _debug_printf_P(PSTR("SensorPlugin::getValues()\n"));
    for(auto sensor: _sensors) {
        sensor->getValues(array);
    }
}

void SensorPlugin::setValue(const String &id, const String &value, bool hasValue, bool state, bool hasState) {
    _debug_printf_P(PSTR("SensorPlugin::setValue(%s)\n"), id.c_str());
}


static SensorPlugin plugin;

PGM_P SensorPlugin::getName() const {
    return PSTR("sensor");
}

void SensorPlugin::setup(PluginSetupMode_t mode) {
    _timer = Scheduler.addTimer(1e4, true, SensorPlugin::timerEvent);
#if IOT_SENSOR_HAVE_LM75A
    _sensors.push_back(new Sensor_LM75A(F("LM75A Temperature"), config.initTwoWire(), IOT_SENSOR_HAVE_LM75A));
#endif
#if IOT_SENSOR_HAVE_BME280
    _sensors.push_back(new Sensor_BME280(F("BME280"), config.initTwoWire(), IOT_SENSOR_HAVE_BME280));
#endif
#if IOT_SENSOR_HAVE_BME680
    _sensors.push_back(new Sensor_BME680(F("BME680"), IOT_SENSOR_HAVE_BME680));
#endif
#if IOT_SENSOR_HAVE_CCS811
    _sensors.push_back(new Sensor_CCS811(F("CCS811"), IOT_SENSOR_HAVE_CCS811));
#endif
#if IOT_SENSOR_HAVE_HLW8012
    _sensors.push_back(new Sensor_HLW8012(F("HLW8012"), IOT_SENSOR_HLW8012_SEL, IOT_SENSOR_HLW8012_CF, IOT_SENSOR_HLW8012_CF1));
#endif
#if IOT_SENSOR_HAVE_HLW8032
    _sensors.push_back(new Sensor_HLW8032(F("HLW8032"), IOT_SENSOR_HLW8032_RX, IOT_SENSOR_HLW8032_TX, IOT_SENSOR_HLW8032_PF));
#endif
#if IOT_SENSOR_HAVE_BATTERY
    _sensors.push_back(new Sensor_Battery(F("Battery")));
#endif

}

SensorPlugin::SensorVector &SensorPlugin::getSensors() {
    return plugin._sensors;
}

size_t SensorPlugin::getSensorCount() {
    return plugin._sensors.size();
}


void SensorPlugin::timerEvent(EventScheduler::TimerPtr timer) {
    plugin._timerEvent();
}

void SensorPlugin::_timerEvent() {
    JsonUnnamedObject json(2);
    json.add(JJ(type), JJ(ue));
    auto &events = json.addArray(JJ(events));
    for(auto sensor: _sensors) {
        sensor->timerEvent(events);
    }
    if (events.size()) {
        WsWebUISocket::broadcast(WsWebUISocket::getSender(), json);
    }
}

void SensorPlugin::reconfigure(PGM_P source) {
}

bool SensorPlugin::hasWebUI() const {
    return true;
}

WebUIInterface *SensorPlugin::getWebUIInterface() {
    return this;
}

void SensorPlugin::createWebUI(WebUI &webUI) {
    auto row = &webUI.addRow();
    row->setExtraClass(JJ(title));
    row->addGroup(F("Sensors"), false);

    if (_sensors.size()) {
        row = &webUI.addRow();
    }

    for(auto sensor: _sensors) {
        sensor->createWebUI(webUI, &row);
    }
}


bool SensorPlugin::hasStatus() const {
    return _sensors.size() != 0;
}

const String SensorPlugin::getStatus() {
    _debug_printf_P(PSTR("SensorPlugin::getStatus(): sensor count %d\n"), _sensors.size());
    PrintHtmlEntitiesString str;
    str.print(F("Sensors:" HTML_S(br)));
    for(auto sensor: _sensors) {
        sensor->getStatus(str);
    }
    return str;
}

#endif
