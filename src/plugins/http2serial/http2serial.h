/**
 * Author: sascha_lammers@gmx.de
 */

#if HTTP2SERIAL

#pragma once

#include <Arduino_compat.h>
#include <Buffer.h>
#include "ws_console_client.h"
#include "serial_handler.h"

#ifndef DEBUG_HTTP2SERIAL
#define DEBUG_HTTP2SERIAL 1
#endif

class Http2Serial {
public:
    Http2Serial();
    virtual ~Http2Serial();

    void broadcast(const Buffer &buffer) {
        broadcast(buffer.get(), buffer.length());
    }
    void broadcast(const String &buffer) {
        broadcast(buffer.c_str(), buffer.length());
    }
    void broadcast(const char *message, size_t len) {
        broadcast(nullptr, (const uint8_t *)message, len);
    }
    void broadcast(const uint8_t *message, size_t len) {
        broadcast(nullptr, message, len);
    }
    void broadcast(WsConsoleClient *sender, const uint8_t *message, size_t len);
    void broadcastOutputBuffer();

    void writeOutputBuffer(const String &buffer) {
        writeOutputBuffer(buffer.c_str(), buffer.length());
    }
    void writeOutputBuffer(const char *buffer, size_t len) {
        writeOutputBuffer((const uint8_t *)buffer, len);
    }
    void writeOutputBuffer(const uint8_t *buffer, size_t len);

    bool isTimeToSend();
    void resetOutputBufferTimer();
    void clearOutputBuffer();
    void outputLoop();
    static void onData(uint8_t type, const uint8_t *buffer, size_t len);

    SerialHandler *getSerialHandler();

public:
    static Http2Serial *_instance;

private:
    bool _outputBufferEnabled;
    unsigned long _outputBufferFlushDelay;
    Buffer _outputBuffer;
    SerialHandler *_serialHandler;
    SerialWrapper *_serialWrapper;

};

#endif