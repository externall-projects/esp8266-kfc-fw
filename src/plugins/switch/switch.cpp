/**
 * Author: sascha_lammers@gmx.de
 */

#include "switch.h"
#include <LoopFunctions.h>
#include <PrintHtmlEntities.h>
#include <KFCForms.h>
#include <ESPAsyncWebServer.h>
#include <EventTimer.h>
#include "PluginComponent.h"
#include "plugins.h"
#include "progmem_data.h"

#if DEBUG_IOT_SWITCH
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

PROGMEM_STRING_DEF(mqtt_switch_state, "/switch/state");
PROGMEM_STRING_DEF(mqtt_switch_set, "/switch/set");
PROGMEM_STRING_DEF(iot_switch_states_file, "/switch.states");

SwitchPlugin plugin;

SwitchPlugin::SwitchPlugin() : MQTTComponent(ComponentTypeEnum_t::SWITCH), _states(0), _pins({IOT_SWITCH_CHANNEL_PINS})
{
    REGISTER_PLUGIN(this);
}

void SwitchPlugin::setup(PluginSetupMode_t mode)
{
    _readConfig();
    _readStates();
    for (size_t i = 0; i < _pins.size(); i++) {
        if (_configs[i].state == SwitchStateEnum::RESTORE) {
            _setChannel(i, _states & (1 << i));
        }
        else {
            _setChannel(i, _configs[i].state);
        }
        pinMode(_pins[i], OUTPUT);
    }
    auto mqttClient = MQTTClient::getClient();
    if (mqttClient) {
        mqttClient->registerComponent(this);
    }
#if IOT_SWITCH_PUBLISH_MQTT_INTERVAL
    _updateTimer.add(IOT_SWITCH_PUBLISH_MQTT_INTERVAL, true, [this](EventScheduler::TimerPtr) {
        _publishState(MQTTClient::getClient());
    });
#endif
}

void SwitchPlugin::restart()
{
#if IOT_SWITCH_PUBLISH_MQTT_INTERVAL
    _updateTimer.remove();
#endif
#if IOT_SWITCH_STORE_STATES
    if (_delayedWrite.active()) {
        _delayedWrite->detach(); // stop timer and store now
        _delayedWrite->getCallback()(nullptr);
    }
#endif
}

void SwitchPlugin::reconfigure(PGM_P source)
{
    _readConfig();
}

void SwitchPlugin::getStatus(Print &output)
{
    output.printf_P(PSTR("%u channel switch"), _pins.size());
    for (size_t i = 0; i < _pins.size(); i++) {
        output.printf_P(PSTR(HTML_S(br) "%s - %s"), _getChannelName(i).c_str(), _getChannel(i) ? SPGM(On) : SPGM(Off));
    }
}

void SwitchPlugin::createConfigureForm(AsyncWebServerRequest *request, Form &form)
{
    using KFCConfigurationClasses::Plugins;

    form.setFormUI(F("Switch Configuration"));

    FormUI::ItemsList states;
    states.emplace_back(String(SwitchStateEnum::OFF), FSPGM(Off));
    states.emplace_back(String(SwitchStateEnum::ON), FSPGM(On));
    states.emplace_back(String(SwitchStateEnum::RESTORE), F("Restore Last State"));

    for (size_t i = 0; i < _pins.size(); i++) {

        FormGroup *group = nullptr;
        if (_pins.size() > 1) {
            group = &form.addGroup(PrintString(F("channel_%u"), i), PrintString(F("Channel %u"), i), true);
        }

        form.add(PrintString(F("name[%u]"), i), _names[i], [this, i](const String &name, FormField &, bool) {
            _names[i] = name;
            return false;
        }, FormField::InputFieldType::TEXT)->setFormUI(new FormUI(FormUI::TEXT, F("Name")));

        form.add<SwitchStateEnum>(PrintString(F("state[%u]"), i), _configs[i].state, [this, i](SwitchStateEnum state, FormField &, bool) {
            _configs[i].state = state;
            return false;
        }, FormField::InputFieldType::SELECT)->setFormUI((new FormUI(FormUI::SELECT, F("Default State")))->addItems(states));

        if (group) {
            group->end();
        }
    }

    form.setValidateCallback([this](Form &form) {
        if (form.isValid()) {
            _debug_println(F("Storing config"));
            Plugins::IOTSwitch::setConfig(_names, _configs);
            return true;
        }
        return false;

    });

    form.finalize();
}

void SwitchPlugin::createAutoDiscovery(MQTTAutoDiscovery::Format_t format, MQTTAutoDiscoveryVector &vector)
{
    for (size_t i = 0; i < _pins.size(); i++) {
        auto discovery = new MQTTAutoDiscovery();
        discovery->create(this, i, format);
        discovery->addStateTopic(MQTTClient::formatTopic(i, FSPGM(mqtt_switch_state)));
        discovery->addCommandTopic(MQTTClient::formatTopic(i, FSPGM(mqtt_switch_set)));
        discovery->addPayloadOn(FSPGM(1));
        discovery->addPayloadOff(FSPGM(0));
        discovery->finalize();
        vector.emplace_back(discovery);
    }
}

uint8_t SwitchPlugin::getAutoDiscoveryCount() const
{
    return _pins.size();
}

void SwitchPlugin::onConnect(MQTTClient *client)
{
    _debug_println();
    MQTTComponent::onConnect(client);
    auto qos = client->getDefaultQos();
    for (size_t i = 0; i < _pins.size(); i++) {
        _debug_printf_P(PSTR("subscribe=%s\n"), MQTTClient::formatTopic(i, FSPGM(mqtt_switch_set)).c_str());
        client->subscribe(this, MQTTClient::formatTopic(i, FSPGM(mqtt_switch_set)), qos);
    }
    _publishState(client);
}

void SwitchPlugin::onMessage(MQTTClient *client, char *topic, char *payload, size_t len)
{
    _debug_printf_P(PSTR("topic=%s payload=%s\n"), topic, payload);
    for (size_t i = 0; i < _pins.size(); i++) {
        if (MQTTClient::formatTopic(i, FSPGM(mqtt_switch_set)).equals(topic)) {
            bool state = atoi(payload);
            _setChannel(i, state);
            _publishState(client, i);
        }
    }
}

void SwitchPlugin::_setChannel(uint8_t channel, bool state)
{
    _debug_printf_P(PSTR("channel=%u state=%u\n"), channel, state);
    digitalWrite(_pins[channel], state ? IOT_SWITCH_ON_STATE : !IOT_SWITCH_ON_STATE);
    if (state) {
        _states |= (1 << channel);
    }
    else {
        _states &= ~(1 << channel);
    }
    _writeStates();
}

bool SwitchPlugin::_getChannel(uint8_t channel) const
{
    return (digitalRead(_pins[channel]) == IOT_SWITCH_ON_STATE);
}

String SwitchPlugin::_getChannelName(uint8_t channel) const
{
    return _names[channel].length() ? _names[channel] : PrintString(F("Channel %u"), channel);
}

void SwitchPlugin::_readConfig()
{
    KFCConfigurationClasses::Plugins::IOTSwitch::getConfig(_names, _configs);
}

void SwitchPlugin::_readStates()
{
#if IOT_SWITCH_STORE_STATES
    File file = SPIFFS.open(FSPGM(iot_switch_states_file), fs::FileOpenMode::read);
    if (file) {
        if (file.read(reinterpret_cast<uint8_t *>(&_states), sizeof(_states)) != sizeof(_states)) {
            _states = 0;
        }
    }
#endif
    _debug_printf_P(PSTR("states=%s\n"), String(_states, 2).c_str());
}

void SwitchPlugin::_writeStates()
{
    _debug_printf_P(PSTR("states=%s\n"), String(_states, 2).c_str());
#if IOT_SWITCH_STORE_STATES
    _delayedWrite.remove();
    _delayedWrite.add(IOT_SWITCH_STORE_STATES_WRITE_DELAY, false, [this](EventScheduler::TimerPtr) {
        _debug_printf_P(PSTR("SwitchPlugin::_writeStates(): delayed write states=%s\n"), String(_states, 2).c_str());
        File file = SPIFFS.open(FSPGM(iot_switch_states_file), fs::FileOpenMode::write);
        if (file) {
            file.write(reinterpret_cast<const uint8_t *>(&_states), sizeof(_states));
        }
    });
#endif
}

void SwitchPlugin::_publishState(MQTTClient *client, int8_t channel)
{
    if (client) {
        auto qos = client->getDefaultQos();
        for (size_t i = 0; i < _pins.size(); i++) {
            if (channel == -1 || (uint8_t)channel == i) {
                _debug_printf_P(PSTR("pin=%u state=%u\n"), _pins[i], _getChannel(i));
                client->publish(MQTTClient::formatTopic(i, FSPGM(mqtt_switch_state)), qos, true, _getChannel(i) ? FSPGM(1) : FSPGM(0));
            }
        }
    }
}
