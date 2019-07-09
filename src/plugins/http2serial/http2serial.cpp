/*
 * Author: sascha_lammers@gmx.de
 */

#if HTTP2SERIAL

// Allows to connect over web sockets to the serial port, similar to Serial2TCP

#include <Arduino_compat.h>
#include <ESPAsyncWebServer.h>
#include <Buffer.h>
#include <LoopFunctions.h>
#include "http2serial.h"
#include "kfc_fw_config.h"
#include "serial_handler.h"
#include "web_server.h"
#include "web_socket.h"
#include "plugins.h"

//TODO lots of old code here, refactor

// TODO http//s://terminal.jcubic.pl/

#if DEBUG_HTTP2SERIAL
#include "debug_local_def.h"
#else
#include "debug_local_undef.h"
#endif

Http2Serial *Http2Serial::_instance = nullptr;

Http2Serial::Http2Serial() {
#ifdef HTTP2SERIAL_BAUD
    if_debug_printf_P(PSTR("Reconfiguring serial port to %d baud\n"), HTTP2SERIAL_BAUD);
    Serial.flush();
    Serial.begin(HTTP2SERIAL_BAUD);
#endif
    _serialWrapper = &serialHandler.getWrapper();
    _serialHandler = &serialHandler;
#if AT_MODE_SUPPORTED && HTTP2SERIAL_DISABLE_AT_MODE
    disable_at_mode();
#endif
    resetOutputBufferTimer();
    _outputBufferEnabled = true;

#if HTTP2SERIAL_SERIAL_HANDLER
    LoopFunctions::add(Http2Serial::inputLoop);
#endif
    LoopFunctions::add([this]() {
        this->outputLoop();
    }, reinterpret_cast<LoopFunctions::CallbackPtr_t>(this));
    _serialHandler->addHandler(onData, SerialHandler::RECEIVE|SerialHandler::LOCAL_TX);
}


Http2Serial::~Http2Serial() {
#if HTTP2SERIAL_SERIAL_HANDLER
    LoopFunctions::remove(Http2Serial::inputLoop);
#endif
    LoopFunctions::remove(reinterpret_cast<LoopFunctions::CallbackPtr_t>(this));
    _serialHandler->removeHandler(onData);
#ifdef HTTP2SERIAL_BAUD
    Serial.flush();
    Serial.begin(115200);
#endif
#if AT_MODE_SUPPORTED && HTTP2SERIAL_DISABLE_AT_MODE
    enable_at_mode();
#endif
}

void Http2Serial::broadcast(WsConsoleClient *sender, const uint8_t *message, size_t len) {

    if (WsClientManager::getWsClientCount(true)) {
        for(const auto &pair: WsClientManager::getWsClientManager()->getClients()) {
            if (pair.socket->status() == WS_CONNECTED && pair.wsClient != sender && pair.wsClient->isAuthenticated()) {
                pair.socket->text((uint8_t *)message, len);
            }
        }
    }
}

/**
 * The output buffer has a small time delay to combine multiple characters from the console into a single web socket message
 */
void Http2Serial::broadcastOutputBuffer() {

    if (_outputBuffer.length()) {
        broadcast(_outputBuffer);
        _outputBuffer.clear();
    }
    resetOutputBufferTimer();
}

void Http2Serial::writeOutputBuffer(const uint8_t *buffer, size_t len) {

    if (!_outputBufferEnabled) {
        return;
    }

    const uint8_t *ptr = buffer;
    for(;;) {
        int space = SERIAL_BUFFER_MAX_LEN - _outputBuffer.length();
        if (space > 0) {
            if (space >= (int)len) {
                space = len;
            }
            _outputBuffer.write(ptr, space);
            ptr += space;
            len -= space;
        }
        if (len == 0) {
            break;
        }
        broadcastOutputBuffer();
        resetOutputBufferTimer();
    }

    if (millis() > _outputBufferFlushDelay || !_outputBufferFlushDelay) {
        broadcastOutputBuffer();
        resetOutputBufferTimer();
    }
}

bool Http2Serial::isTimeToSend() {
    return (_outputBuffer.length() != 0) && (millis() > _outputBufferFlushDelay);
}

void Http2Serial::resetOutputBufferTimer() {
    _outputBufferFlushDelay = millis() + SERIAL_BUFFER_FLUSH_DELAY;
}

void Http2Serial::clearOutputBuffer() {
    _outputBuffer.clear();
    resetOutputBufferTimer();
}

void Http2Serial::outputLoop() {
    if (isTimeToSend()) {
        broadcastOutputBuffer();
    }
    auto *handler = getSerialHandler();
    if (handler != &serialHandler) {
        handler->serialLoop();
    }
}

void Http2Serial::onData(uint8_t type, const uint8_t *buffer, size_t len) {
    if (Http2Serial::_instance) {
        static bool locked = false;
        if (!locked) {
            locked = true;
            Http2Serial::_instance->writeOutputBuffer(buffer, len);  // store data before sending
            locked = false;
        }
    }
}

SerialHandler *Http2Serial::getSerialHandler() {
    return _serialHandler;
}

void http2serial_install_web_server_hook() {
    auto server = get_web_server_object();
    if (server) {
        AsyncWebSocket *ws_console = new AsyncWebSocket("/serial_console");

        ws_console->onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
            WsClient::onWsEvent(server, client, type, data, len, arg, WsConsoleClient::getInstance);
        });
        server->addHandler(ws_console);
        if_debug_printf_P(PSTR("Web socket for http2serial running on port %hu\n"), config.get<uint16_t>(_H(Config().http_port)));
    }
}

void http2serial_setup() {
    auto plugin = get_plugin_by_name(F("remote"));
    if (plugin) {
        static auto prev_callback = plugin->reconfigurePlugin;
        plugin->reconfigurePlugin = [] {
            // call previous reconfigure function and install web server hook again
            prev_callback();
            http2serial_install_web_server_hook();
        };
    }

    http2serial_install_web_server_hook();
}

bool http2_serial_at_mode_command_handler(Stream &serial, const String &command, int8_t argc, char **argv) {

    if (command.length() == 0) {
        serial.print(F(
            " AT+H2SBD <baud>\n"
            "    Set serial port rate\n"
        ));
    } else if (command.equalsIgnoreCase(F("H2SBD"))) {
        if (argc == 1) {
            uint32_t rate = atoi(argv[0]);
            if (rate) {
                Serial.flush();
                Serial.begin(rate);
                serial.printf_P(PSTR("Set serial rate to %d\n"), (unsigned)rate);
            }
            return true;
        }
    }
    return false;
}

void add_plugin_http2serial() {
    Plugin_t plugin;

    init_plugin(F("http2serial"), plugin, false, false, 200);

    plugin.setupPlugin = http2serial_setup;
#if AT_MODE_SUPPORTED
    plugin.atModeCommandHandler = http2_serial_at_mode_command_handler;
#endif
    register_plugin(plugin);
}

#endif
