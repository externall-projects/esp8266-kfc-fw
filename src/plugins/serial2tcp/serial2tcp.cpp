/**
 * Author: sascha_lammers@gmx.de
 */

#if SERIAL2TCP

#include "serial2tcp.h"
#include "plugins.h"
#include "Serial2TcpBase.h"
// #include <LoopFunctions.h>
// #include <PrintHtmlEntitiesString.h>
// #include "kfc_fw_config.h"
// #include "status.h"
#include "at_mode.h"
#include "progmem_data.h"
// #include "misc.h"
// #if IOT_DIMMER_MODULE
// #include "plugins/dimmer_module/dimmer_module.h"
// #endif

#if DEBUG_SERIAL2TCP
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

// Plugin

class Serial2TcpPlugin : public PluginComponent {
public:
    Serial2TcpPlugin();
    virtual PGM_P getName() const;
    virtual PluginPriorityEnum_t getSetupPriority() const override;
    virtual void setup(PluginSetupMode_t mode) override;
    virtual void reconfigure(PGM_P source) override;
    virtual bool hasStatus() const override;
    virtual const String getStatus() override;
    virtual bool canHandleForm(const String &formName) const override;
    virtual void createConfigureForm(AsyncWebServerRequest *request, Form &form) override;
    virtual bool hasAtMode() const override;
    virtual void atModeHelpGenerator() override;
    virtual bool atModeHandler(Stream &serial, const String &command, int8_t argc, char **argv) override;
};

static Serial2TcpPlugin plugin;

Serial2TcpPlugin::Serial2TcpPlugin() {
    register_plugin(this);
}

PGM_P Serial2TcpPlugin::getName() const {
    return PSTR("serial2tcp");
}

Serial2TcpPlugin::PluginPriorityEnum_t Serial2TcpPlugin::getSetupPriority() const {
    return PluginPriorityEnum_t::MAX_PRIORITY;
}

void Serial2TcpPlugin::setup(PluginSetupMode_t mode) {
    auto instance = Serial2TcpBase::createInstance();
    if (instance) {
        instance->begin();
    }
}

void Serial2TcpPlugin::reconfigure(PGM_P source) {
    setup(PLUGIN_SETUP_DEFAULT);
}

bool Serial2TcpPlugin::hasStatus() const {
    return true;
}

const String Serial2TcpPlugin::getStatus() {

    PrintHtmlEntitiesString out;
    auto cfg = config._H_GET(Config().serial2tcp);
    auto flags = config._H_GET(Config().flags);
    if (flags.serial2TCPMode == SERIAL2TCP_MODE_DISABLED) {
        return(FSPGM(Disabled));
    }
    else {
        if (Serial2TcpBase::isServer(flags)) {
            out.print(F("Server Mode: "));
            out.printf_P(PSTR("Listening on port %u"), cfg.port);
        }
        else {
            out.print(F("Client Mode: "));
            out.printf_P(PSTR("Connecting to %s:%u, Auto connect " HTML_S(strong) "%s" HTML_E(strong) ", Auto reconnect " HTML_S(strong) "%s" HTML_E(strong)),
                cfg.host,
                cfg.port,
                cfg.auto_connect ? SPGM(enabled) : SPGM(disabled),
                cfg.auto_reconnect ? SPGM(enabled) : SPGM(disabled)
            );
        }
        if (Serial2TcpBase::isTLS(flags)) {
            out.print(F(", TLS enabled"));
        }
        auto instance = Serial2TcpBase::getInstance();
        if (instance) {
            instance->getStatus(out);
        }
    }
    return out;
}

bool Serial2TcpPlugin::canHandleForm(const String &formName) const {
    return nameEquals(formName);
}

void Serial2TcpPlugin::createConfigureForm(AsyncWebServerRequest *request, Form &form) {
    //TODO
}

#if AT_MODE_SUPPORTED

#include "at_mode.h"

#if IOT_DIMMER_MODULE
PROGMEM_AT_MODE_HELP_COMMAND_DEF_PNPN(S2TCPF, "S2TCPF","Reset ATmega once on incoming data");
#endif
PROGMEM_AT_MODE_HELP_COMMAND_DEF_PPPN(S2TCPD, "S2TCPD", "<0/1>", "Enable or disable Serial2Tcp debug output");
PROGMEM_AT_MODE_HELP_COMMAND_DEF_PPPN(S2TCP, "S2TCP", "<0=disable/1=server/2=client>", "Enable or disable Serial2Tcp");

bool Serial2TcpPlugin::hasAtMode() const {
    return true;
}

void Serial2TcpPlugin::atModeHelpGenerator() {
    at_mode_add_help(PROGMEM_AT_MODE_HELP_COMMAND_T(S2TCPF));
    at_mode_add_help(PROGMEM_AT_MODE_HELP_COMMAND_T(S2TCPD));
    at_mode_add_help(PROGMEM_AT_MODE_HELP_COMMAND_T(S2TCP));
}

bool Serial2TcpPlugin::atModeHandler(Stream &serial, const String &command, int8_t argc, char **argv) {

    if (constexpr_String_equalsIgnoreCase(command, PROGMEM_AT_MODE_HELP_COMMAND(S2TCPD))) {
        if (argc == 1) {
            Serial2TcpBase::_debugOutput = atoi(argv[0]);
            serial.printf_P(PSTR("+S2TCPD: debug output %s\n"), Serial2TcpBase::_debugOutput ? PSTR("enabled") : PSTR("disabled"));
            return true;
        }
        else {
            at_mode_print_invalid_arguments(serial);
        }
    }
#if IOT_DIMMER_MODULE
    else if (constexpr_String_equalsIgnoreCase(command, PROGMEM_AT_MODE_HELP_COMMAND(S2TCPF))) {
        Serial2TcpBase::_resetAtmega = true;
        serial.println(F("+S2TCPF: reset enabled"));
        return true;
    }
#endif
    else if (constexpr_String_equalsIgnoreCase(command, PROGMEM_AT_MODE_HELP_COMMAND(S2TCP))) {
        if (argc == 1) {
            auto enable = atoi(argv[0]);
            if (enable) {
                if (enable == 2) {
                    serial.println(F("+S2TCP: Serial2TCP client with auto connect enabled"));
                    config._H_W_GET(Config().flags).serial2TCPMode = SERIAL2TCP_MODE_UNSECURE_SERVER;
                    config._H_W_GET(Config().serial2tcp).auto_connect = true;
                }
                else {
                    serial.println(F("+S2TCP: Serial2TCP server enabled"));
                    config._H_W_GET(Config().flags).serial2TCPMode = SERIAL2TCP_MODE_UNSECURE_SERVER;
                }

                auto instance = Serial2TcpBase::createInstance();
                if (instance) {
                    instance->begin();
                }
            }
            else {
                config._H_W_GET(Config().flags).serial2TCPMode = SERIAL2TCP_MODE_DISABLED;

                Serial2TcpBase::destroyInstance();
                serial.println(F("+S2TCP: Serial2TCP disabled"));
            }
            return true;
        }
    }
    return false;

}

#endif

#endif