/**
 * Author: sascha_lammers@gmx.de
 */

#if SYSLOG

#include <Arduino_compat.h>
#include <Buffer.h>
#include <KFCSyslog.h>
#include <PrintHtmlEntitiesString.h>
#include <LoopFunctions.h>
#include "kfc_fw_config.h"
#include "progmem_data.h"
#include "../include/templates.h"
#include "plugins.h"

#if defined(ESP32)
#define SYSLOG_PLUGIN_QUEUE_SIZE        4096
#elif defined(ESP8266)
#define SYSLOG_PLUGIN_QUEUE_SIZE        512
#endif

void syslog_process_queue();

#if DEBUG_USE_SYSLOG

SyslogStream *debugSyslog = nullptr;

void syslog_setup_debug_logger() {

    SyslogParameter parameter;
    parameter.setHostname(config._H_STR(Config().device_name));
    parameter.setAppName(FSPGM(kfcfw));
    parameter.setProcessId(F("DEBUG"));
	parameter.setSeverity(SYSLOG_DEBUG);

    SyslogFilter *filter = _debug_new SyslogFilter(parameter);
    filter->addFilter(F("*.*"), F(DEBUG_USE_SYSLOG_TARGET));

	SyslogQueue *queue = _debug_new SyslogMemoryQueue(SYSLOG_PLUGIN_QUEUE_SIZE * 4);
    debugSyslog = _debug_new SyslogStream(filter, queue);

    DEBUG_OUTPUT = *debugSyslog;

    debug_printf_P(PSTR("Debug Syslog enabled, target " DEBUG_USE_SYSLOG_TARGET "\n"));

    LoopFunctions::add(syslog_process_queue);
}

#endif

SyslogStream *syslog = nullptr;

void syslog_setup_logger() {

    if (syslog) {
        _logger.setSyslog(nullptr);
        delete syslog;
        syslog = nullptr;
#if DEBUG_USE_SYSLOG == 0
        LoopFunctions::remove(syslog_process_queue);
#endif
    }

    if (config._H_GET(Config().flags).syslogProtocol != SYSLOG_PROTOCOL_NONE) {

        SyslogParameter parameter;
        parameter.setHostname(config.getString(_H(Config().device_name)));
        parameter.setAppName(FSPGM(kfcfw));
        parameter.setFacility(SYSLOG_FACILITY_KERN);
        parameter.setSeverity(SYSLOG_NOTICE);

        SyslogFilter *filter = _debug_new SyslogFilter(parameter);
        filter->addFilter(F("*.*"), SyslogFactory::create(filter->getParameter(), (SyslogProtocol)config._H_GET(Config().flags).syslogProtocol, config._H_STR(Config().syslog_host), config._H_GET(Config().syslog_port)));

#if DEBUG_USE_SYSLOG
        SyslogQueue *queue = debugSyslog ? debugSyslog->getQueue() : _debug_new SyslogMemoryQueue(SYSLOG_PLUGIN_QUEUE_SIZE);
#else
        SyslogQueue *queue = _debug_new SyslogMemoryQueue(SYSLOG_PLUGIN_QUEUE_SIZE);
#endif
        syslog = _debug_new SyslogStream(filter, queue);

        _logger.setSyslog(syslog);
        LoopFunctions::add(syslog_process_queue);
    }
}

void syslog_setup() {
#if DEBUG_USE_SYSLOG
    syslog_setup_debug_logger();
#endif
    syslog_setup_logger();
}

void syslog_reconfigure(PGM_P source) {
    syslog_setup();
}

void syslog_process_queue() {
    static MillisTimer timer(1000UL);
    if (timer.reached()) {
#if DEBUG_USE_SYSLOG
        if (debugSyslog) {
            if (debugSyslog->hasQueuedMessages()) {
                debugSyslog->deliverQueue();
            }
        }
#endif
        if (syslog) {
            if (syslog->hasQueuedMessages()) {
                syslog->deliverQueue();
            }
        }
        timer.restart();
    }
}

const String syslog_get_status() {
#if SYSLOG
    PrintHtmlEntitiesString out;
    switch(config._H_GET(Config().flags).syslogProtocol) {
        case SYSLOG_PROTOCOL_UDP:
            out.printf_P(PSTR("UDP @ %s:%u"), config._H_STR(Config().syslog_host), config._H_GET(Config().syslog_port));
            break;
        case SYSLOG_PROTOCOL_TCP:
            out.printf_P(PSTR("TCP @ %s:%u"), config._H_STR(Config().syslog_host), config._H_GET(Config().syslog_port));
            break;
        case SYSLOG_PROTOCOL_TCP_TLS:
            out.printf_P(PSTR("TCP TLS @ %s:%u"), config._H_STR(Config().syslog_host), config._H_GET(Config().syslog_port));
            break;
        default:
            out += FSPGM(Disabled);
            break;
    }
    // #if SYSLOG_SPIFF_QUEUE_SIZE
    // if (!_Config.getOptions().isSyslogProtocol(SYSLOG_PROTOCOL_NONE) && syslogQueue && syslogQueue->hasSyslog()) {
    //     SyslogQueueInfo *info = syslogQueue->getQueueInfo();
    //     size_t currentSize = /*Syslog::*/syslogQueue->getSize();
    //     out.printf_P(PSTR(HTML_S(br) "Queue buffer on SPIFF %s/%s (%.2f%%)"), formatBytes(currentSize).c_str(), formatBytes(SYSLOG_SPIFF_QUEUE_SIZE).c_str(), currentSize * 100.0 / (float)SYSLOG_SPIFF_QUEUE_SIZE);
    //     if (info->transmitted || info->dropped) {
    //         out.printf_P(PSTR(HTML_S(br) "Transmitted %u, dropped %u"), info->transmitted, info->dropped);
    //     }
    // } else {
    //     return F("Syslog disabled");
    // }
    // #endif
#if DEBUG_USE_SYSLOG
    out.print(F(HTML_S(br) "Debugging enabled, target " DEBUG_USE_SYSLOG_TARGET));
#endif
    return out;
#else
    return FSPGM(Not_supported);
#endif
}

void syslog_create_settings_form(AsyncWebServerRequest *request, Form &form) {

    form.add<uint8_t>(F("syslog_enabled"), _H_STRUCT_FORMVALUE(Config().flags, uint8_t, syslogProtocol));
    form.addValidator(new FormRangeValidator(SYSLOG_PROTOCOL_NONE, SYSLOG_PROTOCOL_FILE));

    form.add<sizeof Config().syslog_host>(F("syslog_host"), config._H_W_STR(Config().syslog_host));
    form.addValidator(new FormValidHostOrIpValidator(true));

    form.add<uint16_t>(F("syslog_port"), &config._H_W_GET(Config().syslog_port));
    form.addValidator(new FormTCallbackValidator<uint16_t>([](uint16_t value, FormField &field) {
        if (value == 0) {
            value = 514;
            field.setValue(String(value));
        }
        return true;
    }));
    form.addValidator(new FormRangeValidator(F("Invalid port"), 1, 65535));

    form.finalize();
}

#if AT_MODE_SUPPORTED

#include "at_mode.h"

PROGMEM_AT_MODE_HELP_COMMAND_DEF_PNPN(SQC, "SQC", "Clear syslog queue");
PROGMEM_AT_MODE_HELP_COMMAND_DEF_PNPN(SQI, "SQI", "Display syslog queue info");
PROGMEM_AT_MODE_HELP_COMMAND_DEF_PNPN(SQD, "SQD", "Display syslog queue");

bool syslog_at_mode_command_handler(Stream &serial, const String &command, int8_t argc, char **argv) {

    if (command.length() == 0) {

        at_mode_add_help(PROGMEM_AT_MODE_HELP_COMMAND_T(SQC));
        at_mode_add_help(PROGMEM_AT_MODE_HELP_COMMAND_T(SQI));
        at_mode_add_help(PROGMEM_AT_MODE_HELP_COMMAND_T(SQD));

    } else if (constexpr_String_equalsIgnoreCase(command, PROGMEM_AT_MODE_HELP_COMMAND(SQC))) {
        if (syslog) {
            syslog->getQueue()->cleanUp();
            serial.println(F("Syslog queue cleared"));
        }
        return true;
    } else if (constexpr_String_equalsIgnoreCase(command, PROGMEM_AT_MODE_HELP_COMMAND(SQI))) {
        if (syslog) {
            serial.printf_P(PSTR("Syslog queue size: %d\n"), syslog->getQueue()->size());
        }
        return true;
    } else if (constexpr_String_equalsIgnoreCase(command, PROGMEM_AT_MODE_HELP_COMMAND(SQD))) {
        if (syslog) {
            auto queue = syslog->getQueue();
            size_t index = 0;
            while(index < queue->size()) {
                auto &item = queue->at(index++);
                if (item) {
                    serial.printf_P(PSTR("Syslog queue id %d (failures %d): %s\n"), item->getId(), item->getFailureCount(), item->getMessage().c_str());
                }
            }
        }
        return true;
    }
    return false;
}

#endif

void syslog_prepare_deep_sleep(uint32_t time, RFMode mode) {

    //TODO the send timeout could be reduces for short sleep intervals

    if (WiFi.isConnected()) {
#if DEBUG_USE_SYSLOG
        if (debugSyslog) {
            ulong timeout = millis() + 2000;    // long timeout in debug mode
            while(debugSyslog->hasQueuedMessages() && millis() < timeout) {
                debugSyslog->deliverQueue();
                delay(1);
            }
        }
#endif
        if (syslog) {
            ulong timeout = millis() + 150;
            while(syslog->hasQueuedMessages() && millis() < timeout) {
                syslog->deliverQueue();
                delay(1);
            }
        }
    }
}

PROGMEM_PLUGIN_CONFIG_DEF(
/* pluginName               */ syslog,
/* setupPriority            */ PLUGIN_PRIO_SYSLOG,
/* allowSafeMode            */ false,
/* autoSetupWakeUp          */ true,
/* rtcMemoryId              */ 0,
/* setupPlugin              */ syslog_setup,
/* statusTemplate           */ syslog_get_status,
/* configureForm            */ syslog_create_settings_form,
/* reconfigurePlugin        */ syslog_reconfigure,
/* reconfigure Dependencies */ nullptr,
/* prepareDeepSleep         */ syslog_prepare_deep_sleep,
/* atModeCommandHandler     */ syslog_at_mode_command_handler
);

#endif
